/*
* (C) 2024 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/pcurves_instance.h>

#include <botan/internal/pcurves_solinas.h>
#include <botan/internal/pcurves_wrap.h>

namespace Botan::PCurve {

namespace {

template <typename Params>
class Secp384r1Rep final {
   public:
      static constexpr auto P = Params::P;
      static constexpr size_t N = Params::N;
      typedef typename Params::W W;

      constexpr static std::array<W, N> redc(const std::array<W, 2 * N>& z) {
         const int64_t X00 = get_uint32(z.data(), 0);
         const int64_t X01 = get_uint32(z.data(), 1);
         const int64_t X02 = get_uint32(z.data(), 2);
         const int64_t X03 = get_uint32(z.data(), 3);
         const int64_t X04 = get_uint32(z.data(), 4);
         const int64_t X05 = get_uint32(z.data(), 5);
         const int64_t X06 = get_uint32(z.data(), 6);
         const int64_t X07 = get_uint32(z.data(), 7);
         const int64_t X08 = get_uint32(z.data(), 8);
         const int64_t X09 = get_uint32(z.data(), 9);
         const int64_t X10 = get_uint32(z.data(), 10);
         const int64_t X11 = get_uint32(z.data(), 11);
         const int64_t X12 = get_uint32(z.data(), 12);
         const int64_t X13 = get_uint32(z.data(), 13);
         const int64_t X14 = get_uint32(z.data(), 14);
         const int64_t X15 = get_uint32(z.data(), 15);
         const int64_t X16 = get_uint32(z.data(), 16);
         const int64_t X17 = get_uint32(z.data(), 17);
         const int64_t X18 = get_uint32(z.data(), 18);
         const int64_t X19 = get_uint32(z.data(), 19);
         const int64_t X20 = get_uint32(z.data(), 20);
         const int64_t X21 = get_uint32(z.data(), 21);
         const int64_t X22 = get_uint32(z.data(), 22);
         const int64_t X23 = get_uint32(z.data(), 23);

         // One copy of P-384 is added to prevent underflow
         const int64_t S0 = 0xFFFFFFFF + X00 + X12 + X20 + X21 - X23;
         const int64_t S1 = 0x00000000 + X01 + X13 + X22 + X23 - X12 - X20;
         const int64_t S2 = 0x00000000 + X02 + X14 + X23 - X13 - X21;
         const int64_t S3 = 0xFFFFFFFF + X03 + X12 + X15 + X20 + X21 - X14 - X22 - X23;
         const int64_t S4 = 0xFFFFFFFE + X04 + X12 + X13 + X16 + X20 + X21 * 2 + X22 - X15 - X23 * 2;
         const int64_t S5 = 0xFFFFFFFF + X05 + X13 + X14 + X17 + X21 + X22 * 2 + X23 - X16;
         const int64_t S6 = 0xFFFFFFFF + X06 + X14 + X15 + X18 + X22 + X23 * 2 - X17;
         const int64_t S7 = 0xFFFFFFFF + X07 + X15 + X16 + X19 + X23 - X18;
         const int64_t S8 = 0xFFFFFFFF + X08 + X16 + X17 + X20 - X19;
         const int64_t S9 = 0xFFFFFFFF + X09 + X17 + X18 + X21 - X20;
         const int64_t SA = 0xFFFFFFFF + X10 + X18 + X19 + X22 - X21;
         const int64_t SB = 0xFFFFFFFF + X11 + X19 + X20 + X23 - X22;

         std::array<W, N> r = {};

         SolinasAccum sum(r);

         sum.accum(S0);
         sum.accum(S1);
         sum.accum(S2);
         sum.accum(S3);
         sum.accum(S4);
         sum.accum(S5);
         sum.accum(S6);
         sum.accum(S7);
         sum.accum(S8);
         sum.accum(S9);
         sum.accum(SA);
         sum.accum(SB);
         const auto S = sum.final_carry(0);

         BOTAN_DEBUG_ASSERT(S <= 4);

         const auto correction = p384_mul_mod_384(S);
         W borrow = bigint_sub2(r.data(), N, correction.data(), N);

         bigint_cnd_add(borrow, r.data(), N, P.data(), N);

         return r;
      }

      constexpr static std::array<W, N> one() { return std::array<W, N>{1}; }

      constexpr static std::array<W, N> to_rep(const std::array<W, N>& x) { return x; }

      constexpr static std::array<W, N> wide_to_rep(const std::array<W, 2 * N>& x) { return redc(x); }

      constexpr static std::array<W, N> from_rep(const std::array<W, N>& z) { return z; }

   private:
      // Return (i*P-384) % 2**384
      //
      // Assumes i is small
      constexpr static std::array<W, N> p384_mul_mod_384(W i) {
         static_assert(WordInfo<W>::bits == 32 || WordInfo<W>::bits == 64);

         // For small i, multiples of P-384 have a simple structure so it's faster to
         // compute the value directly vs a (constant time) table lookup

         auto r = P;
         if constexpr(WordInfo<W>::bits == 32) {
            r[4] -= i;
            r[3] -= i;
            r[1] += i;
            r[0] -= i;
         } else {
            const uint64_t i32 = static_cast<uint64_t>(i) << 32;
            r[2] -= i;
            r[1] -= i32;
            r[0] += i32;
            r[0] -= i;
         }
         return r;
      }
};

// clang-format off
namespace secp384r1 {

class Params final : public EllipticCurveParameters<
   "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFF",
   "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFC",
   "B3312FA7E23EE7E4988E056BE3F82D19181D9C6EFE8141120314088F5013875AC656398D8A2ED19D2A85C8EDD3EC2AEF",
   "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF581A0DB248B0A77AECEC196ACCC52973",
   "AA87CA22BE8B05378EB1C71EF320AD746E1D3B628BA79B9859F741E082542A385502F25DBF55296C3A545E3872760AB7",
   "3617DE4A96262C6F5D9E98BF9292DC29F8F41DBD289A147CE9DA3113B5F0B8C00A60B1CE1D7E819D7A431D7C90EA0E5F",
   -12> {
};

class Curve final : public EllipticCurve<Params, Secp384r1Rep> {};

}

// clang-format on

}  // namespace

std::shared_ptr<const PrimeOrderCurve> PCurveInstance::secp384r1() {
   return PrimeOrderCurveImpl<secp384r1::Curve>::instance();
}

}  // namespace Botan::PCurve
