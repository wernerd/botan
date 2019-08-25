/*
* (C) 2014,2015,2019 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include "tests.h"

#if defined(BOTAN_HAS_EC_HASH_TO_CURVE)
   #include <botan/ec_h2c.h>
   #include <botan/kdf.h>
#endif

#if defined(BOTAN_HAS_ECC_GROUP)
   #include <botan/ec_group.h>
#endif

namespace Botan_Tests {

namespace {

#if defined(BOTAN_HAS_ECC_GROUP)

class ECC_Pointmult_Tests final : public Text_Based_Test
   {
   public:
      ECC_Pointmult_Tests() : Text_Based_Test("pubkey/ecc.vec", "m,X,Y") {}

      Test::Result run_one_test(const std::string& group_id, const VarMap& vars) override
         {
         Test::Result result("ECC Scalarmult " + group_id);

         const Botan::BigInt m = vars.get_req_bn("m");
         const Botan::BigInt X = vars.get_req_bn("X");
         const Botan::BigInt Y = vars.get_req_bn("Y");

         Botan::EC_Group group(Botan::OID::from_string(group_id));

         const Botan::PointGFp& base_point = group.get_base_point();

         const Botan::PointGFp p1 = base_point * m;
         result.test_eq("p1 affine X", p1.get_affine_x(), X);
         result.test_eq("p1 affine Y", p1.get_affine_y(), Y);

         std::vector<Botan::BigInt> ws;
         const Botan::PointGFp p2 = group.blinded_base_point_multiply(m, Test::rng(), ws);
         result.test_eq("p2 affine X", p2.get_affine_x(), X);
         result.test_eq("p2 affine Y", p2.get_affine_y(), Y);

         const Botan::PointGFp p3 = group.blinded_var_point_multiply(base_point, m, Test::rng(), ws);
         result.test_eq("p3 affine X", p3.get_affine_x(), X);
         result.test_eq("p3 affine Y", p3.get_affine_y(), Y);

         return result;
         }
   };

BOTAN_REGISTER_TEST("ecc_pointmul", ECC_Pointmult_Tests);

#endif

#if defined(BOTAN_HAS_EC_HASH_TO_CURVE)

class ECC_H2Ct_Tests final : public Text_Based_Test
   {
   public:
      ECC_H2Ct_Tests() : Text_Based_Test("pubkey/ec_h2c.vec", "Group,Hash,Input,Point") {}

      Test::Result run_one_test(const std::string& method, const VarMap& vars) override
         {
         Test::Result result("ECC hash to curve " + method);

         const std::string group_id = vars.get_req_str("Group");
         const std::string hash = vars.get_req_str("Hash");
         const std::vector<uint8_t> input = vars.get_req_bin("Input");
         const std::vector<uint8_t> exp_point_bin = vars.get_req_bin("Point");

         Botan::EC_Group group(group_id);
         const Botan::PointGFp expected = group.OS2ECP(exp_point_bin);

         if(method == "SWU")
            {
            std::unique_ptr<Botan::KDF> kdf = Botan::KDF::create(hash);
            const auto point = Botan::hash_to_curve_swu(group, *kdf, input.data(), input.size());

            printf("%s\n", Botan::hex_encode(point.encode(Botan::PointGFp::COMPRESSED)).c_str());
            result.confirm("Generated point is on the curve", point.on_the_curve());

            result.test_eq("Affine X", point.get_affine_x(), expected.get_affine_x());
            result.test_eq("Affine Y", point.get_affine_y(), expected.get_affine_y());
            }
         else
            {
            throw Test_Error("Unknown h2c method " + method);
            }

         return result;
         }
   };

BOTAN_REGISTER_TEST("ec_h2c", ECC_H2Ct_Tests);

#endif


}

}
