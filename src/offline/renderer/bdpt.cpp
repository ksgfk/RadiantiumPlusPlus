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
  enum class VertexType {
    Camera,
    Light,
    Surface
  };

  struct PathVertex {
    VertexType Type{};        //记录顶点的类型
    SurfaceInteraction Si{};  //记录随机游走的信息, 如果顶点是采样的, 位置等信息也放这里面
    Spectrum Throughput{0};   //这个顶点到下一个顶点的吞吐量
    Int32 Depth{0};
    Float PdfFwd{0};           //路径采样算法生成的当前顶点每单位面积的概率
    Float PdfRev{0};           //反向传输时顶点的概率密度
    Float RR{0};               //继续下一个顶点的概率
    bool IsDelta{false};       //到下一个顶点是不是delta函数
    const Light* Li{nullptr};  //如果顶点是采样的, 采样时使用的光源

    const Vector3& p() const { return Si.P; }
    const Vector3& ng() const { return Si.N; }
    const Vector3& ns() const {
      if (Type == VertexType::Surface) {
        return Si.Shading.N;
      } else {
        return Si.N;
      }
    }
    bool IsOnSurface() const { return ng() != Vector3::Zero(); }
    Float ConvertDensity(Float pdf, const PathVertex& next) const {  //把概率密度 立体角转换为面积
      if (next.IsInfiniteLight()) {                                  //无限远定向光源, 无法使用面积密度表示概率
        return pdf;
      }
      Vector3 w = next.p() - p();
      Float dist2 = w.squaredNorm();
      if (dist2 == 0) {
        return 0;
      }
      Float invDist2 = Rcp(dist2);
      if (next.IsOnSurface()) {
        pdf *= AbsDot(next.ng(), w * std::sqrt(invDist2));
      }
      return pdf * invDist2;
    }
    bool IsLight() const {
      return Type == VertexType::Light || (Type == VertexType::Surface && Si.Shape->IsLight());
    }
    const Light* GetLight() const {
      return Type == VertexType::Light ? Li : Si.Shape->GetLight();
    }
    bool IsInfiniteLight() const {
      return Type == VertexType::Light && (HasFlag(Li->Flags(), LightType::Infinite) || HasFlag(Li->Flags(), LightType::DeltaDirection));
    }
    bool IsConnectible() const {  //非delta才能连接
      switch (Type) {
        case VertexType::Camera:
          return true;
        case VertexType::Light:
          return !HasFlag(GetLight()->Flags(), LightType::DeltaDirection);
        case VertexType::Surface:
          return Si.Shape->GetBsdf()->HasAnyTypeExceptDelta();
        default:
          return false;
      }
    }
    Spectrum fs(const PathVertex& next, TransportMode mode) const {  //评估measurement equation
      switch (Type) {
        case VertexType::Surface: {
          Vector3 toNext = (next.p() - p()).normalized();
          Vector3 wo = Si.ToLocal(toNext);
          Float correction = mode == TransportMode::Importance ? CorrectShadingNormal(Si, wo) : Float(1);
          BsdfContext ctx{mode, (UInt32)BsdfType::All};
          Spectrum f = Si.Shape->GetBsdf()->Eval(ctx, Si, wo);
          Spectrum result(f * correction);
          return result;
        }
        default:
          return Spectrum(0);
      }
    }
    Spectrum Le(const Scene& scene, const PathVertex& v) const {
      if (!IsLight()) {
        return Spectrum(0);
      }
      const Light* light = GetLight();
      // if (light->IsEnv()) {
      if (IsInfiniteLight()) {
        Vector3 toV = (v.p() - p()).normalized();
        SurfaceInteraction si{};
        si.Wi = -toV;
        Spectrum le = light->Eval(si);
        return le;
      } else {
        Vector3 toV = (v.p() - p()).normalized();
        SurfaceInteraction tsi = Si;
        tsi.Wi = Si.ToLocal(toV);
        return light->Eval(tsi);
      }
    }
    //采样时选择光源的概率密度
    Float PdfLightOrigin(const Scene& scene, const PathVertex& v) const {
      const Light* light = GetLight();
      // if (light->IsEnv()) {
      if (IsInfiniteLight()) {
        Vector3 w = (v.p() - p()).normalized();
        return InfiniteLightDensity(scene, w);
      } else {
        Float pdfChoice = scene.PdfLight(light);
        Vector3 w = (v.p() - p()).normalized();
        auto [pdfPos, pdfDir] = light->PdfLe(Si.ToPsr(), w);
        return pdfPos * pdfChoice;
      }
    }
    //是Pdf函数里的特殊情况
    Float PdfLight(const Scene& scene, const PathVertex& v) const {
      const Light* light = GetLight();
      Vector3 w = v.p() - p();
      Float invDist2 = 1 / w.squaredNorm();
      w *= std::sqrt(invDist2);
      Float pdf;
      // if (light->IsEnv()) {
      if (IsInfiniteLight()) {
        // BoundingBox3 bound = scene.GetWorldBound();
        // BoundingSphere sph = BoundingSphere::FromBox(bound);
        // sph.Radius = std::max(RayEpsilon, sph.Radius * (1 + ShadowEpsilon));
        // pdf = 1 / (PI * Sqr(sph.Radius));
        auto [pdfPos, pdfDir] = light->PdfLe(PositionSampleResult{}, -w);
        pdf = pdfPos;
      } else {
        auto [pdfPos, pdfDir] = light->PdfLe(Si.ToPsr(), w);
        pdf = pdfDir * invDist2;
      }
      if (v.IsOnSurface()) {
        pdf *= AbsDot(v.ng(), w);
      }
      return pdf;
    }
    //给定前一个顶点上一个顶点, 它会计算下一个顶点的pdf
    Float Pdf(const Scene& scene, const PathVertex* prev, const PathVertex& next) const {
      if (Type == VertexType::Light) {
        return PdfLight(scene, next);
      }
      Vector3 wn = next.p() - p();
      if (wn.squaredNorm() == 0) {
        return 0;
      }
      wn = wn.normalized();
      Vector3 wp;
      if (prev) {
        wp = prev->p() - p();
        if (wp.squaredNorm() == 0) {
          return 0;
        }
        wp = wp.normalized();
      } else {
        Ray ray{p(), wn};
        auto [pdfPos, pdfDir] = scene.GetCamera().PdfWe(ray);
        return ConvertDensity(pdfDir, next);
      }
      SurfaceInteraction rsi = Si;
      rsi.Wi = Si.ToLocal(wp);
      BsdfContext ctx{TransportMode::Radiance, (UInt32)BsdfType::All};
      Float pdf = Si.Shape->GetBsdf()->Pdf(ctx, rsi, Si.ToLocal(wn));
      return ConvertDensity(pdf, next);
    }
  };
  //这玩意的作用是, 临时将值赋值给target, 保存target原本的值, 超出作用域（析构）时将值改回去
  template <typename Type>
  class ScopedAssignment {
   public:
    ScopedAssignment(Type* target = nullptr, Type value = Type())
        : target(target) {
      if (target) {
        backup = *target;
        *target = value;
      }
    }
    ~ScopedAssignment() {
      if (target) *target = backup;
    }
    ScopedAssignment(const ScopedAssignment&) = delete;
    ScopedAssignment& operator=(const ScopedAssignment&) = delete;
    ScopedAssignment& operator=(ScopedAssignment&& other) {
      if (target) *target = backup;
      target = other.target;
      backup = other.backup;
      other.target = nullptr;
      return *this;
    }

   private:
    Type *target, backup;
  };

  BDPT(BuildContext* ctx, const ConfigNode& cfg) : Renderer(ctx, cfg) {
    _maxDepth = cfg.ReadOrDefault("max_depth", -1);
    _rrDepth = cfg.ReadOrDefault("rr_depth", 3);
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
          thread_local std::vector<PathVertex> cameraPath;
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
                cameraPath.clear();
                Vector2 scrPos(x + dist(rng), y + dist(rng));
                Ray ray = camera.SampleRay(scrPos);
                Spectrum li = Li(ray, scene, camera, *localSampler, tempFb, lightPath, cameraPath, Vector2(x, y));
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
          for (UInt32 y = 0; y < tempFb.rows(); y++) {
            for (UInt32 x = 0; x < tempFb.cols(); x++) {
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
      std::vector<PathVertex>& lightPath,
      std::vector<PathVertex>& cameraPath,
      const Vector2& scrPos) {
    //分别从相机和光源生成路径
    GenerateCameraPath(ray, scene, camera, sampler, cameraPath);
    GenerateLightPath(scene, camera, sampler, lightPath);
    //将路径上的顶点连接起来
    Spectrum l(0);
    for (int t = 1; t <= (int)cameraPath.size(); ++t) {
      for (int s = 0; s <= (int)lightPath.size(); ++s) {
        int depth = t + s - 2;
        if ((s == 1 && t == 1) || depth < 0) {
          continue;
        }
        Vector2 screenPos = scrPos;
        Spectrum pathL = ConnectBdpt(scene, camera, sampler, lightPath, cameraPath, s, t, screenPos);
        if (t == 1) {
          img((int)screenPos.x(), (int)screenPos.y()) += pathL;
        } else {
          l += pathL;
        }
      }
    }
    return l;
  }

  void GenerateCameraPath(
      const Ray& ray,
      const Scene& scene,
      const Camera& camera,
      Sampler& sampler,
      std::vector<PathVertex>& cameraPath) {
    auto [pdfPos, pdfDir] = camera.PdfWe(ray);
    PathVertex cameraVertex{};
    cameraVertex.Type = VertexType::Camera;
    cameraVertex.Throughput = Spectrum(1);  //一般来说相机评估值都是均匀的, 从1开始
    cameraVertex.Depth = 0;
    cameraVertex.PdfFwd = 0;
    cameraVertex.PdfRev = 0;
    cameraVertex.RR = 1;
    cameraVertex.Si.P = ray.O;
    cameraVertex.IsDelta = false;  //这里不能是delta, why?
    cameraPath.emplace_back(cameraVertex);
    RandomWalk(scene, camera, sampler, ray, cameraVertex.Throughput, pdfDir, TransportMode::Radiance, cameraPath);
  }

  void GenerateLightPath(
      const Scene& scene,
      const Camera& camera,
      Sampler& sampler,
      std::vector<PathVertex>& lightPath) {
    auto [lightIndex, selectPdf] = scene.SampleLight(sampler.Next1D());
    const Light* light = scene.GetLight(lightIndex);
    auto [ray, le, psr, pdfPos, pdfDir] = light->SampleLe(sampler.Next2D(), sampler.Next2D());
    if (pdfPos <= 0 || pdfDir <= 0 || le.IsBlack()) {
      return;
    }
    Spectrum throughput(le * AbsDot(psr.N, ray.D) / (selectPdf * pdfPos * pdfDir));
    PathVertex lightVertex{};
    lightVertex.Type = VertexType::Light;
    lightVertex.Throughput = le;
    lightVertex.Depth = 0;
    lightVertex.PdfFwd = pdfPos * selectPdf;
    lightVertex.PdfRev = 0;
    lightVertex.RR = 1;
    lightVertex.Si = SurfaceInteraction(psr);
    lightVertex.IsDelta = psr.IsDelta;
    lightVertex.Li = light;
    lightPath.emplace_back(lightVertex);
    RandomWalk(scene, camera, sampler, ray, throughput, pdfDir, TransportMode::Importance, lightPath);
    if (lightVertex.IsInfiniteLight()) {
      if (lightPath.size() > 1) {
        lightPath[1].PdfFwd = pdfPos;
        if (lightPath[1].IsOnSurface()) {
          lightPath[1].PdfFwd *= AbsDot(ray.D, lightPath[1].ng());
        }
      }
      lightPath[0].PdfFwd = InfiniteLightDensity(scene, ray.D);
    }
  }

  void RandomWalk(
      const Scene& scene,
      const Camera& camera,
      Sampler& sampler,
      Ray ray,
      Spectrum throughput,
      Float pdf,
      TransportMode mode,
      std::vector<PathVertex>& path) {
    Int32 depth = 0;
    Float pdfFwd = pdf, pdfRev = 0;
    Float rr = 1, eta = 1;
    for (;; depth++) {
      if (throughput.IsBlack()) {
        break;
      }
      if (depth >= _rrDepth) {
        Float maxThroughput = throughput.MaxComponent();
        Float rrVal = std::min(maxThroughput * Math::Sqr(eta), Float(0.95));
        if (sampler.Next1D() > rrVal) {
          break;
        }
        throughput *= Math::Rcp(rrVal);
        rr *= rrVal;
      }
      SurfaceInteraction si{};
      bool isHit = scene.RayIntersect(ray, si);
      if (!isHit) {
        if (scene.GetEnvLight() != nullptr && mode == TransportMode::Radiance) {  //从相机出发时, 才去捕捉无限远处的灯光
          PathVertex vex{};
          vex.Type = VertexType::Light;
          vex.Li = scene.GetEnvLight();
          vex.Si.P = ray(-1);
          vex.Si.N = -ray.D;
          vex.Throughput = throughput;
          vex.PdfFwd = pdfFwd;
          path.emplace_back(vex);
        }
        break;
      }
      if (!si.Shape->HasBsdf()) {
        break;
      }
      const Bsdf* bsdf = si.BSDF(ray);
      PathVertex& now = path.emplace_back(PathVertex{});
      PathVertex& prev = *(path.rbegin() + 1);
      now.Type = VertexType::Surface;
      now.Si = si;
      now.Depth = depth;
      now.RR = rr;
      now.Throughput = throughput;
      now.PdfRev = 0;
      now.PdfFwd = prev.ConvertDensity(pdfFwd * rr, now);
      if (_maxDepth >= 0 && (depth + 1) > _maxDepth) {
        break;
      }
      if (si.Shape->IsLight()) {
        break;
      }
      BsdfContext ctx{mode, (UInt32)BsdfType::All};
      auto [bsr, fs] = bsdf->Sample(ctx, si, sampler.Next1D(), sampler.Next2D());
      if (bsr.Pdf <= 0) {
        break;
      }
      bool isDelta = HasFlag(bsr.TypeMask, BsdfType::Delta);
      Float correction = mode == TransportMode::Importance ? CorrectShadingNormal(si, bsr.Wo) : Float(1);
      throughput = Spectrum(throughput.cwiseProduct(fs) * correction / bsr.Pdf);
      eta *= bsr.Eta;
      ray = si.SpawnRay(si.ToWorld(bsr.Wo));
      pdfFwd = bsr.Pdf;
      if (isDelta) {
        pdfRev = pdfFwd;
        now.IsDelta = true;
      } else {
        pdfRev = BsdfRevPdf(ctx, si, bsr.Wo);
        now.IsDelta = false;
      }
      prev.PdfRev = now.ConvertDensity(pdfRev * rr, prev);
    }
  }

  Spectrum ConnectBdpt(
      const Scene& scene,
      const Camera& camera,
      Sampler& sampler,
      std::vector<PathVertex>& lightPath,
      std::vector<PathVertex>& cameraPath,
      int s, int t,
      Vector2& scrPos) {
    Spectrum l(0);
    //不可能将 相机路径上的光源 连接到 光源路径上的顶点
    if (t > 1 && s != 0 && cameraPath[t - 1].Type == VertexType::Light) {
      return l;
    }
    PathVertex sampled{};
    if (s == 0) {  //暴力路径追踪
      const PathVertex& pt = cameraPath[t - 1];
      if (pt.IsLight()) {
        Spectrum le = pt.Le(scene, cameraPath[t - 2]);
        l += Spectrum(le.cwiseProduct(pt.Throughput));
      }
    } else if (t == 1) {  //光源开始的路径追踪, 就是particle tracer
      const PathVertex& qs = lightPath[s - 1];
      if (qs.IsConnectible()) {
        auto [dsr, we] = camera.SampleDirection(qs.Si, sampler.Next2D());
        if (dsr.Pdf > 0) {
          sampled.Type = VertexType::Camera;
          sampled.Throughput = Spectrum(we / dsr.Pdf);
          sampled.Si = SurfaceInteraction(dsr);
          sampled.IsDelta = true;  //这里必须是delta, why?
          Spectrum fs = qs.fs(sampled, TransportMode::Importance);
          Spectrum result(qs.Throughput.cwiseProduct(fs).cwiseProduct(sampled.Throughput));
          scrPos = dsr.UV;
          if (!result.IsBlack() && !scene.IsOcclude(qs.Si, dsr.P)) {
            l += result;
          }
        }
      }
    } else if (s == 1) {  //对光源采样的路径追踪
      const PathVertex& pt = cameraPath[t - 1];
      if (pt.IsConnectible()) {
        auto [light, dsr, li] = scene.SampleLightDirection(pt.Si, sampler.Next1D(), sampler.Next2D());
        if (dsr.Pdf > 0) {
          sampled.Type = VertexType::Light;
          sampled.Throughput = Spectrum(li / dsr.Pdf);
          sampled.Si = SurfaceInteraction(dsr);
          sampled.IsDelta = dsr.IsDelta;
          sampled.Li = light;
          sampled.PdfFwd = sampled.PdfLightOrigin(scene, pt);
          Spectrum fs = pt.fs(sampled, TransportMode::Radiance);
          Spectrum result(pt.Throughput.cwiseProduct(fs).cwiseProduct(sampled.Throughput));
          if (!result.IsBlack() && !scene.IsOcclude(pt.Si, dsr.P)) {
            l += result;
          }
        }
      }
    } else {  //连接顶点
      const PathVertex& qs = lightPath[s - 1];
      const PathVertex& pt = cameraPath[t - 1];
      if (qs.IsConnectible() && pt.IsConnectible()) {
        Spectrum lightFs = qs.fs(pt, TransportMode::Importance);
        Spectrum cameraFs = pt.fs(qs, TransportMode::Radiance);
        Float g = G(qs, pt);
        Spectrum result(qs.Throughput.cwiseProduct(lightFs).cwiseProduct(cameraFs).cwiseProduct(pt.Throughput) * g);
        if (!result.IsBlack() && !scene.IsOcclude(pt.Si, qs.p())) {
          l += result;
        }
      }
    }
    if (l.IsBlack()) {
      return l;
    }
    Float mis = MISWeight(scene, lightPath, cameraPath, sampled, s, t);
    Spectrum result(l * mis);
    return result;
  }

  Float MISWeight(
      const Scene& scene,
      std::vector<PathVertex>& lightVertices, std::vector<PathVertex>& cameraVertices,
      PathVertex& sampled, int s, int t) {
    if (s + t == 2) return 1;
    if (_useMis) {
      //直接从pbrt复制的, 根据veach论文里写的, mis权重是一大堆比值...以后再细看吧...
      Float sumRi = 0;
      // Define helper function _remap0_ that deals with Dirac delta functions
      auto remap0 = [](Float f) -> Float { return f != 0 ? f : 1; };

      // Temporarily update vertex properties for current strategy

      // Look up connection vertices and their predecessors
      PathVertex *qs = s > 0 ? &lightVertices[s - 1] : nullptr,
                 *pt = t > 0 ? &cameraVertices[t - 1] : nullptr,
                 *qsMinus = s > 1 ? &lightVertices[s - 2] : nullptr,
                 *ptMinus = t > 1 ? &cameraVertices[t - 2] : nullptr;

      // Update sampled vertex for $s=1$ or $t=1$ strategy
      ScopedAssignment<PathVertex> a1;
      if (s == 1)
        a1 = {qs, sampled};
      else if (t == 1)
        a1 = {pt, sampled};

      // Mark connection vertices as non-degenerate
      ScopedAssignment<bool> a2, a3;
      if (pt) a2 = {&pt->IsDelta, false};
      if (qs) a3 = {&qs->IsDelta, false};

      // Update reverse density of vertex $\pt{}_{t-1}$
      ScopedAssignment<Float> a4;
      if (pt)
        a4 = {&pt->PdfRev, s > 0 ? qs->Pdf(scene, qsMinus, *pt)
                                 : pt->PdfLightOrigin(scene, *ptMinus)};

      // Update reverse density of vertex $\pt{}_{t-2}$
      ScopedAssignment<Float> a5;
      if (ptMinus)
        a5 = {&ptMinus->PdfRev, s > 0 ? pt->Pdf(scene, qs, *ptMinus)
                                      : pt->PdfLight(scene, *ptMinus)};

      // Update reverse density of vertices $\pq{}_{s-1}$ and $\pq{}_{s-2}$
      ScopedAssignment<Float> a6;
      if (qs) a6 = {&qs->PdfRev, pt->Pdf(scene, ptMinus, *qs)};
      ScopedAssignment<Float> a7;
      if (qsMinus) a7 = {&qsMinus->PdfRev, qs->Pdf(scene, pt, *qsMinus)};

      // Consider hypothetical connection strategies along the camera subpath
      Float ri = 1;
      for (int i = t - 1; i > 0; --i) {
        ri *=
            remap0(cameraVertices[i].PdfRev) / remap0(cameraVertices[i].PdfFwd);
        if (!cameraVertices[i].IsDelta && !cameraVertices[i - 1].IsDelta)
          sumRi += ri;
      }

      // Consider hypothetical connection strategies along the light subpath
      ri = 1;
      for (int i = s - 1; i >= 0; --i) {
        ri *= remap0(lightVertices[i].PdfRev) / remap0(lightVertices[i].PdfFwd);
        bool deltaLightvertex = i > 0 ? lightVertices[i - 1].IsDelta
                                      : lightVertices[0].IsDelta;
        if (!lightVertices[i].IsDelta && !deltaLightvertex) sumRi += ri;
      }
      return 1 / (1 + sumRi);
    } else {
      Float sumRi = 0;
      for (int i = t - 1; i > 0; --i) {
        if (!cameraVertices[i].IsDelta && !cameraVertices[i - 1].IsDelta)
          sumRi++;
      }
      for (int i = s - 1; i >= 0; --i) {
        bool deltaLightvertex = i > 0 ? lightVertices[i - 1].IsDelta
                                      : lightVertices[0].IsDelta;
        if (!lightVertices[i].IsDelta && !deltaLightvertex) sumRi++;
      }
      return 1 / (1 + sumRi);
    }
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

  static Float BsdfRevPdf(const BsdfContext& ctx, const SurfaceInteraction& si, const Vector3& wo) {
    SurfaceInteraction rsi = si;
    rsi.Wi = wo;
    return si.Shape->GetBsdf()->Pdf(ctx, rsi, si.Wi);
  }

  static Float G(const PathVertex& v0, const PathVertex& v1) {
    Vector3 d = v0.p() - v1.p();
    Float g = 1 / d.squaredNorm();
    return g;
  }

  static Float InfiniteLightDensity(const Scene& scene, const Vector3& w) {
    const Light* envLight = scene.GetEnvLight();
    DirectionSampleResult dsr{};
    dsr.Dir = -w;
    return scene.PdfLightDirection(envLight, Interaction{}, dsr);
  }

  Int32 _maxDepth;
  Int32 _rrDepth;
  bool _useMis;
};

}  // namespace Rad

RAD_FACTORY_RENDERER_DECLARATION(BDPT, "bdpt");
