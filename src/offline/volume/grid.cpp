#include <rad/offline/render/volume.h>

#include <rad/offline/common.h>
#include <rad/offline/build.h>
#include <rad/offline/render.h>
#include <rad/offline/asset.h>

namespace Rad {

enum class VolumeWrapMode {
  Repeat,
  Clamp
};

class Grid final : public Volume {
 public:
  Grid(BuildContext* ctx, const ConfigNode& cfg) : Volume() {
    std::string assetName = cfg.Read<std::string>("asset_name");
    VolumeAsset* asset = ctx->GetVolume(assetName);
    std::string gridName;
    if (cfg.TryRead("grid_name", gridName)) {
      _grid = asset->GetGrid(gridName);
    } else {
      _grid = asset->GetGrid();
    }
    ConfigNode toWorldNode;
    Matrix4 toWorld;
    if (cfg.TryRead("to_world", toWorldNode)) {
      toWorld = toWorldNode.AsTransform();
    } else {
      toWorld = Matrix4::Identity();
    }
    _toWorld = Transform(toWorld);
    UpdateBoundingBox();
    _maxValue = _grid->GetMaxValue();
    std::string wrapStr = cfg.ReadOrDefault("wrap", std::string("clamp"));
    if (wrapStr == "clamp") {
      _wrap = WrapMode::Clamp;
    } else if (wrapStr == "repeat") {
      _wrap = WrapMode::Repeat;
    } else {
      Logger::Get()->info("unknwon wrapper: {}, use clamp", wrapStr);
      _wrap = WrapMode::Clamp;
    }
  }
  ~Grid() noexcept override = default;

 protected:
  Spectrum EvalImpl(const Interaction& it) const override {
    Vector3 p = _toWorld.ApplyAffineToLocal(it.P);
    Vector3 wrapP(Wrap(p.x(), _wrap), Wrap(p.y(), _wrap), Wrap(p.z(), _wrap));
    Vector3 fp = wrapP.cwiseProduct((_grid->GetSize().cast<Float>() - Vector3::Constant(1)));
    Eigen::Vector3i ip = fp.cast<int>();
    Int32 dpu = (fp.x() > ip.x() + Float(0.5)) ? 1 : -1;
    Int32 dpv = (fp.y() > ip.y() + Float(0.5)) ? 1 : -1;
    Int32 dpw = (fp.z() > ip.z() + Float(0.5)) ? 1 : -1;
    Int32 apu = std::clamp(ip.x() + dpu, 0, _grid->GetSize().x() - 1);
    Int32 apv = std::clamp(ip.y() + dpv, 0, _grid->GetSize().y() - 1);
    Int32 apw = std::clamp(ip.z() + dpw, 0, _grid->GetSize().z() - 1);
    Float du = std::min(std::abs(fp.x() - ip.x() - Float(0.5)), Float(1));
    Float dv = std::min(std::abs(fp.y() - ip.y() - Float(0.5)), Float(1));
    Float dw = std::min(std::abs(fp.z() - ip.z() - Float(0.5)), Float(1));
    Spectrum u0v0w0 = Read(ip.x(), ip.y(), ip.z());
    Spectrum u1v0w0 = Read(apu, ip.y(), ip.z());
    Spectrum u0v1w0 = Read(ip.x(), apv, ip.z());
    Spectrum u0v0w1 = Read(ip.x(), ip.y(), apw);
    Spectrum u1v1w0 = Read(apu, apv, ip.z());
    Spectrum u0v1w1 = Read(ip.x(), apv, apw);
    Spectrum u1v0w1 = Read(apu, ip.y(), apw);
    Spectrum u1v1w1 = Read(apu, apv, apw);
    auto du_v0w0 = Math::Fmadd(u0v0w0, Vector3::Constant(1 - du), (u1v0w0 * du));
    auto du_v1w0 = Math::Fmadd(u0v1w0, Vector3::Constant(1 - du), (u1v1w0 * du));
    auto dv_w0 = Math::Fmadd(du_v0w0, Vector3::Constant(1 - dv), du_v1w0 * dv);
    auto du_v0w1 = Math::Fmadd(u0v0w1, Vector3::Constant(1 - du), (u1v0w1 * du));
    auto du_v1w1 = Math::Fmadd(u0v1w1, Vector3::Constant(1 - du), (u1v1w1 * du));
    auto dv_w1 = Math::Fmadd(du_v0w1, Vector3::Constant(1 - dv), du_v1w1 * dv);
    auto result = Math::Fmadd(dv_w0, Vector3::Constant(1 - dw), dv_w1 * dw);
    return result;
  }

 private:
  static Float Wrap(Float v, WrapMode wrap) {
    switch (wrap) {
      case WrapMode::Repeat:
        return std::clamp(v - std::floor(v), Float(0), Float(1));
      case WrapMode::Clamp:
      default:
        return std::clamp(v, Float(0), Float(1));
    }
  }

  Spectrum Read(Int32 u, Int32 v, Int32 w) const {
    const auto& data = _grid->GetData();
    const auto& size = _grid->GetSize();
    const Int32 channel = _grid->GetChannelCount();
    size_t start = (((size_t)w * (size_t)size.y() + (size_t)v) * (size_t)size.x() + (size_t)u) * (size_t)channel;
    Spectrum spec;
    if (channel == 1) {
      spec = Spectrum(data[start]);
    } else {
      spec = Spectrum(data[start], data[start + 1], data[start + 2]);
    }
    return spec;
  }

  Share<StaticGrid> _grid;
  WrapMode _wrap;
};

}  // namespace Rad

RAD_FACTORY_VOLUME_DECLARATION(Grid, "volume_grid");
