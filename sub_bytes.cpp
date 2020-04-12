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

const uint8_t SD[256] = {
   0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E,
   0x81, 0xF3, 0xD7, 0xFB, 0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87,
   0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB, 0x54, 0x7B, 0x94, 0x32,
   0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
   0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49,
   0x6D, 0x8B, 0xD1, 0x25, 0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16,
   0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92, 0x6C, 0x70, 0x48, 0x50,
   0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
   0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05,
   0xB8, 0xB3, 0x45, 0x06, 0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02,
   0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B, 0x3A, 0x91, 0x11, 0x41,
   0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
   0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8,
   0x1C, 0x75, 0xDF, 0x6E, 0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89,
   0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B, 0xFC, 0x56, 0x3E, 0x4B,
   0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
   0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59,
   0x27, 0x80, 0xEC, 0x5F, 0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D,
   0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF, 0xA0, 0xE0, 0x3B, 0x4D,
   0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
   0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63,
   0x55, 0x21, 0x0C, 0x7D };


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

      //Figure 9:  Bottom linear transform in reverse direction
      /* wups
      P0 = M52 + M61
      P1 = M58 + M59
      P2 = M54 + M62
      P3 = M47 + M50
      P4 = M48 + M56
      P5 = M46 + M51
      P6 = M49 + M60
      P7 = P0 + P1
      P8 = M50 + M53
      P9 = M55 + M63

      P10 = M57 + P4
      P11 = P0 + P3
      P12 = M46 + M48
      P13 = M49 + M51
      P14 = M49 + M62
      P15 = M54 + M59
      P16 = M57 + M61
      P17 = M58 + P2
      P18 = M63 + P5
      P19 = P2 + P3

      P20 = P4 + P6
      P22 = P2 + P7
      P23 = P7 + P8
      P24 = P5 + P7
      P25 = P6 + P10
      P26 = P9 + P11
      P27 = P10 + P18
      P28 = P11 + P25
      P29 = P15 + P20
      W0 = P13 + P22

      W1 = P26 + P29
      W2 = P17 + P28
      W3 = P12 + P22
      W4 = P23 + P27
      W5 = P19 + P24
      W6 = P14 + P23
      W7 = P9 + P16
      */

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
   // Figure 5:  Top linear transform in forward direction.
   const word T1 = x0 ^ x3;
   const word T2 = x0 ^ x5;
   const word T3 = x0 ^ x6;
   const word T4 = x3 ^ x5;
   const word T5 = x4 ^ x6;
   const word T6 = T1 ^ T5;
   const word T7 = x1 ^ x2;

   const word T8 = x7 ^ T6;
   const word T9 = x7 ^ T7;
   const word T10 = T6 ^ T7;
   const word T11 = x1 ^ x5;
   const word T12 = x2 ^ x5;
   const word T13 = T3 ^ T4;
   const word T14 = T6 ^ T11;

   const word T15 = T5 ^ T11;
   const word T16 = T5 ^ T12;
   const word T17 = T9 ^ T16;
   const word T18 = x3 ^ x7;
   const word T19 = T7 ^ T18;
   const word T20 = T1 ^ T19;
   const word T21 = x6 ^ x7;

   const word T22 = T7 ^ T21;
   const word T23 = T2 ^ T22;
   const word T24 = T2 ^ T10;
   const word T25 = T20 ^ T17;
   const word T26 = T3 ^ T16;
   const word T27 = T1 ^ T12;

   const word D = x7;

   // Figure 7:  Shared part of AES S-box circuit
   const word M1 = T13 & T6;
   const word M2 = T23 & T8;
   const word M3 = T14 ^ M1;
   const word M4 = T19 & D;
   const word M5 = M4 ^ M1;
   const word M6 = T3 & T16;
   const word M7 = T22 & T9;
   const word M8 = T26 ^ M6;
   const word M9 = T20 & T17;
   const word M10 = M9 ^ M6;
   const word M11 = T1 & T15;
   const word M12 = T4 & T27;
   const word M13 = M12 ^ M11;
   const word M14 = T2 & T10;
   const word M15 = M14 ^ M11;
   const word M16 = M3 ^ M2;

   const word M17 = M5 ^ T24;
   const word M18 = M8 ^ M7;
   const word M19 = M10 ^ M15;
   const word M20 = M16 ^ M13;
   const word M21 = M17 ^ M15;
   const word M22 = M18 ^ M13;
   const word M23 = M19 ^ T25;
   const word M24 = M22 ^ M23;
   const word M25 = M22 & M20;
   const word M26 = M21 ^ M25;
   const word M27 = M20 ^ M21;
   const word M28 = M23 ^ M25;
   const word M29 = M28 & M27;
   const word M30 = M26 & M24;
   const word M31 = M20 & M23;
   const word M32 = M27 & M31;

   const word M33 = M27 ^ M25;
   const word M34 = M21 & M22;
   const word M35 = M24 & M34;
   const word M36 = M24 ^ M25;
   const word M37 = M21 ^ M29;
   const word M38 = M32 ^ M33;
   const word M39 = M23 ^ M30;
   const word M40 = M35 ^ M36;
   const word M41 = M38 ^ M40;
   const word M42 = M37 ^ M39;
   const word M43 = M37 ^ M38;
   const word M44 = M39 ^ M40;
   const word M45 = M42 ^ M41;
   const word M46 = M44 & T6;
   const word M47 = M40 & T8;
   const word M48 = M39 & D;

   const word M49 = M43 & T16;
   const word M50 = M38 & T9;
   const word M51 = M37 & T17;
   const word M52 = M42 & T15;
   const word M53 = M45 & T27;
   const word M54 = M41 & T10;
   const word M55 = M44 & T13;
   const word M56 = M40 & T23;
   const word M57 = M39 & T19;
   const word M58 = M43 & T3;
   const word M59 = M38 & T22;
   const word M60 = M37 & T20;
   const word M61 = M42 & T1;
   const word M62 = M45 & T4;
   const word M63 = M41 & T2;

      // Figure 8:  Bottom linear transform in forward direction.
   const word L0 = M61 ^ M62;
   const word L1 = M50 ^ M56;
   const word L2 = M46 ^ M48;
   const word L3 = M47 ^ M55;
   const word L4 = M54 ^ M58;
   const word L5 = M49 ^ M61;
   const word L6 = M62 ^ L5;
   const word L7 = M46 ^ L3;
   const word L8 = M51 ^ M59;
   const word L9 = M52 ^ M53;
   const word L10 = M53 ^ L4;
   const word L11 = M60 ^ L2;
   const word L12 = M48 ^ M51;
   const word L13 = M50 ^ L0;
   const word L14 = M52 ^ M61;
   const word L15 = M55 ^ L1;
   const word L16 = M56 ^ L0;
   const word L17 = M57 ^ L1;
   const word L18 = M58 ^ L8;
   const word L19 = M63 ^ L4;

   const word L20 = L0 ^ L1;
   const word L21 = L1 ^ L7;
   const word L22 = L3 ^ L12;
   const word L23 = L18 ^ L2;
   const word L24 = L15 ^ L9;
   const word L25 = L6 ^ L10;
   const word L26 = L7 ^ L9;
   const word L27 = L8 ^ L10;
   const word L28 = L11 ^ L14;
   const word L29 = L11 ^ L17;

   const word S0 = L6 ^ L24;
   const word S1 = ~(L16 ^ L26);
   const word S2 = ~(L19 ^ L28);
   const word S3 = L6 ^ L21;
   const word S4 = L20 ^ L22;
   const word S5 = L25 ^ L29;
   const word S6 = ~(L13 ^ L27);
   const word S7 = ~(L6 ^ L23);

   T r = 0;

   for(size_t i = 0; i != sizeof(T); i += 1)
      {
      r = (r << 1) | ((S0 >> i) & 1);
      r = (r << 1) | ((S1 >> i) & 1);
      r = (r << 1) | ((S2 >> i) & 1);
      r = (r << 1) | ((S3 >> i) & 1);
      r = (r << 1) | ((S4 >> i) & 1);
      r = (r << 1) | ((S5 >> i) & 1);
      r = (r << 1) | ((S6 >> i) & 1);
      r = (r << 1) | ((S7 >> i) & 1);
      }

   return r;
   }

