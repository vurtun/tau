#ifndef IMG_INCLUDE_IMAGE_H
#define IMG_INCLUDE_IMAGE_H

// DOCUMENTATION
//
// Limitations:
//    - no 12-bit-per-channel JPEG
//    - no JPEGs with arithmetic coding
//    - GIF always returns *comp=4
//
// Basic usage (see HDR discussion below for HDR usage):
//    int x,y,n;
//    unsigned char *data = img_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ...
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... replace '0' with '1'..'4' to force that many components per pixel
//    // ... but 'n' will always be the number that it would have been if you
//    said 0 img_image_free(data)
//
// Standard parameters:
//    int *x                 -- outputs image width in pixels
//    int *y                 -- outputs image height in pixels
//    int *channels_in_file  -- outputs # of image components in image file
//    int desired_channels   -- if non-zero, # of image components requested in
//    result
//
// The return value from an image loader is an 'unsigned char *' which points
// to the pixel data, or NULL on an allocation failure or if the image is
// corrupt or invalid. The pixel data consists of *y scanlines of *x pixels,
// with each pixel consisting of N interleaved 8-bit components; the first
// pixel pointed to is top-left-most in the image. There is no padding between
// image scanlines or between pixels, regardless of format. The number of
// components N is 'desired_channels' if desired_channels is non-zero, or
// *channels_in_file otherwise. If desired_channels is non-zero,
// *channels_in_file has the number of components that _would_ have been
// output otherwise. E.g. if you set desired_channels to 4, you will always
// get RGBA output, but you can check *channels_in_file to see if it's trivially
// opaque because e.g. there were only 3 channels in the source image.
//
// An output image with N components has the following components interleaved
// in this order in each pixel:
//
//     N=#comp     components
//       1           grey
//       2           grey, alpha
//       3           red, green, blue
//       4           red, green, blue, alpha
//
// If image loading fails for any reason, the return value will be NULL,
// and *x, *y, *channels_in_file will be unchanged. The function
// img_failure_reason() can be queried for an extremely brief, end-user
// unfriendly explanation of why the load failed. Define IMG_NO_FAILURE_STRINGS
// to avoid compiling these strings at all, and IMG_FAILURE_USERMSG to get
// slightly more user-friendly ones.
//
// Paletted PNG, BMP, GIF, and PIC images are automatically depalettized.
//
// ===========================================================================
//
// UNICODE:
//
//   If compiling for Windows and you wish to use Unicode filenames, compile
//   with
//       #define IMG_WINDOWS_UTF8
//   and pass utf8-encoded filenames. Call img_convert_wchar_to_utf8 to convert
//   Windows wchar_t filenames to utf8.
//
// ===========================================================================
//
// Philosophy
//
// stb libraries are designed with the following priorities:
//
//    1. easy to use
//    2. easy to maintain
//    3. good performance
//
// Sometimes I let "good performance" creep up in priority over "easy to
// maintain", and for best performance I may provide less-easy-to-use APIs that
// give higher performance, in addition to the easy-to-use ones. Nevertheless,
// it's important to keep in mind that from the standpoint of you, a client of
// this library, all you care about is #1 and #3, and stb libraries DO NOT
// emphasize #3 above all.
//
// Some secondary priorities arise directly from the first two, some of which
// provide more explicit reasons why performance can't be emphasized.
//
//    - Portable ("ease of use")
//    - Small source code footprint ("easy to maintain")
//    - No dependencies ("ease of use")
//
// ===========================================================================
//
// I/O callbacks
//
// I/O callbacks allow you to read from arbitrary sources, like packaged
// files or some other source. Data read from callbacks are processed
// through a small internal buffer (currently 128 bytes) to try to reduce
// overhead.
//
// The three functions you must define are "read" (reads some bytes of data),
// "skip" (skips some bytes of data), "eof" (reports if the stream is at the
// end).
//
// ===========================================================================
//
// SIMD support
//
// The JPEG decoder will try to automatically use SIMD kernels on x86 when
// supported by the compiler. For ARM Neon support, you must explicitly
// request it.
//
// (The old do-it-yourself SIMD API is no longer supported in the current
// code.)
//
// On x86, SSE2 will automatically be used when available based on a run-time
// test; if not, the generic C versions are used as a fall-back. On ARM targets,
// the typical path is to have separate builds for NEON and non-NEON devices
// (at least this is true for iOS and Android). Therefore, the NEON support is
// toggled by a build flag: define IMG_NEON to get NEON loops.
//
// If for some reason you do not want to use any of SIMD code, or if
// you have issues compiling it, you can disable it entirely by
// defining IMG_NO_SIMD.
//
// ===========================================================================
//
// HDR image support   (disable by defining IMG_NO_HDR)
//
// stb_image supports loading HDR images in general, and currently the Radiance
// .HDR file format specifically. You can still load any file through the
// existing interface; if you attempt to load an HDR file, it will be
// automatically remapped to LDR, assuming gamma 2.2 and an arbitrary scale
// factor defaulting to 1; both of these constants can be reconfigured through
// this interface:
//
//     img_hdr_to_ldr_gamma(2.2f);
//     img_hdr_to_ldr_scale(1.0f);
//
// (note, do not use _inverse_ constants; img_image will invert them
// appropriately).
//
// Additionally, there is a new, parallel interface for loading files as
// (linear) floats to preserve the full dynamic range:
//
//    float *data = img_loadf(filename, &x, &y, &n, 0);
//
// If you load LDR images through this interface, those images will
// be promoted to floating point values, run through the inverse of
// constants corresponding to the above:
//
//     img_ldr_to_hdr_scale(1.0f);
//     img_ldr_to_hdr_gamma(2.2f);
//
// Finally, given a filename (or an open file or memory block--see header
// file for details) containing image data, you can query for the "most
// appropriate" interface to use (that is, whether the image is HDR or
// not), using:
//
//     img_is_hdr(char *filename);
//
// ===========================================================================
//
// iPhone PNG support:
//
// By default we convert iphone-formatted PNGs back to RGB, even though
// they are internally encoded differently. You can disable this conversion
// by calling img_convert_iphone_png_to_rgb(0), in which case
// you will always just get the native iphone "format" through (which
// is BGR stored in RGB).
//
// Call img_set_unpremultiply_on_load(1) as well to force a divide per
// pixel to remove any premultiplied alpha *only* if the image file explicitly
// says there's premultiplied data (currently only happens in iPhone images,
// and only if iPhone convert-to-rgb processing is on).
//
// ===========================================================================
//
// ADDITIONAL CONFIGURATION
//
//  - You can suppress implementation of any of the decoders to reduce
//    your code footprint by #defining one or more of the following
//    symbols before creating the implementation.
//
//        IMG_NO_JPEG
//        IMG_NO_PNG
//        IMG_NO_BMP
//        IMG_NO_PSD
//        IMG_NO_TGA
//        IMG_NO_GIF
//        IMG_NO_HDR
//        IMG_NO_PIC
//        IMG_NO_PNM   (.ppm and .pgm)
//
//  - You can request *only* certain decoders and suppress all other ones
//    (this will be more forward-compatible, as addition of new decoders
//    doesn't require you to disable them explicitly):
//
//        IMG_ONLY_JPEG
//        IMG_ONLY_PNG
//        IMG_ONLY_BMP
//        IMG_ONLY_PSD
//        IMG_ONLY_TGA
//        IMG_ONLY_GIF
//        IMG_ONLY_HDR
//        IMG_ONLY_PIC
//        IMG_ONLY_PNM   (.ppm and .pgm)
//
//   - If you use IMG_NO_PNG (or _ONLY_ without PNG), and you still
//     want the zlib decoder to be available, #define IMG_SUPPORT_ZLIB
//
//  - If you define IMG_MAX_DIMENSIONS, stb_image will reject images greater
//    than that size (in either width or height) without further processing.
//    This is to let programs in the wild set an upper bound to prevent
//    denial-of-service attacks on untrusted data, as one could generate a
//    valid image of gigantic dimensions and force stb_image to allocate a
//    huge block of memory and spend disproportionate time decoding it. By
//    default this is set to (1 << 24), which is 16777216, but that's still
//    very big.

