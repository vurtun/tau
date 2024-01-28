#if defined(_WIN64) || defined(_WIN32)
  #define SYS_WIN
#elif defined(__CYGWIN__) && !defined(_WIN32)
  #define SYS_WIN_CYG
#elif defined(__ANDROID__)
  #define SYS_ANDROID
#elif defined(__linux__)
  #define SYS_LINUX
#elif defined(__unix__) || !defined(__APPLE__) && defined(__MACH__)
  #include <sys/param.h>
  #if defined(BSD)
    #define SYS_BSD
  #endif
#elif defined(__APPLE__) && defined(__MACH__) // Apple OSX and iOS (Darwin)
  #include <TargetConditionals.h>
  #if TARGET_IPHONE_SIMULATOR == 1
    #define SYS_IOS
  #elif TARGET_OS_IPHONE == 1
    #define SYS_IOS
  #elif TARGET_OS_MAC == 1
    #define SYS_MAC
  #endif
#elif defined(__sun) && defined(__SVR4)
    #define SYS_SOLARIS
#else
  #error Unkown operating system
#endif

/* compiler specific intrinisics */
#ifdef _MSC_VER

#define alignto(x) __declspec(align(x))
#define cpu_bit_cnt(u) __popcnt(u)
#define cpu_bit_cnt64(u) __popcnt64(u)

static inline int
cpu_bit_ffs32(unsigned u) {
  unsigned long idx = 0;
  unsigned char ret = _BitScanForward(&idx, u);
  if (ret == 0) {
    return 32;
  }
  return casti(idx);
}
static inline int
cpu_bit_ffs64(unsigned long long u) {
  unsigned long idx = 0;
  unsigned char ret = _BitScanForward64(&idx, u);
  if (ret == 0) {
    return 64;
  }
  return casti(idx);

}
#define lfence() _ReadBarrier()
#define sfence() _WriteBarrier()

#define atom_cmp_xchg(val, new_val, exp) _InterlockedCompareExchange64((long long volatile *)val, new_val, exp)
#define atom_xchg(val, new_val) _InterlockedExchange64((long long volatile *)val, new_val)
#define atom_add(val, add) _InterlockedExchangeAdd64((long long volatile *)val, add)
#define atom_sub(val, add) _InterlockedExchangeSub64((long long volatile *)val, add)

#define force_inline __forceinline

#else

#define alignto(x) __attribute__((aligned(x)))
#define cpu_bit_cnt(u) __builtin_popcount(u)
#define cpu_bit_cnt64(u) __builtin_popcountll(u)
#define cpu_bit_ffs32(u) __builtin_ctz(u)
#define cpu_bit_ffs64(u) __builtin_ctzll(u)

#define lfence() asm volatile("" ::: "memory")
#define sfence() asm volatile("" ::: "memory")

#define atom_cmp_xchg(val, new_val, exp) __sync_val_compare_and_swap(val, exp, new_val)
#define atom_xchg(val, new_val) __sync_lock_test_and_set(val, new_val)
#define atom_add(val, add) __sync_fetch_and_add(val, add)
#define atom_sub(val, sub) __sync_fetch_and_sub(val, sub)

#define force_inline __attribute__((always_inline))

#endif

#define sse_align alignto(16)
#define avx_align alignto(32)

/* os specific definitions */
#if defined(SYS_WIN) || defined(SYS_WIN_CYG)

#define COL_R 0
#define COL_G 8
#define COL_B 16
#define COL_A 24

