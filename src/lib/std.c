#if __has_include(<stdckdint.h>)
  #include <stdckdint.h>
#else
  #define ckd_add(R, A, B) __builtin_add_overflow ((A), (B), (R))
  #define ckd_sub(R, A, B) __builtin_sub_overflow ((A), (B), (R))
  #define ckd_mul(R, A, B) __builtin_mul_overflow ((A), (B), (R))
#endif

static purist inline int
chk_add(int a, int b, unsigned *ovf) {
  int ret = 0;
  *ovf |= ckd_add(&ret, a, b);
  return ret;
}
static purist inline int
chk_sub(int a, int b, unsigned *ovf) {
  int ret = 0;
  *ovf |= ckd_sub(&ret, a, b);
  return ret;
}
static purist inline int
chk_mul(int a, int b, unsigned *ovf) {
  int ret = 0;
  *ovf |= ckd_mul(&ret, a, b);
  return ret;
}
static purist inline int
npow2(int input) {
  unsigned int val = castu(input);
  val--;
  val |= val >> 1;
  val |= val >> 2;
  val |= val >> 4;
  val |= val >> 8;
  val |= val >> 16;
  val++;
  return casti(val);
}
/* ---------------------------------------------------------------------------
 *                                Foreach
 * ---------------------------------------------------------------------------
 */
#define forever while(1)
#define loopr(i,r) (int i = (r).lo; i != (r).hi; i += 1)
#define looprn(i,r,n) (int i = (r).lo; i != min((r).lo + n, (r).hi); i += 1)
#define loopi(i,j,r) (int i = (r).lo, j = 0; i != (r).hi; i += 1, ++j)
#define loop(i,n) (int i = 0; i < (n); ++i)
#define loopn(i,n,m) (int i = 0; i < min(n,m); ++i)

/* ---------------------------------------------------------------------------
 *                                Memory
 * ---------------------------------------------------------------------------
 */
static inline void
mcpy(void* restrict dst, void const *restrict src, int cnt) {
  assert(cnt >= 0);
  assert(dst != 0 || !cnt);
  assert(src != 0 || !cnt);

  unsigned char *restrict dst8 = dst;
  const unsigned char *restrict src8 = src;
  for loop(i, cnt) {
    dst8[i] = src8[i];
  }
}
static inline void
mset(void *addr, int val, int cnt) {
  assert(addr);
  assert(cnt >= 0);
  unsigned char *dst = addr;
  for loop(i, cnt) {
    dst[i] = castb(val);
  }
}
#define swap(x,y) do {\
  unsigned char uniqid(t)[szof(x) == szof(y) ? szof(x) : -1]; \
  mcpy(uniqid(t),&y,szof(x)); \
  mcpy(&y,&x,szof(x)); \
  mcpy(&x,uniqid(t),szof(x)); \
} while(0)

/* ---------------------------------------------------------------------------
 *                                Range
 * ---------------------------------------------------------------------------
 */
#define rng(b,e,n) rng__mk(rng__bnd(b,n), rng__bnd(e,n), n)
#define intvl(b,n) rng(b,n,n)
#define rngn(n) rng(0,(n),(n))
#define rng_inv (struct rng){-1,-1,-1,-1}
#define rng_nil (struct rng){0}

#define slc(b,e) rng((b),(e),(e)-(b))
#define slc_beg(p,r) ((p)+(r).lo)
#define slc_end(p,r) ((p)+(r).hi)
#define slc_at(p,r,i) (slc_beg(p,r)[i])
#define slc_ptr(p,r,i) ((p)+(r).lo+(i))

#define rng_has_incl(a,b) ((a)->lo <= (b)->lo && (a)->hi >= (b)->hi)
#define rng_has_inclv(a,v) ((v) >= (a)->lo && (v) <= (a)->hi)
#define rng_has_excl(a,b) ((a)->lo < (b)->lo && (a)->hi > (b)->hi)
#define rng_has_exclv(a,v) ((v) > (a)->lo && (v) < (a)->hi)
#define rng_overlaps(a,b) (max((a)->lo, (b)->lo) <= min((a)->hi, (b)->hi))
#define rng_is_inv(a) (a.cnt < 0 || a.total < 0)

#define rng_clamp(a,v) clamp((a)->lo, v, (a)->hi)
#define rng_cnt(r) ((r)->cnt)
#define rng_shft(r, d) (r)->lo += (d), (r)->hi += (d)
#define rng_norm(r,v) (castf(castd(rng_clamp(r,v) - (r)->lo) / castd(rng_cnt(r))))

#define rng_rhs(r,n) rng_sub(r,n,(r)->cnt)
#define rng_lhs(r,n) rng_sub(r,0,n)
#define rng_cut_lhs(r,n) *(r) = rng_rhs(s,n)
#define rng_cut_rhs(r,n) *(r) = rng_lhs(s,n)

static force_inline int
rng__bnd(int idx, int cnt) {
  int lft = max(cnt, 1) - 1;
  int val = (idx < 0) ? (cnt - idx) : idx;
  return clamp(val, 0, lft);
}
static force_inline struct rng
rng__mk(int low, int high, int cnt) {
  assert(low <= high);
  struct rng ret = {.lo = low, .hi = high};
  ret.cnt = abs(ret.hi - ret.lo);
  ret.total = cnt;
  return ret;
}
static force_inline struct rng
rng_sub(const struct rng *rng, int beg, int end) {
  struct rng ret = rng(beg, end, rng->total);
  rng_shft(&ret, rng->lo);
  return ret;
}
static force_inline struct rng
rng_put(const struct rng *rng, const struct rng *val) {
  struct rng ret = *val;
  rng_shft(&ret, rng->lo);
  return ret;
}

/* ---------------------------------------------------------------------------
 *                                  Hash
 * ---------------------------------------------------------------------------
 */
#define FNV1A32_HASH_INITIAL 2166136261U
#define FNV1A64_HASH_INITIAL 14695981039346656037llu

