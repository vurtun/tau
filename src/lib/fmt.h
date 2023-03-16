/*
API:
====
int str_sprintf( char * buf, char const * fmt, ... )
int str_snprintf( char * buf, int count, char const * fmt, ... )
  Convert an arg list into a buffer.  fmt_snprintf always returns
  a zero-terminated string (unlike regular snprintf).

int str_vsprintf( char * buf, char const * fmt, va_list va )
int str_vsnprintf( char * buf, int count, char const * fmt, va_list va )
  Convert a va_list arg list into a buffer.  fmt_vsnprintf always returns
  a zero-terminated string (unlike regular snprintf).

int str_vsprintfcb( FMT_SPRINTFCB * callback, void * user, char * buf, char
const * fmt, va_list va ) typedef char * FMT_SPRINTFCB( char const * buf, void *
user, int len ); Convert into a buffer, calling back every FMT_MIN chars. Your
callback can then copy the chars out, print them or whatever. This function is
actually the workhorse for everything else. The buffer you pass in must hold at
least FMT_MIN characters.
    // you return the next buffer to use or 0 to stop converting

void fmt_set_sep( char comma, char period )
  Set the comma and period characters to use.

FLOATS/DOUBLES:
===============
This code uses a internal float->ascii conversion method that uses
doubles with error correction (double-doubles, for ~105 bits of
precision).  This conversion is round-trip perfect - that is, an atof
of the values output here will give you the bit-exact double back.

One difference is that our insignificant digits will be different than
with MSVC or GCC (but they don't match each other either).  We also
don't attempt to find the minimum length matching float (pre-MSVC15
doesn't either).

If you don't need float or doubles at all, define FMT_NOFLOAT
and you'll save 4K of code space.

64-BIT INTS:
============
This library also supports 64-bit integers and you can use MSVC style or
GCC style indicators (%I64d or %lld).  It supports the C99 specifiers
for size_t and ptr_diff_t (%jd %zd) as well.

EXTRAS:
=======
Like some GCCs, for integers and floats, you can use a ' (single quote)
specifier and commas will be inserted on the thousands: "%'d" on 12345
would print 12,345.

For integers and floats, you can use a "$" specifier and the number
will be converted to float and then divided to get kilo, mega, giga or
tera and then printed, so "%$d" 1000 is "1.0 k", "%$.2d" 2536000 is
"2.53 M", etc. For byte values, use two $:s, like "%$$d" to turn
2536000 to "2.42 Mi". If you prefer JEDEC suffixes to SI ones, use three
$:s: "%$$$d" -> "2.42 M". To remove the space between the number and the
suffix, add "_" specifier: "%_$d" -> "2.53M".

In addition to octal and hexadecimal conversions, you can print
integers in binary: "%b" for 256 would print 100.

PERFORMANCE vs MSVC 2008 32-/64-bit (GCC is even slower than MSVC):
===================================================================
"%d" across all 32-bit ints (4.8x/4.0x faster than 32-/64-bit MSVC)
"%24d" across all 32-bit ints (4.5x/4.2x faster)
"%x" across all 32-bit ints (4.5x/3.8x faster)
"%08x" across all 32-bit ints (4.3x/3.8x faster)
"%f" across e-10 to e+10 floats (7.3x/6.0x faster)
"%e" across e-10 to e+10 floats (8.1x/6.0x faster)
"%g" across e-10 to e+10 floats (10.0x/7.1x faster)
"%f" for values near e-300 (7.9x/6.5x faster)
"%f" for values near e+300 (10.0x/9.1x faster)
"%e" for values near e-300 (10.1x/7.0x faster)
"%e" for values near e+300 (9.2x/6.0x faster)
"%.320f" for values near e-300 (12.6x/11.2x faster)
"%a" for random values (8.6x/4.3x faster)
"%I64d" for 64-bits with 32-bit values (4.8x/3.4x faster)
"%I64d" for 64-bits > 32-bit values (4.9x/5.5x faster)
"%s%s%s" for 64 char strings (7.1x/7.3x faster)
"...512 char string..." ( 35.0x/32.5x faster!)
*/
#if defined(__clang__)
#if defined(__has_feature) && defined(__has_attribute)
#if __has_feature(address_sanitizer)
#if __has_attribute(__no_sanitize__)
#define FMT__ASAN __attribute__((__no_sanitize__("address")))
#elif __has_attribute(__no_sanitize_address__)
#define FMT__ASAN __attribute__((__no_sanitize_address__))
#elif __has_attribute(__no_address_safety_analysis__)
#define FMT__ASAN __attribute__((__no_address_safety_analysis__))
#endif
#endif
#endif
#elif __GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
#if __SANITIZE_ADDRESS__
#define FMT__ASAN __attribute__((__no_sanitize_address__))
#endif
#endif

#ifndef FMT__ASAN
#define FMT__ASAN
#endif

#define FMT__PUBLICDEC static
#define FMT__PUBLICDEF static FMT__ASAN

#ifndef FMT_MIN
#define FMT_MIN 512  // how many characters per callback
#endif

typedef char* FMT_SPRINTFCB(const char *buf, void *user, int len);
static int fmtv(char *buf, char const *fmt, va_list va);
static int fmtvsn(char *buf, int count, char const *fmt, va_list va);
static int fmts(char *buf, char const *fmt, ...);
static int fmtsn(char *buf, int count, char const *fmt, ...);
static int fmtvscb(FMT_SPRINTFCB *callback, void *user, char *buf, char const *fmt, va_list va);
static void fmt_set_sep(char comma, char period);

