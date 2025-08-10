/*
* (C) 2024 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/ec_inner_bn.h>

namespace Botan {

const EC_Scalar_Data_BN& EC_Scalar_Data_BN::checked_ref(const EC_Scalar_Data& data) {
   const auto* p = dynamic_cast<const EC_Scalar_Data_BN*>(&data);
   if(!p) {
      throw Invalid_State("Failed conversion to EC_Scalar_Data_BN");
   }
   return *p;
}

const std::shared_ptr<const EC_Group_Data>& EC_Scalar_Data_BN::group() const {
   return m_group;
}

size_t EC_Scalar_Data_BN::bytes() const {
   return this->group()->order_bytes();
}

std::unique_ptr<EC_Scalar_Data> EC_Scalar_Data_BN::clone() const {
   return std::make_unique<EC_Scalar_Data_BN>(this->group(), this->value());
}

bool EC_Scalar_Data_BN::is_zero() const {
   return this->value().is_zero();
}

bool EC_Scalar_Data_BN::is_eq(const EC_Scalar_Data& other) const {
   return (value() == checked_ref(other).value());
}

void EC_Scalar_Data_BN::assign(const EC_Scalar_Data& other) {
   m_v = checked_ref(other).value();
}

void EC_Scalar_Data_BN::square_self() {
   m_group->square_mod_order(m_v);
}

std::unique_ptr<EC_Scalar_Data> EC_Scalar_Data_BN::negate() const {
   return std::make_unique<EC_Scalar_Data_BN>(m_group, m_group->mod_order(-m_v));
}

std::unique_ptr<EC_Scalar_Data> EC_Scalar_Data_BN::invert() const {
   return std::make_unique<EC_Scalar_Data_BN>(m_group, m_group->inverse_mod_order(m_v));
}

std::unique_ptr<EC_Scalar_Data> EC_Scalar_Data_BN::add(const EC_Scalar_Data& other) const {
   return std::make_unique<EC_Scalar_Data_BN>(m_group, m_group->mod_order(m_v + checked_ref(other).value()));
}

std::unique_ptr<EC_Scalar_Data> EC_Scalar_Data_BN::sub(const EC_Scalar_Data& other) const {
   return std::make_unique<EC_Scalar_Data_BN>(m_group, m_group->mod_order(m_v - checked_ref(other).value()));
}

std::unique_ptr<EC_Scalar_Data> EC_Scalar_Data_BN::mul(const EC_Scalar_Data& other) const {
   return std::make_unique<EC_Scalar_Data_BN>(m_group, m_group->multiply_mod_order(m_v, checked_ref(other).value()));
}

void EC_Scalar_Data_BN::serialize_to(std::span<uint8_t> bytes) const {
   BOTAN_ARG_CHECK(bytes.size() == m_group->order_bytes(), "Invalid output length");
   m_v.serialize_to(bytes);
}

EC_AffinePoint_Data_BN::EC_AffinePoint_Data_BN(std::shared_ptr<const EC_Group_Data> group, EC_Point pt) :
      m_group(std::move(group)), m_pt(std::move(pt)) {
   m_pt.force_affine();
   m_xy = m_pt.xy_bytes();
}

EC_AffinePoint_Data_BN::EC_AffinePoint_Data_BN(std::shared_ptr<const EC_Group_Data> group,
                                               std::span<const uint8_t> pt) :
      m_group(std::move(group)) {
   BOTAN_ASSERT_NONNULL(m_group);
   m_pt = Botan::OS2ECP(pt.data(), pt.size(), m_group->curve());
   m_xy = m_pt.xy_bytes();
}

std::unique_ptr<EC_AffinePoint_Data> EC_AffinePoint_Data_BN::clone() const {
   return std::make_unique<EC_AffinePoint_Data_BN>(m_group, m_pt);
}

const std::shared_ptr<const EC_Group_Data>& EC_AffinePoint_Data_BN::group() const {
   return m_group;
}

std::unique_ptr<EC_AffinePoint_Data> EC_AffinePoint_Data_BN::mul(const EC_Scalar_Data& scalar,
                                                                 RandomNumberGenerator& rng,
                                                                 std::vector<BigInt>& ws) const {
   BOTAN_ARG_CHECK(scalar.group() == m_group, "Curve mismatch");
   const auto& bn = EC_Scalar_Data_BN::checked_ref(scalar);

   EC_Point_Var_Point_Precompute mul(m_pt, rng, ws);

   const auto order = m_group->order() * m_group->cofactor();
   auto pt = mul.mul(bn.value(), rng, order, ws);
   return std::make_unique<EC_AffinePoint_Data_BN>(m_group, std::move(pt));
}

size_t EC_AffinePoint_Data_BN::field_element_bytes() const {
   return m_xy.size() / 2;
}

void EC_AffinePoint_Data_BN::serialize_x_to(std::span<uint8_t> bytes) const {
   const size_t fe_bytes = this->field_element_bytes();
   BOTAN_ARG_CHECK(bytes.size() == fe_bytes, "Invalid output size");
   copy_mem(bytes, std::span{m_xy}.first(fe_bytes));
}

void EC_AffinePoint_Data_BN::serialize_y_to(std::span<uint8_t> bytes) const {
   const size_t fe_bytes = this->field_element_bytes();
   BOTAN_ARG_CHECK(bytes.size() == fe_bytes, "Invalid output size");
   copy_mem(bytes, std::span{m_xy}.last(fe_bytes));
}

void EC_AffinePoint_Data_BN::serialize_xy_to(std::span<uint8_t> bytes) const {
   const size_t fe_bytes = this->field_element_bytes();
   BOTAN_ARG_CHECK(bytes.size() == 2 * fe_bytes, "Invalid output size");
   copy_mem(bytes, m_xy);
}

void EC_AffinePoint_Data_BN::serialize_compressed_to(std::span<uint8_t> bytes) const {
   const size_t fe_bytes = this->field_element_bytes();
   BOTAN_ARG_CHECK(bytes.size() == 1 + fe_bytes, "Invalid output size");
   const bool y_is_odd = (m_xy[m_xy.size() - 1] & 0x01) == 0x01;

   BufferStuffer stuffer(bytes);
   stuffer.append(y_is_odd ? 0x03 : 0x02);
   serialize_x_to(stuffer.next(fe_bytes));
}

void EC_AffinePoint_Data_BN::serialize_uncompressed_to(std::span<uint8_t> bytes) const {
   const size_t fe_bytes = this->field_element_bytes();
   BOTAN_ARG_CHECK(bytes.size() == 1 + 2 * fe_bytes, "Invalid output size");
   BufferStuffer stuffer(bytes);
   stuffer.append(0x04);
   stuffer.append(m_xy);
}

EC_Mul2Table_Data_BN::EC_Mul2Table_Data_BN(const EC_AffinePoint_Data& g, const EC_AffinePoint_Data& h) :
      m_group(g.group()), m_tbl(g.to_legacy_point(), h.to_legacy_point()) {
   BOTAN_ARG_CHECK(h.group() == m_group, "Curve mismatch");
}

std::unique_ptr<EC_AffinePoint_Data> EC_Mul2Table_Data_BN::mul2_vartime(const EC_Scalar_Data& x,
                                                                        const EC_Scalar_Data& y) const {
   BOTAN_ARG_CHECK(x.group() == m_group && y.group() == m_group, "Curve mismatch");

   const auto& bn_x = EC_Scalar_Data_BN::checked_ref(x);
   const auto& bn_y = EC_Scalar_Data_BN::checked_ref(y);
   auto pt = m_tbl.multi_exp(bn_x.value(), bn_y.value());

   if(pt.is_zero()) {
      return nullptr;
   }
   return std::make_unique<EC_AffinePoint_Data_BN>(m_group, std::move(pt));
}

bool EC_Mul2Table_Data_BN::mul2_vartime_x_mod_order_eq(const EC_Scalar_Data& v,
                                                       const EC_Scalar_Data& x,
                                                       const EC_Scalar_Data& y) const {
   BOTAN_ARG_CHECK(x.group() == m_group && y.group() == m_group && v.group() == m_group, "Curve mismatch");

   const auto& bn_v = EC_Scalar_Data_BN::checked_ref(v);
   const auto& bn_x = EC_Scalar_Data_BN::checked_ref(x);
   const auto& bn_y = EC_Scalar_Data_BN::checked_ref(y);
   const auto pt = m_tbl.multi_exp(bn_x.value(), bn_y.value());

   if(pt.is_zero()) {
      return false;
   }

   /*
   * The trick used below doesn't work for curves with cofactors
   */
   if(m_group->has_cofactor()) {
      return m_group->mod_order(pt.get_affine_x()) == bn_v.value();
   }

   /*
   * Note we're working with the projective coordinate directly here!
   * Nominally we're comparing v with the affine x coordinate.
   *
   * return m_group->mod_order(pt.get_affine_x()) == bn_v.value();
   *
   * However by instead projecting r to an identical z as the x
   * coordinate, we can compare without having to perform an
   * expensive inversion in the field.
   *
   * That is, given (x*z2) and r, instead of checking if
   *    (x*z2)*z2^-1 == r,
   * we check if
   *    (x*z2) == (r*z2)
   */
   auto& curve = m_group->curve();

   secure_vector<word> ws;
   BigInt vr = bn_v.value();
   curve.to_rep(vr, ws);
   BigInt z2, v_z2;
   curve.sqr(z2, pt.get_z(), ws);
   curve.mul(v_z2, vr, z2, ws);

   /*
   * Since (typically) the group order is slightly less than the size
   * of the field elements, its possible the signer had to reduce the
   * r component. If they did not reduce r, then this value is correct.
   *
   * Due to the Hasse bound, this case occurs almost always; the
   * probability that a reduction was actually required is
   * approximately 1 in 2^(n/2) where n is the bit length of the curve.
   */
   if(pt.get_x() == v_z2) {
      return true;
   }

   if(m_group->order_is_less_than_p()) {
      vr = bn_v.value() + m_group->order();
      if(vr < m_group->p()) {
         curve.to_rep(vr, ws);
         curve.mul(v_z2, vr, z2, ws);

         if(pt.get_x() == v_z2) {
            return true;
         }
      }
   }

   // Reject:
   return false;
}

}  // namespace Botan
