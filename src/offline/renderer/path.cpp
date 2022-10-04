#include <rad/offline/render/renderer.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

//#define RAD_IS_CHECK_PATH_NAN //有时候路径上会出现一些奇怪的NaN...开这个debug

namespace Rad {

/**
 * @brief 路径追踪
 * 为了计算渲染方程, 路径追踪会从相机胶卷上的一个点发射光线
 * 之后光线会在场景中随机游走, 同时尝试将交点与光源建立光路
 * 使用MIS将直连光源的路径与BSDF路径结合, 可以得到方差更小的结果
 * 无论如何, 只有成功建立从相机到光源的路径才能得出有效结果
 * 如果光线在场景中游走时无法找到光源, 或者无法直接与光源建立连接, 都会使方差变大
 *
 * 此外, delta路径无法应用光源重要性采样的权重, 要正好采样到delta函数有值的方向是不可能的
 * 同样的, 如果光源是delta的也不会启用BSDF重要性采样的权重
 */
class Path final : public SampleRenderer {
 public:
  Path(BuildContext* ctx, const ConfigNode& cfg) : SampleRenderer(ctx, cfg) {
    _maxDepth = cfg.ReadOrDefault("max_depth", -1);
    _rrDepth = cfg.ReadOrDefault("rr_depth", 3);
  }

