#include <stdint.h>
#include <stdio.h>

static const uint64_t B[] = {0x5555555555555555,
                             0x3333333333333333,
                             0x0F0F0F0F0F0F0F0F,
                             0x00FF00FF00FF00FF,
                             0x0000FFFF0000FFFF,
};
static const uint32_t S[] = {1, 2, 4, 8, 16};

uint32_t compact(uint64_t x)
   {
   x &= 0x5555555555555555;
   x = (x ^ (x >>  1)) & 0x3333333333333333;
   x = (x ^ (x >>  2)) & 0x0f0f0f0f0f0f0f0f;
   x = (x ^ (x >>  4)) & 0x00ff00ff00ff00ff;
   x = (x ^ (x >>  8)) & 0x0000ffff0000ffff;
   x = (x ^ (x >>  16)) & 0xFFFFffff;
return x;
   }

uint64_t encode(uint32_t x32)
   {
   uint64_t x = x32;
   x = (x ^ (x << 16)) & 0x0000FFFF0000FFFF;
   x = (x ^ (x <<  8)) & 0x00ff00ff00ff00ff;
   x = (x ^ (x <<  4)) & 0x0f0f0f0f0f0f0f0f;
   x = (x ^ (x <<  2)) & 0x3333333333333333;
   x = (x ^ (x <<  1)) & 0x5555555555555555;
   return x;
   }

int main()
   {
uint64_t x = 0xABCDEF00; // Interleave lower 16 bits of x and y, so the bits of x
uint64_t y = 0x69696939; // are in the even positions and bits from y in the odd;
uint64_t z = 0; // z gets the resulting 32-bit Morton Number.
                // x and y must initially be less than 65536.
printf("%016lX\n", compact(encode(x)));
printf("%016lX\n", compact(encode(y)));

x = (x | (x << S[4])) & B[4];
x = (x | (x << S[3])) & B[3];
x = (x | (x << S[2])) & B[2];
x = (x | (x << S[1])) & B[1];
x = (x | (x << S[0])) & B[0];

y = (y | (y << S[4])) & B[4];
y = (y | (y << S[3])) & B[3];
y = (y | (y << S[2])) & B[2];
y = (y | (y << S[1])) & B[1];
y = (y | (y << S[0])) & B[0];


printf("%016lX\n", x);
printf("%016lX\n", y);

z = x | (y << 1);

printf("%016lX\n", z);

printf("%08X\n", compact(z));
printf("%08X\n", compact(z >> 1));
   }
