#pragma once

#include "../common.h"
#include "factory.h"

#include <vector>
#include <functional>

#define RAD_GET_FACTORY_FUNC(factory, type) \
  RadCreate##factory##type##FactoryFunc__

#define RAD_GET_FACTORY_TEXTURE_FUNC(type) \
  RadCreateTexture##type##FactoryFunc__

RAD_FACTORY_FUNC_DECLARATION(Shape, Mesh);
RAD_FACTORY_FUNC_DECLARATION(Shape, Sphere);
RAD_FACTORY_FUNC_DECLARATION(Shape, Rectangle);
RAD_FACTORY_FUNC_DECLARATION(Shape, Cube);
RAD_FACTORY_FUNC_DECLARATION(Camera, Perspective);
RAD_FACTORY_FUNC_DECLARATION(Accel, Embree);
RAD_FACTORY_FUNC_DECLARATION(Sampler, Independent);
RAD_FACTORY_FUNC_DECLARATION(Renderer, AO);
RAD_FACTORY_FUNC_DECLARATION(Renderer, GBuffer);
RAD_FACTORY_FUNC_DECLARATION(Renderer, Direct);
RAD_FACTORY_FUNC_DECLARATION(Renderer, Path);
RAD_FACTORY_FUNC_DECLARATION(Renderer, VolPath);
RAD_FACTORY_FUNC_DECLARATION(Renderer, ParticleTracer);
RAD_FACTORY_FUNC_DECLARATION(Renderer, BDPT);
RAD_FACTORY_FUNC_DECLARATION(Asset, ModelObj);
RAD_FACTORY_FUNC_DECLARATION(Asset, ImageDefault);
RAD_FACTORY_FUNC_DECLARATION(Asset, ImageExr);
RAD_FACTORY_FUNC_DECLARATION(Asset, VolumeOpenVdb);
RAD_FACTORY_TEXTURE_FUNC_DECLARATION(Bitmap);
RAD_FACTORY_TEXTURE_FUNC_DECLARATION(Chessboard);
RAD_FACTORY_FUNC_DECLARATION(Light, DiffuseArea);
RAD_FACTORY_FUNC_DECLARATION(Light, Skybox);
RAD_FACTORY_FUNC_DECLARATION(Light, Point);
RAD_FACTORY_FUNC_DECLARATION(Light, Projection);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, Diffuse);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, RoughMetal);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, PerfectMirror);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, PerfectGlass);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, TwoSide);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, RoughGlass);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, Disney);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, Plastic);
RAD_FACTORY_FUNC_DECLARATION(Bsdf, RoughPlastic);
RAD_FACTORY_FUNC_DECLARATION(Medium, HomogeneousMedium);
RAD_FACTORY_FUNC_DECLARATION(Medium, HeterogeneousMedium);
RAD_FACTORY_FUNC_DECLARATION(PhaseFunction, IsotropicPhase);
RAD_FACTORY_FUNC_DECLARATION(Volume, Grid);

namespace Rad::Factories {

inline std::vector<std::function<Unique<Factory>(void)>> GetFactoryCreateFuncs() {
  return {
      RAD_GET_FACTORY_FUNC(Shape, Mesh),
      RAD_GET_FACTORY_FUNC(Shape, Sphere),
      RAD_GET_FACTORY_FUNC(Shape, Rectangle),
      RAD_GET_FACTORY_FUNC(Shape, Cube),
      RAD_GET_FACTORY_FUNC(Camera, Perspective),
      RAD_GET_FACTORY_FUNC(Accel, Embree),
      RAD_GET_FACTORY_FUNC(Sampler, Independent),
      RAD_GET_FACTORY_FUNC(Renderer, AO),
      RAD_GET_FACTORY_FUNC(Renderer, GBuffer),
      RAD_GET_FACTORY_FUNC(Renderer, Direct),
      RAD_GET_FACTORY_FUNC(Renderer, Path),
      RAD_GET_FACTORY_FUNC(Renderer, VolPath),
      RAD_GET_FACTORY_FUNC(Renderer, ParticleTracer),
      RAD_GET_FACTORY_FUNC(Renderer, BDPT),
      RAD_GET_FACTORY_FUNC(Asset, ModelObj),
      RAD_GET_FACTORY_FUNC(Asset, ImageDefault),
      RAD_GET_FACTORY_FUNC(Asset, ImageExr),
      RAD_GET_FACTORY_FUNC(Asset, VolumeOpenVdb),
      RAD_GET_FACTORY_TEXTURE_FUNC(Bitmap),
      RAD_GET_FACTORY_TEXTURE_FUNC(Chessboard),
      RAD_GET_FACTORY_FUNC(Light, DiffuseArea),
      RAD_GET_FACTORY_FUNC(Light, Skybox),
      RAD_GET_FACTORY_FUNC(Light, Point),
      RAD_GET_FACTORY_FUNC(Light, Projection),
      RAD_GET_FACTORY_FUNC(Bsdf, Diffuse),
      RAD_GET_FACTORY_FUNC(Bsdf, RoughMetal),
      RAD_GET_FACTORY_FUNC(Bsdf, PerfectMirror),
      RAD_GET_FACTORY_FUNC(Bsdf, PerfectGlass),
      RAD_GET_FACTORY_FUNC(Bsdf, TwoSide),
      RAD_GET_FACTORY_FUNC(Bsdf, RoughGlass),
      RAD_GET_FACTORY_FUNC(Bsdf, Disney),
      RAD_GET_FACTORY_FUNC(Bsdf, Plastic),
      RAD_GET_FACTORY_FUNC(Bsdf, RoughPlastic),
      RAD_GET_FACTORY_FUNC(Medium, HomogeneousMedium),
      RAD_GET_FACTORY_FUNC(Medium, HeterogeneousMedium),
      RAD_GET_FACTORY_FUNC(PhaseFunction, IsotropicPhase),
      RAD_GET_FACTORY_FUNC(Volume, Grid),
  };
}

}  // namespace Rad::Factories
