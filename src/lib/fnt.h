// private structure
typedef struct
{
   unsigned char *data;
   int cursor;
   int size;
} fnt__buf;

//////////////////////////////////////////////////////////////////////////////
//
// TEXTURE BAKING API
//
// If you use this API, you only have to call two functions ever.
//

typedef struct
{
   unsigned short x0,y0,x1,y1; // coordinates of bbox in bitmap
   float xoff,yoff,xadvance;
} fnt_bakedchar;

static int fnt_BakeFontBitmap(const unsigned char *data, int offset,  // font location (use offset=0 for plain .ttf)
                                float pixel_height,                     // height of font in pixels
                                unsigned char *pixels, int pw, int ph,  // bitmap to be filled in
                                int first_char, int num_chars,          // characters to bake
                                fnt_bakedchar *chardata);             // you allocate this, it's num_chars long
// if return is positive, the first unused row of the bitmap
// if return is negative, returns the negative of the number of characters that fit
// if return is 0, no characters fit and no rows were used
// This uses a very crappy packing.

typedef struct
{
   float x0,y0,s0,t0; // top-left
   float x1,y1,s1,t1; // bottom-right
} fnt_aligned_quad;

static void fnt_GetBakedQuad(const fnt_bakedchar *chardata, int pw, int ph,  // same data as above
                               int char_index,             // character to display
                               float *xpos, float *ypos,   // pointers to current position in screen pixel space
                               fnt_aligned_quad *q,      // output: quad to draw
                               int opengl_fillrule);       // true if opengl fill rule; false if DX9 or earlier
// Call GetBakedQuad with char_index = 'character - first_char', and it
// creates the quad you need to draw and advances the current position.
//
// The coordinate system used assumes y increases downwards.
//
// Characters will extend both above and below the current position;
// see discussion of "BASELINE" above.
//
// It's inefficient; you might want to c&p it and optimize it.

static void fnt_GetScaledFontVMetrics(const unsigned char *fontdata, int index, float size, float *ascent, float *descent, float *lineGap);
// Query the font vertical metrics without having to create a font first.


//////////////////////////////////////////////////////////////////////////////
//
// NEW TEXTURE BAKING API
//
// This provides options for packing multiple fonts into one atlas, not
// perfectly but better than nothing.

typedef struct
{
   unsigned short x0,y0,x1,y1; // coordinates of bbox in bitmap
   float xoff,yoff,xadvance;
   float xoff2,yoff2;
} fnt_packedchar;

typedef struct fnt_pack_context fnt_pack_context;
typedef struct fnt_fontinfo fnt_fontinfo;

static int  fnt_PackBegin(fnt_pack_context *spc, unsigned char *pixels, int width, int height, int stride_in_bytes, int padding, void *alloc_context);
// Initializes a packing context stored in the passed-in fnt_pack_context.
// Future calls using this context will pack characters into the bitmap passed
// in here: a 1-channel bitmap that is width * height. stride_in_bytes is
// the distance from one row to the next (or 0 to mean they are packed tightly
// together). "padding" is the amount of padding to leave between each
// character (normally you want '1' for bitmaps you'll use as textures with
// bilinear filtering).
//
// Returns 0 on failure, 1 on success.

static void fnt_PackEnd  (fnt_pack_context *spc);
// Cleans up the packing context and frees all memory.

#define FNT_POINT_SIZE(x)   (-(x))

static int  fnt_PackFontRange(fnt_pack_context *spc, const unsigned char *fontdata, int font_index, float font_size,
                                int first_unicode_char_in_range, int num_chars_in_range, fnt_packedchar *chardata_for_range);
// Creates character bitmaps from the font_index'th font found in fontdata (use
// font_index=0 if you don't know what that is). It creates num_chars_in_range
// bitmaps for characters with unicode values starting at first_unicode_char_in_range
// and increasing. Data for how to render them is stored in chardata_for_range;
// pass these to fnt_GetPackedQuad to get back renderable quads.
//
// font_size is the full height of the character from ascender to descender,
// as computed by fnt_ScaleForPixelHeight. To use a point size as computed
// by fnt_ScaleForMappingEmToPixels, wrap the point size in FNT_POINT_SIZE()
// and pass that result as 'font_size':
//       ...,                  20 , ... // font max minus min y is 20 pixels tall
//       ..., FNT_POINT_SIZE(20), ... // 'M' is 20 pixels tall

