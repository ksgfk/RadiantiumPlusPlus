#include <rad/offline/render/shape.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/memory.h>

using namespace Rad::Math;

namespace Rad {

struct alignas(16) EmbreeSphere {
  Eigen::Vector3f Center;
  float Radius;
  RTCGeometry Geometry;
  unsigned int GeomID;
};

/* https://pbr-book.org/3ed-2018/Shapes/Spheres#IntersectionTests
 * 首先球的隐式表达为:
 * (P - C)^2 - r^2 = 0 (1), 其中 p 表示球面上一点的坐标, c表示球心坐标, r表示半径
 * 射线的表达式为:
 * r(t) = O + t * D (2)
 * 球面上一点可以用射线与这一点相交来代替, 所以将球公式 (1) 改写:
 * (O + t * D - C)^2 - r^2 = 0 (3)
 * 除了 t, 也就是射线从起点出发, 经过的距离, 其他参数都是已知的
 * 我们将公式 (3) 展开
 * (3) = (t * D + (O - C))^2 - r^2
 *     = (t * D)^2 + 2 * t * D * (O - C) + (O - C)^2 - r^2
 *     = D^2 * t^2 + 2 * D * (O - C) * t + (O - C)^2 - r^2 (4)
 * 很容易发现这是个标准的一元二次方程, 且
 * a = D^2
 * b = 2 * D * (O - C)
 * c = (O - C)^2 - r^2
 * 解出t1, t2就可以求出射线与球是否有交点, 最近的交点就是最小的那个解
 */
static std::pair<bool, float> SphereIntersect(
    const Eigen::Vector3f& rayO, const Eigen::Vector3f& rayD,
    const Eigen::Vector3f& center, float radius,
    float rayNear, float rayFar) {
  auto o = rayO - center;
  auto a = rayD.squaredNorm();
  auto b = 2 * o.dot(rayD);
  auto c = o.squaredNorm() - Sqr(radius);
  auto [found, nearT, farT] = SolveQuadratic(a, b, c);
  bool outBounds = !(nearT <= rayFar && farT >= rayNear);
  bool inBounds = nearT < rayNear && farT > rayFar;
  bool isHit = found && !outBounds && !inBounds;
  auto t = nearT < 0 ? farT : nearT;
  return std::make_pair(isHit, t);
}

static void EmbreeSphereBoundingBox(const struct RTCBoundsFunctionArguments* args) {
  const EmbreeSphere* spheres = (const EmbreeSphere*)args->geometryUserPtr;
  RTCBounds* bounds_o = args->bounds_o;
  const EmbreeSphere& sphere = spheres[args->primID];
  auto r = Eigen::Vector3f(spheres->Radius, spheres->Radius, spheres->Radius);
  Eigen::Vector3f min = sphere.Center - r;
  Eigen::Vector3f max = sphere.Center + r;
  bounds_o->lower_x = min.x();
  bounds_o->lower_y = min.y();
  bounds_o->lower_z = min.z();
  bounds_o->upper_x = max.x();
  bounds_o->upper_y = max.y();
  bounds_o->upper_z = max.z();
}

static void EmbreeSphereIntersect(const RTCIntersectFunctionNArguments* args) {
  int* valid = args->valid;
  void* ptr = args->geometryUserPtr;
  RTCRayHit* rayhit = (RTCRayHit*)args->rayhit;
  RTCRay& ray = rayhit->ray;
  RTCHit& hit = rayhit->hit;
  unsigned int primID = args->primID;
  assert(args->N == 1);
  const EmbreeSphere* spheres = (const EmbreeSphere*)ptr;
  const EmbreeSphere& sphere = spheres[primID];
  if (!valid[0]) {
    return;
  }
  Eigen::Vector3f rayO = Eigen::Vector3f(ray.org_x, ray.org_y, ray.org_z);
  Eigen::Vector3f rayD = Eigen::Vector3f(ray.dir_x, ray.dir_y, ray.dir_z);
  auto [isHit, t] = SphereIntersect(
      rayO, rayD,
      sphere.Center, sphere.Radius,
      ray.tnear, ray.tfar);
  if (isHit) {
    RTCHit potentialHit;
    potentialHit.u = 0.0f;
    potentialHit.v = 0.0f;
    potentialHit.instID[0] = args->context->instID[0];
    potentialHit.geomID = sphere.GeomID;
    potentialHit.primID = primID;
    Eigen::Vector3f n = ((rayD * t + rayO) - sphere.Center).normalized();
    potentialHit.Ng_x = n.x();
    potentialHit.Ng_y = n.y();
    potentialHit.Ng_z = n.z();
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

static void EmbreeSphereOccluded(const RTCOccludedFunctionNArguments* args) {
  int* valid = args->valid;
  void* ptr = args->geometryUserPtr;
  RTCRay& ray = *(RTCRay*)args->ray;
  unsigned int primID = args->primID;
  assert(args->N == 1);
  const EmbreeSphere* spheres = (const EmbreeSphere*)ptr;
  const EmbreeSphere& sphere = spheres[primID];
  if (!valid[0]) {
    return;
  }
  Eigen::Vector3f rayO = Eigen::Vector3f(ray.org_x, ray.org_y, ray.org_z);
  Eigen::Vector3f rayD = Eigen::Vector3f(ray.dir_x, ray.dir_y, ray.dir_z);
  auto [isHit, t] = SphereIntersect(
      rayO, rayD,
      sphere.Center, sphere.Radius,
      ray.tnear, ray.tfar);
  if (isHit) {
    RTCHit potentialHit;
    potentialHit.u = 0.0f;
    potentialHit.v = 0.0f;
    potentialHit.instID[0] = args->context->instID[0];
    potentialHit.geomID = sphere.GeomID;
    potentialHit.primID = primID;
    Eigen::Vector3f n = ((rayD * t + rayO) - sphere.Center).normalized();
    potentialHit.Ng_x = n.x();
    potentialHit.Ng_y = n.y();
    potentialHit.Ng_z = n.z();
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

class Sphere final : public Shape {
 public:
  Sphere(BuildContext* ctx, const ConfigNode& cfg) {
    const Matrix4& toWorld = ctx->GetShapeMatrix();
    Vector3 localCenter = cfg.ReadOrDefault("center", Vector3(Vector3::Constant(0)));
    Float localRadius = cfg.ReadOrDefault("radius", Float(1));
    Transform transform(toWorld);
    if (transform.HasNonUniformScale()) {
      throw RadArgumentException("Sphere does not support non uniform transformations");
    }
    _center = transform.ApplyAffineToWorld(localCenter);
    _radius = transform.ApplyLinearToWorld(Vector3(0, 0, localRadius)).norm();
    Eigen::Translation<Float, 3> t(_center);
    Eigen::UniformScaling<Float> s(_radius);
    Eigen::Transform<Float, 3, Eigen::Affine> affine(toWorld);
    auto trans = affine * t * s;
    _toWorld = Transform(trans.matrix());
    _surfaceArea = 4 * PI * Sqr(_radius);

    _giveEmbreeData = (EmbreeSphere*)Memory::AlignedAlloc(16, sizeof(EmbreeSphere));
  }

  ~Sphere() noexcept override {
    if (_giveEmbreeData != nullptr) {
      Memory::AlignedFree(_giveEmbreeData);
      _giveEmbreeData = nullptr;
    }
  }

  void SubmitToEmbree(RTCDevice device, RTCScene scene, UInt32 id) const override {
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_USER);
    EmbreeSphere* sphere = _giveEmbreeData;
    sphere->Center = _center.cast<Float32>();
    sphere->Radius = Float32(_radius);
    sphere->Geometry = geom;
    sphere->GeomID = rtcAttachGeometry(scene, geom);
    rtcSetGeometryUserPrimitiveCount(geom, 1);
    rtcSetGeometryUserData(geom, sphere);
    rtcSetGeometryBoundsFunction(geom, EmbreeSphereBoundingBox, nullptr);
    rtcSetGeometryIntersectFunction(geom, EmbreeSphereIntersect);
    rtcSetGeometryOccludedFunction(geom, EmbreeSphereOccluded);
    rtcCommitGeometry(geom);
    rtcReleaseGeometry(geom);
  }

  SurfaceInteraction ComputeInteraction(const Ray& ray, const HitShapeRecord& rec) override {
    SurfaceInteraction si{};
    si.T = rec.T;
    si.Shading.N = rec.GeometryNormal;
    si.P = Fmadd(si.Shading.N, Vector3::Constant(_radius), _center);
    {
      Vector3 local = _toWorld.ApplyAffineToLocal(si.P);
      Float rd2 = Sqr(local.x()) + Sqr(local.y());
      Float theta = UnitAngleZ(local);
      Float phi = std::atan2(local.y(), local.x());
      if (phi < 0) {
        phi += 2 * PI;
      }
      si.UV = Vector2(phi / (2 * PI), theta / PI);
      si.dPdU = Vector3(-local.y(), local.x(), 0.f);
      Float rd = std::sqrt(rd2);
      Float invRd = Rcp(rd);
      Float cosPhi = local.x() * invRd;
      Float sinPhi = local.y() * invRd;
      si.dPdV = Vector3(local.z() * cosPhi, local.z() * sinPhi, -rd);
      if (rd == 0) {
        si.dPdV = Vector3(1, 0, 0);
      }
      si.dPdU = _toWorld.ApplyLinearToWorld(si.dPdU * (2 * PI));
      si.dPdV = _toWorld.ApplyLinearToWorld(si.dPdV * PI);
    }
    si.N = si.Shading.N;
    {
      Float invRadius = Rcp(_radius);
      si.dNdU = si.dPdU * invRadius;
      si.dNdV = si.dNdV * invRadius;
    }
    si.Shape = this;
    return si;
  }

  PositionSampleResult SamplePosition(const Vector2& xi) const override {
    Vector3 local = Warp::SquareToUniformSphere(xi);
    PositionSampleResult psr{};
    psr.P = Fmadd(local, Vector3::Constant(_radius), _center);
    psr.N = local;
    psr.IsDelta = _radius == 0;
    psr.Pdf = 1 / _surfaceArea;
    return psr;
  }

  Float PdfPosition(const PositionSampleResult& psr) const override {
    return 1 / _surfaceArea;
  }

  DirectionSampleResult SampleDirection(const Interaction& ref, const Vector2& xi) const override {
    /* https://pbr-book.org/3ed-2018/Light_Transport_I_Surface_Reflection/Sampling_Light_Sources#x2-SamplingSpheres
     * 在球体上采样时, 我们当然可以在球的表面均匀选取一个点, 这是完全正确的
     * 但是思考一下, 我们看向球的时候, 实际上只能看到一个半球表面, 另一半是看不见的
     * 因此更好的采样方法是只采样可见的那个半球
     * pbrt给出参考图, 我们实际可视范围是一个锥体, 锥体角度是
     * \theta_max = \mathrm{arcsin}(\frac{r}{p-p_center})
     * (pbrt的图画的不准确...视线锥体与圆的切线歪了)
     * 然后就可以在锥体内均匀采样, 再投影到半球表面
     * 怎么去投影呢, 一般做法是从球心与采样点生成射线, 再与球相交, 这个方法是不稳定的, 因为浮点误差
     * 幸运的是大佬们发现了更好的方法
     * https://www.akalin.com/sampling-visible-sphere
     * 文字不是很好描述, 直接看图就解出来了
     *
     * 当然, 如果参考点本身就在球内, 就只能均匀采样了
     */
    Vector3 dcv = _center - ref.P;
    Float dc2 = dcv.squaredNorm();
    Float radiusAdj = _radius * (Float(1) - Math::RayEpsilon);
    bool isOut = dc2 > Sqr(radiusAdj);
    DirectionSampleResult dsr;
    if (isOut) {
      Float invDc = Rsqrt(dc2);
      Float sinThetaMax = _radius * invDc;
      Float sinThetaMax2 = Sqr(sinThetaMax);
      Float invSinThetaMax = Rcp(sinThetaMax);
      Float cosThetaMax = SafeSqrt(1.f - sinThetaMax2);
      Float sinTheta2 = sinThetaMax2 > Float(0.00068523262)
                            ? Float(1) - Sqr(Fmadd(cosThetaMax - Float(1), xi.x(), Float(1)))
                            : sinThetaMax2 * xi.x();
      Float cosTheta = SafeSqrt(Float(1) - sinTheta2);
      Float cosAlpha = sinTheta2 * invSinThetaMax +
                       cosTheta * SafeSqrt(Fmadd(-sinTheta2, Sqr(invSinThetaMax), Float(1)));
      Float sinAlpha = SafeSqrt(Fmadd(-cosAlpha, cosAlpha, Float(1)));
      auto [sinPhi, cosPhi] = SinCos(xi.y() * (Float(2) * PI));
      Vector3 d = Frame(dcv * -invDc).ToWorld(Vector3(cosPhi * sinAlpha, sinPhi * sinAlpha, cosAlpha));
      dsr.P = Fmadd(d, Vector3::Constant(_radius), _center);
      dsr.N = d;
      dsr.Dir = dsr.P - ref.P;
      Float dist2 = dsr.Dir.squaredNorm();
      dsr.Dist = std::sqrt(dist2);
      dsr.Dir = dsr.Dir / dsr.Dist;
      dsr.Pdf = Warp::SquareToUniformConePdf(cosThetaMax);
    } else {
      Vector3 d = Warp::SquareToUniformSphere(xi);
      dsr.P = Fmadd(d, Vector3::Constant(_radius), _center);
      dsr.N = d;
      dsr.Dir = dsr.P - ref.P;
      Float dist2 = dsr.Dir.squaredNorm();
      dsr.Dist = std::sqrt(dist2);
      dsr.Dir = dsr.Dir / dsr.Dist;
      dsr.Pdf = (1 / _surfaceArea) * dist2 / AbsDot(dsr.Dir, dsr.N);
    }
    dsr.IsDelta = _radius == 0;
    return dsr;
  }

  Float PdfDirection(const Interaction& ref, const DirectionSampleResult& dsr) const override {
    Float sinAlpha = _radius * Rcp((_center - ref.P).norm()),
          cosAlpha = SafeSqrt(Float(1) - sinAlpha * sinAlpha);
    return sinAlpha < OneMinusEpsilon<Float>()
               ? Warp::SquareToUniformConePdf(cosAlpha)
               : (1 / _surfaceArea) * Sqr(dsr.Dist) / AbsDot(dsr.Dir, dsr.N);
  }

 private:
  Vector3 _center;
  Float _radius;
  Transform _toWorld;
  EmbreeSphere* _giveEmbreeData;
};

}  // namespace Rad

RAD_FACTORY_SHAPE_DECLARATION(Sphere, "sphere");
