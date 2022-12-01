#include <rad/realtime/model.h>

#include <rad/core/constant.h>

namespace Rad {

ImmutableModel::ImmutableModel() noexcept = default;

ImmutableModel::ImmutableModel(
    std::vector<Vector3f>&& pos,
    std::vector<Vector3f>&& nor,
    std::vector<Vector2f>&& tex,
    std::vector<size_t>&& ind) {
  _positions = std::move(pos);
  _normals = std::move(nor);
  _texcoords = std::move(tex);
  _indices = std::move(ind);
  if (_indices.size() % 3 != 0) {
    throw std::invalid_argument("indices must an integer multiple of 3");
  }
}

ImmutableModel::ImmutableModel(
    std::vector<Vector3f>&& pos,
    std::vector<Vector3f>&& nor,
    std::vector<Vector2f>&& tex,
    std::vector<Vector3f>&& tan,
    std::vector<size_t>&& ind) {
  _positions = std::move(pos);
  _normals = std::move(nor);
  _texcoords = std::move(tex);
  _tangent = std::move(tan);
  _indices = std::move(ind);
  if (_indices.size() % 3 != 0) {
    throw std::invalid_argument("indices must an integer multiple of 3");
  }
}

ImmutableModel ImmutableModel::CreateSphere(float radius, int numberSlices) {
  const Vector3f axisX = {1.0f, 0.0f, 0.0f};

  uint32_t numberParallels = numberSlices / 2;
  uint32_t numberVertices = (numberParallels + 1) * (numberSlices + 1);
  uint32_t numberIndices = numberParallels * numberSlices * 6;

  float angleStep = (2.0f * PI_F) / ((float)numberSlices);

  std::vector<Vector3f> vertices(numberVertices, Vector3f{});
  std::vector<Vector3f> normals(numberVertices, Vector3f{});
  std::vector<Vector2f> texCoords(numberVertices, Vector2f{});
  std::vector<Vector3f> tangents(numberVertices, Vector3f{});

  for (uint32_t i = 0; i < numberParallels + 1; i++) {
    for (uint32_t j = 0; j < (uint32_t)(numberSlices + 1); j++) {
      uint32_t vertexIndex = (i * (numberSlices + 1) + j);
      uint32_t normalIndex = (i * (numberSlices + 1) + j);
      uint32_t texCoordsIndex = (i * (numberSlices + 1) + j);
      uint32_t tangentIndex = (i * (numberSlices + 1) + j);

      float px = radius * std::sin(angleStep * (float)i) * std::sin(angleStep * (float)j);
      float py = radius * std::cos(angleStep * (float)i);
      float pz = radius * std::sin(angleStep * (float)i) * std::cos(angleStep * (float)j);
      vertices[vertexIndex] = {px, py, pz};

      float nx = vertices[vertexIndex].x() / radius;
      float ny = vertices[vertexIndex].y() / radius;
      float nz = vertices[vertexIndex].z() / radius;
      normals[normalIndex] = {nx, ny, nz};

      float tx = (float)j / (float)numberSlices;
      float ty = 1.0f - (float)i / (float)numberParallels;
      texCoords[texCoordsIndex] = {tx, ty};

      Eigen::AngleAxisf mat(360.0f * texCoords[texCoordsIndex].x(), Vector3f{0, 1.0f, 0});
      tangents[tangentIndex] = mat.matrix() * Vector3f(axisX.x(), axisX.y(), axisX.z());
    }
  }

  uint32_t indexIndices = 0;
  std::vector<size_t> indices(numberIndices, size_t{});
  for (uint32_t i = 0; i < numberParallels; i++) {
    for (uint32_t j = 0; j < (uint32_t)(numberSlices); j++) {
      indices[indexIndices++] = i * ((size_t)numberSlices + 1) + j;
      indices[indexIndices++] = ((size_t)i + 1) * ((size_t)numberSlices + 1) + j;
      indices[indexIndices++] = ((size_t)i + 1) * ((size_t)numberSlices + 1) + ((size_t)j + 1);

      indices[indexIndices++] = i * ((size_t)numberSlices + 1) + j;
      indices[indexIndices++] = ((size_t)i + 1) * ((size_t)numberSlices + 1) + ((size_t)j + 1);
      indices[indexIndices++] = (size_t)i * ((size_t)numberSlices + 1) + ((size_t)j + 1);
    }
  }

  return ImmutableModel(std::move(vertices),
                        std::move(normals),
                        std::move(texCoords),
                        std::move(tangents),
                        std::move(indices));
}

ImmutableModel ImmutableModel::CreateCube(float halfExtend) {
  constexpr const float cubeVertices[] =
      {-1.0f, -1.0f, -1.0f, +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, -1.0f, +1.0f,
       -1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f,
       -1.0f, -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, -1.0f, +1.0f,
       -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f,
       -1.0f, -1.0f, -1.0f, +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f, -1.0f, +1.0f,
       +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, +1.0f};
  constexpr const float cubeNormals[] =
      {0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
       0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f,
       0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,
       0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f,
       -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
       +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f};
  constexpr const float cubeTexCoords[] =
      {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
       0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
       1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
       0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
       0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
       1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
  constexpr const float cubeTangents[] =
      {+1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f,
       +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f,
       -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
       +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f,
       0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f, +1.0f,
       0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f};
  constexpr const size_t cubeIndices[] =
      {0, 2, 1, 0, 3, 2,
       4, 5, 6, 4, 6, 7,
       8, 9, 10, 8, 10, 11,
       12, 15, 14, 12, 14, 13,
       16, 17, 18, 16, 18, 19,
       20, 23, 22, 20, 22, 21};

  constexpr const uint32_t numberVertices = 24;
  constexpr const uint32_t numberIndices = 36;

  std::vector<Vector3f> vertices(numberVertices, Vector3f{});
  std::vector<Vector3f> normals(numberVertices, Vector3f{});
  std::vector<Vector2f> texCoords(numberVertices, Vector2f{});
  std::vector<Vector3f> tangents(numberVertices, Vector3f{});
  for (uint32_t i = 0; i < numberVertices; i++) {
    vertices[i] = {cubeVertices[i * 4 + 0] * halfExtend,
                   cubeVertices[i * 4 + 1] * halfExtend,
                   cubeVertices[i * 4 + 2] * halfExtend};
    normals[i] = {cubeNormals[i * 3 + 0],
                  cubeNormals[i * 3 + 1],
                  cubeNormals[i * 3 + 2]};
    texCoords[i] = {cubeTexCoords[i * 2 + 0],
                    cubeTexCoords[i * 2 + 1]};
    tangents[i] = {cubeTangents[i * 2 + 0],
                   cubeTangents[i * 2 + 1],
                   cubeTangents[i * 2 + 2]};
  }
  std::vector<size_t> indices(cubeIndices, cubeIndices + numberIndices);
  return ImmutableModel(std::move(vertices),
                        std::move(normals),
                        std::move(texCoords),
                        std::move(tangents),
                        std::move(indices));
}

ImmutableModel ImmutableModel::CreateQuad(float halfExtend) {
  constexpr const float quadVertices[] =
      {-1.0f, -1.0f, 0.0f, +1.0f,
       +1.0f, -1.0f, 0.0f, +1.0f,
       -1.0f, +1.0f, 0.0f, +1.0f,
       +1.0f, +1.0f, 0.0f, +1.0f};
  constexpr const float quadNormal[] =
      {0.0f, 0.0f, 1.0f,
       0.0f, 0.0f, 1.0f,
       0.0f, 0.0f, 1.0f,
       0.0f, 0.0f, 1.0f};
  constexpr const float quadTex[] =
      {0.0f, 0.0f,
       1.0f, 0.0f,
       0.0f, 1.0f,
       1.0f, 1.0f};
  constexpr const float quadTan[] =
      {1.0f, 0.0f, 0.0f,
       1.0f, 0.0f, 0.0f,
       1.0f, 0.0f, 0.0f,
       1.0f, 0.0f, 0.0f};
  constexpr const size_t quadIndices[] =
      {0, 1, 2,
       1, 3, 2};
  constexpr const uint32_t numberVertices = 4;
  constexpr const uint32_t numberIndices = 6;
  std::vector<Vector3f> vertices(numberVertices, Vector3f{});
  std::vector<Vector3f> normals(numberVertices, Vector3f{});
  std::vector<Vector2f> texCoords(numberVertices, Vector2f{});
  std::vector<Vector3f> tangents(numberVertices, Vector3f{});
  for (uint32_t i = 0; i < numberVertices; i++) {
    vertices[i] = {quadVertices[i * 4 + 0] * halfExtend,
                   quadVertices[i * 4 + 1] * halfExtend,
                   quadVertices[i * 4 + 2]};
    normals[i] = {quadNormal[i * 3 + 0],
                  quadNormal[i * 3 + 1],
                  quadNormal[i * 3 + 2]};
    texCoords[i] = {quadTex[i * 2 + 0],
                    quadTex[i * 2 + 1]};
    tangents[i] = {quadTan[i * 3 + 0],
                   quadTan[i * 3 + 1],
                   quadTan[i * 3 + 2]};
  }
  std::vector<size_t> indices(quadIndices, quadIndices + numberIndices);
  return ImmutableModel(std::move(vertices),
                        std::move(normals),
                        std::move(texCoords),
                        std::move(tangents),
                        std::move(indices));
}

ImmutableModel ImmutableModel::CreateCylinder(float halfExtend, float radius, int numberSlices) {
  int numberVertices = (numberSlices + 2) * 2 + (numberSlices + 1) * 2;
  int numberIndices = numberSlices * 3 * 2 + numberSlices * 6;
  float angleStep = (2.0f * PI_F) / ((float)numberSlices);
  std::vector<Vector3f> positions(numberVertices, Vector3f{});
  std::vector<Vector3f> normals(numberVertices, Vector3f{});
  std::vector<Vector2f> texCoords(numberVertices, Vector2f{});
  std::vector<Vector3f> tangents(numberVertices, Vector3f{});
  std::vector<size_t> indices(numberIndices, 0);

  size_t vertexCounter = 0;
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
  tangents[vertexCounter][3] = 1.0f;

  texCoords[vertexCounter][0] = 0.0f;
  texCoords[vertexCounter][1] = 0.0f;

  vertexCounter++;
  // Bottom
  for (int i = 0; i < numberSlices + 1; i++) {
    float currentAngle = angleStep * (float)i;

    positions[vertexCounter][0] = std::cos(currentAngle) * radius;
    positions[vertexCounter][1] = -halfExtend;
    positions[vertexCounter][2] = -std::sin(currentAngle) * radius;

    normals[vertexCounter][0] = 0.0f;
    normals[vertexCounter][1] = -1.0f;
    normals[vertexCounter][2] = 0.0f;

    tangents[vertexCounter][0] = std::sin(currentAngle);
    tangents[vertexCounter][1] = 0.0f;
    tangents[vertexCounter][2] = std::cos(currentAngle);
    tangents[vertexCounter][3] = 1.0f;

    texCoords[vertexCounter][0] = 0.0f;
    texCoords[vertexCounter][1] = 0.0f;

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
  tangents[vertexCounter][3] = 1.0f;

  texCoords[vertexCounter][0] = 1.0f;
  texCoords[vertexCounter][1] = 1.0f;

  vertexCounter++;
  // Top
  for (int i = 0; i < numberSlices + 1; i++) {
    float currentAngle = angleStep * (float)i;

    positions[vertexCounter][0] = std::cos(currentAngle) * radius;
    positions[vertexCounter][1] = halfExtend;
    positions[vertexCounter][2] = -std::sin(currentAngle) * radius;

    normals[vertexCounter][0] = 0.0f;
    normals[vertexCounter][1] = 1.0f;
    normals[vertexCounter][2] = 0.0f;

    tangents[vertexCounter][0] = -std::sin(currentAngle);
    tangents[vertexCounter][1] = 0.0f;
    tangents[vertexCounter][2] = -std::cos(currentAngle);
    tangents[vertexCounter][3] = 1.0f;

    texCoords[vertexCounter][0] = 1.0f;
    texCoords[vertexCounter][1] = 1.0f;

    vertexCounter++;
  }
  for (int i = 0; i < numberSlices + 1; i++) {
    float currentAngle = angleStep * (float)i;
    float sign = -1.0f;
    for (int j = 0; j < 2; j++) {
      positions[vertexCounter][0] = std::cos(currentAngle) * radius;
      positions[vertexCounter][1] = halfExtend * sign;
      positions[vertexCounter][2] = -std::sin(currentAngle) * radius;

      normals[vertexCounter][0] = std::cos(currentAngle);
      normals[vertexCounter][1] = 0.0f;
      normals[vertexCounter][2] = -std::sin(currentAngle);

      tangents[vertexCounter][0] = -std::sin(currentAngle);
      tangents[vertexCounter][1] = 0.0f;
      tangents[vertexCounter][2] = -std::cos(currentAngle);

      texCoords[vertexCounter][0] = (float)i / (float)numberSlices;
      texCoords[vertexCounter][1] = (sign + 1.0f) / 2.0f;

      vertexCounter++;

      sign = 1.0f;
    }
  }
  // index
  // Bottom
  size_t centerIndex = 0;
  size_t indexCounter = 1;
  size_t indexIndices = 0;

  for (int i = 0; i < numberSlices; i++) {
    indices[indexIndices++] = centerIndex;
    indices[indexIndices++] = indexCounter + 1;
    indices[indexIndices++] = indexCounter;

    indexCounter++;
  }
  indexCounter++;

  // Top
  centerIndex = indexCounter;
  indexCounter++;

  for (int i = 0; i < numberSlices; i++) {
    indices[indexIndices++] = centerIndex;
    indices[indexIndices++] = indexCounter;
    indices[indexIndices++] = indexCounter + 1;

    indexCounter++;
  }
  indexCounter++;

  // Sides
  for (int i = 0; i < numberSlices; i++) {
    indices[indexIndices++] = indexCounter;
    indices[indexIndices++] = indexCounter + 2;
    indices[indexIndices++] = indexCounter + 1;

    indices[indexIndices++] = indexCounter + 2;
    indices[indexIndices++] = indexCounter + 3;
    indices[indexIndices++] = indexCounter + 1;

    indexCounter += 2;
  }

  return ImmutableModel(std::move(positions),
                        std::move(normals),
                        std::move(texCoords),
                        std::move(tangents),
                        std::move(indices));
}

ImmutableModel ImmutableModel::CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount) {
  float stackHeight = height / stackCount;
  float radiusStep = (topRadius - bottomRadius) / stackCount;
  uint32_t ringCount = stackCount + 1;

  std::vector<Vector3f> positions;
  std::vector<Vector3f> normals;
  std::vector<Vector2f> texCoords;
  std::vector<Vector3f> tangents;
  std::vector<size_t> indices;

  for (uint32_t i = 0; i < ringCount; ++i) {
    float y = -0.5f * height + i * stackHeight;
    float r = bottomRadius + i * radiusStep;
    float dTheta = 2.0f * PI_F / sliceCount;
    for (uint32_t j = 0; j <= sliceCount; ++j) {
      float c = std::cos(j * dTheta);
      float s = std::sin(j * dTheta);
      Vector3f position(r * c, y, r * s);

      Vector2f texCoord{};
      texCoord.x() = (float)j / sliceCount;
      texCoord.y() = 1.0f - (float)i / stackCount;

      Vector3f tangent(-s, 0.0f, c);

      float dr = bottomRadius - topRadius;
      Vector3f bitangent(dr * c, -height, dr * s);

      Vector3f T = tangent;
      Vector3f B = bitangent;
      Vector3f N = T.cross(B).normalized();
      Vector3f normal = N;

      positions.emplace_back(position);
      texCoords.emplace_back(texCoord);
      tangents.emplace_back(tangent);
      normals.emplace_back(normal);
    }
  }
  size_t ringVertexCount = size_t(sliceCount) + 1;
  for (size_t i = 0; i < stackCount; ++i) {
    for (size_t j = 0; j < sliceCount; ++j) {
      indices.push_back(i * ringVertexCount + j);
      indices.push_back((i + 1) * ringVertexCount + j);
      indices.push_back((i + 1) * ringVertexCount + j + 1);

      indices.push_back(i * ringVertexCount + j);
      indices.push_back((i + 1) * ringVertexCount + j + 1);
      indices.push_back(i * ringVertexCount + j + 1);
    }
  }
  {  // TOP
    size_t baseIndex = positions.size();
    float y = 0.5f * height;
    float dTheta = 2.0f * PI_F / sliceCount;
    for (size_t i = 0; i <= sliceCount; ++i) {
      float x = topRadius * std::cos(i * dTheta);
      float z = topRadius * std::sin(i * dTheta);
      float u = x / height + 0.5f;
      float v = z / height + 0.5f;

      positions.emplace_back(Vector3f{x, y, z});
      texCoords.emplace_back(Vector2f{u, v});
      tangents.emplace_back(Vector3f{1.0f, 0.0f, 0.0f});
      normals.emplace_back(Vector3f{0.0f, 1.0f, 0.0f});
    }
    positions.emplace_back(Vector3f{0.0f, y, 0.0f});
    texCoords.emplace_back(Vector2f{0.5f, 0.5f});
    tangents.emplace_back(Vector3f{1.0f, 0.0f, 0.0f});
    normals.emplace_back(Vector3f{0.0f, 1.0f, 0.0f});
    size_t centerIndex = positions.size() - 1;
    for (size_t i = 0; i < sliceCount; ++i) {
      indices.push_back(centerIndex);
      indices.push_back(baseIndex + i + 1);
      indices.push_back(baseIndex + i);
    }
  }
  {  // bottom
    size_t baseIndex = positions.size();
    float y = -0.5f * height;
    float dTheta = 2.0f * PI_F / sliceCount;
    for (size_t i = 0; i <= sliceCount; ++i) {
      float x = bottomRadius * cosf(i * dTheta);
      float z = bottomRadius * sinf(i * dTheta);
      float u = x / height + 0.5f;
      float v = z / height + 0.5f;

      positions.emplace_back(Vector3f{x, y, z});
      texCoords.emplace_back(Vector2f{u, v});
      tangents.emplace_back(Vector3f{1.0f, 0.0f, 0.0f});
      normals.emplace_back(Vector3f{0.0f, -1.0f, 0.0f});
    }
    positions.emplace_back(Vector3f{0.0f, y, 0.0f});
    texCoords.emplace_back(Vector2f{0.5f, 0.5f});
    tangents.emplace_back(Vector3f{1.0f, 0.0f, 0.0f});
    normals.emplace_back(Vector3f{0.0f, -1.0f, 0.0f});
    size_t centerIndex = positions.size() - 1;
    for (size_t i = 0; i < sliceCount; ++i) {
      indices.push_back(centerIndex);
      indices.push_back(baseIndex + i);
      indices.push_back(baseIndex + i + 1);
    }
  }
  positions.shrink_to_fit();
  normals.shrink_to_fit();
  texCoords.shrink_to_fit();
  tangents.shrink_to_fit();
  indices.shrink_to_fit();
  return ImmutableModel(std::move(positions),
                        std::move(normals),
                        std::move(texCoords),
                        std::move(tangents),
                        std::move(indices));
}

ImmutableModel ImmutableModel::CreateGrid(float width, float depth, uint32_t m, uint32_t n) {
  size_t vertexCount = size_t(m) * size_t(n);
  size_t faceCount = (size_t(m) - 1) * (size_t(n) - 1) * 2;
  std::vector<Vector3f> positions(vertexCount);
  std::vector<Vector3f> normals(vertexCount);
  std::vector<Vector2f> texCoords(vertexCount);
  std::vector<Vector3f> tangents(vertexCount);
  std::vector<size_t> indices(faceCount * 3);
  float halfWidth = 0.5f * width;
  float halfDepth = 0.5f * depth;
  float dx = width / (n - 1);
  float dz = depth / (m - 1);
  float du = 1.0f / (n - 1);
  float dv = 1.0f / (m - 1);
  for (size_t i = 0; i < m; ++i) {
    float z = halfDepth - i * dz;
    for (size_t j = 0; j < n; ++j) {
      float x = -halfWidth + j * dx;
      positions[i * n + j] = Vector3f(x, 0.0f, z);
      normals[i * n + j] = Vector3f(0.0f, 1.0f, 0.0f);
      tangents[i * n + j] = Vector3f(1.0f, 0.0f, 0.0f);
      texCoords[i * n + j].x() = j * du;
      texCoords[i * n + j].y() = i * dv;
    }
  }
  size_t k = 0;
  for (size_t i = 0; i < size_t(m) - 1; ++i) {
    for (size_t j = 0; j < size_t(n) - 1; ++j) {
      indices[k] = i * n + j;
      indices[k + 1] = i * n + j + 1;
      indices[k + 2] = (i + 1) * n + j;
      indices[k + 3] = (i + 1) * n + j;
      indices[k + 4] = i * n + j + 1;
      indices[k + 5] = (i + 1) * n + j + 1;
      k += 6;
    }
  }
  return ImmutableModel(std::move(positions),
                        std::move(normals),
                        std::move(texCoords),
                        std::move(tangents),
                        std::move(indices));
}

}  // namespace Rad
