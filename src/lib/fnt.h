// stb_truetype.h - v1.19 - public domain
// authored from 2009-2016 by Sean Barrett / RAD Game Tools
//
//   This library processes TrueType files:
//        parse files
//        extract glyph metrics
//        extract glyph shapes
//        render glyphs to one-channel bitmaps with antialiasing (box filter)
//        render glyphs to one-channel SDF bitmaps (signed-distance
//        field/function)
//
//   Todo:
//        non-MS cmaps
//        crashproof on bad data
//        hinting? (no longer patented)
//        cleartype-style AA?
//        optimize: use simple memory allocator for intermediates
//        optimize: build edge-list directly from curves
//        optimize: rasterize directly from curves?
//
// ADDITIONAL CONTRIBUTORS
//
//   Mikko Mononen: compound shape support, more cmap formats
//   Tor Andersson: kerning, subpixel rendering
//   Dougall Johnson: OpenType / Type 2 font handling
//   Daniel Ribeiro Maciel: basic GPOS-based kerning
//
//   Misc other:
//       Ryan Gordon
//       Simon Glass
//       github:IntellectualKitty
//       Imanol Celaya
//       Daniel Ribeiro Maciel
//
//   Bug/warning reports/fixes:
//       "Zer" on mollyrocket       Fabian "ryg" Giesen
//       Cass Everitt               Martins Mozeiko
//       stoiko (Haemimont Games)   Cap Petschulat
//       Brian Hook                 Omar Cornut
//       Walter van Niftrik         github:aloucks
//       David Gow                  Peter LaValle
//       David Given                Sergey Popov
//       Ivan-Assen Ivanov          Giumo X. Clanjor
//       Anthony Pesch              Higor Euripedes
//       Johan Duparc               Thomas Fields
//       Hou Qiming                 Derek Vinyard
//       Rob Loach                  Cort Stratton
//       Kenney Phillis Jr.         github:oyvindjam
//       Brian Costabile            github:vassvik
//
// VERSION HISTORY
//
//   1.19 (2018-02-11) GPOS kerning, FNT_fmod
//   1.18 (2018-01-29) add missing function
//   1.17 (2017-07-23) make more arguments const; doc fix
//   1.16 (2017-07-12) SDF support
//   1.15 (2017-03-03) make more arguments const
//   1.14 (2017-01-16) num-fonts-in-TTC function
//   1.13 (2017-01-02) support OpenType fonts, certain Apple fonts
//   1.12 (2016-10-25) suppress warnings about casting away const with
//   -Wcast-qual 1.11 (2016-04-02) fix unused-variable warning 1.10 (2016-04-02)
//   user-defined fabs(); rare memory leak; remove duplicate typedef 1.09
//   (2016-01-16) warning fix; avoid crash on outofmem; use allocation userdata
//   properly 1.08 (2015-09-13) document fnt_Rasterize(); fixes for vertical &
//   horizontal edges 1.07 (2015-08-01) allow PackFontRanges to accept arrays of
//   sparse codepoints;
//                     variant PackFontRanges to pack and render in separate
//                     phases; fix fnt_GetFontOFfsetForIndex (never worked for
//                     non-0 input?); fixed an assert() bug in the new
//                     rasterizer replace assert() with FNT_assert() in new
//                     rasterizer
//
//   Full history can be found at the end of this file.
//
// LICENSE
//
//   See end of file for license information.
//
// USAGE
//
//   Include this file in whatever places neeed to refer to it. In ONE C/C++
//   file, write:
//      #define STB_TRUETYPE_IMPLEMENTATION
//   before the #include of this file. This expands out the actual
//   implementation into that C/C++ file.
//
//   To make the implementation private to the file that generates the
//   implementation,
//      #define FNT_STATIC
//
//   Simple 3D API (don't ship this, but it's fine for tools and quick start)
//           fnt_BakeFontBitmap()               -- bake a font to a bitmap for
//           use as texture fnt_GetBakedQuad()                 -- compute quad
//           to draw for a given char
//
//   Improved 3D API (more shippable):
//           #include "stb_rect_pack.h"           -- optional, but you really
//           want it fnt_PackBegin() fnt_PackSetOversampling()          -- for
//           improved quality on small fonts fnt_PackFontRanges() -- pack and
//           renders fnt_PackEnd() fnt_GetPackedQuad()
//
//   "Load" a font file from a memory buffer (you have to keep the buffer
//   loaded)
//           fnt_InitFont()
//           fnt_GetFontOffsetForIndex()        -- indexing for TTC font
//           collections fnt_GetNumberOfFonts()             -- number of fonts
//           for TTC font collections
//
//   Render a unicode codepoint to a bitmap
//           fnt_GetCodepointBitmap()           -- allocates and returns a
//           bitmap fnt_MakeCodepointBitmap()          -- renders into bitmap
//           you provide fnt_GetCodepointBitmapBox()        -- how big the
//           bitmap must be
//
//   Character advance/positioning
//           fnt_GetCodepointHMetrics()
//           fnt_GetFontVMetrics()
//           fnt_GetFontVMetricsOS2()
//           fnt_GetCodepointKernAdvance()
//
//   Starting with version 1.06, the rasterizer was replaced with a new,
//   faster and generally-more-precise rasterizer. The new rasterizer more
//   accurately measures pixel coverage for anti-aliasing, except in the case
//   where multiple shapes overlap, in which case it overestimates the AA pixel
//   coverage. Thus, anti-aliasing of intersecting shapes may look wrong. If
//   this turns out to be a problem, you can re-enable the old rasterizer with
//        #define FNT_RASTERIZER_VERSION 1
//   which will incur about a 15% speed hit.
//
// ADDITIONAL DOCUMENTATION
//
//   Immediately after this block comment are a series of sample programs.
//
//   After the sample programs is the "header file" section. This section
//   includes documentation for each API function.
//
//   Some important concepts to understand to use this library:
//
//      Codepoint
//         Characters are defined by unicode codepoints, e.g. 65 is
//         uppercase A, 231 is lowercase c with a cedilla, 0x7e30 is
//         the hiragana for "ma".
//
//      Glyph
//         A visual character shape (every codepoint is rendered as
//         some glyph)
//
//      Glyph index
//         A font-specific integer ID representing a glyph
//
//      Baseline
//         Glyph shapes are defined relative to a baseline, which is the
//         bottom of uppercase characters. Characters extend both above
//         and below the baseline.
//
//      Current Point
//         As you draw text to the screen, you keep track of a "current point"
//         which is the origin of each character. The current point's vertical
//         position is the baseline. Even "baked fonts" use this model.
//
//      Vertical Font Metrics
//         The vertical qualities of the font, used to vertically position
//         and space the characters. See docs for fnt_GetFontVMetrics.
//
//      Font Size in Pixels or Points
//         The preferred interface for specifying font sizes in stb_truetype
//         is to specify how tall the font's vertical extent should be in
//         pixels. If that sounds good enough, skip the next paragraph.
//
//         Most font APIs instead use "points", which are a common typographic
//         measurement for describing font size, defined as 72 points per inch.
//         stb_truetype provides a point API for compatibility. However, true
//         "per inch" conventions don't make much sense on computer displays
//         since different monitors have different number of pixels per
//         inch. For example, Windows traditionally uses a convention that
//         there are 96 pixels per inch, thus making 'inch' measurements have
//         nothing to do with inches, and thus effectively defining a point to
//         be 1.333 pixels. Additionally, the TrueType font data provides
//         an explicit scale factor to scale a given font's glyphs to points,
//         but the author has observed that this scale factor is often wrong
//         for non-commercial fonts, thus making fonts scaled in points
//         according to the TrueType spec incoherently sized in practice.
//
// DETAILED USAGE:
//
//  Scale:
//    Select how high you want the font to be, in points or pixels.
//    Call ScaleForPixelHeight or ScaleForMappingEmToPixels to compute
//    a scale factor SF that will be used by all other functions.
//
//  Baseline:
//    You need to select a y-coordinate that is the baseline of where
//    your text will appear. Call GetFontBoundingBox to get the
//    baseline-relative bounding box for all characters. SF*-y0 will be the
//    distance in pixels that the worst-case character could extend above the
//    baseline, so if you want the top edge of characters to appear at the top
//    of the screen where y=0, then you would set the baseline to SF*-y0.
//
//  Current point:
//    Set the current point where the first character will appear. The
//    first character could extend left of the current point; this is font
//    dependent. You can either choose a current point that is the leftmost
//    point and hope, or add some padding, or check the bounding box or
//    left-side-bearing of the first character to be displayed and set
//    the current point based on that.
//
//  Displaying a character:
//    Compute the bounding box of the character. It will contain signed values
//    relative to <current_point, baseline>. I.e. if it returns x0,y0,x1,y1,
//    then the character should be displayed in the rectangle from
//    <current_point+SF*x0, baseline+SF*y0> to
//    <current_point+SF*x1,baseline+SF*y1).
//
//  Advancing for the next character:
//    Call GlyphHMetrics, and compute 'current_point += SF * advance'.
//
//
// ADVANCED USAGE
//
//   Quality:
//
//    - Use the functions with Subpixel at the end to allow your characters
//      to have subpixel positioning. Since the font is anti-aliased, not
//      hinted, this is very import for quality. (This is not possible with
//      baked fonts.)
//
//    - Kerning is now supported, and if you're supporting subpixel rendering
//      then kerning is worth using to give your text a polished look.
//
//   Performance:
//
//    - Convert Unicode codepoints to glyph indexes and operate on the glyphs;
//      if you don't do this, stb_truetype is forced to do the conversion on
//      every call.
//
//    - There are a lot of memory allocations. We should modify it to take
//      a temp buffer and allocate from the temp buffer (without freeing),
//      should help performance a lot.
//
// NOTES
//
//   The system uses the raw data found in the .ttf file without changing it
//   and without building auxiliary data structures. This is a bit inefficient
//   on little-endian systems (the data is big-endian), but assuming you're
//   caching the bitmaps or glyph shapes this shouldn't be a big deal.
//
//   It appears to be very hard to programmatically determine what font a
//   given file is in a general way. I provide an API for this, but I don't
//   recommend it.
//
//
// SOURCE STATISTICS (based on v0.6c, 2050 LOC)
//
//   Documentation & header file        520 LOC  \___ 660 LOC documentation
//   Sample code                        140 LOC  /
//   Truetype parsing                   620 LOC  ---- 620 LOC TrueType
//   Software rasterization             240 LOC  \                           .
//   Curve tesselation                  120 LOC   \__ 550 LOC Bitmap creation
//   Bitmap management                  100 LOC   /
//   Baked bitmap interface              70 LOC  /
//   Font name matching & access        150 LOC  ---- 150
//   C runtime library abstraction       60 LOC  ----  60
//
//
// PERFORMANCE MEASUREMENTS FOR 1.06:
//
//                      32-bit     64-bit
//   Previous release:  8.83 s     7.68 s
//   Pool allocations:  7.72 s     6.34 s
//   Inline sort     :  6.54 s     5.65 s
//   New rasterizer  :  5.63 s     5.00 s

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
////
////  SAMPLE PROGRAMS
////
//
//  Incomplete text-in-3d-api example, which draws quads properly aligned to be
//  lossless
//
#if 0
#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate
                                     // implementation
