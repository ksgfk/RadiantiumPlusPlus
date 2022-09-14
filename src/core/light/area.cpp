#include <radiantium/light.h>

#include <radiantium/texture.h>
#include <radiantium/frame.h>
#include <radiantium/factory.h>
#include <radiantium/build_context.h>
#include <radiantium/config_node.h>

namespace rad {

class Area : public ILight {
 public:
  Area(std::unique_ptr<ITexture> radiance) : _radiance(std::move(radiance)) {}
  ~Area() noexcept override {}

  UInt32 Flags() const override { return (UInt32)LightFlag::Surface; }
  Spectrum Eval(const SurfaceInteraction& si) const override {
    Float cosThetaI = Frame::CosTheta(si.Wi);
    if (cosThetaI <= 0) {
      return Spectrum(0);
    }
    auto radiance = _radiance->Sample(si.UV);
    return radiance;
  }

 private:
  std::unique_ptr<ITexture> _radiance;
};

}  // namespace rad

namespace rad::factory {
class AreaFactory : public ILightFactory {
 public:
  ~AreaFactory() noexcept override {}
  std::string UniqueId() const override { return "area"; }
  std::unique_ptr<ILight> Create(const BuildContext* context, const IConfigNode* config) const override {
    auto radiance = IConfigNode::GetTexture(
        config, context, "radiance", RgbSpectrum(1));
    return std::make_unique<Area>(std::move(radiance));
  }
};
std::unique_ptr<IFactory> CreateAreaLightFactory() {
  return std::make_unique<AreaFactory>();
}
}  // namespace rad::factory
