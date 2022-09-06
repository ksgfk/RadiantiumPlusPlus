#include <radiantium/factory.h>

#include <radiantium/build_context.h>

#include <radiantium/tracing_accel.h>

namespace rad {
namespace factory_help {

void RegisterSystemFactories(BuildContext* context) {
  context->RegisterFactory(rad::factory::CreateEmbreeFactory());
}

}
}  // namespace rad