  Spectrum Li(const Ray& ray_, const Scene& scene, Sampler* sampler) const override {
#if 0
    Ray ray = ray_;
    Spectrum throughput(1);
    Spectrum result(0);
    Float eta = 1;
    Int32 depth = 0;
    BsdfContext ctx{};
    for (;; depth++) {
      SurfaceInteraction si{};
      bool anyHit = scene.RayIntersect(ray, si);
      std::optional<Light*> hitLight = scene.GetLight(si);
      if (hitLight) {
        Spectrum li = (*hitLight)->Eval(si);
        Spectrum le(throughput.cwiseProduct(li));
        result += le;
      }
      if (!anyHit || (_maxDepth >= 0 && depth + 1 >= _maxDepth)) {
        break;
      }
      Bsdf* bsdf = si.Shape->GetBsdf();
      auto [bsr, f] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
      if (bsr.Pdf <= 0) {
        break;
      } else {
        throughput = Spectrum(throughput.cwiseProduct(f) / bsr.Pdf);
        eta *= bsr.Eta;
      }
      ray = si.SpawnRay(si.ToWorld(bsr.Wo));
      if (depth >= _rrDepth) {
        Float maxThroughput = throughput.MaxComponent();
        Float rr = std::min(maxThroughput * Math::Sqr(eta), Float(0.95));
        if (sampler->Next1D() > rr) {
          break;
        }
        throughput *= Math::Rcp(rr);
      }
    }
    return result;
#endif

#if 0
    Ray ray = ray_;
    BsdfContext ctx{};
    Spectrum throughput(1);
    Spectrum result(0);
    Float eta = 1;
    Int32 depth = 0;
    bool isSpecularPath = true;
    for (;; depth++) {
      SurfaceInteraction si{};
      bool anyHit = scene.RayIntersect(ray, si);
      std::optional<Light*> hitLight = scene.GetLight(si);
      if (hitLight && isSpecularPath) {
        Spectrum li = (*hitLight)->Eval(si);
        Spectrum le(throughput.cwiseProduct(li));
        result += le;
      }
      if (!anyHit || (_maxDepth >= 0 && depth + 1 >= _maxDepth)) {
        break;
      }
      if (si.Shape->IsLight()) {
        break;
      }
      Bsdf* bsdf = si.Shape->GetBsdf();
      {
        auto [l, dsr, li] = scene.SampleLightDirection(si, sampler->Next1D(), sampler->Next2D());
        if (dsr.Pdf <= 0) {
          break;
        }
        Vector3 wo = si.ToLocal(dsr.Dir);
        Spectrum f = bsdf->Eval(ctx, si, wo);
        if (!scene.IsOcclude(si, dsr.P)) {
          Spectrum le(throughput.cwiseProduct(f).cwiseProduct(li) / dsr.Pdf);
          result += le;
        }
      }
      {
        auto [bsr, f] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
        if (bsr.Pdf <= 0) {
          break;
        } else {
          throughput = Spectrum(throughput.cwiseProduct(f) / bsr.Pdf);
          eta *= bsr.Eta;
        }
        ray = si.SpawnRay(si.ToWorld(bsr.Wo));
        isSpecularPath = bsr.HasType(BsdfType::Delta);
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
    return result;
#endif

#if 1
    Ray ray = ray_;
    Spectrum throughput(1);       //路径吞吐量
    Spectrum result(0);           //最终结果
    Float eta = 1;                //路径上由于透射造成的辐射缩放
    Int32 depth = 0;              //路径深度
    bool isSpecularPath = true;   //是不是delta路径
    SurfaceInteraction prevSi{};  //上一个着色点
    Float prevBsdfPdf = 1;        //上一个着色点采样的下一条路径的概率密度
    BsdfContext ctx{};
    for (;; depth++) {
      SurfaceInteraction si{};
      bool anyHit = scene.RayIntersect(ray, si);
      std::optional<Light*> hitLight = scene.GetLight(si);
      //直接光照, 随机游走的路径直连光源
      if (hitLight) {
        Float weight;
        // delta路径不启用光源的权重
        if (isSpecularPath) {
          weight = 1;
        } else {
          DirectionSampleResult dsr = si.ToDsr(prevSi);
          Float lightPdf = scene.PdfLightDirection(*hitLight, prevSi, dsr);
          weight = MisWeight(prevBsdfPdf, lightPdf);
        }
        Spectrum li = (*hitLight)->Eval(si);
        Spectrum le(throughput.cwiseProduct(li) * weight);
#if defined(RAD_IS_CHECK_PATH_NAN)
        if (le.HasNaN()) {
          Logger::Get()->warn("1 weight: {} li: {}", weight, li);
        }
#endif
        result += le;
      }
      //没碰到任何物体, 或者深度到达最大值了就强制结束游走
      if (!anyHit || (_maxDepth >= 0 && depth + 1 >= _maxDepth)) {
        break;
      }
      if (si.Shape->IsLight()) {
        break;
      }
      Bsdf* bsdf = si.Shape->GetBsdf();
      if (bsdf->HasAnyTypeExceptDelta()) {  // BSDF只有delta的lobe就不启用光源采样了
        auto [l, dsr, li] = scene.SampleLightDirection(si, sampler->Next1D(), sampler->Next2D());
        if (dsr.Pdf > 0) {
          Vector3 wo = si.ToLocal(dsr.Dir);
          Spectrum f = bsdf->Eval(ctx, si, wo);
          Float bsdfPdf = bsdf->Pdf(ctx, si, wo);
          //可见性测试, 通过测试才能建立有效光路
          if (!scene.IsOcclude(si, dsr.P)) {
            Float weight = dsr.IsDelta ? 1 : MisWeight(dsr.Pdf, bsdfPdf);  //如果是delta的光源则不使用BSDF的权重
            Spectrum le(throughput.cwiseProduct(f).cwiseProduct(li) * weight / dsr.Pdf);
#if defined(RAD_IS_CHECK_PATH_NAN)
            if (le.HasNaN()) {
              Logger::Get()->warn("2 weight: {} pdf: {}", weight, dsr.Pdf);
            }
#endif
            result += le;
          }
        }
      }
      {
        //采样下一条路径
        auto [bsr, f] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
        if (bsr.Pdf <= 0) {
          break;
        } else {
          throughput = Spectrum(throughput.cwiseProduct(f) / bsr.Pdf);
#if defined(RAD_IS_CHECK_PATH_NAN)
          if (throughput.HasNaN()) {
            Logger::Get()->warn("3 f: {} pdf: {} bsdf: {}", f, bsr.Pdf, bsdf->Flags());
          }
#endif
          eta *= bsr.Eta;
        }
        ray = si.SpawnRay(si.ToWorld(bsr.Wo));
        prevBsdfPdf = bsr.Pdf;
        prevSi = si;
        isSpecularPath = bsr.HasType(BsdfType::Delta);
      }
      //轮盘赌, 让随机游走有终止条件
      if (depth >= _rrDepth) {
        Float maxThroughput = throughput.MaxComponent();
        Float rr = std::min(maxThroughput * Math::Sqr(eta), Float(0.95));
        if (sampler->Next1D() > rr) {
          break;
        }
        throughput *= Math::Rcp(rr);
#if defined(RAD_IS_CHECK_PATH_NAN)
        if (throughput.HasNaN()) {
          Logger::Get()->warn("4 rr: {}", Math::Rcp(rr));
        }
#endif
      }
    }
    return result;
#endif
  }

  // power heuristic
  Float MisWeight(Float a, Float b) const {
    a *= a;
    b *= b;
    Float w = a / (a + b);
    return std::isfinite(w) ? w : 0;
  }

 private:
  Int32 _maxDepth;
  Int32 _rrDepth;
};

}  // namespace Rad

RAD_FACTORY_RENDERER_DECLARATION(Path, "path");
