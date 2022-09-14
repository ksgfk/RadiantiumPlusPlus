#include <radiantium/factory.h>

#include <radiantium/build_context.h>

#include <radiantium/tracing_accel.h>
#include <radiantium/camera.h>
#include <radiantium/asset.h>
#include <radiantium/shape.h>
#include <radiantium/sampler.h>
#include <radiantium/renderer.h>
#include <radiantium/texture.h>
#include <radiantium/bsdf.h>

namespace rad {
namespace factory_help {

void RegisterSystemFactories(BuildContext* context) {
  /////////////////////////////////
  // Accel
  context->RegisterFactory(rad::factory::CreateEmbreeFactory());
  /////////////////////////////////
  // Camera
  context->RegisterFactory(rad::factory::CreatePerspectiveFactory());
  /////////////////////////////////
  // Asset
  context->RegisterFactory(rad::factory::CreateObjModelFactory());
  context->RegisterFactory(rad::factory::CreateDefaultImageFactory());
  /////////////////////////////////
  // Shape
  context->RegisterFactory(rad::factory::CreateMeshFactory());
  context->RegisterFactory(rad::factory::CreateSphereFactory());
  /////////////////////////////////
  // Sampler
  context->RegisterFactory(rad::factory::CreateIndependentSamplerFactory());
  /////////////////////////////////
  // Renderer
  context->RegisterFactory(rad::factory::CreateAORenderer());
  context->RegisterFactory(rad::factory::CreateGBufferRenderer());
  /////////////////////////////////
  // Texture
  context->RegisterFactory(rad::factory::CreateBitmapFactory());
  context->RegisterFactory(rad::factory::CreateDefaultImageFactory());
  /////////////////////////////////
  // Bsdf
  context->RegisterFactory(rad::factory::CreateDiffuseFactory());
}

}  // namespace factory_help
}  // namespace rad