typedef struct
{
   float font_size;
   int first_unicode_codepoint_in_range;  // if non-zero, then the chars are continuous, and this is the first codepoint
   int *array_of_unicode_codepoints;       // if non-zero, then this is an array of unicode codepoints
   int num_chars;
   fnt_packedchar *chardata_for_range; // output
   unsigned char h_oversample, v_oversample; // don't set these, they're used internally
} fnt_pack_range;

static int  fnt_PackFontRanges(fnt_pack_context *spc, const unsigned char *fontdata, int font_index, fnt_pack_range *ranges, int num_ranges);
// Creates character bitmaps from multiple ranges of characters stored in
// ranges. This will usually create a better-packed bitmap than multiple
// calls to fnt_PackFontRange. Note that you can call this multiple
// times within a single PackBegin/PackEnd.

static void fnt_PackSetOversampling(fnt_pack_context *spc, unsigned int h_oversample, unsigned int v_oversample);
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

static void fnt_PackSetSkipMissingCodepoints(fnt_pack_context *spc, int skip);
// If skip != 0, this tells stb_truetype to skip any codepoints for which
// there is no corresponding glyph. If skip=0, which is the default, then
// codepoints without a glyph recived the font's "missing character" glyph,
// typically an empty box by convention.

static void fnt_GetPackedQuad(const fnt_packedchar *chardata, int pw, int ph,  // same data as above
                               int char_index,             // character to display
                               float *xpos, float *ypos,   // pointers to current position in screen pixel space
                               fnt_aligned_quad *q,      // output: quad to draw
                               int align_to_integer);

static int  fnt_PackFontRangesGatherRects(fnt_pack_context *spc, const fnt_fontinfo *info, fnt_pack_range *ranges, int num_ranges, rpq_rect *rects);
static void fnt_PackFontRangesPackRects(fnt_pack_context *spc, rpq_rect *rects, int num_rects);
static int  fnt_PackFontRangesRenderIntoRects(fnt_pack_context *spc, const fnt_fontinfo *info, fnt_pack_range *ranges, int num_ranges, rpq_rect *rects);
// Calling these functions in sequence is roughly equivalent to calling
// fnt_PackFontRanges(). If you more control over the packing of multiple
// fonts, or if you want to pack custom data into a font texture, take a look
// at the source to of fnt_PackFontRanges() and create a custom version
// using these functions, e.g. call GatherRects multiple times,
// building up a single array of rects, then call PackRects once,
// then call RenderIntoRects repeatedly. This may result in a
// better packing than calling PackFontRanges multiple times
// (or it may not).

// this is an opaque structure that you shouldn't mess with which holds
// all the context needed from PackBegin to PackEnd.
struct fnt_pack_context {
   void *user_allocator_context;
   void *pack_info;
   int   width;
   int   height;
   int   stride_in_bytes;
   int   padding;
   int   skip_missing;
   unsigned int   h_oversample, v_oversample;
   unsigned char *pixels;
   void  *nodes;
};

//////////////////////////////////////////////////////////////////////////////
//
// FONT LOADING
//
//

static int fnt_GetNumberOfFonts(const unsigned char *data);
// This function will determine the number of fonts in a font file.  TrueType
// collection (.ttc) files may contain multiple fonts, while TrueType font
// (.ttf) files only contain one font. The number of fonts can be used for
// indexing with the previous function where the index is between zero and one
// less than the total fonts. If an error occurs, -1 is returned.

static int fnt_GetFontOffsetForIndex(const unsigned char *data, int index);
// Each .ttf/.ttc file may have more than one font. Each font has a sequential
// index number starting from 0. Call this function to get the font offset for
// a given index; it returns -1 if the index is out of range. A regular .ttf
// file will only define one font and it always be at offset 0, so it will
// return '0' for index 0, and -1 for all other indices.

