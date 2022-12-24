#include <rad/offline/render/bsdf.h>

#include <rad/core/config_node.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>
#include <rad/offline/build/config_node_ext.h>

namespace Rad {

class MaskBsdf final : public Bsdf {
 public:
  MaskBsdf(BuildContext* ctx, const ConfigNode& cfg) {
    _opacity = ConfigNodeReadTexture(ctx, cfg, "opacity", 0.5f);
    {
      ConfigNode nestedBsdf = cfg.Read<ConfigNode>("nested");
      std::string type = nestedBsdf.Read<std::string>("type");
      BsdfFactory* factory = ctx->GetFactoryManager().GetFactory<BsdfFactory>(type);
      Unique<Bsdf> instance = factory->Create(ctx, nestedBsdf);
      _nested = std::move(instance);
    }
    _flags = _nested->Flags();
  }
  ~MaskBsdf() noexcept override = default;

  std::pair<BsdfSampleResult, Spectrum> Sample(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      Float lobeXi, const Vector2& dirXi) const override {
    Float opacity = EvalOpacity(si);
    BsdfSampleResult bsr{};
    Spectrum result{};
    if (lobeXi < opacity) {
      lobeXi /= opacity;
      std::tie(bsr, result) = _nested->Sample(context, si, lobeXi, dirXi);
      bsr.Pdf *= opacity;
      result = Spectrum(result * opacity);
    } else {
      result = Spectrum(1);
      bsr.Pdf = 1 - opacity;
      bsr.Eta = 1;
      bsr.Wo = -si.Wi;
      bsr.TypeMask = (UInt32)BsdfType::Null;
    }
    return std::make_pair(bsr, result);
  }

  Spectrum Eval(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    Float opacity = EvalOpacity(si);
    Spectrum f = _nested->Eval(context, si, wo);
    return Spectrum(f * opacity);
  }

  Float Pdf(
      const BsdfContext& context,
      const SurfaceInteraction& si,
      const Vector3& wo) const override {
    Float opacity = EvalOpacity(si);
    Float pdf = _nested->Pdf(context, si, wo);
    return pdf * opacity;
  }

 private:
  Float EvalOpacity(const SurfaceInteraction& si) const {
    return std::clamp(_opacity->Eval(si), Float(0), Float(1));
  }

  Unique<TextureMono> _opacity;
  Unique<Bsdf> _nested;
};

class MaskBsdfFactory final : public BsdfFactory {
 public:
  MaskBsdfFactory() : BsdfFactory("mask") {}
  ~MaskBsdfFactory() noexcept override = default;
  Unique<Bsdf> Create(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<MaskBsdf>(ctx, cfg);
  }
};

Unique<BsdfFactory> _FactoryCreateMaskBsdfFunc_() {
  return std::make_unique<MaskBsdfFactory>();
}

}  // namespace Rad
