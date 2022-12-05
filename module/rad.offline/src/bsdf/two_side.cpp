#include <rad/offline/render/bsdf.h>

#include <rad/core/config_node.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>

#include <optional>

namespace Rad {

/**
 * @brief 双面材质
 * 许多不透明的材质都是单面材质，它们会拒绝法线朝面内的采样和评估
 * 使用这个材质可以反转面法线，来让那些单面材质工作
 */
class TwoSide final : public Bsdf {
 public:
  TwoSide(BuildContext* ctx, const ConfigNode& cfg) {
    auto getBsdfInstance = [](
                               const std::string name,
                               BuildContext* ctx,
                               const ConfigNode& cfg) -> std::optional<Unique<Bsdf>> {
      ConfigNode outsideNode;
      if (cfg.TryRead(name, outsideNode)) {
        std::string type = outsideNode.Read<std::string>("type");
        BsdfFactory* factory = ctx->GetFactoryManager().GetFactory<BsdfFactory>(type);
        Unique<Bsdf> instance = factory->Create(ctx, outsideNode);
        return std::make_optional(std::move(instance));
      } else {
        return std::nullopt;
      }
    };
    if (cfg.HasNode("two_side")) {
      auto instance = getBsdfInstance("two_side", ctx, cfg);
      if (!instance.has_value()) {
        throw Rad::RadArgumentException("config must have {} node", "two_side");
      }
      _resA = std::move(*instance);
      _outside = _resA.get();
      _inside = _resA.get();
    } else {
      auto outside = getBsdfInstance("out_side", ctx, cfg);
      auto inside = getBsdfInstance("in_side", ctx, cfg);
      if (outside.has_value()) {
        _resA = std::move(*outside);
      }
      if (inside.has_value()) {
        _resB = std::move(*inside);
      }
      if (_outside == nullptr && _inside == nullptr) {
        throw Rad::RadArgumentException("config must contains {} or {} at least", "out_side", "in_side");
      }
      if (_outside != nullptr) {
        _inside = _resB.get();
      }
      if (_inside != nullptr) {
        _outside = _resA.get();
      }
    }
    _flags = _inside->Flags() | _outside->Flags();
  }
  ~TwoSide() noexcept override = default;

  std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const override {
    bool isOutside = Frame::CosTheta(si.Wi) > 0;
    bool isInside = Frame::CosTheta(si.Wi) < 0;
    BsdfSampleResult bsr;
    Spectrum f;
    if (isOutside) {
      std::tie(bsr, f) = _outside->Sample(context, si, lobeXi, dirXi);
    } else if (isInside) {
      SurfaceInteraction tsi = si;
      tsi.Wi.z() *= -1;
      BsdfSampleResult tbsr;
      std::tie(tbsr, f) = _inside->Sample(context, tsi, lobeXi, dirXi);
      tbsr.Wo.z() *= -1;
      bsr = tbsr;
    } else {
      bsr = {};
      bsr.Pdf = 0;
      f = Spectrum(0);
    }
    return std::make_pair(bsr, f);
  }

  Spectrum Eval(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    bool isOutside = Frame::CosTheta(si.Wi) > 0;
    bool isInside = Frame::CosTheta(si.Wi) < 0;
    Spectrum f;
    if (isOutside) {
      f = _outside->Eval(context, si, wo);
    } else if (isInside) {
      SurfaceInteraction tsi = si;
      tsi.Wi.z() *= -1;
      Vector3 two = wo;
      two.z() *= -1;
      f = _inside->Eval(context, tsi, two);
    } else {
      f = Spectrum(0);
    }
    return f;
  }

  Float Pdf(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    bool isOutside = Frame::CosTheta(si.Wi) > 0;
    bool isInside = Frame::CosTheta(si.Wi) < 0;
    Float pdf;
    if (isOutside) {
      pdf = _outside->Pdf(context, si, wo);
    } else if (isInside) {
      SurfaceInteraction tsi = si;
      tsi.Wi.z() *= -1;
      Vector3 two = wo;
      two.z() *= -1;
      pdf = _inside->Pdf(context, tsi, two);
    } else {
      pdf = 0;
    }
    return pdf;
  }

 private:
  Unique<Bsdf> _resA;
  Unique<Bsdf> _resB;
  Bsdf* _inside;
  Bsdf* _outside;
};

class TwoSideFactory final : public BsdfFactory {
 public:
  TwoSideFactory() : BsdfFactory("two_side") {}
  ~TwoSideFactory() noexcept override = default;
  Unique<Bsdf> Create(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<TwoSide>(ctx, cfg);
  }
};

Unique<BsdfFactory> _FactoryCreateTwoSideFunc_() {
  return std::make_unique<TwoSideFactory>();
}

}  // namespace Rad
