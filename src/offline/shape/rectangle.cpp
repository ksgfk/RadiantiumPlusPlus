#include <rad/offline/render/shape.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/memory.h>

namespace Rad {

struct alignas(16) EmbreeRectangle {
  Eigen::Matrix4f ToWorld;
  Eigen::Matrix4f ToObject;
  Eigen::Vector3f N;
  RTCGeometry Geometry;
  unsigned int GeomID;
};

/*
 * 根据初中数学知识(应该是初中吧), 我们知道向量点乘如果等于0, 说明两向量是垂直的
 * 我们可以用一个原点p0和一个垂直于平面的法线方向N来表示一个无穷远的平面
 * 现在我们在这个平面上标一个点p0, 它当然满足如下性质:
 * (p - p0) · N = 0
 * 从原点p0到平面上任意一点的方向当然与法线方向是垂直的
 * 回顾一下射线的解析式:
 * O + D * t = p
 * 可以直接带入方程
 * (O + D * t - p0) · N = 0
 * (D * t) · N + (O - p0) · N = 0
 * t = -((O - p0) · N) / (D · N)
 * t = ((p0 - O) · N) / (D · N)
 * 而且, 由于我们会将射线变换到本地坐标系下, p0永远是(0, 0, 0), 法线方向永远是(0, 0, 1), 方程还可以进一步简化
 * t = -O_z / D_z
 */
static std::tuple<bool, Float32, Eigen::Vector2f> RectangleIntersect(
    const EmbreeRectangle& rect,
    const Eigen::Vector3f& rayO_, const Eigen::Vector3f& rayD_, Float32 rayNear, Float32 rayFar) {
  Eigen::Affine3f affine(rect.ToObject);
  Eigen::Vector3f rayO = affine * rayO_;
  //不用归一化, 只进行线性变换会将方向向量按scale缩放, 保证结果t还是世界坐标系下的时间
  Eigen::Vector3f rayD = affine.linear() * rayD_;
  Float32 t = -rayO.z() / rayD.z();
  Eigen::Vector3f local = rayD * t + rayO;
  //射线需要在矩形范围内
  bool isHit = t >= rayNear && t <= rayFar && std::abs(local.x()) <= 1 && std::abs(local.y()) <= 1;
  return std::make_tuple(isHit, t, Eigen::Vector2f(local.x(), local.y()));
}

static void EmbreeRectangleBoundingBox(const struct RTCBoundsFunctionArguments* args) {
  const EmbreeRectangle* rects = (const EmbreeRectangle*)args->geometryUserPtr;
  RTCBounds* bounds = args->bounds_o;
  const EmbreeRectangle& rect = rects[args->primID];
  Eigen::Affine3f affine(rect.ToWorld);
  Eigen::AlignedBox3f bbox;
  bbox.extend(affine * Eigen::Vector3f(-1.0f, -1.0f, 0.0f));
  bbox.extend(affine * Eigen::Vector3f(1.0f, -1.0f, 0.0f));
  bbox.extend(affine * Eigen::Vector3f(1.0f, 1.0f, 0.0f));
  bbox.extend(affine * Eigen::Vector3f(-1.0f, 1.0f, 0.0f));
  bounds->lower_x = bbox.min().x();
  bounds->lower_y = bbox.min().y();
  bounds->lower_z = bbox.min().z();
  bounds->upper_x = bbox.max().x();
  bounds->upper_y = bbox.max().y();
  bounds->upper_z = bbox.max().z();
}

static void EmbreeRectangleIntersect(const RTCIntersectFunctionNArguments* args) {
  int* valid = args->valid;
  void* ptr = args->geometryUserPtr;
  RTCRayHit* rayhit = (RTCRayHit*)args->rayhit;
  RTCRay& ray = rayhit->ray;
  RTCHit& hit = rayhit->hit;
  unsigned int primID = args->primID;
  assert(args->N == 1);
  const EmbreeRectangle* rects = (const EmbreeRectangle*)ptr;
  const EmbreeRectangle& rect = rects[primID];
  if (!valid[0]) {
    return;
  }
  Eigen::Vector3f rayO = Eigen::Vector3f(ray.org_x, ray.org_y, ray.org_z);
  Eigen::Vector3f rayD = Eigen::Vector3f(ray.dir_x, ray.dir_y, ray.dir_z);
  auto [isHit, t, uv] = RectangleIntersect(
      rect,
      rayO, rayD, ray.tnear, ray.tfar);
  if (isHit) {
    RTCHit potentialHit;
    potentialHit.u = uv.x();
    potentialHit.v = uv.y();
    potentialHit.instID[0] = args->context->instID[0];
    potentialHit.geomID = rect.GeomID;
    potentialHit.primID = primID;
    potentialHit.Ng_x = rect.N.x();
    potentialHit.Ng_y = rect.N.y();
    potentialHit.Ng_z = rect.N.z();
    int imask;
    bool mask = 1;
    imask = mask ? -1 : 0;
    const float oldT = ray.tfar;
    ray.tfar = t;
    RTCFilterFunctionNArguments fargs;
    fargs.valid = (int*)&imask;
    fargs.geometryUserPtr = ptr;
    fargs.context = args->context;
    fargs.ray = (RTCRayN*)args->rayhit;
    fargs.hit = (RTCHitN*)&potentialHit;
    fargs.N = 1;
    rtcFilterIntersection(args, &fargs);
    if (imask == -1) {
      hit = potentialHit;
    } else {
      ray.tfar = oldT;
    }
  }
}

static void EmbreeRectangleOccluded(const RTCOccludedFunctionNArguments* args) {
  int* valid = args->valid;
  void* ptr = args->geometryUserPtr;
  RTCRay& ray = *(RTCRay*)args->ray;
  unsigned int primID = args->primID;
  assert(args->N == 1);
  const EmbreeRectangle* rects = (const EmbreeRectangle*)ptr;
  const EmbreeRectangle& rect = rects[primID];
  if (!valid[0]) {
    return;
  }
  Eigen::Vector3f rayO = Eigen::Vector3f(ray.org_x, ray.org_y, ray.org_z);
  Eigen::Vector3f rayD = Eigen::Vector3f(ray.dir_x, ray.dir_y, ray.dir_z);
  auto [isHit, t, uv] = RectangleIntersect(
      rect,
      rayO, rayD, ray.tnear, ray.tfar);
  if (isHit) {
    RTCHit potentialHit;
    potentialHit.u = uv.x();
    potentialHit.v = uv.y();
    potentialHit.instID[0] = args->context->instID[0];
    potentialHit.geomID = rect.GeomID;
    potentialHit.primID = primID;
    potentialHit.Ng_x = rect.N.x();
    potentialHit.Ng_y = rect.N.y();
    potentialHit.Ng_z = rect.N.z();
    int imask;
    bool mask = 1;
    imask = mask ? -1 : 0;
    const float oldT = ray.tfar;
    ray.tfar = t;
    RTCFilterFunctionNArguments fargs;
    fargs.valid = (int*)&imask;
    fargs.geometryUserPtr = ptr;
    fargs.context = args->context;
    fargs.ray = (RTCRayN*)args->ray;
    fargs.hit = (RTCHitN*)&potentialHit;
    fargs.N = 1;
    rtcFilterOcclusion(args, &fargs);
    if (imask == -1) {
      ray.tfar = -std::numeric_limits<float>::infinity();
    } else {
      ray.tfar = oldT;
    }
  }
}

class Rectangle final : public Shape {
 public:
  Rectangle(BuildContext* ctx, const ConfigNode& cfg) {
    const Matrix4& toWorld = ctx->GetShapeMatrix();
    _toWorld = Transform(toWorld);
    Vector3 dpdu = _toWorld.ApplyLinearToWorld(Vector3(2, 0, 0));
    Vector3 dpdv = _toWorld.ApplyLinearToWorld(Vector3(0, 2, 0));
    Vector3 normal = _toWorld.ApplyNormalToWorld(Vector3(0, 0, 1)).normalized();
    _frame = Frame(dpdu, dpdv, normal);
    _surfaceArea = dpdu.cross(dpdv).norm();
    _giveEmbreeData = (EmbreeRectangle*)Memory::AlignedAlloc(16, sizeof(EmbreeRectangle));
  }
  ~Rectangle() noexcept override {
    if (_giveEmbreeData != nullptr) {
      Memory::AlignedFree(_giveEmbreeData);
      _giveEmbreeData = nullptr;
    }
  }