#include "stb_truetype.h"

unsigned char ttf_buffer[1<<20];
unsigned char temp_bitmap[512*512];

fnt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
GLuint ftex;

void my_fnt_initfont(void)
{
   fread(ttf_buffer, 1, 1<<20, fopen("c:/windows/fonts/times.ttf", "rb"));
   fnt_BakeFontBitmap(ttf_buffer,0, 32.0, temp_bitmap,512,512, 32,96, cdata); // no guarantee this fits!
   // can free ttf_buffer at this point
   glGenTextures(1, &ftex);
   glBindTexture(GL_TEXTURE_2D, ftex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512,512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
   // can free temp_bitmap at this point
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void my_fnt_print(float x, float y, char *text)
{
   // assume orthographic projection with units = screen pixels, origin at top left
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, ftex);
   glBegin(GL_QUADS);
   while (*text) {
      if (*text >= 32 && *text < 128) {
         fnt_aligned_quad q;
         fnt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9
         glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y0);
         glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y0);
         glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y1);
         glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y1);
      }
      ++text;
   }
   glEnd();
}
#endif
//
//
//////////////////////////////////////////////////////////////////////////////
//
// Complete program (this compiles): get a single bitmap, print as ASCII art
//
#if 0
#include <stdio.h>
#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate
                                     // implementation
