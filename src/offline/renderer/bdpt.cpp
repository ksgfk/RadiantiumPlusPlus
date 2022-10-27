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
  struct PathState {
    Ray WoRay;
    Spectrum Throughput;
    Int32 Length;
    bool IsFiniteLight;
    bool IsSpecularPath;
    Float VCM;
    Float VC;
    Float Eta;
    Float RR;
  };

  struct PathVertex {
    SurfaceInteraction Si;
    Vector3 Throughput;
    Int32 Length;
    Float VCM;
    Float VC;
    Float RR;
  };

  BDPT(BuildContext* ctx, const ConfigNode& cfg) : Renderer(ctx, cfg) {
    _maxDepth = cfg.ReadOrDefault("max_depth", 5);
    _rrDepth = cfg.ReadOrDefault("rr_depth", 3);
    _useMis = cfg.ReadOrDefault("use_mis", true);
    _isOnlyLightTrace = cfg.ReadOrDefault("only_light_trace", false);
    _isOnlyPathTrace = cfg.ReadOrDefault("only_path_trace", false);
    _lightCount = (Float)_scene->GetCamera().GetFrameBuffer().size();
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
                Eigen::Vector2i sp((int)x, (int)y);
                Ray ray = camera.SampleRay(scrPos);
                Spectrum li = Li(ray, scene, camera, *localSampler, tempFb, lightPath);
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
          Float coeff = Float(1) / sampler.SampleCount();
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
      const Ray& ray,
      const Scene& scene,
      const Camera& camera,
      Sampler& sampler,
      MatrixX<Spectrum>& img,
      std::vector<PathVertex>& lightPath) {
    if (!_isOnlyPathTrace) {
      PathState lightState = GenerateLightSample(scene, sampler);
      for (;; ++lightState.Length) {
        SurfaceInteraction si{};
        const bool isHit = scene.RayIntersect(lightState.WoRay, si);
        if (!isHit) {
          break;  //这里不用管env, 已经是从光源出发了
        }
        if (!si.Shape->HasBsdf()) {
          break;
        }
        const Bsdf* bsdf = si.BSDF(lightState.WoRay);
        const Float cosThetaFix = Frame::CosTheta(si.Wi);
        {
          if (lightState.Length > 1 || lightState.IsFiniteLight) {
            lightState.VCM *= Mis(Sqr(si.T));
          }
          lightState.VCM /= Mis(std::abs(cosThetaFix));
          lightState.VC /= Mis(std::abs(cosThetaFix));
        }
        if (bsdf->HasAnyTypeExceptDelta()) {
          PathVertex lightVertex{};
          lightVertex.Si = si;
          lightVertex.Throughput = lightState.Throughput;
          lightVertex.Length = lightState.Length;
          lightVertex.VCM = lightState.VCM;
          lightVertex.VC = lightState.VC;
          lightVertex.RR = lightState.RR;
          lightPath.emplace_back(lightVertex);
          ConnectToCamera(scene, camera, sampler, lightState, lightVertex, img);
        }
        if (_maxDepth >= 0 && lightState.Length + 2 > _maxDepth) {
          break;
        }
        if (!SampleScattering(sampler, lightState, TransportMode::Importance, si)) {
          break;
        }
        if (lightState.Length >= _rrDepth) {
          Float maxThroughput = lightState.Throughput.MaxComponent();
          Float rr = std::min(maxThroughput * Math::Sqr(lightState.Eta), Float(0.95));
          if (sampler.Next1D() > rr) {
            break;
          }
          lightState.Throughput *= Math::Rcp(rr);
          lightState.RR *= rr;
        }
      }
    }
    Spectrum color(0);
    {
      PathState cameraState = GenerateCameraSample(scene, camera, sampler, ray);
      for (;; cameraState.Length++) {
        SurfaceInteraction si{};
        bool isHit = scene.RayIntersect(cameraState.WoRay, si);
        if (!isHit) {
          const Light* env = scene.GetEnvLight();
          std::optional<const Light*> l = env == nullptr ? std::nullopt : std::make_optional(env);
          Spectrum le = GetLightRadiance(l, scene, cameraState, si);
          color += Spectrum(cameraState.Throughput.cwiseProduct(le));
          break;
        }
        if (!si.Shape->HasBsdf()) {
          break;
        }
        const Bsdf* bsdf = si.BSDF(cameraState.WoRay);
        const Float cosThetaFix = Frame::CosTheta(si.Wi);
        {
          cameraState.VCM *= Mis(Sqr(si.T));
          cameraState.VCM /= Mis(std::abs(cosThetaFix));
          cameraState.VC /= Mis(std::abs(cosThetaFix));
        }
        if (si.Shape->IsLight()) {
          const Light* light = si.Shape->GetLight();
          std::optional<const Light*> l = std::make_optional(light);
          Spectrum le = GetLightRadiance(l, scene, cameraState, si);
          color += Spectrum(cameraState.Throughput.cwiseProduct(le));
          break;
        }
        if (_isOnlyLightTrace) {
          break;
        }
        if (_maxDepth >= 0 && cameraState.Length >= _maxDepth) {
          break;
        }
        if (bsdf->HasAnyTypeExceptDelta()) {
          Spectrum le = DirectIllumination(scene, sampler, cameraState, si);
          color += Spectrum(cameraState.Throughput.cwiseProduct(le));
          for (const PathVertex& lightVertex : lightPath) {
            Spectrum connectResult = ConnectVertex(scene, cameraState, si, lightVertex);
            color += connectResult;
          }
        }
        if (!SampleScattering(sampler, cameraState, TransportMode::Radiance, si)) {
          break;
        }
        if (cameraState.Length >= _rrDepth) {
          Float maxThroughput = cameraState.Throughput.MaxComponent();
          Float rr = std::min(maxThroughput * Math::Sqr(cameraState.Eta), Float(0.95));
          if (sampler.Next1D() > rr) {
            break;
          }
          cameraState.Throughput *= Math::Rcp(rr);
          cameraState.RR *= rr;
        }
      }
    }
    return color;
  }

  PathState GenerateLightSample(const Scene& scene, Sampler& sampler) {
    //选取一个光源
    auto [lightIndex, lightPickProb] = scene.SampleLight(sampler.Next1D());
    const Light* light = scene.GetLight(lightIndex);
    auto [ray, throughput, emissionPdfW, directPdfA, cosLight] = light->Emit(sampler.Next2D(), sampler.Next2D());
    emissionPdfW *= lightPickProb;
    directPdfA *= lightPickProb;
    PathState oLightState{};
    oLightState.Throughput = Spectrum(throughput / emissionPdfW);
    oLightState.WoRay = ray;
    oLightState.Length = 1;
    oLightState.IsFiniteLight = !HasFlag(light->Flags(), LightType::Infinite);
    {
      oLightState.VCM = Mis(directPdfA / emissionPdfW);
      if (!HasFlag(light->Flags(), LightType::Delta)) {
        const float usedCosLight = oLightState.IsFiniteLight ? cosLight : Float(1);
        oLightState.VC = Mis(usedCosLight / emissionPdfW);
      } else {
        oLightState.VC = Float(0);
      }
    }
    oLightState.IsSpecularPath = true;
    oLightState.Eta = 1;
    oLightState.RR = 1;
    return oLightState;
  }

  PathState GenerateCameraSample(
      const Scene& scene,
      const Camera& camera,
      Sampler& sampler,
      const Ray& primaryRay) {
    auto [cameraPdfA, cameraPdfW] = camera.PdfWe(primaryRay);
    PathState oCameraState{};
    oCameraState.Throughput = Spectrum(1);
    oCameraState.WoRay = primaryRay;
    oCameraState.Length = 1;
    oCameraState.IsSpecularPath = true;
    oCameraState.VCM = Mis(sampler.SampleCount() / cameraPdfW);
    oCameraState.VC = 0;
    oCameraState.Eta = 1;
    oCameraState.RR = 1;
    return oCameraState;
  }

  void ConnectToCamera(
      const Scene& scene,
      const Camera& camera,
      Sampler& sampler,
      const PathState& aLightState,
      const PathVertex& v,
      MatrixX<Spectrum>& img) {
    const SurfaceInteraction& si = v.Si;
    auto [dsr, we] = camera.SampleDirection(si, sampler.Next2D());
    if (dsr.Pdf <= 0 || we.IsBlack()) {
      return;
    }
    Vector3 toCamera = (camera.WorldPosition() - si.P).normalized();
    Vector3 wo = si.ToLocal(toCamera);
    const Bsdf* bsdf = si.Shape->GetBsdf();
    BsdfContext ctx{TransportMode::Importance, (UInt32)BsdfType::All};
    Spectrum fs = bsdf->Eval(ctx, si, wo);
    auto [bsdfFwdPdf, bsdfRevPdf] = BsdfPdf(ctx, si, wo);
    if (bsdfFwdPdf <= 0 || fs.IsBlack()) {
      return;
    }
    bsdfRevPdf *= aLightState.RR;
    Float cameraPdfA = we[0];
    Float wLight = Mis(cameraPdfA / sampler.SampleCount()) * (aLightState.VCM + aLightState.VC * Mis(bsdfRevPdf));
    Float misWeight = _isOnlyLightTrace ? Float(1) : (Float(1) / (wLight + Float(1)));
    Spectrum ij(aLightState.Throughput.cwiseProduct(fs).cwiseProduct(we) * misWeight / dsr.Pdf);
    if (ij.IsBlack()) {
      return;
    }
    if (scene.IsOcclude(si, dsr.P)) {
      return;
    }
    img((int)dsr.UV.x(), (int)dsr.UV.y()) += ij;
  }

  Float Mis(float aPdf) const {
    return _useMis ? aPdf : 1;
  }

  bool SampleScattering(
      Sampler& sampler,
      PathState& state,
      TransportMode mode,
      const SurfaceInteraction& si) {
    const Bsdf* bsdf = si.Shape->GetBsdf();
    BsdfContext ctx{mode, (UInt32)BsdfType::All};
    auto [bsr, fs] = bsdf->Sample(ctx, si, sampler.Next1D(), sampler.Next2D());
    if (bsr.Pdf <= 0) {
      return false;
    }
    Float correction = mode == TransportMode::Importance ? CorrectShadingNormal(si, bsr.Wo) : Float(1);
    state.Throughput = Spectrum(state.Throughput.cwiseProduct(fs) * correction / bsr.Pdf);
    state.Eta *= bsr.Eta;
    if (state.Throughput.IsBlack()) {
      return false;
    }
    state.WoRay = si.SpawnRay(si.ToWorld(bsr.Wo));
    bool isDelta = HasFlag(bsdf->Flags(), BsdfType::Delta);
    Float bsdfRevPdf;
    if (isDelta) {
      bsdfRevPdf = bsr.Pdf;
    } else {
      SurfaceInteraction rSi = si;
      rSi.Wi = bsr.Wo;
      bsdfRevPdf = bsdf->Pdf(ctx, rSi, si.Wi);
    }
    Float cosThetaOut = Frame::AbsCosTheta(bsr.Wo);
    if (isDelta) {
      state.VCM = 0.f;
      state.VC *= Mis(cosThetaOut);
      state.IsSpecularPath = true;
    } else {
      state.VC = Mis(cosThetaOut / bsr.Pdf) * (state.VC * Mis(bsdfRevPdf) + state.VCM);
      state.VCM = Mis(Float(1) / bsr.Pdf);
      state.IsSpecularPath = false;
    }
    return true;
  }

  Spectrum GetLightRadiance(
      std::optional<const Light*> l,
      const Scene& scene,
      const PathState& cameraState,
      const SurfaceInteraction& si) {
    if (!l.has_value()) {
      return Spectrum(0);
    }
    const Light* light = *l;
    Float selectPdf = scene.PdfLight(light);
    Interaction refPoint{0, cameraState.WoRay.O, Vector3{}};
    auto [li, directPdfA, emissionPdfW] = light->GetRadiance(refPoint, si);
    if (li.IsBlack()) {
      return Spectrum(0);
    }
    if (cameraState.Length == 1) {
      return li;
    }
    directPdfA *= selectPdf;
    emissionPdfW *= selectPdf;
    Float wCamera = Mis(directPdfA) * cameraState.VCM + Mis(emissionPdfW) * cameraState.VC;
    Float misWeight = _isOnlyPathTrace ? Float(1) : (Float(1) / (Float(1) + wCamera));
    Spectrum result(li * misWeight);
    return result;
  }

  Spectrum DirectIllumination(
      const Scene& scene,
      Sampler& sampler,
      const PathState& cameraState,
      const SurfaceInteraction& si) {
    auto [lightIndex, lightPickProb] = scene.SampleLight(sampler.Next1D());
    const Light* light = scene.GetLight(lightIndex);
    auto [dsr, li, directPdfW, emissionPdfW, cosAtLight] = light->Illuminate(si, sampler.Next2D());
    if (dsr.Pdf <= 0 || li.IsBlack()) {
      return Spectrum(0);
    }
    Vector3 wo = si.ToLocal(dsr.Dir);
    const Bsdf* bsdf = si.Shape->GetBsdf();
    BsdfContext ctx{TransportMode::Radiance, (UInt32)BsdfType::All};
    Spectrum fs = bsdf->Eval(ctx, si, wo);
    auto [bsdfDirPdfW, bsdfRevPdfW] = BsdfPdf(ctx, si, wo);
    Float cosToLight = Frame::AbsCosTheta(wo);
    Float continuationProbability = cameraState.RR;
    bsdfDirPdfW *= HasFlag(light->Flags(), LightType::Delta) ? Float(0) : continuationProbability;
    bsdfRevPdfW *= continuationProbability;
    Float wLight = Mis(bsdfDirPdfW / (lightPickProb * directPdfW));
    Float wCamera = Mis(emissionPdfW * cosToLight / (directPdfW * cosAtLight)) * (cameraState.VCM + cameraState.VC * Mis(bsdfRevPdfW));
    Float misWeight = _isOnlyPathTrace ? Float(1) : (Float(1) / (wLight + Float(1) + wCamera));
    Spectrum result(fs.cwiseProduct(li) * misWeight / (lightPickProb * dsr.Pdf));
    if (result.IsBlack() || scene.IsOcclude(si, dsr.P)) {
      return Spectrum(0);
    }
    return result;
  }

  Spectrum ConnectVertex(
      const Scene& scene,
      const PathState& cameraState,
      const SurfaceInteraction& si,
      const PathVertex& lightVertex) {
    Vector3 direction = lightVertex.Si.P - si.P;
    Float dist2 = direction.squaredNorm();
    Float distance = std::sqrt(dist2);
    direction /= distance;

    const Bsdf* camBsdf = si.Shape->GetBsdf();
    BsdfContext camCtx{TransportMode::Radiance, (UInt32)BsdfType::All};
    Vector3 localToLight = si.ToLocal(direction);
    Float cosCamera = Frame::AbsCosTheta(localToLight);
    Spectrum camFs = camBsdf->Eval(camCtx, si, localToLight);
    auto [cameraBsdfDirPdfW, cameraBsdfRevPdfW] = BsdfPdf(camCtx, si, localToLight);
    if (camFs.IsBlack()) {
      return Spectrum(0);
    }
    Float cameraCont = cameraState.RR;
    cameraBsdfDirPdfW *= cameraCont;
    cameraBsdfRevPdfW *= cameraCont;

    const Bsdf* liBsdf = lightVertex.Si.Shape->GetBsdf();
    BsdfContext liCtx{TransportMode::Importance, (UInt32)BsdfType::All};
    Vector3 localFromLight = lightVertex.Si.ToLocal(-direction);
    Float cosLight = Frame::AbsCosTheta(localFromLight);
    Spectrum liFs = liBsdf->Eval(liCtx, lightVertex.Si, localFromLight);
    auto [lightBsdfDirPdfW, lightBsdfRevPdfW] = BsdfPdf(liCtx, lightVertex.Si, localFromLight);
    if (liFs.IsBlack()) {
      return Spectrum(0);
    }
    Float lightCont = lightVertex.RR;
    lightBsdfDirPdfW *= lightCont;
    lightBsdfRevPdfW *= lightCont;

    Float geometryTerm = cosLight * cosCamera / dist2;
    if (geometryTerm < 0) {
      return Spectrum(0);
    }

    Float cameraBsdfDirPdfA = PdfWtoA(cameraBsdfDirPdfW, distance, cosLight);
    Float lightBsdfDirPdfA = PdfWtoA(lightBsdfDirPdfW, distance, cosCamera);
    Float wLight = Mis(cameraBsdfDirPdfA) * (lightVertex.VCM + lightVertex.VC * Mis(lightBsdfRevPdfW));
    Float wCamera = Mis(lightBsdfDirPdfA) * (cameraState.VCM + cameraState.VC * Mis(cameraBsdfRevPdfW));
    Float misWeight = Float(1) / (wLight + Float(1) + wCamera);
    Spectrum result(camFs.cwiseProduct(liFs) * misWeight * geometryTerm);
    if (result.IsBlack() || scene.IsOcclude(si, lightVertex.Si.P)) {
      return Spectrum(0);
    }
    return result;
  }

  static Float CorrectShadingNormal(const SurfaceInteraction& si, const Vector3& wo) {
    Float idn = si.N.dot(si.ToWorld(si.Wi));
    Float odn = si.N.dot(si.ToWorld(wo));
    if (idn * Frame::CosTheta(si.Wi) <= 0 || odn * Frame::CosTheta(wo) <= 0) {
      return 0;
    }
    Float correction = (Frame::CosTheta(si.Wi) * odn) / (Frame::CosTheta(wo) * idn);
    return std::abs(correction);
  }

  static std::pair<Float, Float> BsdfPdf(const BsdfContext& ctx, const SurfaceInteraction& si, const Vector3& wo) {
    const Bsdf* bsdf = si.Shape->GetBsdf();
    Float bsdfFwdPdf = bsdf->Pdf(ctx, si, wo);
    SurfaceInteraction rSi = si;
    rSi.Wi = wo;
    Float bsdfRevPdf = bsdf->Pdf(ctx, rSi, si.Wi);
    return std::make_pair(bsdfFwdPdf, bsdfRevPdf);
  }

  static Float PdfWtoA(Float aPdfW, Float aDist, Float aCosThere) {
    return aPdfW * std::abs(aCosThere) / Sqr(aDist);
  }

  Int32 _maxDepth;
  Int32 _rrDepth;
  Float _lightCount;
  bool _useMis;
  bool _isOnlyLightTrace;
  bool _isOnlyPathTrace;
};