static const uint32_t Te0[256] = {
    0xc66363a5, 0xf87c7c84, 0xee777799, 0xf67b7b8d,
    0xfff2f20d, 0xd66b6bbd, 0xde6f6fb1, 0x91c5c554,
    0x60303050, 0x02010103, 0xce6767a9, 0x562b2b7d,
    0xe7fefe19, 0xb5d7d762, 0x4dababe6, 0xec76769a,
    0x8fcaca45, 0x1f82829d, 0x89c9c940, 0xfa7d7d87,
    0xeffafa15, 0xb25959eb, 0x8e4747c9, 0xfbf0f00b,
    0x41adadec, 0xb3d4d467, 0x5fa2a2fd, 0x45afafea,
    0x239c9cbf, 0x53a4a4f7, 0xe4727296, 0x9bc0c05b,
    0x75b7b7c2, 0xe1fdfd1c, 0x3d9393ae, 0x4c26266a,
    0x6c36365a, 0x7e3f3f41, 0xf5f7f702, 0x83cccc4f,
    0x6834345c, 0x51a5a5f4, 0xd1e5e534, 0xf9f1f108,
    0xe2717193, 0xabd8d873, 0x62313153, 0x2a15153f,
    0x0804040c, 0x95c7c752, 0x46232365, 0x9dc3c35e,
    0x30181828, 0x379696a1, 0x0a05050f, 0x2f9a9ab5,
    0x0e070709, 0x24121236, 0x1b80809b, 0xdfe2e23d,
    0xcdebeb26, 0x4e272769, 0x7fb2b2cd, 0xea75759f,
    0x1209091b, 0x1d83839e, 0x582c2c74, 0x341a1a2e,
    0x361b1b2d, 0xdc6e6eb2, 0xb45a5aee, 0x5ba0a0fb,
    0xa45252f6, 0x763b3b4d, 0xb7d6d661, 0x7db3b3ce,
    0x5229297b, 0xdde3e33e, 0x5e2f2f71, 0x13848497,
    0xa65353f5, 0xb9d1d168, 0x00000000, 0xc1eded2c,
    0x40202060, 0xe3fcfc1f, 0x79b1b1c8, 0xb65b5bed,
    0xd46a6abe, 0x8dcbcb46, 0x67bebed9, 0x7239394b,
    0x944a4ade, 0x984c4cd4, 0xb05858e8, 0x85cfcf4a,
    0xbbd0d06b, 0xc5efef2a, 0x4faaaae5, 0xedfbfb16,
    0x864343c5, 0x9a4d4dd7, 0x66333355, 0x11858594,
    0x8a4545cf, 0xe9f9f910, 0x04020206, 0xfe7f7f81,
    0xa05050f0, 0x783c3c44, 0x259f9fba, 0x4ba8a8e3,
    0xa25151f3, 0x5da3a3fe, 0x804040c0, 0x058f8f8a,
    0x3f9292ad, 0x219d9dbc, 0x70383848, 0xf1f5f504,
    0x63bcbcdf, 0x77b6b6c1, 0xafdada75, 0x42212163,
    0x20101030, 0xe5ffff1a, 0xfdf3f30e, 0xbfd2d26d,
    0x81cdcd4c, 0x180c0c14, 0x26131335, 0xc3ecec2f,
    0xbe5f5fe1, 0x359797a2, 0x884444cc, 0x2e171739,
    0x93c4c457, 0x55a7a7f2, 0xfc7e7e82, 0x7a3d3d47,
    0xc86464ac, 0xba5d5de7, 0x3219192b, 0xe6737395,
    0xc06060a0, 0x19818198, 0x9e4f4fd1, 0xa3dcdc7f,
    0x44222266, 0x542a2a7e, 0x3b9090ab, 0x0b888883,
    0x8c4646ca, 0xc7eeee29, 0x6bb8b8d3, 0x2814143c,
    0xa7dede79, 0xbc5e5ee2, 0x160b0b1d, 0xaddbdb76,
    0xdbe0e03b, 0x64323256, 0x743a3a4e, 0x140a0a1e,
    0x924949db, 0x0c06060a, 0x4824246c, 0xb85c5ce4,
    0x9fc2c25d, 0xbdd3d36e, 0x43acacef, 0xc46262a6,
    0x399191a8, 0x319595a4, 0xd3e4e437, 0xf279798b,
    0xd5e7e732, 0x8bc8c843, 0x6e373759, 0xda6d6db7,
    0x018d8d8c, 0xb1d5d564, 0x9c4e4ed2, 0x49a9a9e0,
    0xd86c6cb4, 0xac5656fa, 0xf3f4f407, 0xcfeaea25,
    0xca6565af, 0xf47a7a8e, 0x47aeaee9, 0x10080818,
    0x6fbabad5, 0xf0787888, 0x4a25256f, 0x5c2e2e72,
    0x381c1c24, 0x57a6a6f1, 0x73b4b4c7, 0x97c6c651,
    0xcbe8e823, 0xa1dddd7c, 0xe874749c, 0x3e1f1f21,
    0x964b4bdd, 0x61bdbddc, 0x0d8b8b86, 0x0f8a8a85,
    0xe0707090, 0x7c3e3e42, 0x71b5b5c4, 0xcc6666aa,
    0x904848d8, 0x06030305, 0xf7f6f601, 0x1c0e0e12,
    0xc26161a3, 0x6a35355f, 0xae5757f9, 0x69b9b9d0,
    0x17868691, 0x99c1c158, 0x3a1d1d27, 0x279e9eb9,
    0xd9e1e138, 0xebf8f813, 0x2b9898b3, 0x22111133,
    0xd26969bb, 0xa9d9d970, 0x078e8e89, 0x339494a7,
    0x2d9b9bb6, 0x3c1e1e22, 0x15878792, 0xc9e9e920,
    0x87cece49, 0xaa5555ff, 0x50282878, 0xa5dfdf7a,
    0x038c8c8f, 0x59a1a1f8, 0x09898980, 0x1a0d0d17,
    0x65bfbfda, 0xd7e6e631, 0x844242c6, 0xd06868b8,
    0x824141c3, 0x299999b0, 0x5a2d2d77, 0x1e0f0f11,
    0x7bb0b0cb, 0xa85454fc, 0x6dbbbbd6, 0x2c16163a,
};

