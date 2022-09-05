#include <radiantium/wavefront_obj_reader.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <fstream>
#include <sstream>
#include <cctype>
#include <unordered_map>
#include <array>
#include <vector>
#include <limits>

namespace rad {

WavefrontObjReader::WavefrontObjReader(std::unique_ptr<std::istream>&& stream) {
  _stream = std::move(stream);
}

WavefrontObjReader::WavefrontObjReader(const std::filesystem::path& file) {
  _stream = std::make_unique<std::ifstream>(file);
  if (!std::filesystem::exists(file)) {
    throw std::invalid_argument("file not exist");
  }
}

WavefrontObjReader::WavefrontObjReader(const std::string& text) {
  _stream = std::make_unique<std::istringstream>(text);
}

bool WavefrontObjReader::HasError() const {
  return !_error.empty();
}

void WavefrontObjReader::Read() {
  UInt32 allLine = 0;
  std::string buffer;
  while (std::getline(*_stream, buffer)) {
    allLine++;
    if (buffer.empty()) {
      continue;
    }
    Parse(buffer, allLine);
  }
  if (_error.size() > 0 && *_error.rbegin() == '\n') {
    _error.erase(_error.begin() + _error.size() - 1);
  }
}

//////////////////////////////////////////////////
// read string helpers
static bool IsStringWhiteSpace(const std::string& str) {
  for (char c : str) {
    if (!std::isspace(c)) {
      return false;
    }
  }
  return true;
}

static std::string_view TrimStart(std::string_view str) {
  size_t start = 0;
  for (; start < str.size(); start++) {
    if (!std::isspace(str[start])) {
      break;
    }
  }
  return str.substr(start);
}

static std::string_view TrimEnd(std::string_view str) {
  size_t end = str.size() - 1;
  for (; end >= 0; end--) {
    if (!std::isspace(str[end])) {
      break;
    }
  }
  return str.substr(0, end + 1);
}

//////////////////////////////////////////////////
// .obj cmd type
enum class ObjCmd {
  Commit,
  Vertex,
  UV,
  Normal,
  Face,
  Mtllib,
  UseMtl,
  Object,
  Geometry,
  Smooth
};
static std::unordered_map<std::string_view, ObjCmd> __strToObjCmd{
    {"#", ObjCmd::Commit},
    {"v", ObjCmd::Vertex},
    {"vt", ObjCmd::UV},
    {"vn", ObjCmd::Normal},
    {"f", ObjCmd::Face},
    {"mtllib", ObjCmd::Mtllib},
    {"usemtl", ObjCmd::UseMtl},
    {"o", ObjCmd::Object},
    {"g", ObjCmd::Geometry},
    {"s", ObjCmd::Smooth}};

//////////////////////////////////////////////////
// parse functions
template <size_t Count>
static std::pair<bool, size_t> ParseNumberArray(std::string_view str, std::array<float, Count>& arr) {
  std::string_view next = TrimEnd(str);
  int count = 0;
  while (count < arr.max_size()) {
    bool isEnd = false;
    size_t partEnd = next.find(' ');
    if (partEnd == std::string_view::npos) {
      isEnd = true;
    }
    std::string_view part = isEnd ? next : next.substr(0, partEnd);
    float result;
    try {
      result = std::stof(std::string(part.data(), part.size()));
    } catch (const std::invalid_argument&) {
      return std::make_pair(false, count);
    }
    arr[count++] = result;
    if (isEnd) {
      break;
    }
    next = TrimStart(next.substr(partEnd));
  }
  return std::make_pair(true, count);
}

static bool ParseFaceVertex(std::string_view data, int mode, int& v, int& vt, int& vn) {
  switch (mode) {
    case 1: {
      try {
        v = std::stoi(std::string(data.data(), data.size()));
      } catch (const std::invalid_argument&) {
        return false;
      }
      vt = 0;
      vn = 0;
      return true;
    }
    case 2: {
      size_t delimiter = data.find('/');
      if (delimiter == std::string_view::npos) {
        return false;
      }
      std::string_view vData = data.substr(0, delimiter);
      std::string_view vtData = data.substr(delimiter + 1);
      try {
        v = std::stoi(std::string(vData.data(), vData.size()));
        vt = std::stoi(std::string(vtData.data(), vtData.size()));
      } catch (const std::invalid_argument&) {
        return false;
      }
      vn = 0;
      return true;
    }
    case 3: {
      size_t first = data.find('/');
      size_t second = data.find_last_of('/');
      if (first == std::string_view::npos || second == std::string_view::npos) {
        return false;
      }
      std::string_view vData = data.substr(0, first);
      std::string_view vtData = data.substr(first + 1, second - first - 1);
      std::string_view vnData = data.substr(second + 1);
      try {
        v = std::stoi(std::string(vData.data(), vData.size()));
        vt = std::stoi(std::string(vtData.data(), vtData.size()));
        vn = std::stoi(std::string(vnData.data(), vnData.size()));
      } catch (const std::invalid_argument&) {
        return false;
      }
      return true;
    }
    case 4: {
      size_t delimiter = data.find("//");
      if (delimiter == std::string_view::npos) {
        return false;
      }
      std::string_view vData = data.substr(0, delimiter);
      std::string_view vnData = data.substr(delimiter + 1);
      try {
        v = std::stoi(std::string(vData.data(), vData.size()));
        vt = 0;
        vn = std::stoi(std::string(vnData.data(), vnData.size()));
      } catch (const std::invalid_argument&) {
        return false;
      }
    }
    default:
      return false;
  }
}

static bool ParseFace(std::string_view data, WavefrontObjFace& face) {
  size_t first = data.find(' ');
  if (first == std::string_view::npos) {
    return false;
  }
  std::string_view one = TrimStart(data.substr(0, first));
  size_t firstDelimiter = one.find('/');
  /*
   * 1: f v1 v2 v3
   * 2: f v1/vt1 v2/vt2 v3/vt3
   * 3: f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
   * 4: f v1//vn1 v2//vn2 v3//vn3
   */
  int mode;
  if (firstDelimiter == std::string_view::npos) {
    mode = 1;
  } else {
    size_t secondDelimiter = one.find_last_of('/');
    if (secondDelimiter == firstDelimiter) {
      mode = 2;
    } else if (secondDelimiter == firstDelimiter + 1) {
      mode = 4;
    } else {
      mode = 3;
    }
  }
  std::string_view two = TrimStart(data.substr(first));
  size_t second = two.find(' ');
  if (second == std::string_view::npos) {
    return false;
  }
  std::string_view three = TrimEnd(TrimStart(two.substr(second)));
  std::string_view realTwo = two.substr(0, second);
  if (!ParseFaceVertex(one, mode, face.V1, face.Vt1, face.Vn1)) {
    return false;
  }
  if (!ParseFaceVertex(realTwo, mode, face.V2, face.Vt2, face.Vn2)) {
    return false;
  }
  if (!ParseFaceVertex(three, mode, face.V3, face.Vt3, face.Vn3)) {
    return false;
  }
  return true;
}

void WavefrontObjReader::Parse(const std::string& line, int lineNum) {
  if (IsStringWhiteSpace(line)) {
    return;
  }
  std::string_view view = TrimStart(line);
  size_t cmdEnd = view.find(' ');
  if (cmdEnd == std::string_view::npos) {
    return;
  }
  std::string_view cmd = view.substr(0, cmdEnd);
  auto iter = __strToObjCmd.find(cmd);
  if (iter == __strToObjCmd.end()) {
    _error += fmt::format("at line {}: unknown cmd {}\n", lineNum, cmd);
    return;
  }
  ObjCmd objCmd = iter->second;
  std::string_view data = TrimStart(view.substr(cmdEnd));
  switch (objCmd) {
    case ObjCmd::Commit:
      break;  // don't care. ignore it
    case ObjCmd::Vertex: {
      std::array<float, 4> result;
      auto [isSuccess, fullCount] = ParseNumberArray(data, result);
      if (!isSuccess) {
        _error += fmt::format("at line {}: can't parse vertex {}\n", lineNum, data);
      }
      _pos.push_back(Eigen::Vector3f(result[0], result[1], result[2]));
      break;
    }
    case ObjCmd::UV: {
      std::array<float, 2> result;
      auto [isSuccess, fullCount] = ParseNumberArray(data, result);
      if (!isSuccess) {
        _error += fmt::format("at line {}: can't parse uv {}\n", lineNum, data);
      }
      _uv.push_back(Eigen::Vector2f(result[0], result[1]));
      break;
    }
    case ObjCmd::Normal: {
      std::array<float, 3> result;
      auto [isSuccess, fullCount] = ParseNumberArray(data, result);
      if (!isSuccess) {
        _error += fmt::format("at line {}: can't parse normal {}\n", lineNum, data);
      }
      _normal.push_back(Eigen::Vector3f(result[0], result[1], result[2]));
      break;
    }
    case ObjCmd::Face: {
      WavefrontObjFace face;
      bool isSuccess = ParseFace(data, face);
      if (!isSuccess) {
        _error += fmt::format("at line {}: can't parse face {}\n", lineNum, data);
      }
      size_t index = _faces.size();
      _faces.push_back(face);
      if (_objects.size() > 0) {
        _objects.rbegin()->Faces.push_back(index);
      }
      break;
    }
    case ObjCmd::Mtllib: {
      std::string_view v = TrimEnd(data);
      std::string mtl(v.data(), v.size());
      _mtllibs.emplace_back(std::move(mtl));
      break;
    }
    case ObjCmd::UseMtl: {
      if (_objects.size() > 0) {
        std::string_view v = TrimEnd(data);
        std::string mtl(v.data(), v.size());
        _objects.rbegin()->Material = std::move(mtl);
      }
      break;
    }
    case ObjCmd::Geometry:  // it same as Object
    case ObjCmd::Object: {
      std::string_view nameView = TrimEnd(data);
      std::string name(nameView.data(), nameView.size());
      WavefrontObjObject obj{};
      obj.Name = std::move(name);
      _objects.emplace_back(std::move(obj));
      break;
    }
    case ObjCmd::Smooth:
      break;  // yes. we ignore it :)
  }
}

// .obj face support reverse index. so we should check if it is less than zero
static size_t CvtIdx(int f, size_t count) {
  return f >= 0 ? f - 1 : count + f;
}
std::tuple<Eigen::Vector3f, Eigen::Vector3f, Eigen::Vector3f> WavefrontObjReader::GetPosition(size_t faceIndex) const {
  const WavefrontObjFace& f = _faces[faceIndex];
  size_t count = _pos.size();
  size_t a = CvtIdx(f.V1, count), b = CvtIdx(f.V2, count), c = CvtIdx(f.V3, count);
  return std::make_tuple(_pos[a], _pos[b], _pos[c]);
}
std::tuple<Eigen::Vector3f, Eigen::Vector3f, Eigen::Vector3f> WavefrontObjReader::GetNormal(size_t faceIndex) const {
  const WavefrontObjFace& f = _faces[faceIndex];
  size_t count = _normal.size();
  size_t a = CvtIdx(f.Vn1, count), b = CvtIdx(f.Vn2, count), c = CvtIdx(f.Vn3, count);
  return std::make_tuple(_normal[a], _normal[b], _normal[c]);
}
std::tuple<Eigen::Vector2f, Eigen::Vector2f, Eigen::Vector2f> WavefrontObjReader::GetUV(size_t faceIndex) const {
  const WavefrontObjFace& f = _faces[faceIndex];
  size_t count = _uv.size();
  size_t a = CvtIdx(f.Vt1, count), b = CvtIdx(f.Vt2, count), c = CvtIdx(f.Vt3, count);
  return std::make_tuple(_uv[a], _uv[b], _uv[c]);
}

bool WavefrontObjReader::ToModel(const std::string& name, TriangleModel& result) const {
  for (const WavefrontObjObject& o : _objects) {
    if (o.Name == name) {
      std::vector<WavefrontObjFace> face(o.Faces.size());
      for (size_t i = 0; i < o.Faces.size(); i++) {
        face[i] = _faces[o.Faces[i]];
      }
      result = ToModel(face);
    }
  }
  return false;
}

TriangleModel WavefrontObjReader::ToModel() const {
  return ToModel(_faces);
}

struct VertexHash {
  bool operator==(const VertexHash& other) const noexcept {
    return P == other.P && N == other.N && T == other.T;
  }