// class BDPT final : public Renderer {
//  public:
//   enum class VertexType {
//     Surface,
//     Camera,
//     Light
//   };
//   struct PathVertex {
//     VertexType Type;
//     Spectrum Throughput;
//     Float PdfFwd{0};
//     Float PdfRev{0};

//     SurfaceInteraction Si{};
//     bool IsDelta{false};

//     const Light* L{nullptr};

//     Vector3& P() { return Si.P; }
//     Vector3& GeoN() { return Si.N; }
//     const Vector3& P() const { return Si.P; }
//     const Vector3& GeoN() const { return Si.N; }
//     bool IsLight() const {
//       return Type == VertexType::Light || (Type == VertexType::Surface && Si.Shape->IsLight());
//     }
//     bool IsInfiniteLight() const {
//       return Type == VertexType::Light &&
//              (L != nullptr || HasFlag(L->Flags(), LightType::Infinite) ||
//               HasFlag(L->Flags(), LightType::DeltaDirection));
//     }
//     Spectrum Le(const Scene& scene, PathVertex& v) const {
//       if (!IsLight()) {
//         return Spectrum(0);
//       }
//       Vector3 w = v.P() - P();
//       Float dist2 = w.squaredNorm();
//       if (dist2 == 0) {
//         return Spectrum(0);
//       }
//       w /= std::sqrt(dist2);
//       if (IsInfiniteLight()) {
//         return Spectrum(0);  // TODO: 评估无限远的灯光
//       } else {
//         SurfaceInteraction tSi = Si;
//         tSi.Wi = Si.ToLocal(-w);
//         return Si.Shape->GetLight()->Eval(Si);
//       }
//     }
//     bool IsConnectible() const {
//       switch (Type) {
//         case VertexType::Surface:
//           return Si.Shape->GetBsdf()->HasAnyTypeExceptDelta();
//         case VertexType::Camera:
//           return true;
//         case VertexType::Light:
//           return !HasFlag(L->Flags(), LightType::DeltaDirection);
//         default:
//           return false;
//       }
//     }
//     bool IsDeltaLight() const {
//       return Type == VertexType::Light && L != nullptr && HasFlag(L->Flags(), LightType::Delta);
//     }
//     Spectrum Fs(const PathVertex& next, TransportMode mode) const {
//       BsdfContext ctx{mode, (UInt32)BsdfType::All};
//       Vector3 worldWo = (next.P() - P()).normalized();
//       Vector3 wo = Si.ToLocal(worldWo);
//       if (Type == VertexType::Surface) {
//         Spectrum fs = Si.Shape->GetBsdf()->Eval(ctx, Si, wo);
//         if (mode == TransportMode::Importance) {
//           Float correct = CorrectShadingNormal(Si, wo);
//           return Spectrum(fs * correct);
//         } else {
//           return fs;
//         }
//       } else {
//         return Spectrum(0);
//       }
//     }
//     Float Pdf(
//         const Scene& scene,
//         const Camera& camera,
//         const PathVertex* prev,
//         const PathVertex& next) const {
//       if (Type == VertexType::Light) {
//         return PdfLight(scene, next);
//       }
//       Vector3 wn = next.P() - P();
//       if (wn.squaredNorm() == 0) {
//         return 0;
//       }
//       wn = wn.normalized();

