#pragma once

#include "../common.h"
#include "factory.h"

#include <vector>
#include <functional>

#define RAD_GET_FACTORY_FUNC(factory, type) \
  RadCreate##factory##type##FactoryFunc__

#define RAD_GET_FACTORY_TEXTURE_FUNC(type) \
  RadCreateTexture##type##RgbFactoryFunc__, RadCreateTexture##type##GrayFactoryFunc__

RAD_FACTORY_FUNC_DECLARATION(Shape, Mesh);
RAD_FACTORY_FUNC_DECLARATION(Shape, Sphere);
RAD_FACTORY_FUNC_DECLARATION(Shape, Rectangle);
RAD_FACTORY_FUNC_DECLARATION(Camera, Perspective);
RAD_FACTORY_FUNC_DECLARATION(Accel, Embree);
RAD_FACTORY_FUNC_DECLARATION(Sampler, Independent);
RAD_FACTORY_FUNC_DECLARATION(Renderer, AO);
RAD_FACTORY_FUNC_DECLARATION(Renderer, GBuffer);
RAD_FACTORY_FUNC_DECLARATION(Renderer, Direct);
RAD_FACTORY_FUNC_DECLARATION(Renderer, Path);
RAD_FACTORY_FUNC_DECLARATION(Asset, ModelObj);
RAD_FACTORY_FUNC_DECLARATION(Asset, ImageDefault);
RAD_FACTORY_TEXTURE_FUNC_DECLARATION(Bitmap);
RAD_FACTORY_FUNC_DECLARATION(Light, DiffuseArea);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, Diffuse);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, RoughMetal);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, PerfectMirror);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, PerfectGlass);

namespace Rad::Factories {

inline std::vector<std::function<Unique<Factory>(void)>> GetFactoryCreateFuncs() {
  return {
      RAD_GET_FACTORY_FUNC(Shape, Mesh),
      RAD_GET_FACTORY_FUNC(Shape, Sphere),
      RAD_GET_FACTORY_FUNC(Shape, Rectangle),
      RAD_GET_FACTORY_FUNC(Camera, Perspective),
      RAD_GET_FACTORY_FUNC(Accel, Embree),
      RAD_GET_FACTORY_FUNC(Sampler, Independent),
      RAD_GET_FACTORY_FUNC(Renderer, AO),
      RAD_GET_FACTORY_FUNC(Renderer, GBuffer),
      RAD_GET_FACTORY_FUNC(Renderer, Direct),
      RAD_GET_FACTORY_FUNC(Renderer, Path),
      RAD_GET_FACTORY_FUNC(Asset, ModelObj),
      RAD_GET_FACTORY_FUNC(Asset, ImageDefault),
      RAD_GET_FACTORY_TEXTURE_FUNC(Bitmap),
      RAD_GET_FACTORY_FUNC(Light, DiffuseArea),
      RAD_GET_FACTORY_FUNC(Bsdf, Diffuse),
      RAD_GET_FACTORY_FUNC(Bsdf, RoughMetal),
      RAD_GET_FACTORY_FUNC(Bsdf, PerfectMirror),
      RAD_GET_FACTORY_FUNC(Bsdf, PerfectGlass),
  };
}

}  // namespace Rad::Factories
