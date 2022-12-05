#include <rad/core/asset.h>

#include <rad/core/stop_watch.h>
#include <rad/core/logger.h>

namespace Rad {

Asset::Asset(const AssetManager* ctx, const ConfigNode& cfg, AssetType type) : _type(type) {
  _name = cfg.Read<std::string>("name");
  _location = cfg.Read<std::string>("location");
}

AssetManager::AssetManager() {
  _logger = Logger::GetCategory("asset");
}

AssetLoadResult AssetManager::Load(ConfigNode cfg) {
  Stopwatch sw;
  sw.Start();
  std::string type = cfg.Read<std::string>("type");
  AssetFactory* factory = _factory->GetFactory<AssetFactory>(type);
  Unique<Asset> instance = factory->Create(this, cfg);
  if (IsLoaded(instance->GetName())) {
    return AssetLoadResult{false, "asset is loaded"};
  }
  AssetLoadResult result = instance->Load(*_resolver);
  std::string name = instance->GetName();
  if (result.IsSuccess) {
    if (instance->GetType() == AssetType::Image && _isImageBlock) {
      ImageAsset* imageAsset = static_cast<ImageAsset*>(instance.get());
      imageAsset->GenerateBlockBasedImage();
    }
    _assets.emplace(name, std::move(instance));
  }
  sw.Stop();
  _logger->info("load asset {} ({} ms)", name, sw.ElapsedMilliseconds());
  return result;
}

void AssetManager::Unload(const std::string& name) {
  throw RadNotImplementedException("todo");
}

void AssetManager::GarbageCollect() {
  throw RadNotImplementedException("todo");
}

void AssetManager::SetObjectFactory(const FactoryManager& manager) { _factory = &manager; }

void AssetManager::SetWorkDirectory(const std::filesystem::path& path) {
  _resolver = std::make_unique<LocationResolver>(path);
}

bool AssetManager::IsLoaded(const std::string& name) const {
  return _assets.find(name) != _assets.end();
}

Share<Asset> AssetManager::Reference(const std::string& name) const {
  Share<Asset> refA = _assets.at(name);
  return refA;
}

const Asset* AssetManager::Borrow(const std::string& name) const {
  return _assets.at(name).get();
}

Unique<AssetFactory> _FactoryCreateImageStbFunc_();
Unique<AssetFactory> _FactoryCreateImageExrFunc_();
Unique<AssetFactory> _FactoryCreateModelObjFunc_();
Unique<AssetFactory> _FactoryCreateVolumeVdbFunc_();
Unique<AssetFactory> _FactoryCreateVolumeMitsubaVolFunc_();

std::vector<std::function<Unique<Factory>(void)>> GetRadCoreAssetFactories() {
  return {
      _FactoryCreateImageStbFunc_,
      _FactoryCreateImageExrFunc_,
      _FactoryCreateModelObjFunc_,
      _FactoryCreateVolumeVdbFunc_,
      _FactoryCreateVolumeMitsubaVolFunc_,
  };
}

}  // namespace Rad