//       Vector3 wp;
//       if (prev != nullptr) {
//         wp = prev->P() - P();
//         if (wp.squaredNorm() == 0) {
//           return 0;
//         }
//         wp = wp.normalized();
//       }
//       Float pdf = 0;
//       if (Type == VertexType::Camera) {
//         auto [_, pdfDir] = camera.PdfWe(Ray{Si.P, wn, 0, std::numeric_limits<Float>::max()});
//         pdf = pdfDir;
//       } else if (Type == VertexType::Surface) {
//         BsdfContext ctx{};  // pdf并不会检查传输模式
//         SurfaceInteraction tSi = Si;
//         tSi.Wi = Si.ToLocal(wp);
//         pdf = Si.Shape->GetBsdf()->Pdf(ctx, tSi, tSi.ToLocal(wn));
//       }
//       return PdfSolidAngleToArea(P(), pdf, next.P(), next.GeoN());
//     }

//     Float PdfLight(const Scene& scene, const PathVertex& v) const {
//       Vector3 w = v.P() - P();
//       Float invDist2 = Rcp(w.squaredNorm());
//       w *= std::sqrt(invDist2);
//       if (IsInfiniteLight()) {
//         return 0;
//       } else {
//         const Light* light = Type == VertexType::Light ? L : Si.Shape->GetLight();
//         auto [pdfPos, pdfDir] = light->PdfLe(Si.ToPsr(), w);
//         if (v.GeoN() != Vector3::Zero()) {
//           pdfDir *= AbsDot(v.GeoN(), w);
//         }
//         return pdfDir * invDist2;
//       }
//     }
//   };

