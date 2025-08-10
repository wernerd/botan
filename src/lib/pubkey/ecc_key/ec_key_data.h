/*
* (C) 2024 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_EC_KEY_DATA_H_
#define BOTAN_EC_KEY_DATA_H_

#include <botan/ec_apoint.h>
#include <botan/ec_group.h>
#include <botan/ec_scalar.h>

#include <botan/bigint.h>
#include <botan/ec_point.h>

namespace Botan {

class RandomNumberGenerator;

class EC_PublicKey_Data final {
   public:
      EC_PublicKey_Data(EC_Group group, EC_AffinePoint pt) :
            m_group(std::move(group)), m_point(std::move(pt)), m_legacy_point(m_point.to_legacy_point()) {}

      EC_PublicKey_Data(EC_Group group, std::span<const uint8_t> bytes);

      const EC_Group& group() const { return m_group; }

      const EC_AffinePoint& public_key() const { return m_point; }

      const EC_Point& legacy_point() const { return m_legacy_point; }

   private:
      EC_Group m_group;
      EC_AffinePoint m_point;
      EC_Point m_legacy_point;
};

class EC_PrivateKey_Data final {
   public:
      EC_PrivateKey_Data(EC_Group group, const BigInt& x);

      EC_PrivateKey_Data(EC_Group group, EC_Scalar x);

      EC_PrivateKey_Data(EC_Group group, std::span<const uint8_t> bytes);

      std::shared_ptr<EC_PublicKey_Data> public_key(RandomNumberGenerator& rng, bool with_modular_inverse) const;

      std::shared_ptr<EC_PublicKey_Data> public_key(bool with_modular_inverse) const;

      void serialize_to(std::span<uint8_t> output) const;

      template <typename T>
      T serialize() const {
         T bytes(this->group().get_order_bytes());
         this->serialize_to(bytes);
         return bytes;
      }

      const EC_Group& group() const { return m_group; }

      const EC_Scalar& private_key() const { return m_scalar; }

      const BigInt& legacy_bigint() const { return m_legacy_x; }

   private:
      EC_Group m_group;

      EC_Scalar m_scalar;
      BigInt m_legacy_x;
};

}  // namespace Botan

#endif