#define col_rgb_hex(c) \
  (((((c) >> 16u) & 0xff) << COL_R) |\
  (((((c) >>  8u) & 0xff) << COL_G) |\
  (((((c) >>  0u) & 0xff) << COL_B) | (0xffu << COL_A))

#define col_rgba_hex(c) \
  (((((c) >> 16u) & 0xff) << COL_R) |\
  ((((c) >>  8u) & 0xff) << COL_G) |\
  ((((c) >>  0u) & 0xff) << COL_B) |\
  ((((c) >> 24u) & 0xff) << COL_A))

#elif defined(SYS_MAC) || defined(SYS_IOS)

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define COL_R 0
#define COL_G 8
#define COL_B 16
#define COL_A 24

#define col_rgb_hex(c) \
  (((((c) >> 16u) & 0xff) << COL_R) |\
   ((((c) >>  8u) & 0xff) << COL_G) |\
   ((((c) >>  0u) & 0xff) << COL_B) | (0xffu << COL_A))

#define col_rgba_hex(c) \
  (((((c) >> 16u) & 0xff) << COL_R) |\
  ((((c) >>  8u) & 0xff) << COL_G) |\
  ((((c) >>  0u) & 0xff) << COL_B) |\
  ((((c) >> 24u) & 0xff) << COL_A))

#elif defined(__EMSCRIPTEN__)

#define COL_R 0
#define COL_G 8
#define COL_B 16
#define COL_A 24

#define col_rgb_hex(c) \
  (((((c) >> 16u) & 0xff) << COL_R) |\
   ((((c) >>  8u) & 0xff) << COL_G) |\
   ((((c) >>  0u) & 0xff) << COL_B) | (0xffu << COL_A))

#define col_rgba_hex(c) \
  (((((c) >> 16u) & 0xff) << COL_R) |\
  ((((c) >>  8u) & 0xff) << COL_G) |\
  ((((c) >>  0u) & 0xff) << COL_B) |\
  ((((c) >> 24u) & 0xff) << COL_A))

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define COL_R 16
#define COL_G 8
#define COL_B 0
#define COL_A 24

#define col_rgb_hex(c) ((c) | (0xffu << COL_A))
#define col_rgba_hex(c) (c)

#endif

static const unsigned char sse_align aes_seed[16] = {
  178, 201, 95, 240, 40, 41, 143, 216,
  2, 209, 178, 114, 232, 4, 176, 188
};

enum cpu_vendor {
  CPU_UNKNOWN,
  CPU_INTEL,
  CPU_AMD,
  CPU_APPLE,
  CPU_SAMSUNG,
  CPU_EMSCRIPT
};
struct cpu_info {
  enum cpu_vendor vendor;
  unsigned has_simd_128;
  unsigned has_simd_256;

#ifdef __x86_64__
  unsigned has_fpu:1;
  unsigned has_mmx:1;
  unsigned has_sse:1;
  unsigned has_sse2:1;
  unsigned has_sse3:1;
  unsigned has_ssse3:1;
  unsigned has_sse41:1;
  unsigned has_sse42:1;
  unsigned has_aes:1;
  unsigned has_sha:1;
  unsigned has_avx:1;
  unsigned has_avx2:1;
  unsigned has_avx512_f:1;
  unsigned has_avx512_dq:1;
  unsigned has_avx512_ifma:1;
  unsigned has_avx512_pf:1;
  unsigned has_avx512_er:1;
  unsigned has_avx512_cd:1;
  unsigned has_avx512_bw:1;
  unsigned has_avx512_vl:1;
  unsigned has_avx512_vbmi:1;
  unsigned has_avx512_vbmi2:1;
  unsigned has_avx512_vnni:1;
  unsigned has_avx512_bitalg:1;
  unsigned has_avx512_vpopcntdq:1;
  unsigned has_avx512_4vnniw:1;
  unsigned has_avx512_4fmaps:1;
  unsigned has_avx512_vp2intersect:1;
#else

#endif
};

/* x86/x64 */
#ifdef __x86_64__

#define SYS_X86_64 1

#include <emmintrin.h>
#include <smmintrin.h>
#include <xmmintrin.h>
#include <immintrin.h>
#include <wmmintrin.h>

/* simd 128-bit */
#ifdef USE_SIMD_128
#define CPU_SIMD_128
#endif

/* misc */
#define yield() _mm_pause()

static force_inline void
cpu__id(unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx) {
  __asm__ volatile("cpuid" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx),
      "=d" (*edx) : "0" (*eax), "2" (*ecx));
}
static void
cpu_info(struct cpu_info *cpu) {
  unsigned eax, ebx, ecx, edx;

  /* vendor  */
  eax = 0, ecx = 0;
  cpu__id(&eax, &ebx, &ecx, &edx);
  char vendor[13] = {0};
  memcpy(vendor + 0, &ebx, sizeof(ebx));
  memcpy(vendor + 4, &edx, sizeof(edx));
  memcpy(vendor + 8, &ecx, sizeof(ecx));
  if (!strcmp(vendor, "GenuineIntel")) {
    cpu->vendor = CPU_INTEL;
  } else if(!strcmp(vendor, "AuthenticAMD")) {
    cpu->vendor = CPU_AMD;
  } else {
    cpu->vendor = CPU_UNKNOWN;
  }
  /* check features */
  eax = 1, ecx = 0;
  cpu__id(&eax, &ebx, &ecx, &edx);
  cpu->has_fpu = (edx & (1u << 0)) != 0;
  cpu->has_mmx = (edx & (1u << 23)) != 0;
  cpu->has_sse = (edx & (1u << 25)) != 0;
  cpu->has_sse2 = (ecx & (1u << 26)) != 0;
  cpu->has_sse3 = (ecx & (1u << 0)) != 0;
  cpu->has_ssse3 = (ecx & (1u << 9)) != 0;
  cpu->has_sse41 = (ecx & (1u << 19)) != 0;
  cpu->has_sse42 = (ecx & (1u << 20)) != 0;
  cpu->has_aes = (ecx & (1u << 25)) != 0;
  cpu->has_avx = (ecx & (1u << 28)) != 0;
  cpu->has_simd_128 = cpu->has_sse42 && cpu->has_sse41 && cpu->has_sse2 && cpu->has_sse;
  cpu->has_simd_256 = cpu->has_avx;

  /* check extended features */
  eax = 7, ecx = 0;
  cpu__id(&eax, &ebx, &ecx, &edx);
  cpu->has_avx2 = (ebx & (1u << 5)) != 0;
  cpu->has_sha = (ebx & (1u << 29)) != 0;
  cpu->has_avx512_f = (ebx & (1u << 16)) != 0;
  cpu->has_avx512_dq = (ebx & (1u << 17)) != 0;
  cpu->has_avx512_ifma = (ebx & (1u << 21)) != 0;
  cpu->has_avx512_pf = (ebx & (1u << 26)) != 0;
  cpu->has_avx512_er = (ebx & (1u << 27)) != 0;
  cpu->has_avx512_cd = (ebx & (1u << 28)) != 0;
  cpu->has_avx512_bw = (ebx & (1u << 30)) != 0;
  cpu->has_avx512_vl = (ebx & (1u << 31)) != 0;
  cpu->has_avx512_vbmi = (ecx & (1u << 1)) != 0;
  cpu->has_avx512_vbmi2 = (ecx & (1u << 6)) != 0;
  cpu->has_avx512_vnni = (ecx & (1u << 11)) != 0;
  cpu->has_avx512_bitalg = (ecx & (1u << 12)) != 0;
  cpu->has_avx512_vpopcntdq = (ecx & (1u << 14)) != 0;
  cpu->has_avx512_4vnniw = (edx & (1u << 2)) != 0;
  cpu->has_avx512_4fmaps = (edx & (1u << 3)) != 0;
  cpu->has_avx512_vp2intersect = (edx & (1u << 8)) != 0;
}
static force_inline float
cpu_fma(float x, float y, float z) { {
  __asm__ ("vfmaddss %3, %2, %1, %0" : "=x" (x) : "x" (x), "x" (y), "x" (z));
  return x;
}
static force_inline float
cpu_rintf(float x) { {
  __asm__ ("frndint" : "+t"(x));
  return x;
}

#define flt4                __m128
#define flt4_flt(a)         _mm_set_ps1(a)
#define flt4_str(d,r)       _mm_storeu_ps(((float*)(d)),r)
#define flt4_max(a,b)       _mm_max_ps(a,b)
#define flt4_mul(a,b)       _mm_mul_ps(a,b)
#define flt4_add(a,b)       _mm_add_ps(a,b)
#define flt4_cmp_gt(a,b)    _mm_castps_si128(_mm_cmpgt_ps(a,b))
#define flt4_and(a,b)       _mm_and_ps(a,b)
#define flt4_xor(a,b)       _mm_xor_ps(a,b)
#define flt4_zip_lo32(a,b)  _mm_unpacklo_ps(a,b)
#define flt4_zip_hi32(a,b)  _mm_unpackhi_ps(a,b)
#define flt4_zip_lo64(a,b)  _mm_castpd_ps(_mm_unpacklo_pd(_mm_castps_pd(a),_mm_castps_pd(b)))
#define flt4_zip_hi64(a,b)  _mm_castpd_ps(_mm_unpackhi_pd(_mm_castps_pd(a),_mm_castps_pd(b)))
#define flt4_int4(a)        _mm_castsi128_ps(a)
#define flt4_cmpu(a,b)      _mm_cmpunord_ps(a,b)
#define flt4_rsqrt(a)       _mm_rsqrt_ps(a)
#define flt4_rsqrt(a)       _mm_sqrt_ps(a)
#define flt4_strs(d,a)      _mm_store_ss(d,a)
#define flt4_hflt4(s)       _mm_cvtph_ps(s)

static inline float
cpu_rsqrt(float n) {
  flt4_strs(&n, flt4_rsqrt(flt4_flt(n)));
  return n;
}
static inline float
cpu_sqrt(float n) {
  flt4_strs(&n, flt4_sqrt(flt4_flt(n)));
  return n;
}

typedef struct hflt4_t {unsigned short[4];} hflt4;
static inline hflt4
hflt4_flt4(flt4 in) {
  unsigned int sse_align d[4] = {0};
  int4_str(d, _mm_cvtps_ph(s,_MM_FROUND_TO_NEAREST_INT));
  return (hflt4){d[0],d[1],d[2],d[3]};
}
static inline unsigned short
hflt(float in) {
  flt4 i = flt4_flt(in);
  union bit_castqi v = {.hflt = hflt4_flt4(i)};
  return v.u16[0];
}

#define int4                __m128i
#define int4_ld(p)          _mm_loadu_si128((const __m128i*)(const void*)p)
#define int4_set(x,y,z,w)   _mm_setr_epi32(x,y,z,w)
#define int4_str(p,i)       _mm_storeu_si128((__m128i*)(void*)p, i)
#define int4_char(i)        _mm_set1_epi8(i)
#define int4_int(i)         _mm_set1_epi32(i)
#define int4_uint(i)        _mm_set1_epi32(i)
#define int4_sll(v,i)       _mm_slli_epi32(v,i)
#define int4_srl(v,i)       _mm_srli_epi32(v,i)
#define int4_sra(v,i)       _mm_srai_epi32(v,i)
#define int4_and(a,b)       _mm_and_si128(a, b)
#define int4_andnot(a,b)    _mm_andnot_si128(a, b)
#define int4_or(a,b)        _mm_or_si128(a, b)
#define int4_add(a,b)       _mm_add_epi32(a, b)
#define int4_sub(a,b)       _mm_sub_epi32(a, b)
#define int4_mul(a,b)       _mm_mullo_epi32(a, b)
#define int4_blend(a,b,m)   _mm_blendv_epi8(a,b,m)
#define int4_flt4(a)        _mm_castps_si128(a)
#define int4_cmp_gt(a,b)    _mm_cmpgt_epi32(a,b)

/* simd 256-bit */
#ifdef USE_SIMD_256
#define CPU_SIMD_256
#endif

#define flt8                __m256
#define int8                __m256i
#define int8_ld(p)          _mm256_loadu_si256((const __m256i*)(const void*)p)
#define int8_store(p,i)     _mm256_storeu_si256((__m256i*)(void*)p, i)
#define int8_char(i)        _mm256_set1_epi8(i)
#define int8_int(i)         _mm256_set1_epi32(i)
#define int8_sll(v,i)       _mm256_slli_epi32(v,i)
#define int8_srl(v,i)       _mm256_srli_epi32(v,i)
#define int8_and(a,b)       _mm256_and_si256(a, b)
#define int8_andnot(a,b)    _mm256_andnot_si256(a, b)
#define int8_or(a,b)        _mm256_or_si256(a, b)
#define int8_add(a,b)       _mm256_add_epi32(a, b)
#define int8_sub(a,b)       _mm256_sub_epi32(a, b)
#define int8_mul(a,b)       _mm256_mullo_epi32(a, b)

/* string */
#if CPU_SIMD_256

static inline const char*
cpu_str_chr(const char *s, int n, int chr) {
  static const char unsigned ovr_msk[64] = {
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  const char *e = s + n;
  __m256i m = _mm256_set1_epi8(chr & 0xff);
  for (; s < e; s += 32) {
    int r = (int)(e - s); r = r > 32 ? 32 : r;
    __m256i o = _mm256_loadu_si256((const __m256i*)(ovr_msk + 32 - r));
    __m256i d = _mm256_loadu_si256((const __m256i*)(const void*)s);
    __m256i v = _mm256_and_si128(d, o);
    unsigned msk = _mm256_movemask_epi8(_mm256_cmpeq_epi8(v,m));
    if (msk) {
      return s + cpu_bit_ffs32(msk);
    }
  }
  return e;
}
static inline int
cpu_str_fnd(const char *s, int n, const char *needle, int k) {
  const __m256i first = _mm256_set1_epi8(needle[0]);
  const __m256i last  = _mm256_set1_epi8(needle[k-1]);
  for (size_t i = 0; i < n; i += 32) {
    const __m256i block_first = _mm256_loadu_si256((const __m256i*)(s + i));
    const __m256i block_last  = _mm256_loadu_si256((const __m256i*)(s + i + k - 1));
    const __m256i eq_first = _mm256_cmpeq_epi8(first, block_first);
    const __m256i eq_last  = _mm256_cmpeq_epi8(last, block_last);
    unsigned mask = _mm256_movemask_epi8(_mm256_and_si256(eq_first, eq_last));
    while (mask != 0) {
      int bitpos = __builtin_ctz(mask);
      if (memcmp(s + i + bitpos + 1, needle + 1, k - 2) == 0) {
          return i + bitpos;
      }
      mask = mask & (mask - 1);
    }
  }
  return n;
}
#else

static inline const char*
cpu_str_chr(const char *s, int n, int chr) {
  static const char unsigned ovr_msk[32] = {
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  const char *e = s + n;
  __m128i m = _mm_set1_epi8(chr & 0xff);
  for (; s < e; s += 16) {
    int r = (int)(e - s); r = r > 16 ? 16 : r;
    __m128i o = _mm_loadu_si128((const __m128i *)(ovr_msk + 16 - r));
    __m128i d = _mm_loadu_si128((const __m128i *)(const void*)s);
    __m128i v = _mm_and_si128(d, o);
    unsigned msk = _mm_movemask_epi8(_mm_cmpeq_epi8(v,m));
    if (msk) {
      return s + cpu_bit_ffs32(msk);
    }
  }
  return e;
}
static inline int
cpu_str_fnd(const char *s, size_t n, const char *needle, size_t k) {
  const __m128i first = _mm_set1_epi8(needle[0]);
  const __m128i last  = _mm_set1_epi8(needle[k-1]);
  for (size_t i = 0; i < n; i += 16) {
    const __m128i block_first = _mm_loadu_si128((const __m128i*)(s + i));
    const __m128i block_last  = _mm_loadu_si128((const __m128i*)(s + i + k - 1));
    const __m128i eq_first = _mm_cmpeq_epi8(first, block_first);
    const __m128i eq_last  = _mm_cmpeq_epi8(last, block_last);
    unsigned mask = _mm_movemask_epi8(_mm_and_si128(eq_first, eq_last));
    while (mask != 0) {
      int bitpos = __builtin_ctz(mask);
      if (memcmp(s + i + bitpos + 1, needle + 1, k - 2) == 0) {
          return i + bitpos;
      }
      mask = mask & (mask - 1);
    }
  }
  return n;
}
#endif

/* hash */
typedef __m128i       hkey
#define hkey_init     _mm_loadu_si128((const __m128i *)(const void*)aes_seed)
#define hkey_zero     _mm_set1_epi32(0)
#define hkey_eq(a,b)  (_mm_movemask_epi8(_mm_cmpeq_epi8(a, b)) == 0xffff)
#define hkey_int(a)   _mm_set1_epi32(cast(unsigned,a))
#define hkey32(v)     _mm_cvtsi128_si32(v);

static inline hkey
cpu_hash(const void *src, int len, hkey seed) {
  static const char unsigned msk[32] = {
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0
  };
  const unsigned char *at = src;
  hkey h = _mm_xor_si128(_mm_set1_epi32(cast(unsigned,len)), seed);
  int n = len >> 4;
  int over = len & 15;
  while (n--) {
    hash_key prv = h;
    hash_key in = _mm_loadu_si128((const __m128i *)(const void*)at);
    h = _mm_aesdec_si128(h, in);
    h = _mm_aesdec_si128(h, in);
    h = _mm_aesdec_si128(h, in);
    h = _mm_aesdec_si128(h, in);
    h = _mm_xor_si128(h, prv);
    at += 16;
  }
  if (over) {
    hkey prv = h;
    hkey m = _mm_loadu_si128((const __m128i *)(const void*)(msk + 16 - over));
    hkey in = _mm_loadu_si128(at);
    hkey pad = _mm_andnot_si128(m, _mm_set1_epi8(over));
    in = _mm_or_si128(_mm_and_si128(m, in), pad);
    h = _mm_aesdec_si128(h, in);
    h = _mm_aesdec_si128(h, in);
    h = _mm_aesdec_si128(h, in);
    h = _mm_aesdec_si128(h, in);
    h = _mm_xor_si128(h, prv);
  }
  return h;
}

#elif defined(__arm__) || defined(__aarch64__)

#include <arm_neon.h>

#define SYS_ARM 1

/* simd 128-bit */
#ifdef USE_SIMD_128
#define CPU_SIMD_128
#endif

/* misc */
#define yield() __asm__ __volatile__("isb\n")

static void
cpu_info(struct cpu_info *cpu) {
#if defined(SYS_IOS) || defined(SYS_MAC)
  cpu->vendor = CPU_APPLE;
#elif defined(SYS_ANDROID)
  cpu->vendor = CPU_SAMSUNG;
#endif
  cpu->has_simd_128 = 1;
  cpu->has_simd_256 = 1;
}
static inline float
cpu_fma(float x, float y, float z) {
  __asm__("fmadd %s0, %s1, %s2, %s3" : "=w"(x) : "w"(x), "w"(y), "w"(z));
  return x;
}
static inline float
cpu_rintf(float x) {
  __asm__("frintx %s0, %s1" : "=w"(x) : "w"(x));
  return x;
}

/* float4 */
#define flt4                float32x4_t
#define flt4_flt(a)         vdupq_n_f32(a)
#define flt4_str(d,r)       vst1q_f32((float*)d, r)
#define flt4_max(a,b)       vmaxnmq_f32(a,b)
#define flt4_mul(a,b)       vmulq_f32(a,b)
#define flt4_add(a,b)       vaddq_f32(a,b)
#define flt4_cmp_gt(a,b)    vcgtq_f32(a,b)
#define flt4_and(a,b)       vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a),vreinterpretq_u32_f32(b)))
#define flt4_xor(a,b)       vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a),vreinterpretq_u32_f32(b)))
#define flt4_zip_lo32(a,b)  vzip1q_f32(a,b)
#define flt4_zip_hi32(a,b)  vzip2q_f32(a,b)
#define flt4_zip_lo64(a,b)  vreinterpretq_f32_f64(vzip1q_f64(vreinterpretq_f64_f32(a),vreinterpretq_f64_f32(b)))
#define flt4_zip_hi64(a,b)  vreinterpretq_f32_f64(vzip2q_f64(vreinterpretq_f64_f32(a),vreinterpretq_f64_f32(b)))
#define flt4_int4(a)        vreinterpretq_f32_s32(a)
#define flt4_rsqrt(a)       vrsqrteq_f32(a)
#define flt4_sqrt(a)        vsqrtq_f32(a)
#define flt4_strs(d,a)      vst1q_lane_f32(d,a,0)
#define flt4_hflt4(a)       vcvt_f32_f16(a)

static inline flt4
flt4_cmpu(flt4 a, flt4 b) {
  uint32x4_t f32a = vceqq_f32(a, a);
  uint32x4_t f32b = vceqq_f32(b, b);
  return vreinterpretq_f32_u32(vmvnq_u32(vandq_u32(f32a, f32b)));
}
static inline float
cpu_rsqrt(float n) {
  flt4_strs(&n, flt4_rsqrt(flt4_flt(n)));
  return n;
}
static inline float
cpu_sqrt(float n) {
  flt4_strs(&n, flt4_sqrt(flt4_flt(n)));
  return n;
}

#define hflt4               float16x4_t
#define hflt4_flt4(s)       vcvt_f16_f32(s)

static inline unsigned short
hflt(float in) {
  flt4 i = flt4_flt(in);
  union {
    hflt4 hflt;
    unsigned short u16[4];
  } v = {.hflt = hflt4_flt4(i)};
  return v.u16[0];
}

#define int4                int32x4_t
#define int4_ld(p)          vld1q_s32((const int*)(const void*)p)
#define int4_str(p,i)       vst1q_s32((int*)(void*)p, i)
#define int4_char(i)        vreinterpretq_s32_s8(vdupq_n_s8(i))
#define int4_int(i)         vdupq_n_s32(i)
#define int4_uint(i)        vreinterpretq_s32_u32(vdupq_n_u32(i))
#define int4_and(a,b)       vandq_s32(a,b)
#define int4_andnot(a,b)    vbicq_s32(b,a)
#define int4_or(a,b)        vorrq_s32(a,b)
#define int4_add(a,b)       vaddq_s32(a,b)
#define int4_sub(a,b)       vsubq_s32(a,b)
#define int4_mul(a,b)       vmulq_s32(a,b)
#define int4_sll(a,i)       vshlq_s32(a, vdupq_n_s32(i))
#define int4_srl(a,i)       vshlq_s32(a, vdupq_n_s32(-i))
#define int4_sra(v,i)       vshlq_s32(v, vdupq_n_s32(-i))
#define int4_blend(a,b,msk) vbslq_s32(msk, b, a)
#define int4_flt4(a)        vreinterpretq_s32_f32(a)
#define int4_cmp_gt(a,b)    vreinterpretq_s32_u32(vcgtq_s32(a,b))

static inline int4
int4_set(int i3, int i2, int i1, int i0) {
  int sse_align v[4] = {i3, i2, i1, i0};
  return vld1q_s32(v);
}

/* string */
static inline int
chr16_tst_all_ones(uint8x16_t a) {
  unsigned long long lo = vgetq_lane_u64(vreinterpretq_u64_u8(a), 0);
  unsigned long long hi = vgetq_lane_u64(vreinterpretq_u64_u8(a), 1);
  return (lo&hi) == (unsigned long long)-1;
}
static inline int
chr16_tst_all_zero(uint8x16_t a) {
  unsigned long long lo = vgetq_lane_u64(vreinterpretq_u64_u8(a), 0);
  unsigned long long hi = vgetq_lane_u64(vreinterpretq_u64_u8(a), 1);
  return (lo|hi) == 0u;
}
static inline const char*
cpu_str_chr(const char *s, int n, int chr) {
  static const char unsigned ovr_msk[32] = {
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  const char *e = s + n;
  uint8x16_t m = vdupq_n_u8(chr & 0xff);
  for (; s < e; s += 16) {
    int r = (int)(e - s); r = r > 16 ? 16 : r;
    uint8x16_t o = vld1q_u8(ovr_msk + 16 - r);
    uint8x16_t d = vld1q_u8((const unsigned char*)s);
    uint8x16_t v = vandq_u8(d, o);
    uint8x16_t c = vceqq_u8(v, m);
    uint64x2_t p = vreinterpretq_u64_u8(c);
    uint64_t vlo = vgetq_lane_u64(p, 0);
    if (vlo) {
      return s + ((cpu_bit_ffs64(vlo)) >> 3);
    }
    uint64_t vhi = vgetq_lane_u64(p, 1);
    if (vhi) {
      return s + 8 + ((cpu_bit_ffs64(vhi)) >> 3);
    }
  }
  return e;
}
static inline int
cpu_str_fnd(const char *s, size_t n, const char *needle, size_t k) {
  assert(k > 0);
  assert(n > 0);

  const uint8x16_t first = vdupq_n_u8(needle[0]);
  const uint8x16_t last  = vdupq_n_u8(needle[k - 1]);
  const unsigned char *ptr = (unsigned char*)s;
  for (size_t i = 0; i < n; i += 16) {
    const uint8x16_t blk_first = vld1q_u8(ptr + i);
    const uint8x16_t blk_last  = vld1q_u8(ptr + i + k - 1);
    const uint8x16_t eq_first = vceqq_u8(first, blk_first);
    const uint8x16_t eq_last  = vceqq_u8(last, blk_last);
    const uint8x16_t pred_16  = vandq_u8(eq_first, eq_last);
    unsigned long long mask = vgetq_lane_u64(vreinterpretq_u64_u8(pred_16), 0);
    if (mask) {
      for (int j=0; j < 8; j++) {
        if ((mask & 0xff) && (memcmp(s + i + j + 1, needle + 1, k - 2) == 0)) {
            return i + j;
        }
        mask >>= 8;
      }
    }
    mask = vgetq_lane_u64(vreinterpretq_u64_u8(pred_16), 1);
    if (mask) {
      for (int j=0; j < 8; j++) {
        if ((mask & 0xff) && (memcmp(s + i + j + 8 + 1, needle + 1, k - 2) == 0)) {
            return i + j + 8;
        }
        mask >>= 8;
      }
    }
  }
  return n;
}

/* hash */
typedef uint8x16_t        hkey;
#define hkey_init         vld1q_u8((const void*)aes_seed)
#define hkey_zero         vreinterpretq_u8_u32(vdupq_n_u32((0)))
#define hkey_eq(a,b)      chr16_tst_all_ones(vceqq_u8(a,b))
#define hkey32(v)         vgetq_lane_s32(vreinterpretq_s32_u8(v), 0);

static inline hkey
cpu_hash(const void *src, int len, hkey seed) {
  #define hkey__dec(a,key) (vaesimcq_u8(vaesdq_u8(a, (hkey){0})) ^ (key))
  static const char unsigned msk[32] = {
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0
  };
  const unsigned char *at = src;
  hkey h = veorq_u8(vreinterpretq_u8_u32(vdupq_n_u32((unsigned)len)), seed);
  int over = len & 15;
  int n = len >> 4;
  while (n--) {
    hkey prv = h;
    hkey in = vld1q_u8(at);
    h = hkey__dec(h, in);
    h = hkey__dec(h, in);
    h = hkey__dec(h, in);
    h = hkey__dec(h, in);
    h = veorq_u8(h, prv);
    at += 16;
  }
  if (over) {
    hkey prv = h;
    hkey in = vld1q_u8(at);
    hkey m = vld1q_u8((msk + 16 - over));
    hkey pad = vbicq_u8(m, vdupq_n_u8((unsigned char)over));
    in = vandq_u8(m, in)|pad;
    h = hkey__dec(h, in);
    h = hkey__dec(h, in);
    h = hkey__dec(h, in);
    h = hkey__dec(h, in);
    h = veorq_u8(h, prv);
  }
  #undef hkey__dec
  return h;
}

#elif defined(__EMSCRIPTEN__)

#define SYS_EMSCRIPT 1

#include <wasm_simd128.h>

/* simd 128-bit */
typedef struct hflt4_t {unsigned short n[4];} hflt4;

/* misc */
#define yield()

static void
cpu_info(struct cpu_info *cpu) {
  cpu->vendor = CPU_EMSCRIPT;
  cpu->has_simd_128 = 0;
  cpu->has_simd_256 = 0;
}
static inline float
cpu_fma(float x, float y, float z) {
  return x * y + z;
}
static inline float
cpu_rintf(float x) {
  static const float toint = 1/FLT_EPSILON;
  union {float f; uint32_t i;} u = {x};
  int e = u.i>>23 & 0xff;
  int s = u.i>>31;
  float y;
  if (e >= 0x7f+23) {
    return x;
  }
  if (s) {
    y = x - toint + toint;
  } else {
    y = x + toint - toint;
  }
  if (y == 0) {
    return s ? -0.0f : 0.0f;
  }
  return y;
}

static inline float
cpu_rsqrt(float x) {
  /* Exact bits: 13.71 */
  /* δ+max = 7.459289×10^−5 */
  /* δ−max = −7.450387×10^−5 */
  union { float f; int i; } v = {x};
  int k = v.i & 0x00800000;
  float y = 0.0f;
  if (k != 0) {
    v.i = 0x5ed9e91f - (v.i >> 1);
    y = v.f;
    y = 2.33124256f*y*cpu_fma(-x,y*y, 1.0749737f);
  } else {
    v.i = 0x5f19e8fc - (v.i >> 1);
    y = v.f;
    y = 0.824218631f*y*cpu_fma(-x, y*y, 2.1499474f);
  }
  return y;
}
static inline float
cpu_sqrt(float x) {
  /* Exact bits: 13.71 */
  /* δ+max = 7.450372×10^−5 */
  /* δ−max = −7.451108×10^−5 */
  union { float f; int i; } v = {x};
  int k = v.i & 0x00800000;
  float y = 0.0f, c;
  if (k != 0) {
    v.i = 0x5ed9e893 - (v.i >> 1);
    y = v.f;
    c = x*y;
    y = 2.33130789f*c*cpu_fma(y, -c, 1.07495356f);
  } else {
    v.i = 0x5f19e8fd - (v.i >> 1);
    y = v.f;
    c = x*y;
    y = 0.82421863f*c*cpu_fma(y, -c, 2.1499474f);
  }
  return y;
}
static inline unsigned short
hflt(float in) {
  union flt {
    float f;
    unsigned u;
  };
  union flt f = {.f = in};
  union flt f32infty = { 255 << 23 };
  union flt f16max   = { (127 + 16) << 23 };
  union flt denorm_magic = { ((127 - 15) + (23 - 10) + 1) << 23 };
  unsigned sign_mask = 0x80000000u;
  unsigned short o = 0u;
  unsigned sign = f.u & sign_mask;
  f.u ^= sign;
  // NOTE all the integer compares in this function can be safely
  // compiled into signed compares since all operands are below
  // 0x80000000. Important if you want fast straight SSE2 code
  // (since there's no unsigned PCMPGTD).
  if (f.u >= f16max.u) { // result is Inf or NaN (all exponent bits set)
      o = (f.u > f32infty.u) ? 0x7e00 : 0x7c00; // NaN->qNaN and Inf->Inf
  } else { // (De)normalized number or zero
    if (f.u < (113u << 23u)) { // resulting FP16 is subnormal or zero
      // use a magic value to align our 10 mantissa bits at the bottom of
      // the float. as long as FP addition is round-to-nearest-even this just works.
      f.f += denorm_magic.f;
      // and one integer subtract of the bias later, we have our final float!
      o = (unsigned short)(f.u - denorm_magic.u);
    } else {
      unsigned mant_odd = (f.u >> 13u) & 1u; // resulting mantissa is odd
      // update exponent, rounding bias part 1
      f.u += ((15 - 127) << 23) + 0xfff; // rounding bias part 2
      f.u += mant_odd;
      o = (unsigned short)(f.u >> 13u); // take the bits!
    }
  }
  o |= sign >> 16;
  return o;
}
static inline hflt4
hflt4_flt4(float *in) {
  hflt4 ret;
  ret.n[0] = hflt(in[0]);
  ret.n[1] = hflt(in[1]);
  ret.n[2] = hflt(in[2]);
  ret.n[3] = hflt(in[3]);
  return ret;
}

/* string */
static const char*
cpu_str_chr(const char *str, int n, int chr) {
  const char *s = str;
  const char *e = str + n;

  int c = chr & 0xFF;
  unsigned long long m4 = (c << 24)|(c << 16)|(c << 8)|c;
  unsigned long long m = m4 << 32LLU | m4;
  for (;s < e;) {
    while ((((uintptr_t)s) & 7) && s < e) {
    chk1: if (s[0] == c) return s;
    chk2: if (s[0] == 0) return s;
      ++s;
    }
    for (;s < e; s += 8) {
      unsigned long long v = *(unsigned long long*)s;
      unsigned long long c = (~v) & 0x8080808080808080LLU;
      if (((v ^ m) - 0x0101010101010101) & c) goto chk1;
      if ((v - 0x0101010101010101) & c) goto chk2;
    }
  }
  return e;
}
static inline int
cpu_str_fnd(const char *hay, int hay_len, const char *needle, int needle_len) {
  #define CPU_STR_FND_LIMIT 8
  static const char unsigned ovr_msk[16] = {
    255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0
  };
  unsigned long long ndl = *((unsigned long long*)needle);
  unsigned long long ovr = *((unsigned long long*)(ovr_msk + 8 - (needle_len & 7)));
  for (int i = 0; i + needle_len <= hay_len; i++) {
    unsigned long long txt = *((unsigned long long*)(hay + i));
    if (((ndl ^ txt) & ovr) == 0) {
      return i;
    }
  }
  return hay_len;
}

/* hash */
typedef struct hash_key_emsc {
  union {
    unsigned long long u[2];
    unsigned int i[4];
  };
} hkey;
#define hkey_init     (hkey){.u = {15604736489670298034LLU, 13596372671434772738LLU}}
#define hkey_zero     (hkey){0}
#define hkey_eq(a,b)  (memcmp(&a,&b,sizeof(hkey)) == 0)
#define hkey32(v)     v.i[0];

static inline unsigned long long
cpu__hash_rotl64(unsigned long long x, long long r) {
  return (x << r) | (x >> (64 - r));
}
static inline unsigned long long
cpu__hash_fmix64(unsigned long long k) {
  k ^= k >> 33;
  k *= 0xff51afd7ed558ccdllu;
  k ^= k >> 33;
  k *= 0xc4ceb9fe1a85ec53llu;
  k ^= k >> 33;
  return k;
}
static hkey
cpu_hash(const void *src, int len, hkey seed) {
  const unsigned char *data = (const unsigned char*)src;
  unsigned long long h1 = seed.u[0];
  unsigned long long h2 = seed.u[1];
  unsigned long long c1 = 0x87c37b91114253d5LLU;
  unsigned long long c2 = 0x4cf5ad432745937fLLU;

  const int nblocks = len / 16;
  const unsigned long long * blocks = (const unsigned long long *)(data);
  for (int i = 0; i < nblocks; i++) {
    unsigned long long k1 = blocks[i*2+0];
    unsigned long long k2 = blocks[i*2+1];
    k1 *= c1; k1  = cpu__hash_rotl64(k1,31); k1 *= c2; h1 ^= k1;
    h1 = cpu__hash_rotl64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;
    k2 *= c2; k2  = cpu__hash_rotl64(k2,33); k2 *= c1; h2 ^= k2;
    h2 = cpu__hash_rotl64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
  }
  unsigned long long k1 = 0, k2 = 0;
  const unsigned char* tail = (const unsigned char*)(data + nblocks*16);
  switch(len & 15)
  {
  case 15: k2 ^= (unsigned long long)(tail[14]) << 48;
  case 14: k2 ^= (unsigned long long)(tail[13]) << 40;
  case 13: k2 ^= (unsigned long long)(tail[12]) << 32;
  case 12: k2 ^= (unsigned long long)(tail[11]) << 24;
  case 11: k2 ^= (unsigned long long)(tail[10]) << 16;
  case 10: k2 ^= (unsigned long long)(tail[ 9]) << 8;
  case  9: k2 ^= (unsigned long long)(tail[ 8]) << 0;
           k2 *= c2; k2  = cpu__hash_rotl64(k2,33); k2 *= c1; h2 ^= k2;

  case  8: k1 ^= (unsigned long long)(tail[ 7]) << 56;
  case  7: k1 ^= (unsigned long long)(tail[ 6]) << 48;
  case  6: k1 ^= (unsigned long long)(tail[ 5]) << 40;
  case  5: k1 ^= (unsigned long long)(tail[ 4]) << 32;
  case  4: k1 ^= (unsigned long long)(tail[ 3]) << 24;
  case  3: k1 ^= (unsigned long long)(tail[ 2]) << 16;
  case  2: k1 ^= (unsigned long long)(tail[ 1]) << 8;
  case  1: k1 ^= (unsigned long long)(tail[ 0]) << 0;
           k1 *= c1; k1  = cpu__hash_rotl64(k1,31); k1 *= c2; h1 ^= k1;
  };

  h1 ^= len; h2 ^= len;
  h1 += h2; h2 += h1;
  h1 = cpu__hash_fmix64(h1);
  h2 = cpu__hash_fmix64(h2);
  h1 += h2;
  h2 += h1;

  hkey ret;
  ret.u[0] = h1;
  ret.u[1] = h2;
  return ret;
}

#endif

