#include <rad/offline/render/camera.h>

#include <rad/offline/common.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>
#include <rad/offline/build.h>
#include <rad/offline/asset/wavefront_obj_reader.h>

namespace Rad {

class ModelObj final : public ModelAsset {
 public:
  ModelObj(BuildContext* ctx, const ConfigNode& cfg) : ModelAsset(ctx, cfg) {}
  ~ModelObj() noexcept override = default;

  Share<TriangleModel> FullModel() const override { return _full; }
  Share<TriangleModel> GetSubModel(const std::string& name) const override {
    for (const auto& sub : _sub) {
      if (sub.name == name) {
        return sub.model;
      }
    }
    return nullptr;
  }
  virtual bool HasSubModel(const std::string& name) const override {
    for (const auto& sub : _sub) {
      if (sub.name == name) {
        return true;
      }
    }
    return false;
  }

  AssetLoadResult Load(const LocationResolver& resolver) override {
    auto stream = resolver.GetStream(_location, 0);
    WavefrontObjReader reader(std::move(stream));
    reader.Read();
    AssetLoadResult result;
    try {
      if (reader.HasError()) {
        result.IsSuccess = false;
        result.FailResult = reader.Error();
      } else {
        _full = std::make_shared<TriangleModel>(reader.ToModel());
        _sub.reserve(reader.Objects().size());
        for (const auto& obj : reader.Objects()) {
          auto name = obj.Name;
          TriangleModel model = reader.ToModel(name);
          auto ptr = std::make_shared<TriangleModel>(std::move(model));
          _sub.emplace_back(SubModel{std::move(name), std::move(ptr)});
        }
        result.IsSuccess = true;
      }
    } catch (std::exception& e) {
      result.IsSuccess = false;
      result.FailResult = e.what();
    }
    return result;
  }

 private:
  struct SubModel {
    std::string name;
    Share<TriangleModel> model;
  };

  Share<TriangleModel> _full;
  std::vector<SubModel> _sub;
};

}  // namespace Rad

RAD_FACTORY_ASSET_DECLARATION(ModelObj, "model_obj");
