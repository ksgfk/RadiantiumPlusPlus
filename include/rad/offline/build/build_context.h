#pragma once

#include "../common.h"
#include "../utility/location_resolver.h"
#include "factory.h"

#include <string>
#include <filesystem>

namespace Rad {

class BuildContextImpl;

class RadLoadAssetFailException : public RadException {
 public:
  template <typename... Args>
  RadLoadAssetFailException(const char* fmt, const Args&... args) : RadException(fmt, args...) {}
};

class RAD_EXPORT_API BuildContext {
 public:
  explicit BuildContext(
      const std::filesystem::path& workDir,
      const std::string& projName,
      ConfigNode* cfg);
  ~BuildContext() noexcept;

  BuildContext(const BuildContext&) = delete;
  BuildContext(BuildContext&&) = delete;

  void RegisterFactory(Unique<Factory> factory);
  Factory* GetFactory(const std::string& name) const;
  template <typename T>
  T* GetFactory(const std::string& name) const {
    Factory* ptr = GetFactory(name);
    if (ptr == nullptr) {
      return nullptr;
    }
#ifdef RAD_DEBUG_MODE
    T* cast = dynamic_cast<T*>(ptr);
    if (cast == nullptr) {
      return nullptr;
    }
#else
    T* cast = static_cast<T*>(ptr);
#endif
    return cast;
  }

  ImageAsset* GetImage(const std::string& name) const;
  ModelAsset* GetModel(const std::string& name) const;
  VolumeAsset* GetVolume(const std::string& name) const;

  const Matrix4& GetShapeMatrix() const;
  std::vector<Unique<Shape>>&& GetShapes();
  Unique<Scene> GetScene();

  std::pair<Unique<Renderer>, Unique<LocationResolver>> Build();

 private:
  BuildContextImpl* _impl;
};

}  // namespace Rad