// The following structure is defined publicly so you can declare one on
// the stack or as a global or etc, but you should treat it as opaque.
struct fnt_fontinfo
{
   void           * userdata;
   unsigned char  * data;              // pointer to .ttf file
   int              fontstart;         // offset of start of font

   int numGlyphs;                     // number of glyphs, needed for range checking

   int loca,head,glyf,hhea,hmtx,kern,gpos,svg; // table locations as offset from start of .ttf
   int index_map;                     // a cmap mapping for our chosen character encoding
   int indexToLocFormat;              // format needed to map from glyph index to glyph

   fnt__buf cff;                    // cff font data
   fnt__buf charstrings;            // the charstring index
   fnt__buf gsubrs;                 // global charstring subroutines index
   fnt__buf subrs;                  // private charstring subroutines index
   fnt__buf fontdicts;              // array of font dicts
   fnt__buf fdselect;               // map from glyph to fontdict
};

static int fnt_InitFont(fnt_fontinfo *info, const unsigned char *data, int offset);
// Given an offset into the file that defines a font, this function builds
// the necessary cached info for the rest of the system. You must allocate
// the fnt_fontinfo yourself, and fnt_InitFont will fill it out. You don't
// need to do anything special to free it, because the contents are pure
// value data with no additional data structures. Returns 0 on failure.


//////////////////////////////////////////////////////////////////////////////
//
// CHARACTER TO GLYPH-INDEX CONVERSIOn

static int fnt_FindGlyphIndex(const fnt_fontinfo *info, int unicode_codepoint);
// If you're going to perform multiple operations on the same character
// and you want a speed-up, call this function with the character you're
// going to process, then use glyph-based functions instead of the
// codepoint-based functions.
// Returns 0 if the character codepoint is not defined in the font.


//////////////////////////////////////////////////////////////////////////////
//
// CHARACTER PROPERTIES
//

static float fnt_ScaleForPixelHeight(const fnt_fontinfo *info, float pixels);
// computes a scale factor to produce a font whose "height" is 'pixels' tall.
// Height is measured as the distance from the highest ascender to the lowest
// descender; in other words, it's equivalent to calling fnt_GetFontVMetrics
// and computing:
//       scale = pixels / (ascent - descent)
// so if you prefer to measure height by the ascent only, use a similar calculation.

static float fnt_ScaleForMappingEmToPixels(const fnt_fontinfo *info, float pixels);
// computes a scale factor to produce a font whose EM size is mapped to
// 'pixels' tall. This is probably what traditional APIs compute, but
// I'm not positive.

static void fnt_GetFontVMetrics(const fnt_fontinfo *info, int *ascent, int *descent, int *lineGap);
// ascent is the coordinate above the baseline the font extends; descent
// is the coordinate below the baseline the font extends (i.e. it is typically negative)
// lineGap is the spacing between one row's descent and the next row's ascent...
// so you should advance the vertical position by "*ascent - *descent + *lineGap"
//   these are expressed in unscaled coordinates, so you must multiply by
//   the scale factor for a given size

static int  fnt_GetFontVMetricsOS2(const fnt_fontinfo *info, int *typoAscent, int *typoDescent, int *typoLineGap);
// analogous to GetFontVMetrics, but returns the "typographic" values from the OS/2
// table (specific to MS/Windows TTF files).
//
// Returns 1 on success (table present), 0 on failure.

static void fnt_GetFontBoundingBox(const fnt_fontinfo *info, int *x0, int *y0, int *x1, int *y1);
// the bounding box around all possible characters

static void fnt_GetCodepointHMetrics(const fnt_fontinfo *info, int codepoint, int *advanceWidth, int *leftSideBearing);
// leftSideBearing is the offset from the current horizontal position to the left edge of the character
// advanceWidth is the offset from the current horizontal position to the next horizontal position
//   these are expressed in unscaled coordinates

static int  fnt_GetCodepointKernAdvance(const fnt_fontinfo *info, int ch1, int ch2);
// an additional amount to add to the 'advance' value between ch1 and ch2

static int fnt_GetCodepointBox(const fnt_fontinfo *info, int codepoint, int *x0, int *y0, int *x1, int *y1);
// Gets the bounding box of the visible part of the glyph, in unscaled coordinates

