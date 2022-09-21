#include <rad/offline/build/config_node.h>

#include <fstream>
#include <array>

namespace Rad {

}  // namespace Rad

std::ostream& operator<<(std::ostream& os, nlohmann::detail::value_t type) {
  switch (type) {
    case nlohmann::detail::value_t::null:
      os << "null";
      break;
    case nlohmann::detail::value_t::object:
      os << "object";
      break;
    case nlohmann::detail::value_t::array:
      os << "array";
      break;
    case nlohmann::detail::value_t::string:
      os << "string";
      break;
    case nlohmann::detail::value_t::boolean:
      os << "bool";
      break;
    case nlohmann::detail::value_t::number_integer:
      os << "int";
      break;
    case nlohmann::detail::value_t::number_unsigned:
      os << "uint";
      break;
    case nlohmann::detail::value_t::number_float:
      os << "float";
      break;
    case nlohmann::detail::value_t::binary:
      os << "binary";
      break;
    case nlohmann::detail::value_t::discarded:
      os << "discarded";
      break;
  }
  return os;
}