static purist inline no_sanitize_int unsigned
fnv1a32(const void *ptr, int cnt, unsigned hash) {
  const unsigned char *ptr8 = ptr;
  if (!ptr) {
    return FNV1A32_HASH_INITIAL;
  }
  for loop(i,cnt) {
     hash = (hash ^ ptr8[i]) * 16777619u;
  }
  return hash;
}
static purist inline no_sanitize_int unsigned long long
fnv1a64(const void *ptr, int len, unsigned long long hash) {
  const unsigned char *ptr8 = ptr;
  if (!ptr || !len) {
    return FNV1A64_HASH_INITIAL;
  }
  for loop(i,len) {
    hash ^= (unsigned long long)ptr8[i];
    hash *= 1099511628211LLU;
  }
  return hash;
}
static purist unsigned
fnv1au32(unsigned hash, unsigned uid) {
  return fnv1a32(&uid, sizeof(uid), hash);
}
static purist unsigned long long
fnv1au64(unsigned long long uid, unsigned long long hash) {
  return fnv1a64(&uid, sizeof(uid), hash);
}
static purist unsigned long long
hash_ptr(const void *ptr) {
  return fnv1a64(&ptr, szof(void*), FNV1A64_HASH_INITIAL);
}
static purist unsigned long long
hash_int(long long lld) {
  return fnv1a64(&lld, szof(lld), FNV1A64_HASH_INITIAL);
}
static purist unsigned long long
hash_lld(long long lld) {
  return fnv1a64(&lld, szof(lld), FNV1A64_HASH_INITIAL);
}

/* ---------------------------------------------------------------------------
 *                                Random
 * ---------------------------------------------------------------------------
 */
static inline unsigned long long
rnd_gen(unsigned long long val, int idx) {
  return val + castull(idx) * 0x9E3779B97F4A7C15llu;
}
static inline unsigned long long
rnd_mix(unsigned long long val) {
  val = (val ^ (val >> 30)) * 0xBF58476D1CE4E5B9llu;
  val = (val ^ (val >> 27)) * 0x94D049BB133111EBllu;
  return val ^ (val >> 31llu);
}
static inline unsigned long long
rnd_split_mix(unsigned long long *val, int idx) {
  *val = rnd_gen(*val, idx);
  return rnd_mix(*val);
}
static inline unsigned long long
rnd(unsigned long long *val) {
  return rnd_split_mix(val, 1);
}
static inline unsigned
rndu(unsigned long long *val) {
  unsigned long long ret = rnd(val);
  return castu(ret & 0xffffffffu);
}
static inline int
rndi(unsigned long long *val) {
  unsigned rnd = rndu(val);
  long long norm = castll(rnd) - (UINT_MAX/2);
  assert(norm >= INT_MIN && norm <= INT_MAX);
  return casti(norm);
}
static inline double
rndn(unsigned long long *val) {
  unsigned rnd = rndu(val);
  return castd(rnd) / castd(UINT_MAX);
}
static unsigned
rnduu(unsigned long long *x, unsigned mini, unsigned maxi) {
  unsigned lo = min(mini, maxi);
  unsigned hi = max(mini, maxi);
  unsigned rng = castu(-1);
  unsigned n = hi - lo + 1U;
  if (n == 1U)  {
    return mini;
  } else if(n == 0u) {
    return rndu(x);
  } else {
    unsigned v = 0;
    unsigned remainder = rng % n;
    do {v = rndu(x);}
    while(v >= rng - remainder);
    return mini + v % n;
  }
}
static inline float
rndf01(unsigned long long *x) {
  unsigned u = rndu(x);
  double du = castd(u);
  double div = castd((unsigned)-1);
  return castf(du/div);
}
static float
rnduf(unsigned long long *x, float mini, float maxi) {
  unsigned u = rndu(x);
  float lo = min(mini, maxi);
  float hi = max(mini, maxi);

  float rng = hi - lo;
  double du = castd(u);
  double div = castd((unsigned)-1);
  return lo + rng * castf(du/div);
}

/* ---------------------------------------------------------------------------
 *                                  Array
 * ---------------------------------------------------------------------------
 */
#define arrv(b) (b),cntof(b)
#define arr_shfl(a,n,p) do {                              \
  for (int uniqid(i) = 0; uniqid(i) < (n); ++uniqid(i)) { \
    if ((p)[uniqid(i)] >= 0) {                            \
      int uniqid(j) = uniqid(i);                          \
      while ((p)[uniqid(j)] != uniqid(i)) {               \
        int uniqid(d) = (p)[uniqid(j)];                   \
        swap((a)[uniqid(j)],(a)[uniqid(d)]);              \
        (p)[uniqid(j)] = -1 - uniqid(d);                  \
        uniqid(j) = uniqid(d);                            \
      } (p)[uniqid(j)] = -1 - (p)[uniqid(j)];             \
    }                                                     \
}} while (0)

#define arr_eachp(it,a,e) ((it) = (a); (it) < (e); ++(it))
#define arr_each(it,a,n) arr_eachp(it,a,(a)+(n))
#define arr_eachv(it,a) arr_eachp(it,(a),(a)+cntof(a))
#define arr_loopn(i,a,n) (int i = 0; i < min(n,cntof(a)); ++i)
#define arr_loopv(i,a) (int i = 0; i < cntof(a); ++i)
#define arr_loop(i,r) (int (i) = (r).lo; (i) != (r).hi; (i) += 1)
#define arr_rm(a,i,n) memmove(&(a)[i], &a[i+1], (size_t)(casti(n) - 1 - i) * sizeof((a)[0]))

/* ---------------------------------------------------------------------------
 *                                Sequence
 * ---------------------------------------------------------------------------
 */
static inline void
seq_rng(int *seq, struct rng rng) {
  for loopi(i,k,rng) {
    seq[k] = i;
  }
}
static inline void
seq_rngu(unsigned *seq, struct rng rng) {
  for loopi(i,k,rng) {
    seq[k] = castu(i);
  }
}
static inline void
seq_rnd(int *seq, int n, unsigned long long *r) {
  for (int i = n - 1; i > 0; --i) {
    unsigned at = rndu(r) % castu(i + 1);
    iswap(seq[i], seq[at]);
  }
}
static inline void
seq_fix(int *p, int n) {
  for (int i = 0; i < n; ++i) {
    p[i] = -1 - p[i];
  }
}

/* ---------------------------------------------------------------------------
 *                                  Bits
 * ---------------------------------------------------------------------------
 */