static void fnt_GetGlyphHMetrics(const fnt_fontinfo *info, int glyph_index, int *advanceWidth, int *leftSideBearing);
static int  fnt_GetGlyphKernAdvance(const fnt_fontinfo *info, int glyph1, int glyph2);
static int  fnt_GetGlyphBox(const fnt_fontinfo *info, int glyph_index, int *x0, int *y0, int *x1, int *y1);
// as above, but takes one or more glyph indices for greater efficiency

typedef struct fnt_kerningentry
{
   int glyph1; // use fnt_FindGlyphIndex
   int glyph2;
   int advance;
} fnt_kerningentry;

static int  fnt_GetKerningTableLength(const fnt_fontinfo *info);
static int  fnt_GetKerningTable(const fnt_fontinfo *info, fnt_kerningentry* table, int table_length);
// Retrieves a complete list of all of the kerning pairs provided by the font
// fnt_GetKerningTable never writes more than table_length entries and returns how many entries it did write.
// The table will be sorted by (a.glyph1 == b.glyph1)?(a.glyph2 < b.glyph2):(a.glyph1 < b.glyph1)

//////////////////////////////////////////////////////////////////////////////
//
// GLYPH SHAPES (you probably don't need these, but they have to go before
// the bitmaps for C declaration-order reasons)
//

#ifndef FNT_vmove // you can predefine these to use different values (but why?)
   enum {
      FNT_vmove=1,
      FNT_vline,
      FNT_vcurve,
      FNT_vcubic
   };
#endif

#ifndef fnt_vertex // you can predefine this to use different values
                   // (we share this with other code at RAD)
   #define fnt_vertex_type short // can't use fnt_int16 because that's not visible in the header file
   typedef struct
   {
      fnt_vertex_type x,y,cx,cy,cx1,cy1;
      unsigned char type,padding;
   } fnt_vertex;
#endif

static int fnt_IsGlyphEmpty(const fnt_fontinfo *info, int glyph_index);
// returns non-zero if nothing is drawn for this glyph

static int fnt_GetCodepointShape(const fnt_fontinfo *info, int unicode_codepoint, fnt_vertex **vertices);
static int fnt_GetGlyphShape(const fnt_fontinfo *info, int glyph_index, fnt_vertex **vertices);
// returns # of vertices and fills *vertices with the pointer to them
//   these are expressed in "unscaled" coordinates
//
// The shape is a series of contours. Each one starts with
// a FNT_moveto, then consists of a series of mixed
// FNT_lineto and FNT_curveto segments. A lineto
// draws a line from previous endpoint to its x,y; a curveto
// draws a quadratic bezier from previous endpoint to
// its x,y, using cx,cy as the bezier control point.

static void fnt_FreeShape(const fnt_fontinfo *info, fnt_vertex *vertices);
// frees the data allocated above

static unsigned char *fnt_FindSVGDoc(const fnt_fontinfo *info, int gl);
static int fnt_GetCodepointSVG(const fnt_fontinfo *info, int unicode_codepoint, const char **svg);
static int fnt_GetGlyphSVG(const fnt_fontinfo *info, int gl, const char **svg);
// fills svg with the character's SVG data.
// returns data size or 0 if SVG not found.

//////////////////////////////////////////////////////////////////////////////
//
// BITMAP RENDERING
//

static void fnt_FreeBitmap(unsigned char *bitmap, void *userdata);
// frees the bitmap allocated below

static unsigned char *fnt_GetCodepointBitmap(const fnt_fontinfo *info, float scale_x, float scale_y, int codepoint, int *width, int *height, int *xoff, int *yoff);
// allocates a large-enough single-channel 8bpp bitmap and renders the
// specified character/glyph at the specified scale into it, with
// antialiasing. 0 is no coverage (transparent), 255 is fully covered (opaque).
// *width & *height are filled out with the width & height of the bitmap,
// which is stored left-to-right, top-to-bottom.
//
// xoff/yoff are the offset it pixel space from the glyph origin to the top-left of the bitmap

static unsigned char *fnt_GetCodepointBitmapSubpixel(const fnt_fontinfo *info, float scale_x, float scale_y, float shift_x, float shift_y, int codepoint, int *width, int *height, int *xoff, int *yoff);
// the same as fnt_GetCodepoitnBitmap, but you can specify a subpixel
// shift for the character

