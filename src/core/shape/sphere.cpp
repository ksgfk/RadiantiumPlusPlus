#include <radiantium/shape.h>

#include <radiantium/model.h>
#include <radiantium/transform.h>
#include <radiantium/math_ext.h>
#include <radiantium/factory.h>
#include <radiantium/build_context.h>
#include <radiantium/config_node.h>

#include <stdlib.h>

using namespace rad::math;

namespace rad {

struct alignas(16) EmbreeSphere {
  Eigen::Vector3f Center;
  float Radius;
  RTCGeometry Geometry;
  unsigned int GeomID;
};

static std::pair<bool, Float> SphereIntersect(
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
    auto n = ((rayD * t + rayO) - sphere.Center).normalized();
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
    auto n = ((rayD * t + rayO) - sphere.Center).normalized();
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

class Sphere : public IShape {
 public:
  Sphere(const Vec3& center, Float radius, const Transform& transform) {
    if (transform.HasNonUniformScale()) {
      logger::GetLogger()->warn("sphere has non uniform scale. result maybe wrong");
    }
    _center = transform.ApplyAffineToWorld(center);
    _radius = transform.ApplyAffineToWorld(Vec3(radius, 0, 0)).norm();
    Mat4 t;
    t << 1, 0, 0, center.x(),
        0, 1, 0, center.y(),
        0, 0, 1, center.z(),
        0, 0, 0, 1;
    Mat4 s;
    s << radius, 0, 0, 0,
        0, radius, 0, 0,
        0, 0, radius, 0,
        0, 0, 0, 1;
    auto mat = transform.ToWorld * t * s;
    _toWorld = Transform(mat);
  }

  ~Sphere() noexcept override {
    if (_giveEmbreeData != nullptr) {
      AlignedFree(_giveEmbreeData);
      _giveEmbreeData = nullptr;
    }
  }

  size_t PrimitiveCount() override { return 1; }

  void SubmitToEmbree(RTCDevice device, RTCScene scene, UInt32 id) override {
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_USER);
    EmbreeSphere* sphere = (EmbreeSphere*)AlignedAlloc(16, sizeof(EmbreeSphere));
    _giveEmbreeData = sphere;
    sphere->Center = _center.cast<Float32>();
    sphere->Radius = Float(_radius);
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
    SurfaceInteraction si;
    si.T = rec.T;
    si.Shading.N = (ray(si.T) - _center).normalized();
    si.P = Fmadd(si.Shading.N, Vec3(_radius, _radius, _radius), _center);
    {
      Vec3 local = _toWorld.ApplyAffineToLocal(si.P);
      Float rd2 = Sqr(local.x()) + Sqr(local.y());
      Float theta = UnitAngleZ(local);
      Float phi = std::atan2(local.y(), local.x());
      if (phi < 0) {
        phi += 2 * PI;
      }
      si.UV = Vec2(phi / (2 * PI), theta / PI);
      si.dPdU = Vec3(-local.y(), local.x(), 0.f);
      Float rd = std::sqrt(rd2);
      Float invRd = Rcp(rd);
      Float cosPhi = local.x() * invRd;
      Float sinPhi = local.y() * invRd;
      si.dPdV = Vec3(local.z() * cosPhi, local.z() * sinPhi, -rd);
      if (rd == 0) {
        si.dPdV = Vec3(1, 0, 0);
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

 private:
  Vec3 _center;
  Float _radius;
  Transform _toWorld;
  void* _giveEmbreeData = nullptr;
};

}  // namespace rad

namespace rad::factory {
class SphereShapeFactory : public IShapeFactory {
 public:
  ~SphereShapeFactory() noexcept override {}
  std::string UniqueId() const override { return "sphere"; }
  std::unique_ptr<IShape> Create(const BuildContext* context, const IConfigNode* config) const override {
    const Transform& toWorld = context->GetEntityCreateContext().ToWorld;
    Vec3 center = config->GetVec3("center", Vec3(0));
    Float radius = config->GetFloat("radius", 1);
    return std::make_unique<Sphere>(center, radius, toWorld);
  }
};
std::unique_ptr<IFactory> CreateSphereFactory() { return std::make_unique<SphereShapeFactory>(); }
}  // namespace rad::factory
