/*
* (C) 2019 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <iostream>
#include <botan/ec_h2c.h>
#include <botan/numthry.h>
#include <botan/reducer.h>
#include <botan/ec_group.h>
#include <botan/kdf.h>

namespace Botan {

namespace {

BigInt hash_to_base(const EC_Group& group, KDF& kdf,
                    const uint8_t input[], size_t input_len,
                    uint8_t differentiator = 0)
   {
   const size_t order_bytes = group.get_order_bytes(); // should be larger?

   secure_vector<uint8_t> kdf_output(order_bytes);

   // Use differentiator as salt, label is empty
   kdf.kdf(kdf_output.data(), kdf_output.size(),
           input, input_len,
           &differentiator, 1,
           nullptr, 0);

   BigInt v(kdf_output.data(), kdf_output.size());

   return group.mod_order(v);
   }

}

PointGFp hash_to_curve_swu(const EC_Group& group,
                           const HashFunction& hash,
                           KDF& kdf,
                           const uint8_t input[],
                           size_t input_len)
   {
   const BigInt& p = group.get_p();
   const BigInt& a = group.get_a();
   const BigInt& b = group.get_b();

   if(a.is_zero() || b.is_zero())
      throw Invalid_Argument("hash_to_curve_swu does not support this curve");

   const Modular_Reducer mod_p(p);

   // This could be precomputed
   const BigInt n_b_over_a = mod_p.multiply(p - b, inverse_mod(a, p));

   const BigInt t = hash_to_base(group, kdf, input, input_len, 0);
   const BigInt u = hash_to_base(group, kdf, input, input_len, 1);

   const BigInt t2 = mod_p.square(t);
   const BigInt t3 = mod_p.multiply(t, t2);
   const BigInt t4 = mod_p.square(t2);
   const BigInt t6 = mod_p.square(t3);

   // X1(t,u) = u
   const BigInt gx1 = mod_p.reduce(mod_p.cube(u) + mod_p.multiply(a, u) + b);

   // x2(t,u) = (-b/a) * (t^6 * g(u)^3 - 1) / (g(u) * (t^6 * g(u)^2 - t^2))
   // (1 + 1 / (t^4 * g(u)^2 + t^2 * g(u)))

   #if 0
   const BigInt x2_n = mod_p.reduce(mod_p.multiply(t6, mod_p.cube(gx1)) - 1);
   const BigInt x2_d = mod_p.multiply(gx1, mod_p.reduce(mod_p.multiply(t6, mod_p.square(gx1)) - t2));

   const BigInt x2 = mod_p.multiply(x2_n, inverse_mod(x2_d, p));
   #else
   const BigInt d1 = mod_p.multiply(mod_p.square(gx1), t4);
   const BigInt d2 = mod_p.multiply(gx1, t2);
   const BigInt d3 = 1 + inverse_mod(d1 + d2, p);

   const BigInt x2 = mod_p.multiply(d3, n_b_over_a);
   #endif

   const BigInt gx2 = mod_p.reduce(mod_p.cube(x2) + mod_p.multiply(a, x2) + b);

   // X3(t,u) = t^3 * g(u) * X2(t,u)
   const BigInt x3 = mod_p.multiply(mod_p.square(gx1), mod_p.multiply(t3, x2));

   const BigInt gx3 = mod_p.reduce(mod_p.cube(x3) + mod_p.multiply(a, x3) + b);

   const BigInt gx1_sqrt = ressol(gx1, group.get_p());
   const BigInt gx2_sqrt = ressol(gx2, group.get_p());
   const BigInt gx3_sqrt = ressol(gx3, group.get_p());

   const BigInt check1 = mod_p.square(mod_p.multiply(t3, mod_p.square(gx1), gx2));
   const BigInt check2 = mod_p.multiply(gx1, gx2, gx3);

   if(check1 != check2)
      printf("bad check\n");
   else
      printf("check ok\n");
   if(gx1_sqrt < 0 && gx2_sqrt < 0 && gx3_sqrt < 0)
      {
      printf("i give up\n");
      //exit(1);
      }

   const bool use_gx1 = gx1_sqrt > 0;
   const bool use_gx2 = gx2_sqrt > 0;
   const bool use_gx3 = gx3_sqrt > 0;

   BOTAN_ASSERT_NOMSG(use_gx1 || use_gx2 || use_gx3);
   printf("%d %d %d\n", use_gx1, use_gx2, use_gx3);

   BigInt rx, ry;

   rx.ct_cond_assign(use_gx3, x3);
   ry.ct_cond_assign(use_gx3, gx3_sqrt);

   rx.ct_cond_assign(use_gx2, x2);
   ry.ct_cond_assign(use_gx2, gx2_sqrt);

   rx.ct_cond_assign(use_gx1, u);
   ry.ct_cond_assign(use_gx1, gx1_sqrt);

   return group.point(rx, ry);
   /*
   if(gx1_sqrt > 0)
      {
      return group.point(u, gx1_sqrt);
      }
   if(gx2_sqrt > 0)
      {
      return group.point(x2, gx2_sqrt);
      }

   return group.point(x3, gx3_sqrt);
   */
   }

}