template<size_t ROT, typename T>
inline constexpr T rotr(T input)
   {
   static_assert(ROT > 0 && ROT < 8*sizeof(T), "Invalid rotation constant");
   return static_cast<T>((input >> ROT) | (input << (8*sizeof(T) - ROT)));
   }

inline constexpr uint8_t xtime(uint8_t s) { return static_cast<uint8_t>(s << 1) ^ ((s >> 7) * 0x1B); }
inline constexpr uint8_t xtime4(uint8_t s) { return xtime(xtime(s)); }
inline constexpr uint8_t xtime8(uint8_t s) { return xtime(xtime(xtime(s))); }

inline constexpr uint8_t xtime3(uint8_t s) { return xtime(s) ^ s; }
inline constexpr uint8_t xtime9(uint8_t s) { return xtime8(s) ^ s; }
inline constexpr uint8_t xtime11(uint8_t s) { return xtime8(s) ^ xtime(s) ^ s; }
inline constexpr uint8_t xtime13(uint8_t s) { return xtime8(s) ^ xtime4(s) ^ s; }
inline constexpr uint8_t xtime14(uint8_t s) { return xtime8(s) ^ xtime4(s) ^ xtime(s); }

inline uint32_t xtime_32(uint32_t s)
   {
   const uint32_t ref =  make_uint32(xtime(get_byte(0, s)),
                      xtime(get_byte(1, s)),
                      xtime(get_byte(2, s)),
                      xtime(get_byte(3, s)));
   const uint32_t hb = ((s >> 7) & 0x01010101);

   const uint32_t m = (hb << 4) | (hb << 3) | (hb << 1) | hb; // 0x1B

   const uint32_t s0 = (s << 1) & 0xFEFEFEFE;

   //printf("%08X %08X %08X\n", (s >> 7) & 0x01010101, s0, m);
   uint32_t z = s0 ^ m;

   if(ref != z)
      printf("Bad: in=%08X ref=%08X z=%08X\n", s, ref, z);

   return ref;
   /*
   return make_uint32(xtime(get_byte(0, s)),
                      xtime(get_byte(1, s)),
                      xtime(get_byte(2, s)),
                      xtime(get_byte(3, s)));
   */
   }