  void SubmitToEmbree(RTCDevice device, RTCScene scene, UInt32 id) const override {
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_USER);
    EmbreeRectangle* rect = _giveEmbreeData;
    rect->ToWorld = _toWorld.ToWorld.cast<Eigen::Matrix4f::Scalar>();
    rect->ToObject = _toWorld.ToLocal.cast<Eigen::Matrix4f::Scalar>();
    rect->N = _frame.N.cast<Eigen::Matrix4f::Scalar>();
    rect->Geometry = geom;
    rect->GeomID = rtcAttachGeometry(scene, geom);
    rtcSetGeometryUserPrimitiveCount(geom, 1);
    rtcSetGeometryUserData(geom, rect);
    rtcSetGeometryBoundsFunction(geom, EmbreeRectangleBoundingBox, nullptr);
    rtcSetGeometryIntersectFunction(geom, EmbreeRectangleIntersect);
    rtcSetGeometryOccludedFunction(geom, EmbreeRectangleOccluded);
    rtcCommitGeometry(geom);
    rtcReleaseGeometry(geom);
  }

  SurfaceInteraction ComputeInteraction(const Ray& ray, const HitShapeRecord& rec) override {
    SurfaceInteraction si{};
    si.T = rec.T;
    Vector3 p = ray(rec.T);
    //将P投影到矩形上, 消除误差
    si.P = (_toWorld.TranslationToWorld() - p).dot(_frame.N) * _frame.N + p;
    si.N = _frame.N;
    si.Shading.N = _frame.N;
    si.dPdU = _frame.S;
    si.dPdV = _frame.T;
    si.UV = rec.PrimitiveUV * Float(0.5) + Vector2::Constant(Float(0.5));
    si.dNdU = Vector3::Zero();
    si.dNdV = Vector3::Zero();
    si.Shape = this;
    return si;
  }

  PositionSampleResult SamplePosition(const Vector2& xi) const override {
    PositionSampleResult psr{};
    Vector2 sample = xi * 2 - Vector2::Constant(1);
    psr.P = _toWorld.ApplyAffineToWorld(Vector3(sample.x(), sample.y(), 0.f));
    psr.N = _frame.N;
    psr.Pdf = 1 / _surfaceArea;
    psr.UV = xi;
    psr.IsDelta = false;
    return psr;
  }

  Float PdfPosition(const PositionSampleResult& psr) const override {
    return 1 / _surfaceArea;
  }

 private:
  Transform _toWorld;
  Frame _frame;
  EmbreeRectangle* _giveEmbreeData;
};

}  // namespace Rad

RAD_FACTORY_SHAPE_DECLARATION(Rectangle, "rectangle");