static void fnt_MakeCodepointBitmap(const fnt_fontinfo *info, unsigned char *output, int out_w, int out_h, int out_stride, float scale_x, float scale_y, int codepoint);
// the same as fnt_GetCodepointBitmap, but you pass in storage for the bitmap
// in the form of 'output', with row spacing of 'out_stride' bytes. the bitmap
// is clipped to out_w/out_h bytes. Call fnt_GetCodepointBitmapBox to get the
// width and height and positioning info for it first.

static void fnt_MakeCodepointBitmapSubpixel(const fnt_fontinfo *info, unsigned char *output, int out_w, int out_h, int out_stride, float scale_x, float scale_y, float shift_x, float shift_y, int codepoint);
// same as fnt_MakeCodepointBitmap, but you can specify a subpixel
// shift for the character

static void fnt_MakeCodepointBitmapSubpixelPrefilter(const fnt_fontinfo *info, unsigned char *output, int out_w, int out_h, int out_stride, float scale_x, float scale_y, float shift_x, float shift_y, int oversample_x, int oversample_y, float *sub_x, float *sub_y, int codepoint);
// same as fnt_MakeCodepointBitmapSubpixel, but prefiltering
// is performed (see fnt_PackSetOversampling)

static void fnt_GetCodepointBitmapBox(const fnt_fontinfo *font, int codepoint, float scale_x, float scale_y, int *ix0, int *iy0, int *ix1, int *iy1);
// get the bbox of the bitmap centered around the glyph origin; so the
// bitmap width is ix1-ix0, height is iy1-iy0, and location to place
// the bitmap top left is (leftSideBearing*scale,iy0).
// (Note that the bitmap uses y-increases-down, but the shape uses
// y-increases-up, so CodepointBitmapBox and CodepointBox are inverted.)

static void fnt_GetCodepointBitmapBoxSubpixel(const fnt_fontinfo *font, int codepoint, float scale_x, float scale_y, float shift_x, float shift_y, int *ix0, int *iy0, int *ix1, int *iy1);
// same as fnt_GetCodepointBitmapBox, but you can specify a subpixel
// shift for the character

// the following functions are equivalent to the above functions, but operate
// on glyph indices instead of Unicode codepoints (for efficiency)
static unsigned char *fnt_GetGlyphBitmap(const fnt_fontinfo *info, float scale_x, float scale_y, int glyph, int *width, int *height, int *xoff, int *yoff);
static unsigned char *fnt_GetGlyphBitmapSubpixel(const fnt_fontinfo *info, float scale_x, float scale_y, float shift_x, float shift_y, int glyph, int *width, int *height, int *xoff, int *yoff);
static void fnt_MakeGlyphBitmap(const fnt_fontinfo *info, unsigned char *output, int out_w, int out_h, int out_stride, float scale_x, float scale_y, int glyph);
static void fnt_MakeGlyphBitmapSubpixel(const fnt_fontinfo *info, unsigned char *output, int out_w, int out_h, int out_stride, float scale_x, float scale_y, float shift_x, float shift_y, int glyph);
static void fnt_MakeGlyphBitmapSubpixelPrefilter(const fnt_fontinfo *info, unsigned char *output, int out_w, int out_h, int out_stride, float scale_x, float scale_y, float shift_x, float shift_y, int oversample_x, int oversample_y, float *sub_x, float *sub_y, int glyph);
static void fnt_GetGlyphBitmapBox(const fnt_fontinfo *font, int glyph, float scale_x, float scale_y, int *ix0, int *iy0, int *ix1, int *iy1);
static void fnt_GetGlyphBitmapBoxSubpixel(const fnt_fontinfo *font, int glyph, float scale_x, float scale_y,float shift_x, float shift_y, int *ix0, int *iy0, int *ix1, int *iy1);


// @TODO: don't expose this structure
typedef struct
{
   int w,h,stride;
   unsigned char *pixels;
} fnt__bitmap;

