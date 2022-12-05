#include <rad/offline/render/texture.h>

#include <rad/core/config_node.h>
#include <rad/offline/math_ext.h>
#include <rad/offline/transform.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>
#include <rad/offline/build/config_node_ext.h>

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
      toUV = ConfigNodeAsTransform(node);
    } else {
      toUV = Matrix4::Identity();
    }
    _transform = Transform(toUV);
    _a = ConfigNodeReadTexture(ctx, cfg, "color_a", T(Float32(0.4f)));
    _b = ConfigNodeReadTexture(ctx, cfg, "color_b", T(Float32(0.2f)));
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

class ChessboardFactory : public TextureFactory {
 public:
  ChessboardFactory() : TextureFactory("chessboard") {}
  ~ChessboardFactory() noexcept override = default;
  Unique<TextureRGB> CreateRgb(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<Chessboard<Color24f>>(ctx, cfg);
  }
  Unique<TextureMono> CreateMono(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<Chessboard<Float32>>(ctx, cfg);
  }
};

Unique<TextureFactory> _FactoryCreateChessboardFunc_() {
  return std::make_unique<ChessboardFactory>();
}

}  // namespace Rad
