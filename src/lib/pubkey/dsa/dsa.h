/*
* DSA
* (C) 1999-2010,2023 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DSA_H_
#define BOTAN_DSA_H_

#include <botan/pk_keys.h>
#include <memory>

namespace Botan {

class BigInt;
class DL_Group;
class DL_PublicKey;
class DL_PrivateKey;

/**
* DSA Public Key
*/
class BOTAN_PUBLIC_API(2,0) DSA_PublicKey : public virtual Public_Key
   {
   public:
      bool supports_operation(PublicKeyOperation op) const override
         {
         return (op == PublicKeyOperation::Signature);
         }

      /**
      * Load a public key from the ASN.1 encoding
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      DSA_PublicKey(const AlgorithmIdentifier& alg_id,
                    const std::vector<uint8_t>& key_bits);

      /**
      * Load a public key from the integer value
      * @param group the underlying DL group
      * @param y the public value y = g^x mod p
      */
      DSA_PublicKey(const DL_Group& group, const BigInt& y);

      std::string algo_name() const override { return "DSA"; }

      size_t message_parts() const override { return 2; }
      size_t message_part_size() const override;

      AlgorithmIdentifier algorithm_identifier() const override;
      std::vector<uint8_t> public_key_bits() const override;

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      size_t estimated_strength() const override;
      size_t key_length() const override;

      const BigInt& get_int_field(const std::string& field) const override;

      std::unique_ptr<PK_Ops::Verification>
         create_verification_op(const std::string& params,
                                const std::string& provider) const override;

      std::unique_ptr<PK_Ops::Verification>
         create_x509_verification_op(const AlgorithmIdentifier& signature_algorithm,
                                     const std::string& provider) const override;
   private:
      friend class DSA_PrivateKey;

      DSA_PublicKey() = default;

      DSA_PublicKey(std::shared_ptr<const DL_PublicKey> key) :
         m_public_key(key) {}

      std::shared_ptr<const DL_PublicKey> m_public_key;
   };

/**
* DSA Private Key
*/
class BOTAN_PUBLIC_API(2,0) DSA_PrivateKey final :
   public DSA_PublicKey,
   public virtual Private_Key
   {
   public:
      /**
      * Load a private key from the ASN.1 encoding
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded key bits in ANSI X9.57 format
      */
      DSA_PrivateKey(const AlgorithmIdentifier& alg_id,
                     const secure_vector<uint8_t>& key_bits);

      /**
      * Create a new private key.
      * @param group the underlying DL group
      * @param rng the RNG to use
      */
      DSA_PrivateKey(RandomNumberGenerator& rng,
                     const DL_Group& group);

      /**
      * Load a private key
      * @param group the underlying DL group
      * @param private_key the private key
      */
      DSA_PrivateKey(const DL_Group& group,
                     const BigInt& private_key);

      std::unique_ptr<Public_Key> public_key() const override;

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      secure_vector<uint8_t> private_key_bits() const override;

      const BigInt& get_int_field(const std::string& field) const override;

      std::unique_ptr<PK_Ops::Signature>
         create_signature_op(RandomNumberGenerator& rng,
                             const std::string& params,
                             const std::string& provider) const override;
   private:
      std::shared_ptr<const DL_PrivateKey> m_private_key;
   };

}

#endif