#include "stb_truetype.h"

char ttf_buffer[1<<25];

int main(int argc, char **argv)
{
   fnt_info font;
   unsigned char *bitmap;
   int w,h,i,j,c = (argc > 1 ? atoi(argv[1]) : 'a'), s = (argc > 2 ? atoi(argv[2]) : 20);

   fread(ttf_buffer, 1, 1<<25, fopen(argc > 3 ? argv[3] : "c:/windows/fonts/arialbd.ttf", "rb"));

   fnt_InitFont(&font, ttf_buffer, fnt_GetFontOffsetForIndex(ttf_buffer,0));
   bitmap = fnt_GetCodepointBitmap(&font, 0,fnt_ScaleForPixelHeight(&font, s), c, &w, &h, 0,0);

   for (j=0; j < h; ++j) {
      for (i=0; i < w; ++i)
         putchar(" .:ioVM@"[bitmap[j*w+i]>>5]);
      putchar('\n');
   }
   return 0;
}
#endif
//
// Output:
//
//     .ii.
//    @@@@@@.
//   V@Mio@@o
//   :i.  V@V
//     :oM@@M
//   :@@@MM@M
//   @@o  o@M
//  :@@.  M@M
//   @@@o@@@@
//   :M@@V:@@.
//
//////////////////////////////////////////////////////////////////////////////
//
// Complete program: print "Hello World!" banner, with bugs
//
#if 0
char buffer[24<<20];
unsigned char screen[20][79];

int main(int arg, char **argv)
{
   fnt_info font;
   int i,j,ascent,baseline,ch=0;
   float scale, xpos=2; // leave a little padding in case the character extends left
   char *text = "Heljo World!"; // intentionally misspelled to show 'lj' brokenness

   fread(buffer, 1, 1000000, fopen("c:/windows/fonts/arialbd.ttf", "rb"));
   fnt_InitFont(&font, buffer, 0);

   scale = fnt_ScaleForPixelHeight(&font, 15);
   fnt_GetFontVMetrics(&font, &ascent,0,0);
   baseline = (int) (ascent*scale);

   while (text[ch]) {
      int advance,lsb,x0,y0,x1,y1;
      float x_shift = xpos - (float) floor(xpos);
      fnt_GetCodepointHMetrics(&font, text[ch], &advance, &lsb);
      fnt_GetCodepointBitmapBoxSubpixel(&font, text[ch], scale,scale,x_shift,0, &x0,&y0,&x1,&y1);
      fnt_MakeCodepointBitmapSubpixel(&font, &screen[baseline + y0][(int) xpos + x0], x1-x0,y1-y0, 79, scale,scale,x_shift,0, text[ch]);
      // note that this stomps the old data, so where character boxes overlap (e.g. 'lj') it's wrong
      // because this API is really for baking character bitmaps into textures. if you want to render
      // a sequence of characters, you really need to render each bitmap to a temp buffer, then
      // "alpha blend" that into the working buffer
      xpos += (advance * scale);
      if (text[ch+1])
         xpos += scale*fnt_GetCodepointKernAdvance(&font, text[ch],text[ch+1]);
      ++ch;
   }

   for (j=0; j < 20; ++j) {
      for (i=0; i < 78; ++i)
         putchar(" .:ioVM@"[screen[j][i]>>5]);
      putchar('\n');
   }

   return 0;
}
#endif

// #define your own (u)fnt_int8/16/32 before including to override this
#ifndef fnt_uint8
typedef unsigned char fnt_uint8;
typedef signed char fnt_int8;
typedef unsigned short fnt_uint16;
typedef signed short fnt_int16;
typedef unsigned int fnt_uint32;
typedef signed int fnt_int32;
#endif

typedef char fnt__check_size32[sizeof(fnt_int32) == 4 ? 1 : -1];
typedef char fnt__check_size16[sizeof(fnt_int16) == 2 ? 1 : -1];

// e.g. #define your own FNT_ifloor/FNT_iceil() to avoid math.h
#define FNT_ifloor(x) floori(x)
#define FNT_iceil(x) ceili(x)
#define FNT_sqrt(x) sqrta(x)
#define FNT_pow(x, y) powa(x, y)
#define FNT_fmod(x, y) mod(x, y)
#define FNT_cos(x) cosa(x)
#define FNT_fabs(x) absf(x)

// #define your own functions "FNT_malloc" / "FNT_free" to avoid malloc.h
#ifndef FNT_malloc
#include <stdlib.h>
#define FNT_malloc(x, u) ((void)(u), malloc(x))
#define FNT_free(x, u) ((void)(u), free(x))
#endif

#ifndef FNT_assert
#include <assert.h>
#define FNT_assert(x) assert(x)
#endif

#ifndef FNT_strlen
#include <string.h>
#define FNT_strlen(x) strlen(x)
#endif

#ifndef FNT_memcpy
#include <string.h>
#define FNT_memcpy memcpy
#define FNT_memset memset
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////
////   INTERFACE
////
////
// private structure
struct fnt__buf {
  unsigned char *data;
  int cursor;
  int size;
};

