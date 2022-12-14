#include <rad/offline/render/volume.h>

#include <rad/core/config_node.h>
#include <rad/core/asset.h>
#include <rad/core/logger.h>
#include <rad/offline/build/build_context.h>
#include <rad/offline/build/factory.h>
#include <rad/offline/render/texture.h>
#include <rad/offline/math_ext.h>

namespace Rad {

/**
 * @brief 基于网格的体积数据
 */
class Grid final : public Volume {
 public:
  Grid(BuildContext* ctx, const ConfigNode& cfg) : Volume() {
    std::string assetName = cfg.Read<std::string>("asset_name");
    const VolumeAsset* asset = ctx->GetAssetManager().Borrow<VolumeAsset>(assetName);
    std::string gridName;
    if (cfg.TryRead("grid_name", gridName)) {
      _grid = asset->FindGrid(gridName);
    } else {
      _grid = asset->GetGrid();
    }
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
    Vector3 p = it.P;
    // if (p.x() < 0 || p.x() > 1 || p.y() < 0 || p.y() > 1 || p.z() < 0 || p.z() > 1) {
    //   return Spectrum(0);
    // }
    // Vector3 wrapP = p;
    Vector3 wrapP(Wrap(p.x(), _wrap), Wrap(p.y(), _wrap), Wrap(p.z(), _wrap));
    Vector3 fp = wrapP.cwiseProduct((_grid->GetSize().cast<Float>() - Vector3::Constant(1)));
    Eigen::Vector3i ip = fp.cast<int>();
    Int32 dpu = (fp.x() > (ip.x() + Float(0.5))) ? 1 : -1;
    Int32 dpv = (fp.y() > (ip.y() + Float(0.5))) ? 1 : -1;
    Int32 dpw = (fp.z() > (ip.z() + Float(0.5))) ? 1 : -1;
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
    return Spectrum(result);
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
    // if (u < 0 || u >= size.x() || v < 0 || v >= size.x() || w < 0 || w >= size.x()) {
    //   throw RadInvalidOperationException("越界了");
    // }
    size_t start = (((size_t)w * (size_t)size.y() + (size_t)v) * (size_t)size.x() + (size_t)u) * (size_t)channel;
    Spectrum spec;
    if (channel == 1) {
      spec = Spectrum(data[start]);
    } else {
      spec = Spectrum(data[start], data[start + 1], data[start + 2]);
    }
    return spec;
  }

  Share<VolumeGrid> _grid;
  WrapMode _wrap;
};

class GridFactory : public VolumeFactory {
 public:
  GridFactory() : VolumeFactory("volume_grid") {}
  ~GridFactory() noexcept override = default;
  Unique<Volume> Create(BuildContext* ctx, const ConfigNode& cfg) const override {
    return std::make_unique<Grid>(ctx, cfg);
  }
};

Unique<VolumeFactory> _FactoryCreateVolumeGridFunc_() {
  return std::make_unique<GridFactory>();
}

}  // namespace Rad
