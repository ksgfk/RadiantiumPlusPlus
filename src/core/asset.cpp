#include <radiantium/asset.h>

#include <radiantium/radiantium.h>

std::ostream& operator<<(std::ostream& os, rad::AssetType type) {
  switch (type) {
    case rad::AssetType::Model:
      os << "Model";
      break;
    case rad::AssetType::Image:
      os << "Image";
      break;
  }
  return os;
}
