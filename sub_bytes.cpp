#include <stdio.h>

#include <stdint.h>

const uint8_t SE[256] = {
   0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B,
   0xFE, 0xD7, 0xAB, 0x76, 0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
   0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0, 0xB7, 0xFD, 0x93, 0x26,
   0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
   0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2,
   0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
   0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84, 0x53, 0xD1, 0x00, 0xED,
   0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
   0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F,
   0x50, 0x3C, 0x9F, 0xA8, 0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
   0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2, 0xCD, 0x0C, 0x13, 0xEC,
   0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
   0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14,
   0xDE, 0x5E, 0x0B, 0xDB, 0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
   0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79, 0xE7, 0xC8, 0x37, 0x6D,
   0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
   0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F,
   0x4B, 0xBD, 0x8B, 0x8A, 0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
   0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E, 0xE1, 0xF8, 0x98, 0x11,
   0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
   0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F,
   0xB0, 0x54, 0xBB, 0x16 };

template<typename T> inline constexpr uint8_t get_byte(size_t byte_num, T input)
   {
   return static_cast<uint8_t>(
      input >> (((~byte_num)&(sizeof(T)-1)) << 3)
      );
   }

inline constexpr uint32_t make_uint32(uint8_t i0, uint8_t i1, uint8_t i2, uint8_t i3)
   {
   return ((static_cast<uint32_t>(i0) << 24) |
           (static_cast<uint32_t>(i1) << 16) |
           (static_cast<uint32_t>(i2) <<  8) |
           (static_cast<uint32_t>(i3)));
   }

inline uint32_t SE_word(uint32_t x)
   {
   return make_uint32(SE[get_byte(0, x)],
                      SE[get_byte(1, x)],
                      SE[get_byte(2, x)],
                      SE[get_byte(3, x)]);
   }


