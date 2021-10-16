#ifdef offsetof
#undef offsetof
#endif
#ifdef min
#undef min
#undef max
#endif

// clang-format off
#define unused(a) ((void)a)
#define cast(t, p) ((t)(p))
#define szof(a) ((int)sizeof(a))
#define cntof(a) ((int)(sizeof(a) / sizeof((a)[0])))

#define xglue(x, y) x##y
#define glue(x, y) xglue(x, y)
#define tostring(s) #s
#define stringify(s) tostring(s)

#define flag(n) ((1u) << (n))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define clamp(a, v, b) (max(min(b, v), a))
#define zero(d, sz) memset(d, 0, (size_t)(sz))
#define offsetof(st, m) ((int)((uintptr_t) & (((st *)0)->m)))
#define containerof(ptr, type, member)                           \
  (type *)((void *)((char *)(1 ? (ptr) : &((type *)0)->member) - offsetof(type, member)))
#define div_round_up(n, d) (((n) + (d)-1) / (d))
#define ispow2(x) (((x) != 0u) && ((x) & ((x) - 1)) == 0u)
#define between(x, a, b) ((a) <= (x) && (x) < (b))
#define inbox(px, py, x, y, w, h)(between(px, x, x + w) && between(py, y, y + h))

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
// clang-format on

struct str {
  const char *str;
  const char *end;
  int len;
};
struct mem_blk {
  unsigned long long flags;
  int size;
  unsigned char *base;
  int used;
  struct mem_blk *prv;
};
struct scope {
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
#define dyn(T) T*

