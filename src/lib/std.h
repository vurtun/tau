#ifdef offsetof
#undef offsetof
#endif
#ifdef min
#undef min
#undef max
#endif

// clang-format off
#define unused(a) ((void)a)
#define szof(a) ((int)sizeof(a))
#define cntof(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define ptr(p,v) cast(p,cast(uintptr_t,v))

#define cast(t, p) ((t)(p))
#define castc(p) cast(char,p)
#define castb(p) cast(unsigned char,p)
#define casts(p) cast(short,p)
#define castus(p) cast(unsigned short,p)
#define casti(p) cast(int,p)
#define castu(p) cast(unsigned,p)
#define castl(p) cast(long,p)
#define castul(p) cast(unsigned long,p)
#define castll(p) cast(long long,p)
#define castull(p) cast(unsigned long long,p)
#define castf(p) cast(float,p)
#define castd(p) cast(double,p)
#define castsz(p) cast(size_t,p)
#define castss(p) cast(ssize_t,p)
#define recast(T,p) ((T)cast(void*,(p)))

#define xglue(x, y) x##y
#define glue(x, y) xglue(x, y)
#define tostring(s) #s
#define stringify(s) tostring(s)

#define flag(n) ((1u) << (n))
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min3i(a,b,c) min(a,min(b,c))
#define max3i(a,b,c) max(a,max(b,c))
#define clamp(a, v, b) (max(min(b, v), a))
#define clamp01(v) clamp(0,v,1)

#define zero(d, sz) memset(d, 0, (size_t)(sz))
#define offsetof(st, m) ((int)((uintptr_t) & (((st *)0)->m)))
#define containerof(ptr, type, member)\
  (type*)((void*)((char*)(1?(ptr):&((type*)0)->member)-offsetof(type,member)))
#define div_round_up(n, d) (((n) + (d)-1) / (d))
#define ispow2(x) (((x) != 0u) && ((x) & ((x) - 1)) == 0u)

#define twocc(s) castu((s[0] << 8u) + s[1])
#define threecc(s) castu((s[0] << 16u) + (s[1] << 8u) + s[2])
#define fourcc(s) castu((s[0] << 24u) + (s[1] << 16u) + (s[2] << 8u) + s[3])

#define between(x, a, b) ((a) <= (x) && (x) < (b))
#define inbox(px, py, x, y, w, h)(between(px, x, x + w) && between(py, y, y + h))
#define iswap(x,y) ((x) ^= (y), (y) ^= (x), (x) ^= (y))

#define alignof(t)((int)((char *)(&((struct {char c;t _h;}*)0)->_h)-(char *)0))
#define isaligned(x, mask) (!((uintptr_t)(x) & (uintptr_t)(mask - 1)))
#define type_aligned(x, t) isaligned(x, alignof(t))
#define align_mask(a) ((a)-1)
#define align_down_masked(n, m) ((n) & ~(m))
#define align_down(n, a) align_down_masked(n, align_mask(a))
#define align_up(n, a) align_down((n) + align_mask(a), (a))
#define align_down_ptr(p, a) \
  ((void *)align_down((uintptr_t)(p), (uintptr_t)(a)))
#define align_up_ptr(p, a) ((void *)align_up((uintptr_t)(p), (uintptr_t)(a)))

#define uniqid(name) glue(name, __LINE__)
#define compiler_assert(exp) \
  typedef char uniqid(_compile_assert_array)[(exp) ? 1 : -1]
#define fall_through __attribute__((fallthrough));

#define BITS_PER_BYTE CHAR_BIT
#define BITS_PER_LONG (BITS_PER_BYTE * sizeof(unsigned long))
#define bit_mask(nr) (1ull << ((unsigned long)(nr) % BITS_PER_LONG))
#define bit_word(nr) ((unsigned long)(nr) / BITS_PER_LONG)
#define bit_word_idx(nr) ((unsigned long)(nr) & (BITS_PER_LONG - 1))
#define bit_word_nxt(nr) ((unsigned long)((nr) + BITS_PER_LONG - bit_word_idx(nr)))
#define bits_to_long(nr) (int)div_round_up((unsigned long)nr, BITS_PER_LONG)

#define heap_parent(i) (((i)-1) >> 1)
#define heap_right(i) (((i)+1) << 1)
#define heap_left(i) (heap_right(i)-1)
// clang-format on

#define KB(n) (1024 * n)
#define MB(n) (1024 * KB(n))
#define GB(n) (1024 * MB(n))

#define MAX_FILE_PATH (4*1024)
#define MAX_FILE_NAME (256)

typedef int (*sort_f)(const void *a, const void *b);
struct rng {
  int lo, hi, cnt, total;
};
struct str {
  const char *ptr;
  struct rng rng;
};
struct str_fnd_tbl {
  int tbl[UCHAR_MAX+1];
};
struct mem_blk {
  unsigned long long flags;
  int size;
  unsigned char *base;
  int used;
  struct mem_blk *prv;
};
struct arena_scope {
  struct mem_blk *blk;
  int used;
};
struct arena {
  struct mem_blk *blk;
  int blksiz;
  int tmp_cnt;
};
struct lst_elm {
  struct lst_elm *prv;
  struct lst_elm *nxt;
};
struct color {
  unsigned char r, g, b, a;
};
struct guid {
  unsigned d1;
  unsigned short d2;
  unsigned short d3;
  unsigned char d4[8];
};
#define confine for
#define dyn(T) T*
#define TBL_CAP(n) ((n)+((n)>>2)+1)
#define tbl(T,N) {                    \
  unsigned long long keys[TBL_CAP(N)];\
  T vals[TBL_CAP(N)];                 \
  int cnt;                            \
}
#define str_buf(N) {    \
  int cnt;              \
  char mem[N];          \
}