template<typename T>
T aes_S(T x)
   {
   /*
   const T x0 = (x >> 7) & 1;
   const T x1 = (x >> 6) & 1;
   const T x2 = (x >> 5) & 1;
   const T x3 = (x >> 4) & 1;
   const T x4 = (x >> 3) & 1;
   const T x5 = (x >> 2) & 1;
   const T x6 = (x >> 1) & 1;
   const T x7 = (x >> 0) & 1;
   */
   static_assert(sizeof(T) <= 8, "Expected input size");

   typedef uint32_t word;

   word x0 = 0;
   word x1 = 0;
   word x2 = 0;
   word x3 = 0;
   word x4 = 0;
   word x5 = 0;
   word x6 = 0;
   word x7 = 0;

   for(size_t i = 0; i != 8*sizeof(T); i += 8)
      {
      x0 = (x0 << 1) | ((x >> (7+i)) & 1);
      x1 = (x1 << 1) | ((x >> (6+i)) & 1);
      x2 = (x2 << 1) | ((x >> (5+i)) & 1);
      x3 = (x3 << 1) | ((x >> (4+i)) & 1);
      x4 = (x4 << 1) | ((x >> (3+i)) & 1);
      x5 = (x5 << 1) | ((x >> (2+i)) & 1);
      x6 = (x6 << 1) | ((x >> (1+i)) & 1);
      x7 = (x7 << 1) | ((x >> (0+i)) & 1);
      }

   /*
   Circuit for AES S-box from https://eprint.iacr.org/2009/191.pdf
   */

   // Figure 2, the top linear transformation
   const word y14 = x3 ^ x5;
   const word y13 = x0 ^ x6;
   const word y9 = x0 ^ x3;
   const word y8 = x0 ^ x5;
   const word t0 = x1 ^ x2;
   const word y1 = t0 ^ x7;
   const word y4 = y1 ^ x3;
   const word y12 = y13 ^ y14;
   const word y2 = y1 ^ x0;
   const word y5 = y1 ^ x6;
   const word y3 = y5 ^ y8;
   const word t1 = x4 ^ y12;
   const word y15 = t1 ^ x5;
   const word y20 = t1 ^ x1;
   const word y6 = y15 ^ x7;
   const word y10 = y15 ^ t0;
   const word y11 = y20 ^ y9;
   const word y7 = x7 ^ y11;
   const word y17 = y10 ^ y11;
   const word y19 = y10 ^ y8;
   const word y16 = t0 ^ y11;
   const word y21 = y13 ^ y16;
   const word y18 = x0 ^ y16;

   // Figure 3, the middle non-linear section
   const word t2 = y12 & y15;
   const word t3 = y3 & y6;
   const word t4 = t3 ^ t2;
   const word t5 = y4 & x7;
   const word t6 = t5 ^ t2;
   const word t7 = y13 & y16;
   const word t8 = y5 & y1;
   const word t9 = t8 ^ t7;
   const word t10 = y2 & y7;
   const word t11 = t10 ^ t7;
   const word t12 = y9 & y11;
   const word t13 = y14 & y17;
   const word t14 = t13 ^ t12;
   const word t15 = y8 & y10;
   const word t16 = t15 ^ t12;
   const word t17 = t4 ^ t14;
   const word t18 = t6 ^ t16;
   const word t19 = t9 ^ t14;
   const word t20 = t11 ^ t16;
   const word t21 = t17 ^ y20;
   const word t22 = t18 ^ y19;
   const word t23 = t19 ^ y21;
   const word t24 = t20 ^ y18;
   const word t25 = t21 ^ t22;
   const word t26 = t21 & t23;
   const word t27 = t24 ^ t26;
   const word t28 = t25 & t27;
   const word t29 = t28 ^ t22;
   const word t30 = t23 ^ t24;
   const word t31 = t22 ^ t26;
   const word t32 = t31 & t30;
   const word t33 = t32 ^ t24;
   const word t34 = t23 ^ t33;
   const word t35 = t27 ^ t33;
   const word t36 = t24 & t35;
   const word t37 = t36 ^ t34;
   const word t38 = t27 ^ t36;
   const word t39 = t29 & t38;
   const word t40 = t25 ^ t39;
   const word t41 = t40 ^ t37;
   const word t42 = t29 ^ t33;
   const word t43 = t29 ^ t40;
   const word t44 = t33 ^ t37;
   const word t45 = t42 ^ t41;
   const word z0 = t44 & y15;
   const word z1 = t37 & y6;
   const word z2 = t33 & x7;
   const word z3 = t43 & y16;
   const word z4 = t40 & y1;
   const word z5 = t29 & y7;
   const word z6 = t42 & y11;
   const word z7 = t45 & y17;
   const word z8 = t41 & y10;
   const word z9 = t44 & y12;
   const word z10 = t37 & y3;
   const word z11 = t33 & y4;
   const word z12 = t43 & y13;
   const word z13 = t40 & y5;
   const word z14 = t29 & y2;
   const word z15 = t42 & y9;
   const word z16 = t45 & y14;
   const word z17 = t41 & y8;

   // Figure 4, bottom linear transformation
   const word t46 = z15 ^ z16;
   const word t47 = z10 ^ z11;
   const word t48 = z5 ^ z13;
   const word t49 = z9 ^ z10;
   const word t50 = z2 ^ z12;
   const word t51 = z2 ^ z5;
   const word t52 = z7 ^ z8;
   const word t53 = z0 ^ z3;
   const word t54 = z6 ^ z7;
   const word t55 = z16 ^ z17;
   const word t56 = z12 ^ t48;
   const word t57 = t50 ^ t53;
   const word t58 = z4 ^ t46;
   const word t59 = z3 ^ t54;
   const word t60 = t46 ^ t57;
   const word t61 = z14 ^ t57;
   const word t62 = t52 ^ t58;
   const word t63 = t49 ^ t58;
   const word t64 = z4 ^ t59;
   const word t65 = t61 ^ t62;
   const word t66 = z1 ^ t63;
   const word t67 = t64 ^ t65;
   const word s7 = t48 ^ ~t60;
   const word s6 = t56 ^ ~t62;
   const word s5 = t47 ^ t65;
   const word s4 = t51 ^ t66;
   const word s3 = t53 ^ t66;
   const word s2 = t55 ^ ~t67;
   const word s1 = t64 ^ ~s3;
   const word s0 = t59 ^ t63;

   T r = 0;

   for(size_t i = 0; i != sizeof(T); i += 1)
      {
      r = (r << 1) | ((s0 >> i) & 1);
      r = (r << 1) | ((s1 >> i) & 1);
      r = (r << 1) | ((s2 >> i) & 1);
      r = (r << 1) | ((s3 >> i) & 1);
      r = (r << 1) | ((s4 >> i) & 1);
      r = (r << 1) | ((s5 >> i) & 1);
      r = (r << 1) | ((s6 >> i) & 1);
      r = (r << 1) | ((s7 >> i) & 1);
      }

   return r;
   }

