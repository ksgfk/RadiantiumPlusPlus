#pragma once

#include <rad/core/types.h>

namespace Rad {

class ImmutableModel {
 public:
  ImmutableModel() noexcept;
  ImmutableModel(
      std::vector<Vector3f>&& pos,
      std::vector<Vector3f>&& nor,
      std::vector<Vector2f>&& tex,
      std::vector<size_t>&& ind);
  ImmutableModel(
      std::vector<Vector3f>&& pos,
      std::vector<Vector3f>&& nor,
      std::vector<Vector2f>&& tex,
      std::vector<Vector3f>&& tan,
      std::vector<size_t>&& ind);
  ~ImmutableModel() noexcept = default;

  const std::vector<Vector3f>& GetPositions() const { return _positions; }
  const std::vector<Vector3f>& GetNormals() const { return _normals; }
  const std::vector<Vector2f>& GetTexCoords() const { return _texcoords; }
  const std::vector<Vector3f>& GetTangents() const { return _tangent; }
  const std::vector<size_t>& GetIndices() const { return _indices; }

  inline bool HasNormal() const { return _normals.size() > 0; }
  inline bool HasTexCoord() const { return _normals.size() > 0; }
  inline bool HasTangent() const { return _normals.size() > 0; }

  inline size_t GetVertexCount() const { return _positions.size(); }
  inline size_t GetIndexCount() const { return _indices.size(); }
  inline size_t GetTriangleCount() const { return GetIndexCount() / 3; }

  static ImmutableModel CreateSphere(float radius, int numberSlices);
  static ImmutableModel CreateCube(float halfExtend);
  static ImmutableModel CreateQuad(float halfExtend);
  static ImmutableModel CreateCylinder(float halfExtend, float radius, int numberSlices);
  static ImmutableModel CreateCylinder(
      float bottomRadius,
      float topRadius,
      float height,
      uint32_t sliceCount,
      uint32_t stackCount);
  static ImmutableModel CreateGrid(float width, float depth, uint32_t m, uint32_t n);

 private:
  std::vector<Vector3f> _positions;
  std::vector<Vector3f> _normals;
  std::vector<Vector2f> _texcoords;
  std::vector<Vector3f> _tangent;
  std::vector<size_t> _indices;
};

}  // namespace Rad