//////////////////////////////////////////////////////////////////////////////
//
// TEXTURE BAKING API
//
// If you use this API, you only have to call two functions ever.
//

struct fnt_baked_char {
  unsigned short x0, y0, x1, y1;  // coordinates of bbox in bitmap
  float xoff, yoff;
  float xadvance;
};
static int
fnt_bake_bitmap(
    const unsigned char *data,
    int offset,          // font location (use offset=0 for plain .ttf)
    float pixel_height,  // height of font in pixels
    unsigned char *pixels, int pw, int ph,  // bitmap to be filled in
    int first_char, int num_chars,          // characters to bake
    struct fnt_baked_char *chardata);  // you allocate this, it's num_chars long
// if return is positive, the first unused row of the bitmap
// if return is negative, returns the negative of the number of characters that
// fit if return is 0, no characters fit and no rows were used This uses a very
// crappy packing.

struct fnt_aligned_quad {
  float x0, y0, s0, t0;  // top-left
  float x1, y1, s1, t1;  // bottom-right
};
static void
fnt_get_baked_quad(
    const struct fnt_baked_char *chardata, int pw,
    int ph,          // same data as above
    int char_index,  // character to display
    float *xpos,
    float *ypos,  // pointers to current position in screen pixel space
    struct fnt_aligned_quad *q,  // output: quad to draw
    int opengl_fillrule);  // true if opengl fill rule; false if DX9 or earlier
// Call GetBakedQuad with char_index = 'character - first_char', and it
// creates the quad you need to draw and advances the current position.
//
// The coordinate system used assumes y increases downwards.
//
// Characters will extend both above and below the current position;
// see discussion of "BASELINE" above.
//
// It's inefficient; you might want to c&p it and optimize it.

//////////////////////////////////////////////////////////////////////////////
//
// NEW TEXTURE BAKING API
//
// This provides options for packing multiple fonts into one atlas, not
// perfectly but better than nothing.
struct fnt_packed_char {
  unsigned short x0, y0, x1, y1;  // coordinates of bbox in bitmap
  float xoff, yoff, xadvance;
  float xoff2, yoff2;
};
// this is an opaque structure that you shouldn't mess with which holds
// all the context needed from PackBegin to PackEnd.
struct fnt_pack_ctx {
  void *user_allocator_context;
  void *pack_info;
  int width;
  int height;
  int stride_in_bytes;
  int padding;
  unsigned int h_oversample, v_oversample;
  unsigned char *pixels;
  void *nodes;
};

struct fnt_info;
#ifndef STB_RECT_PACK_VERSION
struct stbrp_rect;
#endif

static int
fnt_pack_begin(struct fnt_pack_ctx *spc, unsigned char *pix, int w, int h,
               int stride_in_bytes, int padding, void *alloc_ctx);
// Initializes a packing context stored in the passed-in fnt_pack_ctx.
// Future calls using this context will pack characters into the bitmap passed
// in here: a 1-channel bitmap that is width * height. stride_in_bytes is
// the distance from one row to the next (or 0 to mean they are packed tightly
// together). "padding" is the amount of padding to leave between each
// character (normally you want '1' for bitmaps you'll use as textures with
// bilinear filtering).
//
// Returns 0 on failure, 1 on success.

static void
fnt_pack_end(struct fnt_pack_ctx *spc);
// Cleans up the packing context and frees all memory.

#define FNT_POINT_SIZE(x) (-(x))

static int
fnt_pack_range(struct fnt_pack_ctx *spc, const unsigned char *fontdata,
               int font_index, float font_size, int first_unicode_char_in_range,
               int num_chars_in_range,
               struct fnt_packed_char *chardata_for_range);
// Creates character bitmaps from the font_index'th font found in fontdata (use
// font_index=0 if you don't know what that is). It creates num_chars_in_range
// bitmaps for characters with unicode values starting at
// first_unicode_char_in_range and increasing. Data for how to render them is
// stored in chardata_for_range; pass these to fnt_GetPackedQuad to get back
// renderable quads.
//
// font_size is the full height of the character from ascender to descender,
// as computed by fnt_ScaleForPixelHeight. To use a point size as computed
// by fnt_ScaleForMappingEmToPixels, wrap the point size in FNT_POINT_SIZE()
// and pass that result as 'font_size':
//       ...,                  20 , ... // font max minus min y is 20 pixels
//       tall
//       ..., FNT_POINT_SIZE(20), ... // 'M' is 20 pixels tall
struct fnt_pack_range {
  float font_size;
  int first_unicode_codepoint_in_range;  // if non-zero, then the chars are
                                         // continuous, and this is the first
                                         // codepoint
  int *array_of_unicode_codepoints;  // if non-zero, then this is an array of
                                     // unicode codepoints
  int num_chars;
  struct fnt_packed_char *chardata_for_range;  // output
  unsigned char h_oversample,
      v_oversample;  // don't set these, they're used internally
};

static int
fnt_pack_ranges(struct fnt_pack_ctx *spc, const unsigned char *fontdata,
                int font_index, struct fnt_pack_range *ranges, int num_ranges);
// Creates character bitmaps from multiple ranges of characters stored in
// ranges. This will usually create a better-packed bitmap than multiple
// calls to fnt_PackFontRange. Note that you can call this multiple
// times within a single PackBegin/PackEnd.

static void
fnt_pack_set_oversampling(struct fnt_pack_ctx *spc, unsigned int h_oversample,
                          unsigned int v_oversample);
// Oversampling a font increases the quality by allowing higher-quality subpixel
// positioning, and is especially valuable at smaller text sizes.
//
// This function sets the amount of oversampling for all following calls to
// fnt_PackFontRange(s) or fnt_PackFontRangesGatherRects for a given
// pack context. The default (no oversampling) is achieved by h_oversample=1
// and v_oversample=1. The total number of pixels required is
// h_oversample*v_oversample larger than the default; for example, 2x2
// oversampling requires 4x the storage of 1x1. For best results, render
// oversampled textures with bilinear filtering. Look at the readme in
// stb/tests/oversample for information about oversampled fonts
//
// To use with PackFontRangesGather etc., you must set it before calls
// call to PackFontRangesGatherRects.

