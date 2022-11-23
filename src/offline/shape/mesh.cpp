#include <rad/offline/render/shape.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/utility/distribution.h>

using namespace Rad::Math;

namespace Rad {

MeshBase::MeshBase(BuildContext* ctx, const ConfigNode& cfg) {
  const Matrix4& toWorld = ctx->GetShapeMatrix();
  _toWorld = Transform(toWorld);
}

void MeshBase::SubmitToEmbree(RTCDevice device, RTCScene scene, UInt32 id) const {
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
      _triangleCount);
  std::copy(_position.get(), _position.get() + _vertexCount, vertices);
  std::copy(_indices.get(), _indices.get() + _indexCount, indices);
  rtcCommitGeometry(geo);
  rtcAttachGeometryByID(scene, geo, id);
  rtcReleaseGeometry(geo);
}

SurfaceInteraction MeshBase::ComputeInteraction(const Ray& ray, const HitShapeRecord& rec) {
  UInt32 face = rec.PrimitiveIndex * 3;
  UInt32 f0 = _indices[face + 0], f1 = _indices[face + 1], f2 = _indices[face + 2];
  Vector3 p0 = _position[f0].cast<Float>(), p1 = _position[f1].cast<Float>(), p2 = _position[f2].cast<Float>();
  Float t = rec.T;
  Vector2 primUV = rec.PrimitiveUV;
  Vector3 bary(1.f - primUV.x() - primUV.y(), primUV.x(), primUV.y());
  Vector3 dp0 = p1 - p0, dp1 = p2 - p0;
  SurfaceInteraction si{};
  si.P = p0 * bary.x() + (p1 * bary.y() + (p2 * bary.z()));
  si.T = t;
  si.N = (dp0.cross(dp1)).normalized();
  si.Shape = this;
  if (_uv == nullptr) {
    si.UV = primUV;
    std::tie(si.dPdU, si.dPdV) = CoordinateSystem(si.N);
  } else {
    Vector2 uv0 = _uv[f0].cast<Float>(), uv1 = _uv[f1].cast<Float>(), uv2 = _uv[f2].cast<Float>();
    si.UV = uv0 * bary.x() + (uv1 * bary.y() + (uv2 * bary.z()));
    Vector2 duv0 = uv1 - uv0, duv1 = uv2 - uv0;
    Float det = duv0.x() * duv1.y() - (duv0.y() * duv1.x());
    Float invDet = det == 0 ? 0 : Rcp(det);
    if (invDet == 0) {
      si.dPdU = Vector3::Zero();
      si.dPdV = Vector3::Zero();
    } else {
      si.dPdU = (duv1.y() * dp0 - (duv0.y() * dp1)) * invDet;
      si.dPdV = (-duv1.x() * dp0 + (duv0.x() * dp1)) * invDet;
    }
  }
  if (_normal == nullptr) {
    si.Shading.N = si.N;
  } else {
    Vector3 n0 = _normal[f0].cast<Float>(), n1 = _normal[f1].cast<Float>(), n2 = _normal[f2].cast<Float>();
    Vector3 shN = n0 * bary.x() + (n1 * bary.y() + (n2 * bary.z()));
    Float il = Rsqrt(shN.squaredNorm());
    shN *= il;
    si.Shading.N = shN;
    si.dNdU = (n1 - n0) * il;
    si.dNdV = (n2 - n0) * il;
    si.dNdU = -shN * shN.dot(si.dNdU) + si.dNdU;
    si.dNdV = -shN * shN.dot(si.dNdV) + si.dNdV;
  }
  return si;
}

