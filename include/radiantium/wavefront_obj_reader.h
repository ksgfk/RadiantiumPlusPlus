#pragma once

#include "radiantium.h"
#include "radiantium/model.h"

#include <memory>
#include <istream>
#include <string>
#include <filesystem>
#include <tuple>
#include <vector>

namespace rad {

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

class WavefrontObjReader {
 public:
  WavefrontObjReader(std::unique_ptr<std::istream>&& stream);
  WavefrontObjReader(const std::filesystem::path& file);
  WavefrontObjReader(const std::string& text);

  /* Properties */
  bool HasError() const;
  const std::string& Error() const { return _error; }
  const std::vector<Vec3>& Positions() const { return _pos; }
  const std::vector<Vec2>& UVs() const { return _uv; }
  const std::vector<Vec3>& Normals() const { return _normal; }
  const std::vector<WavefrontObjFace>& Faces() const { return _faces; }
  const std::vector<std::string>& Mtllibs() const { return _mtllibs; }
  const std::vector<WavefrontObjObject>& Objects() const { return _objects; }

  /* Methods */
  void Read();
  std::tuple<Vec3, Vec3, Vec3> GetPosition(size_t faceIndex) const;
  std::tuple<Vec3, Vec3, Vec3> GetNormal(size_t faceIndex) const;
  std::tuple<Vec2, Vec2, Vec2> GetUV(size_t faceIndex) const;
  bool ToModel(const std::string& name, TriangleModel& result) const;
  TriangleModel ToModel() const;

 private:
  void Parse(const std::string& line, int lineNum);
  TriangleModel ToModel(const std::vector<WavefrontObjFace>& faces) const;

  std::unique_ptr<std::istream> _stream;
  std::string _error;

  std::vector<Vec3> _pos;
  std::vector<Vec2> _uv;
  std::vector<Vec3> _normal;
  std::vector<WavefrontObjFace> _faces;
  std::vector<std::string> _mtllibs;
  std::vector<WavefrontObjObject> _objects;
};

}  // namespace rad
