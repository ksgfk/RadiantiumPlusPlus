#include <radiantium/factory.h>

#include <radiantium/build_context.h>

#include <radiantium/tracing_accel.h>
#include <radiantium/camera.h>
#include <radiantium/asset.h>
#include <radiantium/shape.h>
#include <radiantium/sampler.h>
#include <radiantium/renderer.h>

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
  /////////////////////////////////
  // Shape
  context->RegisterFactory(rad::factory::CreateMeshFactory());
  /////////////////////////////////
  // Sampler
  context->RegisterFactory(rad::factory::CreateIndependentSamplerFactory());
  /////////////////////////////////
  // Renderer
  context->RegisterFactory(rad::factory::CreateAORenderer());
}

}  // namespace factory_help
}  // namespace rad
