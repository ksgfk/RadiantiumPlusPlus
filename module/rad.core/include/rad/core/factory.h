#pragma once

#include "types.h"
#include "config_node.h"

#include <map>

namespace Rad {

/**
 * @brief 工厂方法模式的基类
 */
class RAD_EXPORT_API Factory {
 public:
  Factory(const std::string& name) : _name(name) {}
  virtual ~Factory() noexcept = default;

  const std::string& GetName() const { return _name; }

 protected:
  std::string _name;
};

/**
 * @brief 管理工厂类
 */
class RAD_EXPORT_API FactoryManager {
 public:
  FactoryManager() = default;
  FactoryManager(const FactoryManager&) = delete;
  FactoryManager& operator=(const FactoryManager&) = delete;

  /**
   * @brief 注册一个工厂，工厂名不可以重复
   */
  void RegisterFactory(Unique<Factory> factory);

  /**
   * @brief 获取工厂，如果工厂不存在会抛异常
   */
  Factory* GetFactory(const std::string& name) const;
  template <typename T>
  T* GetFactory(const std::string& name) const {
    Factory* ptr = GetFactory(name);
    if (ptr == nullptr) {
      return nullptr;
    }
#ifdef RAD_DEFINE_DEBUG
    T* cast = dynamic_cast<T*>(ptr);
    if (cast == nullptr) {
      return nullptr;
    }
#else
    T* cast = static_cast<T*>(ptr);
#endif
    return cast;
  }

 private:
  std::map<std::string, Unique<Factory>> _factories;
};

}  // namespace Rad
