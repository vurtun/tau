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

#define lfence() _ReadBarrier()
#define sfence() _WriteBarrier()

#define atom_cmp_xchg(val, new_val, exp) _InterlockedCompareExchange64((long long volatile *)val, new_val, exp)
#define atom_xchg(val, new_val) _InterlockedExchange64((long long volatile *)val, new_val)
#define atom_add(val, add) _InterlockedExchangeAdd64((long long volatile *)val, add)
#define atom_sub(val, add) _InterlockedExchangeSub64((long long volatile *)val, add)

#else

#define lfence() asm volatile("" ::: "memory")
#define sfence() asm volatile("" ::: "memory")

#define atom_cmp_xchg(val, new_val, exp) __sync_val_compare_and_swap(val, exp, new_val)
#define atom_xchg(val, new_val) __sync_lock_test_and_set(val, new_val)
#define atom_add(val, add) __sync_fetch_and_add(val, add)
#define atom_sub(val, sub) __sync_fetch_and_sub(val, sub)

#endif

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
#include <immintrin.h>
#include <wmmintrin.h>

/* simd 128-bit */
#ifdef USE_SIMD_128
#define CPU_SIMD_128
#endif

define int4 __m128i
#define flt4 __m128

#define int4_ld(p) _mm_loadu_si128((const __m128i*)(void*)p)
#define int4_str(p,i) _mm_storeu_si128((__m128i*)(void*)p, i)
#define int4_char(p,c) _mm_set1_epi8(i)
#define int4_int(p,i) _mm_set1_epi32(i)
#define int4_sll(v,i) _mm_slli_si32(v,i)
#define int4_srl(v,i) _mm_srli_si32(v,i)
#define int4_and(a,b) _mm_and_si128(a, b)
#define int4_andnot(a,b) _mm_andnot_si128(a, b)
#define int4_or(a,b) _mm_or_si128(a, b)
#define int4_add(a,b) _mm_add_epi32(a, b)
#define int4_sub(a,b) _mm_sub_epi32(a, b)
#define int4_mul(a,b) _mm_mullo_epi32(a, b)

/* simd 256-bit */
#ifdef USE_SIMD_256
#define CPU_SIMD_256
#endif

#define int8 __m256i
#define flt8 __m256

#define int8_load(p) _mm256_loadu_si256((const __m256i*)(void*)p)
#define int8_store(p,i) _mm256_storeu_si256((__m256i*)(void*)p, i)
#define int8_char(p,c) _mm256_set1_epi8(i)
#define int8_int(p,i) _mm256_set1_epi32(i)
#define int8_sll(v,i) _mm256_slli_si32(v,i)
#define int8_srl(v,i) _mm256_srli_si32(v,i)
#define int8_and(a,b) _mm256_and_si256(a, b)
#define int8_andnot(a,b) _mm256_andnot_si256(a, b)
#define int8_or(a,b) _mm256_or_si128(a, b)
#define int8_add(a,b) _mm256_add_epi32(a, b)
#define int8_sub(a,b) _mm256_sub_epi32(a, b)
#define int8_mul(a,b) _mm256_mullo_epi32(a, b)

/* aes */
#define aes128 __m128i
#define aes128_load(p) _mm_loadu_si128((const __m128i *)(void*)p)
#define aes128_dec(a,key) _mm_aesdec_si128(a, key)
#define aes128_and(a,b) _mm_and_si128(a, b)
#define aes128_eq(a, b) (_mm_movemask_epi8(_mm_cmpeq_epi8(a, b)) == 0xffff)

/* misc */
#define yield() _mm_pause()

static void
cpu__id(unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx) {
  __asm__ volatile("cpuid" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx),
      "=d" (*edx) : "0" (*eax), "2" (*ecx));
}
static void
cpu_info(struct sys_cpu_info *cpu) {
  unsigned eax, ebx, ecx, edx;

  /* vendor  */
  eax = 0, ecx = 0;
  cpu__id(&eax, &ebx, &ecx, &edx);
  char vendor[13] = {0};
  memcpy(vendor + 0, &ebx, sizeof(ebx));
  memcpy(vendor + 4, &edx, sizeof(edx));
  memcpy(vendor + 8, &ecx, sizeof(ecx));
  if (!strcmp(vendor, "GenuineIntel")) {
    cpu->vendor = SYS_CPU_INTEL;
  } else if(!strcmp(vendor, "AuthenticAMD")) {
    cpu->vendor = SYS_CPU_AMD;
  } else {
    cpu->vendor = SYS_CPU_UNKNOWN;
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

#define int4 int32x4_t
#define flt4 float32x4_t

#define int4_ld(p) vld1q_s32((const int*)(const void*)p)
#define int4_str(p,i) vst1q_s32((int*)(void*)p, i)
#define int4_char(i) vreinterpretq_s32_s8(vdupq_n_s8(i))
#define int4_int(i) vdupq_n_s32(i)
#define int4_and(a,b) vandq_s32(a,b)
#define int4_andnot(a,b) vbicq_s32(b,a)
#define int4_or(a,b) vorrq_s32(a,b)
#define int4_add(a,b) vaddq_s32(a,b)
#define int4_sub(a,b) vsubq_s32(a,b)
#define int4_mul(a,b) vmulq_s32(a,b)
#define int4_sll(a,i) vshlq_s32(a, vdupq_n_s32(i))
#define int4_srl(a,i) vshlq_s32(a, vdupq_n_s32(-i))

/* simd 256-bit */
#ifdef USE_SIMD_256
#define CPU_SIMD_256
#endif

#define int8 int
#define flt8 float

#define int8_ld(p) 0
#define int8_str(p,i) 0
#define int8_char(i) 0
#define int8_int(i) 0
#define int8_sll(v,i) 0
#define int8_srl(v,i) 0
#define int8_and(a,b) 0
#define int8_andnot(a,b) 0
#define int8_or(a,b) 0
#define int8_add(a,b) 0
#define int8_sub(a,b) 0
#define int8_mul(a,b) 0

/* aes */
#define aes128 uint8x16_t
#define aes128_load(p) vld1q_u8(p)
#define aes128_dec(a,key) (vaesimcq_u8(vaesdq_u8(a, (aes128){0})) ^ (key))
#define aes128_and(a,b) (vandq_u8(a, b))
#define aes128_eq(a, b) aes128__eq((a), (b))

static inline int
aes128__eq(aes128 a, aes128 b) {
  static const uint8x16_t pows = {
    1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128};
  uint8x16_t in = vceqq_u8(a, b);
  uint64x2_t msk = vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vandq_u8(in, pows))));

  unsigned short out = 0;
  vst1q_lane_u8((unsigned char*)&out + 0, vreinterpretq_u8_u64(msk), 0);
  vst1q_lane_u8((unsigned char*)&out + 1, vreinterpretq_u8_u64(msk), 8);
  return out == 0xFFFF;
}
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