#ifndef IMG_NO_STDIO
#include <stdio.h>
#endif  // IMG_NO_STDIO

#define IMG_VERSION 1

enum {
  IMG_default = 0,  // only used for desired_channels
  IMG_grey = 1,
  IMG_grey_alpha = 2,
  IMG_rgb = 3,
  IMG_rgb_alpha = 4
};

#include <stdlib.h>
typedef unsigned char img_uc;
typedef unsigned short img_us;

#ifdef __cplusplus
extern "C" {
#endif

#define IMG_NO_FAILURE_STRINGS
#define IMG_NO_TGA
#define IMG_NO_PSD
#define IMG_NO_HDR
#define IMG_NO_PIC
#define IMG_NO_PNM


//////////////////////////////////////////////////////////////////////////////
//
// PRIMARY API - works on images of any type
//

//
// load image by filename, open file, or memory buffer
//

typedef struct {
  int (*read)(void *user, char *data,
              int size);  // fill 'data' with 'size' bytes.  return number of
                          // bytes actually read
  void (*skip)(void *user, int n);  // skip the next 'n' bytes, or 'unget' the
                                    // last -n bytes if negative
  int (*eof)(void *user);  // returns nonzero if we are at end of file/data
} img_io_callbacks;

////////////////////////////////////
//
// 8-bits-per-channel interface
//

static img_uc *
img_load_from_memory(img_uc const *buffer, int len, int *x, int *y,
                     int *channels_in_file, int desired_channels);
static img_uc *
img_load_from_callbacks(img_io_callbacks const *clbk, void *user, int *x,
                        int *y, int *channels_in_file, int desired_channels);

#ifndef IMG_NO_STDIO
static img_uc *
img_load(char const *filename, int *x, int *y, int *channels_in_file,
         int desired_channels);
static img_uc *
img_load_from_file(FILE *f, int *x, int *y, int *channels_in_file,
                   int desired_channels);
// for img_load_from_file, file pointer is left pointing immediately after image
#endif

#ifndef IMG_NO_GIF
static img_uc *
img_load_gif_from_memory(img_uc const *buffer, int len, int **delays, int *x,
                         int *y, int *z, int *comp, int req_comp);
#endif

#ifdef IMG_WINDOWS_UTF8
static int
img_convert_wchar_to_utf8(char *buffer, size_t bufferlen, const wchar_t *input);
#endif

////////////////////////////////////
//
// 16-bits-per-channel interface
//

static img_us *
img_load_16_from_memory(img_uc const *buffer, int len, int *x, int *y,
                        int *channels_in_file, int desired_channels);
static img_us *
img_load_16_from_callbacks(img_io_callbacks const *clbk, void *user, int *x,
                           int *y, int *channels_in_file, int desired_channels);

#ifndef IMG_NO_STDIO
static img_us *
img_load_16(char const *filename, int *x, int *y, int *channels_in_file,
            int desired_channels);
static img_us *
img_load_from_file_16(FILE *f, int *x, int *y, int *channels_in_file,
                      int desired_channels);
#endif

////////////////////////////////////
//
// float-per-channel interface
//
#ifndef IMG_NO_LINEAR
static float *
img_loadf_from_memory(img_uc const *buffer, int len, int *x, int *y,
                      int *channels_in_file, int desired_channels);
static float *
img_loadf_from_callbacks(img_io_callbacks const *clbk, void *user, int *x,
                         int *y, int *channels_in_file, int desired_channels);

#ifndef IMG_NO_STDIO
static float *
img_loadf(char const *filename, int *x, int *y, int *channels_in_file,
          int desired_channels);
static float *
img_loadf_from_file(FILE *f, int *x, int *y, int *channels_in_file,
                    int desired_channels);
#endif
#endif

#ifndef IMG_NO_HDR
   static void   img_hdr_to_ldr_gamma(float gamma;
   static void   img_hdr_to_ldr_scale(float scale);
#endif  // IMG_NO_HDR

#ifndef IMG_NO_LINEAR
   static void   img_ldr_to_hdr_gamma(float gamma);
   static void   img_ldr_to_hdr_scale(float scale);
#endif  // IMG_NO_LINEAR

// img_is_hdr is always defined, but always returns false if IMG_NO_HDR
static int    img_is_hdr_from_callbacks(img_io_callbacks const *clbk, void *user);
static int    img_is_hdr_from_memory(img_uc const *buffer, int len);
#ifndef IMG_NO_STDIO
static int      img_is_hdr          (char const *filename);
static int      img_is_hdr_from_file(FILE *f);
#endif  // IMG_NO_STDIO


// get a VERY brief reason for failure
// on most compilers (and ALL modern mainstream compilers) this is threadsafe
static const char *img_failure_reason  (void);

// free the loaded image -- this is just free()
static void     img_image_free      (void *retval_from_img_load);

// get image dimensions & components without fully decoding
static int      img_info_from_memory(img_uc const *buffer, int len, int *x, int *y, int *comp);
static int      img_info_from_callbacks(img_io_callbacks const *clbk, void *user, int *x, int *y, int *comp);
static int      img_is_16_bit_from_memory(img_uc const *buffer, int len);
static int      img_is_16_bit_from_callbacks(img_io_callbacks const *clbk, void *user);

#ifndef IMG_NO_STDIO
static int      img_info               (char const *filename,     int *x, int *y, int *comp);
static int      img_info_from_file     (FILE *f,                  int *x, int *y, int *comp);
static int      img_is_16_bit          (char const *filename);
static int      img_is_16_bit_from_file(FILE *f);
#endif



// for image formats that explicitly notate that they have premultiplied alpha,
// we just return the colors as stored in the file. set this flag to force
// unpremultiplication. results are undefined if the unpremultiply overflow.
static void img_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply);

// indicate whether we should process iphone images back to canonical format,
// or just pass them through "as-is"
static void img_convert_iphone_png_to_rgb(int flag_true_if_should_convert);

// flip the image vertically, so the first pixel in the output array is the bottom left
static void img_set_flip_vertically_on_load(int flag_true_if_should_flip);

// as above, but only applies to images loaded on the thread that calls the function
// this function is only available if your compiler supports thread-local variables;
// calling it will fail to link if your compiler doesn't
static void img_set_flip_vertically_on_load_thread(int flag_true_if_should_flip);

// ZLIB client - used by PNG, available for other purposes

static char *img_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen);
static char *img_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header);
static char *img_zlib_decode_malloc(const char *buffer, int len, int *outlen);
static int   img_zlib_decode_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

static char *img_zlib_decode_noheader_malloc(const char *buffer, int len, int *outlen);
static int   img_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

#ifdef __cplusplus
   }
#endif

#endif  // IMG_INCLUDE_STB_IMAGE_H
