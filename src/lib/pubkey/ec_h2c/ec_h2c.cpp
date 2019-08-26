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
#include <botan/hkdf.h>
#include <botan/hash.h>


#include <botan/hex.h> // for debugging

namespace Botan {

BigInt hash_to_base(const EC_Group& group,
                    const std::string& hash_fn,
                    const uint8_t input[], size_t input_len,
                    const uint8_t domain_sep[], size_t domain_sep_len,
                    uint8_t ctr,
                    size_t k)
   {


#if 0
   // draft-04:
   std::unique_ptr<MessageAuthenticationCode> hmac = MessageAuthenticationCode::create_or_throw("HMAC(" + hash_fn + ")");

   secure_vector<uint8_t> prk(hmac->output_length());
   HKDF_Extract hkdf_extract(hmac->clone());
   const size_t prk_written = hkdf_extract.kdf(
      prk.data(), prk.size(), input, input_len, domain_sep, domain_sep_len, nullptr, 0);

   BOTAN_ASSERT_NOMSG(prk_written == prk.size());

   // HKDF-Extract(salt,IKM) -> PRK
   HKDF_Expand hkdf_expand(hmac->clone());
   const uint8_t salt[5] = { 'H', '2', 'C', ctr, 0x00 };

   const size_t L = (group.get_p_bits() + k) / 8;
   secure_vector<uint8_t> kdf_output(L);

   const size_t kdf_output_written =
      hkdf_expand.kdf(kdf_output.data(), kdf_output.size(),
                       prk.data(), prk.size(),
                       &salt[0], sizeof(salt),
                       nullptr, 0);
   BOTAN_ASSERT_NOMSG(kdf_output_written == kdf_output.size());

   BigInt v(kdf_output.data(), kdf_output.size());

   return group.mod_order(v);// wrong!
#else

   // matching the sage code:
   std::unique_ptr<HashFunction> hash = HashFunction::create_or_throw(hash_fn);

   hash->update("h2b");
   hash->update(domain_sep, domain_sep_len);
   hash->update_be(uint32_t(input_len));
   hash->update(input, input_len);
   const secure_vector<uint8_t> xin = hash->final();
   //printf("xin = %s\n", hex_encode(xin).c_str());

   const size_t h_bytes = hash->output_length();
   const size_t h_bits = h_bytes * 8;
   const size_t hash_invocations = (group.get_p_bits() + k + h_bits - 1) / h_bits;

   secure_vector<uint8_t> t(hash_invocations * h_bytes);
   for(size_t i = 0; i != hash_invocations; ++i)
      {
      hash->update(xin);
      hash->update(ctr);
      hash->update(uint8_t(0)); // idx
      hash->update(uint8_t(i)); // jdk
      hash->final(&t[i * h_bytes]);
      }

   //printf("t = %s\n", hex_encode(t).c_str());
   BigInt bn(t);
   return bn % group.get_p(); // not const time! need group.reduce_mod_p()
#endif
   }

static void dump(const char* n, const BigInt& v)
   {
   //std::cout << n << " " << std::hex << v << "\n";
   }

static int32_t sgn0(const BigInt& v, const BigInt& p)
   {
   if(v > (p-1)/2)
      return -1;
   else
      return 1;
   }

PointGFp map_to_curve_sswu(const EC_Group& group, const BigInt& u)
   {
   const BigInt& p = group.get_p();
   const BigInt& A = group.get_a();
   const BigInt& B = group.get_b();

   if(A.is_zero() || B.is_zero() || p % 4 == 1)
      throw Invalid_Argument("map_to_curve_sswu does not support this curve");

   // FIXME depends on curve!!!!!
   BigInt Z;
   if(p.bits() == 384)
      Z = p - 1;
   else
      Z = p - 2;

   // These could be precomputed:
   const Modular_Reducer mod_p(p);
   const BigInt c1 = mod_p.multiply(p - B, inverse_mod(A, p));
   dump("MB_OVER_A", c1);
   const BigInt c2 = mod_p.multiply(p - 1, inverse_mod(Z, p));
   dump("M1_OVER_Z", c2);

   /*
   1.   t1 = Z * u^2
   2.   t2 = t1^2
   3.   x1 = t1 + t2
   4.   x1 = inv0(x1)
   5.   e1 = x1 == 0
   6.   x1 = x1 + 1
   7.   x1 = CMOV(x1, c2, e1)    // if (t1 + t2) == 0, set x1 = -1 / Z
   8.   x1 = x1 * c1      // x1 = (-B / A) * (1 + (1 / (Z^2 * u^4 + Z * u^2)))
   9.  gx1 = x1^2
   10. gx1 = gx1 + A
   11. gx1 = gx1 * x1
   12. gx1 = gx1 + B             // gx1 = g(x1) = x1^3 + A * x1 + B
   13.  x2 = t1 * x1             // x2 = Z * u^2 * x1
   14.  t2 = t1 * t2
   15. gx2 = gx1 * t2            // gx2 = (Z * u^2)^3 * gx1
   16.  e2 = is_square(gx1)
   17.   x = CMOV(x2, x1, e2)    // If is_square(gx1), x = x1, else x = x2
   18.  y2 = CMOV(gx2, gx1, e2)  // If is_square(gx1), y2 = gx1, else y2 = gx2
   19.   y = sqrt(y2)
   20.  e3 = sgn0(u) == sgn0(y)  // fix sign of y
   21.   y = CMOV(-y, y, e3)
   22. return (x, y)
   */

   BigInt t1 = mod_p.multiply(Z, mod_p.square(u));
   dump("t1", t1);
   BigInt t2 = mod_p.square(t1);
   dump("t2", t2);
   BigInt x1 = inverse_mod(t1 + t2, p);
   const bool e1 = x1.is_zero();
   x1 += 1;
   x1.ct_cond_assign(e1, c2);
   x1 = mod_p.multiply(x1, c1);
   dump("x1", x1);
   BigInt gx1 = mod_p.square(x1);
   gx1 += A;
   gx1 = mod_p.multiply(gx1, x1);
   gx1 += B;
   gx1 = mod_p.reduce(gx1);
   dump("gx1", gx1);

   const BigInt x2 = mod_p.multiply(t1, x1);
   dump("x2", x2);
   t2 = mod_p.multiply(t1, t2);
   const BigInt gx2 = mod_p.multiply(gx1, t2);
   dump("gx2", gx2);

   const BigInt gx1_euler_crit = power_mod(gx1, (p - 1)/2, p);
   const bool gx1_is_square = (gx1_euler_crit <= 1);
   //printf("gx1_is_square %d\n", gx1_is_square);

   BigInt x = x2;
   x.ct_cond_assign(gx1_is_square, x1);

   BigInt y2 = gx2;
   y2.ct_cond_assign(gx1_is_square, gx1);

   BigInt y = power_mod(y2, (p + 1)/4, p);
   if(mod_p.square(y) != y2)
      printf("bad sqrt\n");
   dump("x", x);
   dump("y", y);

   PointGFp pt = group.point(x, y);

   if(sgn0(u, p) != sgn0(y, p))
      pt.negate();

   return pt;
   }

PointGFp hash_to_curve_sswu(const EC_Group& group,
                            const std::string& hash_fn,
                            const uint8_t input[],
                            size_t input_len,
                            const uint8_t domain_sep[],
                            size_t domain_sep_len)
   {
   const BigInt u = hash_to_base(group, hash_fn, input, input_len, domain_sep, domain_sep_len, 0);
   return map_to_curve_sswu(group, u);
   }

}
