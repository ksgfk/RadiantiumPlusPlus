#pragma once

#include "../common.h"
#include "triangle_model.h"

#include <memory>
#include <istream>
#include <string>
#include <filesystem>
#include <tuple>
#include <vector>

namespace Rad {

struct WavefrontObjFace {
  Int32 V1, V2, V3;
  Int32 Vt1, Vt2, Vt3;
  Int32 Vn1, Vn2, Vn3;
};

struct WavefrontObjObject {
  std::string Name;
  std::string Material;
  std::vector<size_t> Faces;
};
/**
 * @brief .obj 格式模型解析器
 */
class WavefrontObjReader {
 public:
  WavefrontObjReader(Unique<std::istream> stream);
  WavefrontObjReader(const std::filesystem::path& file);
  WavefrontObjReader(const std::string& text);

  bool HasError() const;
  const std::string& Error() const { return _error; }
  const std::vector<Eigen::Vector3f>& Positions() const { return _pos; }
  const std::vector<Eigen::Vector2f>& UVs() const { return _uv; }
  const std::vector<Eigen::Vector3f>& Normals() const { return _normal; }
  const std::vector<WavefrontObjFace>& Faces() const { return _faces; }
  const std::vector<std::string>& Mtllibs() const { return _mtllibs; }
  const std::vector<WavefrontObjObject>& Objects() const { return _objects; }

  void Read();
  std::tuple<Eigen::Vector3f, Eigen::Vector3f, Eigen::Vector3f> GetPosition(size_t faceIndex) const;
  std::tuple<Eigen::Vector3f, Eigen::Vector3f, Eigen::Vector3f> GetNormal(size_t faceIndex) const;
  std::tuple<Eigen::Vector2f, Eigen::Vector2f, Eigen::Vector2f> GetUV(size_t faceIndex) const;
  TriangleModel ToModel(const std::string& name) const;
  TriangleModel ToModel() const;

 private:
  void Parse(const std::string& line, int lineNum);
  TriangleModel ToModel(const std::vector<WavefrontObjFace>& faces) const;

  Unique<std::istream> _stream;
  std::string _error;

  std::vector<Eigen::Vector3f> _pos;
  std::vector<Eigen::Vector2f> _uv;
  std::vector<Eigen::Vector3f> _normal;
  std::vector<WavefrontObjFace> _faces;
  std::vector<std::string> _mtllibs;
  std::vector<WavefrontObjObject> _objects;
};

}  // namespace Rad