  int P, N, T;
};

}  // namespace rad

// copy from https://github.com/dotnet/runtime/blob/v6.0.8/src/libraries/System.Private.CoreLib/src/System/HashCode.cs
template <>
struct std::hash<rad::VertexHash> {
  static size_t RotateLeft(size_t value, size_t offset) {
    return (value << offset) | (value >> (sizeof(size_t) - offset));
  }
  static size_t QueueRound(size_t hash, size_t queuedValue) {
    return RotateLeft(hash + queuedValue * 3266489917U, 17) * 668265263U;
  }
  static size_t MixFinal(size_t hash) {
    hash ^= hash >> 15;
    hash *= 2246822519U;
    hash ^= hash >> 13;
    hash *= 3266489917U;
    hash ^= hash >> 16;
    return hash;
  }
  size_t operator()(const rad::VertexHash o) const noexcept {
    size_t hash = 114514 + 374761393;
    hash += 12;
    hash = QueueRound(hash, o.P);
    hash = QueueRound(hash, o.N);
    hash = QueueRound(hash, o.T);
    hash = MixFinal(hash);
    return hash;
  }
};

namespace rad {

TriangleModel WavefrontObjReader::ToModel(const std::vector<WavefrontObjFace>& faces) const {
  std::unordered_map<VertexHash, UInt32> unique;
  std::vector<Eigen::Vector3f> p;
  std::vector<Eigen::Vector3f> n;
  std::vector<Eigen::Vector2f> u;
  std::vector<UInt32> ind;
  UInt32 count = 0;
  VertexHash v[3];
  for (const WavefrontObjFace& f : faces) {
    v[0] = VertexHash{f.V1, f.Vn1, f.Vt1};
    v[1] = VertexHash{f.V2, f.Vn2, f.Vt2};
    v[2] = VertexHash{f.V3, f.Vn3, f.Vt3};
    for (size_t j = 0; j < 3; j++) {
      auto iter = unique.find(v[j]);
      UInt32 index;
      if (iter == unique.end()) {
        unique.emplace(v[j], count);
        index = count;
        p.push_back(_pos[CvtIdx(v[j].P, _pos.size())]);
        if (v[j].N != 0) {
          n.push_back(_normal[CvtIdx(v[j].N, _normal.size())]);
        }
        if (v[j].T != 0) {
          u.push_back(_uv[CvtIdx(v[j].T, _uv.size())]);
        }
        count++;
      } else {
        index = iter->second;
      }
      ind.push_back(index);
    }
  }
  p.shrink_to_fit();
  ind.shrink_to_fit();
  n.shrink_to_fit();
  u.shrink_to_fit();
  if (p.size() >= std::numeric_limits<UInt32>::max()) {
    throw std::out_of_range("model tooooooooo bug");
  }
  if (ind.size() >= std::numeric_limits<UInt32>::max()) {
    throw std::out_of_range("model tooooooooo bug");
  }
  return TriangleModel(
      p.data(),
      static_cast<UInt32>(p.size()),
      ind.data(),
      static_cast<UInt32>(ind.size()),
      n.size() == 0 ? nullptr : n.data(),
      u.size() == 0 ? nullptr : u.data());
}

}  // namespace rad
