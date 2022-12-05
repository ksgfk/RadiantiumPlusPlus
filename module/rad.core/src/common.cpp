#include <rad/core/common.h>

#include <rad/core/logger.h>
#include <openvdb/openvdb.h>

namespace Rad {

void RadCoreInit() {
  Logger::Init();
  openvdb::initialize();
}

void RadCoreShutdown() {
  Logger::Shutdown();
}

}  // namespace Rad