#define AES_T(T, V0, V1, V2, V3)                                     \
   (T[get_byte(0, V0)] ^                                            \
    rotr< 8>(T[get_byte(1, V1)]) ^                                      \
    rotr<16>(T[get_byte(2, V2)]) ^                                      \
    rotr<24>(T[get_byte(3, V3)]))

#if 0
void mix_columns(uint32_t& V0, uint32_t& V1, uint32_t& V2, uint32_t& V3)
   {
   const uint32_t Z0 = aes_S(V0);
   const uint32_t Z1 = rotr<8>(aes_S(V1));
   const uint32_t Z2 = rotr<16>(aes_S(V2));
   const uint32_t Z3 = rotr<24>(aes_S(V3));

   const uint32_t xtime_Z0 = xtime_32(Z0);
   const uint32_t xtime_Z1 = xtime_32(Z1);
   const uint32_t xtime_Z2 = xtime_32(Z2);
   const uint32_t xtime_Z3 = xtime_32(Z3);
   const uint32_t xtime3_Z0 = xtime_Z0 ^ Z0;
   const uint32_t xtime3_Z1 = xtime_Z0 ^ Z1;
   const uint32_t xtime3_Z2 = xtime_Z0 ^ Z2;
   const uint32_t xtime3_Z3 = xtime_Z0 ^ Z3;

   const uint32_t T00 = make_uint32(get_byte(0, xtime_Z0), get_byte(0, Z0), get_byte(0, Z0), get_byte(0, xtime3_Z0));
   const uint32_t T01 = make_uint32(get_byte(1, xtime_Z0), get_byte(1, Z0), get_byte(1, Z0), get_byte(1, xtime3_Z0));
   const uint32_t T02 = make_uint32(get_byte(2, xtime_Z0), get_byte(2, Z0), get_byte(2, Z0), get_byte(2, xtime3_Z0));
   const uint32_t T03 = make_uint32(get_byte(3, xtime_Z0), get_byte(3, Z0), get_byte(3, Z0), get_byte(3, xtime3_Z0));

   V0 = T00 ^ rotr<8>(T01) ^ rotr<16>(T02) ^ rotr<24>(T03);
   }
