#pragma once

#include "../common.h"
#include "config_node.h"

#include <string>

#define RAD_FACTORY(type)                                                            \
  class type##Factory : public Factory {                                             \
   public:                                                                           \
    type##Factory(const std::string& name) : Factory(name) {}                        \
    virtual ~type##Factory() noexcept = default;                                     \
    virtual Unique<type> Create(BuildContext* ctx, const ConfigNode& cfg) const = 0; \
  };

#define RAD_FACTORY_BASE_DECLARATION(factory, type, name)                                                 \
  class Rad##factory##type##Factory__ : public Rad::factory##Factory {                                    \
   public:                                                                                                \
    Rad##factory##type##Factory__() : factory##Factory(name) {}                                           \
    ~Rad##factory##type##Factory__() noexcept override = default;                                         \
    Rad::Unique<Rad::factory> Create(Rad::BuildContext* ctx, const Rad::ConfigNode& cfg) const override { \
      return std::make_unique<Rad::type>(ctx, cfg);                                                       \
    }                                                                                                     \
  };

#define RAD_FACTORY_TEXTURE_BASE_DECLARATION(type, name)                                                      \
  class RadTexture##type##RgbFactory__ : public Rad::TextureBaseFactory {                                     \
   public:                                                                                                    \
    RadTexture##type##RgbFactory__() : TextureBaseFactory(fmt::format("{}_rgb", name)) {}                     \
    ~RadTexture##type##RgbFactory__() noexcept override = default;                                            \
    Rad::Unique<Rad::TextureBase> Create(Rad::BuildContext* ctx, const Rad::ConfigNode& cfg) const override { \
      return std::make_unique<Rad::type<Rad::Color>>(ctx, cfg);                                               \
    }                                                                                                         \
  };                                                                                                          \
  class RadTexture##type##GrayFactory__ : public Rad::TextureBaseFactory {                                    \
   public:                                                                                                    \
    RadTexture##type##GrayFactory__() : TextureBaseFactory(fmt::format("{}_gray", name)) {}                   \
    ~RadTexture##type##GrayFactory__() noexcept override = default;                                           \
    Rad::Unique<Rad::TextureBase> Create(Rad::BuildContext* ctx, const Rad::ConfigNode& cfg) const override { \
      return std::make_unique<Rad::type<Rad::Float32>>(ctx, cfg);                                             \
    }                                                                                                         \
  };

#define RAD_FACTORY_FUNC_DECLARATION(factory, type) \
  Rad::Unique<Rad::factory##Factory> RadCreate##factory##type##FactoryFunc__();

#define RAD_FACTORY_FUNC_DEFINITION(factory, type)                               \
  Rad::Unique<Rad::factory##Factory> RadCreate##factory##type##FactoryFunc__() { \
    return std::make_unique<Rad##factory##type##Factory__>();                    \
  }

#define RAD_FACTORY_TEXTURE_FUNC_DECLARATION(type)                                 \
  Rad::Unique<Rad::TextureBaseFactory> RadCreateTexture##type##RgbFactoryFunc__(); \
  Rad::Unique<Rad::TextureBaseFactory> RadCreateTexture##type##GrayFactoryFunc__();

#define RAD_FACTORY_TEXTURE_FUNC_DEFINITION(type)                                    \
  Rad::Unique<Rad::TextureBaseFactory> RadCreateTexture##type##RgbFactoryFunc__() {  \
    return std::make_unique<RadTexture##type##RgbFactory__>();                       \
  }                                                                                  \
  Rad::Unique<Rad::TextureBaseFactory> RadCreateTexture##type##GrayFactoryFunc__() { \
    return std::make_unique<RadTexture##type##GrayFactory__>();                      \
  }

#define RAD_FACTORY_SHAPE_DECLARATION(type, name)  \
  RAD_FACTORY_BASE_DECLARATION(Shape, type, name); \
  RAD_FACTORY_FUNC_DEFINITION(Shape, type)

#define RAD_FACTORY_BSDF_DECLARATION(type, name)  \
  RAD_FACTORY_BASE_DECLARATION(Bsdf, type, name); \
  RAD_FACTORY_FUNC_DEFINITION(Bsdf, type)

#define RAD_FACTORY_LIGHT_DECLARATION(type, name)  \
  RAD_FACTORY_BASE_DECLARATION(Light, type, name); \
  RAD_FACTORY_FUNC_DEFINITION(Light, type)

#define RAD_FACTORY_CAMERA_DECLARATION(type, name)  \
  RAD_FACTORY_BASE_DECLARATION(Camera, type, name); \
  RAD_FACTORY_FUNC_DEFINITION(Camera, type)

#define RAD_FACTORY_SAMPLER_DECLARATION(type, name)  \
  RAD_FACTORY_BASE_DECLARATION(Sampler, type, name); \
  RAD_FACTORY_FUNC_DEFINITION(Sampler, type)

#define RAD_FACTORY_ACCEL_DECLARATION(type, name)  \
  RAD_FACTORY_BASE_DECLARATION(Accel, type, name); \
  RAD_FACTORY_FUNC_DEFINITION(Accel, type)

#define RAD_FACTORY_RENDERER_DECLARATION(type, name)  \
  RAD_FACTORY_BASE_DECLARATION(Renderer, type, name); \
  RAD_FACTORY_FUNC_DEFINITION(Renderer, type)

#define RAD_FACTORY_TEXTURE_DECLARATION(type, name) \
  RAD_FACTORY_TEXTURE_BASE_DECLARATION(type, name); \
  RAD_FACTORY_TEXTURE_FUNC_DEFINITION(type)

#define RAD_FACTORY_ASSET_DECLARATION(type, name)  \
  RAD_FACTORY_BASE_DECLARATION(Asset, type, name); \
  RAD_FACTORY_FUNC_DEFINITION(Asset, type)

namespace Rad {

class Factory {
 public:
  Factory(const std::string& name) : _name(name) {}
  virtual ~Factory() noexcept = default;

  const std::string& Name() const { return _name; }

 protected:
  std::string _name;
};

RAD_FACTORY(Shape);
RAD_FACTORY(Bsdf);
RAD_FACTORY(Light);
RAD_FACTORY(Camera);
RAD_FACTORY(Sampler);
RAD_FACTORY(Accel);
RAD_FACTORY(Renderer);
RAD_FACTORY(TextureBase);
RAD_FACTORY(Asset);

}  // namespace Rad