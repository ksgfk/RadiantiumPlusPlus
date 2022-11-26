#include <rad/offline/api.h>

#include <rad/core/logger.h>

#include <openvdb/openvdb.h>

void RadInit() {
  Rad::Logger::Init();
  openvdb::initialize();
}

void RadShutdown() {
  Rad::Logger::Shutdown();
}
