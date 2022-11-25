#include <rad/offline/render/renderer.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/global_control.h>
#include <tbb/enumerable_thread_specific.h>

using namespace Rad::Math;

namespace Rad {

class ParticleTracer final : public Renderer {
 public:
  ParticleTracer(BuildContext* ctx, const ConfigNode& cfg) : Renderer(ctx, cfg) {
    _maxDepth = cfg.ReadOrDefault("max_depth", -1);
    _rrDepth = cfg.ReadOrDefault("rr_depth", 3);
  }
  ~ParticleTracer() noexcept override = default;

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
    UInt32 actualThreadCount;
    if (_threadCount > 0) {
      ctrl = std::make_unique<tbb::global_control>(tbb::global_control::max_allowed_parallelism, _threadCount);
      actualThreadCount = std::thread::hardware_concurrency();
    } else {
      actualThreadCount = (UInt32)_threadCount;
    }
    UInt32 spp = sampler.SampleCount();
    _allTask = spp;
    Float sampleScale = frameBuffer.size() / ((Float)spp);
    UInt64 grainSize = std::max((UInt64)actualThreadCount / (4 * _allTask), UInt64(1));
    tbb::blocked_range<UInt32> block(0, spp, grainSize);
    std::mutex mutex;
    tbb::enumerable_thread_specific<MatrixX<Spectrum>> tlsData(frameBuffer.rows(), frameBuffer.cols());
    _sw.Start();
    tbb::parallel_for(
        block, [&](const tbb::blocked_range<UInt32>& r) {
          MatrixX<Spectrum>& tempFb = tlsData.local();
          UInt64 completeCount = 0;
          tempFb.setZero();
          completeCount = 0;
          Unique<Sampler> localSampler = sampler.Clone(sampler.GetSeed() + r.begin());
          for (UInt32 i = r.begin(); i < r.end(); i++) {
            Sample(scene, camera, localSampler.get(), tempFb, sampleScale);
            localSampler->Advance();
            completeCount++;
          }
          {
            std::lock_guard<std::mutex> lock(mutex);
            frameBuffer += tempFb;
          }
          _completeTask += completeCount;
        },
        part);
    _sw.Stop();
  }

  void Sample(
      const Scene& scene,
      const Camera& camera,
      Sampler* sampler,
      MatrixX<Spectrum>& image,
      Float sampleScale) {
    if (_maxDepth != 0) {
      SampleLight(scene, camera, sampler, image, sampleScale);
    }
    auto [lightIndex, lightPdf] = scene.SampleLight(sampler->Next1D());
    const Light* light = scene.GetLight(lightIndex);
    auto [ray, li] = light->SampleRay(sampler->Next2D(), sampler->Next2D());
    TraceLightRay(ray, scene, camera, sampler, Spectrum(li / lightPdf), image, sampleScale);
  }

  void SampleLight(
      const Scene& scene,
      const Camera& camera,
      Sampler* sampler,
      MatrixX<Spectrum>& image,
      Float sampleScale) {
    auto [lightIndex, lightPdf] = scene.SampleLight(sampler->Next1D());
    const Light* light = scene.GetLight(lightIndex);
    if (HasFlag(light->Flags(), LightType::Delta)) {
      return;
    }
    SurfaceInteraction si{};
    Spectrum li{};
    if (HasFlag(light->Flags(), LightType::Infinite)) {
      Interaction refIt{};
      refIt.P = camera.WorldPosition();
      DirectionSampleResult dsr;
      std::tie(dsr, li) = light->SampleDirection(refIt, sampler->Next2D());
      lightPdf *= dsr.Pdf > 0 ? (dsr.Pdf * Sqr(dsr.Dist)) : 0;
      si = SurfaceInteraction(dsr);
    } else {
      auto [psr, pdf] = light->SamplePosition(sampler->Next2D());
      lightPdf *= pdf;
      si = SurfaceInteraction(psr);
      li = light->Eval(si);
    }
    auto [dsr, we] = camera.SampleDirection(si, sampler->Next2D());
    auto weight = li.cwiseProduct(we) / lightPdf;
    si.Wi = si.ToLocal(dsr.Dir);
    si.Shape = light->GetShape();
    ConnetCamera(scene, si, dsr, nullptr, Spectrum(weight), image, sampleScale);
  }