static void
fnt_get_packed_quad(
    const struct fnt_packed_char *chardata, int pw,
    int ph,          // same data as above
    int char_index,  // character to display
    float *xpos,
    float *ypos,  // pointers to current position in screen pixel space
    struct fnt_aligned_quad *q,  // output: quad to draw
    int align_to_integer);

static int
fnt_pack_ranges_gather_rects(struct fnt_pack_ctx *spc,
                             const struct fnt_info *info,
                             struct fnt_pack_range *ranges, int num_ranges,
                             struct stbrp_rect *rects);
static void
fnt_pack_ranges_pack_rects(struct fnt_pack_ctx *spc, struct stbrp_rect *rects,
                           int num_rects);
static int
fnt_pack_ranges_render_into_rects(struct fnt_pack_ctx *spc,
                                  const struct fnt_info *info,
                                  struct fnt_pack_range *ranges, int num_ranges,
                                  struct stbrp_rect *rects);
// Calling these functions in sequence is roughly equivalent to calling
// fnt_PackFontRanges(). If you more control over the packing of multiple
// fonts, or if you want to pack custom data into a font texture, take a look
// at the source to of fnt_PackFontRanges() and create a custom version
// using these functions, e.g. call GatherRects multiple times,
// building up a single array of rects, then call PackRects once,
// then call RenderIntoRects repeatedly. This may result in a
// better packing than calling PackFontRanges multiple times
// (or it may not).

//////////////////////////////////////////////////////////////////////////////
//
// FONT LOADING
//
//
static int
fnt_get_number_of_fonts(const unsigned char *data);
// This function will determine the number of fonts in a font file.  TrueType
// collection (.ttc) files may contain multiple fonts, while TrueType font
// (.ttf) files only contain one font. The number of fonts can be used for
// indexing with the previous function where the index is between zero and one
// less than the total fonts. If an error occurs, -1 is returned.

static int
fnt_get_font_offset_for_index(const unsigned char *data, int index);
// Each .ttf/.ttc file may have more than one font. Each font has a sequential
// index number starting from 0. Call this function to get the font offset for
// a given index; it returns -1 if the index is out of range. A regular .ttf
// file will only define one font and it always be at offset 0, so it will
// return '0' for index 0, and -1 for all other indices.

// The following structure is defined publically so you can declare one on
// the stack or as a global or etc, but you should treat it as opaque.
struct fnt_info {
  void *userdata;
  unsigned char *data;  // pointer to .ttf file
  int fontstart;        // offset of start of font

  int numGlyphs;  // number of glyphs, needed for range checking

  int loca, head, glyf, hhea, hmtx, kern,
      gpos;              // table locations as offset from start of .ttf
  int index_map;         // a cmap mapping for our chosen character encoding
  int indexToLocFormat;  // format needed to map from glyph index to glyph

  struct fnt__buf cff;          // cff font data
  struct fnt__buf charstrings;  // the charstring index
  struct fnt__buf gsubrs;       // global charstring subroutines index
  struct fnt__buf subrs;        // private charstring subroutines index
  struct fnt__buf fontdicts;    // array of font dicts
  struct fnt__buf fdselect;     // map from glyph to fontdict
};

static int
fnt_init(struct fnt_info *info, const unsigned char *data, int offset);
// Given an offset into the file that defines a font, this function builds
// the necessary cached info for the rest of the system. You must allocate
// the fnt_info yourself, and fnt_InitFont will fill it out. You don't
// need to do anything special to free it, because the contents are pure
// value data with no additional data structures. Returns 0 on failure.

//////////////////////////////////////////////////////////////////////////////
//
// CHARACTER TO GLYPH-INDEX CONVERSIOn

static int
fnt_fnd_glyph_index(const struct fnt_info *info, int unicode_codepoint);
// If you're going to perform multiple operations on the same character
// and you want a speed-up, call this function with the character you're
// going to process, then use glyph-based functions instead of the
// codepoint-based functions.

//////////////////////////////////////////////////////////////////////////////
//
// CHARACTER PROPERTIES
//

static float
fnt_scale_for_pixel_height(const struct fnt_info *info, float pixels);
// computes a scale factor to produce a font whose "height" is 'pixels' tall.
// Height is measured as the distance from the highest ascender to the lowest
// descender; in other words, it's equivalent to calling fnt_GetFontVMetrics
// and computing:
//       scale = pixels / (ascent - descent)
// so if you prefer to measure height by the ascent only, use a similar
// calculation.

static float
fnt_scale_for_mapping_em_to_pixels(const struct fnt_info *info, float pixels);
// computes a scale factor to produce a font whose EM size is mapped to
// 'pixels' tall. This is probably what traditional APIs compute, but
// I'm not positive.

static void
fnt_get_vmetrics(const struct fnt_info *info, int *ascent, int *descent,
                 int *lineGap);
// ascent is the coordinate above the baseline the font extends; descent
// is the coordinate below the baseline the font extends (i.e. it is typically
// negative) lineGap is the spacing between one row's descent and the next row's
// ascent... so you should advance the vertical position by "*ascent - *descent
// + *lineGap"
//   these are expressed in unscaled coordinates, so you must multiply by
//   the scale factor for a given size

static int
fnt_get_vmetrics_OS2(const struct fnt_info *info, int *typoAscent,
                     int *typoDescent, int *typoLineGap);
// analogous to GetFontVMetrics, but returns the "typographic" values from the
// OS/2 table (specific to MS/Windows TTF files).
//
// Returns 1 on success (table present), 0 on failure.

static void
fnt_get_bounding_box(const struct fnt_info *info, int *x0, int *y0, int *x1,
                     int *y1);
// the bounding box around all possible characters

static void
fnt_get_codepoint_hmetrics(const struct fnt_info *info, int codepoint,
                           int *advanceWidth, int *leftSideBearing);
