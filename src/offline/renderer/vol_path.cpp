#include <rad/offline/render/renderer.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

//#define RAD_IS_CHECK_VOL_PATH_NAN

using namespace Rad::Math;

namespace Rad {

class VolPath final : public SampleRenderer {
 public:
  using WeightMatrix = Eigen::Matrix<Float, Spectrum::ComponentCount, Spectrum::ComponentCount, Eigen::ColMajor>;

  VolPath(BuildContext* ctx, const ConfigNode& cfg) : SampleRenderer(ctx, cfg) {
    _maxDepth = cfg.ReadOrDefault("max_depth", -1);
    _rrDepth = cfg.ReadOrDefault("rr_depth", 5);
  }

  Spectrum Li(const RayDifferential& ray_, const Scene& scene, Sampler* sampler) const override {
    Ray ray = ray_;                            //不使用光线微分
    Medium* medium = scene.GetGlobalMedium();  //路径上的介质
    Spectrum throughput(1);                    //路径吞吐量
    Spectrum result(0);                        //最终结果
    Float eta = 1;                             //路径上由于透射造成的辐射缩放
    Int32 depth = 0;                           //路径深度
    bool isSpecularPath = true;                //是不是delta路径
    Interaction prevIts{};                     //上一个着色点, 可能是表面或者介质里面, 我们用基类来储存就够了
    Float prevPdf = 1;                         //上一个着色点采样的下一条路径的概率密度
    UInt32 channel = SampleChannel(*sampler);  //我们随机使用一条颜色通道来评估介质
    BsdfContext ctx{};
    for (;; depth++) {
      //轮盘赌, 终止随机游走
      if (depth >= _rrDepth) {
        Float maxThroughput = throughput.MaxComponent();
        Float rr = std::min(maxThroughput * Math::Sqr(eta), Float(0.95));
        if (sampler->Next1D() > rr) {
          break;
        }
        throughput *= Math::Rcp(rr);
      }
      if (_maxDepth >= 0 && depth + 1 >= _maxDepth) {
        break;
      }
      //本轮采样需要用到的临时变量
      SurfaceInteraction si{};
      MediumInteraction mei{};
      //采样辐射传输方程(Radiative Transfer Equation)
      bool isSampleMedium = medium != nullptr;
      if (isSampleMedium) {
        mei = medium->SampleInteraction(ray, sampler->Next1D(), channel);
        if (medium->IsHomogeneous() && mei.IsValid()) {  //为均匀介质优化碰撞检测
          ray.MaxT = mei.T;
        }
        scene.RayIntersect(ray, si);
        if (si.T < mei.T) {  //如果在更近的地方撞到了任何物体, 拒绝掉这次采样
          mei.T = std::numeric_limits<Float>::infinity();
        }
        isSampleMedium = mei.IsValid();
        //将透射率应用到路径上
        auto [tr, pdf] = medium->EvalTrAndPdf(mei, si);
        Float freeFlightPdf = pdf[channel];
        throughput = freeFlightPdf <= 0 ? Spectrum(0) : Spectrum(throughput.cwiseProduct(tr) / freeFlightPdf);
#if defined(RAD_IS_CHECK_VOL_PATH_NAN)
        if (throughput.HasNaN()) {
          Logger::Get()->warn("1 tr: {} freeFlightPdf: {}", tr, freeFlightPdf);
        }
#endif
      }
      if (isSampleMedium) {  //本次采样在介质里
        // TODO: 目前只实现了均匀介质, 所以不存在null scatter
        throughput = Spectrum(throughput.cwiseProduct(mei.SigmaS) * mei.CombinedExtinction[channel] / mei.SigmaT[channel]);
        const PhaseFunction& phase = medium->GetPhaseFunction();
        //接下来进行光源重要性采样
        auto [li, dsr] = SampleLight(scene, *sampler, medium, channel, mei);
        if (dsr.Pdf > 0) {
          Float phaseVal = phase.Eval(mei, dsr.Dir);
          Float weight = dsr.IsDelta ? 1 : MisWeight(dsr.Pdf, phaseVal);
          Spectrum lo(throughput.cwiseProduct(li) * weight * phaseVal / dsr.Pdf);
#if defined(RAD_IS_CHECK_VOL_PATH_NAN)
          if (lo.HasNaN()) {
            Logger::Get()->warn("2 li: {} weight: {} phase: {} dsr.Pdf: {}", li, weight, phaseVal, dsr.Pdf);
          }
#endif
          result += lo;
        }
        //采样下一个游走方向
        auto [wo, phasePdf] = phase.Sample(mei, sampler->Next2D());
        //保存下一次散射需要的值
        ray = mei.SpawnRay(wo);
        isSpecularPath = false;
        prevIts = mei;
        prevPdf = phasePdf;
      } else {  //本次采样在表面上
        if (!si.IsValid()) {
          scene.RayIntersect(ray, si);
        }
        std::optional<Light*> hitLight = scene.GetLight(si);
        //直接光照, 随机游走的路径直连光源
        if (hitLight) {
          Float weight;
          // delta路径不启用光源的权重
          if (isSpecularPath) {
            weight = 1;
          } else {
            Float lightPdf = scene.PdfLightDirection(*hitLight, prevIts, si.ToDsr(prevIts));
            weight = MisWeight(prevPdf, lightPdf);
          }
          Spectrum li = (*hitLight)->Eval(si);
          Spectrum lo(throughput.cwiseProduct(li) * weight);
          result += lo;
#if defined(RAD_IS_CHECK_VOL_PATH_NAN)
          if (lo.HasNaN()) {
            Logger::Get()->warn("3 weight: {} li: {}", weight, li);
          }
#endif
        }
        if (!si.IsValid()) {
          break;
        }
        if (si.Shape->IsLight()) {
          break;
        }
        if (!si.Shape->HasBsdf()) {
          ray = si.SpawnRay(ray.D);
          medium = si.GetMedium(ray);
          depth--;
          continue;
        }
        Bsdf* bsdf = si.BSDF(ray);
        if (bsdf->HasAnyTypeExceptDelta()) {
          auto [li, dsr] = SampleLight(scene, *sampler, medium, channel, si);
          if (dsr.Pdf > 0) {
            Vector3 wo = si.ToLocal(dsr.Dir);
            Spectrum f = bsdf->Eval(ctx, si, wo);
            Float bsdfPdf = bsdf->Pdf(ctx, si, wo);
            Float weight = dsr.IsDelta ? 1 : MisWeight(dsr.Pdf, bsdfPdf);
            Spectrum lo(throughput.cwiseProduct(f).cwiseProduct(li) * weight / dsr.Pdf);
            result += lo;
#if defined(RAD_IS_CHECK_VOL_PATH_NAN)
            if (lo.HasNaN()) {
              Logger::Get()->warn("4 weight: {} pdf: {} li: {}", weight, dsr.Pdf, li);
            }
#endif
          }
        }
        auto [bsr, f] = bsdf->Sample(ctx, si, sampler->Next1D(), sampler->Next2D());
        if (bsr.Pdf <= 0) {
          break;
        } else {
          throughput = Spectrum(throughput.cwiseProduct(f) / bsr.Pdf);
          eta *= bsr.Eta;
#if defined(RAD_IS_CHECK_VOL_PATH_NAN)
          if (throughput.HasNaN()) {
            Logger::Get()->warn("5 f: {} pdf: {} bsdf: {}", f, bsr.Pdf, bsdf->Flags());
          }
#endif
        }
        ray = si.SpawnRay(si.ToWorld(bsr.Wo));
        prevPdf = bsr.Pdf;
        prevIts = si;
        isSpecularPath = bsr.HasType(BsdfType::Delta);
        medium = si.GetMedium(ray);
      }
    }
    return result;
  }