//   //这玩意的作用是, 临时替换掉一个值, 并在超出作用域后恢复现场
//   template <typename Type>
//   class ScopedAssignment {
//    public:
//     ScopedAssignment(Type* target = nullptr, Type value = Type())
//         : target(target) {
//       if (target) {
//         backup = *target;
//         *target = value;
//       }
//     }
//     ~ScopedAssignment() {
//       if (target) *target = backup;
//     }
//     ScopedAssignment(const ScopedAssignment&) = delete;
//     ScopedAssignment& operator=(const ScopedAssignment&) = delete;
//     ScopedAssignment& operator=(ScopedAssignment&& other) {
//       if (target) *target = backup;
//       target = other.target;
//       backup = other.backup;
//       other.target = nullptr;
//       return *this;
//     }

//    private:
//     Type *target, backup;
//   };

//   BDPT(BuildContext* ctx, const ConfigNode& cfg) : Renderer(ctx, cfg) {
//     _maxDepth = cfg.ReadOrDefault("max_depth", 5);
//     _visualizeStrategies = cfg.ReadOrDefault("visualize_strategies", false);
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
//     for (int depth = 0; depth <= _maxDepth; ++depth) {
//       for (int s = 0; s <= depth + 2; ++s) {
//         int t = depth + 2 - s;
//         if (t == 0 || (s == 1 && t == 1)) {
//           continue;
//         }
//         const auto& buffer = _weightBuffer[BufferIndex(s, t)];
//         resolver.SaveOpenExr(buffer, fmt::format("d{}_s{}_t{}", depth, s, t));
//       }
//     }
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
//     if (_visualizeStrategies) {
//       int bufferCount = (1 + _maxDepth) * (6 + _maxDepth) / 2;
//       _weightBuffer.resize(bufferCount);
//       for (int depth = 0; depth <= _maxDepth; ++depth) {
//         for (int s = 0; s <= depth + 2; ++s) {
//           int t = depth + 2 - s;
//           if (t == 0 || (s == 1 && t == 1)) {
//             continue;
//           }
//           MatrixX<Spectrum> buffer(frameBuffer.rows(), frameBuffer.cols());
//           _weightBuffer[BufferIndex(s, t)] = std::move(buffer);
//         }
//       }
//     }
//     _sw.Start();
//     tbb::parallel_for(
//         block, [&](const tbb::blocked_range2d<UInt32>& r) {
//           UInt32 seed = r.rows().begin() * camera.Resolution().x() + r.cols().begin();
//           thread_local auto rng = std::mt19937(seed);
//           thread_local std::vector<PathVertex> lightPath;
//           thread_local std::vector<PathVertex> cameraPath;
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
//                 cameraPath.clear();
//                 Vector2 scrPos(x + dist(rng), y + dist(rng));
//                 Eigen::Vector2i sp((int)x, (int)y);
//                 Ray ray = camera.SampleRay(scrPos);
//                 Spectrum li = Li(ray, scene, camera, *localSampler, tempFb, cameraPath, lightPath, sp);
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
//           Float coeff = Float(1) / sampler.SampleCount();
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
//     if (_visualizeStrategies) {
//       Float coeff = Float(1) / _scene->GetCamera().GetSampler().SampleCount();
//       for (size_t i = 0; i < _weightBuffer.size(); i++) {
//         auto& b = _weightBuffer[i];
//         for (UInt32 y = 0; y < b.cols(); y++) {
//           for (UInt32 x = 0; x < b.rows(); x++) {
//             b(y, x) *= coeff;
//           }
//         }
//       }
//     }
//   }