// leftSideBearing is the offset from the current horizontal position to the
// left edge of the character advanceWidth is the offset from the current
// horizontal position to the next horizontal position
//   these are expressed in unscaled coordinates
static int
fnt_get_codepoint_kern_advance(const struct fnt_info *info, int ch1, int ch2);

// an additional amount to add to the 'advance' value between ch1 and ch2

static int
fnt_get_codepoint_box(const struct fnt_info *info, int codepoint, int *x0,
                      int *y0, int *x1, int *y1);
// Gets the bounding box of the visible part of the glyph, in unscaled
// coordinates

static void
fnt_get_glyph_hmetrics(const struct fnt_info *info, int glyph_index,
                       int *advanceWidth, int *leftSideBearing);
static int
fnt_get_glyph_kern_advance(const struct fnt_info *info, int glyph1, int glyph2);
static int
fnt_get_glyph_box(const struct fnt_info *info, int glyph_index, int *x0,
                  int *y0, int *x1, int *y1);
// as above, but takes one or more glyph indices for greater efficiency

//////////////////////////////////////////////////////////////////////////////
//
// GLYPH SHAPES (you probably don't need these, but they have to go before
// the bitmaps for C declaration-order reasons)
//

#ifndef FNT_vmove  // you can predefine these to use different values (but why?)
enum { FNT_vmove = 1, FNT_vline, FNT_vcurve, FNT_vcubic };
#endif

#ifndef fnt_vertex  // you can predefine this to use different values // (we
                    // share this with other code at RAD)
#define fnt_vertex_type \
  short  // can't use fnt_int16 because that's not visible in the header file
struct fnt_vertex {
  fnt_vertex_type x, y, cx, cy, cx1, cy1;
  unsigned char type, padding;
};
#endif

static int
fnt_is_glyph_empty(const struct fnt_info *info, int glyph_index);
// returns non-zero if nothing is drawn for this glyph

static int
fnt_get_codepoint_shape(const struct fnt_info *info, int unicode_codepoint,
                        struct fnt_vertex **vertices);
static int
fnt_get_glyph_shape(const struct fnt_info *info, int glyph_index,
                    struct fnt_vertex **vertices);
// returns # of vertices and fills *vertices with the pointer to them
//   these are expressed in "unscaled" coordinates
//
// The shape is a series of countours. Each one starts with
// a FNT_moveto, then consists of a series of mixed
// FNT_lineto and FNT_curveto segments. A lineto
// draws a line from previous endpoint to its x,y; a curveto
// draws a quadratic bezier from previous endpoint to
// its x,y, using cx,cy as the bezier control point.

static void
fnt_free_shape(const struct fnt_info *info, struct fnt_vertex *vertices);
// frees the data allocated above

//////////////////////////////////////////////////////////////////////////////
//
// BITMAP RENDERING
//

static void
fnt_free_bitmap(unsigned char *bitmap, void *userdata);
// frees the bitmap allocated below

static unsigned char *
fnt_get_codepoint_bitmap(const struct fnt_info *info, float scale_x,
                         float scale_y, int codepoint, int *width, int *height,
                         int *xoff, int *yoff);
// allocates a large-enough single-channel 8bpp bitmap and renders the
// specified character/glyph at the specified scale into it, with
// antialiasing. 0 is no coverage (transparent), 255 is fully covered (opaque).
// *width & *height are filled out with the width & height of the bitmap,
// which is stored left-to-right, top-to-bottom.
//
// xoff/yoff are the offset it pixel space from the glyph origin to the top-left
// of the bitmap

static unsigned char *
fnt_get_codepoint_bitmap_subpixel(const struct fnt_info *info, float scale_x,
                                  float scale_y, float shift_x, float shift_y,
                                  int codepoint, int *width, int *height,
                                  int *xoff, int *yoff);
// the same as fnt_GetCodepoitnBitmap, but you can specify a subpixel
// shift for the character

static void
fnt_make_codepoint_bitmap(const struct fnt_info *info, unsigned char *output,
                          int out_w, int out_h, int out_stride, float scale_x,
                          float scale_y, int codepoint);
// the same as fnt_GetCodepointBitmap, but you pass in storage for the bitmap
// in the form of 'output', with row spacing of 'out_stride' bytes. the bitmap
// is clipped to out_w/out_h bytes. Call fnt_GetCodepointBitmapBox to get the
// width and height and positioning info for it first.

static void
fnt_make_codepoint_bitmap_subpixel(const struct fnt_info *info,
                                   unsigned char *output, int out_w, int out_h,
                                   int out_stride, float scale_x, float scale_y,
                                   float shift_x, float shift_y, int codepoint);
// same as fnt_MakeCodepointBitmap, but you can specify a subpixel
// shift for the character

static void
fnt_make_codepoint_bitmap_subpixel_prefilter(
    const struct fnt_info *info, unsigned char *output, int out_w, int out_h,
    int out_stride, float scale_x, float scale_y, float shift_x, float shift_y,
    int oversample_x, int oversample_y, float *sub_x, float *sub_y,
    int codepoint);
// same as fnt_MakeCodepointBitmapSubpixel, but prefiltering
// is performed (see fnt_PackSetOversampling)

static void
fnt_get_codepoint_bitmap_box(const struct fnt_info *font, int codepoint,
                             float scale_x, float scale_y, int *ix0, int *iy0,
                             int *ix1, int *iy1);
// get the bbox of the bitmap centered around the glyph origin; so the
// bitmap width is ix1-ix0, height is iy1-iy0, and location to place
// the bitmap top left is (leftSideBearing*scale,iy0).
// (Note that the bitmap uses y-increases-down, but the shape uses
// y-increases-up, so CodepointBitmapBox and CodepointBox are inverted.)

static void
fnt_get_codepoint_bitmap_box_subpixel(const struct fnt_info *font,
                                      int codepoint, float scale_x,
                                      float scale_y, float shift_x,
                                      float shift_y, int *ix0, int *iy0,
                                      int *ix1, int *iy1);