static purist inline unsigned
bit_rev32(unsigned x){
  x = ((x & 0x55555555) << 1) | ((x >> 1) & 0x55555555);
  x = ((x & 0x33333333) << 2) | ((x >> 2) & 0x33333333);
  x = ((x & 0x0F0F0F0F) << 4) | ((x >> 4) & 0x0F0F0F0F);
  x = (x << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24);
  return x;
}
static purist inline unsigned long long
bit_rev64(unsigned long long x){
  unsigned x1 = castu(x & 0xffffffffu);
  unsigned x2 = castu(x >> 32);
  return (castull(bit_rev32(x1)) << 32)|bit_rev32(x2);
}
static purist inline unsigned
bit_eqv(unsigned x, unsigned y) {
  /* Calculates bitwise equivalence.
   * Bitwise equivalence is the opposite of xor. It sets all bits that
   * are the same to 1, whereas xor sets all bits that are different to 1.
   * Thus, you can simply use ~ to get bitwise equivalence from xor.
   * Example:
   *    11001100, 11110000 -> 11000011
   */
  return castu(~(x ^ y));
}

/* ---------------------------------------------------------------------------
 *                                  Bitset
 * ---------------------------------------------------------------------------
 */
#define bit_loop(i,x,s,n) \
  (int i = bit_ffs(s,n,0), x = 0; i < n; i = bit_ffs(s,n,i+1), x = x + 1)
static int bit_xor(unsigned long *addr, int nr);

static purist inline int
bit_tst(const unsigned long *addr, int nr) {
  assert(addr);
  unsigned long msk = (unsigned long)nr & (BITS_PER_LONG - 1);
  return (1ul & (addr[bit_word(nr)] >> msk)) != 0;
}
static inline int
bit_tst_clr(unsigned long *addr, int nr) {
  assert(addr);
  if (bit_tst(addr, nr)) {
    bit_xor(addr, nr);
    return 1;
  }
  return 0;
}
static inline int
bit_set(unsigned long *addr, int nr) {
  unsigned long m = bit_mask(nr);
  unsigned long *p = addr + bit_word(nr);
  int ret = casti(*p &m);
  *p |= m;
  return ret;
}
static inline void
bit_set_on(unsigned long *addr, int nr, int cond) {
  if (cond) {
    bit_set(addr, nr);
  }
}
static inline int
bit_clr(unsigned long *addr, int nr) {
  assert(addr);
  unsigned long m = bit_mask(nr);
  unsigned long *p = addr + bit_word(nr);
  int ret = casti((*p & m));
  *p &= ~m;
  return ret;
}
static inline void
bit_clr_on(unsigned long *addr, int nr, int cond) {
  if (cond) {
    bit_clr(addr, nr);
  }
}
static inline int
bit_xor(unsigned long *addr, int nr) {
  assert(addr);
  unsigned long m = bit_mask(nr);
  unsigned long *p = addr + bit_word(nr);
  *p ^= m;
  return (*p & m) ? 1 : 0;
}
static inline void
bit_fill(unsigned long *addr, int byte, int nbits) {
  assert(addr);
  int n = bits_to_long(nbits);
  mset(addr, byte, n * szof(long));
}
static int
bit_ffs(const unsigned long *addr, int nbits, int idx) {
  assert(addr);
  unsigned long off = bit_word_idx(idx);
  unsigned long long n = (unsigned long long)bits_to_long(nbits);
  for (unsigned long i = bit_word(idx); i < n; ++i) {
    unsigned long cmsk = bit_mask(off) - 1U;
    unsigned long c = addr[i] & (unsigned long)~cmsk;
    if (!c) {
      off = bit_word_nxt(off);
      continue;
    }
    int pos = cpu_bit_ffs64(c);
    return (int)(i * BITS_PER_LONG) + pos;
  }
  return nbits;
}
static int
bit_ffz(const unsigned long *addr, int nbits, int idx) {
  assert(addr);
  assert(nbits >= 0);
  unsigned long off = bit_word_idx(idx);
  unsigned long long n = (unsigned long long)bits_to_long(nbits);
  for (unsigned long i = bit_word(idx); i < n; ++i) {
    unsigned long cmsk = bit_mask(off) - 1U;
    unsigned long c = (~addr[i]) & (unsigned long)~cmsk;
    if (!c) {
      off = bit_word_nxt(off);
      continue;
    }
    int pos = cpu_bit_ffs64(c);
    return (int)(i * BITS_PER_LONG) + pos;
  }
  return nbits;
}
static int
bit_cnt_set(const unsigned long *addr, int nbits, int idx) {
  assert(addr);
  assert(nbits >= 0);
  assert(idx < nbits);

  unsigned long widx = bit_word(idx);
  unsigned long cmsk = max(1U, bit_mask(idx)) - 1U;
  if ((unsigned long)nbits < BITS_PER_LONG) {
    cmsk |= ~(max(1U, bit_mask(nbits)) - 1U);
  }
  unsigned long w = addr[widx] & ~cmsk;
  unsigned long long n = (unsigned long long)bits_to_long(nbits);
  int cnt = cpu_bit_cnt64(w);

  for (unsigned long i = widx + 1; i < n; ++i) {
    w = addr[i];
    if ((unsigned long)nbits - BITS_PER_LONG * i < BITS_PER_LONG) {
      cmsk |= ~(bit_mask(nbits) - 1U);
      w = w & ~cmsk;
    }
    cnt += cpu_bit_cnt64(w);
  }
  return cnt;
}
static int
bit_cnt_zero(const unsigned long *addr, int nbits, int idx) {
  assert(addr);
  assert(nbits >= 0);

  unsigned long widx = bit_word(idx);
  unsigned long cmsk = max(1U, bit_mask(idx)) - 1U;
  if ((unsigned long)nbits < BITS_PER_LONG) {
    cmsk |= ~(max(1U, bit_mask(nbits)) - 1U);
  }
  unsigned long long n = (unsigned long long)bits_to_long(nbits);
  unsigned long w = addr[widx] & ~cmsk;
  int cnt = cpu_bit_cnt64(~w);
  for (unsigned long i = widx + 1; i < n; ++i) {
    w = addr[i];
    if ((unsigned long)nbits - BITS_PER_LONG * i < BITS_PER_LONG) {
      cmsk |= ~(bit_mask(nbits) - 1U);
      w = w & ~cmsk;
    }
    cnt += cpu_bit_cnt64(~w);
  }
  return min(nbits, cnt);
}
static int
bit_set_at(const unsigned long *addr, int nbits, int off, int idx) {
  assert(addr);
  assert(nbits >= 0);
  assert(off < nbits);
  if (!idx) {
    return bit_ffs(addr, nbits, idx);
  }
  for (int i = 0; i <= idx && off < nbits; ++off) {
    if (bit_tst(addr, off) && i++ == idx) break;
  }
  return off;
}
static int
bit_zero_at(const unsigned long *addr, int nbits, int off, int idx) {
  assert(addr);
  assert(nbits >= 0);
  assert(off < nbits);
  if (!idx) {
    return bit_ffz(addr, nbits, idx);
  }
  for (int i = 0; i <= idx && off < nbits; ++off) {
    if (!bit_tst(addr, off) && i++ == idx) {
      break;
    }
  }
  return off;
}