  UInt32 SampleChannel(Sampler& sampler) const {
    constexpr UInt32 all = Spectrum::ComponentCount;
    return std::min((UInt32)(sampler.Next1D() * all), all - 1);
  }

  std::pair<Spectrum, DirectionSampleResult> SampleLight(
      const Scene& scene,
      Sampler& sampler,
      Medium* medium,
      UInt32 channel,
      Interaction& refIts) const {
    //采样一个光源
    auto [l, dsr, li] = scene.SampleLightDirection(refIts, sampler.Next1D(), sampler.Next2D());
    //采样失败
    if (dsr.Pdf <= 0) {
      return std::make_pair(Spectrum(0), dsr);
    }
    //计算到光源的透射率
    Ray ray = refIts.SpawnRay(dsr.Dir);
    Float totalDist = 0;            //已经经过的距离
    Spectrum resultTr(1);           //透射率结果
    bool active = true;             //是否继续评估透射率
    bool needsIntersection = true;  //本轮需不需要求交测试
    SurfaceInteraction si{};
    while (active) {
      Float remainingDist = dsr.Dist * (1 - ShadowEpsilon) - totalDist;  //距离光源采样点还有多远
      ray.MaxT = remainingDist;
      active &= remainingDist > 0;  //如果超出距离就结束评估
      if (!active) {
        break;
      }
      bool escapedMedium = false;                       //介质采样是否无效
      bool activeMedium = active && medium != nullptr;  //是否要采样RTE
      bool activeSurface = active && !activeMedium;     //是否检查表面
      if (activeMedium) {
        MediumInteraction mei = medium->SampleInteraction(ray, sampler.Next1D(), channel);
        if (medium->IsHomogeneous() && mei.IsValid()) {  //均匀介质优化
          ray.MaxT = std::min(mei.T, remainingDist);     //限制一下范围
        }
        bool intersect = needsIntersection && activeMedium;
        if (intersect) {
          scene.RayIntersect(ray, si);
        }
        if (si.T < mei.T) {  //如果距离采样点更近的地方有遮挡, 就拒绝掉这次介质采样
          mei.T = std::numeric_limits<Float>::infinity();
        }
        bool isSpectral = medium->HasSpectralExtinction() && activeMedium;
        bool notSpectral = !isSpectral && activeMedium;
        if (isSpectral) {  //具有光谱变化的介质叠加贡献
          Float t = std::min(remainingDist, std::min(mei.T, si.T)) - mei.MinT;
          Spectrum tr = ExpSpectrum(Spectrum(mei.CombinedExtinction * -t));
          Spectrum freeFlightPdf = (si.T < mei.T || mei.T > remainingDist) ? tr : Spectrum(tr.cwiseProduct(mei.CombinedExtinction));
          Float trPdf = freeFlightPdf[channel];
          resultTr = trPdf > 0 ? Spectrum(tr / trPdf) : 0;
        }
        if (activeMedium && (mei.T > remainingDist) && mei.IsValid()) {  //采样有效, 但是超出最大范围了
          totalDist = dsr.Dist;
        }
        if (activeMedium && (mei.T > remainingDist)) {
          mei.T = std::numeric_limits<Float>::infinity();
        }
        escapedMedium = activeMedium && !mei.IsValid();  //这次介质采样是否有效
        activeMedium &= mei.IsValid();
        isSpectral &= activeMedium;
        notSpectral &= activeMedium;
        if (activeMedium) {  //有效的话叠加已经过的距离
          totalDist += mei.T;
        }
        if (activeMedium) {
          ray.O = mei.P;  //设置下一轮光线起点位置
          si.T = si.T - mei.T;
          if (isSpectral) {
            resultTr = Spectrum(resultTr.cwiseProduct(mei.SigmaN));
          }
          if (notSpectral) {
            resultTr = Spectrum(resultTr.cwiseProduct(mei.SigmaN).cwiseProduct(mei.CombinedExtinction.cwiseInverse()));
          }
        }
      }
      bool intersect = activeSurface && needsIntersection;  //如果路径上没有介质, 就检查可见性
      if (intersect) {
        scene.RayIntersect(ray, si);
      }
      needsIntersection &= !intersect;  //如果这轮啥都没检查到, 下一轮还是要检查
      activeSurface |= escapedMedium;   //介质可能采样失败, 这个时候要回退到可见性检查
      if (activeSurface) {
        totalDist += si.T;
      }
      activeSurface &= si.IsValid() && active && !activeMedium;  //如果有碰到任何东西
      if (activeSurface) {
        if (si.Shape->HasBsdf()) {
          resultTr = Spectrum(0);
        }
        ray = si.SpawnRay(ray.D);
      }
      ray.MaxT = remainingDist;
      needsIntersection |= activeSurface;
      active &= (activeMedium || activeSurface) && !resultTr.IsBlack();
      bool has_medium_trans = activeSurface && si.Shape->HasMedia();
      if (has_medium_trans) {
        medium = si.GetMedium(ray);
      }
    }
    Spectrum result(li.cwiseProduct(resultTr));
    return std::make_pair(result, dsr);
  }

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

RAD_FACTORY_RENDERER_DECLARATION(VolPath, "vol_path");
