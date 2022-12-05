#include <rad/offline/render/medium.h>

#include <rad/offline/build/config_node_ext.h>
#include <rad/offline/render/volume.h>
#include <rad/offline/transform.h>

namespace Rad {

/**
 * @brief 非均匀的参与介质
 * 假设介质数据定义在[-1,1]^3范围内，使用时需要在外面套一个[-1,1]^3的模型，一般是cube
 */
class HeterogeneousMedium final : public Medium {
 public:
  HeterogeneousMedium(BuildContext* ctx, const Matrix4& entityToWorld, const ConfigNode& cfg) : Medium(ctx, cfg) {
    //将数据缩小到[0,1]^3来采样
    Eigen::Translation<Float, 3> t(Vector3::Constant(Float(-0.5)));
    Eigen::DiagonalMatrix<Float, 3> s(Vector3::Constant(Float(2)));
    Eigen::Transform<Float, 3, Eigen::Affine> affine = s * t;

    _sigmaT = ConfigNodeReadVolume(ctx, cfg, "sigma_t", Spectrum(Float(1)));
    _albedo = ConfigNodeReadVolume(ctx, cfg, "albedo", Spectrum(Float(0.75)));
    _scale = cfg.ReadOrDefault("scale", Float(1));
    Matrix4 toWorld = entityToWorld * affine.matrix();
    _toWorld = Transform(toWorld);

    BoundingBox3 box(Vector3::Constant(0), Vector3::Constant(1));
    _bbox = _toWorld.ApplyBoxToWorld(box);
    _maxDensity = _sigmaT->GetMaxValue() * _scale;
    _isHomogeneous = false;
    _hasSpectralExtinction = true;
  }

  std::tuple<bool, Float, Float> IntersectAABB(const Ray& ray) const override {
    return BoundingBox3RayIntersect(_bbox, ray);
  }

  Spectrum GetMajorant(const MediumInteraction& mi) const override {
    return _maxDensity;
  }

  std::tuple<Spectrum, Spectrum, Spectrum> GetScatteringCoefficients(const MediumInteraction& mi) const override {
    Spectrum sigmat = EvalSigmaT(mi);
    Spectrum albedo = EvalAlbedo(mi);
    Spectrum sigmas(sigmat.cwiseProduct(albedo));
    Spectrum sigman(Spectrum::Constant(_maxDensity) - sigmat);
    return {sigmas, sigman, sigmat};
  }

  Spectrum EvalSigmaT(const MediumInteraction& mi) const {
    Interaction its{};
    its.P = _toWorld.ApplyAffineToLocal(mi.P);
    Spectrum e = _sigmaT->Eval(its);
    Spectrum sigmat(e * _scale);
    return sigmat;
  }

  Spectrum EvalAlbedo(const MediumInteraction& mi) const {
    Interaction its{};
    its.P = _toWorld.ApplyAffineToLocal(mi.P);
    Spectrum albedo = _albedo->Eval(its);
    return albedo;
  }

 private:
  static std::tuple<bool, Float, Float> BoundingBox3RayIntersect(
      const BoundingBox3& box,
      const Ray& ray) {
    auto active = !ray.D.isZero(0) || box.contains(ray.O);
    auto d_rcp = ray.D.cwiseInverse();
    auto t1 = (box.min() - ray.O).cwiseProduct(d_rcp);
    auto t2 = (box.max() - ray.O).cwiseProduct(d_rcp);
    auto t1p = t1.cwiseMin(t2);
    auto t2p = t1.cwiseMax(t2);
    Float mint = t1p.maxCoeff();
    Float maxt = t2p.minCoeff();
    active = active && maxt >= mint;
    return std::make_tuple(active, mint, maxt);
  }

  Unique<Volume> _sigmaT;
  Unique<Volume> _albedo;
  Float _scale;
  Float _maxDensity;
  Transform _toWorld;
  BoundingBox3 _bbox;
};

class HeterogeneousMediumFactory : public MediumFactory {
 public:
  HeterogeneousMediumFactory() : MediumFactory("heterogeneous") {}
  virtual ~HeterogeneousMediumFactory() noexcept = default;
  Unique<Medium> Create(BuildContext* ctx, const Matrix4& toWorld, const ConfigNode& cfg) const override {
    return std::make_unique<HeterogeneousMedium>(ctx, toWorld, cfg);
  }
};

Unique<MediumFactory> _FactoryCreateHeterogeneousMediumFunc_() {
  return std::make_unique<HeterogeneousMediumFactory>();
}

}  // namespace Rad