//   Spectrum Li(
//       const Ray& ray,
//       const Scene& scene,
//       const Camera& camera,
//       Sampler& sampler,
//       MatrixX<Spectrum>& img,
//       std::vector<PathVertex>& cameraPath,
//       std::vector<PathVertex>& lightPath,
//       const Eigen::Vector2i& sp) {
//     GenerateCameraSubpath(scene, sampler, camera, ray, cameraPath);
//     GenerateLightPath(scene, sampler, lightPath);
//     Spectrum l(0);
//     // t和s表示使用顶点的数量
//     for (int t = 1; t <= (int)cameraPath.size(); t++) {
//       for (int s = 0; s <= (int)lightPath.size(); s++) {
//         int depth = t + s - 2;  //至少要使用两个顶点才能连成一个边
//         if ((s == 1 && t == 1) || depth < 0 || depth > _maxDepth) {
//           continue;
//         }
//         Vector2 filmPos = sp.cast<Float>();
//         Float mis = 0;
//         Spectrum pathL = Connect(scene, sampler, camera, cameraPath, lightPath, s, t, filmPos, mis);
//         if (!pathL.IsBlack()) {
//           if (_visualizeStrategies) {
//             std::lock_guard<std::mutex> lock(_wbLock);
//             Spectrum vlu = mis == 0 ? Spectrum(0) : Spectrum(pathL / mis);
//             _weightBuffer[BufferIndex(s, t)].coeffRef((int)filmPos.x(), (int)filmPos.y()) += vlu;
//           }
//           if (t != 1) {
//             l += pathL;
//           } else {
//             img((int)filmPos.x(), (int)filmPos.y()) += pathL;
//           }
//         }
//       }
//     }
//     return l;
//   }

