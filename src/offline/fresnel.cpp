#include <rad/offline/render/fresnel.h>

using namespace Rad::Math;

namespace Rad {

Vector3 Fresnel::Reflect(const Vector3& wi) {
  return Vector3(-wi.x(), -wi.y(), wi.z());
}

Vector3 Fresnel::Conductor(Float cos_theta_i, const Vector3& eta, const Vector3& k) {
  Float cos_theta_i_2 = cos_theta_i * cos_theta_i,
        sin_theta_i_2 = 1.f - cos_theta_i_2,
        sin_theta_i_4 = sin_theta_i_2 * sin_theta_i_2;

  auto eta_r = eta,
       eta_i = k;

  // Float temp_1 = eta_r * eta_r - eta_i * eta_i - sin_theta_i_2,
  //       a_2_pb_2 = dr::safe_sqrt(temp_1 * temp_1 + 4.f * eta_i * eta_i * eta_r * eta_r),
  //       a = dr::safe_sqrt(.5f * (a_2_pb_2 + temp_1));

  auto temp_1 = eta_r.cwiseProduct(eta_r) - eta_i.cwiseProduct(eta_i) - Vector3::Constant(sin_theta_i_2);
  auto a_2_pb_2 = (temp_1.cwiseProduct(temp_1) + eta_i.cwiseProduct(eta_i).cwiseProduct(eta_r).cwiseProduct(eta_r) * 4).cwiseMax(0).cwiseSqrt();
  auto a = ((a_2_pb_2 + temp_1) * Float(0.5)).cwiseMax(0).cwiseSqrt();

  // auto term_1 = a_2_pb_2 + cos_theta_i_2,
  //      term_2 = 2.f * cos_theta_i * a;

  auto term_1 = a_2_pb_2 + Vector3::Constant(cos_theta_i_2);
  auto term_2 = a * 2 * cos_theta_i;

  // auto r_s = (term_1 - term_2) / (term_1 + term_2);
  auto r_s = (term_1 - term_2).cwiseProduct((term_1 + term_2).cwiseInverse());

  // auto term_3 = a_2_pb_2 * cos_theta_i_2 + sin_theta_i_4,
  //      term_4 = term_2 * sin_theta_i_2;
  auto term_3 = a_2_pb_2 * cos_theta_i_2 + Vector3::Constant(sin_theta_i_4);
  auto term_4 = term_2 * sin_theta_i_2;

  // auto r_p = r_s * (term_3 - term_4) / (term_3 + term_4);
  auto r_p = r_s.cwiseProduct(term_3 - term_4).cwiseProduct((term_3 + term_4).cwiseInverse());

  return (r_s + r_p) * 0.5f;
}

}  // namespace Rad