  void ConnetCamera(
      const Scene& scene,
      const SurfaceInteraction& si,
      const DirectionSampleResult& cameraSample,
      const Bsdf* bsdf,
      const Spectrum& weight,
      MatrixX<Spectrum>& image,
      Float sampleScale) {
    if (cameraSample.Pdf <= 0 || weight.IsBlack()) {
      return;
    }
    if (scene.IsOcclude(si, cameraSample.P)) {
      return;
    }
    Vector3 toCamera = si.SpawnRayTo(cameraSample.P).D;
    Vector3 wo = si.ToLocal(toCamera);
    Spectrum fs(1);
    if (si.Shape == nullptr) {
      if (Frame::CosTheta(wo) <= 0) {
        fs = Spectrum(0);
      }
    } else {
      if (bsdf == nullptr) {
        fs *= std::max(Float(0), Frame::CosTheta(wo));
      } else {
        BsdfContext ctx{TransportMode::Importance, (UInt32)BsdfType::All};
        Float idn = si.N.dot(si.ToWorld(si.Wi));
        Float odn = si.N.dot(toCamera);
        if (idn * Frame::CosTheta(si.Wi) <= 0 || odn * Frame::CosTheta(wo) <= 0) {
          fs = Spectrum(0);
        } else {
          Float correction = (Frame::CosTheta(si.Wi) * odn) / (Frame::CosTheta(wo) * idn);
          Spectrum bsdfVal = bsdf->Eval(ctx, si, wo);
          auto t = fs.cwiseProduct(bsdfVal) * std::abs(correction) / cameraSample.Pdf;
          fs = Spectrum(t);
        }
      }
    }
    auto ij = weight.cwiseProduct(fs) * sampleScale;
    image((int)cameraSample.UV.x(), (int)cameraSample.UV.y()) += Spectrum(ij);
  }

  void TraceLightRay(
      Ray ray,
      const Scene& scene,
      const Camera& camera,
      Sampler* sampler,
      Spectrum throughput,
      MatrixX<Spectrum>& image,
      Float sampleScale) {
    Float eta(1);
    Int32 depth = 1;
    SurfaceInteraction si;
    bool isHit = scene.RayIntersect(ray, si);
    if (!isHit || !si.Shape->HasBsdf()) {
      return;
    }
    if (_maxDepth >= 0 && depth >= _maxDepth) {
      return;
    }
    for (;;) {
      Bsdf* bsdf = si.BSDF(ray);
      auto [camDsr, we] = camera.SampleDirection(si, sampler->Next2D());
      ConnetCamera(scene, si, camDsr, bsdf, Spectrum(throughput.cwiseProduct(we)), image, sampleScale);
      BsdfContext ctx{TransportMode::Importance, (UInt32)BsdfType::All};
      auto [bsr, fs] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
      Float idn = si.N.dot(-ray.D);
      Float odn = si.N.dot(si.ToWorld(bsr.Wo));
      if (idn * Frame::CosTheta(si.Wi) <= 0 || odn * Frame::CosTheta(bsr.Wo) <= 0) {
        break;
      }
      Float correction = (Frame::CosTheta(si.Wi) * odn) / (Frame::CosTheta(bsr.Wo) * idn);
      throughput = Spectrum(throughput.cwiseProduct(fs) * std::abs(correction) / bsr.Pdf);
      eta *= bsr.Eta;
      if (throughput.IsBlack()) {
        break;
      }
      ray = si.SpawnRay(si.ToWorld(bsr.Wo));
      isHit = scene.RayIntersect(ray, si);
      depth++;
      if (!isHit || !si.Shape->HasBsdf()) {
        break;
      }
      if (_maxDepth >= 0 && depth >= _maxDepth) {
        return;
      }
      if (depth >= _rrDepth) {
        Float maxThroughput = throughput.MaxComponent();
        Float rr = std::min(maxThroughput * Math::Sqr(eta), Float(0.95));
        if (sampler->Next1D() > rr) {
          break;
        }
        throughput *= Math::Rcp(rr);
      }
    }
  }

  Int32 _maxDepth;
  Int32 _rrDepth;
};

}  // namespace Rad

RAD_FACTORY_RENDERER_DECLARATION(ParticleTracer, "particle_tracer");