//   void GenerateCameraSubpath(
//       const Scene& scene,
//       Sampler& sampler,
//       const Camera& camera,
//       const Ray& ray,
//       std::vector<PathVertex>& cameraPath) {
//     auto [pdfPos, pdfDir] = camera.PdfWe(ray);
//     PathVertex v{};
//     v.Type = VertexType::Camera;
//     v.P() = ray.O;
//     v.Throughput = Spectrum(1);  //和普通路径追踪起点是1一个道理, 但是其它类型的相机似乎有其他的初始值
//     cameraPath.emplace_back(v);
//     RandomWalk(scene, sampler, ray, v.Throughput, pdfDir, TransportMode::Radiance, cameraPath);
//   }

//   void GenerateLightPath(
//       const Scene& scene,
//       Sampler& sampler,
//       std::vector<PathVertex>& lightPath) {
//     //随机选择一个光源
//     auto [lightIndex, selectPdf] = scene.SampleLight(sampler.Next1D());
//     const Light* light = scene.GetLight(lightIndex);
//     auto [psr, ray, le, pdfDir] = light->SampleLe(sampler.Next2D(), sampler.Next2D());
//     Float pdfPos = psr.Pdf;
//     if (pdfPos <= 0 || pdfDir <= 0 || le.IsBlack()) {
//       return;
//     }
//     PathVertex v{};
//     v.Type = VertexType::Light;
//     v.Si = SurfaceInteraction(psr);
//     v.IsDelta = psr.IsDelta;
//     v.Throughput = le;
//     v.PdfFwd = pdfPos * selectPdf;
//     v.L = light;
//     lightPath.emplace_back(v);
//     Spectrum throughput(le * AbsDot(psr.N, ray.D) / (pdfPos * selectPdf * pdfDir));
//     RandomWalk(scene, sampler, ray, throughput, pdfDir, TransportMode::Importance, lightPath);
//     // TODO: 这里忽略掉天空盒
//   }

