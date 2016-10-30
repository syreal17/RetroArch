/* Copyright  (C) 2010-2016 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (vector_2.c).
 * ---------------------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdint.h>
#include <math.h>

#include <gfx/math/vector_2.h>

union uif32
{
   float f;
   uint32_t i;
};

static float overflow(void)
{
   unsigned i;
   volatile float f = 1e10;

   for(i = 0; i < 10; ++i)	
      f *= f;             /* this will overflow before
                             the for­loop terminates */

   return f;
}

int16_t toFloat16(const float f)
{
   int i, s, e, m;
   union uif32 Entry;

   Entry.f = f;
   i       = (int)Entry.i;

   /*
    * Our floating point number, f, is represented by the bit
    * pattern in integer i.  Disassemble that bit pattern into
    * the sign, s, the exponent, e, and the significand, m.
    * Shift s into the position where it will go in in the
    * resulting half number.
    * Adjust e, accounting for the different exponent bias
    * of float and half (127 versus 15).
    */

   s =  (i >> 16) & 0x00008000;
   e = ((i >> 23) & 0x000000ff) - (127 - 15);
   m =   i        & 0x007fffff;

   /* Now reassemble s, e and m into a half: */

   if(e <= 0)
   {
      /*
       * E is less than -10.  The absolute value of f is
       * less than half_MIN (f may be a small normalized
       * float, a denormalized float or a zero).
       *
       * We convert f to a half zero.
       */
      if(e < -10)
         return (int16_t)(s);

      /*
       * E is between -10 and 0.  F is a normalized float,
       * whose magnitude is less than __half_NRM_MIN.
       *
       * We convert f to a denormalized half.
       */

      m = (m | 0x00800000) >> (1 - e);

      //
      // Round to nearest, round "0.5" up.
      //
      // Rounding may cause the significand to overflow and make
      // our number normalized.  Because of the way a half's bits
      // are laid out, we don't have to treat this case separately;
      // the code below will handle it correctly.
      // 

      if(m & 0x00001000) 
         m += 0x00002000;

      //
      // Assemble the half from s, e (zero) and m.
      //

      return (int16_t)(s | (m >> 13));
   }
   else if(e == 0xff - (127 - 15))
   {
      /* F is an infinity; convert f to a half
       * infinity with the same sign as f. */
      if(m == 0)
         return (int16_t)(s | 0x7c00);

      /*
       * F is a NAN; we produce a half NAN that preserves
       * the sign bit and the 10 leftmost bits of the
       * significand of f, with one exception: If the 10
       * leftmost bits are all zero, the NAN would turn 
       * into an infinity, so we have to set at least one
       * bit in the significand.
       */

      m >>= 13;

      return (int16_t)(s | 0x7c00 | m | (m == 0));
   }

   /* E is greater than zero.  F is a normalized float.
    * We try to convert f to a normalized half. */

   /* Round to nearest, round "0.5" up */

   if(m &  0x00001000)
   {
      m += 0x00002000;

      if(m & 0x00800000)
      {
         m =  0;     /* overflow in significand, */
         e += 1;     /* adjust exponent */
      }
   }

   /* Handle exponent overflow */

   if (e > 30)
   {
      overflow();        /* Cause a hardware floating point overflow; */

      /* if this returns, the half becomes an
       * infinity with the same sign as F. */
      return (int16_t)(s | 0x7c00);
   }

   /* Assemble the half from s, e and m. */

   return (int16_t)(s | (e << 10) | (m >> 13));
}

unsigned vec2_packHalf2x16(const float a, const float b)
{
   /* TODO/FIXME - doesn't look completely correct compared to
    * GLM function - split in the middle */
   return (((int32_t)toFloat16(a) << 16) | ((toFloat16(b) & 0xffff)));
}

float vec2_dot(const float *a, const float *b) 
{
	return (a[0]*b[0]) + (a[1]*b[1]);
}

float vec2_cross(const float *a, const float *b) 
{
	return (a[0]*b[1]) - (a[1]*b[0]);
}

void vec2_add(float *dst, const float *src)
{
   unsigned i;
   unsigned n = 2;

   for (i = 0; i < n; i++)
      dst[i] += src[i];
}

void vec2_subtract(float *dst, const float *src)
{
   unsigned i;
   unsigned n = 2;

   for (i = 0; i < n; i++)
      dst[i] -= src[i];
}

void vec2_copy(float *dst, const float *src)
{
   unsigned i;
   unsigned n = 2;

   for (i = 0; i < n; i++)
      dst[i] = src[i];
}
