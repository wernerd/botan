/*
 * Tests for Crystals Kyber
 * - simple roundtrip test
 * - KAT tests using the KAT vectors from
 *   https://csrc.nist.gov/CSRC/media/Projects/post-quantum-cryptography/documents/round-3/submissions/Kyber-Round3.zip
 *
 * (C) 2021-2022 Jack Lloyd
 * (C) 2021-2022 Manuel Glaser and Michael Boric, Rohde & Schwarz Cybersecurity
 * (C) 2021-2022 René Meusel and Hannes Rantzsch, neXenio GmbH
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#include "test_rng.h"
#include "tests.h"

#include <iterator>
#include <memory>

#if defined(BOTAN_HAS_KYBER) || defined(BOTAN_HAS_KYBER_90S)
   #include <botan/block_cipher.h>
   #include <botan/hex.h>
   #include <botan/kyber.h>
   #include <botan/oids.h>
   #include <botan/rng.h>
#endif

namespace Botan_Tests {

#if defined(BOTAN_HAS_KYBER) || defined(BOTAN_HAS_KYBER_90S)

class KYBER_Tests final : public Test
   {
   public:
      Test::Result run_kyber_test(const char* test_name, Botan::KyberMode mode, size_t strength)
         {
         Test::Result result(test_name);

         const size_t shared_secret_length = 32;

         // Alice
         const Botan::Kyber_PrivateKey priv_key(Test::rng(), mode);
         const auto pub_key = priv_key.public_key();

         result.test_eq("estimated strength private", priv_key.estimated_strength(), strength);
         result.test_eq("estimated strength public", pub_key->estimated_strength(), strength);

         // Serialize
         const auto priv_key_bits = priv_key.private_key_bits();
         const auto pub_key_bits = pub_key->public_key_bits();

         // Bob (reading from serialized public key)
         Botan::Kyber_PublicKey alice_pub_key(pub_key_bits, mode, Botan::KyberKeyEncoding::Full);
         const auto enc = alice_pub_key.create_kem_encryption_op(Test::rng(), "", "");

         Botan::secure_vector<uint8_t> cipher_text, key_bob;
         enc->kem_encrypt(cipher_text, key_bob, shared_secret_length, Test::rng(), nullptr, 0);

         // Alice (reading from serialized private key)
         Botan::Kyber_PrivateKey alice_priv_key(priv_key_bits, mode, Botan::KyberKeyEncoding::Full);
         const auto dec = alice_priv_key.create_kem_decryption_op(Test::rng(), "", "");
         const auto key_alice =
            dec->kem_decrypt(cipher_text.data(), cipher_text.size(), shared_secret_length, nullptr, 0);

         result.confirm("shared secrets are equal", key_alice == key_bob);

         //
         // negative tests
         //

         // Broken cipher_text from Alice (wrong length)
         result.test_throws("fail to read cipher_text", "unexpected length of ciphertext buffer", [&]
            {
            dec->kem_decrypt(cipher_text.data(), cipher_text.size() - 5, shared_secret_length, nullptr, 0);
            });

         // Invalid cipher_text from Alice
         Botan::secure_vector<uint8_t> reverse_cipher_text;
         std::copy(cipher_text.crbegin(), cipher_text.crend(), std::back_inserter(reverse_cipher_text));
         const auto key_alice2 =
            dec->kem_decrypt(reverse_cipher_text.data(), reverse_cipher_text.size(), shared_secret_length, nullptr, 0);
         result.confirm("shared secrets are not equal", key_alice != key_alice2);

         return result;
         }

      std::vector<Test::Result> run() override
         {
         std::vector<Test::Result> results;

#if defined(BOTAN_HAS_KYBER_90S)
         results.push_back(run_kyber_test("Kyber512_90s API", Botan::KyberMode::Kyber512_90s, 128));
         results.push_back(run_kyber_test("Kyber768_90s API", Botan::KyberMode::Kyber768_90s, 192));
         results.push_back(run_kyber_test("Kyber1024_90s API", Botan::KyberMode::Kyber1024_90s, 256));
#endif
#if defined(BOTAN_HAS_KYBER)
         results.push_back(run_kyber_test("Kyber512 API", Botan::KyberMode::Kyber512, 128));
         results.push_back(run_kyber_test("Kyber768 API", Botan::KyberMode::Kyber768, 192));
         results.push_back(run_kyber_test("Kyber1024 API", Botan::KyberMode::Kyber1024, 256));
#endif

         return results;
         }
   };
BOTAN_REGISTER_TEST("kyber", "kyber_pairwise", KYBER_Tests);

namespace {

Test::Result run_kyber_test(const char* test_name, const VarMap& vars, Botan::KyberMode mode,
                            const std::string& algo_name)
   {
   Test::Result result(test_name);

   // read input from test file
   const auto random_in = vars.get_req_bin("Random");
   const auto pk_in = vars.get_req_bin("PK");
   const auto sk_in = vars.get_req_bin("SK");
   const auto ct_in = vars.get_req_bin("CT");
   const auto ss_in = vars.get_req_bin("SS");

   const size_t shared_secret_length = 32;

   // Kyber test RNG
   Fixed_Output_RNG kyber_test_rng(random_in);

   // Alice
   Botan::Kyber_PrivateKey priv_key(kyber_test_rng, mode);
   priv_key.set_binary_encoding(Botan::KyberKeyEncoding::Raw);
   const auto pub_key = priv_key.public_key();
   result.test_eq("Public Key Output", priv_key.public_key_bits(), pk_in);
   result.test_eq("Secret Key Output", priv_key.private_key_bits(), sk_in);

   // Bob
   auto enc = pub_key->create_kem_encryption_op(kyber_test_rng, "", "");
   Botan::secure_vector<uint8_t> cipher_text, key_bob;
   enc->kem_encrypt(cipher_text, key_bob, shared_secret_length, kyber_test_rng, nullptr, 0);
   result.test_eq("Cipher-Text Output", cipher_text, ct_in);
   result.test_eq("Key B Output", key_bob, ss_in);

   // Alice
   auto dec = priv_key.create_kem_decryption_op(kyber_test_rng, "", "");
   auto key_alice = dec->kem_decrypt(cipher_text.data(), cipher_text.size(), shared_secret_length, nullptr, 0);
   result.test_eq("Key A Output", key_alice, ss_in);

   // Algorithm identifiers
   result.test_eq("algo name", priv_key.algo_name(), algo_name);
   result.test_eq("algo id", Botan::OIDS::oid2str_or_throw(priv_key.algorithm_identifier().oid()), algo_name);

   return result;
   }

} // namespace

#define REGISTER_KYBER_KAT_TEST(mode)                                                                                  \
    class KYBER_KAT_##mode final : public Text_Based_Test                                                              \
    {                                                                                                                  \
      public:                                                                                                          \
        KYBER_KAT_##mode() : Text_Based_Test("pubkey/kyber_" #mode ".vec", "Count,Seed,Random,PK,SK,CT,SS")            \
        {                                                                                                              \
        }                                                                                                              \
                                                                                                                       \
        Test::Result run_one_test(const std::string &name, const VarMap &vars) override                                \
        {                                                                                                              \
            return run_kyber_test("Kyber_" #mode, vars, Botan::KyberMode::Kyber##mode, name);                          \
        }                                                                                                              \
    };                                                                                                                 \
    BOTAN_REGISTER_TEST("kyber", "kyber_kat_" #mode, KYBER_KAT_##mode)

#if defined(BOTAN_HAS_KYBER_90S)
   REGISTER_KYBER_KAT_TEST(512_90s);
   REGISTER_KYBER_KAT_TEST(768_90s);
   REGISTER_KYBER_KAT_TEST(1024_90s);
#endif
#if defined(BOTAN_HAS_KYBER)
   REGISTER_KYBER_KAT_TEST(512);
   REGISTER_KYBER_KAT_TEST(768);
   REGISTER_KYBER_KAT_TEST(1024);
#endif

#undef REGISTER_KYBER_KAT_TEST

class Kyber_Encoding_Test : public Text_Based_Test
   {
   public:
      Kyber_Encoding_Test()
         : Text_Based_Test("pubkey/kyber_encodings.vec", "PrivateRaw,PrivateFull,PublicRaw,PublicFull", "Error")
         {
         }

   private:
      Botan::KyberMode name_to_mode(const std::string& algo_name)
         {
         if(algo_name == "Kyber-512-r3")
            { return Botan::KyberMode::Kyber512; }
         if(algo_name == "Kyber-512-90s-r3")
            { return Botan::KyberMode::Kyber512_90s; }
         if(algo_name == "Kyber-768-r3")
            { return Botan::KyberMode::Kyber768; }
         if(algo_name == "Kyber-768-90s-r3")
            { return Botan::KyberMode::Kyber768_90s; }
         if(algo_name == "Kyber-1024-r3")
            { return Botan::KyberMode::Kyber1024; }
         if(algo_name == "Kyber-1024-90s-r3")
            { return Botan::KyberMode::Kyber1024_90s; }

         throw Botan::Invalid_Argument("don't know kyber mode: " + algo_name);
         }

   public:
      Test::Result run_one_test(const std::string& algo_name, const VarMap& vars) override
         {
         Test::Result result("kyber_encodings");

         const auto mode = name_to_mode(algo_name);

         const auto sk_full = Botan::hex_decode_locked(vars.get_req_str("PrivateFull"));
         const auto pk_raw = Botan::hex_decode(vars.get_req_str("PublicRaw"));
         const auto sk_raw = Botan::hex_decode_locked(vars.get_req_str("PrivateRaw"));
         const auto pk_full = Botan::hex_decode(vars.get_req_str("PublicFull"));
         const auto error = vars.get_opt_str("Error", "");

         if(!error.empty())
            {
            // negative tests

            result.test_throws("failing decoding", error, [&]
               {
               if(!sk_full.empty())
                  Botan::Kyber_PrivateKey(sk_full, mode, Botan::KyberKeyEncoding::Full);
               if(!sk_raw.empty())
                  Botan::Kyber_PrivateKey(sk_raw, mode, Botan::KyberKeyEncoding::Raw);
               if(!pk_raw.empty())
                  Botan::Kyber_PublicKey(pk_raw, mode, Botan::KyberKeyEncoding::Raw);
               if(!pk_full.empty())
                  Botan::Kyber_PublicKey(pk_full, mode, Botan::KyberKeyEncoding::Full);
               });

            return result;
            }

         const auto pk_matches = [&](const auto &pk, const std::string &from_encoding)
            {
            pk->set_binary_encoding(Botan::KyberKeyEncoding::Raw);
            result.test_eq(from_encoding + " matches raw public key", pk->public_key_bits(), pk_raw);
            pk->set_binary_encoding(Botan::KyberKeyEncoding::Full);
            result.test_eq(from_encoding + " matches full public key", pk->public_key_bits(), pk_full);
            };

         const auto sk_matches = [&](const auto &sk, const std::string &from_encoding)
            {
            pk_matches(sk, from_encoding);

            sk->set_binary_encoding(Botan::KyberKeyEncoding::Raw);
            result.test_eq(from_encoding + " matches raw private key", sk->private_key_bits(), sk_raw);
            sk->set_binary_encoding(Botan::KyberKeyEncoding::Full);
            result.test_eq(from_encoding + " matches full private key", sk->private_key_bits(), sk_full);
            };

         const auto skr = std::make_unique<Botan::Kyber_PrivateKey>(sk_raw, mode, Botan::KyberKeyEncoding::Raw);
         sk_matches(skr, "raw");
         const auto pkr = std::make_unique<Botan::Kyber_PublicKey>(pk_raw, mode, Botan::KyberKeyEncoding::Raw);
         pk_matches(pkr, "raw");

         const auto skf = std::make_unique<Botan::Kyber_PrivateKey>(sk_full, mode, Botan::KyberKeyEncoding::Full);
         sk_matches(skf, "full");
         const auto pkf = std::make_unique<Botan::Kyber_PublicKey>(pk_full, mode, Botan::KyberKeyEncoding::Full);
         pk_matches(pkf, "full");

         return result;
         }
   };

BOTAN_REGISTER_TEST("kyber", "kyber_encodings", Kyber_Encoding_Test);

#endif

} // namespace Botan_Tests