//   void RandomWalk(
//       const Scene& scene,
//       Sampler& sampler,
//       Ray ray,
//       Spectrum throughput,
//       Float pdf,
//       TransportMode mode,
//       std::vector<PathVertex>& path) {
//     //正向和反向概率密度
//     Float pdfFwd = pdf, pdfRev = 0;
//     int depth = 0;
//     BsdfContext ctx{mode, (UInt32)BsdfType::All};
//     while (true) {
//       SurfaceInteraction si{};
//       bool isHit = scene.RayIntersect(ray, si);
//       if (!isHit) {
//         // TODO: 这里忽略掉天空盒
//         break;
//       }
//       if (!si.Shape->HasBsdf()) {
//         break;
//       }
//       PathVertex& vertex = path.emplace_back(PathVertex{});
//       PathVertex& prev = *(path.rbegin() + 1);
//       Bsdf* bsdf = si.BSDF(ray);
//       vertex.Type = VertexType::Surface;
//       vertex.Si = si;
//       vertex.PdfFwd = PdfSolidAngleToArea(prev.P(), pdfFwd, si.P, si.N);
//       vertex.Throughput = throughput;
//       if (++depth >= _maxDepth) {
//         break;
//       }
//       auto [bsr, fs] = bsdf->Sample(ctx, si, sampler.Next1D(), sampler.Next2D());
//       if (bsr.Pdf <= 0) {
//         break;
//       }
//       Float correct = mode == TransportMode::Importance ? CorrectShadingNormal(si, bsr.Wo) : Float(1);
//       throughput = Spectrum(throughput.cwiseProduct(fs) * correct / bsr.Pdf);
//       bool sampleDelta = bsr.HasType(BsdfType::Delta);
//       if (sampleDelta) {
//         vertex.IsDelta = true;
//         pdfFwd = 0;
//         pdfRev = 0;
//       } else {
//         pdfFwd = bsr.Pdf;
//         SurfaceInteraction revSi = si;
//         revSi.Wi = bsr.Wo;
//         pdfRev = bsdf->Pdf(ctx, revSi, si.Wi);
//       }
//       ray = si.SpawnRay(si.ToWorld(bsr.Wo));
//       prev.PdfRev = PdfSolidAngleToArea(si.P, pdfRev, prev.P(), prev.GeoN());
//     }
//   }

//   Spectrum Connect(
//       const Scene& scene,
//       Sampler& sampler,
//       const Camera& camera,
//       std::vector<PathVertex>& cameraPath,
//       std::vector<PathVertex>& lightPath,
//       int s, int t,
//       Vector2& filmPos,
//       Float& mis) {
//     // TODO: 这里忽略掉天空盒的一些判断
//     Spectrum l(0);
//     PathVertex sampled{};
//     if (s == 0) {  //不使用来自光源的路径, 只使用相机路径
//       const PathVertex& pt = cameraPath[t - 1];
//       if (pt.IsLight()) {  //相当于普通路径追踪里打中光源的情况
//         Spectrum le = pt.Le(scene, cameraPath[t - 2]);
//         l = Spectrum(le.cwiseProduct(pt.Throughput));
//       }
//     } else if (t == 1) {  // light trace
//       const PathVertex& qs = lightPath[s - 1];
//       if (qs.IsConnectible()) {  //实际上是排除delta direction情况的光源
//         auto [dsr, wi] = camera.SampleDirection(qs.Si, sampler.Next2D());
//         if (dsr.Pdf > 0 && !wi.IsBlack() && !scene.IsOcclude(qs.Si, dsr.P)) {
//           sampled.Type = VertexType::Camera;
//           sampled.Si = SurfaceInteraction(dsr);
//           sampled.Throughput = Spectrum(wi / dsr.Pdf);
//           Spectrum fs = qs.Fs(sampled, TransportMode::Importance);
//           l = Spectrum(qs.Throughput.cwiseProduct(fs).cwiseProduct(sampled.Throughput));
//           filmPos = dsr.UV;
//         }
//       }
//     } else if (s == 1) {  //在光源上采样一个点, 并将它连接到相机路径上, 像是路径追踪的光源重要性采样
//       const PathVertex& pt = cameraPath[t - 1];
//       if (pt.IsConnectible()) {  //实际上是排除delta bsdf
//         auto [lightIndex, selectPdf] = scene.SampleLight(sampler.Next1D());
//         const Light* light = scene.GetLight(lightIndex);
//         auto [dsr, li] = light->SampleDirection(pt.Si, sampler.Next2D());
//         if (dsr.Pdf > 0 && !li.IsBlack() && !scene.IsOcclude(pt.Si, dsr.P)) {
//           sampled.Type = VertexType::Light;
//           sampled.Si = SurfaceInteraction(dsr);
//           sampled.IsDelta = dsr.IsDelta;
//           sampled.Throughput = Spectrum(li / (selectPdf * dsr.Pdf));
//           sampled.L = light;
//           sampled.PdfFwd = PdfLightOrigin(scene, sampled, pt.P());
//           Spectrum fs = pt.Fs(sampled, TransportMode::Radiance);
//           l = Spectrum(pt.Throughput.cwiseProduct(fs).cwiseProduct(sampled.Throughput));
//         }
//       }
//     } else {  //这种情况就是光源路径和相机路径中间的点
//       const PathVertex& qs = lightPath[s - 1];
//       const PathVertex& pt = cameraPath[t - 1];
//       if (qs.IsConnectible() && pt.IsConnectible()) {
//         Spectrum fsrev = qs.Fs(pt, TransportMode::Importance);
//         Spectrum fsfwd = pt.Fs(qs, TransportMode::Radiance);
//         Float g = G(scene, qs, pt);
//         l = Spectrum(qs.Throughput.cwiseProduct(fsrev).cwiseProduct(fsfwd).cwiseProduct(pt.Throughput) * g);
//       }
//     }
//     if (l.IsBlack()) {
//       return Spectrum(0);
//     }
//     mis = MisWeight(scene, camera, cameraPath, lightPath, s, t, sampled);
//     Spectrum result(l * mis);
//     return result;
//   }

