#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "radiantium.h"

namespace rad {

class IConfigNode {
 public:
  enum class Type {
    Empty,
    Bool,
    Int,
    Float,
    Array,
    Mat4,
    String,
    Object,
    Unknown
  };

  virtual ~IConfigNode() noexcept = default;

  virtual Type GetType() const = 0;
  virtual bool HasProperty(const std::string& name) const = 0;

  virtual bool GetBool(const std::string& name) const = 0;
  virtual bool GetBool(const std::string& name, bool defVal) const = 0;
  virtual Int32 GetInt32(const std::string& name) const = 0;
  virtual Int32 GetInt32(const std::string& name, Int32 defVal) const = 0;
  virtual Float GetFloat(const std::string& name) const = 0;
  virtual Float GetFloat(const std::string& name, Float defVal) const = 0;
  virtual Vec2 GetVec2(const std::string& name) const = 0;
  virtual Vec2 GetVec2(const std::string& name, const Vec2& defVal) const = 0;
  virtual Vec3 GetVec3(const std::string& name) const = 0;
  virtual Vec3 GetVec3(const std::string& name, const Vec3& defVal) const = 0;
  virtual Vec4 GetVec4(const std::string& name) const = 0;
  virtual Vec4 GetVec4(const std::string& name, const Vec4& defVal) const = 0;
  virtual Mat4 GetMat4(const std::string& name) const = 0;
  virtual Mat4 GetMat4(const std::string& name, const Mat4& defVal) const = 0;
  virtual std::string GetString(const std::string& name) const = 0;
  virtual std::string GetString(const std::string& name, const std::string& defVal) const = 0;
  virtual std::unique_ptr<IConfigNode> GetObject(const std::string& name) const = 0;
  virtual bool TryGetObject(const std::string& name, std::unique_ptr<IConfigNode>& node) const = 0;
  virtual std::vector<std::unique_ptr<IConfigNode>> GetArray(const std::string& name) const = 0;
  virtual bool TryGetArray(const std::string& name, std::vector<std::unique_ptr<IConfigNode>>& array) const = 0;
};

namespace config {

std::unique_ptr<IConfigNode> CreateJsonConfig(const std::filesystem::path& path);
std::unique_ptr<IConfigNode> CreateJsonConfig(const std::string& config);

}  // namespace config

}  // namespace rad
