#pragma once

#include <rad/core/factory.h>
#include <rad/offline/fwd.h>
#include <rad/offline/render/texture.h>

#include <string>

//定义渲染器所需类型的工厂

namespace Rad {

class RAD_EXPORT_API ShapeFactory : public Factory {
 public:
  ShapeFactory(const std::string& name) : Factory(name) {}
  virtual ~ShapeFactory() noexcept = default;
  virtual Unique<Shape> Create(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) const = 0;
};

class RAD_EXPORT_API BsdfFactory : public Factory {
 public:
  BsdfFactory(const std::string& name) : Factory(name) {}
  virtual ~BsdfFactory() noexcept = default;
  virtual Unique<Bsdf> Create(BuildContext* ctx, const ConfigNode& cfg) const = 0;
};

class RAD_EXPORT_API MediumFactory : public Factory {
 public:
  MediumFactory(const std::string& name) : Factory(name) {}
  virtual ~MediumFactory() noexcept = default;
  virtual Unique<Medium> Create(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) const = 0;
};

class RAD_EXPORT_API PhaseFunctionFactory : public Factory {
 public:
  PhaseFunctionFactory(const std::string& name) : Factory(name) {}
  virtual ~PhaseFunctionFactory() noexcept = default;
  virtual Unique<PhaseFunction> Create(BuildContext* ctx, const ConfigNode& cfg) const = 0;
};

class RAD_EXPORT_API VolumeFactory : public Factory {
 public:
  VolumeFactory(const std::string& name) : Factory(name) {}
  virtual ~VolumeFactory() noexcept = default;
  virtual Unique<Volume> Create(BuildContext* ctx, const ConfigNode& cfg) const = 0;
};

class RAD_EXPORT_API LightFactory : public Factory {
 public:
  LightFactory(const std::string& name) : Factory(name) {}
  virtual ~LightFactory() noexcept = default;
  virtual Unique<Light> Create(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) const = 0;
};

class RAD_EXPORT_API CameraFactory : public Factory {
 public:
  CameraFactory(const std::string& name) : Factory(name) {}
  virtual ~CameraFactory() noexcept = default;
  virtual Unique<Camera> Create(BuildContext* ctx, const ConfigNode& cfg) const = 0;
};

class RAD_EXPORT_API SamplerFactory : public Factory {
 public:
  SamplerFactory(const std::string& name) : Factory(name) {}
  virtual ~SamplerFactory() noexcept = default;
  virtual Unique<Sampler> Create(BuildContext* ctx, const ConfigNode& cfg) const = 0;
};

class RAD_EXPORT_API AccelFactory : public Factory {
 public:
  AccelFactory(const std::string& name) : Factory(name) {}
  virtual ~AccelFactory() noexcept = default;
  virtual Unique<Accel> Create(BuildContext* ctx, std::vector<Unique<Shape>> shapes, const ConfigNode& cfg) const = 0;
};

class RAD_EXPORT_API TextureFactory : public Factory {
 public:
  TextureFactory(const std::string& name) : Factory(name) {}
  virtual ~TextureFactory() noexcept = default;
  virtual Unique<TextureRGB> CreateRgb(BuildContext* ctx, const ConfigNode& cfg) const = 0;
  virtual Unique<TextureMono> CreateMono(BuildContext* ctx, const ConfigNode& cfg) const = 0;
};

class RAD_EXPORT_API RendererFactory : public Factory {
 public:
  RendererFactory(const std::string& name) : Factory(name) {}
  virtual ~RendererFactory() noexcept = default;
  virtual Unique<Renderer> Create(BuildContext* ctx, Unique<Scene> scene, const ConfigNode& cfg) const = 0;
};

RAD_EXPORT_API std::vector<std::function<Unique<Factory>(void)>> GetRadOfflineFactories();

}  // namespace Rad
