#include <rad/offline/render/shape.h>

#include <rad/core/config_node.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>
#include <rad/offline/render/mesh_base.h>

namespace Rad {

/**
 * @brief [-1,1]^3的矩形，拿数据拼出来的
 */
class Cube final : public MeshBase {
 public:
  Cube(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) : MeshBase(ctx, toWorld, cfg) {
    std::vector<Eigen::Vector3f> vertices =
        {{1, -1, -1},
         {1, -1, 1},
         {-1, -1, 1},
         {-1, -1, -1},
         {1, 1, -1},
         {-1, 1, -1},
         {-1, 1, 1},
         {1, 1, 1},
         {1, -1, -1},
         {1, 1, -1},
         {1, 1, 1},
         {1, -1, 1},
         {1, -1, 1},
         {1, 1, 1},
         {-1, 1, 1},
         {-1, -1, 1},
         {-1, -1, 1},
         {-1, 1, 1},
         {-1, 1, -1},
         {-1, -1, -1},
         {1, 1, -1},
         {1, -1, -1},
         {-1, -1, -1},
         {-1, 1, -1}};
    std::vector<Eigen::Vector3f> normals =
        {{0, -1, 0},
         {0, -1, 0},
         {0, -1, 0},
         {0, -1, 0},
         {0, 1, 0},
         {0, 1, 0},
         {0, 1, 0},
         {0, 1, 0},
         {1, 0, 0},
         {1, 0, 0},
         {1, 0, 0},
         {1, 0, 0},
         {0, 0, 1},
         {0, 0, 1},
         {0, 0, 1},
         {0, 0, 1},
         {-1, 0, 0},
         {-1, 0, 0},
         {-1, 0, 0},
         {-1, 0, 0},
         {0, 0, -1},
         {0, 0, -1},
         {0, 0, -1},
         {0, 0, -1}};
    std::vector<Eigen::Vector2f> texcoords =
        {{0, 1},
         {1, 1},
         {1, 0},
         {0, 0},
         {0, 1},
         {1, 1},
         {1, 0},
         {0, 0},
         {0, 1},
         {1, 1},
         {1, 0},
         {0, 0},
         {0, 1},
         {1, 1},
         {1, 0},
         {0, 0},
         {0, 1},
         {1, 1},
         {1, 0},
         {0, 0},
         {0, 1},
         {1, 1},
         {1, 0},
         {0, 0}};
    std::vector<Eigen::Vector3i> triangles =
        {{0, 1, 2},
         {3, 0, 2},
         {4, 5, 6},
         {7, 4, 6},
         {8, 9, 10},
         {11, 8, 10},
         {12, 13, 14},
         {15, 12, 14},
         {16, 17, 18},
         {19, 16, 18},
         {20, 21, 22},
         {23, 20, 22}};
    _indexCount = 36;
    _vertexCount = 24;
    _triangleCount = 12;

    _position = Share<Eigen::Vector3f[]>(new Eigen::Vector3f[_vertexCount]);
    _normal = Share<Eigen::Vector3f[]>(new Eigen::Vector3f[_vertexCount]);
    _uv = Share<Eigen::Vector2f[]>(new Eigen::Vector2f[_vertexCount]);
    _indices = Share<UInt32[]>(new UInt32[_indexCount]);
    for (UInt32 i = 0; i < _vertexCount; i++) {
      _position[i] = _toWorld.ApplyAffineToWorld(vertices[i].cast<Float>()).cast<Float32>();
      _normal[i] = _toWorld.ApplyNormalToWorld(normals[i].cast<Float>()).cast<Float32>();
      _uv[i] = _uv[i];
    }
    for (UInt32 i = 0; i < _triangleCount; i++) {
      for (UInt32 j = 0; j < 3; j++) {
        _indices[i * 3 + j] = triangles[i][j];
      }
    }
    UpdateDistibution();
  }
  ~Cube() noexcept override = default;
};

class CubeFactory final : public ShapeFactory {
 public:
  CubeFactory() : ShapeFactory("cube") {}
  ~CubeFactory() noexcept override = default;
  Unique<Shape> Create(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) const override {
    return std::make_unique<Cube>(ctx, toWorld, cfg);
  }
};

Unique<ShapeFactory> _FactoryCreateRectangleFunc_() {
  return std::make_unique<CubeFactory>();
}

}  // namespace Rad