/* ---------------------------------------------------------------------------
 *                                  Unicode
 * ---------------------------------------------------------------------------
 */
// clang-format off
#define is_upper(c) (((unsigned)c - 'A') < 26)
#define is_lower(c) (((unsigned)c - 'a') < 26)
#define to_lower(c) is_upper(c) ? (c) | 32 : c
#define to_upper(c) is_lower(c) ? ((c) & ~32) : c
#define is_digit(c) (((c) >= '0') && ((c) <= '9'))
#define is_hex(c) (is_digit(c) || (((c) >= 'a') && ((c) <= 'f')) || (((c) >= 'A') && ((c) <= 'F')))
#define is_alpha(c) (is_lower(c) || is_upper(c))
#define is_printable(c) ((c) >= 32 && (c) <= 126)
// clang-format on

static int
is_space(long c) {
  switch (c) {
  default: return 0;
  case 0x0020:
  case 0x0009:
  case 0x000a:
  case 0x000b:
  case 0x000c:
  case 0x000d:
  case 0x00A0:
  case 0x1680:
  case 0x2000:
  case 0x2001:
  case 0x2002:
  case 0x2003:
  case 0x2004:
  case 0x2005:
  case 0x2006:
  case 0x2007:
  case 0x2008:
  case 0x2009:
  case 0x200A:
  case 0x202F:
  case 0x205F:
  case 0x3000:
    return 1;
  }
}
static int
is_quote(long c) {
  switch (c) {
  default: return 0;
  case '\"':
  case '`':
  case '\'':
  case 0x00AB:
  case 0x00BB:
  case 0x2018:
  case 0x2019:
  case 0x201A:
  case 0x201C:
  case 0x201D:
  case 0x201E:
  case 0x2039:
  case 0x203A:
    return 1;
  }
}
static int
is_punct(long c) {
  switch (c) {
  default: return is_quote(c);
  case ',':
  case '.':
  case ';':
  case '(':
  case ')':
  case '{':
  case '}':
  case '[':
  case ']':
  case '<':
  case '>':
  case '|':
  case '/':
  case '?':
  case '#':
  case '~':
  case '@':
  case '=':
  case '+':
  case '-':
  case '_':
  case '*':
  case '&':
  case '^':
  case '%':
  case '$':
  case '!':
  case '\\':
  case ':':
  case 0x0964:
  case 0x0589:
  case 0x3002:
  case 0x06D4:
  case 0x2CF9:
  case 0x0701:
  case 0x1362:
  case 0x166E:
  case 0x1803:
  case 0x2FCE:
  case 0xA4FF:
  case 0xA60E:
  case 0xA6F3:
  case 0x083D:
  case 0x1B5F:
  case 0x060C:
  case 0x3001:
  case 0x055D:
  case 0x07F8:
  case 0x1363:
  case 0x1808:
  case 0xA4FE:
  case 0xA60D:
  case 0xA6F5:
  case 0x1B5E:
  case 0x2047:
  case 0x2048:
  case 0x2049:
  case 0x203D:
  case 0x2757:
  case 0x203C:
  case 0x2E18:
  case 0x00BF:
  case 0x061F:
  case 0x055E:
  case 0x0706:
  case 0x1367:
  case 0x2CFA:
  case 0x2CFB:
  case 0xA60F:
  case 0xA6F7:
  case 0x11143:
  case 0xAAF1:
  case 0x00A1:
  case 0x07F9:
  case 0x1944:
  case 0x00B7:
  case 0x1039F:
  case 0x103D0:
  case 0x12470:
  case 0x1361:
  case 0x1091:
  case 0x0830:
  case 0x058A:
  case 0x1806:
  case 0x0387:
  case 0x061B:
  case 0x1364:
  case 0x2024:
  case 0x1365:
  case 0xA6F6:
  case 0x1B5D:
  case 0x2026:
  case 0xFE19:
  case 0x0EAF:
  case 0x00AB:
  case 0x2039:
  case 0x00BB:
  case 0x203A:
  case 0x00AF:
  case 0x00B2:
  case 0x00B3:
  case 0x00B4:
  case 0x00B5:
  case 0x00B6:
  case 0x00B8:
  case 0x00B9:
  case 0x00BA:
  case 0x2010:
  case 0x2013:
  case 0x2014:
  case 0x2015:
  case 0x2016:
  case 0x2020:
  case 0x2021:
  case 0x2022:
  case 0x2025:
  case 0x2030:
  case 0x2031:
  case 0x2032:
  case 0x2033:
  case 0x2034:
  case 0x2035:
  case 0x203E:
  case 0x2041:
  case 0x2043:
  case 0x2044:
  case 0x204F:
  case 0x2057:
    return 1;
  }
}
/* ---------------------------------------------------------------------------
 *                                  String
 * ---------------------------------------------------------------------------
 */
// clang-format off
#define cstrn(s) casti(strlen(s))
#define str(p,r) (struct str){.ptr = (p), .rng = (r)}
#define strn(s,n) str(s,rngn(n))
#define str0(s) str(s,rngn(cstrn(s)))
#define strv(s) (struct str){.ptr = (s),.rng ={0,cntof(s)-1,cntof(s)-1,cntof(s)-1}}
#define strf(s) (s).rng.cnt, str_beg(s)
#define str_nil (struct str){0,rngn(0)}
#define str_inv (struct str){0,rng_inv}
#define str_len(s) rng_cnt(&(s).rng)
#define str_is_empty(s) (str_len(s) == 0)
#define str_is_inv(s) ((s).ptr == 0 || rng_is_inv((s).rng))
#define str_is_val(s) (!rng_is_inv((s).rng))
#define str_eq(a,b) (str_cmp(a,b) == 0)
#define str_neq(a,b) (str_cmp(a,b) != 0)
#define str_sub(s,b,e) str((s).ptr, rng_sub(&(s).rng, b, e))
#define str_rhs(s,n) str_sub(s, min(str_len(s), n), str_len(s))
#define str_lhs(s,n) str_sub(s, 0, min(str_len(s), n))
#define str_cut_lhs(s,n) *(s) = str_rhs(*(s), n)
#define str_cut_rhs(s,n) *(s) = str_lhs(*(s), n)
#define str_beg(s) (((s).ptr) ? (slc_beg((s).ptr, (s).rng)) : 0)
#define str_end(s) (((s).ptr) ? (slc_end((s).ptr, (s).rng)) : 0)
#define str_at(s,i) slc_at((s).ptr, (s).rng, i)
#define str_ptr(s,i) slc_ptr((s).ptr, (s).rng, i)