//   Float MisWeight(
//       const Scene& scene,
//       const Camera& camera,
//       std::vector<PathVertex>& cameraPath,
//       std::vector<PathVertex>& lightPath,
//       int s, int t,
//       PathVertex& sampled) {
//     if (s + t == 2) {
//       return 1;
//     }
//     Float sumRi = 0;
//     //将delta函数映射为1
//     auto remap0 = [](Float f) -> Float { return f != 0 ? f : 1; };
//     PathVertex *qs = s > 0 ? &lightPath[s - 1] : nullptr,
//                *pt = t > 0 ? &cameraPath[t - 1] : nullptr,
//                *qsMinus = s > 1 ? &lightPath[s - 2] : nullptr,
//                *ptMinus = t > 1 ? &cameraPath[t - 2] : nullptr;

//     // s==1 或 t==1 时, 会在相机/光源上采样一个顶点, 我们需要把这个顶点替换掉
//     ScopedAssignment<PathVertex> a1;
//     if (s == 1) {
//       a1 = {qs, sampled};
//     } else if (t == 1) {
//       a1 = {pt, sampled};
//     }

//     ScopedAssignment<bool> a2, a3;
//     if (pt) {
//       a2 = {&pt->IsDelta, false};
//     }
//     if (qs) {
//       a3 = {&qs->IsDelta, false};
//     }
//     // pt_{t-1}
//     ScopedAssignment<Float> a4;
//     if (pt) {
//       if (s > 0) {
//         a4 = {&pt->PdfRev, qs->Pdf(scene, camera, qsMinus, *pt)};
//       } else {
//         a4 = {&pt->PdfRev, PdfLightOrigin(scene, *pt, ptMinus->P())};
//       }
//     }
//     // pt_{t-2}
//     ScopedAssignment<Float> a5;
//     if (ptMinus) {
//       if (s > 0) {
//         a5 = {&ptMinus->PdfRev, pt->Pdf(scene, camera, qs, *ptMinus)};
//       } else {
//         a5 = {&ptMinus->PdfRev, pt->PdfLight(scene, *ptMinus)};
//       }
//     }
//     //$\pq{}_{s-1}$ and $\pq{}_{s-2}$
//     ScopedAssignment<Float> a6;
//     if (qs) a6 = {&qs->PdfRev, pt->Pdf(scene, camera, ptMinus, *qs)};
//     ScopedAssignment<Float> a7;
//     if (qsMinus) a7 = {&qsMinus->PdfRev, qs->Pdf(scene, camera, pt, *qsMinus)};

//     Float ri = 1;
//     for (int i = t - 1; i > 0; --i) {
//       ri *= remap0(cameraPath[i].PdfRev) / remap0(cameraPath[i].PdfFwd);
//       if (!cameraPath[i].IsDelta && !cameraPath[i - 1].IsDelta) {
//         sumRi += ri;
//       }
//     }
//     ri = 1;
//     for (int i = s - 1; i >= 0; --i) {
//       ri *= remap0(lightPath[i].PdfRev) / remap0(lightPath[i].PdfFwd);
//       bool deltaLightvertex = i > 0 ? lightPath[i - 1].IsDelta
//                                     : lightPath[0].IsDeltaLight();
//       if (!lightPath[i].IsDelta && !deltaLightvertex) {
//         sumRi += ri;
//       }
//     }
//     return 1 / (1 + sumRi);
//   }

//   static Float PdfSolidAngleToArea(
//       const Vector3& nowP,
//       Float pdf,
//       const Vector3& nextP,
//       const Vector3& nextN) {
//     Vector3 w = nextP - nowP;
//     Float rcpDist2 = Rcp(w.squaredNorm());
//     if (nextN.isZero()) {
//       return pdf * rcpDist2;
//     } else {
//       return pdf * rcpDist2 * AbsDot(nextN, w * std::sqrt(rcpDist2));
//     }
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

//   static Float PdfLightOrigin(
//       const Scene& scene,
//       const PathVertex& lightVert,
//       const Vector3& refP) {
//     Vector3 w = refP - lightVert.P();
//     Float dist2 = w.squaredNorm();
//     if (dist2 == 0) {
//       return 0;
//     }
//     w /= std::sqrt(dist2);
//     if (lightVert.IsInfiniteLight()) {
//       return 0;  // TODO: 忽略环境光
//     } else {
//       const Light* light = lightVert.Type == VertexType::Light ? lightVert.L : lightVert.Si.Shape->GetLight();
//       Float selectPdf = scene.PdfLight(light);
//       Float pdfPos = light->PdfPosition(lightVert.Si.ToPsr());
//       return pdfPos * selectPdf;
//     }
//   }

//   static Float G(
//       const Scene& scene,
//       const PathVertex& v0,
//       const PathVertex& v1) {
//     if (scene.IsOcclude(v0.Si, v1.P())) {
//       return 0;
//     }
//     Float g = Rcp((v0.P() - v1.P()).norm());
//     return g;
//   }

//   static int BufferIndex(int s, int t) {
//     int above = s + t - 2;
//     return s + above * (5 + above) / 2;
//   }

//   Int32 _maxDepth;
//   bool _visualizeStrategies;
//   std::vector<MatrixX<Spectrum>> _weightBuffer;
//   std::mutex _wbLock;
// };

}  // namespace Rad

RAD_FACTORY_RENDERER_DECLARATION(BDPT, "bdpt");
