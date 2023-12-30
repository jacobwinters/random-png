
/* pngrutil.c - utilities to read a PNG file
 *
 * Copyright (c) 2018-2022 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2018 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * This file contains routines that are only called from within
 * libpng itself during the course of reading an image.
 */

#import <limits.h>
#import <stddef.h>

// from png.h
#define PNG_FILTER_VALUE_NONE  0
#define PNG_FILTER_VALUE_SUB   1
#define PNG_FILTER_VALUE_UP    2
#define PNG_FILTER_VALUE_AVG   3
#define PNG_FILTER_VALUE_PAETH 4
#define PNG_FILTER_VALUE_LAST  5

// from pngconf.h
#if CHAR_BIT == 8 && UCHAR_MAX == 255
   typedef unsigned char png_byte;
#else
#  error "libpng requires 8 bit bytes"
#endif
typedef png_byte              * png_bytep;
typedef const png_byte        * png_const_bytep;

static void
png_read_filter_row_sub(size_t rowbytes, png_byte pixel_depth, png_bytep row,
    png_const_bytep prev_row)
{
   size_t i;
   size_t istop = rowbytes;
   unsigned int bpp = (pixel_depth + 7) >> 3;
   png_bytep rp = row + bpp;

   //PNG_UNUSED(prev_row)

   for (i = bpp; i < istop; i++)
   {
      *rp = (png_byte)(((int)(*rp) + (int)(*(rp-bpp))) & 0xff);
      rp++;
   }
}

static void
png_read_filter_row_up(size_t rowbytes, png_byte pixel_depth, png_bytep row,
    png_const_bytep prev_row)
{
   size_t i;
   size_t istop = rowbytes;
   png_bytep rp = row;
   png_const_bytep pp = prev_row;

   for (i = 0; i < istop; i++)
   {
      *rp = (png_byte)(((int)(*rp) + (int)(*pp++)) & 0xff);
      rp++;
   }
}

static void
png_read_filter_row_avg(size_t rowbytes, png_byte pixel_depth, png_bytep row,
    png_const_bytep prev_row)
{
   size_t i;
   png_bytep rp = row;
   png_const_bytep pp = prev_row;
   unsigned int bpp = (pixel_depth + 7) >> 3;
   size_t istop = rowbytes - bpp;

   for (i = 0; i < bpp; i++)
   {
      *rp = (png_byte)(((int)(*rp) +
         ((int)(*pp++) / 2 )) & 0xff);

      rp++;
   }

   for (i = 0; i < istop; i++)
   {
      *rp = (png_byte)(((int)(*rp) +
         (int)(*pp++ + *(rp-bpp)) / 2 ) & 0xff);

      rp++;
   }
}

static void
png_read_filter_row_paeth_1byte_pixel(size_t rowbytes, png_byte pixel_depth, png_bytep row,
    png_const_bytep prev_row)
{
   png_bytep rp_end = row + rowbytes;
   int a, c;

   /* First pixel/byte */
   c = *prev_row++;
   a = *row + c;
   *row++ = (png_byte)a;

   /* Remainder */
   while (row < rp_end)
   {
      int b, pa, pb, pc, p;

      a &= 0xff; /* From previous iteration or start */
      b = *prev_row++;

      p = b - c;
      pc = a - c;

#ifdef PNG_USE_ABS
      pa = abs(p);
      pb = abs(pc);
      pc = abs(p + pc);
#else
      pa = p < 0 ? -p : p;
      pb = pc < 0 ? -pc : pc;
      pc = (p + pc) < 0 ? -(p + pc) : p + pc;
#endif

      /* Find the best predictor, the least of pa, pb, pc favoring the earlier
       * ones in the case of a tie.
       */
      if (pb < pa)
      {
         pa = pb; a = b;
      }
      if (pc < pa) a = c;

      /* Calculate the current pixel in a, and move the previous row pixel to c
       * for the next time round the loop
       */
      c = b;
      a += *row;
      *row++ = (png_byte)a;
   }
}

static void
png_read_filter_row_paeth_multibyte_pixel(size_t rowbytes, png_byte pixel_depth, png_bytep row,
    png_const_bytep prev_row)
{
   unsigned int bpp = (pixel_depth + 7) >> 3;
   png_bytep rp_end = row + bpp;

   /* Process the first pixel in the row completely (this is the same as 'up'
    * because there is only one candidate predictor for the first row).
    */
   while (row < rp_end)
   {
      int a = *row + *prev_row++;
      *row++ = (png_byte)a;
   }

   /* Remainder */
   rp_end = rp_end + (rowbytes - bpp);

   while (row < rp_end)
   {
      int a, b, c, pa, pb, pc, p;

      c = *(prev_row - bpp);
      a = *(row - bpp);
      b = *prev_row++;

      p = b - c;
      pc = a - c;

#ifdef PNG_USE_ABS
      pa = abs(p);
      pb = abs(pc);
      pc = abs(p + pc);
#else
      pa = p < 0 ? -p : p;
      pb = pc < 0 ? -pc : pc;
      pc = (p + pc) < 0 ? -(p + pc) : p + pc;
#endif

      if (pb < pa)
      {
         pa = pb; a = b;
      }
      if (pc < pa) a = c;

      a += *row;
      *row++ = (png_byte)a;
   }
}

void /* PRIVATE */
png_read_filter_row(size_t rowbytes, png_byte pixel_depth, png_bytep row,
    png_const_bytep prev_row, int filter)
{
   if (filter > PNG_FILTER_VALUE_NONE && filter < PNG_FILTER_VALUE_LAST)
   {
      unsigned int bpp = (pixel_depth + 7) >> 3;

      if (filter == PNG_FILTER_VALUE_SUB)
         png_read_filter_row_sub(rowbytes, pixel_depth, row, prev_row);
      if (filter == PNG_FILTER_VALUE_UP)
         png_read_filter_row_up(rowbytes, pixel_depth, row, prev_row);
      if (filter == PNG_FILTER_VALUE_AVG)
         png_read_filter_row_avg(rowbytes, pixel_depth, row, prev_row);
      if (filter == PNG_FILTER_VALUE_PAETH)
         if (bpp == 1)
            png_read_filter_row_paeth_1byte_pixel(rowbytes, pixel_depth, row, prev_row);
         else
            png_read_filter_row_paeth_multibyte_pixel(rowbytes, pixel_depth, row, prev_row);
   }
}