// rasterize a shape with quadratic beziers into a bitmap
static void fnt_Rasterize(fnt__bitmap *result,        // 1-channel bitmap to draw into
                               float flatness_in_pixels,     // allowable error of curve in pixels
                               fnt_vertex *vertices,       // array of vertices defining shape
                               int num_verts,                // number of vertices in above array
                               float scale_x, float scale_y, // scale applied to input vertices
                               float shift_x, float shift_y, // translation applied to input vertices
                               int x_off, int y_off,         // another translation applied to input
                               int invert,                   // if non-zero, vertically flip shape
                               void *userdata);              // context for to FNT_MALLOC

//////////////////////////////////////////////////////////////////////////////
//
// Signed Distance Function (or Field) rendering

static void fnt_FreeSDF(unsigned char *bitmap, void *userdata);
// frees the SDF bitmap allocated below

static unsigned char * fnt_GetGlyphSDF(const fnt_fontinfo *info, float scale, int glyph, int padding, unsigned char onedge_value, float pixel_dist_scale, int *width, int *height, int *xoff, int *yoff);
static unsigned char * fnt_GetCodepointSDF(const fnt_fontinfo *info, float scale, int codepoint, int padding, unsigned char onedge_value, float pixel_dist_scale, int *width, int *height, int *xoff, int *yoff);
// These functions compute a discretized SDF field for a single character, suitable for storing
// in a single-channel texture, sampling with bilinear filtering, and testing against
// larger than some threshold to produce scalable fonts.
//        info              --  the font
//        scale             --  controls the size of the resulting SDF bitmap, same as it would be creating a regular bitmap
//        glyph/codepoint   --  the character to generate the SDF for
//        padding           --  extra "pixels" around the character which are filled with the distance to the character (not 0),
//                                 which allows effects like bit outlines
//        onedge_value      --  value 0-255 to test the SDF against to reconstruct the character (i.e. the isocontour of the character)
//        pixel_dist_scale  --  what value the SDF should increase by when moving one SDF "pixel" away from the edge (on the 0..255 scale)
//                                 if positive, > onedge_value is inside; if negative, < onedge_value is inside
//        width,height      --  output height & width of the SDF bitmap (including padding)
//        xoff,yoff         --  output origin of the character
//        return value      --  a 2D array of bytes 0..255, width*height in size
//
// pixel_dist_scale & onedge_value are a scale & bias that allows you to make
// optimal use of the limited 0..255 for your application, trading off precision
// and special effects. SDF values outside the range 0..255 are clamped to 0..255.
//
// Example:
//      scale = fnt_ScaleForPixelHeight(22)
//      padding = 5
//      onedge_value = 180
//      pixel_dist_scale = 180/5.0 = 36.0
//
//      This will create an SDF bitmap in which the character is about 22 pixels
//      high but the whole bitmap is about 22+5+5=32 pixels high. To produce a filled
//      shape, sample the SDF at each pixel and fill the pixel if the SDF value
//      is greater than or equal to 180/255. (You'll actually want to antialias,
//      which is beyond the scope of this example.) Additionally, you can compute
//      offset outlines (e.g. to stroke the character border inside & outside,
//      or only outside). For example, to fill outside the character up to 3 SDF
//      pixels, you would compare against (180-36.0*3)/255 = 72/255. The above
//      choice of variables maps a range from 5 pixels outside the shape to
//      2 pixels inside the shape to 0..255; this is intended primarily for apply
//      outside effects only (the interior range is needed to allow proper
//      antialiasing of the font at *smaller* sizes)
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


static int fnt_FindMatchingFont(const unsigned char *fontdata, const char *name, int flags);
// returns the offset (not index) of the font that matches, or -1 if none
//   if you use FNT_MACSTYLE_DONTCARE, use a font name like "Arial Bold".
//   if you use any other flag, use a font name like "Arial"; this checks
//     the 'macStyle' header field; i don't know if fonts set this consistently
#define FNT_MACSTYLE_DONTCARE     0
#define FNT_MACSTYLE_BOLD         1
#define FNT_MACSTYLE_ITALIC       2
#define FNT_MACSTYLE_UNDERSCORE   4
#define FNT_MACSTYLE_NONE         8   // <= not same as 0, this makes us check the bitfield is 0