PositionSampleResult MeshBase::SamplePosition(const Vector2& xi) const {
  Vector2 txi = xi;
  size_t index;
  std::tie(index, txi.y()) = _dist.SampleReuse(txi.y());
  UInt32 face = UInt32(index) * 3;
  UInt32 f0 = _indices[face + 0], f1 = _indices[face + 1], f2 = _indices[face + 2];
  Vector3 p0 = _position[f0].cast<Float>(), p1 = _position[f1].cast<Float>(), p2 = _position[f2].cast<Float>();
  Vector3 e0 = p1 - p0, e1 = p2 - p0;
  Vector2 b = Warp::SquareToUniformTriangle(txi);
  PositionSampleResult psr{};
  psr.P = e0 * b.x() + (e1 * b.y() + p0);
  psr.Pdf = _dist.Normalization();
  psr.IsDelta = false;
  if (_uv == nullptr) {
    psr.UV = b;
  } else {
    Vector2 uv0 = _uv[f0].cast<Float>(), uv1 = _uv[f1].cast<Float>(), uv2 = _uv[f2].cast<Float>();
    psr.UV = uv0 * (1 - b.x() - b.y()) + (uv1 * b.x() + (uv2 * b.y()));
  }
  if (_normal == nullptr) {
    psr.N = e0.cross(e1).normalized();
  } else {
    Vector3 n0 = _normal[f0].cast<Float>(), n1 = _normal[f1].cast<Float>(), n2 = _normal[f2].cast<Float>();
    psr.N = (n0 * (1 - b.x() - b.y()) + (n1 * b.x() + (n2 * b.y()))).normalized();
  }
  return psr;
}

Float MeshBase::PdfPosition(const PositionSampleResult& psr) const {
  return _dist.Normalization();
}

Float MeshBase::TriangleArea(const Vector3& p0, const Vector3& p1, const Vector3& p2) {
  return (p1 - p0).cross(p2 - p0).norm() * Float(0.5);
}

void MeshBase::UpdateDistibution() {
  std::vector<Float> areaData;
  areaData.reserve(_triangleCount);
  for (UInt32 i = 0; i < _triangleCount; i++) {
    Eigen::Vector3f p0 = _position[_indices[i * 3 + 0]];
    Eigen::Vector3f p1 = _position[_indices[i * 3 + 1]];
    Eigen::Vector3f p2 = _position[_indices[i * 3 + 2]];
    areaData.emplace_back(TriangleArea(p0.cast<Float>(), p1.cast<Float>(), p2.cast<Float>()));
  }
  _dist = DiscreteDistribution1D(areaData);
  _surfaceArea = _dist.Sum();
}

class Mesh final : public MeshBase {
 public:
  Mesh(BuildContext* ctx, const ConfigNode& cfg) : MeshBase(ctx, cfg) {
    std::string assetName = cfg.Read<std::string>("asset_name");
    ModelAsset* modelAsset = ctx->GetModel(assetName);

    Share<TriangleModel> model;
    {
      std::string submodelName;
      if (cfg.TryRead<std::string>("sub_model", submodelName)) {
        model = modelAsset->GetSubModel(submodelName);
      } else {
        model = modelAsset->FullModel();
      }
    }

    if (_toWorld.IsIdentity()) {
      _position = model->GetPosition();
      _normal = model->GetNormal();
    } else {
      _position = std::shared_ptr<Eigen::Vector3f[]>(new Eigen::Vector3f[model->VertexCount()]);
      std::shared_ptr<Eigen::Vector3f[]> p = model->GetPosition();
      for (size_t i = 0; i < model->VertexCount(); i++) {
        _position[i] = _toWorld.ApplyAffineToWorld(p[i].cast<Float>()).cast<Float32>();
      }
      if (model->HasNormal()) {
        _normal = std::shared_ptr<Eigen::Vector3f[]>(new Eigen::Vector3f[model->VertexCount()]);
        std::shared_ptr<Eigen::Vector3f[]> n = model->GetNormal();
        for (size_t i = 0; i < model->VertexCount(); i++) {
          _normal[i] = _toWorld.ApplyNormalToWorld(n[i].cast<Float>()).cast<Float32>();
        }
      }
    }
    _indices = model->GetIndices();
    _uv = model->GetUV();
    _vertexCount = model->VertexCount();
    _indexCount = model->IndexCount();
    _triangleCount = model->TriangleCount();

    UpdateDistibution();
  }
  ~Mesh() noexcept override = default;
};

}  // namespace Rad

RAD_FACTORY_SHAPE_DECLARATION(Mesh, "mesh");