#define str_each(it,c) (const char *it = slc_beg((c).ptr, (c).rng); it < slc_end((c).ptr, (c).rng); it += 1)
#define str_eachr(it,s,r) (const char *it = (s).str + (s).rng.lo + (r).lo; (it) != (s).str + (s).rng.hi - (r).hi; (it) += 1)
#define str_loop(i,s) (int i = 0; i < str_len(s); ++i)
#define str_tok(it, rest, src, delim)                       \
  (struct str rest = src, it = str_split_cut(&rest, delim); \
   str_len(it); it = str_split_cut(&rest, delim))
// clang-format on

static inline struct str
strptr(const char *ptr, const char *begin, const char *end, int total) {
  if (ptr == 0 || begin == 0 || end == 0) {
    return str_nil;
  }
  struct str str = {.ptr = ptr};
  int low = casti(begin - ptr);
  int hih = casti(end - ptr);
  int cnt = casti(end - begin);
  str.rng = (struct rng){low, hih, cnt, total};
  return str;
}
static inline struct str
strp(const char *begin, const char *end) {
  if (begin == 0 || end == 0) {
    return str_nil;
  }
  int cnt = casti(end - begin);
  return strptr(begin, begin, end, cnt);
}
static unsigned long long
str__hash(struct str str, unsigned long long id) {
  assert(str_len(str) >= 0);
  if (str_is_inv(str)) {
    return id;
  }
  return fnv1a64(str_beg(str), str_len(str), id);
}
static unsigned long long
str_hash(struct str str) {
  assert(str_len(str) >= 0);
  return str__hash(str, FNV1A64_HASH_INITIAL);
}
static int
str_cmp(struct str lhs, struct str rhs) {
  assert(lhs.ptr);
  assert(str_len(lhs) >= 0);
  assert(rhs.ptr);
  assert(str_len(rhs) >= 0);

  int cnt = min(str_len(lhs), str_len(rhs));
  for loop(i,cnt) {
    if (str_at(lhs,i) < str_at(rhs,i)) {
      return -1;
    } else if (str_at(lhs,i) > str_at(rhs,i)) {
      return +1;
    }
  }
  if (str_len(lhs) > str_len(rhs)) {
    return +1;
  } else if (str_len(lhs) < str_len(rhs)) {
    return -1;
  }
  return 0;
}
static int
str_fnd(struct str hay, struct str needle) {
  assert(str_len(hay) >= 0);
  assert(str_len(needle) >= 0);
  if (str_len(hay) == 0 ||
      str_len(hay) == 0) {
    return str_len(hay);
  }
  if (str_len(needle) == 1) {
    const char *ret = cpu_str_chr(str_beg(hay), str_len(hay), str_at(needle,0));
    if (ret) {
      return casti(ret - str_beg(hay));
    }
    return str_len(hay);
  } else {
    return cpu_str_fnd(str_beg(hay), castsz(str_len(hay)), str_beg(needle), castsz(str_len(needle)));
  }
}
static int
str_has(struct str hay, struct str needle) {
  assert(hay.ptr);
  assert(str_len(hay) >= 0);
  assert(needle.ptr);
  assert(str_len(needle) >= 0);
  return str_fnd(hay, needle) >= str_len(hay);
}
static struct str
str_split_cut(struct str *str, struct str delim) {
  assert(str);
  assert(delim.ptr);
  assert(str_len(delim) >= 0);

  int pos = str_fnd(*str, delim);
  if (pos < str_len(*str)) {
    struct str ret = str_lhs(*str, pos);
    str_cut_lhs(str, pos + 1);
    return ret;
  } else {
    struct str ret = *str;
    *str = str_nil;
    return ret;
  }
}
static void
ut_str(void) {
  char test_str[64] = "cmd[stk?utf/boot/usr/str_bootbany.exe";
  const struct str hay = str0(test_str);
  int dot_pos = str_fnd(hay, strv("["));
  assert(dot_pos == 3);
  int cmd_pos = str_fnd(hay, strv("cmd"));
  assert(cmd_pos == 0);
  int str_pos = str_fnd(hay, strv("?utf"));
  assert(str_pos == 7);
  int long_pos = str_fnd(hay, strv("boot/usr/str_bootbany"));
  assert(long_pos == 12);
  int close_no = str_fnd(hay, strv("str/"));
  assert(close_no == str_len(hay));
  int no = str_fnd(hay, strv("rock"));
  assert(no == str_len(hay));
}

/* ---------------------------------------------------------------------------
 *                                  UTF-8
 * ---------------------------------------------------------------------------
 */
#define UTF_SIZ 4
#define UTF_BUF (UTF_SIZ + 1)
#define UTF_INVALID 0xFFFD

