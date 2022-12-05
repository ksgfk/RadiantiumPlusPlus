#include <rad/offline/render/mesh_base.h>

#include <rad/core/config_node.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>

namespace Rad {

/**
 * @brief 从资产中读取三角形网格
 */
class Mesh final : public MeshBase {
 public:
  Mesh(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) : MeshBase(ctx, toWorld, cfg) {
    std::string assetName = cfg.Read<std::string>("asset_name");
    const ModelAsset* modelAsset = ctx->GetAssetManager().Borrow<ModelAsset>(assetName);

    Share<TriangleModel> model;
    {
      std::string submodelName;
      if (cfg.TryRead<std::string>("sub_model", submodelName)) {
        model = modelAsset->GetSubModel(submodelName);
      } else {
        model = modelAsset->FullModel();
      }
    }

    if (_toWorld.IsIdentity()) {
      _position = model->GetPosition();
      _normal = model->GetNormal();
    } else {
      _position = std::shared_ptr<Eigen::Vector3f[]>(new Eigen::Vector3f[model->VertexCount()]);
      std::shared_ptr<Eigen::Vector3f[]> p = model->GetPosition();
      for (size_t i = 0; i < model->VertexCount(); i++) {
        _position[i] = _toWorld.ApplyAffineToWorld(p[i].cast<Float>()).cast<Float32>();
      }
      if (model->HasNormal()) {
        _normal = std::shared_ptr<Eigen::Vector3f[]>(new Eigen::Vector3f[model->VertexCount()]);
        std::shared_ptr<Eigen::Vector3f[]> n = model->GetNormal();
        for (size_t i = 0; i < model->VertexCount(); i++) {
          _normal[i] = _toWorld.ApplyNormalToWorld(n[i].cast<Float>()).cast<Float32>();
        }
      }
    }
    _indices = model->GetIndices();
    _uv = model->GetUV();
    _vertexCount = model->VertexCount();
    _indexCount = model->IndexCount();
    _triangleCount = model->TriangleCount();

    UpdateDistibution();
  }
  ~Mesh() noexcept override = default;
};

class MeshFactory final : public ShapeFactory {
 public:
  MeshFactory() : ShapeFactory("mesh") {}
  ~MeshFactory() noexcept override = default;
  Unique<Shape> Create(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) const override {
    return std::make_unique<Mesh>(ctx, toWorld, cfg);
  }
};

Unique<ShapeFactory> _FactoryCreateMeshFunc_() {
  return std::make_unique<MeshFactory>();
}

}  // namespace Rad
