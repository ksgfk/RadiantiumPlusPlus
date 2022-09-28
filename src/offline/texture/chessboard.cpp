#include <rad/offline/render/texture.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

namespace Rad {

/**
 * @brief 国际象棋棋盘纹理
 */
template <typename T>
class Chessboard final : public Texture<T> {
 public:
  Chessboard(BuildContext* ctx, const ConfigNode& cfg) : Texture<T>() {
    ConfigNode node;
    Matrix4 toUV;
    if (cfg.TryRead("to_uv", node)) {
      toUV = node.AsTransform();
    } else {
      toUV = Matrix4::Identity();
    }
    _transform = Transform(toUV);
    _a = cfg.ReadTexture(*ctx, "color_a", T(Float(0.4f)));
    _b = cfg.ReadTexture(*ctx, "color_b", T(Float(0.2f)));
    _remap = cfg.ReadOrDefault("remap", false);
    this->_width = 1;
    this->_height = 1;
  }

 protected:
  T EvalImpl(const SurfaceInteraction& si) const override {
    Vector3 t = _transform.ApplyAffineToWorld(Vector3(si.UV.x(), si.UV.y(), 0));
    Vector2 tuv(t.x(), t.y());
    Vector2 floorUV(std::floor(tuv.x()), std::floor(tuv.y()));
    Vector2 ab = tuv - floorUV;
    bool isRight = ab.x() > 0.5f;
    bool isUp = ab.y() > 0.5f;
    SurfaceInteraction tsi = si;
    T result;
    if (isRight == isUp) {
      if (_remap) {
        tsi.UV = (tuv - Vector2::Constant(Float(0.5))) * 2;
      }
      result = _a->Eval(tsi);
    } else {
      if (_remap) {
        tsi.UV = tuv * 2;
      }
      result = _b->Eval(tsi);
    }
    return result;
  }

  T ReadImpl(UInt32 x, UInt32 y) const override {
    return T(Float(0));
  }

 private:
  Unique<Texture<T>> _a;
  Unique<Texture<T>> _b;
  Transform _transform;
  bool _remap;
};

}  // namespace Rad

RAD_FACTORY_TEXTURE_DECLARATION(Chessboard, "chessboard");
