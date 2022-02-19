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

#define lfence() _ReadBarrier()
#define sfence() _WriteBarrier()

#define atom_cmp_xchg(val, new_val, exp) _InterlockedCompareExchange64((long long volatile *)val, new_val, exp)
#define atom_xchg(val, new_val) _InterlockedExchange64((long long volatile *)val, new_val)
#define atom_add(val, add) _InterlockedExchangeAdd64((long long volatile *)val, add)
#define atom_sub(val, add) _InterlockedExchangeSub64((long long volatile *)val, add)

#else

#define alignto(x) __attribute__((aligned(x)))

#define lfence() asm volatile("" ::: "memory")
#define sfence() asm volatile("" ::: "memory")

#define atom_cmp_xchg(val, new_val, exp) __sync_val_compare_and_swap(val, exp, new_val)
#define atom_xchg(val, new_val) __sync_lock_test_and_set(val, new_val)
#define atom_add(val, add) __sync_fetch_and_add(val, add)
#define atom_sub(val, sub) __sync_fetch_and_sub(val, sub)

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

enum cpu_vendor {
  CPU_UNKNOWN,
  CPU_INTEL,
  CPU_AMD,
  CPU_APPLE,
  CPU_SAMSUNG,
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

#define chr16 __m128i
#define chr16_ld(p) _mm_loadu_si128((const __m128i *)(const void*)p)
#define chr16_eq(a,b) _mm_cmpeq_epi8(a,b)
#define chr16_tst_all_ones(a) _mm_test_all_ones(a)
#define chr16_tst_all_zero(a) _mm_test_all_zero(a,_mm_set1_epi8(0xff))
#define chr16_or(a,b) _mm_or_si128(a,b)

#define flt4 __m128
#define flt4_flt(a) _mm_set_ps1(a)
#define flt4_str(d,r) _mm_storeu_ps(((float*)(d)),r)
#define flt4_max(a,b) _mm_max_ps(a,b)
#define flt4_mul(a,b) _mm_mul_ps(a,b)
#define flt4_add(a,b) _mm_add_ps(a,b)
#define flt4_cmp_gt(a,b) _mm_castps_si128(_mm_cmpgt_ps(a,b))
#define flt4_and(a,b) _mm_and_ps(a,b)
#define flt4_xor(a,b) _mm_xor_ps(a,b)
#define flt4_zip_lo32(a,b) _mm_unpacklo_ps(a,b)
#define flt4_zip_hi32(a,b) _mm_unpackhi_ps(a,b)
#define flt4_zip_lo64(a,b) _mm_castpd_ps(_mm_unpacklo_pd(_mm_castps_pd(a),_mm_castps_pd(b)))
#define flt4_zip_hi64(a,b) _mm_castpd_ps(_mm_unpackhi_pd(_mm_castps_pd(a),_mm_castps_pd(b)))
#define flt4_int4(a) _mm_castsi128_ps(a)
#define flt4_cmpu(a,b) _mm_cmpunord_ps(a,b)
#define flt4_rsqrt(a) _mm_rsqrt_ps(a)
#define flt4_strs(d,a) _mm_store_ss(d,a)

#define int4 __m128i
#define int4_ld(p) _mm_loadu_si128((const __m128i*)(const void*)p)
#define int4_set(x,y,z,w) _mm_setr_epi32(x,y,z,w)
#define int4_str(p,i) _mm_storeu_si128((__m128i*)(void*)p, i)
#define int4_char(i) _mm_set1_epi8(i)
#define int4_int(i) _mm_set1_epi32(i)
#define int4_uint(i) _mm_set1_epi32(i)
#define int4_sll(v,i) _mm_slli_epi32(v,i)
#define int4_srl(v,i) _mm_srli_epi32(v,i)
#define int4_sra(v,i) _mm_srai_epi32(v,i)
#define int4_and(a,b) _mm_and_si128(a, b)
#define int4_andnot(a,b) _mm_andnot_si128(a, b)
#define int4_or(a,b) _mm_or_si128(a, b)
#define int4_add(a,b) _mm_add_epi32(a, b)
#define int4_sub(a,b) _mm_sub_epi32(a, b)
#define int4_mul(a,b) _mm_mullo_epi32(a, b)
#define int4_blend(a,b,m) _mm_blendv_epi8(a,b,m)
#define int4_flt4(a) _mm_castps_si128(a)
#define int4_cmp_gt(a,b) _mm_cmpgt_epi32(a,b)

/* simd 256-bit */
#ifdef USE_SIMD_256
#define CPU_SIMD_256
#endif

#define int8 __m256i
#define flt8 __m256

#define int8_ld(p) _mm256_loadu_si256((const __m256i*)(const void*)p)
#define int8_store(p,i) _mm256_storeu_si256((__m256i*)(void*)p, i)
#define int8_char(i) _mm256_set1_epi8(i)
#define int8_int(i) _mm256_set1_epi32(i)
#define int8_sll(v,i) _mm256_slli_epi32(v,i)
#define int8_srl(v,i) _mm256_srli_epi32(v,i)
#define int8_and(a,b) _mm256_and_si256(a, b)
#define int8_andnot(a,b) _mm256_andnot_si256(a, b)
#define int8_or(a,b) _mm256_or_si256(a, b)
#define int8_add(a,b) _mm256_add_epi32(a, b)
#define int8_sub(a,b) _mm256_sub_epi32(a, b)
#define int8_mul(a,b) _mm256_mullo_epi32(a, b)

/* aes */
#define aes128 __m128i
#define aes128_load(p) _mm_loadu_si128((const __m128i *)(const void*)p)
#define aes128_dec(a,key) _mm_aesdec_si128(a, key)
#define aes128_and(a,b) _mm_and_si128(a, b)
#define aes128_xor(a,b) _mm_xor_si128(a, b)
#define aes128_eq(a, b) (_mm_movemask_epi8(_mm_cmpeq_epi8(a, b)) == 0xffff)
#define aes128_int(a) _mm_set1_epi32(a)
#define aes128_zero() _mm_set1_epi32(0)
#define aes128_lane_int(v) _mm_cvtsi128_si32(v);

/* misc */
#define yield() _mm_pause()

static void
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

#elif defined(__arm__) || defined(__aarch64__)

#include <arm_neon.h>

#define SYS_ARM 1

/* simd 128-bit */
#ifdef USE_SIMD_128
#define CPU_SIMD_128
#endif

#define chr16 uint8x16_t
#define chr16_ld(p) vld1q_u8((const unsigned char*)(const void*)p)
#define chr16_eq(a,b) vceqq_u8(a,b)
#define chr16_or(a,b) vorrq_u8(a,b)

static inline int
chr16_tst_all_ones(chr16 a) {
  unsigned long long lo = vgetq_lane_u64(vreinterpretq_u64_u8(a), 0);
  unsigned long long hi = vgetq_lane_u64(vreinterpretq_u64_u8(a), 1);
  return (lo & hi) == (unsigned long long)-1;
}
static inline int
chr16_tst_all_zero(chr16 a) {
  unsigned long long lo = vgetq_lane_u64(vreinterpretq_u64_u8(a), 0);
  unsigned long long hi = vgetq_lane_u64(vreinterpretq_u64_u8(a), 1);
  return (lo | hi) == 0u;
}

#define flt4 float32x4_t
#define flt4_flt(a) vdupq_n_f32(a)
#define flt4_str(d,r) vst1q_f32((float*)d, r)
#define flt4_max(a,b) vmaxnmq_f32(a,b)
#define flt4_mul(a,b) vmulq_f32(a,b)
#define flt4_add(a,b) vaddq_f32(a,b)
#define flt4_cmp_gt(a,b) vcgtq_f32(a,b)
#define flt4_and(a,b) vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a),vreinterpretq_u32_f32(b)))
#define flt4_xor(a,b) vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a),vreinterpretq_u32_f32(b)))
#define flt4_zip_lo32(a,b) vzip1q_f32(a,b)
#define flt4_zip_hi32(a,b) vzip2q_f32(a,b)
#define flt4_zip_lo64(a,b) vreinterpretq_f32_f64(vzip1q_f64(vreinterpretq_f64_f32(a),vreinterpretq_f64_f32(b)))
#define flt4_zip_hi64(a,b) vreinterpretq_f32_f64(vzip2q_f64(vreinterpretq_f64_f32(a),vreinterpretq_f64_f32(b)))
#define flt4_int4(a) vreinterpretq_f32_s32(a)
#define flt4_rsqrt(a) vrsqrteq_f32(a)
#define flt4_strs(d,a) vst1q_lane_f32(d,a,0)

static inline flt4
flt4_cmpu(flt4 a, flt4 b) {
  uint32x4_t f32a = vceqq_f32(a, a);
  uint32x4_t f32b = vceqq_f32(b, b);
  return vreinterpretq_f32_u32(vmvnq_u32(vandq_u32(f32a, f32b)));
}

#define int4 int32x4_t
#define int4_ld(p) vld1q_s32((const int*)(const void*)p)
#define int4_str(p,i) vst1q_s32((int*)(void*)p, i)
#define int4_char(i) vreinterpretq_s32_s8(vdupq_n_s8(i))
#define int4_int(i) vdupq_n_s32(i)
#define int4_uint(i) vreinterpretq_s32_u32(vdupq_n_u32(i))
#define int4_and(a,b) vandq_s32(a,b)
#define int4_andnot(a,b) vbicq_s32(b,a)
#define int4_or(a,b) vorrq_s32(a,b)
#define int4_add(a,b) vaddq_s32(a,b)
#define int4_sub(a,b) vsubq_s32(a,b)
#define int4_mul(a,b) vmulq_s32(a,b)
#define int4_sll(a,i) vshlq_s32(a, vdupq_n_s32(i))
#define int4_srl(a,i) vshlq_s32(a, vdupq_n_s32(-i))
#define int4_sra(v,i) vshlq_s32(v, vdupq_n_s32(-i))
#define int4_blend(a,b,msk) vbslq_s32(msk, b, a)
#define int4_flt4(a) vreinterpretq_s32_f32(a)
#define int4_cmp_gt(a,b) vreinterpretq_s32_u32(vcgtq_s32(a,b))

static inline int4
int4_set(int i3, int i2, int i1, int i0) {
  int sse_align v[4] = {i3, i2, i1, i0};
  return vld1q_s32(v);
}

/* simd 256-bit */
#ifdef USE_SIMD_256
#define CPU_SIMD_256
#endif

#define int8 int
#define flt8 float

#define int8_ld(p)
#define int8_str(p,i)
#define int8_char(i)
#define int8_int(i)
#define int8_sll(v,i)
#define int8_srl(v,i)
#define int8_and(a,b)
#define int8_andnot(a,b)
#define int8_or(a,b)
#define int8_add(a,b)
#define int8_sub(a,b)
#define int8_mul(a,b)

/* aes */
#define aes128 uint8x16_t
#define aes128_load(p) vld1q_u8(p)
#define aes128_dec(a,key) (vaesimcq_u8(vaesdq_u8(a, (aes128){0})) ^ (key))
#define aes128_and(a,b) (vandq_u8(a, b))
#define aes128_xor(a,b) veorq_u8(a, b)
#define aes128_int(a) vreinterpretq_u8_u32(vdupq_n_u32(a))
#define aes128_zero() aes128_int(0)
#define aes128_lane_int(v) vgetq_lane_s32(vreinterpretq_s32_u8(v), 0);
#define aes128_eq(a,b) chr16_tst_all_ones(vceqq_u8(a,b))

/* misc */
#define yield() __asm__ __volatile__("isb\n");

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
#endif

