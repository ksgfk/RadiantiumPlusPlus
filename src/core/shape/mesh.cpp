#include <radiantium/shape.h>

#include <radiantium/model.h>
#include <radiantium/transform.h>
#include <radiantium/math_ext.h>
#include <radiantium/factory.h>
#include <radiantium/build_context.h>
#include <radiantium/config_node.h>
#include <memory>
#include <limits>
#include <stdexcept>

using namespace rad::math;

namespace rad {

class Mesh : public IShape {
 public:
  Mesh(const TriangleModel& model, const Transform& transform) {
    if (transform.IsIdentity()) {
      _position = model.GetPosition();
      _normal = model.GetNormal();
    } else {
      _position = std::shared_ptr<Eigen::Vector3f[]>(new Eigen::Vector3f[model.VertexCount()]);
      std::shared_ptr<Eigen::Vector3f[]> p = model.GetPosition();
      for (size_t i = 0; i < model.VertexCount(); i++) {
        _position[i] = transform.ApplyAffineToWorld(p[i]);
      }
      if (model.HasNormal()) {
        _normal = std::shared_ptr<Eigen::Vector3f[]>(new Eigen::Vector3f[model.VertexCount()]);
        std::shared_ptr<Eigen::Vector3f[]> n = model.GetNormal();
        for (size_t i = 0; i < model.VertexCount(); i++) {
          _normal[i] = transform.ApplyNormalToWorld(n[i]);
        }
      }
    }
    _indices = model.GetIndices();
    _uv = model.GetUV();
    _vertexCount = model.VertexCount();
    _indexCount = model.IndexCount();
    _triangleCount = model.TriangleCount();
  }

  ~Mesh() noexcept override = default;

  size_t PrimitiveCount() override { return _triangleCount; }

  void SubmitToEmbree(RTCDevice device, RTCScene scene, UInt32 id) override {
    RTCGeometry geo = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    auto vertices = (Eigen::Vector3f*)rtcSetNewGeometryBuffer(
        geo,
        RTC_BUFFER_TYPE_VERTEX,
        0,
        RTC_FORMAT_FLOAT3,
        3 * sizeof(float),
        _vertexCount);
    auto indices = (UInt32*)rtcSetNewGeometryBuffer(
        geo,
        RTC_BUFFER_TYPE_INDEX,
        0,
        RTC_FORMAT_UINT3,
        3 * sizeof(UInt32),
        _indexCount);
    std::copy(_position.get(), _position.get() + _vertexCount, vertices);
    std::copy(_indices.get(), _indices.get() + _indexCount, indices);
    rtcCommitGeometry(geo);
    rtcAttachGeometryByID(scene, geo, id);
    rtcReleaseGeometry(geo);
  }

  SurfaceInteraction ComputeInteraction(const Ray& ray, const HitShapeRecord& rec) override {
    UInt32 face = rec.PrimitiveIndex * 3;
    UInt32 f0 = _indices[face + 0], f1 = _indices[face + 1], f2 = _indices[face + 2];
    Vec3 p0 = _position[f0], p1 = _position[f1], p2 = _position[f2];
    Float t = rec.T;
    Vec2 primUV = rec.PrimitiveUV;
    Vec3 bary(1.f - primUV.x() - primUV.y(), primUV.x(), primUV.y());
    Vec3 dp0 = p1 - p0, dp1 = p2 - p0;
    SurfaceInteraction si;
    si.P = p0 * bary.x() + (p1 * bary.y() + (p2 * bary.z()));
    si.T = t;
    si.N = (dp0.cross(dp1)).normalized();
    si.PrimitiveIndex = rec.PrimitiveIndex;
    if (_uv == nullptr) {
      si.UV = primUV;
      std::tie(si.dPdU, si.dPdV) = CoordinateSystem(si.N);
    } else {
      Vec2 uv0 = _uv[f0], uv1 = _uv[f1], uv2 = _uv[f2];
      si.UV = uv0 * bary.x() + (uv1 * bary.y() + (uv2 * bary.z()));
      Vec2 duv0 = uv1 - uv0, duv1 = uv2 - uv0;
      Float det = duv0.x() * duv1.y() - (duv0.y() * duv1.x());
      Float invDet = det == 0 ? 0 : Rcp(det);
      if (invDet == 0) {
        si.dPdU = Vec3::Zero();
        si.dPdV = Vec3::Zero();
      } else {
        si.dPdU = (duv1.y() * dp0 - (duv0.y() * dp1)) * invDet;
        si.dPdV = (-duv1.x() * dp0 + (duv0.x() * dp1)) * invDet;
      }
    }
    if (_normal == nullptr) {
      si.Shading.N = si.N;
    } else {
      Vec3 n0 = _normal[f0], n1 = _normal[f1], n2 = _normal[f2];
      Vec3 shN = n0 * bary.x() + (n1 * bary.y() + (n2 * bary.z()));
      Float il = Rsqrt(shN.squaredNorm());
      shN *= il;
      si.Shading.N = shN;
      si.dNdU = (n1 - n0) * il;
      si.dNdV = (n2 - n0) * il;
      si.dNdU = -shN * shN.dot(si.dNdU) + si.dNdU;
      si.dNdV = -shN * shN.dot(si.dNdV) + si.dNdV;
    }
    si.Shape = this;
    return si;
  }

 private:
  std::shared_ptr<Eigen::Vector3f[]> _position;
  std::shared_ptr<Eigen::Vector3f[]> _normal;
  std::shared_ptr<Eigen::Vector2f[]> _uv;
  std::shared_ptr<UInt32[]> _indices;
  UInt32 _vertexCount;
  UInt32 _indexCount;
  UInt32 _triangleCount;
};
}  // namespace rad

namespace rad::factory {
class MeshShapeFactory : public IShapeFactory {
 public:
  ~MeshShapeFactory() noexcept override {}
  std::string UniqueId() const override { return "mesh"; }
  std::unique_ptr<IShape> Create(const BuildContext* context, const IConfigNode* config) const override {
    const Transform& toWorld = context->GetEntityCreateContext().ToWorld;
    std::string assetName = config->GetString("asset_name");
    IModelAsset* model = context->GetModel(assetName);
    std::unique_ptr<IShape> result;
    if (config->HasProperty("sub_model")) {
      std::string subModel = config->GetString("sub_model");
      auto sub = model->GetSubModel(subModel);
      result = std::make_unique<Mesh>(*sub, toWorld);
    } else {
      result = std::make_unique<Mesh>(*model->FullModel(), toWorld);
    }
    return result;
  }
};
std::unique_ptr<IFactory> CreateMeshFactory() { return std::make_unique<MeshShapeFactory>(); }
}  // namespace rad::factory