static const unsigned char utf_byte[UTF_SIZ+1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static const unsigned char utf_mask[UTF_SIZ+1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const unsigned utf_min[UTF_SIZ+1] = {0, 0, 0x80, 0x800, 0x10000};
static const unsigned utf_max[UTF_SIZ+1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

#define utf_enc_byte(u,i) ((char)((utf_byte[i]) | ((unsigned char)u & ~utf_mask[i])))
#define utf_tst(c) (((c) & 0xC0) != 0x80)
#define utf_val(u,i) (between(u, utf_min[i], utf_max[i]) && !between(u, 0xD800, 0xDFFF))
#define utf_loop(rune, it, rest, src)\
  (struct str rest = src, it = utf_dec(rune, &rest); it.rng.cnt; it = utf_dec(rune, &rest))
#define utf_loop_rev(rune, it, rest, src)\
  (struct str rest = src, it = utf_dec_rev(rune, &rest); str_len(it); it = utf_dec_rev(rune, &rest))

static struct str
utf_dec(unsigned *rune, struct str *str) {
  assert(str);
  if (str_is_empty(*str)) {
    if (rune) {
      *rune = UTF_INVALID;
    }
    if (str_is_inv(*str)) {
      return str_nil;
    }
    return strptr(str->ptr, str_end(*str), str_end(*str), str->rng.total);
  }
  int cnt = 0;
  unsigned ret = 0;
  const char *ptr = str_beg(*str);
  switch (*ptr & 0xf0) {
    // clang-format off
    case 0xf0: ret = (*ptr & 0x07); cnt = 3; break;
    case 0xe0: ret = (*ptr & 0x0f); cnt = 2; break;
    case 0xc0: ret = (*ptr & 0x1f); cnt = 1; break;
    case 0xd0: ret = (*ptr & 0x1f); cnt = 1; break;
    default:   ret = (*ptr & 0xff); cnt = 0; break;
    // clang-format on
  }
  if (str_beg(*str) + cnt + 1 > str_end(*str)) {
    if (rune) {
      *rune = UTF_INVALID;
    }
    *str = strptr(str->ptr, str_end(*str), str_end(*str), str->rng.total);
    return *str;
  }
  struct str view = strptr(str->ptr, ptr, ptr + cnt + 1, str->rng.total);
  for (int i = 0; i < cnt; ++i) {
    ret = (ret << 6) | (*(++ptr) & 0x3f);
  }
  if (rune) {
    *rune = ret;
  }
  *str = strptr(str->ptr, str_beg(*str) + cnt + 1, str_end(*str), str->rng.total);
  return view;
}
static unsigned
utf_get(struct str str) {
  unsigned rune;
  utf_dec(&rune, &str);
  return rune;
}
static struct str
utf_dec_rev(unsigned *rune, struct str *str) {
  const char *ptr = str_end(*str);
  while (ptr > str_beg(*str)) {
    char pnt = *(--ptr);
    if (utf_tst(pnt)) {
      struct str rest = strptr(str->ptr, ptr, str_end(*str), str->rng.total);
      struct str itr = utf_dec(rune, &rest);
      *str = strptr(str->ptr, str_beg(*str), ptr, str->rng.total);
      return itr;
    }
  }
  *str = str_nil;
  *rune = UTF_INVALID;
  return str_nil;
}
static int
utf_enc(char *buf, int cap, unsigned rune) {
  int cnt = 0;
  if (!utf_val(rune, 0)) {
    return 0;
  }
  for (cnt = 1; rune > utf_max[cnt]; ++cnt);
  if (cap < cnt || !cnt || cnt > UTF_SIZ) {
    return 0;
  }
  for (int i = cnt - 1; i != 0; --i) {
    buf[i] = utf_enc_byte(rune, 0);
    rune >>= 6;
  }
  buf[0] = utf_enc_byte(rune, cnt);
  return cnt;
}
static struct str
utf_at(unsigned *rune, struct str str, int idx) {
  int i = 0;
  unsigned glyph = 0;
  for utf_loop(&glyph, itr, _, str) {
    if (i >= idx) {
      if (rune) {
        *rune = glyph;
      }
      return itr;
    }
    i++;
  }
  if (rune) {
    *rune = UTF_INVALID;
  }
  if (str_is_inv(str)) {
    return str_nil;
  }
  return strptr(str.ptr, str_end(str), str_end(str), str.rng.total);
}
static int
utf_at_idx(struct str str, int idx) {
  struct str view = utf_at(0, str, idx);
  if (str_len(view)) {
    return casti(str_beg(view) - str_beg(str));
  }
  return str_len(str);
}
static int
utf_len(struct str str) {
  int i = 0;
  unsigned rune = 0;
  for utf_loop(&rune, _, _2, str) {
    i++;
  }
  return i;
}
/* ---------------------------------------------------------------------------
 *                                String
 * ---------------------------------------------------------------------------
 */
static inline struct str
str_set(char *buf, int cnt, struct str str) {
  assert(buf);
  assert(str_len(str) >= 0);
  if (str_is_empty(str)) {
    return strn(buf,0);
  } else if (str_len(str) > cnt) {
    return str_inv;
  }
  mcpy(buf, str_beg(str), str_len(str));
  return strn(buf, str_len(str));
}
static inline struct str
str_sqz(char *buf, int cnt, struct str str) {
  assert(buf);
  assert(str_len(str) >= 0);

  int len = min(cnt, str_len(str));
  mcpy(buf, str_beg(str), len);
  return strn(buf,len);
}
static struct str
str_add(char *buf, int cap, struct str in, struct str str) {
  assert(buf);
  if (str_len(in) + str_len(str) < cap) {
    mcpy(buf + str_len(in), str_beg(str), str_len(str));
    return strn(buf, str_len(str) + str_len(in));
  }
  int nnn = 0;
  unsigned rune = 0;
  int cnt = cap - str_len(str);
  for utf_loop(&rune, itr, _, str) {
    int len = casti(str_end(itr) - str_beg(str));
    if (len >= cnt) {
      break;
    }
    nnn = len;
  }
  mcpy(buf + str_len(in), str_beg(str), nnn);
  return strn(buf, str_len(in) + nnn);
}
static struct str
str_rm(char *buf, struct str in, int cnt) {
  assert(buf);
  int left = max(0, str_len(in) - cnt);
  return strn(buf, left);
}
static struct str
str_put(char *buf, int cap, struct str in, int pos, struct str str) {
  assert(buf);
  if (pos >= str_len(in)) {
    return str_add(buf, cap, in, str);
  }
  if (str_len(in) + str_len(str) < cap) {
    memmove(buf + pos + str_len(str), buf + pos, castsz(str_len(in) - pos));
    mcpy(buf + pos, str_beg(str), str_len(str));
    return strn(buf, str_len(in) + str_len(str));
  }
  unsigned rune = 0;
  int cnt = cap - str_len(str);
  for utf_loop(&rune, itr, _, str) {
    int len = casti(str_end(itr) - str_beg(str));
    if (len >= -cnt) {
      break;
    }
  }
  memmove(buf + pos + str_len(str), buf + pos, cast(size_t, str_len(in) - pos));
  mcpy(buf + pos, str_beg(str), str_len(str));
  return strn(buf, str_len(str) + str_len(in));
}
static struct str
str_del(char *buf, struct str in, int pos, int len) {
  assert(buf);
  if (pos >= str_len(in)) {
    return str_rm(buf, in, len);
  }
  assert(pos + len <= str_len(in));
  memmove(buf + pos, buf + pos + len, castsz(str_len(in) - pos));
  return strn(buf, str_len(in) - len);
}
static struct str
str_fmtsn(char *buf, int n, const char *fmt, ...) {
  assert(buf);
  int ret;
  va_list va;
  va_start(va, fmt);
  ret = fmtvsn(buf, n, fmt, va);
  va_end(va);
  return strn(buf, ret);
}
static struct str
str_add_fmt(char *b, int cap, struct str in, const char *fmt, ...) {
  assert(b);
  va_list va;
  va_start(va, fmt);
  int left = max(0, cap - str_len(in));
  char *dst = b + str_len(in);
  int ret = fmtvsn(dst, left, fmt, va);
  va_end(va);
  return strn(b, str_len(in) + ret);
}

/* ---------------------------------------------------------------------------
 *                                Time
 * ---------------------------------------------------------------------------
 */
#define time_inf      (9223372036854775807ll)
#define time_ninf     (-9223372036854775807ll)
#define time_ns(ns)   castll((ns))
#define time_us(us)   (castll((us))*1000ll)
#define time_ms(ms)   (castll((ms))*1000000ll)
#define time_sec(s)   (castll((s))*1000000000ll)
#define time_min(m)   (castll((m))*60000000000ll)
#define time_hours(h) (castll((m))*3600000000000ll)

#define time_flt_sec(s)   (castll(castd((s))*1000000000.0))
#define time_flt_min(s)   (castll(castd((s))*60000000000.0))
#define time_flt_hour(s)  (castll(castd((s))*3600000000000.0))

#define time_30fps  time_ns(33333333)
#define time_60fps  time_ns(16666666)
#define time_90fps  time_ns(11111111)
#define time_120fps time_ns(8333333)
#define time_144fps time_ns(6944444)
#define time_240fps time_ns(4166667)

#define ns_time(t)    (t)
#define us_time(t)    ((t)/1000ll)
#define ms_time(t)    ((t)/1000000ll)
#define sec_time(t)   ((t)/1000000000ll)
#define min_time(t)   ((t)/60000000000ll)
#define hour_time(t)  ((t)/3600000000000ll)

#define sec_flt_time(t)   castf(castd(t)*(1.0/1000000000.0))
#define min_flt_time(t)   castf(castd(t)*(1.0/10000000000.0))
#define hour_flt_time(t)  castf(castd(t)*(1.0/3600000000000.0))

static long long
time_sub(long long a, long long b) {
  if (a == time_inf) {
    return (b == time_inf) ? 0ll : time_inf;
  } else if (a == time_ninf) {
    return (b == time_ninf) ? 0ll : time_ninf;
  } else if (b == time_inf) {
    return time_ninf;
  } else if (b == time_ninf) {
    return time_inf;
  }
  return a - b;
}
static long long
time_add(long long a, long long b) {
  if (a == time_inf) {
    return (b == time_ninf) ? 0ll : time_inf;
  } else if (a == time_ninf) {
    return (b == time_inf) ? 0ll : time_ninf;
  } else if (b == time_inf) {
    return time_inf;
  } else if (b == time_ninf) {
    return time_ninf;
  }
  return a + b;
}
static long long
time_neg(long long a) {
  if (a == time_inf) {
    return time_ninf;
  } else if (a == time_ninf) {
    return time_inf;
  } else {
    return -a;
  }
}
static long long
time_mul(long long a, long long b) {
  return a * b;
}
static long long
time_div(long long a, long long b) {
  if (a == time_inf) {
    if (b == time_inf) {
      return 1ll;
    } else if (b == time_ninf){
      return -1ll;
    } else {
      return time_inf;
    }
  } else if (a == time_ninf) {
    if (b == time_inf) {
      return -1ll;
    } else if (b == time_ninf){
      return 1ll;
    } else {
      return time_ninf;
    }
  } else if (b == time_inf) {
    return time_inf;
  } else if (b == time_ninf) {
    return time_ninf;
  }
  return a / b;
}
static long long
time_mod(long long a, long long b) {
  assert(b != 0ll);
  if (a == time_inf || a == time_ninf) {
    return a;
  } else if (b == time_inf || b == time_ninf) {
    return 0;
  } else {
    return a % b;
  }
}

/* ---------------------------------------------------------------------------
 *                                  Path
 * ---------------------------------------------------------------------------
 */
static void
path_norm(char *path) {
  char *ptr = path;
  for (; *ptr; ptr++) {
    if (*ptr == '\\') {
      *ptr = '/';
    }
  }
  if (ptr != path && ptr[-1] == '/') {
    ptr[-1] = 0;
  }
}
static struct str
path_file(struct str path) {
  for (const char *p = str_end(path); p > str_beg(path); --p) {
    if (p[-1] == '/') {
      return strptr(path.ptr, p, str_end(path), path.rng.total);
    }
  }
  return path;
}
static struct str
path_ext(struct str path) {
  for (const char *p = str_end(path); p > str_beg(path); --p) {
    if (p[-1] == '.') {
      return strptr(path.ptr, p, str_end(path), path.rng.total);
    }
  }
  return str_nil;
}

/* ---------------------------------------------------------------------------
 *                              String-Buffer
 * ---------------------------------------------------------------------------
 */
// clang-format off
#define str_buf_push(b,s) str_buf__push(&(b)->cnt, (b)->mem, cntof((b)->mem), s)
#define str_buf_sqz(b,s,m) str_buf__sqz(&(b)->cnt, (b)->mem, cntof((b)->mem), s, m)
#define str_buf_get(b,h) str_buf__get((b)->mem, (b)->cnt, h)
#define str_buf_clr(b) str_buf__clr((b)->mem, &(b)->cnt)
#define str_buf_off(hdl) casti(((hdl) >> 16u) & 0xffff)
#define str_buf_len(hdl) casti((hdl) & 0xffff)
// clang-format on

static unsigned
str_buf__push(int *cnt, char *mem, int cap, struct str s) {
  assert(cnt);
  assert(*cnt >= 0);
  assert(*cnt <= 0xffff);
  assert(*cnt <= cap);
  assert(cap >= 0);
  assert(mem);
  assert(cap);

  unsigned off = castu(*cnt) & 0xffff;
  int lft = max(0, cap - *cnt);
  char *dst = mem + *cnt;
  struct str p = str_set(dst, lft, s);
  int n = str_is_val(p) * str_len(p);
  unsigned ret = (off << 16u)|(n & 0xffff);
  *cnt += n;
  return ret;
}
static unsigned
str_buf__sqz(int *cnt, char *mem, int cap, struct str s, int max_len) {
  assert(cnt);
  assert(cap <= 0x10000);
  assert(*cnt >= 0);
  assert(*cnt <= 0x10000);
  assert(*cnt <= cap);
  assert(cap >= 0);
  assert(*cnt <= cap);
  assert(mem);
  assert(cap);

  unsigned off = castu(*cnt) & 0xffff;
  int lft = min(max_len, max(0, cap - *cnt));
  char *dst = mem + *cnt;
  struct str p = str_sqz(dst, lft, s);
  *cnt += str_len(p);
  unsigned ret = (off << 16u)|(str_len(p) & 0xffff);
  return ret;
}
static struct str
str_buf__get(char *mem, int cnt, unsigned hdl) {
  assert(mem);
  assert(cnt >= 0);
  assert(str_buf_len(hdl) <= cnt);
  assert(str_buf_off(hdl) <= cnt);
  assert(str_buf_len(hdl) + str_buf_off(hdl) <= cnt);

  int off = str_buf_off(hdl);
  int len = str_buf_len(hdl);
  return strn(mem + off, len);
}
static void
str_buf__clr(char *mem, int *cnt) {
  assert(mem);
  assert(cnt);
  assert(*cnt >= 0);
  *cnt = 0;
}

/* ---------------------------------------------------------------------------
 *                                TABLE
 * ---------------------------------------------------------------------------
 */
// clang-format off
#define tbl__is_del(k) (((k) >> 63llu) != 0)
#define tbl__dist(h,n,i) (((i) + (n) - ((h) % n)) % (n))
#define tbl__key(k) ((k) != 0u && !tbl__is_del(k))
#define tbl__loop(n,i,t,cap) (int n = tbl__nxt_idx(t,cap,0), i = 0; n < cap && i < cap; n = tbl__nxt_idx(t,cap,n+1),++i)
#define tbl__clr(s,cnt,cap) do {mset(s,0,szof(unsigned long long)*cap); *cnt = 0;} while(0)

#define tbl_fnd(t, k) tbl__fnd((t)->keys, cntof((t)->keys), k)
#define tbl_del(t, k) tbl__del((t)->keys, cntof((t)->keys), &(t)->cnt, k)
#define tbl_clr(t) tbl__clr((t)->keys, &(t)->cnt, cntof((t)->keys))
#define tbl_loop(n,i,t) tbl__loop(n,i,(t)->keys,cntof((t)->keys))
#define tbl_val(t,i) ((i) < cntof((t)->keys))
#define tbl_inval(t,i) (!tbl_val(t,i))
#define tbl_get(t,i) (tbl_val(t,i) ? ((t)->vals + (i)): 0)
#define tbl_unref(t,i,d) (tbl_val(t,i) ? (t)->vals[i] : (d))
#define tbl_has(t,k) tbl_val(t,tbl_fnd(t,k))
#define tbl_put(t, k, v) do {\
  assert(szof(*(v)) == szof((t)->vals[0]));\
  tbl__put((t)->keys,(t)->vals,&(t)->cnt,cntof((t)->keys),k,v,szof((t)->vals[0]));\
} while(0)
// clang-format on

static inline unsigned long long
tbl__hash(unsigned long long key) {
  unsigned long long hsh = key & 0x7fffffffffffffffllu;
  return hsh | (hsh == 0);
}
static void
tbl__swap(void *a, void *b, void *tmp, int siz) {
  mcpy(tmp, a, siz);
  mcpy(a, b, siz);
  mcpy(b, tmp, siz);
}
static inline long long
tbl__store(unsigned long long *keys, void *vals, int *cnt,
           unsigned long long idx, unsigned long long hsh,
           void* val, int val_siz) {

  assert(cnt);
  assert(keys);

  keys[idx] = hsh;
  if (vals) {
    unsigned long long off = (unsigned long long)val_siz * idx;
    mcpy((unsigned char*)vals + off, val, val_siz);
  }
  *cnt += 1;
  return castll(idx);
}
static long long
tbl__put(unsigned long long *keys, void *vals, int *cnt, int cap,
         unsigned long long key, void *val, int val_siz) {

  assert(keys);
  assert(cnt);

  unsigned long long hsh = tbl__hash(key);
  unsigned long long siz = castull(cap);
  unsigned long long idx = hsh % siz;
  unsigned long long beg = idx;
  unsigned long long dist = 0;
  do {
    unsigned long long slot = keys[idx];
    if (!slot) {
      return tbl__store(keys, vals, cnt, idx, hsh, val, val_siz);
    }
    unsigned long long d = tbl__dist(slot, siz, idx);
    if (d++ > dist++) {
      continue;
    }
    if (tbl__is_del(slot)) {
      return tbl__store(keys, vals, cnt, idx, hsh, val, val_siz);
    }
    iswap(hsh, keys[idx]);
    if (vals) {
      void *tmp_val = (unsigned char*)vals + cap * val_siz;
      void *cur_val = (unsigned char*)vals + idx * (unsigned long long)val_siz;
      tbl__swap(cur_val, val, tmp_val, val_siz);
    }
    dist = d;
  } while ((idx = ((idx + 1) % siz)) != beg);
  return castll(siz);
}
static int
tbl__fnd(unsigned long long *set, int cap, unsigned long long key) {
  assert(set);
  unsigned long long h = tbl__hash(key);
  unsigned long long n = castull(cap);
  unsigned long long i = h % n, b = i, dist = 0;
  do {
    if (!set[i] || dist > tbl__dist(set[i],n,i)) {
      return cap;
    } else if(set[i] == h) {
      return casti(i);
    }
    dist++;
  } while ((i = ((i + 1) % n)) != b);
  return cap;
}
static int
tbl__del(unsigned long long* set, int cap, int *cnt, unsigned long long key) {
  assert(set);
  if (!cap) {
    return cap;
  }
  int i = tbl__fnd(set, cap, key);
  if (i < cap) {
    set[i] |= 0x8000000000000000llu;
    *cnt -= 1;
  }
  return i;
}
static int
tbl__nxt_idx(unsigned long long *keys, int cap, int i) {
  assert(keys);
  for (; i < cap; ++i) {
    if (tbl__key(keys[i])) {
      return i;
    }
  }
  return cap;
}

