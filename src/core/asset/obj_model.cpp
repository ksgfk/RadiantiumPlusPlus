#include <radiantium/asset.h>

#include <radiantium/wavefront_obj_reader.h>
#include <radiantium/factory.h>
#include <radiantium/config_node.h>

namespace rad {

class ModelObj : public IModelAsset {
 public:
  ModelObj(const std::string& location, const std::string& name) : _location(location), _name(name) {}
  ~ModelObj() noexcept override {}

  bool IsValid() const override { return _isValid; }
  const std::string& Name() const override { return _name; }
  size_t SubModelCount() const override { return _subModel.size(); }
  bool HasSubModel(const std::string& name) const override {
    for (const auto& sub : _subModel) {
      if (sub.first == name) {
        return true;
      }
    }
    return false;
  }
  std::shared_ptr<TriangleModel> GetSubModel(const std::string& name) const override {
    for (const auto& sub : _subModel) {
      if (sub.first == name) {
        return sub.second;
      }
    }
    return nullptr;
  }
  std::shared_ptr<TriangleModel> FullModel() const override { return _fullModel; }
  void Load(const LocationResolver& resolver) override {
    auto stream = resolver.GetStream(_location, 0);
    WavefrontObjReader reader(std::move(stream));
    reader.Read();
    if (reader.HasError()) {
      logger::GetLogger()->error("load {} fail. reason:\n{}", _location, reader.Error());
      _isValid = false;
    } else {
      _fullModel = std::make_shared<TriangleModel>(reader.ToModel());
      _subModel.reserve(reader.Objects().size());
      for (const auto& obj : reader.Objects()) {
        const auto& name = obj.Name;
        TriangleModel model;
        reader.ToModel(name, model);
        auto ptr = std::make_shared<TriangleModel>(std::move(model));
        _subModel.emplace_back(std::make_pair(std::move(name), std::move(ptr)));
      }
      _isValid = true;
    }
  }

 private:
  std::string _location;
  std::string _name;
  bool _isValid = false;
  std::vector<std::pair<std::string, std::shared_ptr<TriangleModel>>> _subModel;
  std::shared_ptr<TriangleModel> _fullModel;
};

}  // namespace rad

namespace rad::factory {
class ModelObjFactory : public IAssetFactory {
 public:
  ~ModelObjFactory() noexcept override {}
  std::string UniqueId() const override { return "model_obj"; }
  std::unique_ptr<IAsset> Create(const BuildContext* context, const IConfigNode* config) const override {
    std::string location = config->GetString("location");
    std::string name = config->GetString("name");
    return std::make_unique<ModelObj>(location, name);
  }
};
std::unique_ptr<IFactory> CreateObjModelFactory() {
  return std::make_unique<ModelObjFactory>();
}

}  // namespace rad::factory