// same as fnt_GetCodepointBitmapBox, but you can specify a subpixel
// shift for the character

// the following functions are equivalent to the above functions, but operate
// on glyph indices instead of Unicode codepoints (for efficiency)
static unsigned char *
fnt_get_glyph_bitmap(const struct fnt_info *info, float scale_x, float scale_y,
                     int glyph, int *width, int *height, int *xoff, int *yoff);
static unsigned char *
fnt_get_glyph_bitmap_subpixel(const struct fnt_info *info, float scale_x,
                              float scale_y, float shift_x, float shift_y,
                              int glyph, int *width, int *height, int *xoff,
                              int *yoff);
static void
fnt_make_glyph_bitmap(const struct fnt_info *info, unsigned char *output,
                      int out_w, int out_h, int out_stride, float scale_x,
                      float scale_y, int glyph);
static void
fnt_make_glyph_bitmap_subpixel(const struct fnt_info *info,
                               unsigned char *output, int out_w, int out_h,
                               int out_stride, float scale_x, float scale_y,
                               float shift_x, float shift_y, int glyph);
static void
fnt_make_glyph_bitmap_subpixel_prefilter(
    const struct fnt_info *info, unsigned char *output, int out_w, int out_h,
    int out_stride, float scale_x, float scale_y, float shift_x, float shift_y,
    int oversample_x, int oversample_y, float *sub_x, float *sub_y, int glyph);
static void
fnt_get_glyph_bitmap_box(const struct fnt_info *font, int glyph, float scale_x,
                         float scale_y, int *ix0, int *iy0, int *ix1, int *iy1);
static void
fnt_get_glyph_bitmap_box_subpixel(const struct fnt_info *font, int glyph,
                                  float scale_x, float scale_y, float shift_x,
                                  float shift_y, int *ix0, int *iy0, int *ix1,
                                  int *iy1);

// @TODO: don't expose this structure
struct fnt__bitmap {
  int w, h, stride;
  unsigned char *pixels;
};

// rasterize a shape with quadratic beziers into a bitmap
static void
fnt_rasterize(struct fnt__bitmap *result,  // 1-channel bitmap to draw into
              float flatness_in_pixels,    // allowable error of curve in pixels
              struct fnt_vertex *vertices,  // array of vertices defining shape
              int num_verts,                // number of vertices in above array
              float scale_x, float scale_y,  // scale applied to input vertices
              float shift_x,
              float shift_y,         // translation applied to input vertices
              int x_off, int y_off,  // another translation applied to input
              int invert,            // if non-zero, vertically flip shape
              void *userdata);       // context for to FNT_MALLOC

//////////////////////////////////////////////////////////////////////////////
//
// Signed Distance Function (or Field) rendering

static void
fnt_free_sdf(unsigned char *bitmap, void *userdata);
// frees the SDF bitmap allocated below

static unsigned char *
fnt_get_glyph_sdf(const struct fnt_info *info, float scale, int glyph,
                  int padding, unsigned char onedge_value,
                  float pixel_dist_scale, int *width, int *height, int *xoff,
                  int *yoff);
static unsigned char *
fnt_get_codepoint_sdf(const struct fnt_info *info, float scale, int codepoint,
                      int padding, unsigned char onedge_value,
                      float pixel_dist_scale, int *width, int *height,
                      int *xoff, int *yoff);
// These functions compute a discretized SDF field for a single character,
// suitable for storing in a single-channel texture, sampling with bilinear
// filtering, and testing against larger than some threshhold to produce
// scalable fonts.
//        info              --  the font
//        scale             --  controls the size of the resulting SDF bitmap,
//        same as it would be creating a regular bitmap glyph/codepoint   -- the
//        character to generate the SDF for padding           --  extra "pixels"
//        around the character which are filled with the distance to the
//        character (not 0),
//                                 which allows effects like bit outlines
//        onedge_value      --  value 0-255 to test the SDF against to
//        reconstruct the character (i.e. the isocontour of the character)
//        pixel_dist_scale  --  what value the SDF should increase by when
//        moving one SDF "pixel" away from the edge (on the 0..255 scale)
//                                 if positive, > onedge_value is inside; if
//                                 negative, < onedge_value is inside
//        width,height      --  output height & width of the SDF bitmap
//        (including padding) xoff,yoff         --  output origin of the
//        character return value      --  a 2D array of bytes 0..255,
//        width*height in size
//
// pixel_dist_scale & onedge_value are a scale & bias that allows you to make
// optimal use of the limited 0..255 for your application, trading off precision
// and special effects. SDF values outside the range 0..255 are clamped to
// 0..255.
//
// Example:
//      scale = fnt_ScaleForPixelHeight(22)
//      padding = 5
//      onedge_value = 180
//      pixel_dist_scale = 180/5.0 = 36.0
//
//      This will create an SDF bitmap in which the character is about 22 pixels
//      high but the whole bitmap is about 22+5+5=32 pixels high. To produce a
//      filled shape, sample the SDF at each pixel and fill the pixel if the SDF
//      value is greater than or equal to 180/255. (You'll actually want to
//      antialias, which is beyond the scope of this example.) Additionally, you
//      can compute offset outlines (e.g. to stroke the character border inside
//      & outside, or only outside). For example, to fill outside the character
//      up to 3 SDF pixels, you would compare against (180-36.0*3)/255 = 72/255.
//      The above choice of variables maps a range from 5 pixels outside the
//      shape to 2 pixels inside the shape to 0..255; this is intended primarily
//      for apply outside effects only (the interior range is needed to allow
//      proper antialiasing of the font at *smaller* sizes)
//
// The function computes the SDF analytically at each SDF pixel, not by e.g.
// building a higher-res bitmap and approximating it. In theory the quality
// should be as high as possible for an SDF of this size & representation, but
// unclear if this is true in practice (perhaps building a higher-res bitmap
// and computing from that can allow drop-out prevention).
//
// The algorithm has not been optimized at all, so expect it to be slow
// if computing lots of characters or very large sizes.