template<typename T>
T aes_S2(T x)
   {
   static_assert(sizeof(T) <= 8, "Expected input size");

   typedef uint32_t word;

   word x0 = 0;
   word x1 = 0;
   word x2 = 0;
   word x3 = 0;
   word x4 = 0;
   word x5 = 0;
   word x6 = 0;
   word x7 = 0;

   for(size_t i = 0; i != 8*sizeof(T); i += 8)
      {
      x0 = (x0 << 1) | ((x >> (7+i)) & 1);
      x1 = (x1 << 1) | ((x >> (6+i)) & 1);
      x2 = (x2 << 1) | ((x >> (5+i)) & 1);
      x3 = (x3 << 1) | ((x >> (4+i)) & 1);
      x4 = (x4 << 1) | ((x >> (3+i)) & 1);
      x5 = (x5 << 1) | ((x >> (2+i)) & 1);
      x6 = (x6 << 1) | ((x >> (1+i)) & 1);
      x7 = (x7 << 1) | ((x >> (0+i)) & 1);
      }

   /*
   Circuit for AES S-box from https://eprint.iacr.org/2011/332.pdf
   */
#if 0
   // Figure 5:  Top linear transform in forward direction.
   T1 = U0 + U3
   T2 = U0 + U5
   T3 = U0 + U6
   T4 = U3 + U5
   T5 = U4 + U6
   T6 = T1 + T5
   T7 = U1 + U2

      T8 = U7 + T6
      T9 = U7 + T7
      T10 = T6 + T7
      T11 = U1 + U5
      T12 = U2 + U5
      T13 = T3 + T4
      T14 = T6 + T11

      T15 = T5 + T11
      T16 = T5 + T12
      T17 = T9 + T16
      T18 = U3 + U7
      T19 = T7 + T18
      T20 = T1 + T19
      T21 = U6 + U7

      T22 = T7 + T21
      T23 = T2 + T22
      T24 = T2 + T10
      T25 = T20 + T17
      T26 = T3 + T16
      T27 = T1 + T12

      M1 = T13 x T6
      M2 = T23 x T8
      M3 = T14 + M1
      M4 = T19 x D
      M5 = M4 + M1
      M6 = T3 x T16
      M7 = T22 x T9
      M8 = T26 + M6
      M9 = T20 x T17
      M10 = M9 + M6
      M11 = T1 x T15
      M12 = T4 x T27
      M13 = M12 + M11
      M14 = T2 x T10
      M15 = M14 + M11
      M16 = M3 + M2

      M17 = M5 + T24
      M18 = M8 + M7
      M19 = M10 + M15
      M20 = M16 + M13
      M21 = M17 + M15
      M22 = M18 + M13
      M23 = M19 + T25
      M24 = M22 + M23
      M25 = M22 x M20
      M26 = M21 + M25
      M27 = M20 + M21
      M28 = M23 + M25
      M29 = M28 x M27
      M30 = M26 x M24
      M31 = M20 x M23
      M32 = M27 x M31

      M33 = M27 + M25
      M34 = M21 x M22
      M35 = M24 x M34
      M36 = M24 + M25
      M37 = M21 + M29
      M38 = M32 + M33
      M39 = M23 + M30
      M40 = M35 + M36
      M41 = M38 + M40
      M42 = M37 + M39
      M43 = M37 + M38
      M44 = M39 + M40
      M45 = M42 + M41
      M46 = M44 x T6
      M47 = M40 x T8
      M48 = M39 x D

   T r = 0;

   for(size_t i = 0; i != sizeof(T); i += 1)
      {
      r = (r << 1) | ((s0 >> i) & 1);
      r = (r << 1) | ((s1 >> i) & 1);
      r = (r << 1) | ((s2 >> i) & 1);
      r = (r << 1) | ((s3 >> i) & 1);
      r = (r << 1) | ((s4 >> i) & 1);
      r = (r << 1) | ((s5 >> i) & 1);
      r = (r << 1) | ((s6 >> i) & 1);
      r = (r << 1) | ((s7 >> i) & 1);
      }

   return r;
   }

int main()
   {
#if 0
   uint16_t z = 0;

   while(z <= 255)
      {
      if(aes_S<uint8_t>(z) != SE[z])
         {
         printf("bad %d\n", z);
         return 1;
         }
      ++z;
      }
#endif

   uint32_t f32 = 0xFF010203;

   for(size_t i = 0; i <= 0xFFFFFF; ++i)
      {
      uint32_t ref = SE_word(i);
      uint32_t ct = aes_S<uint32_t>(i);
      if(ref != ct)
         {
         printf("%d %08X %08X\n", i, ref, ct);
         return 1;
         }
      }
   printf("ok\n");
   }