static int fnt_CompareUTF8toUTF16_bigendian(const char *s1, int len1, const char *s2, int len2);
// returns 1/0 whether the first string interpreted as utf8 is identical to
// the second string interpreted as big-endian utf16... useful for strings from next func

static const char *fnt_GetFontNameString(const fnt_fontinfo *font, int *length, int platformID, int encodingID, int languageID, int nameID);
// returns the string (which may be big-endian double byte, e.g. for unicode)
// and puts the length in bytes in *length.
//
// some of the values for the IDs are below; for more see the truetype spec:
//     http://developer.apple.com/textfonts/TTRefMan/RM06/Chap6name.html
//     http://www.microsoft.com/typography/otspec/name.htm

enum { // platformID
   FNT_PLATFORM_ID_UNICODE   =0,
   FNT_PLATFORM_ID_MAC       =1,
   FNT_PLATFORM_ID_ISO       =2,
   FNT_PLATFORM_ID_MICROSOFT =3
};

enum { // encodingID for FNT_PLATFORM_ID_UNICODE
   FNT_UNICODE_EID_UNICODE_1_0    =0,
   FNT_UNICODE_EID_UNICODE_1_1    =1,
   FNT_UNICODE_EID_ISO_10646      =2,
   FNT_UNICODE_EID_UNICODE_2_0_BMP=3,
   FNT_UNICODE_EID_UNICODE_2_0_FULL=4
};

enum { // encodingID for FNT_PLATFORM_ID_MICROSOFT
   FNT_MS_EID_SYMBOL        =0,
   FNT_MS_EID_UNICODE_BMP   =1,
   FNT_MS_EID_SHIFTJIS      =2,
   FNT_MS_EID_UNICODE_FULL  =10
};

enum { // encodingID for FNT_PLATFORM_ID_MAC; same as Script Manager codes
   FNT_MAC_EID_ROMAN        =0,   FNT_MAC_EID_ARABIC       =4,
   FNT_MAC_EID_JAPANESE     =1,   FNT_MAC_EID_HEBREW       =5,
   FNT_MAC_EID_CHINESE_TRAD =2,   FNT_MAC_EID_GREEK        =6,
   FNT_MAC_EID_KOREAN       =3,   FNT_MAC_EID_RUSSIAN      =7
};

enum { // languageID for FNT_PLATFORM_ID_MICROSOFT; same as LCID...
       // problematic because there are e.g. 16 english LCIDs and 16 arabic LCIDs
   FNT_MS_LANG_ENGLISH     =0x0409,   FNT_MS_LANG_ITALIAN     =0x0410,
   FNT_MS_LANG_CHINESE     =0x0804,   FNT_MS_LANG_JAPANESE    =0x0411,
   FNT_MS_LANG_DUTCH       =0x0413,   FNT_MS_LANG_KOREAN      =0x0412,
   FNT_MS_LANG_FRENCH      =0x040c,   FNT_MS_LANG_RUSSIAN     =0x0419,
   FNT_MS_LANG_GERMAN      =0x0407,   FNT_MS_LANG_SPANISH     =0x0409,
   FNT_MS_LANG_HEBREW      =0x040d,   FNT_MS_LANG_SWEDISH     =0x041D
};

enum { // languageID for FNT_PLATFORM_ID_MAC
   FNT_MAC_LANG_ENGLISH      =0 ,   FNT_MAC_LANG_JAPANESE     =11,
   FNT_MAC_LANG_ARABIC       =12,   FNT_MAC_LANG_KOREAN       =23,
   FNT_MAC_LANG_DUTCH        =4 ,   FNT_MAC_LANG_RUSSIAN      =32,
   FNT_MAC_LANG_FRENCH       =1 ,   FNT_MAC_LANG_SPANISH      =6 ,
   FNT_MAC_LANG_GERMAN       =2 ,   FNT_MAC_LANG_SWEDISH      =5 ,
   FNT_MAC_LANG_HEBREW       =10,   FNT_MAC_LANG_CHINESE_SIMPLIFIED =33,
   FNT_MAC_LANG_ITALIAN      =3 ,   FNT_MAC_LANG_CHINESE_TRAD =19
};