//////////////////////////////////////////////////////////////////////////////
//
// Finding the right font...
//
// You should really just solve this offline, keep your own tables
// of what font is what, and don't try to get it out of the .ttf file.
// That's because getting it out of the .ttf file is really hard, because
// the names in the file can appear in many possible encodings, in many
// possible languages, and e.g. if you need a case-insensitive comparison,
// the details of that depend on the encoding & language in a complex way
// (actually underspecified in truetype, but also gigantic).
//
// But you can use the provided functions in two possible ways:
//     fnt_FindMatchingFont() will use *case-sensitive* comparisons on
//             unicode-encoded names to try to find the font you want;
//             you can run this before calling fnt_InitFont()
//
//     fnt_GetFontNameString() lets you get any of the various strings
//             from the file yourself and do your own comparisons on them.
//             You have to have called fnt_InitFont() first.

static int
fnt_find_matching_font(const unsigned char *fontdata, const char *name,
                       int flags);
// returns the offset (not index) of the font that matches, or -1 if none
//   if you use FNT_MACSTYLE_DONTCARE, use a font name like "Arial Bold".
//   if you use any other flag, use a font name like "Arial"; this checks
//     the 'macStyle' header field; i don't know if fonts set this consistently
#define FNT_MACSTYLE_DONTCARE 0
#define FNT_MACSTYLE_BOLD 1
#define FNT_MACSTYLE_ITALIC 2
#define FNT_MACSTYLE_UNDERSCORE 4
#define FNT_MACSTYLE_NONE \
  8  // <= not same as 0, this makes us check the bitfield is 0

static int
fnt_compare_utf8_to_utf16_bigendian(const char *s1, int len1, const char *s2,
                                    int len2);
// returns 1/0 whether the first string interpreted as utf8 is identical to
// the second string interpreted as big-endian utf16... useful for strings from
// next func

static const char *
fnt_get_font_name_string(const struct fnt_info *font, int *length,
                         int platformID, int encodingID, int languageID,
                         int nameID);
// returns the string (which may be big-endian double byte, e.g. for unicode)
// and puts the length in bytes in *length.
//
// some of the values for the IDs are below; for more see the truetype spec:
//     http://developer.apple.com/textfonts/TTRefMan/RM06/Chap6name.html
//     http://www.microsoft.com/typography/otspec/name.htm

enum {  // platformID
  FNT_PLATFORM_ID_UNICODE = 0,
  FNT_PLATFORM_ID_MAC = 1,
  FNT_PLATFORM_ID_ISO = 2,
  FNT_PLATFORM_ID_MICROSOFT = 3
};

enum {  // encodingID for FNT_PLATFORM_ID_UNICODE
  FNT_UNICODE_EID_UNICODE_1_0 = 0,
  FNT_UNICODE_EID_UNICODE_1_1 = 1,
  FNT_UNICODE_EID_ISO_10646 = 2,
  FNT_UNICODE_EID_UNICODE_2_0_BMP = 3,
  FNT_UNICODE_EID_UNICODE_2_0_FULL = 4
};

enum {  // encodingID for FNT_PLATFORM_ID_MICROSOFT
  FNT_MS_EID_SYMBOL = 0,
  FNT_MS_EID_UNICODE_BMP = 1,
  FNT_MS_EID_SHIFTJIS = 2,
  FNT_MS_EID_UNICODE_FULL = 10
};

enum {  // encodingID for FNT_PLATFORM_ID_MAC; same as Script Manager codes
  FNT_MAC_EID_ROMAN = 0,
  FNT_MAC_EID_ARABIC = 4,
  FNT_MAC_EID_JAPANESE = 1,
  FNT_MAC_EID_HEBREW = 5,
  FNT_MAC_EID_CHINESE_TRAD = 2,
  FNT_MAC_EID_GREEK = 6,
  FNT_MAC_EID_KOREAN = 3,
  FNT_MAC_EID_RUSSIAN = 7
};

enum {  // languageID for FNT_PLATFORM_ID_MICROSOFT; same as LCID...
        // problematic because there are e.g. 16 english LCIDs and 16 arabic
        // LCIDs
  FNT_MS_LANG_ENGLISH = 0x0409,
  FNT_MS_LANG_ITALIAN = 0x0410,
  FNT_MS_LANG_CHINESE = 0x0804,
  FNT_MS_LANG_JAPANESE = 0x0411,
  FNT_MS_LANG_DUTCH = 0x0413,
  FNT_MS_LANG_KOREAN = 0x0412,
  FNT_MS_LANG_FRENCH = 0x040c,
  FNT_MS_LANG_RUSSIAN = 0x0419,
  FNT_MS_LANG_GERMAN = 0x0407,
  FNT_MS_LANG_SPANISH = 0x0409,
  FNT_MS_LANG_HEBREW = 0x040d,
  FNT_MS_LANG_SWEDISH = 0x041D
};

enum {  // languageID for FNT_PLATFORM_ID_MAC
  FNT_MAC_LANG_ENGLISH = 0,
  FNT_MAC_LANG_JAPANESE = 11,
  FNT_MAC_LANG_ARABIC = 12,
  FNT_MAC_LANG_KOREAN = 23,
  FNT_MAC_LANG_DUTCH = 4,
  FNT_MAC_LANG_RUSSIAN = 32,
  FNT_MAC_LANG_FRENCH = 1,
  FNT_MAC_LANG_SPANISH = 6,
  FNT_MAC_LANG_GERMAN = 2,
  FNT_MAC_LANG_SWEDISH = 5,
  FNT_MAC_LANG_HEBREW = 10,
  FNT_MAC_LANG_CHINESE_SIMPLIFIED = 33,
  FNT_MAC_LANG_ITALIAN = 3,
  FNT_MAC_LANG_CHINESE_TRAD = 19
};
