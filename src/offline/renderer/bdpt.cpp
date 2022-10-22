#include <rad/offline/render/renderer.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/global_control.h>

#include <random>

using namespace Rad::Math;

namespace Rad {

class BDPT final : public Renderer {
 public:
  struct PathVertex {
  };

  BDPT(BuildContext* ctx, const ConfigNode& cfg) : Renderer(ctx, cfg) {
    _maxDepth = cfg.ReadOrDefault("max_depth", 5);
    _useMis = cfg.ReadOrDefault("use_mis", true);
  }
  ~BDPT() noexcept override = default;
  void Start() override {
    if (_renderThread != nullptr) {
      return;
    }
    std::thread renderThread([&]() { Render(); });
    _renderThread = std::make_unique<std::thread>(std::move(renderThread));
  }

  void SaveResult(const LocationResolver& resolver) const override {
    resolver.SaveOpenExr(_scene->GetCamera().GetFrameBuffer());
  }

 private:
  void Render() {
    Scene& scene = *_scene;
    Camera& camera = scene.GetCamera();
    const Sampler& sampler = camera.GetSampler();
    MatrixX<Spectrum>& frameBuffer = camera.GetFrameBuffer();
    tbb::affinity_partitioner part;
    std::unique_ptr<tbb::global_control> ctrl;
    if (_threadCount > 0) {
      ctrl = std::make_unique<tbb::global_control>(tbb::global_control::max_allowed_parallelism, _threadCount);
    }
    tbb::blocked_range2d<UInt32> block(
        0, camera.Resolution().x(),
        0, camera.Resolution().y());
    std::mutex mutex;
    _sw.Start();
    tbb::parallel_for(
        block, [&](const tbb::blocked_range2d<UInt32>& r) {
          UInt32 seed = r.rows().begin() * camera.Resolution().x() + r.cols().begin();
          thread_local auto rng = std::mt19937(seed);
          thread_local std::vector<PathVertex> lightPath;
          thread_local MatrixX<Spectrum> tempFb(frameBuffer.rows(), frameBuffer.cols());
          std::uniform_real_distribution<Float> dist;
          Unique<Sampler> localSampler = sampler.Clone(sampler.GetSeed() + seed);
          tempFb.setZero();
          for (UInt32 y = r.cols().begin(); y != r.cols().end(); y++) {
            for (UInt32 x = r.rows().begin(); x != r.rows().end(); x++) {
              Spectrum radiance(0);
              for (UInt32 i = 0; i < sampler.SampleCount(); i++) {
                if (_isStop) {
                  break;
                }
                localSampler->Advance();
                lightPath.clear();
                Vector2 scrPos(x + dist(rng), y + dist(rng));
                Ray ray = camera.SampleRay(scrPos);
                Spectrum li = Li(ray, scene, camera, localSampler.get(), tempFb, lightPath);
                if (li.HasNaN() || li.HasInfinity() || li.HasNegative()) {
                  _logger->warn("invalid spectrum {}", li);
                } else {
                  radiance += li;
                }
              }
              tempFb(x, y) += radiance;
              if (_isStop) {
                break;
              }
            }
            if (_isStop) {
              break;
            }
          }
          Float32 coeff = Float(1) / sampler.SampleCount();
          for (UInt32 y = 0; y < tempFb.cols(); y++) {
            for (UInt32 x = 0; x < tempFb.rows(); x++) {
              tempFb(y, x) *= coeff;
            }
          }
          {
            std::lock_guard<std::mutex> lock(mutex);
            frameBuffer += tempFb;
          }
        },
        part);
    _sw.Stop();
  }

  Spectrum Li(
      const Ray& ray_,
      const Scene& scene,
      const Camera& camera,
      Sampler* sampler,
      MatrixX<Spectrum>& img,
      std::vector<PathVertex>& lightPath) {
  }