#endif

uint32_t t_tables(uint32_t V0, uint32_t V1, uint32_t V2, uint32_t V3)
   {
   uint8_t b0 = get_byte(0, V0);
   uint8_t b1 = get_byte(1, V1);
   uint8_t b2 = get_byte(2, V2);
   uint8_t b3 = get_byte(3, V3);

   const uint32_t s = aes_S<uint32_t>(make_uint32(b0, b1, b2, b3));

   const uint32_t xtime_s = xtime_32(s);
   const uint32_t xtime3_s = xtime_s ^ s;

   const uint32_t T0 = make_uint32(get_byte(0, xtime_s),  get_byte(0, s),        get_byte(0, s),        get_byte(0, xtime3_s));
   const uint32_t T1 = make_uint32(get_byte(1, xtime3_s), get_byte(1, xtime_s),  get_byte(1, s),        get_byte(1, s));
   const uint32_t T2 = make_uint32(get_byte(2, s),        get_byte(2, xtime3_s), get_byte(2, xtime_s),  get_byte(2, s));
   const uint32_t T3 = make_uint32(get_byte(3, s),        get_byte(3, s),        get_byte(3, xtime3_s), get_byte(3, xtime_s));

   return (T0 ^ T1 ^ T2 ^ T3);
   }

int main()
   {

   #if 1
   uint32_t V0 = 0x66666666;
   uint32_t V1 = 0xFFFEFDFC;
   uint32_t V2 = 0x66676869;
   uint32_t V3 = 0x80FE0199;
   #else
   uint32_t V0 = 0x00000000;
   uint32_t V1 = 0x00000000;
   uint32_t V2 = 0x00000000;
   uint32_t V3 = 0x00000000;

   #endif
   uint32_t R0 = AES_T(Te0, V0, V1, V2, V3);
   uint32_t R1 = AES_T(Te0, V1, V2, V3, V0);
   uint32_t R2 = AES_T(Te0, V2, V3, V0, V1);
   uint32_t R3 = AES_T(Te0, V3, V0, V1, V2);

   printf("REF: %08X %08X %08X %08X\n", R0, R1, R2, R3);

   uint32_t Z0 = t_tables(V0, V1, V2, V3);
   uint32_t Z1 = t_tables(V1, V2, V3, V0);
   uint32_t Z2 = t_tables(V2, V3, V0, V1);
   uint32_t Z3 = t_tables(V3, V0, V1, V2);
   printf("CT:  %08X %08X %08X %08X\n", Z0, Z1, Z2, Z3);


   #if 0
   uint32_t TE[256] = { 0 };
   for(size_t i = 0; i != 256; i += 4)
      {
      const uint32_t s = aes_S<uint32_t>(make_uint32(i, i+1, i+2, i+3));

      const uint32_t xtime_s = xtime_32(s);
      const uint32_t xtime3_s = xtime_s ^ s;
      const uint8_t s0 = get_byte(0, s);
      const uint8_t s1 = get_byte(1, s);
      const uint8_t s2 = get_byte(2, s);
      const uint8_t s3 = get_byte(3, s);
      TE[i+0] = make_uint32(get_byte(0, xtime_s), s0, s0, get_byte(0, xtime3_s));
      TE[i+1] = make_uint32(get_byte(1, xtime_s), s1, s1, get_byte(1, xtime3_s));
      TE[i+2] = make_uint32(get_byte(2, xtime_s), s2, s2, get_byte(2, xtime3_s));
      TE[i+3] = make_uint32(get_byte(3, xtime_s), s3, s3, get_byte(3, xtime3_s));
      }

   for(size_t i = 0; i != 256; ++i)
      {
      if(TE[i] != Te0[i])
         {
         printf("Bad table @ %d\n", i);
         return 1;
         }
      }
   printf("table ok\n");
   #endif

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

   #if 0
   printf("%016llX\n", aes_S<uint64_t>(0x0001020304050607));
   return 0;
   for(size_t i = 0; i <= 0xFFFFFFFF; ++i)
      {
      uint32_t ref = SE_word(i);
      uint64_t ct = aes_S<uint64_t>(i | ((uint64_t)(i) << 32));
      if(ref != ct >> 32 || ref != ct & 0xFFFFFFFF)
         {
         printf("%d %08X %08X\n", i, ref, ct);
         return 1;
         }
      }
   printf("ok\n");
   #endif
   }
