#include <radiantium/texture.h>

#include <radiantium/spectrum.h>
#include <radiantium/factory.h>
#include <radiantium/build_context.h>
#include <radiantium/config_node.h>

namespace rad {

class ConstTexture : public ITexture {
 public:
  ConstTexture(const RgbSpectrum& rgb) : _rgb(rgb) {}
  ~ConstTexture() noexcept override {}

  RgbSpectrum Sample(const Vec2& uv) const override { return _rgb; }
  RgbSpectrum Read(UInt32 x, UInt32 y) const override { return _rgb; }

  UInt32 Width() const override { return 1; }
  UInt32 Height() const override { return 1; }

 private:
  const RgbSpectrum _rgb;
};

}  // namespace rad

namespace rad::factory {
class ConstTextureFactory : public ITextureFactory {
 public:
  ~ConstTextureFactory() noexcept override {}
  std::string UniqueId() const override { return "texture_const"; }
  std::unique_ptr<ITexture> Create(const BuildContext* context, const IConfigNode* config) const override {
    Vec3 rgb = config->GetVec3("value", Vec3(Float(0.5), Float(0.5), Float(0.5)));
    return std::make_unique<ConstTexture>(rgb);
  }
};
std::unique_ptr<IFactory> CreateConstTextureFactory() {
  return std::make_unique<ConstTextureFactory>();
}
}  // namespace rad::factory
