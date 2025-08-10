/*
* (C) 2024 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PCURVES_UTIL_H_
#define BOTAN_PCURVES_UTIL_H_

#include <botan/internal/mp_core.h>
#include <array>

namespace Botan {

namespace {

template <WordType W, size_t N, size_t XN>
inline consteval std::array<W, N> reduce_mod(const std::array<W, XN>& x, const std::array<W, N>& p) {
   std::array<W, N + 1> r = {0};
   std::array<W, N + 1> t = {0};

   const size_t x_bits = XN * WordInfo<W>::bits;

   for(size_t i = 0; i != x_bits; ++i) {
      const size_t b = x_bits - 1 - i;

      const size_t b_word = b / WordInfo<W>::bits;
      const size_t b_bit = b % WordInfo<W>::bits;
      const bool x_b = (x[b_word] >> b_bit) & 1;

      shift_left<1>(r);
      if(x_b) {
         r[0] += 1;
      }

      const W carry = bigint_sub3(t.data(), r.data(), N + 1, p.data(), N);

      if(carry == 0) {
         std::swap(r, t);
      }
   }

   std::array<W, N> rs;
   copy_mem(rs, std::span{r}.template first<N>());
   return rs;
}

template <WordType W, size_t N>
inline consteval std::array<W, N> montygomery_r(const std::array<W, N>& p) {
   std::array<W, N + 1> x = {0};
   x[N] = 1;
   return reduce_mod(x, p);
}

template <WordType W, size_t N>
inline consteval std::array<W, N> mul_mod(const std::array<W, N>& x,
                                          const std::array<W, N>& y,
                                          const std::array<W, N>& p) {
   std::array<W, 2 * N> z;
   comba_mul<N>(z.data(), x.data(), y.data());
   return reduce_mod(z, p);
}

template <WordType W, size_t N>
inline constexpr auto monty_redc_pdash1(const std::array<W, 2 * N>& z, const std::array<W, N>& p) -> std::array<W, N> {
   static_assert(N >= 1);

   std::array<W, N> ws;

   word3<W> accum;

   accum.add(z[0]);

   ws[0] = accum.monty_step_pdash1();

   for(size_t i = 1; i != N; ++i) {
      for(size_t j = 0; j < i; ++j) {
         accum.mul(ws[j], p[i - j]);
      }

      accum.add(z[i]);

      ws[i] = accum.monty_step_pdash1();
   }

   for(size_t i = 0; i != N - 1; ++i) {
      for(size_t j = i + 1; j != N; ++j) {
         accum.mul(ws[j], p[N + i - j]);
      }

      accum.add(z[N + i]);

      ws[i] = accum.extract();
   }

   accum.add(z[2 * N - 1]);

   ws[N - 1] = accum.extract();
   // w1 is the final part, which is not stored in the workspace
   const W w1 = accum.extract();

   std::array<W, N> r;
   bigint_monty_maybe_sub<N>(r.data(), w1, ws.data(), p.data());

   return r;
}

template <WordType W, size_t N>
inline constexpr auto monty_redc(const std::array<W, 2 * N>& z, const std::array<W, N>& p, W p_dash)
   -> std::array<W, N> {
   static_assert(N >= 1);

   std::array<W, N> ws;

   word3<W> accum;

   accum.add(z[0]);

   ws[0] = accum.monty_step(p[0], p_dash);

   for(size_t i = 1; i != N; ++i) {
      for(size_t j = 0; j < i; ++j) {
         accum.mul(ws[j], p[i - j]);
      }

      accum.add(z[i]);

      ws[i] = accum.monty_step(p[0], p_dash);
   }

   for(size_t i = 0; i != N - 1; ++i) {
      for(size_t j = i + 1; j != N; ++j) {
         accum.mul(ws[j], p[N + i - j]);
      }

      accum.add(z[N + i]);

      ws[i] = accum.extract();
   }

   accum.add(z[2 * N - 1]);

   ws[N - 1] = accum.extract();
   // w1 is the final part, which is not stored in the workspace
   const W w1 = accum.extract();

   std::array<W, N> r;
   bigint_monty_maybe_sub<N>(r.data(), w1, ws.data(), p.data());

   return r;
}

template <uint8_t X, WordType W, size_t N>
inline consteval std::array<W, N> p_minus(const std::array<W, N>& p) {
   // TODO combine into p_plus_x_over_y<-1, 1>
   static_assert(X > 0);
   std::array<W, N> r;
   W x = X;
   bigint_sub3(r.data(), p.data(), N, &x, 1);
   std::reverse(r.begin(), r.end());
   return r;
}

template <WordType W, size_t N>
inline consteval std::array<W, N> p_plus_1_over_4(const std::array<W, N>& p) {
   const W one = 1;
   std::array<W, N> r;
   bigint_add3_nc(r.data(), p.data(), N, &one, 1);
   shift_right<2>(r);
   std::reverse(r.begin(), r.end());
   return r;
}

template <WordType W, size_t N>
inline consteval std::array<W, N> p_minus_1_over_2(const std::array<W, N>& p) {
   const W one = 1;
   std::array<W, N> r;
   bigint_sub3(r.data(), p.data(), N, &one, 1);
   shift_right<1>(r);
   std::reverse(r.begin(), r.end());
   return r;
}

template <WordType W, size_t N>
inline consteval size_t count_bits(const std::array<W, N>& p) {
   auto get_bit = [&](size_t i) {
      const size_t w = i / WordInfo<W>::bits;
      const size_t b = i % WordInfo<W>::bits;
      return static_cast<uint8_t>((p[w] >> b) & 0x01);
   };

   size_t b = WordInfo<W>::bits * N;

   while(get_bit(b - 1) == 0) {
      b -= 1;
   }

   return b;
}

template <WordType W, size_t N, size_t L>
inline constexpr auto bytes_to_words(std::span<const uint8_t, L> bytes) {
   static_assert(L <= WordInfo<W>::bytes * N);

   // TODO: This could be optimized quite a bit which is relevant
   // since it executes at runtime
   std::array<W, N> r = {};
   for(size_t i = 0; i != L; ++i) {
      shift_left<8>(r);
      r[0] += bytes[i];
   }
   return r;
}

// Extract a WindowBits sized window out of s, depending on offset.
template <size_t WindowBits, typename W, size_t N>
constexpr size_t read_window_bits(std::span<const W, N> words, size_t offset) {
   static_assert(WindowBits >= 1 && WindowBits <= 7);

   const uint8_t WindowMask = static_cast<uint8_t>(1 << WindowBits) - 1;

   const size_t W_bits = sizeof(W) * 8;
   const auto bit_shift = offset % W_bits;
   const auto word_offset = words.size() - 1 - (offset / W_bits);

   const bool single_byte_window = bit_shift <= (W_bits - WindowBits) || word_offset == 0;

   const auto w0 = words[word_offset];

   if(single_byte_window) {
      return (w0 >> bit_shift) & WindowMask;
   } else {
      // Otherwise we must join two words and extract the result
      const auto w1 = words[word_offset - 1];
      const auto combined = ((w0 >> bit_shift) | (w1 << (W_bits - bit_shift)));
      return combined & WindowMask;
   }
}

}  // namespace

}  // namespace Botan

#endif
