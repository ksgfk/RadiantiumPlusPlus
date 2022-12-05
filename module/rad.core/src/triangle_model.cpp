#include <rad/core/triangle_model.h>

#include <rad/core/math_base.h>

using namespace Rad::Math;

namespace Rad {

TriangleModel::TriangleModel(
    const Eigen::Vector3f* pos,
    UInt32 vertexCount,
    const UInt32* indices,
    UInt32 indexCount,
    const Eigen::Vector3f* normal,
    const Eigen::Vector2f* uv,
    const Eigen::Vector3f* tangent) {
  if (indexCount % 3 != 0) {
    throw RadException("Invalid index number {}, must be an integer multiple of 3", indexCount);
  }
  _vertexCount = vertexCount;
  _indexCount = indexCount;
  _triangleCount = indexCount / 3;
  _position = Share<Eigen::Vector3f[]>(new Eigen::Vector3f[vertexCount]);
  std::copy(pos, pos + vertexCount, _position.get());
  _indices = Share<UInt32[]>(new UInt32[indexCount]);
  std::copy(indices, indices + indexCount, _indices.get());
  if (normal != nullptr) {
    _normal = Share<Eigen::Vector3f[]>(new Eigen::Vector3f[vertexCount]);
    std::copy(normal, normal + vertexCount, _normal.get());
  }
  if (uv != nullptr) {
    _uv = Share<Eigen::Vector2f[]>(new Eigen::Vector2f[vertexCount]);
    std::copy(uv, uv + vertexCount, _uv.get());
  }
  if (tangent != nullptr) {
    _tangent = Share<Eigen::Vector3f[]>(new Eigen::Vector3f[vertexCount]);
    std::copy(tangent, tangent + vertexCount, _tangent.get());
  }
}

TriangleModel TriangleModel::CreateSphere(Float32 radius, Int32 numberSlices) {
  const Vector3f axisX = {1.0f, 0.0f, 0.0f};

  UInt32 numberParallels = numberSlices / 2;
  UInt32 numberVertices = (numberParallels + 1) * (numberSlices + 1);
  UInt32 numberIndices = numberParallels * numberSlices * 6;

  Float32 angleStep = (2.0f * PI_F) / ((Float32)numberSlices);

  std::vector<Vector3f> positions(numberVertices, Vector3f{});
  std::vector<Vector3f> normals(numberVertices, Vector3f{});
  std::vector<Vector2f> uvs(numberVertices, Vector2f{});
  std::vector<Vector3f> tangents(numberVertices, Vector3f{});

  for (UInt32 i = 0; i < numberParallels + 1; i++) {
    for (UInt32 j = 0; j < (UInt32)(numberSlices + 1); j++) {
      UInt32 vertexIndex = (i * (numberSlices + 1) + j);
      UInt32 normalIndex = (i * (numberSlices + 1) + j);
      UInt32 texCoordsIndex = (i * (numberSlices + 1) + j);
      UInt32 tangentIndex = (i * (numberSlices + 1) + j);

      Float32 px = radius * std::sin(angleStep * (Float32)i) * std::sin(angleStep * (Float32)j);
      Float32 py = radius * std::cos(angleStep * (Float32)i);
      Float32 pz = radius * std::sin(angleStep * (Float32)i) * std::cos(angleStep * (Float32)j);
      positions[vertexIndex] = {px, py, pz};

      Float32 nx = positions[vertexIndex].x() / radius;
      Float32 ny = positions[vertexIndex].y() / radius;
      Float32 nz = positions[vertexIndex].z() / radius;
      normals[normalIndex] = {nx, ny, nz};

      Float32 tx = (Float32)j / (Float32)numberSlices;
      Float32 ty = 1.0f - (Float32)i / (Float32)numberParallels;
      uvs[texCoordsIndex] = {tx, ty};

      Eigen::AngleAxisf mat(Radian(360.0f * uvs[texCoordsIndex].x()), Vector3f{0, 1.0f, 0});
      tangents[tangentIndex] = mat.matrix() * Vector3f(axisX.x(), axisX.y(), axisX.z());
    }
  }

  UInt32 indexIndices = 0;
  std::vector<UInt32> indices(numberIndices, UInt32{0});
  for (UInt32 i = 0; i < numberParallels; i++) {
    for (UInt32 j = 0; j < (UInt32)(numberSlices); j++) {
      indices[indexIndices++] = i * (numberSlices + 1) + j;
      indices[indexIndices++] = (i + 1) * (numberSlices + 1) + j;
      indices[indexIndices++] = (i + 1) * (numberSlices + 1) + (j + 1);

      indices[indexIndices++] = i * (numberSlices + 1) + j;
      indices[indexIndices++] = (i + 1) * (numberSlices + 1) + (j + 1);
      indices[indexIndices++] = i * (numberSlices + 1) + (j + 1);
    }
  }

  return TriangleModel(
      positions.data(), (UInt32)positions.size(),
      indices.data(), (UInt32)indices.size(),
      normals.data(),
      uvs.data(),
      tangents.data());
}

TriangleModel TriangleModel::CreateCube(Float32 halfExtend) {
  constexpr const Float32 cubeVertices[] =
      {-1.0f, -1.0f, -1.0f, +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, -1.0f, +1.0f,
       -1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f,
       -1.0f, -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, -1.0f, +1.0f,
       -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f,
       -1.0f, -1.0f, -1.0f, +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f,
       +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f};
  constexpr const Float32 cubeNormals[] =
      {0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
       0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f,
       0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,
       0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f,
       -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
       +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f};
  constexpr const Float32 cubeTexCoords[] =
      {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
       0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
       1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
       0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
       0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
       1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
  constexpr const Float32 cubeTangents[] =
      {+1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f,
       +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f,
       -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
       +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f,
       0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f,
       0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f};
  constexpr const UInt32 cubeIndices[] =
      {0, 2, 1, 0, 3, 2,
       4, 5, 6, 4, 6, 7,
       8, 9, 10, 8, 10, 11,
       12, 15, 14, 12, 14, 13,
       16, 17, 18, 16, 18, 19,
       20, 23, 22, 20, 22, 21};

  constexpr const UInt32 numberVertices = 24;
  constexpr const UInt32 numberIndices = 36;

  std::vector<Vector3f> positions(numberVertices, Vector3f{});
  std::vector<Vector3f> normals(numberVertices, Vector3f{});
  std::vector<Vector2f> uvs(numberVertices, Vector2f{});
  std::vector<Vector3f> tangents(numberVertices, Vector3f{});
  for (UInt32 i = 0; i < numberVertices; i++) {
    positions[i] = {cubeVertices[i * 4 + 0] * halfExtend,
                    cubeVertices[i * 4 + 1] * halfExtend,
                    cubeVertices[i * 4 + 2] * halfExtend};
    normals[i] = {cubeNormals[i * 3 + 0],
                  cubeNormals[i * 3 + 1],
                  cubeNormals[i * 3 + 2]};
    uvs[i] = {cubeTexCoords[i * 2 + 0],
              cubeTexCoords[i * 2 + 1]};
    tangents[i] = {cubeTangents[i * 2 + 0],
                   cubeTangents[i * 2 + 1],
                   cubeTangents[i * 2 + 2]};
  }
  std::vector<UInt32> indices(cubeIndices, cubeIndices + numberIndices);
  return TriangleModel(
      positions.data(), (UInt32)positions.size(),
      indices.data(), (UInt32)indices.size(),
      normals.data(),
      uvs.data(),
      tangents.data());
}

TriangleModel TriangleModel::CreateQuad(Float32 halfExtend) {
  constexpr const Float32 quadVertices[] =
      {-1.0f, -1.0f, 0.0f, +1.0f,
       +1.0f, -1.0f, 0.0f, +1.0f,
       -1.0f, +1.0f, 0.0f, +1.0f,
       +1.0f, +1.0f, 0.0f, +1.0f};
  constexpr const Float32 quadNormal[] =
      {0.0f, 0.0f, 1.0f,
       0.0f, 0.0f, 1.0f,
       0.0f, 0.0f, 1.0f,
       0.0f, 0.0f, 1.0f};
  constexpr const Float32 quadTex[] =
      {0.0f, 0.0f,
       1.0f, 0.0f,
       0.0f, 1.0f,
       1.0f, 1.0f};
  constexpr const Float32 quadTan[] =
      {1.0f, 0.0f, 0.0f,
       1.0f, 0.0f, 0.0f,
       1.0f, 0.0f, 0.0f,
       1.0f, 0.0f, 0.0f};
  constexpr const UInt32 quadIndices[] =
      {0, 1, 2,
       1, 3, 2};
  constexpr const UInt32 numberVertices = 4;
  constexpr const UInt32 numberIndices = 6;
  std::vector<Vector3f> positions(numberVertices, Vector3f{});
  std::vector<Vector3f> normals(numberVertices, Vector3f{});
  std::vector<Vector2f> uvs(numberVertices, Vector2f{});
  std::vector<Vector3f> tangents(numberVertices, Vector3f{});
  for (UInt32 i = 0; i < numberVertices; i++) {
    positions[i] = {quadVertices[i * 4 + 0] * halfExtend,
                    quadVertices[i * 4 + 1] * halfExtend,
                    quadVertices[i * 4 + 2]};
    normals[i] = {quadNormal[i * 3 + 0],
                  quadNormal[i * 3 + 1],
                  quadNormal[i * 3 + 2]};
    uvs[i] = {quadTex[i * 2 + 0],
              quadTex[i * 2 + 1]};
    tangents[i] = {quadTan[i * 3 + 0],
                   quadTan[i * 3 + 1],
                   quadTan[i * 3 + 2]};
  }
  std::vector<UInt32> indices(quadIndices, quadIndices + numberIndices);
  return TriangleModel(
      positions.data(), (UInt32)positions.size(),
      indices.data(), (UInt32)indices.size(),
      normals.data(),
      uvs.data(),
      tangents.data());
}

TriangleModel TriangleModel::CreateCylinder(Float32 halfExtend, Float32 radius, Int32 numberSlices) {
  Int32 numberVertices = (numberSlices + 2) * 2 + (numberSlices + 1) * 2;
  Int32 numberIndices = numberSlices * 3 * 2 + numberSlices * 6;
  Float32 angleStep = (2.0f * PI_F) / ((Float32)numberSlices);
  std::vector<Vector3f> positions(numberVertices, Vector3f{});
  std::vector<Vector3f> normals(numberVertices, Vector3f{});
  std::vector<Vector2f> uvs(numberVertices, Vector2f{});
  std::vector<Vector3f> tangents(numberVertices, Vector3f{});
  std::vector<UInt32> indices(numberIndices, 0);

  UInt32 vertexCounter = 0;
  // Center bottom
  positions[vertexCounter][0] = 0.0f;
  positions[vertexCounter][1] = -halfExtend;
  positions[vertexCounter][2] = 0.0f;
  normals[vertexCounter][0] = 0.0f;
  normals[vertexCounter][1] = -1.0f;
  normals[vertexCounter][2] = 0.0f;
  tangents[vertexCounter][0] = 0.0f;
  tangents[vertexCounter][1] = 0.0f;
  tangents[vertexCounter][2] = 1.0f;
  uvs[vertexCounter][0] = 0.0f;
  uvs[vertexCounter][1] = 0.0f;
  vertexCounter++;
  // Bottom
  for (Int32 i = 0; i < numberSlices + 1; i++) {
    Float32 currentAngle = angleStep * (Float32)i;
    positions[vertexCounter][0] = std::cos(currentAngle) * radius;
    positions[vertexCounter][1] = -halfExtend;
    positions[vertexCounter][2] = -std::sin(currentAngle) * radius;
    normals[vertexCounter][0] = 0.0f;
    normals[vertexCounter][1] = -1.0f;
    normals[vertexCounter][2] = 0.0f;
    tangents[vertexCounter][0] = std::sin(currentAngle);
    tangents[vertexCounter][1] = 0.0f;
    tangents[vertexCounter][2] = std::cos(currentAngle);
    uvs[vertexCounter][0] = 0.0f;
    uvs[vertexCounter][1] = 0.0f;
    vertexCounter++;
  }
  // Center top
  positions[vertexCounter][0] = 0.0f;
  positions[vertexCounter][1] = halfExtend;
  positions[vertexCounter][2] = 0.0f;
  normals[vertexCounter][0] = 0.0f;
  normals[vertexCounter][1] = 1.0f;
  normals[vertexCounter][2] = 0.0f;
  tangents[vertexCounter][0] = 0.0f;
  tangents[vertexCounter][1] = 0.0f;
  tangents[vertexCounter][2] = -1.0f;
  uvs[vertexCounter][0] = 1.0f;
  uvs[vertexCounter][1] = 1.0f;
  vertexCounter++;
  // Top
  for (Int32 i = 0; i < numberSlices + 1; i++) {
    Float32 currentAngle = angleStep * (Float32)i;
    positions[vertexCounter][0] = std::cos(currentAngle) * radius;
    positions[vertexCounter][1] = halfExtend;
    positions[vertexCounter][2] = -std::sin(currentAngle) * radius;
    normals[vertexCounter][0] = 0.0f;
    normals[vertexCounter][1] = 1.0f;
    normals[vertexCounter][2] = 0.0f;
    tangents[vertexCounter][0] = -std::sin(currentAngle);
    tangents[vertexCounter][1] = 0.0f;
    tangents[vertexCounter][2] = -std::cos(currentAngle);
    uvs[vertexCounter][0] = 1.0f;
    uvs[vertexCounter][1] = 1.0f;
    vertexCounter++;
  }
  for (Int32 i = 0; i < numberSlices + 1; i++) {
    Float32 currentAngle = angleStep * (Float32)i;
    Float32 sign = -1.0f;
    for (Int32 j = 0; j < 2; j++) {
      positions[vertexCounter][0] = std::cos(currentAngle) * radius;
      positions[vertexCounter][1] = halfExtend * sign;
      positions[vertexCounter][2] = -std::sin(currentAngle) * radius;
      normals[vertexCounter][0] = std::cos(currentAngle);
      normals[vertexCounter][1] = 0.0f;
      normals[vertexCounter][2] = -std::sin(currentAngle);
      tangents[vertexCounter][0] = -std::sin(currentAngle);
      tangents[vertexCounter][1] = 0.0f;
      tangents[vertexCounter][2] = -std::cos(currentAngle);
      uvs[vertexCounter][0] = (Float32)i / (Float32)numberSlices;
      uvs[vertexCounter][1] = (sign + 1.0f) / 2.0f;
      vertexCounter++;
      sign = 1.0f;
    }
  }
  // index
  // Bottom
  UInt32 centerIndex = 0;
  UInt32 indexCounter = 1;
  UInt32 indexIndices = 0;
  for (Int32 i = 0; i < numberSlices; i++) {
    indices[indexIndices++] = centerIndex;
    indices[indexIndices++] = indexCounter + 1;
    indices[indexIndices++] = indexCounter;
    indexCounter++;
  }
  indexCounter++;
  // Top
  centerIndex = indexCounter;
  indexCounter++;
  for (Int32 i = 0; i < numberSlices; i++) {
    indices[indexIndices++] = centerIndex;
    indices[indexIndices++] = indexCounter;
    indices[indexIndices++] = indexCounter + 1;
    indexCounter++;
  }
  indexCounter++;
  // Sides
  for (Int32 i = 0; i < numberSlices; i++) {
    indices[indexIndices++] = indexCounter;
    indices[indexIndices++] = indexCounter + 2;
    indices[indexIndices++] = indexCounter + 1;
    indices[indexIndices++] = indexCounter + 2;
    indices[indexIndices++] = indexCounter + 3;
    indices[indexIndices++] = indexCounter + 1;
    indexCounter += 2;
  }
  return TriangleModel(
      positions.data(), (UInt32)positions.size(),
      indices.data(), (UInt32)indices.size(),
      normals.data(),
      uvs.data(),
      tangents.data());
}

TriangleModel TriangleModel::CreateCylinder(
    Float32 bottomRadius,
    Float32 topRadius,
    Float32 height,
    UInt32 sliceCount,
    UInt32 stackCount) {
  Float32 stackHeight = height / stackCount;
  Float32 radiusStep = (topRadius - bottomRadius) / stackCount;
  UInt32 ringCount = stackCount + 1;

  std::vector<Vector3f> positions;
  std::vector<Vector3f> normals;
  std::vector<Vector2f> uvs;
  std::vector<Vector3f> tangents;
  std::vector<UInt32> indices;

  for (UInt32 i = 0; i < ringCount; ++i) {
    Float32 y = -0.5f * height + i * stackHeight;
    Float32 r = bottomRadius + i * radiusStep;
    Float32 dTheta = 2.0f * PI_F / sliceCount;
    for (UInt32 j = 0; j <= sliceCount; ++j) {
      Float32 c = std::cos(j * dTheta);
      Float32 s = std::sin(j * dTheta);
      Vector3f position(r * c, y, r * s);

      Vector2f texCoord{};
      texCoord.x() = (Float32)j / sliceCount;
      texCoord.y() = 1.0f - (Float32)i / stackCount;

      Vector3f tangent(-s, 0.0f, c);

      Float32 dr = bottomRadius - topRadius;
      Vector3f bitangent(dr * c, -height, dr * s);

      Vector3f T = tangent;
      Vector3f B = bitangent;
      Vector3f N = T.cross(B).normalized();
      Vector3f normal = N;

      positions.emplace_back(position);
      uvs.emplace_back(texCoord);
      tangents.emplace_back(tangent);
      normals.emplace_back(normal);
    }
  }
  UInt32 ringVertexCount = UInt32(sliceCount) + 1;
  for (UInt32 i = 0; i < stackCount; ++i) {
    for (UInt32 j = 0; j < sliceCount; ++j) {
      indices.emplace_back(i * ringVertexCount + j);
      indices.emplace_back((i + 1) * ringVertexCount + j);
      indices.emplace_back((i + 1) * ringVertexCount + j + 1);

      indices.emplace_back(i * ringVertexCount + j);
      indices.emplace_back((i + 1) * ringVertexCount + j + 1);
      indices.emplace_back(i * ringVertexCount + j + 1);
    }
  }
  {  // TOP
    UInt32 baseIndex = (UInt32)positions.size();
    Float32 y = 0.5f * height;
    Float32 dTheta = 2.0f * PI_F / sliceCount;
    for (UInt32 i = 0; i <= sliceCount; ++i) {
      Float32 x = topRadius * std::cos(i * dTheta);
      Float32 z = topRadius * std::sin(i * dTheta);
      Float32 u = x / height + 0.5f;
      Float32 v = z / height + 0.5f;

      positions.emplace_back(Vector3f{x, y, z});
      uvs.emplace_back(Vector2f{u, v});
      tangents.emplace_back(Vector3f{1.0f, 0.0f, 0.0f});
      normals.emplace_back(Vector3f{0.0f, 1.0f, 0.0f});
    }
    positions.emplace_back(Vector3f{0.0f, y, 0.0f});
    uvs.emplace_back(Vector2f{0.5f, 0.5f});
    tangents.emplace_back(Vector3f{1.0f, 0.0f, 0.0f});
    normals.emplace_back(Vector3f{0.0f, 1.0f, 0.0f});
    UInt32 centerIndex = (UInt32)positions.size() - 1;
    for (UInt32 i = 0; i < sliceCount; ++i) {
      indices.emplace_back(centerIndex);
      indices.emplace_back(baseIndex + i + 1);
      indices.emplace_back(baseIndex + i);
    }
  }
  {  // bottom
    UInt32 baseIndex = (UInt32)positions.size();
    Float32 y = -0.5f * height;
    Float32 dTheta = 2.0f * PI_F / sliceCount;
    for (UInt32 i = 0; i <= sliceCount; ++i) {
      Float32 x = bottomRadius * cosf(i * dTheta);
      Float32 z = bottomRadius * sinf(i * dTheta);
      Float32 u = x / height + 0.5f;
      Float32 v = z / height + 0.5f;

      positions.emplace_back(Vector3f{x, y, z});
      uvs.emplace_back(Vector2f{u, v});
      tangents.emplace_back(Vector3f{1.0f, 0.0f, 0.0f});
      normals.emplace_back(Vector3f{0.0f, -1.0f, 0.0f});
    }
    positions.emplace_back(Vector3f{0.0f, y, 0.0f});
    uvs.emplace_back(Vector2f{0.5f, 0.5f});
    tangents.emplace_back(Vector3f{1.0f, 0.0f, 0.0f});
    normals.emplace_back(Vector3f{0.0f, -1.0f, 0.0f});
    UInt32 centerIndex = (UInt32)positions.size() - 1;
    for (UInt32 i = 0; i < sliceCount; ++i) {
      indices.emplace_back(centerIndex);
      indices.emplace_back(baseIndex + i);
      indices.emplace_back(baseIndex + i + 1);
    }
  }
  return TriangleModel(
      positions.data(), (UInt32)positions.size(),
      indices.data(), (UInt32)indices.size(),
      normals.data(),
      uvs.data(),
      tangents.data());
}

TriangleModel TriangleModel::CreateGrid(Float32 width, Float32 depth, UInt32 m, UInt32 n) {
  UInt32 vertexCount = m * n;
  UInt32 faceCount = (m - 1) * (n - 1) * 2;
  std::vector<Vector3f> positions(vertexCount);
  std::vector<Vector3f> normals(vertexCount);
  std::vector<Vector2f> uvs(vertexCount);
  std::vector<Vector3f> tangents(vertexCount);
  std::vector<UInt32> indices(faceCount * 3);
  Float32 halfWidth = 0.5f * width;
  Float32 halfDepth = 0.5f * depth;
  Float32 dx = width / (n - 1);
  Float32 dz = depth / (m - 1);
  Float32 du = 1.0f / (n - 1);
  Float32 dv = 1.0f / (m - 1);
  for (UInt32 i = 0; i < m; ++i) {
    Float32 z = halfDepth - i * dz;
    for (UInt32 j = 0; j < n; ++j) {
      Float32 x = -halfWidth + j * dx;
      positions[i * n + j] = Vector3f(x, 0.0f, z);
      normals[i * n + j] = Vector3f(0.0f, 1.0f, 0.0f);
      tangents[i * n + j] = Vector3f(1.0f, 0.0f, 0.0f);
      uvs[i * n + j].x() = j * du;
      uvs[i * n + j].y() = i * dv;
    }
  }
  UInt32 k = 0;
  for (UInt32 i = 0; i < m - 1; ++i) {
    for (UInt32 j = 0; j < n - 1; ++j) {
      indices[k] = i * n + j;
      indices[k + 1] = i * n + j + 1;
      indices[k + 2] = (i + 1) * n + j;
      indices[k + 3] = (i + 1) * n + j;
      indices[k + 4] = i * n + j + 1;
      indices[k + 5] = (i + 1) * n + j + 1;
      k += 6;
    }
  }
  return TriangleModel(
      positions.data(), (UInt32)positions.size(),
      indices.data(), (UInt32)indices.size(),
      normals.data(),
      uvs.data(),
      tangents.data());
}

}  // namespace Rad
