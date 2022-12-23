#include <rad/offline/build/factory.h>

namespace Rad {

Unique<SamplerFactory> _FactoryCreateIndependentFunc_();
Unique<TextureFactory> _FactoryCreateBitmapFunc_();
Unique<TextureFactory> _FactoryCreateChessboardFunc_();
Unique<AccelFactory> _FactoryCreateEmbreeFunc_();
Unique<ShapeFactory> _FactoryCreateCubeFunc_();
Unique<ShapeFactory> _FactoryCreateRectangleFunc_();
Unique<ShapeFactory> _FactoryCreateSphereFunc_();
Unique<ShapeFactory> _FactoryCreateMeshFunc_();
Unique<LightFactory> _FactoryCreateDiffuseAreaFunc_();
Unique<LightFactory> _FactoryCreateSkyboxFunc_();
Unique<LightFactory> _FactoryCreatePointFunc_();
Unique<LightFactory> _FactoryCreateProjectionFunc_();
Unique<BsdfFactory> _FactoryCreateDiffuseFunc_();
Unique<BsdfFactory> _FactoryCreatePerfectGlassFunc_();
Unique<BsdfFactory> _FactoryCreatePerfectMirrorFunc_();
Unique<BsdfFactory> _FactoryCreateTwoSideFunc_();
Unique<BsdfFactory> _FactoryCreateDisneyFunc_();
Unique<BsdfFactory> _FactoryCreatePlasticFunc_();
Unique<BsdfFactory> _FactoryCreateRoughMetalFunc_();
Unique<BsdfFactory> _FactoryCreateRoughPlasticFunc_();
Unique<BsdfFactory> _FactoryCreateRoughGlassFunc_();
Unique<RendererFactory> _FactoryCreatePathFunc_();
Unique<RendererFactory> _FactoryCreateAOFunc_();
Unique<RendererFactory> _FactoryCreateBdptFunc_();
Unique<RendererFactory> _FactoryCreatePTracerFunc_();
Unique<RendererFactory> _FactoryCreateVolPathFunc_();
Unique<CameraFactory> _FactoryCreatePerspectiveFunc_();
Unique<PhaseFunctionFactory> _FactoryCreateIsotropicPhaseFunc_();
Unique<PhaseFunctionFactory> _FactoryCreateHenyeyGreensteinFunc_();
Unique<MediumFactory> _FactoryCreateHomogeneousMediumFunc_();
Unique<MediumFactory> _FactoryCreateHeterogeneousMediumFunc_();
Unique<VolumeFactory> _FactoryCreateVolumeGridFunc_();

std::vector<std::function<Unique<Factory>(void)>> GetRadOfflineFactories() {
  return {
      _FactoryCreateIndependentFunc_,
      _FactoryCreateBitmapFunc_,
      _FactoryCreateChessboardFunc_,
      _FactoryCreateEmbreeFunc_,
      _FactoryCreateCubeFunc_,
      _FactoryCreateRectangleFunc_,
      _FactoryCreateSphereFunc_,
      _FactoryCreateMeshFunc_,
      _FactoryCreateDiffuseAreaFunc_,
      _FactoryCreateSkyboxFunc_,
      _FactoryCreatePointFunc_,
      _FactoryCreateProjectionFunc_,
      _FactoryCreateDiffuseFunc_,
      _FactoryCreatePerfectGlassFunc_,
      _FactoryCreatePerfectMirrorFunc_,
      _FactoryCreateTwoSideFunc_,
      _FactoryCreateDisneyFunc_,
      _FactoryCreatePlasticFunc_,
      _FactoryCreateRoughMetalFunc_,
      _FactoryCreateRoughPlasticFunc_,
      _FactoryCreateRoughGlassFunc_,
      _FactoryCreatePathFunc_,
      _FactoryCreateAOFunc_,
      _FactoryCreateBdptFunc_,
      _FactoryCreatePTracerFunc_,
      _FactoryCreateVolPathFunc_,
      _FactoryCreatePerspectiveFunc_,
      _FactoryCreateIsotropicPhaseFunc_,
      _FactoryCreateHenyeyGreensteinFunc_,
      _FactoryCreateHomogeneousMediumFunc_,
      _FactoryCreateHeterogeneousMediumFunc_,
      _FactoryCreateVolumeGridFunc_,
  };
}

}  // namespace Rad
