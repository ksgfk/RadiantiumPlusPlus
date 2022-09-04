#include <radiantium/shape.h>

#include <radiantium/model.h>
#include <radiantium/transform.h>
#include <limits>
#include <stdexcept>

namespace rad {

class Mesh : public IShape {
 public:
  Mesh(const TriangleModel& model, const Transform& transform) {
    if (transform.IsIdentity()) {
      _pos = model.GetPositionPtr();
      _normal = model.GetNormalPtr();
      _needRelease = false;
    } else {
      _pos = new Eigen::Vector3f[model.VertexCount()];
      const std::vector<Eigen::Vector3f>& p = model.GetPosition();
      for (size_t i = 0; i < p.size(); i++) {
        _pos[i] = transform.ApplyAffineToWorld(p[i]);
      }
      _normal = new Eigen::Vector3f[model.VertexCount()];
      const std::vector<Eigen::Vector3f>& n = model.GetNormal();
      for (size_t i = 0; i < n.size(); i++) {
        _normal[i] = transform.ApplyNormalToWorld(n[i]);
      }
      _needRelease = true;
    }
    _indices = model.GetIndicePtr();
    _uv = model.GetUVPtr();
    if (model.VertexCount() >= std::numeric_limits<UInt32>::max()) {
      throw std::invalid_argument("toooo much vertex");
    }
    if (model.IndexCount() >= std::numeric_limits<UInt32>::max()) {
      throw std::invalid_argument("toooo much index");
    }
    _vertexCount = static_cast<UInt32>(model.VertexCount());
    _indexCount = static_cast<UInt32>(model.IndexCount());
    _triangleCount = model.TriangleCount();
  }

  ~Mesh() noexcept override {
    if (_needRelease) {
      if (_pos != nullptr) {
        delete[] _pos;
        _pos = nullptr;
      }
      if (_normal != nullptr) {
        delete[] _normal;
        _normal = nullptr;
      }
    }
  }

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
    std::copy(_pos, _pos + _vertexCount, vertices);
    std::copy(_indices, _indices + _indexCount, indices);
    rtcCommitGeometry(geo);
    rtcAttachGeometryByID(scene, geo, id);
    rtcReleaseGeometry(geo);
  }

 private:
  Eigen::Vector3f* _pos;
  Eigen::Vector3f* _normal;
  Eigen::Vector2f* _uv;
  UInt32* _indices;
  UInt32 _vertexCount;
  UInt32 _indexCount;
  UInt32 _triangleCount;
  bool _needRelease;
};

std::unique_ptr<IShape> CreateMesh(const TriangleModel& model, const Transform& transform) {
  return std::make_unique<Mesh>(model, transform);
}

}  // namespace rad
