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

   for(size_t i = 0; i <= 0xFFFFFFFF; ++i)
      {
      uint32_t ref = SE_word(i);
      uint32_t ct = aes_S2<uint32_t>(i);
      if(ref != ct)
         {
         printf("%d %08X %08X\n", i, ref, ct);
         return 1;
         }
      }
   printf("ok\n");
   }
