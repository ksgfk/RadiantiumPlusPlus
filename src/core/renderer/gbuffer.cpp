#include <radiantium/renderer.h>

#include <radiantium/factory.h>
#include <radiantium/build_context.h>
#include <radiantium/config_node.h>
#include <radiantium/warp.h>
#include <radiantium/math_ext.h>

namespace rad {

class GBuffer : public BlockSampleRenderer {
 public:
  GBuffer(std::unique_ptr<World> world, Int32 spp, Int32 taskCount, const std::string& name)
      : BlockSampleRenderer(std::move(world), spp, taskCount) {
    if (name == "pos") {
      _mode = 0;
    } else if (name == "geo_normal") {
      _mode = 1;
    } else if (name == "shading_normal") {
      _mode = 2;
    } else if (name == "uv") {
      _mode = 3;
    } else if (name == "depth") {
      _mode = 4;
    } else {
      _mode = 2;
    }
  }

  Spectrum Li(const Ray& ray, const World* world, ISampler* sampler) override {
    SurfaceInteraction si;
    if (!world->RayIntersect(ray, si)) {
      return Spectrum(0);
    }
    switch (_mode) {
      case 0: {
        return Spectrum(si.P.cwiseMax(0));
      }
      case 1: {
        return Spectrum(si.N.cwiseAbs());
      }
      case 2: {
        return Spectrum(si.Shading.N.cwiseAbs());
      }
      case 3: {
        return Spectrum(si.UV.x(), si.UV.y(), 0);
      }
      case 4: {
        Float depth = (si.T - ray.MinT) / (ray.MaxT - ray.MinT);
        return Spectrum(depth);
      }
      default:
        return Spectrum(0);
    }
  }

 private:
  int _mode;
};

}  // namespace rad

namespace rad::factory {
class GBufferFactory : public IRendererFactory {
 public:
  ~GBufferFactory() noexcept override {}
  std::string UniqueId() const override { return "gbuffer"; }
  std::unique_ptr<IRenderer> Create(BuildContext* context, const IConfigNode* config) const override {
    Int32 spp = config->GetInt32("spp", 32);
    Int32 threadCount = config->GetInt32("thread_count", -1);
    std::string name = config->GetString("buffer_name", "shading_normal");
    return std::make_unique<GBuffer>(context->MoveWorld(), spp, threadCount, name);
  }
};
std::unique_ptr<IFactory> CreateGBufferRenderer() {
  return std::make_unique<GBufferFactory>();
}
}  // namespace rad::factory