  Int32 _maxDepth;
  bool _useMis;
};

// class BDPT final : public Renderer {
//  public:
//   struct SubPathState {
//     Vector3 mOrigin;        // Path origin
//     Vector3 mDirection;     // Where to go next
//     Spectrum mThroughput;   // Path throughput
//     UInt32 mPathLength;     // Number of path segments, including this
//     UInt32 mIsFiniteLight;  // Just generate by finite light
//     UInt32 mSpecularPath;   // All scattering events so far were specular

//     Float dVCM;  // MIS quantity used for vertex connection and merging
//     Float dVC;   // MIS quantity used for vertex connection
//   };

//   struct PathVertex {
//     Vector3 mHitpoint;     // Position of the vertex
//     Spectrum mThroughput;  // Path throughput (including emission)
//     UInt32 mPathLength;    // Number of segments between source and vertex

//     // Stores all required local information, including incoming direction.
//     Bsdf* mBsdf;
//     SurfaceInteraction Si;

//     Float dVCM;  // MIS quantity used for vertex connection and merging
//     Float dVC;   // MIS quantity used for vertex connection
//   };

//  public:
//   BDPT(BuildContext* ctx, const ConfigNode& cfg) : Renderer(ctx, cfg) {
//     _maxDepth = cfg.ReadOrDefault("max_depth", 5);
//     _useMis = cfg.ReadOrDefault("use_mis", true);
//   }
//   ~BDPT() noexcept override = default;

//   void Start() override {
//     if (_renderThread != nullptr) {
//       return;
//     }
//     std::thread renderThread([&]() { Render(); });
//     _renderThread = std::make_unique<std::thread>(std::move(renderThread));
//   }

//   void SaveResult(const LocationResolver& resolver) const override {
//     resolver.SaveOpenExr(_scene->GetCamera().GetFrameBuffer());
//   }

//  private:
//   void Render() {
//     Scene& scene = *_scene;
//     Camera& camera = scene.GetCamera();
//     const Sampler& sampler = camera.GetSampler();
//     MatrixX<Spectrum>& frameBuffer = camera.GetFrameBuffer();
//     tbb::affinity_partitioner part;
//     std::unique_ptr<tbb::global_control> ctrl;
//     if (_threadCount > 0) {
//       ctrl = std::make_unique<tbb::global_control>(tbb::global_control::max_allowed_parallelism, _threadCount);
//     }
//     tbb::blocked_range2d<UInt32> block(
//         0, camera.Resolution().x(),
//         0, camera.Resolution().y());
//     std::mutex mutex;
//     _sw.Start();
//     tbb::parallel_for(
//         block, [&](const tbb::blocked_range2d<UInt32>& r) {
//           UInt32 seed = r.rows().begin() * camera.Resolution().x() + r.cols().begin();
//           thread_local auto rng = std::mt19937(seed);
//           thread_local std::vector<PathVertex> lightPath;
//           thread_local MatrixX<Spectrum> tempFb(frameBuffer.rows(), frameBuffer.cols());
//           std::uniform_real_distribution<Float> dist;
//           Unique<Sampler> localSampler = sampler.Clone(sampler.GetSeed() + seed);
//           tempFb.setZero();
//           for (UInt32 y = r.cols().begin(); y != r.cols().end(); y++) {
//             for (UInt32 x = r.rows().begin(); x != r.rows().end(); x++) {
//               Spectrum radiance(0);
//               for (UInt32 i = 0; i < sampler.SampleCount(); i++) {
//                 if (_isStop) {
//                   break;
//                 }
//                 localSampler->Advance();
//                 lightPath.clear();
//                 Vector2 scrPos(x + dist(rng), y + dist(rng));
//                 Ray ray = camera.SampleRay(scrPos);
//                 Spectrum li = Li(ray, scene, camera, localSampler.get(), tempFb, lightPath);
//                 if (li.HasNaN() || li.HasInfinity() || li.HasNegative()) {
//                   _logger->warn("invalid spectrum {}", li);
//                 } else {
//                   radiance += li;
//                 }
//               }
//               tempFb(x, y) += radiance;
//               if (_isStop) {
//                 break;
//               }
//             }
//             if (_isStop) {
//               break;
//             }
//           }
//           Float32 coeff = Float(1) / sampler.SampleCount();
//           for (UInt32 y = 0; y < tempFb.cols(); y++) {
//             for (UInt32 x = 0; x < tempFb.rows(); x++) {
//               tempFb(y, x) *= coeff;
//             }
//           }
//           {
//             std::lock_guard<std::mutex> lock(mutex);
//             frameBuffer += tempFb;
//           }
//         },
//         part);
//     _sw.Stop();
//   }

//   Spectrum Li(
//       const Ray& ray_,
//       const Scene& scene,
//       const Camera& camera,
//       Sampler* sampler,
//       MatrixX<Spectrum>& img,
//       std::vector<PathVertex>& lightPath) {
//     //从光源开始追踪
//     {
//       auto [lightState, ray] = GenerateLightSample(scene, sampler);
//       for (; lightState.mPathLength <= (UInt32)_maxDepth; lightState.mPathLength++) {
//         SurfaceInteraction si{};
//         if (!scene.RayIntersect(ray, si)) {
//           break;
//         }
//         if (!si.Shape->HasBsdf()) {
//           break;
//         }
//         Bsdf* bsdf = si.BSDF(ray);
//         {
//           if (lightState.mPathLength > 1 || lightState.mIsFiniteLight == 1) {
//             lightState.dVCM *= Mis(Sqr(si.T));
//           }
//           lightState.dVCM /= Mis(Frame::AbsCosTheta(si.Wi));
//           lightState.dVC /= Mis(Frame::AbsCosTheta(si.Wi));
//         }
//         if (bsdf->HasAnyTypeExceptDelta()) {
//           PathVertex lightVertex;
//           lightVertex.mHitpoint = si.P;
//           lightVertex.mThroughput = lightState.mThroughput;
//           lightVertex.mPathLength = lightState.mPathLength;
//           lightVertex.mBsdf = bsdf;
//           lightVertex.Si = si;

//           lightVertex.dVCM = lightState.dVCM;
//           lightVertex.dVC = lightState.dVC;

//           lightPath.push_back(lightVertex);

//           ConnectCamera(scene, camera, sampler, lightVertex, img);
//         }
//         BsdfContext ctx{TransportMode::Importance, (UInt32)BsdfType::All};
//         auto [bsr, fs] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
//         if (bsr.Pdf <= 0) {
//           break;
//         }
//         Float correct = CorrectShadingNormal(si, bsr.Wo);
//         auto tmp = lightState.mThroughput.cwiseProduct(fs) * correct / bsr.Pdf;
//         lightState.mThroughput = Spectrum(tmp);
//         ray = si.SpawnRay(si.ToWorld(bsr.Wo));
//         lightState.mOrigin = ray.O;
//         lightState.mDirection = ray.D;
//       }
//     }
//     //现在从相机开始连接光源
//     Spectrum color(0);
//     {
//       auto [cameraState, ray] = GenerateCameraSample(scene, camera, sampler, ray_);
//       for (; cameraState.mPathLength <= _maxDepth; cameraState.mPathLength++) {
//         SurfaceInteraction si{};
//         bool isHit = scene.RayIntersect(ray, si);
//         if (!isHit) {
//           // TODO: Env
//           break;
//         }
//         if (!si.Shape->HasBsdf()) {
//           break;
//         }
//         Bsdf* bsdf = si.BSDF(ray);
//         {
//           cameraState.dVCM *= Mis(Sqr(si.T));
//           cameraState.dVCM /= Mis(Frame::AbsCosTheta(si.Wi));
//           cameraState.dVC /= Mis(Frame::AbsCosTheta(si.Wi));
//         }
//         if (si.Shape->IsLight()) {
//           Light* light = si.Shape->GetLight();
//           Spectrum li = GetLightRadiance(scene, light, cameraState, si);
//           auto tmp = cameraState.mThroughput.cwiseProduct(li);
//           color += Spectrum(tmp);
//           break;
//         }
//         if (bsdf->HasAnyTypeExceptDelta()) {
//           color += DirectIllumination(scene, sampler, cameraState, si, bsdf);
//           for (size_t j = 0; j < lightPath.size(); j++) {
//             const PathVertex& lightVertex = lightPath[j];
//             Spectrum weight = ConnectVertex(scene, lightVertex, si, bsdf, cameraState);
//             auto tmp = cameraState.mThroughput.cwiseProduct(lightVertex.mThroughput).cwiseProduct(weight);
//             color += Spectrum(tmp);
//           }
//         }
//         BsdfContext ctx{TransportMode::Radiance, (UInt32)BsdfType::All};
//         auto [bsr, fs] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
//         if (bsr.Pdf <= 0) {
//           break;
//         }
//         auto tmp = cameraState.mThroughput.cwiseProduct(fs) / bsr.Pdf;
//         cameraState.mThroughput = Spectrum(tmp);
//         ray = si.SpawnRay(si.ToWorld(bsr.Wo));
//         cameraState.mOrigin = ray.O;
//         cameraState.mDirection = ray.D;
//       }
//     }
//     return color;
//   }

//   Float Mis(Float t) const {
//     return _useMis ? t * t : Float(1);
//   }

//   void ConnectCamera(
//       const Scene& scene,
//       const Camera& camera,
//       Sampler* sampler,
//       const PathVertex& vert,
//       MatrixX<Spectrum>& img) {
//     if (vert.mThroughput.IsBlack()) {
//       return;
//     }
//     const SurfaceInteraction& vSi = vert.Si;
//     auto [dsr, we, cameraPdfW, cameraPdfA, cosAtCamera] = camera.SampleDirectionWithPdf(vSi, sampler->Next2D());
//     if (dsr.Pdf <= 0 || we.IsBlack()) {
//       return;
//     }
//     Vector3 offsetO = vSi.OffsetP(dsr.P - vSi.P);
//     Vector3 toCamera = dsr.P - offsetO;
//     Float toCamDist2 = toCamera.squaredNorm();
//     toCamera /= std::sqrt(toCamDist2);
//     Vector3 wo = vSi.ToLocal(toCamera);
//     BsdfContext ctx{TransportMode::Importance, (UInt32)BsdfType::All};
//     Spectrum fs = vert.mBsdf->Eval(ctx, vSi, wo);
//     if (fs.IsBlack() || scene.IsOcclude(vSi, dsr.P)) {
//       return;
//     }
//     Float correct = CorrectShadingNormal(vSi, wo);
//     auto ij = vert.mThroughput.cwiseProduct(fs).cwiseProduct(we) * correct / dsr.Pdf;

//     Float weight = 1;
//     Float lightVertPdfA = cameraPdfW * AbsDot(vSi.Shading.N, toCamera) * Rcp(toCamDist2);
//     SurfaceInteraction rSi = vSi;
//     rSi.Wi = wo;
//     Float revBsdfPdfW = vert.mBsdf->Pdf(ctx, rSi, vSi.Wi);
//     Float mis0 = (vert.dVCM + vert.dVC * Mis(revBsdfPdfW)) * Mis(lightVertPdfA);
//     weight = (Float(1) / (Float(1) + mis0));

//     img((int)dsr.UV.x(), (int)dsr.UV.y()) += Spectrum(ij * weight);
//   }

//   std::pair<SubPathState, Ray> GenerateLightSample(
//       const Scene& scene,
//       Sampler* sampler) {
//     //随机选取一个光源
//     auto [lightIndex, selectPdf] = scene.SampleLight(sampler->Next1D());
//     const Light* light = scene.GetLight(lightIndex);
//     auto [lightRay, le, emissionPdfW, directPdfA, cosLight] = light->SampleRayWithPdf(sampler->Next2D(), sampler->Next2D());
//     emissionPdfW *= selectPdf;
//     directPdfA *= selectPdf;
//     SubPathState sps{};
//     sps.mThroughput = Spectrum(le / selectPdf);
//     sps.mOrigin = lightRay.O;
//     sps.mDirection = lightRay.D;
//     sps.mPathLength = 1;
//     sps.mIsFiniteLight = true;  // TODO:
//     {
//       sps.dVCM = Mis(directPdfA / emissionPdfW);
//       if (!HasFlag(light->Flags(), LightType::Delta)) {
//         // const float usedCosLight = light->IsFinite() ? cosLight : 1.f; //TODO:
//         const float usedCosLight = cosLight;
//         sps.dVC = Mis(usedCosLight / emissionPdfW);
//       } else {
//         sps.dVC = 0.f;
//       }
//     }
//     return std::make_pair(sps, lightRay);
//   }

//   std::pair<SubPathState, Ray> GenerateCameraSample(
//       const Scene& scene,
//       const Camera& camera,
//       Sampler* sampler,
//       const Ray& ray) {
//     auto [emissionPdfW, directPdfA] = camera.PdfDirection(ray);
//     SubPathState sps{};
//     sps.mOrigin = ray.O;
//     sps.mDirection = ray.D;
//     sps.mThroughput = Spectrum(1);
//     sps.mPathLength = 1;
//     sps.mSpecularPath = 1;
//     sps.dVCM = Mis(1 / emissionPdfW);
//     sps.dVC = 0;
//     return std::make_pair(sps, ray);
//   }

//   Spectrum GetLightRadiance(
//       const Scene& scene,
//       const Light* light,
//       const SubPathState& aCameraState,
//       const SurfaceInteraction& si) {
//     Interaction ref = Interaction{0, aCameraState.mOrigin, Vector3()};
//     auto [li, emissionPdfW, directPdfA] = light->GetRadiance(ref, si.ToDsr(ref));
//     if (aCameraState.mPathLength == 1) {
//       return li;
//     }
//     Float selectPdf = scene.PdfLight(light);
//     directPdfA *= selectPdf;
//     emissionPdfW *= selectPdf;
//     Float wCamera = Mis(directPdfA) * aCameraState.dVCM + Mis(emissionPdfW) * aCameraState.dVC;
//     Float misWeight = Float(1) / (Float(1) + wCamera);
//     return Spectrum(li * misWeight);
//   }

//   Spectrum DirectIllumination(
//       const Scene& scene,
//       Sampler* sampler,
//       const SubPathState& aCameraState,
//       const SurfaceInteraction& si,
//       const Bsdf* bsdf) {
//     auto [lightIndex, selectPdf] = scene.SampleLight(sampler->Next1D());
//     const Light* light = scene.GetLight(lightIndex);
//     Interaction start{};
//     start.P = aCameraState.mOrigin;
//     auto [dsr, li] = light->SampleDirection(si, sampler->Next2D());
//     Float directPdfW = light->PdfPosition(dsr);
//     if (dsr.Pdf <= 0) {
//       return Spectrum(0);
//     }
//     BsdfContext ctx{TransportMode::Radiance, (UInt32)BsdfType::All};
//     Vector3 wo = si.ToLocal(dsr.Dir);
//     Spectrum fs = bsdf->Eval(ctx, si, wo);
//     auto tmp = aCameraState.mThroughput.cwiseProduct(li).cwiseProduct(fs) / (dsr.Pdf * selectPdf);
//     Spectrum tmmp(tmp);
//     if (tmmp.IsBlack() || scene.IsOcclude(si, dsr.P)) {
//       return Spectrum(0);
//     }
//     Float bsdfPdfW = bsdf->Pdf(ctx, si, wo);
//     SurfaceInteraction rSi = si;
//     rSi.Wi = wo;
//     Float bsdfRevPdfW = bsdf->Pdf(ctx, rSi, si.Wi);
//     Float cosAtEyeVertex = AbsDot(si.Shading.N, dsr.Dir);
//     Float cosAtLight = AbsDot(dsr.N, dsr.Dir);
//     Float mis0 = dsr.IsDelta ? 0.0f : Mis(bsdfPdfW / directPdfW);
//     Float mis1 = Mis(cosAtEyeVertex * dsr.Pdf / (cosAtLight * directPdfW)) * (aCameraState.dVCM + aCameraState.dVC * Mis(bsdfRevPdfW));
//     Float weight = Float(1) / (mis0 + mis1 + Float(1));
//     return Spectrum(tmmp * weight);
//   }

//   Spectrum ConnectVertex(
//       const Scene& scene,
//       const PathVertex& lightVertex,
//       const SurfaceInteraction& si,
//       const Bsdf* bsdf,
//       const SubPathState& aCameraState) {
//     Vector3 direction = lightVertex.mHitpoint - si.P;
//     Float dist2 = direction.squaredNorm();
//     Float distance = std::sqrt(dist2);
//     direction /= distance;
//     BsdfContext ctx{TransportMode::Radiance, (UInt32)BsdfType::All};
//     Spectrum cameraBsdfFactor = bsdf->Eval(ctx, si, si.ToLocal(direction));
//     Float cameraBsdfDirPdfW = bsdf->Pdf(ctx, si, si.ToLocal(direction));
//     SurfaceInteraction rSi = si;
//     rSi.Wi = si.ToLocal(direction);
//     Float cameraBsdfRevPdfW = bsdf->Pdf(ctx, rSi, si.Wi);
//     Float cosCamera = AbsDot(si.N, direction);
//     if (cameraBsdfFactor.IsBlack()) {
//       return Spectrum(0);
//     }
//     BsdfContext lctx{TransportMode::Importance, (UInt32)BsdfType::All};
//     Spectrum lightBsdfFactor = lightVertex.mBsdf->Eval(lctx, lightVertex.Si, lightVertex.Si.ToLocal(-direction));
//     Float lightBsdfDirPdfW = lightVertex.mBsdf->Pdf(lctx, lightVertex.Si, lightVertex.Si.ToLocal(-direction));
//     rSi = lightVertex.Si;
//     rSi.Wi = lightVertex.Si.ToLocal(-direction);
//     Float lightBsdfRevPdfW = lightVertex.mBsdf->Pdf(lctx, rSi, lightVertex.Si.Wi);
//     if (lightBsdfFactor.IsBlack()) {
//       return Spectrum(0);
//     }
//     Float cosLight = AbsDot(lightVertex.Si.N, -direction);
//     Float geometryTerm = cosLight * cosCamera / dist2;
//     Float cameraBsdfDirPdfA = PdfWtoA(cameraBsdfDirPdfW, distance, cosLight);
//     Float lightBsdfDirPdfA = PdfWtoA(lightBsdfDirPdfW, distance, cosCamera);
//     Float wLight = Mis(cameraBsdfDirPdfA) * (lightVertex.dVCM + lightVertex.dVC * Mis(lightBsdfRevPdfW));
//     Float wCamera = Mis(lightBsdfDirPdfA) * (aCameraState.dVCM + aCameraState.dVC * Mis(cameraBsdfRevPdfW));
//     Float misWeight = Float(1) / (wLight + Float(1) + wCamera);
//     auto contrib = cameraBsdfFactor.cwiseProduct(lightBsdfFactor) * (misWeight * geometryTerm);
//     if (scene.IsOcclude(si, lightVertex.mHitpoint)) {
//       return Spectrum(0);
//     }
//     return Spectrum(contrib);
//   }

//   static Float CorrectShadingNormal(const SurfaceInteraction& si, const Vector3& wo) {
//     Float idn = si.N.dot(si.ToWorld(si.Wi));
//     Float odn = si.N.dot(si.ToWorld(wo));
//     if (idn * Frame::CosTheta(si.Wi) <= 0 || odn * Frame::CosTheta(wo) <= 0) {
//       return 0;
//     }
//     Float correction = (Frame::CosTheta(si.Wi) * odn) / (Frame::CosTheta(wo) * idn);
//     return std::abs(correction);
//   }

//   static Float PdfWtoA(Float aPdfW, Float aDist, Float aCosThere) {
//     return aPdfW * std::abs(aCosThere) / Sqr(aDist);
//   }

//   Int32 _maxDepth;
//   bool _useMis;
// };

}  // namespace Rad

RAD_FACTORY_RENDERER_DECLARATION(BDPT, "bdpt");
