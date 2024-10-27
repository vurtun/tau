static inline int
npow2(int x) {
  unsigned int v = castu(x);
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return casti(v);
}
/* ---------------------------------------------------------------------------
 *                                Foreach
 * ---------------------------------------------------------------------------
 */
#define forever while(1)
#define loopr(i,r) (int i = (r).lo; i != (r).hi; i += 1)
#define loopi(i,j,r) (int i = (r).lo, j = 0; i != (r).hi; i += 1, ++j)
#define loop(i,n) (int i = 0; i < (n); ++i)

/* ---------------------------------------------------------------------------
 *                                Memory
 * ---------------------------------------------------------------------------
 */
static inline void
mcpy(void* restrict dst, void const *restrict src, int n) {
  assert(dst);
  assert(src);
  assert(n >= 0);

  unsigned char *restrict d = dst;
  const unsigned char *restrict s = src;
  for loop(i, n) {
    d[i] = s[i];
  }
}
static inline void
mset(void *addr, int c, int n) {
  assert(addr);
  assert(n >= 0);
  unsigned char *dst = addr;
  for loop(i, n) {
    dst[i] = castb(c);
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
rng__bnd(int i, int n) {
  int l = max(n, 1) - 1;
  int v = (i < 0) ? (n - i) : i;
  return clamp(v, 0, l);
}
static force_inline struct rng
rng__mk(int lo, int hi, int n) {
  assert(lo <= hi);
  struct rng r = {.lo = lo, .hi = hi};
  r.cnt = abs(r.hi - r.lo);
  r.total = n;
  return r;
}
static force_inline struct rng
rng_sub(const struct rng *r, int b, int e) {
  struct rng ret = rng(b, e, r->total);
  rng_shft(&ret, r->lo);
  return ret;
}
static force_inline struct rng
rng_put(const struct rng *r, const struct rng *v) {
  struct rng ret = *v;
  rng_shft(&ret, r->lo);
  return ret;
}

/* ---------------------------------------------------------------------------
 *                                  Hash
 * ---------------------------------------------------------------------------
 */
#define FNV1A32_HASH_INITIAL 2166136261u
#define FNV1A64_HASH_INITIAL 14695981039346656037llu

static inline unsigned
fnv1a32(const void *ptr, int size, unsigned h) {
  const unsigned char *p = ptr;
  for loop(i,size) {
     h = (h ^ p[i]) * 16777619u;
  }
  return h;
}
static inline unsigned long long
fnv1a64(const void *ptr, int len, unsigned long long h) {
  const unsigned char *p = ptr;
  for loop(i,len) {
    h ^= (unsigned long long)p[i];
    h *= 1099511628211llu;
  }
  return h;
}
static unsigned
fnv1au32(unsigned h, unsigned id) {
  return fnv1a32(&id, sizeof(id), h);
}
static unsigned long long
fnv1au64(unsigned long long id, unsigned long long h) {
  return fnv1a64(&id, sizeof(id), h);
}
static unsigned long long
hash_ptr(const void *ptr) {
  return fnv1a64(&ptr, szof(void *), FNV1A64_HASH_INITIAL);
}
static unsigned long long
hash_int(long long d) {
  return fnv1a64(&d, szof(d), FNV1A64_HASH_INITIAL);
}
static unsigned long long
hash_lld(long long d) {
  return fnv1a64(&d, szof(d), FNV1A64_HASH_INITIAL);
}

/* ---------------------------------------------------------------------------
 *                                  Random
 * ---------------------------------------------------------------------------
 */
static inline unsigned long long
rnd_gen(unsigned long long x, int n) {
  return x + castull(n) * 0x9E3779B97F4A7C15llu;
}
static inline unsigned long long
rnd_mix(unsigned long long z) {
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9llu;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBllu;
  return z ^ (z >> 31llu);
}
static inline unsigned long long
rnd_split_mix(unsigned long long *x, int i) {
  *x = rnd_gen(*x, i);
  return rnd_mix(*x);
}
static inline unsigned long long
rnd(unsigned long long *x) {
  return rnd_split_mix(x, 1);
}
static inline unsigned
rndu(unsigned long long *x) {
  unsigned long long z = rnd(x);
  return castu(z & 0xffffffffu);
}
static inline int
rndi(unsigned long long *x) {
  unsigned z = rndu(x);
  long long n = castll(z) - (UINT_MAX/2);
  assert(n >= INT_MIN && n <= INT_MAX);
  return casti(n);
}
static inline double
rndn(unsigned long long *x) {
  unsigned n = rndu(x);
  return castd(n) / castd(UINT_MAX);
}
static unsigned
rnduu(unsigned long long *x, unsigned mini, unsigned maxi) {
  unsigned lo = min(mini, maxi);
  unsigned hi = max(mini, maxi);
  unsigned rng = castu(-1);
  unsigned n = hi - lo + 1u;
  if (n == 1u)  {
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
  float lo = min(mini, maxi);
  float hi = max(mini, maxi);
  unsigned u = rndu(x);
  float rng = hi - lo;
  double du = castd(u);
  double div = castd((unsigned)-1);
  return lo + rng * castf(du/div);
}

/* ---------------------------------------------------------------------------
 *                                  Guid
 * ---------------------------------------------------------------------------
 */
/* format: 00000000-0000-0000-0000-000000000000 */
#define GUID_STR_LEN 37
#define GUID_BUF_LEN (GUID_STR_LEN+1)
#define guid_hash32(g) ((g)->d1)
#define guid_eq(a,b) (a)->d1 == (b)->d1 && (a)->d2 == (b)->d2 && (a)->d3 == (b)->d3 && !memcmp((a)->d4, (b)->d4)

static inline void
guid_gen(struct guid *ret, struct sys *s, uintptr_t gen) {
  assert(s);
  assert(ret);
  s->rnd.gen128(gen, cast(void*,ret));
}
static inline int
guid__hex4(unsigned char *dst, int c) {
  assert(dst);
  if (c >= '0' && c <= '9') {
    *dst = castb(c-'0');
  } else if(c >= 'a' && c <= 'f') {
    *dst = castb(10 + c - 'a');
  } else if(c >= 'A' && c <= 'F') {
    *dst = castb(10 + c - 'A');
  } else {
    return 0;
  }
  return 1;
}
static inline int
guid__hex8(unsigned char *dst, const char *s) {
  assert(s);
  assert(dst);
  unsigned char v[2] = {0};
  if (!guid__hex4(v+1, s[0]) ||
      !guid__hex4(v+1, s[1])) {
    return 0;
  }
  *dst = castb((v[0] << 4) | v[1]);
  return 1;
}
static inline int
guid__hex16(unsigned short *dst, const char *s) {
  assert(s);
  assert(dst);
  unsigned char v[2];
  if (!guid__hex8(v+0, s + 0) ||
      !guid__hex8(v+1, s + 2)) {
    return 0;
  }
  *dst = castus((v[0] << 8) | v[1]);
  return 1;
}
static inline int
guid__hex32(unsigned *dst, const char *s) {
  assert(s);
  assert(dst);
  unsigned char v[4];
  if (!guid__hex8(v + 0, s + 0) ||
      !guid__hex8(v + 1, s + 2) ||
      !guid__hex8(v + 2, s + 4) ||
      !guid__hex8(v + 3, s + 6)) {
    return 0;
  }
  *dst = castu((v[0] << 24) | (v[1] << 16) | (v[2] << 8) | (v[3]));
  return 1;
}
static force_inline int
guid__sep(const char *s) {
  assert(s);
  return *s == '-';
}
static inline int
guid_str(struct guid *g, struct str gstr) {
  assert(g);
  if (gstr.rng.cnt < GUID_STR_LEN) {
    return 0;
  }
  const char *s = gstr.ptr + gstr.rng.lo;
  int ret =
    guid__hex32(&g->d1, s + 0) &&
    guid__sep(s + 8) &&
    guid__hex16(&g->d2, s + 9) &&
    guid__sep(s + 13) &&
    guid__hex16(&g->d3, s + 14) &&
    guid__sep(s + 18) &&
    guid__hex8(&g->d4[0], s + 19) &&
    guid__hex8(&g->d4[1], s + 21) &&
    guid__sep(s + 23) &&
    guid__hex8(&g->d4[2], s + 24) &&
    guid__hex8(&g->d4[3], s + 26) &&
    guid__hex8(&g->d4[4], s + 28) &&
    guid__hex8(&g->d4[5], s + 30) &&
    guid__hex8(&g->d4[6], s + 32) &&
    guid__hex8(&g->d4[7], s + 34);
  if (!ret) {
    return 0;
  }
  return ret;
}
static inline void
guid__put(char *dst, int byte) {
  static const char *hex_chars = "0123456789abcdef";
  dst[0] = hex_chars[byte >> 4];
  dst[1] = hex_chars[byte & 15];
}
static void
str_guid(char *buf, const struct guid *g) {
  assert(buf);
  assert(g);
  const char *src = (const char*)g;
  guid__put(buf + 0, src[3]);
  guid__put(buf + 2, src[2]);
  guid__put(buf + 4, src[1]);
  guid__put(buf + 6, src[0]);
  buf[8] = '-';
  guid__put(buf + 9, src[5]);
  guid__put(buf + 11, src[4]);
  buf[13] = '-';
  guid__put(buf + 14, src[7]);
  guid__put(buf + 16, src[6]);
  buf[18] = '-';
  guid__put(buf + 19, src[8]);
  guid__put(buf + 21, src[9]);
  buf[23] = '-';
  guid__put(buf + 24, src[10]);
  guid__put(buf + 26, src[11]);
  guid__put(buf + 28, src[12]);
  guid__put(buf + 30, src[13]);
  guid__put(buf + 32, src[14]);
  guid__put(buf + 34, src[15]);
}
static inline unsigned long long
guid_hash64(const struct guid *g) {
  unsigned long long v[2] = {0};
  compiler_assert(szof(v) == sizeof(*g));
  mcpy(v, g, szof(v));
  return fnv1au64(v[0], v[1]);
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
static inline unsigned
bit_rev32(unsigned x){
  x = ((x & 0x55555555) << 1) | ((x >> 1) & 0x55555555);
  x = ((x & 0x33333333) << 2) | ((x >> 2) & 0x33333333);
  x = ((x & 0x0F0F0F0F) << 4) | ((x >> 4) & 0x0F0F0F0F);
  x = (x << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24);
  return x;
}
static inline unsigned long long
bit_rev64(unsigned long long x){
  unsigned x1 = castu(x & 0xffffffffu);
  unsigned x2 = castu(x >> 32);
  return (castull(bit_rev32(x1)) << 32)|bit_rev32(x2);
}
static inline unsigned
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

static inline int
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
    unsigned long cmsk = bit_mask(off) - 1u;
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
    unsigned long cmsk = bit_mask(off) - 1u;
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
  unsigned long cmsk = max(1u, bit_mask(idx)) - 1u;
  if ((unsigned long)nbits < BITS_PER_LONG) {
    cmsk |= ~(max(1u, bit_mask(nbits)) - 1u);
  }
  unsigned long w = addr[widx] & ~cmsk;
  unsigned long long n = (unsigned long long)bits_to_long(nbits);
  int cnt = cpu_bit_cnt64(w);
  for (unsigned long i = widx + 1; i < n; ++i) {
    w = addr[i];
    if ((unsigned long)nbits - BITS_PER_LONG * i < BITS_PER_LONG) {
      cmsk |= ~(bit_mask(nbits) - 1u);
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
  unsigned long cmsk = max(1u, bit_mask(idx)) - 1u;
  if ((unsigned long)nbits < BITS_PER_LONG) {
    cmsk |= ~(max(1u, bit_mask(nbits)) - 1u);
  }
  unsigned long long n = (unsigned long long)bits_to_long(nbits);
  unsigned long w = addr[widx] & ~cmsk;
  int cnt = cpu_bit_cnt64(~w);
  for (unsigned long i = widx + 1; i < n; ++i) {
    w = addr[i];
    if ((unsigned long)nbits - BITS_PER_LONG * i < BITS_PER_LONG) {
      cmsk |= ~(bit_mask(nbits) - 1u);
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
enum {
  MAX_CASE_FOLD_RUNE_CNT = 3
};
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
static int
fold_case(long *out, long c) {
  switch(c) {
  case 0x0041: out[0] = 0x0061; return 1; /* LATIN CAPITAL LETTER A */
  case 0x0042: out[0] = 0x0062; return 1; /* LATIN CAPITAL LETTER B */
  case 0x0043: out[0] = 0x0063; return 1; /* LATIN CAPITAL LETTER C */
  case 0x0044: out[0] = 0x0064; return 1; /* LATIN CAPITAL LETTER D */
  case 0x0045: out[0] = 0x0065; return 1; /* LATIN CAPITAL LETTER E */
  case 0x0046: out[0] = 0x0066; return 1; /* LATIN CAPITAL LETTER F */
  case 0x0047: out[0] = 0x0067; return 1; /* LATIN CAPITAL LETTER G */
  case 0x0048: out[0] = 0x0068; return 1; /* LATIN CAPITAL LETTER H */
  case 0x0049: out[0] = 0x0069; return 1; /* LATIN CAPITAL LETTER I */
  case 0x004A: out[0] = 0x006A; return 1; /* LATIN CAPITAL LETTER J */
  case 0x004B: out[0] = 0x006B; return 1; /* LATIN CAPITAL LETTER K */
  case 0x004C: out[0] = 0x006C; return 1; /* LATIN CAPITAL LETTER L */
  case 0x004D: out[0] = 0x006D; return 1; /* LATIN CAPITAL LETTER M */
  case 0x004E: out[0] = 0x006E; return 1; /* LATIN CAPITAL LETTER N */
  case 0x004F: out[0] = 0x006F; return 1; /* LATIN CAPITAL LETTER O */
  case 0x0050: out[0] = 0x0070; return 1; /* LATIN CAPITAL LETTER P */
  case 0x0051: out[0] = 0x0071; return 1; /* LATIN CAPITAL LETTER Q */
  case 0x0052: out[0] = 0x0072; return 1; /* LATIN CAPITAL LETTER R */
  case 0x0053: out[0] = 0x0073; return 1; /* LATIN CAPITAL LETTER S */
  case 0x0054: out[0] = 0x0074; return 1; /* LATIN CAPITAL LETTER T */
  case 0x0055: out[0] = 0x0075; return 1; /* LATIN CAPITAL LETTER U */
  case 0x0056: out[0] = 0x0076; return 1; /* LATIN CAPITAL LETTER V */
  case 0x0057: out[0] = 0x0077; return 1; /* LATIN CAPITAL LETTER W */
  case 0x0058: out[0] = 0x0078; return 1; /* LATIN CAPITAL LETTER X */
  case 0x0059: out[0] = 0x0079; return 1; /* LATIN CAPITAL LETTER Y */
  case 0x005A: out[0] = 0x007A; return 1; /* LATIN CAPITAL LETTER Z */
  case 0x00B5: out[0] = 0x03BC; return 1; /* MICRO SIGN */
  case 0x00C0: out[0] = 0x00E0; return 1; /* LATIN CAPITAL LETTER A WITH GRAVE */
  case 0x00C1: out[0] = 0x00E1; return 1; /* LATIN CAPITAL LETTER A WITH ACUTE */
  case 0x00C2: out[0] = 0x00E2; return 1; /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
  case 0x00C3: out[0] = 0x00E3; return 1; /* LATIN CAPITAL LETTER A WITH TILDE */
  case 0x00C4: out[0] = 0x00E4; return 1; /* LATIN CAPITAL LETTER A WITH DIAERESIS */
  case 0x00C5: out[0] = 0x00E5; return 1; /* LATIN CAPITAL LETTER A WITH RING ABOVE */
  case 0x00C6: out[0] = 0x00E6; return 1; /* LATIN CAPITAL LETTER AE */
  case 0x00C7: out[0] = 0x00E7; return 1; /* LATIN CAPITAL LETTER C WITH CEDILLA */
  case 0x00C8: out[0] = 0x00E8; return 1; /* LATIN CAPITAL LETTER E WITH GRAVE */
  case 0x00C9: out[0] = 0x00E9; return 1; /* LATIN CAPITAL LETTER E WITH ACUTE */
  case 0x00CA: out[0] = 0x00EA; return 1; /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
  case 0x00CB: out[0] = 0x00EB; return 1; /* LATIN CAPITAL LETTER E WITH DIAERESIS */
  case 0x00CC: out[0] = 0x00EC; return 1; /* LATIN CAPITAL LETTER I WITH GRAVE */
  case 0x00CD: out[0] = 0x00ED; return 1; /* LATIN CAPITAL LETTER I WITH ACUTE */
  case 0x00CE: out[0] = 0x00EE; return 1; /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
  case 0x00CF: out[0] = 0x00EF; return 1; /* LATIN CAPITAL LETTER I WITH DIAERESIS */
  case 0x00D0: out[0] = 0x00F0; return 1; /* LATIN CAPITAL LETTER ETH */
  case 0x00D1: out[0] = 0x00F1; return 1; /* LATIN CAPITAL LETTER N WITH TILDE */
  case 0x00D2: out[0] = 0x00F2; return 1; /* LATIN CAPITAL LETTER O WITH GRAVE */
  case 0x00D3: out[0] = 0x00F3; return 1; /* LATIN CAPITAL LETTER O WITH ACUTE */
  case 0x00D4: out[0] = 0x00F4; return 1; /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
  case 0x00D5: out[0] = 0x00F5; return 1; /* LATIN CAPITAL LETTER O WITH TILDE */
  case 0x00D6: out[0] = 0x00F6; return 1; /* LATIN CAPITAL LETTER O WITH DIAERESIS */
  case 0x00D8: out[0] = 0x00F8; return 1; /* LATIN CAPITAL LETTER O WITH STROKE */
  case 0x00D9: out[0] = 0x00F9; return 1; /* LATIN CAPITAL LETTER U WITH GRAVE */
  case 0x00DA: out[0] = 0x00FA; return 1; /* LATIN CAPITAL LETTER U WITH ACUTE */
  case 0x00DB: out[0] = 0x00FB; return 1; /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
  case 0x00DC: out[0] = 0x00FC; return 1; /* LATIN CAPITAL LETTER U WITH DIAERESIS */
  case 0x00DD: out[0] = 0x00FD; return 1; /* LATIN CAPITAL LETTER Y WITH ACUTE */
  case 0x00DE: out[0] = 0x00FE; return 1; /* LATIN CAPITAL LETTER THORN */
  case 0x00DF: out[0] = 0x0073; out[1] = 0x0073; return 2; /* LATIN SMALL LETTER SHARP S */
  case 0x0100: out[0] = 0x0101; return 1; /* LATIN CAPITAL LETTER A WITH MACRON */
  case 0x0102: out[0] = 0x0103; return 1; /* LATIN CAPITAL LETTER A WITH BREVE */
  case 0x0104: out[0] = 0x0105; return 1; /* LATIN CAPITAL LETTER A WITH OGONEK */
  case 0x0106: out[0] = 0x0107; return 1; /* LATIN CAPITAL LETTER C WITH ACUTE */
  case 0x0108: out[0] = 0x0109; return 1; /* LATIN CAPITAL LETTER C WITH CIRCUMFLEX */
  case 0x010A: out[0] = 0x010B; return 1; /* LATIN CAPITAL LETTER C WITH DOT ABOVE */
  case 0x010C: out[0] = 0x010D; return 1; /* LATIN CAPITAL LETTER C WITH CARON */
  case 0x010E: out[0] = 0x010F; return 1; /* LATIN CAPITAL LETTER D WITH CARON */
  case 0x0110: out[0] = 0x0111; return 1; /* LATIN CAPITAL LETTER D WITH STROKE */
  case 0x0112: out[0] = 0x0113; return 1; /* LATIN CAPITAL LETTER E WITH MACRON */
  case 0x0114: out[0] = 0x0115; return 1; /* LATIN CAPITAL LETTER E WITH BREVE */
  case 0x0116: out[0] = 0x0117; return 1; /* LATIN CAPITAL LETTER E WITH DOT ABOVE */
  case 0x0118: out[0] = 0x0119; return 1; /* LATIN CAPITAL LETTER E WITH OGONEK */
  case 0x011A: out[0] = 0x011B; return 1; /* LATIN CAPITAL LETTER E WITH CARON */
  case 0x011C: out[0] = 0x011D; return 1; /* LATIN CAPITAL LETTER G WITH CIRCUMFLEX */
  case 0x011E: out[0] = 0x011F; return 1; /* LATIN CAPITAL LETTER G WITH BREVE */
  case 0x0120: out[0] = 0x0121; return 1; /* LATIN CAPITAL LETTER G WITH DOT ABOVE */
  case 0x0122: out[0] = 0x0123; return 1; /* LATIN CAPITAL LETTER G WITH CEDILLA */
  case 0x0124: out[0] = 0x0125; return 1; /* LATIN CAPITAL LETTER H WITH CIRCUMFLEX */
  case 0x0126: out[0] = 0x0127; return 1; /* LATIN CAPITAL LETTER H WITH STROKE */
  case 0x0128: out[0] = 0x0129; return 1; /* LATIN CAPITAL LETTER I WITH TILDE */
  case 0x012A: out[0] = 0x012B; return 1; /* LATIN CAPITAL LETTER I WITH MACRON */
  case 0x012C: out[0] = 0x012D; return 1; /* LATIN CAPITAL LETTER I WITH BREVE */
  case 0x012E: out[0] = 0x012F; return 1; /* LATIN CAPITAL LETTER I WITH OGONEK */
  case 0x0130: out[0] = 0x0069; out[1] = 0x0307; return 2; /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
  case 0x0132: out[0] = 0x0133; return 1; /* LATIN CAPITAL LIGATURE IJ */
  case 0x0134: out[0] = 0x0135; return 1; /* LATIN CAPITAL LETTER J WITH CIRCUMFLEX */
  case 0x0136: out[0] = 0x0137; return 1; /* LATIN CAPITAL LETTER K WITH CEDILLA */
  case 0x0139: out[0] = 0x013A; return 1; /* LATIN CAPITAL LETTER L WITH ACUTE */
  case 0x013B: out[0] = 0x013C; return 1; /* LATIN CAPITAL LETTER L WITH CEDILLA */
  case 0x013D: out[0] = 0x013E; return 1; /* LATIN CAPITAL LETTER L WITH CARON */
  case 0x013F: out[0] = 0x0140; return 1; /* LATIN CAPITAL LETTER L WITH MIDDLE DOT */
  case 0x0141: out[0] = 0x0142; return 1; /* LATIN CAPITAL LETTER L WITH STROKE */
  case 0x0143: out[0] = 0x0144; return 1; /* LATIN CAPITAL LETTER N WITH ACUTE */
  case 0x0145: out[0] = 0x0146; return 1; /* LATIN CAPITAL LETTER N WITH CEDILLA */
  case 0x0147: out[0] = 0x0148; return 1; /* LATIN CAPITAL LETTER N WITH CARON */
  case 0x0149: out[0] = 0x02BC; out[1] = 0x006E; return 2; /* LATIN SMALL LETTER N PRECEDED BY APOSTROPHE */
  case 0x014A: out[0] = 0x014B; return 1; /* LATIN CAPITAL LETTER ENG */
  case 0x014C: out[0] = 0x014D; return 1; /* LATIN CAPITAL LETTER O WITH MACRON */
  case 0x014E: out[0] = 0x014F; return 1; /* LATIN CAPITAL LETTER O WITH BREVE */
  case 0x0150: out[0] = 0x0151; return 1; /* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
  case 0x0152: out[0] = 0x0153; return 1; /* LATIN CAPITAL LIGATURE OE */
  case 0x0154: out[0] = 0x0155; return 1; /* LATIN CAPITAL LETTER R WITH ACUTE */
  case 0x0156: out[0] = 0x0157; return 1; /* LATIN CAPITAL LETTER R WITH CEDILLA */
  case 0x0158: out[0] = 0x0159; return 1; /* LATIN CAPITAL LETTER R WITH CARON */
  case 0x015A: out[0] = 0x015B; return 1; /* LATIN CAPITAL LETTER S WITH ACUTE */
  case 0x015C: out[0] = 0x015D; return 1; /* LATIN CAPITAL LETTER S WITH CIRCUMFLEX */
  case 0x015E: out[0] = 0x015F; return 1; /* LATIN CAPITAL LETTER S WITH CEDILLA */
  case 0x0160: out[0] = 0x0161; return 1; /* LATIN CAPITAL LETTER S WITH CARON */
  case 0x0162: out[0] = 0x0163; return 1; /* LATIN CAPITAL LETTER T WITH CEDILLA */
  case 0x0164: out[0] = 0x0165; return 1; /* LATIN CAPITAL LETTER T WITH CARON */
  case 0x0166: out[0] = 0x0167; return 1; /* LATIN CAPITAL LETTER T WITH STROKE */
  case 0x0168: out[0] = 0x0169; return 1; /* LATIN CAPITAL LETTER U WITH TILDE */
  case 0x016A: out[0] = 0x016B; return 1; /* LATIN CAPITAL LETTER U WITH MACRON */
  case 0x016C: out[0] = 0x016D; return 1; /* LATIN CAPITAL LETTER U WITH BREVE */
  case 0x016E: out[0] = 0x016F; return 1; /* LATIN CAPITAL LETTER U WITH RING ABOVE */
  case 0x0170: out[0] = 0x0171; return 1; /* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
  case 0x0172: out[0] = 0x0173; return 1; /* LATIN CAPITAL LETTER U WITH OGONEK */
  case 0x0174: out[0] = 0x0175; return 1; /* LATIN CAPITAL LETTER W WITH CIRCUMFLEX */
  case 0x0176: out[0] = 0x0177; return 1; /* LATIN CAPITAL LETTER Y WITH CIRCUMFLEX */
  case 0x0178: out[0] = 0x00FF; return 1; /* LATIN CAPITAL LETTER Y WITH DIAERESIS */
  case 0x0179: out[0] = 0x017A; return 1; /* LATIN CAPITAL LETTER Z WITH ACUTE */
  case 0x017B: out[0] = 0x017C; return 1; /* LATIN CAPITAL LETTER Z WITH DOT ABOVE */
  case 0x017D: out[0] = 0x017E; return 1; /* LATIN CAPITAL LETTER Z WITH CARON */
  case 0x017F: out[0] = 0x0073; return 1; /* LATIN SMALL LETTER LONG S */
  case 0x0181: out[0] = 0x0253; return 1; /* LATIN CAPITAL LETTER B WITH HOOK */
  case 0x0182: out[0] = 0x0183; return 1; /* LATIN CAPITAL LETTER B WITH TOPBAR */
  case 0x0184: out[0] = 0x0185; return 1; /* LATIN CAPITAL LETTER TONE SIX */
  case 0x0186: out[0] = 0x0254; return 1; /* LATIN CAPITAL LETTER OPEN O */
  case 0x0187: out[0] = 0x0188; return 1; /* LATIN CAPITAL LETTER C WITH HOOK */
  case 0x0189: out[0] = 0x0256; return 1; /* LATIN CAPITAL LETTER AFRICAN D */
  case 0x018A: out[0] = 0x0257; return 1; /* LATIN CAPITAL LETTER D WITH HOOK */
  case 0x018B: out[0] = 0x018C; return 1; /* LATIN CAPITAL LETTER D WITH TOPBAR */
  case 0x018E: out[0] = 0x01DD; return 1; /* LATIN CAPITAL LETTER REVERSED E */
  case 0x018F: out[0] = 0x0259; return 1; /* LATIN CAPITAL LETTER SCHWA */
  case 0x0190: out[0] = 0x025B; return 1; /* LATIN CAPITAL LETTER OPEN E */
  case 0x0191: out[0] = 0x0192; return 1; /* LATIN CAPITAL LETTER F WITH HOOK */
  case 0x0193: out[0] = 0x0260; return 1; /* LATIN CAPITAL LETTER G WITH HOOK */
  case 0x0194: out[0] = 0x0263; return 1; /* LATIN CAPITAL LETTER GAMMA */
  case 0x0196: out[0] = 0x0269; return 1; /* LATIN CAPITAL LETTER IOTA */
  case 0x0197: out[0] = 0x0268; return 1; /* LATIN CAPITAL LETTER I WITH STROKE */
  case 0x0198: out[0] = 0x0199; return 1; /* LATIN CAPITAL LETTER K WITH HOOK */
  case 0x019C: out[0] = 0x026F; return 1; /* LATIN CAPITAL LETTER TURNED M */
  case 0x019D: out[0] = 0x0272; return 1; /* LATIN CAPITAL LETTER N WITH LEFT HOOK */
  case 0x019F: out[0] = 0x0275; return 1; /* LATIN CAPITAL LETTER O WITH MIDDLE TILDE */
  case 0x01A0: out[0] = 0x01A1; return 1; /* LATIN CAPITAL LETTER O WITH HORN */
  case 0x01A2: out[0] = 0x01A3; return 1; /* LATIN CAPITAL LETTER OI */
  case 0x01A4: out[0] = 0x01A5; return 1; /* LATIN CAPITAL LETTER P WITH HOOK */
  case 0x01A6: out[0] = 0x0280; return 1; /* LATIN LETTER YR */
  case 0x01A7: out[0] = 0x01A8; return 1; /* LATIN CAPITAL LETTER TONE TWO */
  case 0x01A9: out[0] = 0x0283; return 1; /* LATIN CAPITAL LETTER ESH */
  case 0x01AC: out[0] = 0x01AD; return 1; /* LATIN CAPITAL LETTER T WITH HOOK */
  case 0x01AE: out[0] = 0x0288; return 1; /* LATIN CAPITAL LETTER T WITH RETROFLEX HOOK */
  case 0x01AF: out[0] = 0x01B0; return 1; /* LATIN CAPITAL LETTER U WITH HORN */
  case 0x01B1: out[0] = 0x028A; return 1; /* LATIN CAPITAL LETTER UPSILON */
  case 0x01B2: out[0] = 0x028B; return 1; /* LATIN CAPITAL LETTER V WITH HOOK */
  case 0x01B3: out[0] = 0x01B4; return 1; /* LATIN CAPITAL LETTER Y WITH HOOK */
  case 0x01B5: out[0] = 0x01B6; return 1; /* LATIN CAPITAL LETTER Z WITH STROKE */
  case 0x01B7: out[0] = 0x0292; return 1; /* LATIN CAPITAL LETTER EZH */
  case 0x01B8: out[0] = 0x01B9; return 1; /* LATIN CAPITAL LETTER EZH REVERSED */
  case 0x01BC: out[0] = 0x01BD; return 1; /* LATIN CAPITAL LETTER TONE FIVE */
  case 0x01C4: out[0] = 0x01C6; return 1; /* LATIN CAPITAL LETTER DZ WITH CARON */
  case 0x01C5: out[0] = 0x01C6; return 1; /* LATIN CAPITAL LETTER D WITH SMALL LETTER Z WITH CARON */
  case 0x01C7: out[0] = 0x01C9; return 1; /* LATIN CAPITAL LETTER LJ */
  case 0x01C8: out[0] = 0x01C9; return 1; /* LATIN CAPITAL LETTER L WITH SMALL LETTER J */
  case 0x01CA: out[0] = 0x01CC; return 1; /* LATIN CAPITAL LETTER NJ */
  case 0x01CB: out[0] = 0x01CC; return 1; /* LATIN CAPITAL LETTER N WITH SMALL LETTER J */
  case 0x01CD: out[0] = 0x01CE; return 1; /* LATIN CAPITAL LETTER A WITH CARON */
  case 0x01CF: out[0] = 0x01D0; return 1; /* LATIN CAPITAL LETTER I WITH CARON */
  case 0x01D1: out[0] = 0x01D2; return 1; /* LATIN CAPITAL LETTER O WITH CARON */
  case 0x01D3: out[0] = 0x01D4; return 1; /* LATIN CAPITAL LETTER U WITH CARON */
  case 0x01D5: out[0] = 0x01D6; return 1; /* LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON */
  case 0x01D7: out[0] = 0x01D8; return 1; /* LATIN CAPITAL LETTER U WITH DIAERESIS AND ACUTE */
  case 0x01D9: out[0] = 0x01DA; return 1; /* LATIN CAPITAL LETTER U WITH DIAERESIS AND CARON */
  case 0x01DB: out[0] = 0x01DC; return 1; /* LATIN CAPITAL LETTER U WITH DIAERESIS AND GRAVE */
  case 0x01DE: out[0] = 0x01DF; return 1; /* LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON */
  case 0x01E0: out[0] = 0x01E1; return 1; /* LATIN CAPITAL LETTER A WITH DOT ABOVE AND MACRON */
  case 0x01E2: out[0] = 0x01E3; return 1; /* LATIN CAPITAL LETTER AE WITH MACRON */
  case 0x01E4: out[0] = 0x01E5; return 1; /* LATIN CAPITAL LETTER G WITH STROKE */
  case 0x01E6: out[0] = 0x01E7; return 1; /* LATIN CAPITAL LETTER G WITH CARON */
  case 0x01E8: out[0] = 0x01E9; return 1; /* LATIN CAPITAL LETTER K WITH CARON */
  case 0x01EA: out[0] = 0x01EB; return 1; /* LATIN CAPITAL LETTER O WITH OGONEK */
  case 0x01EC: out[0] = 0x01ED; return 1; /* LATIN CAPITAL LETTER O WITH OGONEK AND MACRON */
  case 0x01EE: out[0] = 0x01EF; return 1; /* LATIN CAPITAL LETTER EZH WITH CARON */
  case 0x01F0: out[0] = 0x006A; out[1] = 0x030C; return 2; /* LATIN SMALL LETTER J WITH CARON */
  case 0x01F1: out[0] = 0x01F3; return 1; /* LATIN CAPITAL LETTER DZ */
  case 0x01F2: out[0] = 0x01F3; return 1; /* LATIN CAPITAL LETTER D WITH SMALL LETTER Z */
  case 0x01F4: out[0] = 0x01F5; return 1; /* LATIN CAPITAL LETTER G WITH ACUTE */
  case 0x01F6: out[0] = 0x0195; return 1; /* LATIN CAPITAL LETTER HWAIR */
  case 0x01F7: out[0] = 0x01BF; return 1; /* LATIN CAPITAL LETTER WYNN */
  case 0x01F8: out[0] = 0x01F9; return 1; /* LATIN CAPITAL LETTER N WITH GRAVE */
  case 0x01FA: out[0] = 0x01FB; return 1; /* LATIN CAPITAL LETTER A WITH RING ABOVE AND ACUTE */
  case 0x01FC: out[0] = 0x01FD; return 1; /* LATIN CAPITAL LETTER AE WITH ACUTE */
  case 0x01FE: out[0] = 0x01FF; return 1; /* LATIN CAPITAL LETTER O WITH STROKE AND ACUTE */
  case 0x0200: out[0] = 0x0201; return 1; /* LATIN CAPITAL LETTER A WITH DOUBLE GRAVE */
  case 0x0202: out[0] = 0x0203; return 1; /* LATIN CAPITAL LETTER A WITH INVERTED BREVE */
  case 0x0204: out[0] = 0x0205; return 1; /* LATIN CAPITAL LETTER E WITH DOUBLE GRAVE */
  case 0x0206: out[0] = 0x0207; return 1; /* LATIN CAPITAL LETTER E WITH INVERTED BREVE */
  case 0x0208: out[0] = 0x0209; return 1; /* LATIN CAPITAL LETTER I WITH DOUBLE GRAVE */
  case 0x020A: out[0] = 0x020B; return 1; /* LATIN CAPITAL LETTER I WITH INVERTED BREVE */
  case 0x020C: out[0] = 0x020D; return 1; /* LATIN CAPITAL LETTER O WITH DOUBLE GRAVE */
  case 0x020E: out[0] = 0x020F; return 1; /* LATIN CAPITAL LETTER O WITH INVERTED BREVE */
  case 0x0210: out[0] = 0x0211; return 1; /* LATIN CAPITAL LETTER R WITH DOUBLE GRAVE */
  case 0x0212: out[0] = 0x0213; return 1; /* LATIN CAPITAL LETTER R WITH INVERTED BREVE */
  case 0x0214: out[0] = 0x0215; return 1; /* LATIN CAPITAL LETTER U WITH DOUBLE GRAVE */
  case 0x0216: out[0] = 0x0217; return 1; /* LATIN CAPITAL LETTER U WITH INVERTED BREVE */
  case 0x0218: out[0] = 0x0219; return 1; /* LATIN CAPITAL LETTER S WITH COMMA BELOW */
  case 0x021A: out[0] = 0x021B; return 1; /* LATIN CAPITAL LETTER T WITH COMMA BELOW */
  case 0x021C: out[0] = 0x021D; return 1; /* LATIN CAPITAL LETTER YOGH */
  case 0x021E: out[0] = 0x021F; return 1; /* LATIN CAPITAL LETTER H WITH CARON */
  case 0x0220: out[0] = 0x019E; return 1; /* LATIN CAPITAL LETTER N WITH LONG RIGHT LEG */
  case 0x0222: out[0] = 0x0223; return 1; /* LATIN CAPITAL LETTER OU */
  case 0x0224: out[0] = 0x0225; return 1; /* LATIN CAPITAL LETTER Z WITH HOOK */
  case 0x0226: out[0] = 0x0227; return 1; /* LATIN CAPITAL LETTER A WITH DOT ABOVE */
  case 0x0228: out[0] = 0x0229; return 1; /* LATIN CAPITAL LETTER E WITH CEDILLA */
  case 0x022A: out[0] = 0x022B; return 1; /* LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON */
  case 0x022C: out[0] = 0x022D; return 1; /* LATIN CAPITAL LETTER O WITH TILDE AND MACRON */
  case 0x022E: out[0] = 0x022F; return 1; /* LATIN CAPITAL LETTER O WITH DOT ABOVE */
  case 0x0230: out[0] = 0x0231; return 1; /* LATIN CAPITAL LETTER O WITH DOT ABOVE AND MACRON */
  case 0x0232: out[0] = 0x0233; return 1; /* LATIN CAPITAL LETTER Y WITH MACRON */
  case 0x023A: out[0] = 0x2C65; return 1; /* LATIN CAPITAL LETTER A WITH STROKE */
  case 0x023B: out[0] = 0x023C; return 1; /* LATIN CAPITAL LETTER C WITH STROKE */
  case 0x023D: out[0] = 0x019A; return 1; /* LATIN CAPITAL LETTER L WITH BAR */
  case 0x023E: out[0] = 0x2C66; return 1; /* LATIN CAPITAL LETTER T WITH DIAGONAL STROKE */
  case 0x0241: out[0] = 0x0242; return 1; /* LATIN CAPITAL LETTER GLOTTAL STOP */
  case 0x0243: out[0] = 0x0180; return 1; /* LATIN CAPITAL LETTER B WITH STROKE */
  case 0x0244: out[0] = 0x0289; return 1; /* LATIN CAPITAL LETTER U BAR */
  case 0x0245: out[0] = 0x028C; return 1; /* LATIN CAPITAL LETTER TURNED V */
  case 0x0246: out[0] = 0x0247; return 1; /* LATIN CAPITAL LETTER E WITH STROKE */
  case 0x0248: out[0] = 0x0249; return 1; /* LATIN CAPITAL LETTER J WITH STROKE */
  case 0x024A: out[0] = 0x024B; return 1; /* LATIN CAPITAL LETTER SMALL Q WITH HOOK TAIL */
  case 0x024C: out[0] = 0x024D; return 1; /* LATIN CAPITAL LETTER R WITH STROKE */
  case 0x024E: out[0] = 0x024F; return 1; /* LATIN CAPITAL LETTER Y WITH STROKE */
  case 0x0345: out[0] = 0x03B9; return 1; /* COMBINING GREEK YPOGEGRAMMENI */
  case 0x0370: out[0] = 0x0371; return 1; /* GREEK CAPITAL LETTER HETA */
  case 0x0372: out[0] = 0x0373; return 1; /* GREEK CAPITAL LETTER ARCHAIC SAMPI */
  case 0x0376: out[0] = 0x0377; return 1; /* GREEK CAPITAL LETTER PAMPHYLIAN DIGAMMA */
  case 0x037F: out[0] = 0x03F3; return 1; /* GREEK CAPITAL LETTER YOT */
  case 0x0386: out[0] = 0x03AC; return 1; /* GREEK CAPITAL LETTER ALPHA WITH TONOS */
  case 0x0388: out[0] = 0x03AD; return 1; /* GREEK CAPITAL LETTER EPSILON WITH TONOS */
  case 0x0389: out[0] = 0x03AE; return 1; /* GREEK CAPITAL LETTER ETA WITH TONOS */
  case 0x038A: out[0] = 0x03AF; return 1; /* GREEK CAPITAL LETTER IOTA WITH TONOS */
  case 0x038C: out[0] = 0x03CC; return 1; /* GREEK CAPITAL LETTER OMICRON WITH TONOS */
  case 0x038E: out[0] = 0x03CD; return 1; /* GREEK CAPITAL LETTER UPSILON WITH TONOS */
  case 0x038F: out[0] = 0x03CE; return 1; /* GREEK CAPITAL LETTER OMEGA WITH TONOS */
  case 0x0390: out[0] = 0x03B9; out[1] = 0x0308; out[2] = 0x0301; return 3; /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
  case 0x0391: out[0] = 0x03B1; return 1; /* GREEK CAPITAL LETTER ALPHA */
  case 0x0392: out[0] = 0x03B2; return 1; /* GREEK CAPITAL LETTER BETA */
  case 0x0393: out[0] = 0x03B3; return 1; /* GREEK CAPITAL LETTER GAMMA */
  case 0x0394: out[0] = 0x03B4; return 1; /* GREEK CAPITAL LETTER DELTA */
  case 0x0395: out[0] = 0x03B5; return 1; /* GREEK CAPITAL LETTER EPSILON */
  case 0x0396: out[0] = 0x03B6; return 1; /* GREEK CAPITAL LETTER ZETA */
  case 0x0397: out[0] = 0x03B7; return 1; /* GREEK CAPITAL LETTER ETA */
  case 0x0398: out[0] = 0x03B8; return 1; /* GREEK CAPITAL LETTER THETA */
  case 0x0399: out[0] = 0x03B9; return 1; /* GREEK CAPITAL LETTER IOTA */
  case 0x039A: out[0] = 0x03BA; return 1; /* GREEK CAPITAL LETTER KAPPA */
  case 0x039B: out[0] = 0x03BB; return 1; /* GREEK CAPITAL LETTER LAMDA */
  case 0x039C: out[0] = 0x03BC; return 1; /* GREEK CAPITAL LETTER MU */
  case 0x039D: out[0] = 0x03BD; return 1; /* GREEK CAPITAL LETTER NU */
  case 0x039E: out[0] = 0x03BE; return 1; /* GREEK CAPITAL LETTER XI */
  case 0x039F: out[0] = 0x03BF; return 1; /* GREEK CAPITAL LETTER OMICRON */
  case 0x03A0: out[0] = 0x03C0; return 1; /* GREEK CAPITAL LETTER PI */
  case 0x03A1: out[0] = 0x03C1; return 1; /* GREEK CAPITAL LETTER RHO */
  case 0x03A3: out[0] = 0x03C3; return 1; /* GREEK CAPITAL LETTER SIGMA */
  case 0x03A4: out[0] = 0x03C4; return 1; /* GREEK CAPITAL LETTER TAU */
  case 0x03A5: out[0] = 0x03C5; return 1; /* GREEK CAPITAL LETTER UPSILON */
  case 0x03A6: out[0] = 0x03C6; return 1; /* GREEK CAPITAL LETTER PHI */
  case 0x03A7: out[0] = 0x03C7; return 1; /* GREEK CAPITAL LETTER CHI */
  case 0x03A8: out[0] = 0x03C8; return 1; /* GREEK CAPITAL LETTER PSI */
  case 0x03A9: out[0] = 0x03C9; return 1; /* GREEK CAPITAL LETTER OMEGA */
  case 0x03AA: out[0] = 0x03CA; return 1; /* GREEK CAPITAL LETTER IOTA WITH DIALYTIKA */
  case 0x03AB: out[0] = 0x03CB; return 1; /* GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA */
  case 0x03B0: out[0] = 0x03C5; out[1] = 0x0308; out[2] = 0x0301; return 3; /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS */
  case 0x03C2: out[0] = 0x03C3; return 1; /* GREEK SMALL LETTER FINAL SIGMA */
  case 0x03CF: out[0] = 0x03D7; return 1; /* GREEK CAPITAL KAI SYMBOL */
  case 0x03D0: out[0] = 0x03B2; return 1; /* GREEK BETA SYMBOL */
  case 0x03D1: out[0] = 0x03B8; return 1; /* GREEK THETA SYMBOL */
  case 0x03D5: out[0] = 0x03C6; return 1; /* GREEK PHI SYMBOL */
  case 0x03D6: out[0] = 0x03C0; return 1; /* GREEK PI SYMBOL */
  case 0x03D8: out[0] = 0x03D9; return 1; /* GREEK LETTER ARCHAIC KOPPA */
  case 0x03DA: out[0] = 0x03DB; return 1; /* GREEK LETTER STIGMA */
  case 0x03DC: out[0] = 0x03DD; return 1; /* GREEK LETTER DIGAMMA */
  case 0x03DE: out[0] = 0x03DF; return 1; /* GREEK LETTER KOPPA */
  case 0x03E0: out[0] = 0x03E1; return 1; /* GREEK LETTER SAMPI */
  case 0x03E2: out[0] = 0x03E3; return 1; /* COPTIC CAPITAL LETTER SHEI */
  case 0x03E4: out[0] = 0x03E5; return 1; /* COPTIC CAPITAL LETTER FEI */
  case 0x03E6: out[0] = 0x03E7; return 1; /* COPTIC CAPITAL LETTER KHEI */
  case 0x03E8: out[0] = 0x03E9; return 1; /* COPTIC CAPITAL LETTER HORI */
  case 0x03EA: out[0] = 0x03EB; return 1; /* COPTIC CAPITAL LETTER GANGIA */
  case 0x03EC: out[0] = 0x03ED; return 1; /* COPTIC CAPITAL LETTER SHIMA */
  case 0x03EE: out[0] = 0x03EF; return 1; /* COPTIC CAPITAL LETTER DEI */
  case 0x03F0: out[0] = 0x03BA; return 1; /* GREEK KAPPA SYMBOL */
  case 0x03F1: out[0] = 0x03C1; return 1; /* GREEK RHO SYMBOL */
  case 0x03F4: out[0] = 0x03B8; return 1; /* GREEK CAPITAL THETA SYMBOL */
  case 0x03F5: out[0] = 0x03B5; return 1; /* GREEK LUNATE EPSILON SYMBOL */
  case 0x03F7: out[0] = 0x03F8; return 1; /* GREEK CAPITAL LETTER SHO */
  case 0x03F9: out[0] = 0x03F2; return 1; /* GREEK CAPITAL LUNATE SIGMA SYMBOL */
  case 0x03FA: out[0] = 0x03FB; return 1; /* GREEK CAPITAL LETTER SAN */
  case 0x03FD: out[0] = 0x037B; return 1; /* GREEK CAPITAL REVERSED LUNATE SIGMA SYMBOL */
  case 0x03FE: out[0] = 0x037C; return 1; /* GREEK CAPITAL DOTTED LUNATE SIGMA SYMBOL */
  case 0x03FF: out[0] = 0x037D; return 1; /* GREEK CAPITAL REVERSED DOTTED LUNATE SIGMA SYMBOL */
  case 0x0400: out[0] = 0x0450; return 1; /* CYRILLIC CAPITAL LETTER IE WITH GRAVE */
  case 0x0401: out[0] = 0x0451; return 1; /* CYRILLIC CAPITAL LETTER IO */
  case 0x0402: out[0] = 0x0452; return 1; /* CYRILLIC CAPITAL LETTER DJE */
  case 0x0403: out[0] = 0x0453; return 1; /* CYRILLIC CAPITAL LETTER GJE */
  case 0x0404: out[0] = 0x0454; return 1; /* CYRILLIC CAPITAL LETTER UKRAINIAN IE */
  case 0x0405: out[0] = 0x0455; return 1; /* CYRILLIC CAPITAL LETTER DZE */
  case 0x0406: out[0] = 0x0456; return 1; /* CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
  case 0x0407: out[0] = 0x0457; return 1; /* CYRILLIC CAPITAL LETTER YI */
  case 0x0408: out[0] = 0x0458; return 1; /* CYRILLIC CAPITAL LETTER JE */
  case 0x0409: out[0] = 0x0459; return 1; /* CYRILLIC CAPITAL LETTER LJE */
  case 0x040A: out[0] = 0x045A; return 1; /* CYRILLIC CAPITAL LETTER NJE */
  case 0x040B: out[0] = 0x045B; return 1; /* CYRILLIC CAPITAL LETTER TSHE */
  case 0x040C: out[0] = 0x045C; return 1; /* CYRILLIC CAPITAL LETTER KJE */
  case 0x040D: out[0] = 0x045D; return 1; /* CYRILLIC CAPITAL LETTER I WITH GRAVE */
  case 0x040E: out[0] = 0x045E; return 1; /* CYRILLIC CAPITAL LETTER SHORT U */
  case 0x040F: out[0] = 0x045F; return 1; /* CYRILLIC CAPITAL LETTER DZHE */
  case 0x0410: out[0] = 0x0430; return 1; /* CYRILLIC CAPITAL LETTER A */
  case 0x0411: out[0] = 0x0431; return 1; /* CYRILLIC CAPITAL LETTER BE */
  case 0x0412: out[0] = 0x0432; return 1; /* CYRILLIC CAPITAL LETTER VE */
  case 0x0413: out[0] = 0x0433; return 1; /* CYRILLIC CAPITAL LETTER GHE */
  case 0x0414: out[0] = 0x0434; return 1; /* CYRILLIC CAPITAL LETTER DE */
  case 0x0415: out[0] = 0x0435; return 1; /* CYRILLIC CAPITAL LETTER IE */
  case 0x0416: out[0] = 0x0436; return 1; /* CYRILLIC CAPITAL LETTER ZHE */
  case 0x0417: out[0] = 0x0437; return 1; /* CYRILLIC CAPITAL LETTER ZE */
  case 0x0418: out[0] = 0x0438; return 1; /* CYRILLIC CAPITAL LETTER I */
  case 0x0419: out[0] = 0x0439; return 1; /* CYRILLIC CAPITAL LETTER SHORT I */
  case 0x041A: out[0] = 0x043A; return 1; /* CYRILLIC CAPITAL LETTER KA */
  case 0x041B: out[0] = 0x043B; return 1; /* CYRILLIC CAPITAL LETTER EL */
  case 0x041C: out[0] = 0x043C; return 1; /* CYRILLIC CAPITAL LETTER EM */
  case 0x041D: out[0] = 0x043D; return 1; /* CYRILLIC CAPITAL LETTER EN */
  case 0x041E: out[0] = 0x043E; return 1; /* CYRILLIC CAPITAL LETTER O */
  case 0x041F: out[0] = 0x043F; return 1; /* CYRILLIC CAPITAL LETTER PE */
  case 0x0420: out[0] = 0x0440; return 1; /* CYRILLIC CAPITAL LETTER ER */
  case 0x0421: out[0] = 0x0441; return 1; /* CYRILLIC CAPITAL LETTER ES */
  case 0x0422: out[0] = 0x0442; return 1; /* CYRILLIC CAPITAL LETTER TE */
  case 0x0423: out[0] = 0x0443; return 1; /* CYRILLIC CAPITAL LETTER U */
  case 0x0424: out[0] = 0x0444; return 1; /* CYRILLIC CAPITAL LETTER EF */
  case 0x0425: out[0] = 0x0445; return 1; /* CYRILLIC CAPITAL LETTER HA */
  case 0x0426: out[0] = 0x0446; return 1; /* CYRILLIC CAPITAL LETTER TSE */
  case 0x0427: out[0] = 0x0447; return 1; /* CYRILLIC CAPITAL LETTER CHE */
  case 0x0428: out[0] = 0x0448; return 1; /* CYRILLIC CAPITAL LETTER SHA */
  case 0x0429: out[0] = 0x0449; return 1; /* CYRILLIC CAPITAL LETTER SHCHA */
  case 0x042A: out[0] = 0x044A; return 1; /* CYRILLIC CAPITAL LETTER HARD SIGN */
  case 0x042B: out[0] = 0x044B; return 1; /* CYRILLIC CAPITAL LETTER YERU */
  case 0x042C: out[0] = 0x044C; return 1; /* CYRILLIC CAPITAL LETTER SOFT SIGN */
  case 0x042D: out[0] = 0x044D; return 1; /* CYRILLIC CAPITAL LETTER E */
  case 0x042E: out[0] = 0x044E; return 1; /* CYRILLIC CAPITAL LETTER YU */
  case 0x042F: out[0] = 0x044F; return 1; /* CYRILLIC CAPITAL LETTER YA */
  case 0x0460: out[0] = 0x0461; return 1; /* CYRILLIC CAPITAL LETTER OMEGA */
  case 0x0462: out[0] = 0x0463; return 1; /* CYRILLIC CAPITAL LETTER YAT */
  case 0x0464: out[0] = 0x0465; return 1; /* CYRILLIC CAPITAL LETTER IOTIFIED E */
  case 0x0466: out[0] = 0x0467; return 1; /* CYRILLIC CAPITAL LETTER LITTLE YUS */
  case 0x0468: out[0] = 0x0469; return 1; /* CYRILLIC CAPITAL LETTER IOTIFIED LITTLE YUS */
  case 0x046A: out[0] = 0x046B; return 1; /* CYRILLIC CAPITAL LETTER BIG YUS */
  case 0x046C: out[0] = 0x046D; return 1; /* CYRILLIC CAPITAL LETTER IOTIFIED BIG YUS */
  case 0x046E: out[0] = 0x046F; return 1; /* CYRILLIC CAPITAL LETTER KSI */
  case 0x0470: out[0] = 0x0471; return 1; /* CYRILLIC CAPITAL LETTER PSI */
  case 0x0472: out[0] = 0x0473; return 1; /* CYRILLIC CAPITAL LETTER FITA */
  case 0x0474: out[0] = 0x0475; return 1; /* CYRILLIC CAPITAL LETTER IZHITSA */
  case 0x0476: out[0] = 0x0477; return 1; /* CYRILLIC CAPITAL LETTER IZHITSA WITH DOUBLE GRAVE ACCENT */
  case 0x0478: out[0] = 0x0479; return 1; /* CYRILLIC CAPITAL LETTER UK */
  case 0x047A: out[0] = 0x047B; return 1; /* CYRILLIC CAPITAL LETTER ROUND OMEGA */
  case 0x047C: out[0] = 0x047D; return 1; /* CYRILLIC CAPITAL LETTER OMEGA WITH TITLO */
  case 0x047E: out[0] = 0x047F; return 1; /* CYRILLIC CAPITAL LETTER OT */
  case 0x0480: out[0] = 0x0481; return 1; /* CYRILLIC CAPITAL LETTER KOPPA */
  case 0x048A: out[0] = 0x048B; return 1; /* CYRILLIC CAPITAL LETTER SHORT I WITH TAIL */
  case 0x048C: out[0] = 0x048D; return 1; /* CYRILLIC CAPITAL LETTER SEMISOFT SIGN */
  case 0x048E: out[0] = 0x048F; return 1; /* CYRILLIC CAPITAL LETTER ER WITH TICK */
  case 0x0490: out[0] = 0x0491; return 1; /* CYRILLIC CAPITAL LETTER GHE WITH UPTURN */
  case 0x0492: out[0] = 0x0493; return 1; /* CYRILLIC CAPITAL LETTER GHE WITH STROKE */
  case 0x0494: out[0] = 0x0495; return 1; /* CYRILLIC CAPITAL LETTER GHE WITH MIDDLE HOOK */
  case 0x0496: out[0] = 0x0497; return 1; /* CYRILLIC CAPITAL LETTER ZHE WITH DESCENDER */
  case 0x0498: out[0] = 0x0499; return 1; /* CYRILLIC CAPITAL LETTER ZE WITH DESCENDER */
  case 0x049A: out[0] = 0x049B; return 1; /* CYRILLIC CAPITAL LETTER KA WITH DESCENDER */
  case 0x049C: out[0] = 0x049D; return 1; /* CYRILLIC CAPITAL LETTER KA WITH VERTICAL STROKE */
  case 0x049E: out[0] = 0x049F; return 1; /* CYRILLIC CAPITAL LETTER KA WITH STROKE */
  case 0x04A0: out[0] = 0x04A1; return 1; /* CYRILLIC CAPITAL LETTER BASHKIR KA */
  case 0x04A2: out[0] = 0x04A3; return 1; /* CYRILLIC CAPITAL LETTER EN WITH DESCENDER */
  case 0x04A4: out[0] = 0x04A5; return 1; /* CYRILLIC CAPITAL LIGATURE EN GHE */
  case 0x04A6: out[0] = 0x04A7; return 1; /* CYRILLIC CAPITAL LETTER PE WITH MIDDLE HOOK */
  case 0x04A8: out[0] = 0x04A9; return 1; /* CYRILLIC CAPITAL LETTER ABKHASIAN HA */
  case 0x04AA: out[0] = 0x04AB; return 1; /* CYRILLIC CAPITAL LETTER ES WITH DESCENDER */
  case 0x04AC: out[0] = 0x04AD; return 1; /* CYRILLIC CAPITAL LETTER TE WITH DESCENDER */
  case 0x04AE: out[0] = 0x04AF; return 1; /* CYRILLIC CAPITAL LETTER STRAIGHT U */
  case 0x04B0: out[0] = 0x04B1; return 1; /* CYRILLIC CAPITAL LETTER STRAIGHT U WITH STROKE */
  case 0x04B2: out[0] = 0x04B3; return 1; /* CYRILLIC CAPITAL LETTER HA WITH DESCENDER */
  case 0x04B4: out[0] = 0x04B5; return 1; /* CYRILLIC CAPITAL LIGATURE TE TSE */
  case 0x04B6: out[0] = 0x04B7; return 1; /* CYRILLIC CAPITAL LETTER CHE WITH DESCENDER */
  case 0x04B8: out[0] = 0x04B9; return 1; /* CYRILLIC CAPITAL LETTER CHE WITH VERTICAL STROKE */
  case 0x04BA: out[0] = 0x04BB; return 1; /* CYRILLIC CAPITAL LETTER SHHA */
  case 0x04BC: out[0] = 0x04BD; return 1; /* CYRILLIC CAPITAL LETTER ABKHASIAN CHE */
  case 0x04BE: out[0] = 0x04BF; return 1; /* CYRILLIC CAPITAL LETTER ABKHASIAN CHE WITH DESCENDER */
  case 0x04C0: out[0] = 0x04CF; return 1; /* CYRILLIC LETTER PALOCHKA */
  case 0x04C1: out[0] = 0x04C2; return 1; /* CYRILLIC CAPITAL LETTER ZHE WITH BREVE */
  case 0x04C3: out[0] = 0x04C4; return 1; /* CYRILLIC CAPITAL LETTER KA WITH HOOK */
  case 0x04C5: out[0] = 0x04C6; return 1; /* CYRILLIC CAPITAL LETTER EL WITH TAIL */
  case 0x04C7: out[0] = 0x04C8; return 1; /* CYRILLIC CAPITAL LETTER EN WITH HOOK */
  case 0x04C9: out[0] = 0x04CA; return 1; /* CYRILLIC CAPITAL LETTER EN WITH TAIL */
  case 0x04CB: out[0] = 0x04CC; return 1; /* CYRILLIC CAPITAL LETTER KHAKASSIAN CHE */
  case 0x04CD: out[0] = 0x04CE; return 1; /* CYRILLIC CAPITAL LETTER EM WITH TAIL */
  case 0x04D0: out[0] = 0x04D1; return 1; /* CYRILLIC CAPITAL LETTER A WITH BREVE */
  case 0x04D2: out[0] = 0x04D3; return 1; /* CYRILLIC CAPITAL LETTER A WITH DIAERESIS */
  case 0x04D4: out[0] = 0x04D5; return 1; /* CYRILLIC CAPITAL LIGATURE A IE */
  case 0x04D6: out[0] = 0x04D7; return 1; /* CYRILLIC CAPITAL LETTER IE WITH BREVE */
  case 0x04D8: out[0] = 0x04D9; return 1; /* CYRILLIC CAPITAL LETTER SCHWA */
  case 0x04DA: out[0] = 0x04DB; return 1; /* CYRILLIC CAPITAL LETTER SCHWA WITH DIAERESIS */
  case 0x04DC: out[0] = 0x04DD; return 1; /* CYRILLIC CAPITAL LETTER ZHE WITH DIAERESIS */
  case 0x04DE: out[0] = 0x04DF; return 1; /* CYRILLIC CAPITAL LETTER ZE WITH DIAERESIS */
  case 0x04E0: out[0] = 0x04E1; return 1; /* CYRILLIC CAPITAL LETTER ABKHASIAN DZE */
  case 0x04E2: out[0] = 0x04E3; return 1; /* CYRILLIC CAPITAL LETTER I WITH MACRON */
  case 0x04E4: out[0] = 0x04E5; return 1; /* CYRILLIC CAPITAL LETTER I WITH DIAERESIS */
  case 0x04E6: out[0] = 0x04E7; return 1; /* CYRILLIC CAPITAL LETTER O WITH DIAERESIS */
  case 0x04E8: out[0] = 0x04E9; return 1; /* CYRILLIC CAPITAL LETTER BARRED O */
  case 0x04EA: out[0] = 0x04EB; return 1; /* CYRILLIC CAPITAL LETTER BARRED O WITH DIAERESIS */
  case 0x04EC: out[0] = 0x04ED; return 1; /* CYRILLIC CAPITAL LETTER E WITH DIAERESIS */
  case 0x04EE: out[0] = 0x04EF; return 1; /* CYRILLIC CAPITAL LETTER U WITH MACRON */
  case 0x04F0: out[0] = 0x04F1; return 1; /* CYRILLIC CAPITAL LETTER U WITH DIAERESIS */
  case 0x04F2: out[0] = 0x04F3; return 1; /* CYRILLIC CAPITAL LETTER U WITH DOUBLE ACUTE */
  case 0x04F4: out[0] = 0x04F5; return 1; /* CYRILLIC CAPITAL LETTER CHE WITH DIAERESIS */
  case 0x04F6: out[0] = 0x04F7; return 1; /* CYRILLIC CAPITAL LETTER GHE WITH DESCENDER */
  case 0x04F8: out[0] = 0x04F9; return 1; /* CYRILLIC CAPITAL LETTER YERU WITH DIAERESIS */
  case 0x04FA: out[0] = 0x04FB; return 1; /* CYRILLIC CAPITAL LETTER GHE WITH STROKE AND HOOK */
  case 0x04FC: out[0] = 0x04FD; return 1; /* CYRILLIC CAPITAL LETTER HA WITH HOOK */
  case 0x04FE: out[0] = 0x04FF; return 1; /* CYRILLIC CAPITAL LETTER HA WITH STROKE */
  case 0x0500: out[0] = 0x0501; return 1; /* CYRILLIC CAPITAL LETTER KOMI DE */
  case 0x0502: out[0] = 0x0503; return 1; /* CYRILLIC CAPITAL LETTER KOMI DJE */
  case 0x0504: out[0] = 0x0505; return 1; /* CYRILLIC CAPITAL LETTER KOMI ZJE */
  case 0x0506: out[0] = 0x0507; return 1; /* CYRILLIC CAPITAL LETTER KOMI DZJE */
  case 0x0508: out[0] = 0x0509; return 1; /* CYRILLIC CAPITAL LETTER KOMI LJE */
  case 0x050A: out[0] = 0x050B; return 1; /* CYRILLIC CAPITAL LETTER KOMI NJE */
  case 0x050C: out[0] = 0x050D; return 1; /* CYRILLIC CAPITAL LETTER KOMI SJE */
  case 0x050E: out[0] = 0x050F; return 1; /* CYRILLIC CAPITAL LETTER KOMI TJE */
  case 0x0510: out[0] = 0x0511; return 1; /* CYRILLIC CAPITAL LETTER REVERSED ZE */
  case 0x0512: out[0] = 0x0513; return 1; /* CYRILLIC CAPITAL LETTER EL WITH HOOK */
  case 0x0514: out[0] = 0x0515; return 1; /* CYRILLIC CAPITAL LETTER LHA */
  case 0x0516: out[0] = 0x0517; return 1; /* CYRILLIC CAPITAL LETTER RHA */
  case 0x0518: out[0] = 0x0519; return 1; /* CYRILLIC CAPITAL LETTER YAE */
  case 0x051A: out[0] = 0x051B; return 1; /* CYRILLIC CAPITAL LETTER QA */
  case 0x051C: out[0] = 0x051D; return 1; /* CYRILLIC CAPITAL LETTER WE */
  case 0x051E: out[0] = 0x051F; return 1; /* CYRILLIC CAPITAL LETTER ALEUT KA */
  case 0x0520: out[0] = 0x0521; return 1; /* CYRILLIC CAPITAL LETTER EL WITH MIDDLE HOOK */
  case 0x0522: out[0] = 0x0523; return 1; /* CYRILLIC CAPITAL LETTER EN WITH MIDDLE HOOK */
  case 0x0524: out[0] = 0x0525; return 1; /* CYRILLIC CAPITAL LETTER PE WITH DESCENDER */
  case 0x0526: out[0] = 0x0527; return 1; /* CYRILLIC CAPITAL LETTER SHHA WITH DESCENDER */
  case 0x0528: out[0] = 0x0529; return 1; /* CYRILLIC CAPITAL LETTER EN WITH LEFT HOOK */
  case 0x052A: out[0] = 0x052B; return 1; /* CYRILLIC CAPITAL LETTER DZZHE */
  case 0x052C: out[0] = 0x052D; return 1; /* CYRILLIC CAPITAL LETTER DCHE */
  case 0x052E: out[0] = 0x052F; return 1; /* CYRILLIC CAPITAL LETTER EL WITH DESCENDER */
  case 0x0531: out[0] = 0x0561; return 1; /* ARMENIAN CAPITAL LETTER AYB */
  case 0x0532: out[0] = 0x0562; return 1; /* ARMENIAN CAPITAL LETTER BEN */
  case 0x0533: out[0] = 0x0563; return 1; /* ARMENIAN CAPITAL LETTER GIM */
  case 0x0534: out[0] = 0x0564; return 1; /* ARMENIAN CAPITAL LETTER DA */
  case 0x0535: out[0] = 0x0565; return 1; /* ARMENIAN CAPITAL LETTER ECH */
  case 0x0536: out[0] = 0x0566; return 1; /* ARMENIAN CAPITAL LETTER ZA */
  case 0x0537: out[0] = 0x0567; return 1; /* ARMENIAN CAPITAL LETTER EH */
  case 0x0538: out[0] = 0x0568; return 1; /* ARMENIAN CAPITAL LETTER ET */
  case 0x0539: out[0] = 0x0569; return 1; /* ARMENIAN CAPITAL LETTER TO */
  case 0x053A: out[0] = 0x056A; return 1; /* ARMENIAN CAPITAL LETTER ZHE */
  case 0x053B: out[0] = 0x056B; return 1; /* ARMENIAN CAPITAL LETTER INI */
  case 0x053C: out[0] = 0x056C; return 1; /* ARMENIAN CAPITAL LETTER LIWN */
  case 0x053D: out[0] = 0x056D; return 1; /* ARMENIAN CAPITAL LETTER XEH */
  case 0x053E: out[0] = 0x056E; return 1; /* ARMENIAN CAPITAL LETTER CA */
  case 0x053F: out[0] = 0x056F; return 1; /* ARMENIAN CAPITAL LETTER KEN */
  case 0x0540: out[0] = 0x0570; return 1; /* ARMENIAN CAPITAL LETTER HO */
  case 0x0541: out[0] = 0x0571; return 1; /* ARMENIAN CAPITAL LETTER JA */
  case 0x0542: out[0] = 0x0572; return 1; /* ARMENIAN CAPITAL LETTER GHAD */
  case 0x0543: out[0] = 0x0573; return 1; /* ARMENIAN CAPITAL LETTER CHEH */
  case 0x0544: out[0] = 0x0574; return 1; /* ARMENIAN CAPITAL LETTER MEN */
  case 0x0545: out[0] = 0x0575; return 1; /* ARMENIAN CAPITAL LETTER YI */
  case 0x0546: out[0] = 0x0576; return 1; /* ARMENIAN CAPITAL LETTER NOW */
  case 0x0547: out[0] = 0x0577; return 1; /* ARMENIAN CAPITAL LETTER SHA */
  case 0x0548: out[0] = 0x0578; return 1; /* ARMENIAN CAPITAL LETTER VO */
  case 0x0549: out[0] = 0x0579; return 1; /* ARMENIAN CAPITAL LETTER CHA */
  case 0x054A: out[0] = 0x057A; return 1; /* ARMENIAN CAPITAL LETTER PEH */
  case 0x054B: out[0] = 0x057B; return 1; /* ARMENIAN CAPITAL LETTER JHEH */
  case 0x054C: out[0] = 0x057C; return 1; /* ARMENIAN CAPITAL LETTER RA */
  case 0x054D: out[0] = 0x057D; return 1; /* ARMENIAN CAPITAL LETTER SEH */
  case 0x054E: out[0] = 0x057E; return 1; /* ARMENIAN CAPITAL LETTER VEW */
  case 0x054F: out[0] = 0x057F; return 1; /* ARMENIAN CAPITAL LETTER TIWN */
  case 0x0550: out[0] = 0x0580; return 1; /* ARMENIAN CAPITAL LETTER REH */
  case 0x0551: out[0] = 0x0581; return 1; /* ARMENIAN CAPITAL LETTER CO */
  case 0x0552: out[0] = 0x0582; return 1; /* ARMENIAN CAPITAL LETTER YIWN */
  case 0x0553: out[0] = 0x0583; return 1; /* ARMENIAN CAPITAL LETTER PIWR */
  case 0x0554: out[0] = 0x0584; return 1; /* ARMENIAN CAPITAL LETTER KEH */
  case 0x0555: out[0] = 0x0585; return 1; /* ARMENIAN CAPITAL LETTER OH */
  case 0x0556: out[0] = 0x0586; return 1; /* ARMENIAN CAPITAL LETTER FEH */
  case 0x0587: out[0] = 0x0565; out[1] = 0x0582; return 2; /* ARMENIAN SMALL LIGATURE ECH YIWN */
  case 0x10A0: out[0] = 0x2D00; return 1; /* GEORGIAN CAPITAL LETTER AN */
  case 0x10A1: out[0] = 0x2D01; return 1; /* GEORGIAN CAPITAL LETTER BAN */
  case 0x10A2: out[0] = 0x2D02; return 1; /* GEORGIAN CAPITAL LETTER GAN */
  case 0x10A3: out[0] = 0x2D03; return 1; /* GEORGIAN CAPITAL LETTER DON */
  case 0x10A4: out[0] = 0x2D04; return 1; /* GEORGIAN CAPITAL LETTER EN */
  case 0x10A5: out[0] = 0x2D05; return 1; /* GEORGIAN CAPITAL LETTER VIN */
  case 0x10A6: out[0] = 0x2D06; return 1; /* GEORGIAN CAPITAL LETTER ZEN */
  case 0x10A7: out[0] = 0x2D07; return 1; /* GEORGIAN CAPITAL LETTER TAN */
  case 0x10A8: out[0] = 0x2D08; return 1; /* GEORGIAN CAPITAL LETTER IN */
  case 0x10A9: out[0] = 0x2D09; return 1; /* GEORGIAN CAPITAL LETTER KAN */
  case 0x10AA: out[0] = 0x2D0A; return 1; /* GEORGIAN CAPITAL LETTER LAS */
  case 0x10AB: out[0] = 0x2D0B; return 1; /* GEORGIAN CAPITAL LETTER MAN */
  case 0x10AC: out[0] = 0x2D0C; return 1; /* GEORGIAN CAPITAL LETTER NAR */
  case 0x10AD: out[0] = 0x2D0D; return 1; /* GEORGIAN CAPITAL LETTER ON */
  case 0x10AE: out[0] = 0x2D0E; return 1; /* GEORGIAN CAPITAL LETTER PAR */
  case 0x10AF: out[0] = 0x2D0F; return 1; /* GEORGIAN CAPITAL LETTER ZHAR */
  case 0x10B0: out[0] = 0x2D10; return 1; /* GEORGIAN CAPITAL LETTER RAE */
  case 0x10B1: out[0] = 0x2D11; return 1; /* GEORGIAN CAPITAL LETTER SAN */
  case 0x10B2: out[0] = 0x2D12; return 1; /* GEORGIAN CAPITAL LETTER TAR */
  case 0x10B3: out[0] = 0x2D13; return 1; /* GEORGIAN CAPITAL LETTER UN */
  case 0x10B4: out[0] = 0x2D14; return 1; /* GEORGIAN CAPITAL LETTER PHAR */
  case 0x10B5: out[0] = 0x2D15; return 1; /* GEORGIAN CAPITAL LETTER KHAR */
  case 0x10B6: out[0] = 0x2D16; return 1; /* GEORGIAN CAPITAL LETTER GHAN */
  case 0x10B7: out[0] = 0x2D17; return 1; /* GEORGIAN CAPITAL LETTER QAR */
  case 0x10B8: out[0] = 0x2D18; return 1; /* GEORGIAN CAPITAL LETTER SHIN */
  case 0x10B9: out[0] = 0x2D19; return 1; /* GEORGIAN CAPITAL LETTER CHIN */
  case 0x10BA: out[0] = 0x2D1A; return 1; /* GEORGIAN CAPITAL LETTER CAN */
  case 0x10BB: out[0] = 0x2D1B; return 1; /* GEORGIAN CAPITAL LETTER JIL */
  case 0x10BC: out[0] = 0x2D1C; return 1; /* GEORGIAN CAPITAL LETTER CIL */
  case 0x10BD: out[0] = 0x2D1D; return 1; /* GEORGIAN CAPITAL LETTER CHAR */
  case 0x10BE: out[0] = 0x2D1E; return 1; /* GEORGIAN CAPITAL LETTER XAN */
  case 0x10BF: out[0] = 0x2D1F; return 1; /* GEORGIAN CAPITAL LETTER JHAN */
  case 0x10C0: out[0] = 0x2D20; return 1; /* GEORGIAN CAPITAL LETTER HAE */
  case 0x10C1: out[0] = 0x2D21; return 1; /* GEORGIAN CAPITAL LETTER HE */
  case 0x10C2: out[0] = 0x2D22; return 1; /* GEORGIAN CAPITAL LETTER HIE */
  case 0x10C3: out[0] = 0x2D23; return 1; /* GEORGIAN CAPITAL LETTER WE */
  case 0x10C4: out[0] = 0x2D24; return 1; /* GEORGIAN CAPITAL LETTER HAR */
  case 0x10C5: out[0] = 0x2D25; return 1; /* GEORGIAN CAPITAL LETTER HOE */
  case 0x10C7: out[0] = 0x2D27; return 1; /* GEORGIAN CAPITAL LETTER YN */
  case 0x10CD: out[0] = 0x2D2D; return 1; /* GEORGIAN CAPITAL LETTER AEN */
  case 0x13F8: out[0] = 0x13F0; return 1; /* CHEROKEE SMALL LETTER YE */
  case 0x13F9: out[0] = 0x13F1; return 1; /* CHEROKEE SMALL LETTER YI */
  case 0x13FA: out[0] = 0x13F2; return 1; /* CHEROKEE SMALL LETTER YO */
  case 0x13FB: out[0] = 0x13F3; return 1; /* CHEROKEE SMALL LETTER YU */
  case 0x13FC: out[0] = 0x13F4; return 1; /* CHEROKEE SMALL LETTER YV */
  case 0x13FD: out[0] = 0x13F5; return 1; /* CHEROKEE SMALL LETTER MV */
  case 0x1C80: out[0] = 0x0432; return 1; /* CYRILLIC SMALL LETTER ROUNDED VE */
  case 0x1C81: out[0] = 0x0434; return 1; /* CYRILLIC SMALL LETTER LONG-LEGGED DE */
  case 0x1C82: out[0] = 0x043E; return 1; /* CYRILLIC SMALL LETTER NARROW O */
  case 0x1C83: out[0] = 0x0441; return 1; /* CYRILLIC SMALL LETTER WIDE ES */
  case 0x1C84: out[0] = 0x0442; return 1; /* CYRILLIC SMALL LETTER TALL TE */
  case 0x1C85: out[0] = 0x0442; return 1; /* CYRILLIC SMALL LETTER THREE-LEGGED TE */
  case 0x1C86: out[0] = 0x044A; return 1; /* CYRILLIC SMALL LETTER TALL HARD SIGN */
  case 0x1C87: out[0] = 0x0463; return 1; /* CYRILLIC SMALL LETTER TALL YAT */
  case 0x1C88: out[0] = 0xA64B; return 1; /* CYRILLIC SMALL LETTER UNBLENDED UK */
  case 0x1C90: out[0] = 0x10D0; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER AN */
  case 0x1C91: out[0] = 0x10D1; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER BAN */
  case 0x1C92: out[0] = 0x10D2; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER GAN */
  case 0x1C93: out[0] = 0x10D3; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER DON */
  case 0x1C94: out[0] = 0x10D4; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER EN */
  case 0x1C95: out[0] = 0x10D5; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER VIN */
  case 0x1C96: out[0] = 0x10D6; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER ZEN */
  case 0x1C97: out[0] = 0x10D7; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER TAN */
  case 0x1C98: out[0] = 0x10D8; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER IN */
  case 0x1C99: out[0] = 0x10D9; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER KAN */
  case 0x1C9A: out[0] = 0x10DA; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER LAS */
  case 0x1C9B: out[0] = 0x10DB; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER MAN */
  case 0x1C9C: out[0] = 0x10DC; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER NAR */
  case 0x1C9D: out[0] = 0x10DD; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER ON */
  case 0x1C9E: out[0] = 0x10DE; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER PAR */
  case 0x1C9F: out[0] = 0x10DF; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER ZHAR */
  case 0x1CA0: out[0] = 0x10E0; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER RAE */
  case 0x1CA1: out[0] = 0x10E1; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER SAN */
  case 0x1CA2: out[0] = 0x10E2; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER TAR */
  case 0x1CA3: out[0] = 0x10E3; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER UN */
  case 0x1CA4: out[0] = 0x10E4; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER PHAR */
  case 0x1CA5: out[0] = 0x10E5; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER KHAR */
  case 0x1CA6: out[0] = 0x10E6; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER GHAN */
  case 0x1CA7: out[0] = 0x10E7; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER QAR */
  case 0x1CA8: out[0] = 0x10E8; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER SHIN */
  case 0x1CA9: out[0] = 0x10E9; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER CHIN */
  case 0x1CAA: out[0] = 0x10EA; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER CAN */
  case 0x1CAB: out[0] = 0x10EB; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER JIL */
  case 0x1CAC: out[0] = 0x10EC; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER CIL */
  case 0x1CAD: out[0] = 0x10ED; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER CHAR */
  case 0x1CAE: out[0] = 0x10EE; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER XAN */
  case 0x1CAF: out[0] = 0x10EF; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER JHAN */
  case 0x1CB0: out[0] = 0x10F0; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER HAE */
  case 0x1CB1: out[0] = 0x10F1; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER HE */
  case 0x1CB2: out[0] = 0x10F2; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER HIE */
  case 0x1CB3: out[0] = 0x10F3; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER WE */
  case 0x1CB4: out[0] = 0x10F4; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER HAR */
  case 0x1CB5: out[0] = 0x10F5; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER HOE */
  case 0x1CB6: out[0] = 0x10F6; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER FI */
  case 0x1CB7: out[0] = 0x10F7; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER YN */
  case 0x1CB8: out[0] = 0x10F8; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER ELIFI */
  case 0x1CB9: out[0] = 0x10F9; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER TURNED GAN */
  case 0x1CBA: out[0] = 0x10FA; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER AIN */
  case 0x1CBD: out[0] = 0x10FD; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER AEN */
  case 0x1CBE: out[0] = 0x10FE; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER HARD SIGN */
  case 0x1CBF: out[0] = 0x10FF; return 1; /* GEORGIAN MTAVRULI CAPITAL LETTER LABIAL SIGN */
  case 0x1E00: out[0] = 0x1E01; return 1; /* LATIN CAPITAL LETTER A WITH RING BELOW */
  case 0x1E02: out[0] = 0x1E03; return 1; /* LATIN CAPITAL LETTER B WITH DOT ABOVE */
  case 0x1E04: out[0] = 0x1E05; return 1; /* LATIN CAPITAL LETTER B WITH DOT BELOW */
  case 0x1E06: out[0] = 0x1E07; return 1; /* LATIN CAPITAL LETTER B WITH LINE BELOW */
  case 0x1E08: out[0] = 0x1E09; return 1; /* LATIN CAPITAL LETTER C WITH CEDILLA AND ACUTE */
  case 0x1E0A: out[0] = 0x1E0B; return 1; /* LATIN CAPITAL LETTER D WITH DOT ABOVE */
  case 0x1E0C: out[0] = 0x1E0D; return 1; /* LATIN CAPITAL LETTER D WITH DOT BELOW */
  case 0x1E0E: out[0] = 0x1E0F; return 1; /* LATIN CAPITAL LETTER D WITH LINE BELOW */
  case 0x1E10: out[0] = 0x1E11; return 1; /* LATIN CAPITAL LETTER D WITH CEDILLA */
  case 0x1E12: out[0] = 0x1E13; return 1; /* LATIN CAPITAL LETTER D WITH CIRCUMFLEX BELOW */
  case 0x1E14: out[0] = 0x1E15; return 1; /* LATIN CAPITAL LETTER E WITH MACRON AND GRAVE */
  case 0x1E16: out[0] = 0x1E17; return 1; /* LATIN CAPITAL LETTER E WITH MACRON AND ACUTE */
  case 0x1E18: out[0] = 0x1E19; return 1; /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX BELOW */
  case 0x1E1A: out[0] = 0x1E1B; return 1; /* LATIN CAPITAL LETTER E WITH TILDE BELOW */
  case 0x1E1C: out[0] = 0x1E1D; return 1; /* LATIN CAPITAL LETTER E WITH CEDILLA AND BREVE */
  case 0x1E1E: out[0] = 0x1E1F; return 1; /* LATIN CAPITAL LETTER F WITH DOT ABOVE */
  case 0x1E20: out[0] = 0x1E21; return 1; /* LATIN CAPITAL LETTER G WITH MACRON */
  case 0x1E22: out[0] = 0x1E23; return 1; /* LATIN CAPITAL LETTER H WITH DOT ABOVE */
  case 0x1E24: out[0] = 0x1E25; return 1; /* LATIN CAPITAL LETTER H WITH DOT BELOW */
  case 0x1E26: out[0] = 0x1E27; return 1; /* LATIN CAPITAL LETTER H WITH DIAERESIS */
  case 0x1E28: out[0] = 0x1E29; return 1; /* LATIN CAPITAL LETTER H WITH CEDILLA */
  case 0x1E2A: out[0] = 0x1E2B; return 1; /* LATIN CAPITAL LETTER H WITH BREVE BELOW */
  case 0x1E2C: out[0] = 0x1E2D; return 1; /* LATIN CAPITAL LETTER I WITH TILDE BELOW */
  case 0x1E2E: out[0] = 0x1E2F; return 1; /* LATIN CAPITAL LETTER I WITH DIAERESIS AND ACUTE */
  case 0x1E30: out[0] = 0x1E31; return 1; /* LATIN CAPITAL LETTER K WITH ACUTE */
  case 0x1E32: out[0] = 0x1E33; return 1; /* LATIN CAPITAL LETTER K WITH DOT BELOW */
  case 0x1E34: out[0] = 0x1E35; return 1; /* LATIN CAPITAL LETTER K WITH LINE BELOW */
  case 0x1E36: out[0] = 0x1E37; return 1; /* LATIN CAPITAL LETTER L WITH DOT BELOW */
  case 0x1E38: out[0] = 0x1E39; return 1; /* LATIN CAPITAL LETTER L WITH DOT BELOW AND MACRON */
  case 0x1E3A: out[0] = 0x1E3B; return 1; /* LATIN CAPITAL LETTER L WITH LINE BELOW */
  case 0x1E3C: out[0] = 0x1E3D; return 1; /* LATIN CAPITAL LETTER L WITH CIRCUMFLEX BELOW */
  case 0x1E3E: out[0] = 0x1E3F; return 1; /* LATIN CAPITAL LETTER M WITH ACUTE */
  case 0x1E40: out[0] = 0x1E41; return 1; /* LATIN CAPITAL LETTER M WITH DOT ABOVE */
  case 0x1E42: out[0] = 0x1E43; return 1; /* LATIN CAPITAL LETTER M WITH DOT BELOW */
  case 0x1E44: out[0] = 0x1E45; return 1; /* LATIN CAPITAL LETTER N WITH DOT ABOVE */
  case 0x1E46: out[0] = 0x1E47; return 1; /* LATIN CAPITAL LETTER N WITH DOT BELOW */
  case 0x1E48: out[0] = 0x1E49; return 1; /* LATIN CAPITAL LETTER N WITH LINE BELOW */
  case 0x1E4A: out[0] = 0x1E4B; return 1; /* LATIN CAPITAL LETTER N WITH CIRCUMFLEX BELOW */
  case 0x1E4C: out[0] = 0x1E4D; return 1; /* LATIN CAPITAL LETTER O WITH TILDE AND ACUTE */
  case 0x1E4E: out[0] = 0x1E4F; return 1; /* LATIN CAPITAL LETTER O WITH TILDE AND DIAERESIS */
  case 0x1E50: out[0] = 0x1E51; return 1; /* LATIN CAPITAL LETTER O WITH MACRON AND GRAVE */
  case 0x1E52: out[0] = 0x1E53; return 1; /* LATIN CAPITAL LETTER O WITH MACRON AND ACUTE */
  case 0x1E54: out[0] = 0x1E55; return 1; /* LATIN CAPITAL LETTER P WITH ACUTE */
  case 0x1E56: out[0] = 0x1E57; return 1; /* LATIN CAPITAL LETTER P WITH DOT ABOVE */
  case 0x1E58: out[0] = 0x1E59; return 1; /* LATIN CAPITAL LETTER R WITH DOT ABOVE */
  case 0x1E5A: out[0] = 0x1E5B; return 1; /* LATIN CAPITAL LETTER R WITH DOT BELOW */
  case 0x1E5C: out[0] = 0x1E5D; return 1; /* LATIN CAPITAL LETTER R WITH DOT BELOW AND MACRON */
  case 0x1E5E: out[0] = 0x1E5F; return 1; /* LATIN CAPITAL LETTER R WITH LINE BELOW */
  case 0x1E60: out[0] = 0x1E61; return 1; /* LATIN CAPITAL LETTER S WITH DOT ABOVE */
  case 0x1E62: out[0] = 0x1E63; return 1; /* LATIN CAPITAL LETTER S WITH DOT BELOW */
  case 0x1E64: out[0] = 0x1E65; return 1; /* LATIN CAPITAL LETTER S WITH ACUTE AND DOT ABOVE */
  case 0x1E66: out[0] = 0x1E67; return 1; /* LATIN CAPITAL LETTER S WITH CARON AND DOT ABOVE */
  case 0x1E68: out[0] = 0x1E69; return 1; /* LATIN CAPITAL LETTER S WITH DOT BELOW AND DOT ABOVE */
  case 0x1E6A: out[0] = 0x1E6B; return 1; /* LATIN CAPITAL LETTER T WITH DOT ABOVE */
  case 0x1E6C: out[0] = 0x1E6D; return 1; /* LATIN CAPITAL LETTER T WITH DOT BELOW */
  case 0x1E6E: out[0] = 0x1E6F; return 1; /* LATIN CAPITAL LETTER T WITH LINE BELOW */
  case 0x1E70: out[0] = 0x1E71; return 1; /* LATIN CAPITAL LETTER T WITH CIRCUMFLEX BELOW */
  case 0x1E72: out[0] = 0x1E73; return 1; /* LATIN CAPITAL LETTER U WITH DIAERESIS BELOW */
  case 0x1E74: out[0] = 0x1E75; return 1; /* LATIN CAPITAL LETTER U WITH TILDE BELOW */
  case 0x1E76: out[0] = 0x1E77; return 1; /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX BELOW */
  case 0x1E78: out[0] = 0x1E79; return 1; /* LATIN CAPITAL LETTER U WITH TILDE AND ACUTE */
  case 0x1E7A: out[0] = 0x1E7B; return 1; /* LATIN CAPITAL LETTER U WITH MACRON AND DIAERESIS */
  case 0x1E7C: out[0] = 0x1E7D; return 1; /* LATIN CAPITAL LETTER V WITH TILDE */
  case 0x1E7E: out[0] = 0x1E7F; return 1; /* LATIN CAPITAL LETTER V WITH DOT BELOW */
  case 0x1E80: out[0] = 0x1E81; return 1; /* LATIN CAPITAL LETTER W WITH GRAVE */
  case 0x1E82: out[0] = 0x1E83; return 1; /* LATIN CAPITAL LETTER W WITH ACUTE */
  case 0x1E84: out[0] = 0x1E85; return 1; /* LATIN CAPITAL LETTER W WITH DIAERESIS */
  case 0x1E86: out[0] = 0x1E87; return 1; /* LATIN CAPITAL LETTER W WITH DOT ABOVE */
  case 0x1E88: out[0] = 0x1E89; return 1; /* LATIN CAPITAL LETTER W WITH DOT BELOW */
  case 0x1E8A: out[0] = 0x1E8B; return 1; /* LATIN CAPITAL LETTER X WITH DOT ABOVE */
  case 0x1E8C: out[0] = 0x1E8D; return 1; /* LATIN CAPITAL LETTER X WITH DIAERESIS */
  case 0x1E8E: out[0] = 0x1E8F; return 1; /* LATIN CAPITAL LETTER Y WITH DOT ABOVE */
  case 0x1E90: out[0] = 0x1E91; return 1; /* LATIN CAPITAL LETTER Z WITH CIRCUMFLEX */
  case 0x1E92: out[0] = 0x1E93; return 1; /* LATIN CAPITAL LETTER Z WITH DOT BELOW */
  case 0x1E94: out[0] = 0x1E95; return 1; /* LATIN CAPITAL LETTER Z WITH LINE BELOW */
  case 0x1E96: out[0] = 0x0068; out[1] = 0x0331; return 2;/* LATIN SMALL LETTER H WITH LINE BELOW */
  case 0x1E97: out[0] = 0x0074; out[1] = 0x0308; return 2;/* LATIN SMALL LETTER T WITH DIAERESIS */
  case 0x1E98: out[0] = 0x0077; out[1] = 0x030A; return 2;/* LATIN SMALL LETTER W WITH RING ABOVE */
  case 0x1E99: out[0] = 0x0079; out[1] = 0x030A; return 2;/* LATIN SMALL LETTER Y WITH RING ABOVE */
  case 0x1E9A: out[0] = 0x0061; out[1] = 0x02BE; return 2;/* LATIN SMALL LETTER A WITH RIGHT HALF RING */
  case 0x1E9B: out[0] = 0x1E61; return 1; /* LATIN SMALL LETTER LONG S WITH DOT ABOVE */
  case 0x1E9E: out[0] = 0x0073; out[1] = 0x0073; return 2; /* LATIN CAPITAL LETTER SHARP S */
  case 0x1EA0: out[0] = 0x1EA1; return 1; /* LATIN CAPITAL LETTER A WITH DOT BELOW */
  case 0x1EA2: out[0] = 0x1EA3; return 1; /* LATIN CAPITAL LETTER A WITH HOOK ABOVE */
  case 0x1EA4: out[0] = 0x1EA5; return 1; /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND ACUTE */
  case 0x1EA6: out[0] = 0x1EA7; return 1; /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND GRAVE */
  case 0x1EA8: out[0] = 0x1EA9; return 1; /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND HOOK ABOVE */
  case 0x1EAA: out[0] = 0x1EAB; return 1; /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND TILDE */
  case 0x1EAC: out[0] = 0x1EAD; return 1; /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND DOT BELOW */
  case 0x1EAE: out[0] = 0x1EAF; return 1; /* LATIN CAPITAL LETTER A WITH BREVE AND ACUTE */
  case 0x1EB0: out[0] = 0x1EB1; return 1; /* LATIN CAPITAL LETTER A WITH BREVE AND GRAVE */
  case 0x1EB2: out[0] = 0x1EB3; return 1; /* LATIN CAPITAL LETTER A WITH BREVE AND HOOK ABOVE */
  case 0x1EB4: out[0] = 0x1EB5; return 1; /* LATIN CAPITAL LETTER A WITH BREVE AND TILDE */
  case 0x1EB6: out[0] = 0x1EB7; return 1; /* LATIN CAPITAL LETTER A WITH BREVE AND DOT BELOW */
  case 0x1EB8: out[0] = 0x1EB9; return 1; /* LATIN CAPITAL LETTER E WITH DOT BELOW */
  case 0x1EBA: out[0] = 0x1EBB; return 1; /* LATIN CAPITAL LETTER E WITH HOOK ABOVE */
  case 0x1EBC: out[0] = 0x1EBD; return 1; /* LATIN CAPITAL LETTER E WITH TILDE */
  case 0x1EBE: out[0] = 0x1EBF; return 1; /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND ACUTE */
  case 0x1EC0: out[0] = 0x1EC1; return 1; /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND GRAVE */
  case 0x1EC2: out[0] = 0x1EC3; return 1; /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND HOOK ABOVE */
  case 0x1EC4: out[0] = 0x1EC5; return 1; /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND TILDE */
  case 0x1EC6: out[0] = 0x1EC7; return 1; /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND DOT BELOW */
  case 0x1EC8: out[0] = 0x1EC9; return 1; /* LATIN CAPITAL LETTER I WITH HOOK ABOVE */
  case 0x1ECA: out[0] = 0x1ECB; return 1; /* LATIN CAPITAL LETTER I WITH DOT BELOW */
  case 0x1ECC: out[0] = 0x1ECD; return 1; /* LATIN CAPITAL LETTER O WITH DOT BELOW */
  case 0x1ECE: out[0] = 0x1ECF; return 1; /* LATIN CAPITAL LETTER O WITH HOOK ABOVE */
  case 0x1ED0: out[0] = 0x1ED1; return 1; /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND ACUTE */
  case 0x1ED2: out[0] = 0x1ED3; return 1; /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND GRAVE */
  case 0x1ED4: out[0] = 0x1ED5; return 1; /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND HOOK ABOVE */
  case 0x1ED6: out[0] = 0x1ED7; return 1; /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND TILDE */
  case 0x1ED8: out[0] = 0x1ED9; return 1; /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND DOT BELOW */
  case 0x1EDA: out[0] = 0x1EDB; return 1; /* LATIN CAPITAL LETTER O WITH HORN AND ACUTE */
  case 0x1EDC: out[0] = 0x1EDD; return 1; /* LATIN CAPITAL LETTER O WITH HORN AND GRAVE */
  case 0x1EDE: out[0] = 0x1EDF; return 1; /* LATIN CAPITAL LETTER O WITH HORN AND HOOK ABOVE */
  case 0x1EE0: out[0] = 0x1EE1; return 1; /* LATIN CAPITAL LETTER O WITH HORN AND TILDE */
  case 0x1EE2: out[0] = 0x1EE3; return 1; /* LATIN CAPITAL LETTER O WITH HORN AND DOT BELOW */
  case 0x1EE4: out[0] = 0x1EE5; return 1; /* LATIN CAPITAL LETTER U WITH DOT BELOW */
  case 0x1EE6: out[0] = 0x1EE7; return 1; /* LATIN CAPITAL LETTER U WITH HOOK ABOVE */
  case 0x1EE8: out[0] = 0x1EE9; return 1; /* LATIN CAPITAL LETTER U WITH HORN AND ACUTE */
  case 0x1EEA: out[0] = 0x1EEB; return 1; /* LATIN CAPITAL LETTER U WITH HORN AND GRAVE */
  case 0x1EEC: out[0] = 0x1EED; return 1; /* LATIN CAPITAL LETTER U WITH HORN AND HOOK ABOVE */
  case 0x1EEE: out[0] = 0x1EEF; return 1; /* LATIN CAPITAL LETTER U WITH HORN AND TILDE */
  case 0x1EF0: out[0] = 0x1EF1; return 1; /* LATIN CAPITAL LETTER U WITH HORN AND DOT BELOW */
  case 0x1EF2: out[0] = 0x1EF3; return 1; /* LATIN CAPITAL LETTER Y WITH GRAVE */
  case 0x1EF4: out[0] = 0x1EF5; return 1; /* LATIN CAPITAL LETTER Y WITH DOT BELOW */
  case 0x1EF6: out[0] = 0x1EF7; return 1; /* LATIN CAPITAL LETTER Y WITH HOOK ABOVE */
  case 0x1EF8: out[0] = 0x1EF9; return 1; /* LATIN CAPITAL LETTER Y WITH TILDE */
  case 0x1EFA: out[0] = 0x1EFB; return 1; /* LATIN CAPITAL LETTER MIDDLE-WELSH LL */
  case 0x1EFC: out[0] = 0x1EFD; return 1; /* LATIN CAPITAL LETTER MIDDLE-WELSH V */
  case 0x1EFE: out[0] = 0x1EFF; return 1; /* LATIN CAPITAL LETTER Y WITH LOOP */
  case 0x1F08: out[0] = 0x1F00; return 1; /* GREEK CAPITAL LETTER ALPHA WITH PSILI */
  case 0x1F09: out[0] = 0x1F01; return 1; /* GREEK CAPITAL LETTER ALPHA WITH DASIA */
  case 0x1F0A: out[0] = 0x1F02; return 1; /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND VARIA */
  case 0x1F0B: out[0] = 0x1F03; return 1; /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND VARIA */
  case 0x1F0C: out[0] = 0x1F04; return 1; /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND OXIA */
  case 0x1F0D: out[0] = 0x1F05; return 1; /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND OXIA */
  case 0x1F0E: out[0] = 0x1F06; return 1; /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND PERISPOMENI */
  case 0x1F0F: out[0] = 0x1F07; return 1; /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND PERISPOMENI */
  case 0x1F18: out[0] = 0x1F10; return 1; /* GREEK CAPITAL LETTER EPSILON WITH PSILI */
  case 0x1F19: out[0] = 0x1F11; return 1; /* GREEK CAPITAL LETTER EPSILON WITH DASIA */
  case 0x1F1A: out[0] = 0x1F12; return 1; /* GREEK CAPITAL LETTER EPSILON WITH PSILI AND VARIA */
  case 0x1F1B: out[0] = 0x1F13; return 1; /* GREEK CAPITAL LETTER EPSILON WITH DASIA AND VARIA */
  case 0x1F1C: out[0] = 0x1F14; return 1; /* GREEK CAPITAL LETTER EPSILON WITH PSILI AND OXIA */
  case 0x1F1D: out[0] = 0x1F15; return 1; /* GREEK CAPITAL LETTER EPSILON WITH DASIA AND OXIA */
  case 0x1F28: out[0] = 0x1F20; return 1; /* GREEK CAPITAL LETTER ETA WITH PSILI */
  case 0x1F29: out[0] = 0x1F21; return 1; /* GREEK CAPITAL LETTER ETA WITH DASIA */
  case 0x1F2A: out[0] = 0x1F22; return 1; /* GREEK CAPITAL LETTER ETA WITH PSILI AND VARIA */
  case 0x1F2B: out[0] = 0x1F23; return 1; /* GREEK CAPITAL LETTER ETA WITH DASIA AND VARIA */
  case 0x1F2C: out[0] = 0x1F24; return 1; /* GREEK CAPITAL LETTER ETA WITH PSILI AND OXIA */
  case 0x1F2D: out[0] = 0x1F25; return 1; /* GREEK CAPITAL LETTER ETA WITH DASIA AND OXIA */
  case 0x1F2E: out[0] = 0x1F26; return 1; /* GREEK CAPITAL LETTER ETA WITH PSILI AND PERISPOMENI */
  case 0x1F2F: out[0] = 0x1F27; return 1; /* GREEK CAPITAL LETTER ETA WITH DASIA AND PERISPOMENI */
  case 0x1F38: out[0] = 0x1F30; return 1; /* GREEK CAPITAL LETTER IOTA WITH PSILI */
  case 0x1F39: out[0] = 0x1F31; return 1; /* GREEK CAPITAL LETTER IOTA WITH DASIA */
  case 0x1F3A: out[0] = 0x1F32; return 1; /* GREEK CAPITAL LETTER IOTA WITH PSILI AND VARIA */
  case 0x1F3B: out[0] = 0x1F33; return 1; /* GREEK CAPITAL LETTER IOTA WITH DASIA AND VARIA */
  case 0x1F3C: out[0] = 0x1F34; return 1; /* GREEK CAPITAL LETTER IOTA WITH PSILI AND OXIA */
  case 0x1F3D: out[0] = 0x1F35; return 1; /* GREEK CAPITAL LETTER IOTA WITH DASIA AND OXIA */
  case 0x1F3E: out[0] = 0x1F36; return 1; /* GREEK CAPITAL LETTER IOTA WITH PSILI AND PERISPOMENI */
  case 0x1F3F: out[0] = 0x1F37; return 1; /* GREEK CAPITAL LETTER IOTA WITH DASIA AND PERISPOMENI */
  case 0x1F48: out[0] = 0x1F40; return 1; /* GREEK CAPITAL LETTER OMICRON WITH PSILI */
  case 0x1F49: out[0] = 0x1F41; return 1; /* GREEK CAPITAL LETTER OMICRON WITH DASIA */
  case 0x1F4A: out[0] = 0x1F42; return 1; /* GREEK CAPITAL LETTER OMICRON WITH PSILI AND VARIA */
  case 0x1F4B: out[0] = 0x1F43; return 1; /* GREEK CAPITAL LETTER OMICRON WITH DASIA AND VARIA */
  case 0x1F4C: out[0] = 0x1F44; return 1; /* GREEK CAPITAL LETTER OMICRON WITH PSILI AND OXIA */
  case 0x1F4D: out[0] = 0x1F45; return 1; /* GREEK CAPITAL LETTER OMICRON WITH DASIA AND OXIA */
  case 0x1F50: out[0] = 0x03C5; out[1] = 0x0313; return 2;/* GREEK SMALL LETTER UPSILON WITH PSILI */
  case 0x1F52: out[0] = 0x03C5; out[1] = 0x0313; out[2] = 0x0300; return 3;/* GREEK SMALL LETTER UPSILON WITH PSILI AND VARIA */
  case 0x1F54: out[0] = 0x03C5; out[1] = 0x0313; out[2] = 0x0301; return 3;/* GREEK SMALL LETTER UPSILON WITH PSILI AND OXIA */
  case 0x1F56: out[0] = 0x03C5; out[1] = 0x0313; out[2] = 0x0342; return 3;/* GREEK SMALL LETTER UPSILON WITH PSILI AND PERISPOMENI */
  case 0x1F59: out[0] = 0x1F51; return 1; /* GREEK CAPITAL LETTER UPSILON WITH DASIA */
  case 0x1F5B: out[0] = 0x1F53; return 1; /* GREEK CAPITAL LETTER UPSILON WITH DASIA AND VARIA */
  case 0x1F5D: out[0] = 0x1F55; return 1; /* GREEK CAPITAL LETTER UPSILON WITH DASIA AND OXIA */
  case 0x1F5F: out[0] = 0x1F57; return 1; /* GREEK CAPITAL LETTER UPSILON WITH DASIA AND PERISPOMENI */
  case 0x1F68: out[0] = 0x1F60; return 1; /* GREEK CAPITAL LETTER OMEGA WITH PSILI */
  case 0x1F69: out[0] = 0x1F61; return 1; /* GREEK CAPITAL LETTER OMEGA WITH DASIA */
  case 0x1F6A: out[0] = 0x1F62; return 1; /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND VARIA */
  case 0x1F6B: out[0] = 0x1F63; return 1; /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND VARIA */
  case 0x1F6C: out[0] = 0x1F64; return 1; /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND OXIA */
  case 0x1F6D: out[0] = 0x1F65; return 1; /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND OXIA */
  case 0x1F6E: out[0] = 0x1F66; return 1; /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND PERISPOMENI */
  case 0x1F6F: out[0] = 0x1F67; return 1; /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND PERISPOMENI */
  case 0x1F80: out[0] = 0x1F00; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ALPHA WITH PSILI AND YPOGEGRAMMENI */
  case 0x1F81: out[0] = 0x1F01; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ALPHA WITH DASIA AND YPOGEGRAMMENI */
  case 0x1F82: out[0] = 0x1F02; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ALPHA WITH PSILI AND VARIA AND YPOGEGRAMMENI */
  case 0x1F83: out[0] = 0x1F03; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ALPHA WITH DASIA AND VARIA AND YPOGEGRAMMENI */
  case 0x1F84: out[0] = 0x1F04; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ALPHA WITH PSILI AND OXIA AND YPOGEGRAMMENI */
  case 0x1F85: out[0] = 0x1F05; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ALPHA WITH DASIA AND OXIA AND YPOGEGRAMMENI */
  case 0x1F86: out[0] = 0x1F06; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ALPHA WITH PSILI AND PERISPOMENI AND YPOGEGRAMMENI */
  case 0x1F87: out[0] = 0x1F07; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ALPHA WITH DASIA AND PERISPOMENI AND YPOGEGRAMMENI */
  case 0x1F88: out[0] = 0x1F00; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND PROSGEGRAMMENI */
  case 0x1F89: out[0] = 0x1F01; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND PROSGEGRAMMENI */
  case 0x1F8A: out[0] = 0x1F02; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND VARIA AND PROSGEGRAMMENI */
  case 0x1F8B: out[0] = 0x1F03; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND VARIA AND PROSGEGRAMMENI */
  case 0x1F8C: out[0] = 0x1F04; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND OXIA AND PROSGEGRAMMENI */
  case 0x1F8D: out[0] = 0x1F05; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND OXIA AND PROSGEGRAMMENI */
  case 0x1F8E: out[0] = 0x1F06; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND PERISPOMENI AND PROSGEGRAMMENI */
  case 0x1F8F: out[0] = 0x1F07; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND PERISPOMENI AND PROSGEGRAMMENI */
  case 0x1F90: out[0] = 0x1F20; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ETA WITH PSILI AND YPOGEGRAMMENI */
  case 0x1F91: out[0] = 0x1F21; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ETA WITH DASIA AND YPOGEGRAMMENI */
  case 0x1F92: out[0] = 0x1F22; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ETA WITH PSILI AND VARIA AND YPOGEGRAMMENI */
  case 0x1F93: out[0] = 0x1F23; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ETA WITH DASIA AND VARIA AND YPOGEGRAMMENI */
  case 0x1F94: out[0] = 0x1F24; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ETA WITH PSILI AND OXIA AND YPOGEGRAMMENI */
  case 0x1F95: out[0] = 0x1F25; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ETA WITH DASIA AND OXIA AND YPOGEGRAMMENI */
  case 0x1F96: out[0] = 0x1F26; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ETA WITH PSILI AND PERISPOMENI AND YPOGEGRAMMENI */
  case 0x1F97: out[0] = 0x1F27; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ETA WITH DASIA AND PERISPOMENI AND YPOGEGRAMMENI */
  case 0x1F98: out[0] = 0x1F20; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ETA WITH PSILI AND PROSGEGRAMMENI */
  case 0x1F99: out[0] = 0x1F21; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ETA WITH DASIA AND PROSGEGRAMMENI */
  case 0x1F9A: out[0] = 0x1F22; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ETA WITH PSILI AND VARIA AND PROSGEGRAMMENI */
  case 0x1F9B: out[0] = 0x1F23; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ETA WITH DASIA AND VARIA AND PROSGEGRAMMENI */
  case 0x1F9C: out[0] = 0x1F24; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ETA WITH PSILI AND OXIA AND PROSGEGRAMMENI */
  case 0x1F9D: out[0] = 0x1F25; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ETA WITH DASIA AND OXIA AND PROSGEGRAMMENI */
  case 0x1F9E: out[0] = 0x1F26; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ETA WITH PSILI AND PERISPOMENI AND PROSGEGRAMMENI */
  case 0x1F9F: out[0] = 0x1F27; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ETA WITH DASIA AND PERISPOMENI AND PROSGEGRAMMENI */
  case 0x1FA0: out[0] = 0x1F60; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER OMEGA WITH PSILI AND YPOGEGRAMMENI */
  case 0x1FA1: out[0] = 0x1F61; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER OMEGA WITH DASIA AND YPOGEGRAMMENI */
  case 0x1FA2: out[0] = 0x1F62; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER OMEGA WITH PSILI AND VARIA AND YPOGEGRAMMENI */
  case 0x1FA3: out[0] = 0x1F63; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER OMEGA WITH DASIA AND VARIA AND YPOGEGRAMMENI */
  case 0x1FA4: out[0] = 0x1F64; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER OMEGA WITH PSILI AND OXIA AND YPOGEGRAMMENI */
  case 0x1FA5: out[0] = 0x1F65; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER OMEGA WITH DASIA AND OXIA AND YPOGEGRAMMENI */
  case 0x1FA6: out[0] = 0x1F66; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER OMEGA WITH PSILI AND PERISPOMENI AND YPOGEGRAMMENI */
  case 0x1FA7: out[0] = 0x1F67; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER OMEGA WITH DASIA AND PERISPOMENI AND YPOGEGRAMMENI */
  case 0x1FA8: out[0] = 0x1F60; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND PROSGEGRAMMENI */
  case 0x1FA9: out[0] = 0x1F61; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND PROSGEGRAMMENI */
  case 0x1FAA: out[0] = 0x1F62; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND VARIA AND PROSGEGRAMMENI */
  case 0x1FAB: out[0] = 0x1F63; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND VARIA AND PROSGEGRAMMENI */
  case 0x1FAC: out[0] = 0x1F64; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND OXIA AND PROSGEGRAMMENI */
  case 0x1FAD: out[0] = 0x1F65; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND OXIA AND PROSGEGRAMMENI */
  case 0x1FAE: out[0] = 0x1F66; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND PERISPOMENI AND PROSGEGRAMMENI */
  case 0x1FAF: out[0] = 0x1F67; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND PERISPOMENI AND PROSGEGRAMMENI */
  case 0x1FB2: out[0] = 0x1F70; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ALPHA WITH VARIA AND YPOGEGRAMMENI */
  case 0x1FB3: out[0] = 0x03B1; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ALPHA WITH YPOGEGRAMMENI */
  case 0x1FB4: out[0] = 0x03AC; out[1] = 0x03B9; return 2; /* GREEK SMALL LETTER ALPHA WITH OXIA AND YPOGEGRAMMENI */
  case 0x1FB6: out[0] = 0x03B1; out[1] = 0x0342; return 2; /* GREEK SMALL LETTER ALPHA WITH PERISPOMENI */
  case 0x1FB7: out[0] = 0x03B1; out[1] = 0x0342; out[2] = 0x03B9; return 3; /* GREEK SMALL LETTER ALPHA WITH PERISPOMENI AND YPOGEGRAMMENI */
  case 0x1FB8: out[0] = 0x1FB0; return 1; /* GREEK CAPITAL LETTER ALPHA WITH VRACHY */
  case 0x1FB9: out[0] = 0x1FB1; return 1; /* GREEK CAPITAL LETTER ALPHA WITH MACRON */
  case 0x1FBA: out[0] = 0x1F70; return 1; /* GREEK CAPITAL LETTER ALPHA WITH VARIA */
  case 0x1FBB: out[0] = 0x1F71; return 1; /* GREEK CAPITAL LETTER ALPHA WITH OXIA */
  case 0x1FBC: out[0] = 0x03B1; out[1] = 0x03B9; return 2;/* GREEK CAPITAL LETTER ALPHA WITH PROSGEGRAMMENI */
  case 0x1FBE: out[0] = 0x03B9; return 1; /* GREEK PROSGEGRAMMENI */
  case 0x1FC2: out[0] = 0x1F74; out[1] = 0x03B9; return 2;/* GREEK SMALL LETTER ETA WITH VARIA AND YPOGEGRAMMENI */
  case 0x1FC3: out[0] = 0x03B7; out[1] = 0x03B9; return 2;/* GREEK SMALL LETTER ETA WITH YPOGEGRAMMENI */
  case 0x1FC4: out[0] = 0x03AE; out[1] = 0x03B9; return 2;/* GREEK SMALL LETTER ETA WITH OXIA AND YPOGEGRAMMENI */
  case 0x1FC6: out[0] = 0x03B7; out[1] = 0x0342; return 2;/* GREEK SMALL LETTER ETA WITH PERISPOMENI */
  case 0x1FC7: out[0] = 0x03B7; out[1] = 0x0342; out[2] = 0x03B9; return 3; /* GREEK SMALL LETTER ETA WITH PERISPOMENI AND YPOGEGRAMMENI */
  case 0x1FC8: out[0] = 0x1F72; return 1; /* GREEK CAPITAL LETTER EPSILON WITH VARIA */
  case 0x1FC9: out[0] = 0x1F73; return 1; /* GREEK CAPITAL LETTER EPSILON WITH OXIA */
  case 0x1FCA: out[0] = 0x1F74; return 1; /* GREEK CAPITAL LETTER ETA WITH VARIA */
  case 0x1FCB: out[0] = 0x1F75; return 1; /* GREEK CAPITAL LETTER ETA WITH OXIA */
  case 0x1FCC: out[0] = 0x03B7; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER ETA WITH PROSGEGRAMMENI */
  case 0x1FD2: out[0] = 0x03B9; out[1] = 0x0308; out[2] = 0x0300; return 3; /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND VARIA */
  case 0x1FD3: out[0] = 0x03B9; out[1] = 0x0308; out[2] = 0x0301; return 3; /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND OXIA */
  case 0x1FD6: out[0] = 0x03B9; out[1] = 0x0342; return 2;/* GREEK SMALL LETTER IOTA WITH PERISPOMENI */
  case 0x1FD7: out[0] = 0x03B9; out[1] = 0x0308; out[2] = 0x0342; return 3; /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND PERISPOMENI */
  case 0x1FD8: out[0] = 0x1FD0; return 1; /* GREEK CAPITAL LETTER IOTA WITH VRACHY */
  case 0x1FD9: out[0] = 0x1FD1; return 1; /* GREEK CAPITAL LETTER IOTA WITH MACRON */
  case 0x1FDA: out[0] = 0x1F76; return 1; /* GREEK CAPITAL LETTER IOTA WITH VARIA */
  case 0x1FDB: out[0] = 0x1F77; return 1; /* GREEK CAPITAL LETTER IOTA WITH OXIA */
  case 0x1FE2: out[0] = 0x03C5; out[1] = 0x0308; out[2] = 0x0300; return 3; /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND VARIA */
  case 0x1FE3: out[0] = 0x03C5; out[1] = 0x0308; out[2] = 0x0301; return 3;/* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND OXIA */
  case 0x1FE4: out[0] = 0x03C1; out[1] = 0x0313; return 2;/* GREEK SMALL LETTER RHO WITH PSILI */
  case 0x1FE6: out[0] = 0x03C5; out[1] = 0x0342; return 2;/* GREEK SMALL LETTER UPSILON WITH PERISPOMENI */
  case 0x1FE7: out[0] = 0x03C5; out[1] = 0x0308; out[2] = 0x0342; return 3;/* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND PERISPOMENI */
  case 0x1FE8: out[0] = 0x1FE0; return 1; /* GREEK CAPITAL LETTER UPSILON WITH VRACHY */
  case 0x1FE9: out[0] = 0x1FE1; return 1; /* GREEK CAPITAL LETTER UPSILON WITH MACRON */
  case 0x1FEA: out[0] = 0x1F7A; return 1; /* GREEK CAPITAL LETTER UPSILON WITH VARIA */
  case 0x1FEB: out[0] = 0x1F7B; return 1; /* GREEK CAPITAL LETTER UPSILON WITH OXIA */
  case 0x1FEC: out[0] = 0x1FE5; return 1; /* GREEK CAPITAL LETTER RHO WITH DASIA */
  case 0x1FF2: out[0] = 0x1F7C; out[1] = 0x03B9; return 2;/* GREEK SMALL LETTER OMEGA WITH VARIA AND YPOGEGRAMMENI */
  case 0x1FF3: out[0] = 0x03C9; out[1] = 0x03B9; return 2;/* GREEK SMALL LETTER OMEGA WITH YPOGEGRAMMENI */
  case 0x1FF4: out[0] = 0x03CE; out[1] = 0x03B9; return 2;/* GREEK SMALL LETTER OMEGA WITH OXIA AND YPOGEGRAMMENI */
  case 0x1FF6: out[0] = 0x03C9; out[1] = 0x0342; return 2;/* GREEK SMALL LETTER OMEGA WITH PERISPOMENI */
  case 0x1FF7: out[0] = 0x03C9; out[1] = 0x0342; out[2] = 0x03B9; return 3; /* GREEK SMALL LETTER OMEGA WITH PERISPOMENI AND YPOGEGRAMMENI */
  case 0x1FF8: out[0] = 0x1F78; return 1; /* GREEK CAPITAL LETTER OMICRON WITH VARIA */
  case 0x1FF9: out[0] = 0x1F79; return 1; /* GREEK CAPITAL LETTER OMICRON WITH OXIA */
  case 0x1FFA: out[0] = 0x1F7C; return 1; /* GREEK CAPITAL LETTER OMEGA WITH VARIA */
  case 0x1FFB: out[0] = 0x1F7D; return 1; /* GREEK CAPITAL LETTER OMEGA WITH OXIA */
  case 0x1FFC: out[0] = 0x03C9; out[1] = 0x03B9; return 2; /* GREEK CAPITAL LETTER OMEGA WITH PROSGEGRAMMENI */
  case 0x2126: out[0] = 0x03C9; return 1; /* OHM SIGN */
  case 0x212A: out[0] = 0x006B; return 1; /* KELVIN SIGN */
  case 0x212B: out[0] = 0x00E5; return 1; /* ANGSTROM SIGN */
  case 0x2132: out[0] = 0x214E; return 1; /* TURNED CAPITAL F */
  case 0x2160: out[0] = 0x2170; return 1; /* ROMAN NUMERAL ONE */
  case 0x2161: out[0] = 0x2171; return 1; /* ROMAN NUMERAL TWO */
  case 0x2162: out[0] = 0x2172; return 1; /* ROMAN NUMERAL THREE */
  case 0x2163: out[0] = 0x2173; return 1; /* ROMAN NUMERAL FOUR */
  case 0x2164: out[0] = 0x2174; return 1; /* ROMAN NUMERAL FIVE */
  case 0x2165: out[0] = 0x2175; return 1; /* ROMAN NUMERAL SIX */
  case 0x2166: out[0] = 0x2176; return 1; /* ROMAN NUMERAL SEVEN */
  case 0x2167: out[0] = 0x2177; return 1; /* ROMAN NUMERAL EIGHT */
  case 0x2168: out[0] = 0x2178; return 1; /* ROMAN NUMERAL NINE */
  case 0x2169: out[0] = 0x2179; return 1; /* ROMAN NUMERAL TEN */
  case 0x216A: out[0] = 0x217A; return 1; /* ROMAN NUMERAL ELEVEN */
  case 0x216B: out[0] = 0x217B; return 1; /* ROMAN NUMERAL TWELVE */
  case 0x216C: out[0] = 0x217C; return 1; /* ROMAN NUMERAL FIFTY */
  case 0x216D: out[0] = 0x217D; return 1; /* ROMAN NUMERAL ONE HUNDRED */
  case 0x216E: out[0] = 0x217E; return 1; /* ROMAN NUMERAL FIVE HUNDRED */
  case 0x216F: out[0] = 0x217F; return 1; /* ROMAN NUMERAL ONE THOUSAND */
  case 0x2183: out[0] = 0x2184; return 1; /* ROMAN NUMERAL REVERSED ONE HUNDRED */
  case 0x24B6: out[0] = 0x24D0; return 1; /* CIRCLED LATIN CAPITAL LETTER A */
  case 0x24B7: out[0] = 0x24D1; return 1; /* CIRCLED LATIN CAPITAL LETTER B */
  case 0x24B8: out[0] = 0x24D2; return 1; /* CIRCLED LATIN CAPITAL LETTER C */
  case 0x24B9: out[0] = 0x24D3; return 1; /* CIRCLED LATIN CAPITAL LETTER D */
  case 0x24BA: out[0] = 0x24D4; return 1; /* CIRCLED LATIN CAPITAL LETTER E */
  case 0x24BB: out[0] = 0x24D5; return 1; /* CIRCLED LATIN CAPITAL LETTER F */
  case 0x24BC: out[0] = 0x24D6; return 1; /* CIRCLED LATIN CAPITAL LETTER G */
  case 0x24BD: out[0] = 0x24D7; return 1; /* CIRCLED LATIN CAPITAL LETTER H */
  case 0x24BE: out[0] = 0x24D8; return 1; /* CIRCLED LATIN CAPITAL LETTER I */
  case 0x24BF: out[0] = 0x24D9; return 1; /* CIRCLED LATIN CAPITAL LETTER J */
  case 0x24C0: out[0] = 0x24DA; return 1; /* CIRCLED LATIN CAPITAL LETTER K */
  case 0x24C1: out[0] = 0x24DB; return 1; /* CIRCLED LATIN CAPITAL LETTER L */
  case 0x24C2: out[0] = 0x24DC; return 1; /* CIRCLED LATIN CAPITAL LETTER M */
  case 0x24C3: out[0] = 0x24DD; return 1; /* CIRCLED LATIN CAPITAL LETTER N */
  case 0x24C4: out[0] = 0x24DE; return 1; /* CIRCLED LATIN CAPITAL LETTER O */
  case 0x24C5: out[0] = 0x24DF; return 1; /* CIRCLED LATIN CAPITAL LETTER P */
  case 0x24C6: out[0] = 0x24E0; return 1; /* CIRCLED LATIN CAPITAL LETTER Q */
  case 0x24C7: out[0] = 0x24E1; return 1; /* CIRCLED LATIN CAPITAL LETTER R */
  case 0x24C8: out[0] = 0x24E2; return 1; /* CIRCLED LATIN CAPITAL LETTER S */
  case 0x24C9: out[0] = 0x24E3; return 1; /* CIRCLED LATIN CAPITAL LETTER T */
  case 0x24CA: out[0] = 0x24E4; return 1; /* CIRCLED LATIN CAPITAL LETTER U */
  case 0x24CB: out[0] = 0x24E5; return 1; /* CIRCLED LATIN CAPITAL LETTER V */
  case 0x24CC: out[0] = 0x24E6; return 1; /* CIRCLED LATIN CAPITAL LETTER W */
  case 0x24CD: out[0] = 0x24E7; return 1; /* CIRCLED LATIN CAPITAL LETTER X */
  case 0x24CE: out[0] = 0x24E8; return 1; /* CIRCLED LATIN CAPITAL LETTER Y */
  case 0x24CF: out[0] = 0x24E9; return 1; /* CIRCLED LATIN CAPITAL LETTER Z */
  case 0x2C00: out[0] = 0x2C30; return 1; /* GLAGOLITIC CAPITAL LETTER AZU */
  case 0x2C01: out[0] = 0x2C31; return 1; /* GLAGOLITIC CAPITAL LETTER BUKY */
  case 0x2C02: out[0] = 0x2C32; return 1; /* GLAGOLITIC CAPITAL LETTER VEDE */
  case 0x2C03: out[0] = 0x2C33; return 1; /* GLAGOLITIC CAPITAL LETTER GLAGOLI */
  case 0x2C04: out[0] = 0x2C34; return 1; /* GLAGOLITIC CAPITAL LETTER DOBRO */
  case 0x2C05: out[0] = 0x2C35; return 1; /* GLAGOLITIC CAPITAL LETTER YESTU */
  case 0x2C06: out[0] = 0x2C36; return 1; /* GLAGOLITIC CAPITAL LETTER ZHIVETE */
  case 0x2C07: out[0] = 0x2C37; return 1; /* GLAGOLITIC CAPITAL LETTER DZELO */
  case 0x2C08: out[0] = 0x2C38; return 1; /* GLAGOLITIC CAPITAL LETTER ZEMLJA */
  case 0x2C09: out[0] = 0x2C39; return 1; /* GLAGOLITIC CAPITAL LETTER IZHE */
  case 0x2C0A: out[0] = 0x2C3A; return 1; /* GLAGOLITIC CAPITAL LETTER INITIAL IZHE */
  case 0x2C0B: out[0] = 0x2C3B; return 1; /* GLAGOLITIC CAPITAL LETTER I */
  case 0x2C0C: out[0] = 0x2C3C; return 1; /* GLAGOLITIC CAPITAL LETTER DJERVI */
  case 0x2C0D: out[0] = 0x2C3D; return 1; /* GLAGOLITIC CAPITAL LETTER KAKO */
  case 0x2C0E: out[0] = 0x2C3E; return 1; /* GLAGOLITIC CAPITAL LETTER LJUDIJE */
  case 0x2C0F: out[0] = 0x2C3F; return 1; /* GLAGOLITIC CAPITAL LETTER MYSLITE */
  case 0x2C10: out[0] = 0x2C40; return 1; /* GLAGOLITIC CAPITAL LETTER NASHI */
  case 0x2C11: out[0] = 0x2C41; return 1; /* GLAGOLITIC CAPITAL LETTER ONU */
  case 0x2C12: out[0] = 0x2C42; return 1; /* GLAGOLITIC CAPITAL LETTER POKOJI */
  case 0x2C13: out[0] = 0x2C43; return 1; /* GLAGOLITIC CAPITAL LETTER RITSI */
  case 0x2C14: out[0] = 0x2C44; return 1; /* GLAGOLITIC CAPITAL LETTER SLOVO */
  case 0x2C15: out[0] = 0x2C45; return 1; /* GLAGOLITIC CAPITAL LETTER TVRIDO */
  case 0x2C16: out[0] = 0x2C46; return 1; /* GLAGOLITIC CAPITAL LETTER UKU */
  case 0x2C17: out[0] = 0x2C47; return 1; /* GLAGOLITIC CAPITAL LETTER FRITU */
  case 0x2C18: out[0] = 0x2C48; return 1; /* GLAGOLITIC CAPITAL LETTER HERU */
  case 0x2C19: out[0] = 0x2C49; return 1; /* GLAGOLITIC CAPITAL LETTER OTU */
  case 0x2C1A: out[0] = 0x2C4A; return 1; /* GLAGOLITIC CAPITAL LETTER PE */
  case 0x2C1B: out[0] = 0x2C4B; return 1; /* GLAGOLITIC CAPITAL LETTER SHTA */
  case 0x2C1C: out[0] = 0x2C4C; return 1; /* GLAGOLITIC CAPITAL LETTER TSI */
  case 0x2C1D: out[0] = 0x2C4D; return 1; /* GLAGOLITIC CAPITAL LETTER CHRIVI */
  case 0x2C1E: out[0] = 0x2C4E; return 1; /* GLAGOLITIC CAPITAL LETTER SHA */
  case 0x2C1F: out[0] = 0x2C4F; return 1; /* GLAGOLITIC CAPITAL LETTER YERU */
  case 0x2C20: out[0] = 0x2C50; return 1; /* GLAGOLITIC CAPITAL LETTER YERI */
  case 0x2C21: out[0] = 0x2C51; return 1; /* GLAGOLITIC CAPITAL LETTER YATI */
  case 0x2C22: out[0] = 0x2C52; return 1; /* GLAGOLITIC CAPITAL LETTER SPIDERY HA */
  case 0x2C23: out[0] = 0x2C53; return 1; /* GLAGOLITIC CAPITAL LETTER YU */
  case 0x2C24: out[0] = 0x2C54; return 1; /* GLAGOLITIC CAPITAL LETTER SMALL YUS */
  case 0x2C25: out[0] = 0x2C55; return 1; /* GLAGOLITIC CAPITAL LETTER SMALL YUS WITH TAIL */
  case 0x2C26: out[0] = 0x2C56; return 1; /* GLAGOLITIC CAPITAL LETTER YO */
  case 0x2C27: out[0] = 0x2C57; return 1; /* GLAGOLITIC CAPITAL LETTER IOTATED SMALL YUS */
  case 0x2C28: out[0] = 0x2C58; return 1; /* GLAGOLITIC CAPITAL LETTER BIG YUS */
  case 0x2C29: out[0] = 0x2C59; return 1; /* GLAGOLITIC CAPITAL LETTER IOTATED BIG YUS */
  case 0x2C2A: out[0] = 0x2C5A; return 1; /* GLAGOLITIC CAPITAL LETTER FITA */
  case 0x2C2B: out[0] = 0x2C5B; return 1; /* GLAGOLITIC CAPITAL LETTER IZHITSA */
  case 0x2C2C: out[0] = 0x2C5C; return 1; /* GLAGOLITIC CAPITAL LETTER SHTAPIC */
  case 0x2C2D: out[0] = 0x2C5D; return 1; /* GLAGOLITIC CAPITAL LETTER TROKUTASTI A */
  case 0x2C2E: out[0] = 0x2C5E; return 1; /* GLAGOLITIC CAPITAL LETTER LATINATE MYSLITE */
  case 0x2C2F: out[0] = 0x2C5F; return 1; /* GLAGOLITIC CAPITAL LETTER CAUDATE CHRIVI */
  case 0x2C60: out[0] = 0x2C61; return 1; /* LATIN CAPITAL LETTER L WITH DOUBLE BAR */
  case 0x2C62: out[0] = 0x026B; return 1; /* LATIN CAPITAL LETTER L WITH MIDDLE TILDE */
  case 0x2C63: out[0] = 0x1D7D; return 1; /* LATIN CAPITAL LETTER P WITH STROKE */
  case 0x2C64: out[0] = 0x027D; return 1; /* LATIN CAPITAL LETTER R WITH TAIL */
  case 0x2C67: out[0] = 0x2C68; return 1; /* LATIN CAPITAL LETTER H WITH DESCENDER */
  case 0x2C69: out[0] = 0x2C6A; return 1; /* LATIN CAPITAL LETTER K WITH DESCENDER */
  case 0x2C6B: out[0] = 0x2C6C; return 1; /* LATIN CAPITAL LETTER Z WITH DESCENDER */
  case 0x2C6D: out[0] = 0x0251; return 1; /* LATIN CAPITAL LETTER ALPHA */
  case 0x2C6E: out[0] = 0x0271; return 1; /* LATIN CAPITAL LETTER M WITH HOOK */
  case 0x2C6F: out[0] = 0x0250; return 1; /* LATIN CAPITAL LETTER TURNED A */
  case 0x2C70: out[0] = 0x0252; return 1; /* LATIN CAPITAL LETTER TURNED ALPHA */
  case 0x2C72: out[0] = 0x2C73; return 1; /* LATIN CAPITAL LETTER W WITH HOOK */
  case 0x2C75: out[0] = 0x2C76; return 1; /* LATIN CAPITAL LETTER HALF H */
  case 0x2C7E: out[0] = 0x023F; return 1; /* LATIN CAPITAL LETTER S WITH SWASH TAIL */
  case 0x2C7F: out[0] = 0x0240; return 1; /* LATIN CAPITAL LETTER Z WITH SWASH TAIL */
  case 0x2C80: out[0] = 0x2C81; return 1; /* COPTIC CAPITAL LETTER ALFA */
  case 0x2C82: out[0] = 0x2C83; return 1; /* COPTIC CAPITAL LETTER VIDA */
  case 0x2C84: out[0] = 0x2C85; return 1; /* COPTIC CAPITAL LETTER GAMMA */
  case 0x2C86: out[0] = 0x2C87; return 1; /* COPTIC CAPITAL LETTER DALDA */
  case 0x2C88: out[0] = 0x2C89; return 1; /* COPTIC CAPITAL LETTER EIE */
  case 0x2C8A: out[0] = 0x2C8B; return 1; /* COPTIC CAPITAL LETTER SOU */
  case 0x2C8C: out[0] = 0x2C8D; return 1; /* COPTIC CAPITAL LETTER ZATA */
  case 0x2C8E: out[0] = 0x2C8F; return 1; /* COPTIC CAPITAL LETTER HATE */
  case 0x2C90: out[0] = 0x2C91; return 1; /* COPTIC CAPITAL LETTER THETHE */
  case 0x2C92: out[0] = 0x2C93; return 1; /* COPTIC CAPITAL LETTER IAUDA */
  case 0x2C94: out[0] = 0x2C95; return 1; /* COPTIC CAPITAL LETTER KAPA */
  case 0x2C96: out[0] = 0x2C97; return 1; /* COPTIC CAPITAL LETTER LAULA */
  case 0x2C98: out[0] = 0x2C99; return 1; /* COPTIC CAPITAL LETTER MI */
  case 0x2C9A: out[0] = 0x2C9B; return 1; /* COPTIC CAPITAL LETTER NI */
  case 0x2C9C: out[0] = 0x2C9D; return 1; /* COPTIC CAPITAL LETTER KSI */
  case 0x2C9E: out[0] = 0x2C9F; return 1; /* COPTIC CAPITAL LETTER O */
  case 0x2CA0: out[0] = 0x2CA1; return 1; /* COPTIC CAPITAL LETTER PI */
  case 0x2CA2: out[0] = 0x2CA3; return 1; /* COPTIC CAPITAL LETTER RO */
  case 0x2CA4: out[0] = 0x2CA5; return 1; /* COPTIC CAPITAL LETTER SIMA */
  case 0x2CA6: out[0] = 0x2CA7; return 1; /* COPTIC CAPITAL LETTER TAU */
  case 0x2CA8: out[0] = 0x2CA9; return 1; /* COPTIC CAPITAL LETTER UA */
  case 0x2CAA: out[0] = 0x2CAB; return 1; /* COPTIC CAPITAL LETTER FI */
  case 0x2CAC: out[0] = 0x2CAD; return 1; /* COPTIC CAPITAL LETTER KHI */
  case 0x2CAE: out[0] = 0x2CAF; return 1; /* COPTIC CAPITAL LETTER PSI */
  case 0x2CB0: out[0] = 0x2CB1; return 1; /* COPTIC CAPITAL LETTER OOU */
  case 0x2CB2: out[0] = 0x2CB3; return 1; /* COPTIC CAPITAL LETTER DIALECT-P ALEF */
  case 0x2CB4: out[0] = 0x2CB5; return 1; /* COPTIC CAPITAL LETTER OLD COPTIC AIN */
  case 0x2CB6: out[0] = 0x2CB7; return 1; /* COPTIC CAPITAL LETTER CRYPTOGRAMMIC EIE */
  case 0x2CB8: out[0] = 0x2CB9; return 1; /* COPTIC CAPITAL LETTER DIALECT-P KAPA */
  case 0x2CBA: out[0] = 0x2CBB; return 1; /* COPTIC CAPITAL LETTER DIALECT-P NI */
  case 0x2CBC: out[0] = 0x2CBD; return 1; /* COPTIC CAPITAL LETTER CRYPTOGRAMMIC NI */
  case 0x2CBE: out[0] = 0x2CBF; return 1; /* COPTIC CAPITAL LETTER OLD COPTIC OOU */
  case 0x2CC0: out[0] = 0x2CC1; return 1; /* COPTIC CAPITAL LETTER SAMPI */
  case 0x2CC2: out[0] = 0x2CC3; return 1; /* COPTIC CAPITAL LETTER CROSSED SHEI */
  case 0x2CC4: out[0] = 0x2CC5; return 1; /* COPTIC CAPITAL LETTER OLD COPTIC SHEI */
  case 0x2CC6: out[0] = 0x2CC7; return 1; /* COPTIC CAPITAL LETTER OLD COPTIC ESH */
  case 0x2CC8: out[0] = 0x2CC9; return 1; /* COPTIC CAPITAL LETTER AKHMIMIC KHEI */
  case 0x2CCA: out[0] = 0x2CCB; return 1; /* COPTIC CAPITAL LETTER DIALECT-P HORI */
  case 0x2CCC: out[0] = 0x2CCD; return 1; /* COPTIC CAPITAL LETTER OLD COPTIC HORI */
  case 0x2CCE: out[0] = 0x2CCF; return 1; /* COPTIC CAPITAL LETTER OLD COPTIC HA */
  case 0x2CD0: out[0] = 0x2CD1; return 1; /* COPTIC CAPITAL LETTER L-SHAPED HA */
  case 0x2CD2: out[0] = 0x2CD3; return 1; /* COPTIC CAPITAL LETTER OLD COPTIC HEI */
  case 0x2CD4: out[0] = 0x2CD5; return 1; /* COPTIC CAPITAL LETTER OLD COPTIC HAT */
  case 0x2CD6: out[0] = 0x2CD7; return 1; /* COPTIC CAPITAL LETTER OLD COPTIC GANGIA */
  case 0x2CD8: out[0] = 0x2CD9; return 1; /* COPTIC CAPITAL LETTER OLD COPTIC DJA */
  case 0x2CDA: out[0] = 0x2CDB; return 1; /* COPTIC CAPITAL LETTER OLD COPTIC SHIMA */
  case 0x2CDC: out[0] = 0x2CDD; return 1; /* COPTIC CAPITAL LETTER OLD NUBIAN SHIMA */
  case 0x2CDE: out[0] = 0x2CDF; return 1; /* COPTIC CAPITAL LETTER OLD NUBIAN NGI */
  case 0x2CE0: out[0] = 0x2CE1; return 1; /* COPTIC CAPITAL LETTER OLD NUBIAN NYI */
  case 0x2CE2: out[0] = 0x2CE3; return 1; /* COPTIC CAPITAL LETTER OLD NUBIAN WAU */
  case 0x2CEB: out[0] = 0x2CEC; return 1; /* COPTIC CAPITAL LETTER CRYPTOGRAMMIC SHEI */
  case 0x2CED: out[0] = 0x2CEE; return 1; /* COPTIC CAPITAL LETTER CRYPTOGRAMMIC GANGIA */
  case 0x2CF2: out[0] = 0x2CF3; return 1; /* COPTIC CAPITAL LETTER BOHAIRIC KHEI */
  case 0xA640: out[0] = 0xA641; return 1; /* CYRILLIC CAPITAL LETTER ZEMLYA */
  case 0xA642: out[0] = 0xA643; return 1; /* CYRILLIC CAPITAL LETTER DZELO */
  case 0xA644: out[0] = 0xA645; return 1; /* CYRILLIC CAPITAL LETTER REVERSED DZE */
  case 0xA646: out[0] = 0xA647; return 1; /* CYRILLIC CAPITAL LETTER IOTA */
  case 0xA648: out[0] = 0xA649; return 1; /* CYRILLIC CAPITAL LETTER DJERV */
  case 0xA64A: out[0] = 0xA64B; return 1; /* CYRILLIC CAPITAL LETTER MONOGRAPH UK */
  case 0xA64C: out[0] = 0xA64D; return 1; /* CYRILLIC CAPITAL LETTER BROAD OMEGA */
  case 0xA64E: out[0] = 0xA64F; return 1; /* CYRILLIC CAPITAL LETTER NEUTRAL YER */
  case 0xA650: out[0] = 0xA651; return 1; /* CYRILLIC CAPITAL LETTER YERU WITH BACK YER */
  case 0xA652: out[0] = 0xA653; return 1; /* CYRILLIC CAPITAL LETTER IOTIFIED YAT */
  case 0xA654: out[0] = 0xA655; return 1; /* CYRILLIC CAPITAL LETTER REVERSED YU */
  case 0xA656: out[0] = 0xA657; return 1; /* CYRILLIC CAPITAL LETTER IOTIFIED A */
  case 0xA658: out[0] = 0xA659; return 1; /* CYRILLIC CAPITAL LETTER CLOSED LITTLE YUS */
  case 0xA65A: out[0] = 0xA65B; return 1; /* CYRILLIC CAPITAL LETTER BLENDED YUS */
  case 0xA65C: out[0] = 0xA65D; return 1; /* CYRILLIC CAPITAL LETTER IOTIFIED CLOSED LITTLE YUS */
  case 0xA65E: out[0] = 0xA65F; return 1; /* CYRILLIC CAPITAL LETTER YN */
  case 0xA660: out[0] = 0xA661; return 1; /* CYRILLIC CAPITAL LETTER REVERSED TSE */
  case 0xA662: out[0] = 0xA663; return 1; /* CYRILLIC CAPITAL LETTER SOFT DE */
  case 0xA664: out[0] = 0xA665; return 1; /* CYRILLIC CAPITAL LETTER SOFT EL */
  case 0xA666: out[0] = 0xA667; return 1; /* CYRILLIC CAPITAL LETTER SOFT EM */
  case 0xA668: out[0] = 0xA669; return 1; /* CYRILLIC CAPITAL LETTER MONOCULAR O */
  case 0xA66A: out[0] = 0xA66B; return 1; /* CYRILLIC CAPITAL LETTER BINOCULAR O */
  case 0xA66C: out[0] = 0xA66D; return 1; /* CYRILLIC CAPITAL LETTER DOUBLE MONOCULAR O */
  case 0xA680: out[0] = 0xA681; return 1; /* CYRILLIC CAPITAL LETTER DWE */
  case 0xA682: out[0] = 0xA683; return 1; /* CYRILLIC CAPITAL LETTER DZWE */
  case 0xA684: out[0] = 0xA685; return 1; /* CYRILLIC CAPITAL LETTER ZHWE */
  case 0xA686: out[0] = 0xA687; return 1; /* CYRILLIC CAPITAL LETTER CCHE */
  case 0xA688: out[0] = 0xA689; return 1; /* CYRILLIC CAPITAL LETTER DZZE */
  case 0xA68A: out[0] = 0xA68B; return 1; /* CYRILLIC CAPITAL LETTER TE WITH MIDDLE HOOK */
  case 0xA68C: out[0] = 0xA68D; return 1; /* CYRILLIC CAPITAL LETTER TWE */
  case 0xA68E: out[0] = 0xA68F; return 1; /* CYRILLIC CAPITAL LETTER TSWE */
  case 0xA690: out[0] = 0xA691; return 1; /* CYRILLIC CAPITAL LETTER TSSE */
  case 0xA692: out[0] = 0xA693; return 1; /* CYRILLIC CAPITAL LETTER TCHE */
  case 0xA694: out[0] = 0xA695; return 1; /* CYRILLIC CAPITAL LETTER HWE */
  case 0xA696: out[0] = 0xA697; return 1; /* CYRILLIC CAPITAL LETTER SHWE */
  case 0xA698: out[0] = 0xA699; return 1; /* CYRILLIC CAPITAL LETTER DOUBLE O */
  case 0xA69A: out[0] = 0xA69B; return 1; /* CYRILLIC CAPITAL LETTER CROSSED O */
  case 0xA722: out[0] = 0xA723; return 1; /* LATIN CAPITAL LETTER EGYPTOLOGICAL ALEF */
  case 0xA724: out[0] = 0xA725; return 1; /* LATIN CAPITAL LETTER EGYPTOLOGICAL AIN */
  case 0xA726: out[0] = 0xA727; return 1; /* LATIN CAPITAL LETTER HENG */
  case 0xA728: out[0] = 0xA729; return 1; /* LATIN CAPITAL LETTER TZ */
  case 0xA72A: out[0] = 0xA72B; return 1; /* LATIN CAPITAL LETTER TRESILLO */
  case 0xA72C: out[0] = 0xA72D; return 1; /* LATIN CAPITAL LETTER CUATRILLO */
  case 0xA72E: out[0] = 0xA72F; return 1; /* LATIN CAPITAL LETTER CUATRILLO WITH COMMA */
  case 0xA732: out[0] = 0xA733; return 1; /* LATIN CAPITAL LETTER AA */
  case 0xA734: out[0] = 0xA735; return 1; /* LATIN CAPITAL LETTER AO */
  case 0xA736: out[0] = 0xA737; return 1; /* LATIN CAPITAL LETTER AU */
  case 0xA738: out[0] = 0xA739; return 1; /* LATIN CAPITAL LETTER AV */
  case 0xA73A: out[0] = 0xA73B; return 1; /* LATIN CAPITAL LETTER AV WITH HORIZONTAL BAR */
  case 0xA73C: out[0] = 0xA73D; return 1; /* LATIN CAPITAL LETTER AY */
  case 0xA73E: out[0] = 0xA73F; return 1; /* LATIN CAPITAL LETTER REVERSED C WITH DOT */
  case 0xA740: out[0] = 0xA741; return 1; /* LATIN CAPITAL LETTER K WITH STROKE */
  case 0xA742: out[0] = 0xA743; return 1; /* LATIN CAPITAL LETTER K WITH DIAGONAL STROKE */
  case 0xA744: out[0] = 0xA745; return 1; /* LATIN CAPITAL LETTER K WITH STROKE AND DIAGONAL STROKE */
  case 0xA746: out[0] = 0xA747; return 1; /* LATIN CAPITAL LETTER BROKEN L */
  case 0xA748: out[0] = 0xA749; return 1; /* LATIN CAPITAL LETTER L WITH HIGH STROKE */
  case 0xA74A: out[0] = 0xA74B; return 1; /* LATIN CAPITAL LETTER O WITH LONG STROKE OVERLAY */
  case 0xA74C: out[0] = 0xA74D; return 1; /* LATIN CAPITAL LETTER O WITH LOOP */
  case 0xA74E: out[0] = 0xA74F; return 1; /* LATIN CAPITAL LETTER OO */
  case 0xA750: out[0] = 0xA751; return 1; /* LATIN CAPITAL LETTER P WITH STROKE THROUGH DESCENDER */
  case 0xA752: out[0] = 0xA753; return 1; /* LATIN CAPITAL LETTER P WITH FLOURISH */
  case 0xA754: out[0] = 0xA755; return 1; /* LATIN CAPITAL LETTER P WITH SQUIRREL TAIL */
  case 0xA756: out[0] = 0xA757; return 1; /* LATIN CAPITAL LETTER Q WITH STROKE THROUGH DESCENDER */
  case 0xA758: out[0] = 0xA759; return 1; /* LATIN CAPITAL LETTER Q WITH DIAGONAL STROKE */
  case 0xA75A: out[0] = 0xA75B; return 1; /* LATIN CAPITAL LETTER R ROTUNDA */
  case 0xA75C: out[0] = 0xA75D; return 1; /* LATIN CAPITAL LETTER RUM ROTUNDA */
  case 0xA75E: out[0] = 0xA75F; return 1; /* LATIN CAPITAL LETTER V WITH DIAGONAL STROKE */
  case 0xA760: out[0] = 0xA761; return 1; /* LATIN CAPITAL LETTER VY */
  case 0xA762: out[0] = 0xA763; return 1; /* LATIN CAPITAL LETTER VISIGOTHIC Z */
  case 0xA764: out[0] = 0xA765; return 1; /* LATIN CAPITAL LETTER THORN WITH STROKE */
  case 0xA766: out[0] = 0xA767; return 1; /* LATIN CAPITAL LETTER THORN WITH STROKE THROUGH DESCENDER */
  case 0xA768: out[0] = 0xA769; return 1; /* LATIN CAPITAL LETTER VEND */
  case 0xA76A: out[0] = 0xA76B; return 1; /* LATIN CAPITAL LETTER ET */
  case 0xA76C: out[0] = 0xA76D; return 1; /* LATIN CAPITAL LETTER IS */
  case 0xA76E: out[0] = 0xA76F; return 1; /* LATIN CAPITAL LETTER CON */
  case 0xA779: out[0] = 0xA77A; return 1; /* LATIN CAPITAL LETTER INSULAR D */
  case 0xA77B: out[0] = 0xA77C; return 1; /* LATIN CAPITAL LETTER INSULAR F */
  case 0xA77D: out[0] = 0x1D79; return 1; /* LATIN CAPITAL LETTER INSULAR G */
  case 0xA77E: out[0] = 0xA77F; return 1; /* LATIN CAPITAL LETTER TURNED INSULAR G */
  case 0xA780: out[0] = 0xA781; return 1; /* LATIN CAPITAL LETTER TURNED L */
  case 0xA782: out[0] = 0xA783; return 1; /* LATIN CAPITAL LETTER INSULAR R */
  case 0xA784: out[0] = 0xA785; return 1; /* LATIN CAPITAL LETTER INSULAR S */
  case 0xA786: out[0] = 0xA787; return 1; /* LATIN CAPITAL LETTER INSULAR T */
  case 0xA78B: out[0] = 0xA78C; return 1; /* LATIN CAPITAL LETTER SALTILLO */
  case 0xA78D: out[0] = 0x0265; return 1; /* LATIN CAPITAL LETTER TURNED H */
  case 0xA790: out[0] = 0xA791; return 1; /* LATIN CAPITAL LETTER N WITH DESCENDER */
  case 0xA792: out[0] = 0xA793; return 1; /* LATIN CAPITAL LETTER C WITH BAR */
  case 0xA796: out[0] = 0xA797; return 1; /* LATIN CAPITAL LETTER B WITH FLOURISH */
  case 0xA798: out[0] = 0xA799; return 1; /* LATIN CAPITAL LETTER F WITH STROKE */
  case 0xA79A: out[0] = 0xA79B; return 1; /* LATIN CAPITAL LETTER VOLAPUK AE */
  case 0xA79C: out[0] = 0xA79D; return 1; /* LATIN CAPITAL LETTER VOLAPUK OE */
  case 0xA79E: out[0] = 0xA79F; return 1; /* LATIN CAPITAL LETTER VOLAPUK UE */
  case 0xA7A0: out[0] = 0xA7A1; return 1; /* LATIN CAPITAL LETTER G WITH OBLIQUE STROKE */
  case 0xA7A2: out[0] = 0xA7A3; return 1; /* LATIN CAPITAL LETTER K WITH OBLIQUE STROKE */
  case 0xA7A4: out[0] = 0xA7A5; return 1; /* LATIN CAPITAL LETTER N WITH OBLIQUE STROKE */
  case 0xA7A6: out[0] = 0xA7A7; return 1; /* LATIN CAPITAL LETTER R WITH OBLIQUE STROKE */
  case 0xA7A8: out[0] = 0xA7A9; return 1; /* LATIN CAPITAL LETTER S WITH OBLIQUE STROKE */
  case 0xA7AA: out[0] = 0x0266; return 1; /* LATIN CAPITAL LETTER H WITH HOOK */
  case 0xA7AB: out[0] = 0x025C; return 1; /* LATIN CAPITAL LETTER REVERSED OPEN E */
  case 0xA7AC: out[0] = 0x0261; return 1; /* LATIN CAPITAL LETTER SCRIPT G */
  case 0xA7AD: out[0] = 0x026C; return 1; /* LATIN CAPITAL LETTER L WITH BELT */
  case 0xA7AE: out[0] = 0x026A; return 1; /* LATIN CAPITAL LETTER SMALL CAPITAL I */
  case 0xA7B0: out[0] = 0x029E; return 1; /* LATIN CAPITAL LETTER TURNED K */
  case 0xA7B1: out[0] = 0x0287; return 1; /* LATIN CAPITAL LETTER TURNED T */
  case 0xA7B2: out[0] = 0x029D; return 1; /* LATIN CAPITAL LETTER J WITH CROSSED-TAIL */
  case 0xA7B3: out[0] = 0xAB53; return 1; /* LATIN CAPITAL LETTER CHI */
  case 0xA7B4: out[0] = 0xA7B5; return 1; /* LATIN CAPITAL LETTER BETA */
  case 0xA7B6: out[0] = 0xA7B7; return 1; /* LATIN CAPITAL LETTER OMEGA */
  case 0xA7B8: out[0] = 0xA7B9; return 1; /* LATIN CAPITAL LETTER U WITH STROKE */
  case 0xA7BA: out[0] = 0xA7BB; return 1; /* LATIN CAPITAL LETTER GLOTTAL A */
  case 0xA7BC: out[0] = 0xA7BD; return 1; /* LATIN CAPITAL LETTER GLOTTAL I */
  case 0xA7BE: out[0] = 0xA7BF; return 1; /* LATIN CAPITAL LETTER GLOTTAL U */
  case 0xA7C0: out[0] = 0xA7C1; return 1; /* LATIN CAPITAL LETTER OLD POLISH O */
  case 0xA7C2: out[0] = 0xA7C3; return 1; /* LATIN CAPITAL LETTER ANGLICANA W */
  case 0xA7C4: out[0] = 0xA794; return 1; /* LATIN CAPITAL LETTER C WITH PALATAL HOOK */
  case 0xA7C5: out[0] = 0x0282; return 1; /* LATIN CAPITAL LETTER S WITH HOOK */
  case 0xA7C6: out[0] = 0x1D8E; return 1; /* LATIN CAPITAL LETTER Z WITH PALATAL HOOK */
  case 0xA7C7: out[0] = 0xA7C8; return 1; /* LATIN CAPITAL LETTER D WITH SHORT STROKE OVERLAY */
  case 0xA7C9: out[0] = 0xA7CA; return 1; /* LATIN CAPITAL LETTER S WITH SHORT STROKE OVERLAY */
  case 0xA7D0: out[0] = 0xA7D1; return 1; /* LATIN CAPITAL LETTER CLOSED INSULAR G */
  case 0xA7D6: out[0] = 0xA7D7; return 1; /* LATIN CAPITAL LETTER MIDDLE SCOTS S */
  case 0xA7D8: out[0] = 0xA7D9; return 1; /* LATIN CAPITAL LETTER SIGMOID S */
  case 0xA7F5: out[0] = 0xA7F6; return 1; /* LATIN CAPITAL LETTER REVERSED HALF H */
  case 0xAB70: out[0] = 0x13A0; return 1; /* CHEROKEE SMALL LETTER A */
  case 0xAB71: out[0] = 0x13A1; return 1; /* CHEROKEE SMALL LETTER E */
  case 0xAB72: out[0] = 0x13A2; return 1; /* CHEROKEE SMALL LETTER I */
  case 0xAB73: out[0] = 0x13A3; return 1; /* CHEROKEE SMALL LETTER O */
  case 0xAB74: out[0] = 0x13A4; return 1; /* CHEROKEE SMALL LETTER U */
  case 0xAB75: out[0] = 0x13A5; return 1; /* CHEROKEE SMALL LETTER V */
  case 0xAB76: out[0] = 0x13A6; return 1; /* CHEROKEE SMALL LETTER GA */
  case 0xAB77: out[0] = 0x13A7; return 1; /* CHEROKEE SMALL LETTER KA */
  case 0xAB78: out[0] = 0x13A8; return 1; /* CHEROKEE SMALL LETTER GE */
  case 0xAB79: out[0] = 0x13A9; return 1; /* CHEROKEE SMALL LETTER GI */
  case 0xAB7A: out[0] = 0x13AA; return 1; /* CHEROKEE SMALL LETTER GO */
  case 0xAB7B: out[0] = 0x13AB; return 1; /* CHEROKEE SMALL LETTER GU */
  case 0xAB7C: out[0] = 0x13AC; return 1; /* CHEROKEE SMALL LETTER GV */
  case 0xAB7D: out[0] = 0x13AD; return 1; /* CHEROKEE SMALL LETTER HA */
  case 0xAB7E: out[0] = 0x13AE; return 1; /* CHEROKEE SMALL LETTER HE */
  case 0xAB7F: out[0] = 0x13AF; return 1; /* CHEROKEE SMALL LETTER HI */
  case 0xAB80: out[0] = 0x13B0; return 1; /* CHEROKEE SMALL LETTER HO */
  case 0xAB81: out[0] = 0x13B1; return 1; /* CHEROKEE SMALL LETTER HU */
  case 0xAB82: out[0] = 0x13B2; return 1; /* CHEROKEE SMALL LETTER HV */
  case 0xAB83: out[0] = 0x13B3; return 1; /* CHEROKEE SMALL LETTER LA */
  case 0xAB84: out[0] = 0x13B4; return 1; /* CHEROKEE SMALL LETTER LE */
  case 0xAB85: out[0] = 0x13B5; return 1; /* CHEROKEE SMALL LETTER LI */
  case 0xAB86: out[0] = 0x13B6; return 1; /* CHEROKEE SMALL LETTER LO */
  case 0xAB87: out[0] = 0x13B7; return 1; /* CHEROKEE SMALL LETTER LU */
  case 0xAB88: out[0] = 0x13B8; return 1; /* CHEROKEE SMALL LETTER LV */
  case 0xAB89: out[0] = 0x13B9; return 1; /* CHEROKEE SMALL LETTER MA */
  case 0xAB8A: out[0] = 0x13BA; return 1; /* CHEROKEE SMALL LETTER ME */
  case 0xAB8B: out[0] = 0x13BB; return 1; /* CHEROKEE SMALL LETTER MI */
  case 0xAB8C: out[0] = 0x13BC; return 1; /* CHEROKEE SMALL LETTER MO */
  case 0xAB8D: out[0] = 0x13BD; return 1; /* CHEROKEE SMALL LETTER MU */
  case 0xAB8E: out[0] = 0x13BE; return 1; /* CHEROKEE SMALL LETTER NA */
  case 0xAB8F: out[0] = 0x13BF; return 1; /* CHEROKEE SMALL LETTER HNA */
  case 0xAB90: out[0] = 0x13C0; return 1; /* CHEROKEE SMALL LETTER NAH */
  case 0xAB91: out[0] = 0x13C1; return 1; /* CHEROKEE SMALL LETTER NE */
  case 0xAB92: out[0] = 0x13C2; return 1; /* CHEROKEE SMALL LETTER NI */
  case 0xAB93: out[0] = 0x13C3; return 1; /* CHEROKEE SMALL LETTER NO */
  case 0xAB94: out[0] = 0x13C4; return 1; /* CHEROKEE SMALL LETTER NU */
  case 0xAB95: out[0] = 0x13C5; return 1; /* CHEROKEE SMALL LETTER NV */
  case 0xAB96: out[0] = 0x13C6; return 1; /* CHEROKEE SMALL LETTER QUA */
  case 0xAB97: out[0] = 0x13C7; return 1; /* CHEROKEE SMALL LETTER QUE */
  case 0xAB98: out[0] = 0x13C8; return 1; /* CHEROKEE SMALL LETTER QUI */
  case 0xAB99: out[0] = 0x13C9; return 1; /* CHEROKEE SMALL LETTER QUO */
  case 0xAB9A: out[0] = 0x13CA; return 1; /* CHEROKEE SMALL LETTER QUU */
  case 0xAB9B: out[0] = 0x13CB; return 1; /* CHEROKEE SMALL LETTER QUV */
  case 0xAB9C: out[0] = 0x13CC; return 1; /* CHEROKEE SMALL LETTER SA */
  case 0xAB9D: out[0] = 0x13CD; return 1; /* CHEROKEE SMALL LETTER S */
  case 0xAB9E: out[0] = 0x13CE; return 1; /* CHEROKEE SMALL LETTER SE */
  case 0xAB9F: out[0] = 0x13CF; return 1; /* CHEROKEE SMALL LETTER SI */
  case 0xABA0: out[0] = 0x13D0; return 1; /* CHEROKEE SMALL LETTER SO */
  case 0xABA1: out[0] = 0x13D1; return 1; /* CHEROKEE SMALL LETTER SU */
  case 0xABA2: out[0] = 0x13D2; return 1; /* CHEROKEE SMALL LETTER SV */
  case 0xABA3: out[0] = 0x13D3; return 1; /* CHEROKEE SMALL LETTER DA */
  case 0xABA4: out[0] = 0x13D4; return 1; /* CHEROKEE SMALL LETTER TA */
  case 0xABA5: out[0] = 0x13D5; return 1; /* CHEROKEE SMALL LETTER DE */
  case 0xABA6: out[0] = 0x13D6; return 1; /* CHEROKEE SMALL LETTER TE */
  case 0xABA7: out[0] = 0x13D7; return 1; /* CHEROKEE SMALL LETTER DI */
  case 0xABA8: out[0] = 0x13D8; return 1; /* CHEROKEE SMALL LETTER TI */
  case 0xABA9: out[0] = 0x13D9; return 1; /* CHEROKEE SMALL LETTER DO */
  case 0xABAA: out[0] = 0x13DA; return 1; /* CHEROKEE SMALL LETTER DU */
  case 0xABAB: out[0] = 0x13DB; return 1; /* CHEROKEE SMALL LETTER DV */
  case 0xABAC: out[0] = 0x13DC; return 1; /* CHEROKEE SMALL LETTER DLA */
  case 0xABAD: out[0] = 0x13DD; return 1; /* CHEROKEE SMALL LETTER TLA */
  case 0xABAE: out[0] = 0x13DE; return 1; /* CHEROKEE SMALL LETTER TLE */
  case 0xABAF: out[0] = 0x13DF; return 1; /* CHEROKEE SMALL LETTER TLI */
  case 0xABB0: out[0] = 0x13E0; return 1; /* CHEROKEE SMALL LETTER TLO */
  case 0xABB1: out[0] = 0x13E1; return 1; /* CHEROKEE SMALL LETTER TLU */
  case 0xABB2: out[0] = 0x13E2; return 1; /* CHEROKEE SMALL LETTER TLV */
  case 0xABB3: out[0] = 0x13E3; return 1; /* CHEROKEE SMALL LETTER TSA */
  case 0xABB4: out[0] = 0x13E4; return 1; /* CHEROKEE SMALL LETTER TSE */
  case 0xABB5: out[0] = 0x13E5; return 1; /* CHEROKEE SMALL LETTER TSI */
  case 0xABB6: out[0] = 0x13E6; return 1; /* CHEROKEE SMALL LETTER TSO */
  case 0xABB7: out[0] = 0x13E7; return 1; /* CHEROKEE SMALL LETTER TSU */
  case 0xABB8: out[0] = 0x13E8; return 1; /* CHEROKEE SMALL LETTER TSV */
  case 0xABB9: out[0] = 0x13E9; return 1; /* CHEROKEE SMALL LETTER WA */
  case 0xABBA: out[0] = 0x13EA; return 1; /* CHEROKEE SMALL LETTER WE */
  case 0xABBB: out[0] = 0x13EB; return 1; /* CHEROKEE SMALL LETTER WI */
  case 0xABBC: out[0] = 0x13EC; return 1; /* CHEROKEE SMALL LETTER WO */
  case 0xABBD: out[0] = 0x13ED; return 1; /* CHEROKEE SMALL LETTER WU */
  case 0xABBE: out[0] = 0x13EE; return 1; /* CHEROKEE SMALL LETTER WV */
  case 0xABBF: out[0] = 0x13EF; return 1; /* CHEROKEE SMALL LETTER YA */
  case 0xFB00: out[0] = 0x0066; out[1] = 0x0066; return 2; /* LATIN SMALL LIGATURE FF */
  case 0xFB01: out[0] = 0x0066; out[1] = 0x0069; return 2; /* LATIN SMALL LIGATURE FI */
  case 0xFB02: out[0] = 0x0066; out[1] = 0x006C; return 2; /* LATIN SMALL LIGATURE FL */
  case 0xFB03: out[0] = 0x0066; out[1] = 0x0066; out[2] = 0x0069; return 3; /* LATIN SMALL LIGATURE FFI */
  case 0xFB04: out[0] = 0x0066; out[1] = 0x0066; out[2] = 0x006C; return 3;/* LATIN SMALL LIGATURE FFL */
  case 0xFB05: out[0] = 0x0073; out[1] = 0x0074; return 2; /* LATIN SMALL LIGATURE LONG S T */
  case 0xFB06: out[0] = 0x0073; out[1] = 0x0074; return 2; /* LATIN SMALL LIGATURE ST */
  case 0xFB13: out[0] = 0x0574; out[1] = 0x0576; return 2; /* ARMENIAN SMALL LIGATURE MEN NOW */
  case 0xFB14: out[0] = 0x0574; out[1] = 0x0565; return 2; /* ARMENIAN SMALL LIGATURE MEN ECH */
  case 0xFB15: out[0] = 0x0574; out[1] = 0x056B; return 2; /* ARMENIAN SMALL LIGATURE MEN INI */
  case 0xFB16: out[0] = 0x057E; out[1] = 0x0576; return 2; /* ARMENIAN SMALL LIGATURE VEW NOW */
  case 0xFB17: out[0] = 0x0574; out[1] = 0x056D; return 2; /* ARMENIAN SMALL LIGATURE MEN XEH */
  case 0xFF21: out[0] = 0xFF41; return 1; /* FULLWIDTH LATIN CAPITAL LETTER A */
  case 0xFF22: out[0] = 0xFF42; return 1; /* FULLWIDTH LATIN CAPITAL LETTER B */
  case 0xFF23: out[0] = 0xFF43; return 1; /* FULLWIDTH LATIN CAPITAL LETTER C */
  case 0xFF24: out[0] = 0xFF44; return 1; /* FULLWIDTH LATIN CAPITAL LETTER D */
  case 0xFF25: out[0] = 0xFF45; return 1; /* FULLWIDTH LATIN CAPITAL LETTER E */
  case 0xFF26: out[0] = 0xFF46; return 1; /* FULLWIDTH LATIN CAPITAL LETTER F */
  case 0xFF27: out[0] = 0xFF47; return 1; /* FULLWIDTH LATIN CAPITAL LETTER G */
  case 0xFF28: out[0] = 0xFF48; return 1; /* FULLWIDTH LATIN CAPITAL LETTER H */
  case 0xFF29: out[0] = 0xFF49; return 1; /* FULLWIDTH LATIN CAPITAL LETTER I */
  case 0xFF2A: out[0] = 0xFF4A; return 1; /* FULLWIDTH LATIN CAPITAL LETTER J */
  case 0xFF2B: out[0] = 0xFF4B; return 1; /* FULLWIDTH LATIN CAPITAL LETTER K */
  case 0xFF2C: out[0] = 0xFF4C; return 1; /* FULLWIDTH LATIN CAPITAL LETTER L */
  case 0xFF2D: out[0] = 0xFF4D; return 1; /* FULLWIDTH LATIN CAPITAL LETTER M */
  case 0xFF2E: out[0] = 0xFF4E; return 1; /* FULLWIDTH LATIN CAPITAL LETTER N */
  case 0xFF2F: out[0] = 0xFF4F; return 1; /* FULLWIDTH LATIN CAPITAL LETTER O */
  case 0xFF30: out[0] = 0xFF50; return 1; /* FULLWIDTH LATIN CAPITAL LETTER P */
  case 0xFF31: out[0] = 0xFF51; return 1; /* FULLWIDTH LATIN CAPITAL LETTER Q */
  case 0xFF32: out[0] = 0xFF52; return 1; /* FULLWIDTH LATIN CAPITAL LETTER R */
  case 0xFF33: out[0] = 0xFF53; return 1; /* FULLWIDTH LATIN CAPITAL LETTER S */
  case 0xFF34: out[0] = 0xFF54; return 1; /* FULLWIDTH LATIN CAPITAL LETTER T */
  case 0xFF35: out[0] = 0xFF55; return 1; /* FULLWIDTH LATIN CAPITAL LETTER U */
  case 0xFF36: out[0] = 0xFF56; return 1; /* FULLWIDTH LATIN CAPITAL LETTER V */
  case 0xFF37: out[0] = 0xFF57; return 1; /* FULLWIDTH LATIN CAPITAL LETTER W */
  case 0xFF38: out[0] = 0xFF58; return 1; /* FULLWIDTH LATIN CAPITAL LETTER X */
  case 0xFF39: out[0] = 0xFF59; return 1; /* FULLWIDTH LATIN CAPITAL LETTER Y */
  case 0xFF3A: out[0] = 0xFF5A; return 1; /* FULLWIDTH LATIN CAPITAL LETTER Z */
  case 0x10400: out[0] = 0x10428; return 1; /* DESERET CAPITAL LETTER LONG I */
  case 0x10401: out[0] = 0x10429; return 1; /* DESERET CAPITAL LETTER LONG E */
  case 0x10402: out[0] = 0x1042A; return 1; /* DESERET CAPITAL LETTER LONG A */
  case 0x10403: out[0] = 0x1042B; return 1; /* DESERET CAPITAL LETTER LONG AH */
  case 0x10404: out[0] = 0x1042C; return 1; /* DESERET CAPITAL LETTER LONG O */
  case 0x10405: out[0] = 0x1042D; return 1; /* DESERET CAPITAL LETTER LONG OO */
  case 0x10406: out[0] = 0x1042E; return 1; /* DESERET CAPITAL LETTER SHORT I */
  case 0x10407: out[0] = 0x1042F; return 1; /* DESERET CAPITAL LETTER SHORT E */
  case 0x10408: out[0] = 0x10430; return 1; /* DESERET CAPITAL LETTER SHORT A */
  case 0x10409: out[0] = 0x10431; return 1; /* DESERET CAPITAL LETTER SHORT AH */
  case 0x1040A: out[0] = 0x10432; return 1; /* DESERET CAPITAL LETTER SHORT O */
  case 0x1040B: out[0] = 0x10433; return 1; /* DESERET CAPITAL LETTER SHORT OO */
  case 0x1040C: out[0] = 0x10434; return 1; /* DESERET CAPITAL LETTER AY */
  case 0x1040D: out[0] = 0x10435; return 1; /* DESERET CAPITAL LETTER OW */
  case 0x1040E: out[0] = 0x10436; return 1; /* DESERET CAPITAL LETTER WU */
  case 0x1040F: out[0] = 0x10437; return 1; /* DESERET CAPITAL LETTER YEE */
  case 0x10410: out[0] = 0x10438; return 1; /* DESERET CAPITAL LETTER H */
  case 0x10411: out[0] = 0x10439; return 1; /* DESERET CAPITAL LETTER PEE */
  case 0x10412: out[0] = 0x1043A; return 1; /* DESERET CAPITAL LETTER BEE */
  case 0x10413: out[0] = 0x1043B; return 1; /* DESERET CAPITAL LETTER TEE */
  case 0x10414: out[0] = 0x1043C; return 1; /* DESERET CAPITAL LETTER DEE */
  case 0x10415: out[0] = 0x1043D; return 1; /* DESERET CAPITAL LETTER CHEE */
  case 0x10416: out[0] = 0x1043E; return 1; /* DESERET CAPITAL LETTER JEE */
  case 0x10417: out[0] = 0x1043F; return 1; /* DESERET CAPITAL LETTER KAY */
  case 0x10418: out[0] = 0x10440; return 1; /* DESERET CAPITAL LETTER GAY */
  case 0x10419: out[0] = 0x10441; return 1; /* DESERET CAPITAL LETTER EF */
  case 0x1041A: out[0] = 0x10442; return 1; /* DESERET CAPITAL LETTER VEE */
  case 0x1041B: out[0] = 0x10443; return 1; /* DESERET CAPITAL LETTER ETH */
  case 0x1041C: out[0] = 0x10444; return 1; /* DESERET CAPITAL LETTER THEE */
  case 0x1041D: out[0] = 0x10445; return 1; /* DESERET CAPITAL LETTER ES */
  case 0x1041E: out[0] = 0x10446; return 1; /* DESERET CAPITAL LETTER ZEE */
  case 0x1041F: out[0] = 0x10447; return 1; /* DESERET CAPITAL LETTER ESH */
  case 0x10420: out[0] = 0x10448; return 1; /* DESERET CAPITAL LETTER ZHEE */
  case 0x10421: out[0] = 0x10449; return 1; /* DESERET CAPITAL LETTER ER */
  case 0x10422: out[0] = 0x1044A; return 1; /* DESERET CAPITAL LETTER EL */
  case 0x10423: out[0] = 0x1044B; return 1; /* DESERET CAPITAL LETTER EM */
  case 0x10424: out[0] = 0x1044C; return 1; /* DESERET CAPITAL LETTER EN */
  case 0x10425: out[0] = 0x1044D; return 1; /* DESERET CAPITAL LETTER ENG */
  case 0x10426: out[0] = 0x1044E; return 1; /* DESERET CAPITAL LETTER OI */
  case 0x10427: out[0] = 0x1044F; return 1; /* DESERET CAPITAL LETTER EW */
  case 0x104B0: out[0] = 0x104D8; return 1; /* OSAGE CAPITAL LETTER A */
  case 0x104B1: out[0] = 0x104D9; return 1; /* OSAGE CAPITAL LETTER AI */
  case 0x104B2: out[0] = 0x104DA; return 1; /* OSAGE CAPITAL LETTER AIN */
  case 0x104B3: out[0] = 0x104DB; return 1; /* OSAGE CAPITAL LETTER AH */
  case 0x104B4: out[0] = 0x104DC; return 1; /* OSAGE CAPITAL LETTER BRA */
  case 0x104B5: out[0] = 0x104DD; return 1; /* OSAGE CAPITAL LETTER CHA */
  case 0x104B6: out[0] = 0x104DE; return 1; /* OSAGE CAPITAL LETTER EHCHA */
  case 0x104B7: out[0] = 0x104DF; return 1; /* OSAGE CAPITAL LETTER E */
  case 0x104B8: out[0] = 0x104E0; return 1; /* OSAGE CAPITAL LETTER EIN */
  case 0x104B9: out[0] = 0x104E1; return 1; /* OSAGE CAPITAL LETTER HA */
  case 0x104BA: out[0] = 0x104E2; return 1; /* OSAGE CAPITAL LETTER HYA */
  case 0x104BB: out[0] = 0x104E3; return 1; /* OSAGE CAPITAL LETTER I */
  case 0x104BC: out[0] = 0x104E4; return 1; /* OSAGE CAPITAL LETTER KA */
  case 0x104BD: out[0] = 0x104E5; return 1; /* OSAGE CAPITAL LETTER EHKA */
  case 0x104BE: out[0] = 0x104E6; return 1; /* OSAGE CAPITAL LETTER KYA */
  case 0x104BF: out[0] = 0x104E7; return 1; /* OSAGE CAPITAL LETTER LA */
  case 0x104C0: out[0] = 0x104E8; return 1; /* OSAGE CAPITAL LETTER MA */
  case 0x104C1: out[0] = 0x104E9; return 1; /* OSAGE CAPITAL LETTER NA */
  case 0x104C2: out[0] = 0x104EA; return 1; /* OSAGE CAPITAL LETTER O */
  case 0x104C3: out[0] = 0x104EB; return 1; /* OSAGE CAPITAL LETTER OIN */
  case 0x104C4: out[0] = 0x104EC; return 1; /* OSAGE CAPITAL LETTER PA */
  case 0x104C5: out[0] = 0x104ED; return 1; /* OSAGE CAPITAL LETTER EHPA */
  case 0x104C6: out[0] = 0x104EE; return 1; /* OSAGE CAPITAL LETTER SA */
  case 0x104C7: out[0] = 0x104EF; return 1; /* OSAGE CAPITAL LETTER SHA */
  case 0x104C8: out[0] = 0x104F0; return 1; /* OSAGE CAPITAL LETTER TA */
  case 0x104C9: out[0] = 0x104F1; return 1; /* OSAGE CAPITAL LETTER EHTA */
  case 0x104CA: out[0] = 0x104F2; return 1; /* OSAGE CAPITAL LETTER TSA */
  case 0x104CB: out[0] = 0x104F3; return 1; /* OSAGE CAPITAL LETTER EHTSA */
  case 0x104CC: out[0] = 0x104F4; return 1; /* OSAGE CAPITAL LETTER TSHA */
  case 0x104CD: out[0] = 0x104F5; return 1; /* OSAGE CAPITAL LETTER DHA */
  case 0x104CE: out[0] = 0x104F6; return 1; /* OSAGE CAPITAL LETTER U */
  case 0x104CF: out[0] = 0x104F7; return 1; /* OSAGE CAPITAL LETTER WA */
  case 0x104D0: out[0] = 0x104F8; return 1; /* OSAGE CAPITAL LETTER KHA */
  case 0x104D1: out[0] = 0x104F9; return 1; /* OSAGE CAPITAL LETTER GHA */
  case 0x104D2: out[0] = 0x104FA; return 1; /* OSAGE CAPITAL LETTER ZA */
  case 0x104D3: out[0] = 0x104FB; return 1; /* OSAGE CAPITAL LETTER ZHA */
  case 0x10570: out[0] = 0x10597; return 1; /* VITHKUQI CAPITAL LETTER A */
  case 0x10571: out[0] = 0x10598; return 1; /* VITHKUQI CAPITAL LETTER BBE */
  case 0x10572: out[0] = 0x10599; return 1; /* VITHKUQI CAPITAL LETTER BE */
  case 0x10573: out[0] = 0x1059A; return 1; /* VITHKUQI CAPITAL LETTER CE */
  case 0x10574: out[0] = 0x1059B; return 1; /* VITHKUQI CAPITAL LETTER CHE */
  case 0x10575: out[0] = 0x1059C; return 1; /* VITHKUQI CAPITAL LETTER DE */
  case 0x10576: out[0] = 0x1059D; return 1; /* VITHKUQI CAPITAL LETTER DHE */
  case 0x10577: out[0] = 0x1059E; return 1; /* VITHKUQI CAPITAL LETTER EI */
  case 0x10578: out[0] = 0x1059F; return 1; /* VITHKUQI CAPITAL LETTER E */
  case 0x10579: out[0] = 0x105A0; return 1; /* VITHKUQI CAPITAL LETTER FE */
  case 0x1057A: out[0] = 0x105A1; return 1; /* VITHKUQI CAPITAL LETTER GA */
  case 0x1057C: out[0] = 0x105A3; return 1; /* VITHKUQI CAPITAL LETTER HA */
  case 0x1057D: out[0] = 0x105A4; return 1; /* VITHKUQI CAPITAL LETTER HHA */
  case 0x1057E: out[0] = 0x105A5; return 1; /* VITHKUQI CAPITAL LETTER I */
  case 0x1057F: out[0] = 0x105A6; return 1; /* VITHKUQI CAPITAL LETTER IJE */
  case 0x10580: out[0] = 0x105A7; return 1; /* VITHKUQI CAPITAL LETTER JE */
  case 0x10581: out[0] = 0x105A8; return 1; /* VITHKUQI CAPITAL LETTER KA */
  case 0x10582: out[0] = 0x105A9; return 1; /* VITHKUQI CAPITAL LETTER LA */
  case 0x10583: out[0] = 0x105AA; return 1; /* VITHKUQI CAPITAL LETTER LLA */
  case 0x10584: out[0] = 0x105AB; return 1; /* VITHKUQI CAPITAL LETTER ME */
  case 0x10585: out[0] = 0x105AC; return 1; /* VITHKUQI CAPITAL LETTER NE */
  case 0x10586: out[0] = 0x105AD; return 1; /* VITHKUQI CAPITAL LETTER NJE */
  case 0x10587: out[0] = 0x105AE; return 1; /* VITHKUQI CAPITAL LETTER O */
  case 0x10588: out[0] = 0x105AF; return 1; /* VITHKUQI CAPITAL LETTER PE */
  case 0x10589: out[0] = 0x105B0; return 1; /* VITHKUQI CAPITAL LETTER QA */
  case 0x1058A: out[0] = 0x105B1; return 1; /* VITHKUQI CAPITAL LETTER RE */
  case 0x1058C: out[0] = 0x105B3; return 1; /* VITHKUQI CAPITAL LETTER SE */
  case 0x1058D: out[0] = 0x105B4; return 1; /* VITHKUQI CAPITAL LETTER SHE */
  case 0x1058E: out[0] = 0x105B5; return 1; /* VITHKUQI CAPITAL LETTER TE */
  case 0x1058F: out[0] = 0x105B6; return 1; /* VITHKUQI CAPITAL LETTER THE */
  case 0x10590: out[0] = 0x105B7; return 1; /* VITHKUQI CAPITAL LETTER U */
  case 0x10591: out[0] = 0x105B8; return 1; /* VITHKUQI CAPITAL LETTER VE */
  case 0x10592: out[0] = 0x105B9; return 1; /* VITHKUQI CAPITAL LETTER XE */
  case 0x10594: out[0] = 0x105BB; return 1; /* VITHKUQI CAPITAL LETTER Y */
  case 0x10595: out[0] = 0x105BC; return 1; /* VITHKUQI CAPITAL LETTER ZE */
  case 0x10C80: out[0] = 0x10CC0; return 1; /* OLD HUNGARIAN CAPITAL LETTER A */
  case 0x10C81: out[0] = 0x10CC1; return 1; /* OLD HUNGARIAN CAPITAL LETTER AA */
  case 0x10C82: out[0] = 0x10CC2; return 1; /* OLD HUNGARIAN CAPITAL LETTER EB */
  case 0x10C83: out[0] = 0x10CC3; return 1; /* OLD HUNGARIAN CAPITAL LETTER AMB */
  case 0x10C84: out[0] = 0x10CC4; return 1; /* OLD HUNGARIAN CAPITAL LETTER EC */
  case 0x10C85: out[0] = 0x10CC5; return 1; /* OLD HUNGARIAN CAPITAL LETTER ENC */
  case 0x10C86: out[0] = 0x10CC6; return 1; /* OLD HUNGARIAN CAPITAL LETTER ECS */
  case 0x10C87: out[0] = 0x10CC7; return 1; /* OLD HUNGARIAN CAPITAL LETTER ED */
  case 0x10C88: out[0] = 0x10CC8; return 1; /* OLD HUNGARIAN CAPITAL LETTER AND */
  case 0x10C89: out[0] = 0x10CC9; return 1; /* OLD HUNGARIAN CAPITAL LETTER E */
  case 0x10C8A: out[0] = 0x10CCA; return 1; /* OLD HUNGARIAN CAPITAL LETTER CLOSE E */
  case 0x10C8B: out[0] = 0x10CCB; return 1; /* OLD HUNGARIAN CAPITAL LETTER EE */
  case 0x10C8C: out[0] = 0x10CCC; return 1; /* OLD HUNGARIAN CAPITAL LETTER EF */
  case 0x10C8D: out[0] = 0x10CCD; return 1; /* OLD HUNGARIAN CAPITAL LETTER EG */
  case 0x10C8E: out[0] = 0x10CCE; return 1; /* OLD HUNGARIAN CAPITAL LETTER EGY */
  case 0x10C8F: out[0] = 0x10CCF; return 1; /* OLD HUNGARIAN CAPITAL LETTER EH */
  case 0x10C90: out[0] = 0x10CD0; return 1; /* OLD HUNGARIAN CAPITAL LETTER I */
  case 0x10C91: out[0] = 0x10CD1; return 1; /* OLD HUNGARIAN CAPITAL LETTER II */
  case 0x10C92: out[0] = 0x10CD2; return 1; /* OLD HUNGARIAN CAPITAL LETTER EJ */
  case 0x10C93: out[0] = 0x10CD3; return 1; /* OLD HUNGARIAN CAPITAL LETTER EK */
  case 0x10C94: out[0] = 0x10CD4; return 1; /* OLD HUNGARIAN CAPITAL LETTER AK */
  case 0x10C95: out[0] = 0x10CD5; return 1; /* OLD HUNGARIAN CAPITAL LETTER UNK */
  case 0x10C96: out[0] = 0x10CD6; return 1; /* OLD HUNGARIAN CAPITAL LETTER EL */
  case 0x10C97: out[0] = 0x10CD7; return 1; /* OLD HUNGARIAN CAPITAL LETTER ELY */
  case 0x10C98: out[0] = 0x10CD8; return 1; /* OLD HUNGARIAN CAPITAL LETTER EM */
  case 0x10C99: out[0] = 0x10CD9; return 1; /* OLD HUNGARIAN CAPITAL LETTER EN */
  case 0x10C9A: out[0] = 0x10CDA; return 1; /* OLD HUNGARIAN CAPITAL LETTER ENY */
  case 0x10C9B: out[0] = 0x10CDB; return 1; /* OLD HUNGARIAN CAPITAL LETTER O */
  case 0x10C9C: out[0] = 0x10CDC; return 1; /* OLD HUNGARIAN CAPITAL LETTER OO */
  case 0x10C9D: out[0] = 0x10CDD; return 1; /* OLD HUNGARIAN CAPITAL LETTER NIKOLSBURG OE */
  case 0x10C9E: out[0] = 0x10CDE; return 1; /* OLD HUNGARIAN CAPITAL LETTER RUDIMENTA OE */
  case 0x10C9F: out[0] = 0x10CDF; return 1; /* OLD HUNGARIAN CAPITAL LETTER OEE */
  case 0x10CA0: out[0] = 0x10CE0; return 1; /* OLD HUNGARIAN CAPITAL LETTER EP */
  case 0x10CA1: out[0] = 0x10CE1; return 1; /* OLD HUNGARIAN CAPITAL LETTER EMP */
  case 0x10CA2: out[0] = 0x10CE2; return 1; /* OLD HUNGARIAN CAPITAL LETTER ER */
  case 0x10CA3: out[0] = 0x10CE3; return 1; /* OLD HUNGARIAN CAPITAL LETTER SHORT ER */
  case 0x10CA4: out[0] = 0x10CE4; return 1; /* OLD HUNGARIAN CAPITAL LETTER ES */
  case 0x10CA5: out[0] = 0x10CE5; return 1; /* OLD HUNGARIAN CAPITAL LETTER ESZ */
  case 0x10CA6: out[0] = 0x10CE6; return 1; /* OLD HUNGARIAN CAPITAL LETTER ET */
  case 0x10CA7: out[0] = 0x10CE7; return 1; /* OLD HUNGARIAN CAPITAL LETTER ENT */
  case 0x10CA8: out[0] = 0x10CE8; return 1; /* OLD HUNGARIAN CAPITAL LETTER ETY */
  case 0x10CA9: out[0] = 0x10CE9; return 1; /* OLD HUNGARIAN CAPITAL LETTER ECH */
  case 0x10CAA: out[0] = 0x10CEA; return 1; /* OLD HUNGARIAN CAPITAL LETTER U */
  case 0x10CAB: out[0] = 0x10CEB; return 1; /* OLD HUNGARIAN CAPITAL LETTER UU */
  case 0x10CAC: out[0] = 0x10CEC; return 1; /* OLD HUNGARIAN CAPITAL LETTER NIKOLSBURG UE */
  case 0x10CAD: out[0] = 0x10CED; return 1; /* OLD HUNGARIAN CAPITAL LETTER RUDIMENTA UE */
  case 0x10CAE: out[0] = 0x10CEE; return 1; /* OLD HUNGARIAN CAPITAL LETTER EV */
  case 0x10CAF: out[0] = 0x10CEF; return 1; /* OLD HUNGARIAN CAPITAL LETTER EZ */
  case 0x10CB0: out[0] = 0x10CF0; return 1; /* OLD HUNGARIAN CAPITAL LETTER EZS */
  case 0x10CB1: out[0] = 0x10CF1; return 1; /* OLD HUNGARIAN CAPITAL LETTER ENT-SHAPED SIGN */
  case 0x10CB2: out[0] = 0x10CF2; return 1; /* OLD HUNGARIAN CAPITAL LETTER US */
  case 0x118A0: out[0] = 0x118C0; return 1; /* WARANG CITI CAPITAL LETTER NGAA */
  case 0x118A1: out[0] = 0x118C1; return 1; /* WARANG CITI CAPITAL LETTER A */
  case 0x118A2: out[0] = 0x118C2; return 1; /* WARANG CITI CAPITAL LETTER WI */
  case 0x118A3: out[0] = 0x118C3; return 1; /* WARANG CITI CAPITAL LETTER YU */
  case 0x118A4: out[0] = 0x118C4; return 1; /* WARANG CITI CAPITAL LETTER YA */
  case 0x118A5: out[0] = 0x118C5; return 1; /* WARANG CITI CAPITAL LETTER YO */
  case 0x118A6: out[0] = 0x118C6; return 1; /* WARANG CITI CAPITAL LETTER II */
  case 0x118A7: out[0] = 0x118C7; return 1; /* WARANG CITI CAPITAL LETTER UU */
  case 0x118A8: out[0] = 0x118C8; return 1; /* WARANG CITI CAPITAL LETTER E */
  case 0x118A9: out[0] = 0x118C9; return 1; /* WARANG CITI CAPITAL LETTER O */
  case 0x118AA: out[0] = 0x118CA; return 1; /* WARANG CITI CAPITAL LETTER ANG */
  case 0x118AB: out[0] = 0x118CB; return 1; /* WARANG CITI CAPITAL LETTER GA */
  case 0x118AC: out[0] = 0x118CC; return 1; /* WARANG CITI CAPITAL LETTER KO */
  case 0x118AD: out[0] = 0x118CD; return 1; /* WARANG CITI CAPITAL LETTER ENY */
  case 0x118AE: out[0] = 0x118CE; return 1; /* WARANG CITI CAPITAL LETTER YUJ */
  case 0x118AF: out[0] = 0x118CF; return 1; /* WARANG CITI CAPITAL LETTER UC */
  case 0x118B0: out[0] = 0x118D0; return 1; /* WARANG CITI CAPITAL LETTER ENN */
  case 0x118B1: out[0] = 0x118D1; return 1; /* WARANG CITI CAPITAL LETTER ODD */
  case 0x118B2: out[0] = 0x118D2; return 1; /* WARANG CITI CAPITAL LETTER TTE */
  case 0x118B3: out[0] = 0x118D3; return 1; /* WARANG CITI CAPITAL LETTER NUNG */
  case 0x118B4: out[0] = 0x118D4; return 1; /* WARANG CITI CAPITAL LETTER DA */
  case 0x118B5: out[0] = 0x118D5; return 1; /* WARANG CITI CAPITAL LETTER AT */
  case 0x118B6: out[0] = 0x118D6; return 1; /* WARANG CITI CAPITAL LETTER AM */
  case 0x118B7: out[0] = 0x118D7; return 1; /* WARANG CITI CAPITAL LETTER BU */
  case 0x118B8: out[0] = 0x118D8; return 1; /* WARANG CITI CAPITAL LETTER PU */
  case 0x118B9: out[0] = 0x118D9; return 1; /* WARANG CITI CAPITAL LETTER HIYO */
  case 0x118BA: out[0] = 0x118DA; return 1; /* WARANG CITI CAPITAL LETTER HOLO */
  case 0x118BB: out[0] = 0x118DB; return 1; /* WARANG CITI CAPITAL LETTER HORR */
  case 0x118BC: out[0] = 0x118DC; return 1; /* WARANG CITI CAPITAL LETTER HAR */
  case 0x118BD: out[0] = 0x118DD; return 1; /* WARANG CITI CAPITAL LETTER SSUU */
  case 0x118BE: out[0] = 0x118DE; return 1; /* WARANG CITI CAPITAL LETTER SII */
  case 0x118BF: out[0] = 0x118DF; return 1; /* WARANG CITI CAPITAL LETTER VIYO */
  case 0x16E40: out[0] = 0x16E60; return 1; /* MEDEFAIDRIN CAPITAL LETTER M */
  case 0x16E41: out[0] = 0x16E61; return 1; /* MEDEFAIDRIN CAPITAL LETTER S */
  case 0x16E42: out[0] = 0x16E62; return 1; /* MEDEFAIDRIN CAPITAL LETTER V */
  case 0x16E43: out[0] = 0x16E63; return 1; /* MEDEFAIDRIN CAPITAL LETTER W */
  case 0x16E44: out[0] = 0x16E64; return 1; /* MEDEFAIDRIN CAPITAL LETTER ATIU */
  case 0x16E45: out[0] = 0x16E65; return 1; /* MEDEFAIDRIN CAPITAL LETTER Z */
  case 0x16E46: out[0] = 0x16E66; return 1; /* MEDEFAIDRIN CAPITAL LETTER KP */
  case 0x16E47: out[0] = 0x16E67; return 1; /* MEDEFAIDRIN CAPITAL LETTER P */
  case 0x16E48: out[0] = 0x16E68; return 1; /* MEDEFAIDRIN CAPITAL LETTER T */
  case 0x16E49: out[0] = 0x16E69; return 1; /* MEDEFAIDRIN CAPITAL LETTER G */
  case 0x16E4A: out[0] = 0x16E6A; return 1; /* MEDEFAIDRIN CAPITAL LETTER F */
  case 0x16E4B: out[0] = 0x16E6B; return 1; /* MEDEFAIDRIN CAPITAL LETTER I */
  case 0x16E4C: out[0] = 0x16E6C; return 1; /* MEDEFAIDRIN CAPITAL LETTER K */
  case 0x16E4D: out[0] = 0x16E6D; return 1; /* MEDEFAIDRIN CAPITAL LETTER A */
  case 0x16E4E: out[0] = 0x16E6E; return 1; /* MEDEFAIDRIN CAPITAL LETTER J */
  case 0x16E4F: out[0] = 0x16E6F; return 1; /* MEDEFAIDRIN CAPITAL LETTER E */
  case 0x16E50: out[0] = 0x16E70; return 1; /* MEDEFAIDRIN CAPITAL LETTER B */
  case 0x16E51: out[0] = 0x16E71; return 1; /* MEDEFAIDRIN CAPITAL LETTER C */
  case 0x16E52: out[0] = 0x16E72; return 1; /* MEDEFAIDRIN CAPITAL LETTER U */
  case 0x16E53: out[0] = 0x16E73; return 1; /* MEDEFAIDRIN CAPITAL LETTER YU */
  case 0x16E54: out[0] = 0x16E74; return 1; /* MEDEFAIDRIN CAPITAL LETTER L */
  case 0x16E55: out[0] = 0x16E75; return 1; /* MEDEFAIDRIN CAPITAL LETTER Q */
  case 0x16E56: out[0] = 0x16E76; return 1; /* MEDEFAIDRIN CAPITAL LETTER HP */
  case 0x16E57: out[0] = 0x16E77; return 1; /* MEDEFAIDRIN CAPITAL LETTER NY */
  case 0x16E58: out[0] = 0x16E78; return 1; /* MEDEFAIDRIN CAPITAL LETTER X */
  case 0x16E59: out[0] = 0x16E79; return 1; /* MEDEFAIDRIN CAPITAL LETTER D */
  case 0x16E5A: out[0] = 0x16E7A; return 1; /* MEDEFAIDRIN CAPITAL LETTER OE */
  case 0x16E5B: out[0] = 0x16E7B; return 1; /* MEDEFAIDRIN CAPITAL LETTER N */
  case 0x16E5C: out[0] = 0x16E7C; return 1; /* MEDEFAIDRIN CAPITAL LETTER R */
  case 0x16E5D: out[0] = 0x16E7D; return 1; /* MEDEFAIDRIN CAPITAL LETTER O */
  case 0x16E5E: out[0] = 0x16E7E; return 1; /* MEDEFAIDRIN CAPITAL LETTER AI */
  case 0x16E5F: out[0] = 0x16E7F; return 1; /* MEDEFAIDRIN CAPITAL LETTER Y */
  case 0x1E900: out[0] = 0x1E922; return 1; /* ADLAM CAPITAL LETTER ALIF */
  case 0x1E901: out[0] = 0x1E923; return 1; /* ADLAM CAPITAL LETTER DAALI */
  case 0x1E902: out[0] = 0x1E924; return 1; /* ADLAM CAPITAL LETTER LAAM */
  case 0x1E903: out[0] = 0x1E925; return 1; /* ADLAM CAPITAL LETTER MIIM */
  case 0x1E904: out[0] = 0x1E926; return 1; /* ADLAM CAPITAL LETTER BA */
  case 0x1E905: out[0] = 0x1E927; return 1; /* ADLAM CAPITAL LETTER SINNYIIYHE */
  case 0x1E906: out[0] = 0x1E928; return 1; /* ADLAM CAPITAL LETTER PE */
  case 0x1E907: out[0] = 0x1E929; return 1; /* ADLAM CAPITAL LETTER BHE */
  case 0x1E908: out[0] = 0x1E92A; return 1; /* ADLAM CAPITAL LETTER RA */
  case 0x1E909: out[0] = 0x1E92B; return 1; /* ADLAM CAPITAL LETTER E */
  case 0x1E90A: out[0] = 0x1E92C; return 1; /* ADLAM CAPITAL LETTER FA */
  case 0x1E90B: out[0] = 0x1E92D; return 1; /* ADLAM CAPITAL LETTER I */
  case 0x1E90C: out[0] = 0x1E92E; return 1; /* ADLAM CAPITAL LETTER O */
  case 0x1E90D: out[0] = 0x1E92F; return 1; /* ADLAM CAPITAL LETTER DHA */
  case 0x1E90E: out[0] = 0x1E930; return 1; /* ADLAM CAPITAL LETTER YHE */
  case 0x1E90F: out[0] = 0x1E931; return 1; /* ADLAM CAPITAL LETTER WAW */
  case 0x1E910: out[0] = 0x1E932; return 1; /* ADLAM CAPITAL LETTER NUN */
  case 0x1E911: out[0] = 0x1E933; return 1; /* ADLAM CAPITAL LETTER KAF */
  case 0x1E912: out[0] = 0x1E934; return 1; /* ADLAM CAPITAL LETTER YA */
  case 0x1E913: out[0] = 0x1E935; return 1; /* ADLAM CAPITAL LETTER U */
  case 0x1E914: out[0] = 0x1E936; return 1; /* ADLAM CAPITAL LETTER JIIM */
  case 0x1E915: out[0] = 0x1E937; return 1; /* ADLAM CAPITAL LETTER CHI */
  case 0x1E916: out[0] = 0x1E938; return 1; /* ADLAM CAPITAL LETTER HA */
  case 0x1E917: out[0] = 0x1E939; return 1; /* ADLAM CAPITAL LETTER QAAF */
  case 0x1E918: out[0] = 0x1E93A; return 1; /* ADLAM CAPITAL LETTER GA */
  case 0x1E919: out[0] = 0x1E93B; return 1; /* ADLAM CAPITAL LETTER NYA */
  case 0x1E91A: out[0] = 0x1E93C; return 1; /* ADLAM CAPITAL LETTER TU */
  case 0x1E91B: out[0] = 0x1E93D; return 1; /* ADLAM CAPITAL LETTER NHA */
  case 0x1E91C: out[0] = 0x1E93E; return 1; /* ADLAM CAPITAL LETTER VA */
  case 0x1E91D: out[0] = 0x1E93F; return 1; /* ADLAM CAPITAL LETTER KHA */
  case 0x1E91E: out[0] = 0x1E940; return 1; /* ADLAM CAPITAL LETTER GBE */
  case 0x1E91F: out[0] = 0x1E941; return 1; /* ADLAM CAPITAL LETTER ZAL */
  case 0x1E920: out[0] = 0x1E942; return 1; /* ADLAM CAPITAL LETTER KPO */
  case 0x1E921: out[0] = 0x1E943; return 1; /* ADLAM CAPITAL LETTER SHA */
  default: break;
  }
  out[0] = c;
  return 1;
}

/* ---------------------------------------------------------------------------
 *                                  String
 * ---------------------------------------------------------------------------
 */
// clang-format off
#define H1(s,i,x)   (x*65599u+(unsigned char)s[((i)<(cntof(s)-1))?cntof(s)-2-(i):(cntof(s)-1)])
#define H4(s,i,x)   H1(s,i,H1(s,i+1,H1(s,i+2,H1(s,i+3,x))))
#define H8(s,i,x)   H4(s,i,H4(s,i+4,x))
#define H16(s,i,x)  H4(s,i,H4(s,i+4,H4(s,i+8,H4(s,i+12,x))))
#define H32(s,i,x)  H16(s,i,H16(s,i+16,x))
#define H64(s,i,x)  H16(s,i,H16(s,i+16,H16(s,i+32,H16(s,i+48,x))))

#define STR_HASH4(s)    castu((H4(s,0,0)^(H4(s,0,0)>>16)))
#define STR_HASH8(s)    castu((H8(s,0,0)^(H8(s,0,0)>>16)))
#define STR_HASH16(s)   castu((H16(s,0,0)^(H16(s,0,0)>>16)))
#define STR_HASH32(s)   castu((H32(s,0,0)^(H32(s,0,0)>>16)))
#define STR_HASH64(s)   castu((H64(s,0,0)^(H64(s,0,0)>>16)))

#define match(s) switch(str__match_hash(s))
#define with4(s) case STR_HASH4(s)
#define with8(s) case STR_HASH8(s)
#define with16(s) case STR_HASH16(s)
#define with32(s) case STR_HASH32(s)
#define with64(s) case STR_HASH64(s)

#define cstrn(s) casti(strlen(s))
#define str(p,r) (struct str){.ptr = p, .rng = r}
#define strptr(p,b,e,n) (struct str){.ptr = p, .rng = rng(casti(b-p),casti(e-p), n)}
#define strp(b,e) strptr(b,b,e,casti(e-b))
#define strn(s,n) str(s,rngn(n))
#define str0(s) str(s,rngn(cstrn(s)))
#define strv(s) (struct str){.ptr = s, .rng = {0,cntof(s)-1,cntof(s)-1,cntof(s)-1}}
#define strf(s) s.rng.cnt, str_beg(s)
#define str_nil (struct str){0,rngn(0)}
#define str_inv (struct str){0,rng_inv}
#define str_len(s) rng_cnt(&(s).rng)
#define str_is_empty(s) (str_len(s) == 0)
#define str_is_inv(s) rng_is_inv(s.rng)
#define str_is_val(s) (!rng_is_inv(s.rng))
#define str_eq(a,b) (str_cmp(a,b) == 0)
#define str_neq(a,b) (str_cmp(a,b) != 0)
#define str_sub(s,b,e) str((s).ptr, rng_sub(&(s).rng, b, e))
#define str_rhs(s,n) str_sub(s, min(str_len(s), n), str_len(s))
#define str_lhs(s,n) str_sub(s, 0, min(str_len(s), n))
#define str_cut_lhs(s,n) *(s) = str_rhs(*(s), n)
#define str_cut_rhs(s,n) *(s) = str_lhs(*(s), n)
#define str_beg(s) slc_beg((s).ptr, (s).rng)
#define str_end(s) slc_end((s).ptr, (s).rng)
#define str_at(s,i) slc_at((s).ptr, (s).rng, i)
#define str_ptr(s,i) slc_ptr((s).ptr, (s).rng, i)

#define str_each(it,c) (const char *it = slc_beg((c).ptr, (c).rng); it < slc_end((c).ptr, (c).rng); it += 1)
#define str_eachr(it,s,r) (const char *it = (s).str + (s).rng.lo + (r).lo; (it) != (s).str + (s).rng.hi - (r).hi; (it) += 1)
#define str_loop(i,s) (int i = 0; i < str_len(s); ++i)
#define str_tok(it, rest, src, delim)                       \
  (struct str rest = src, it = str_split_cut(&rest, delim); \
   str_len(it); it = str_split_cut(&rest, delim))
// clang-format on

static inline unsigned
str__match_hash(struct str s) {
  unsigned int h = 0;
  for str_loop(i,s) {
    h = 65599u * h + castb(str_at(s,i));
  }
  return h ^ (h >> 16);
}
static unsigned long long
str__hash(struct str s, unsigned long long id) {
  assert(str_len(s) >= 0);
  return fnv1a64(str_beg(s), str_len(s), id);
}
static unsigned long long
str_hash(struct str s) {
  assert(str_len(s) >= 0);
  return str__hash(s, FNV1A64_HASH_INITIAL);
}
static int
str_cmp(struct str a, struct str b) {
  assert(a.ptr);
  assert(str_len(a) >= 0);
  assert(b.ptr);
  assert(str_len(b) >= 0);

  int n = min(str_len(a), str_len(b));
  for loop(i,n) {
    if (str_at(a,i) < str_at(b,i)) {
      return -1;
    } else if (str_at(a,i) > str_at(b,i)) {
      return +1;
    }
  }
  if (str_len(a) > str_len(b)) {
    return +1;
  } else if (str_len(a) < str_len(b)) {
    return -1;
  }
  return 0;
}
static int
str_fnd(struct str hay, struct str needle) {
  assert(str_len(hay) >= 0);
  assert(str_len(needle) >= 0);
  if (str_len(needle) == 1) {
    const char *ret = cpu_str_chr(str_beg(hay), str_len(hay), str_at(needle,0));
    return ret ? casti(ret - str_beg(hay)) : str_len(hay);
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
str_split_cut(struct str *s, struct str delim) {
  assert(s);
  assert(delim.ptr);
  assert(str_len(delim) >= 0);

  int p = str_fnd(*s, delim);
  if (p < str_len(*s)) {
    struct str ret = str_lhs(*s, p);
    str_cut_lhs(s, p + 1);
    return ret;
  } else {
    struct str ret = *s;
    *s = str_nil;
    return ret;
  }
}
static int
str_fzy(struct str s, struct str p) {
  const char *pat = str_beg(p);
  const char *str = str_beg(s);

  int run = 1;
  int score = 0;
  int remain = 0;
  for (; str < str_end(s) && pat < str_end(p); str++) {
    // clang-format off
    while (*str == ' ' && str < str_end(s)) {str++;}
    while (*pat == ' ' && pat < str_end(p)) {pat++;}
    // clang-format on
    if (to_lower(*str) == to_lower(*pat)) {
      score += run;
      run++; pat++;
    } else {
      score--; run = 1;
    }
  }
  remain = casti(str_end(s) - str);
  int val = score + remain + casti(str_beg(s) - str);
  int left = casti(str_end(p) - pat);
  return casti(val * left - remain);
}
static void
ut_str(struct sys *s) {
  unused(s);
  static const struct str hay = strv("cmd[stk?utf/boot/usr/str_bootbany.exe");
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
  int end = str_fnd(strv("test.exe"), strv(".exe"));
  assert(end == 4);
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
utf_dec(unsigned *rune, struct str *s) {
  assert(s);
  if (str_is_empty(*s)) {
    if (rune) *rune = UTF_INVALID;
    return strptr(s->ptr, str_end(*s), str_end(*s), s->rng.total);
  }
  int n = 0;
  unsigned ret = 0;
  const char *p = str_beg(*s);
  switch (*p & 0xf0) {
    // clang-format off
    case 0xf0: ret = (*p & 0x07), n = 3; break;
    case 0xe0: ret = (*p & 0x0f), n = 2; break;
    case 0xc0: ret = (*p & 0x1f), n = 1; break;
    case 0xd0: ret = (*p & 0x1f), n = 1; break;
    default:   ret = (*p & 0xff), n = 0; break;
    // clang-format on
  }
  if (str_beg(*s) + n + 1 > str_end(*s)) {
    if (rune) *rune = UTF_INVALID;
    *s = strptr(s->ptr, str_end(*s), str_end(*s), s->rng.total);
    return *s;
  }
  struct str view = strptr(s->ptr, p, p + n + 1, s->rng.total);
  for (int i = 0; i < n; ++i) {
    ret = (ret << 6) | (*(++p) & 0x3f);
  }
  if (rune) {
    *rune = ret;
  }
  *s = strptr(s->ptr, str_beg(*s) + n + 1, str_end(*s), s->rng.total);
  return view;
}
static unsigned
utf_get(struct str s) {
  unsigned rune;
  utf_dec(&rune, &s);
  return rune;
}
static struct str
utf_dec_rev(unsigned *rune, struct str *s) {
  const char *p = str_end(*s);
  while (p > str_beg(*s)) {
    char c = *(--p);
    if (utf_tst(c)) {
      struct str r = strptr(s->ptr, p, str_end(*s), s->rng.total);
      struct str it = utf_dec(rune, &r);
      *s = strptr(s->ptr, str_beg(*s), p, s->rng.total);
      return it;
    }
  }
  *s = str_nil;
  *rune = UTF_INVALID;
  return str_nil;
}
static int
utf_enc(char *c, int cap, unsigned u) {
  int n = 0;
  if (!utf_val(u, 0)) {
    return 0;
  }
  for (n = 1; u > utf_max[n]; ++n);
  if (cap < n || !n || n > UTF_SIZ) {
    return 0;
  }
  for (int i = n - 1; i != 0; --i) {
    c[i] = utf_enc_byte(u, 0);
    u >>= 6;
  }
  c[0] = utf_enc_byte(u, n);
  return n;
}
static struct str
utf_at(unsigned *rune, struct str s, int idx) {
  int i = 0;
  unsigned glyph = 0;
  for utf_loop(&glyph, it, _, s) {
    if (i >= idx) {
      if (rune) *rune = glyph;
      return it;
    }
    i++;
  }
  if (rune) {
    *rune = UTF_INVALID;
  }
  return strptr(s.ptr, str_end(s), str_end(s), s.rng.total);
}
static int
utf_at_idx(struct str s, int idx) {
  struct str view = utf_at(0, s, idx);
  if (str_len(view)) {
    return casti(str_beg(view) - str_beg(s));
  }
  return str_len(s);
}
static int
utf_len(struct str s) {
  int i = 0;
  unsigned rune = 0;
  for utf_loop(&rune, _, __, s) {
    i++;
  }
  return i;
}
/* ---------------------------------------------------------------------------
 *                                String
 * ---------------------------------------------------------------------------
 */
static inline struct str
str_set(char *b, int n, struct str s) {
  assert(b);
  assert(str_len(s) >= 0);
  if (str_is_empty(s)) {
    return strn(b,0);
  } else if (str_len(s) > n) {
    return str_inv;
  }
  mcpy(b, str_beg(s), str_len(s));
  return strn(b, str_len(s));
}
static inline struct str
str_sqz(char *b, int n, struct str s) {
  assert(b);
  assert(str_len(s) >= 0);

  int l = min(n, str_len(s));
  mcpy(b, str_beg(s), l);
  return strn(b,l);
}
static struct str
str_add(char *b, int cap, struct str in, struct str s) {
  assert(b);
  if (str_len(in) + str_len(s) < cap) {
    mcpy(b + str_len(in), str_beg(s), str_len(s));
    return strn(b, str_len(s) + str_len(in));
  }
  int nn = 0;
  unsigned rune = 0;
  int n = cap - str_len(s);
  for utf_loop(&rune, it, _, s) {
    int len = casti(str_end(it) - str_beg(s));
    if (len >= n) {
      break;
    }
    nn = len;
  }
  mcpy(b + str_len(in), str_beg(s), nn);
  return strn(b, str_len(in) + nn);
}
static struct str
str_rm(char *b, struct str in, int cnt) {
  assert(b);
  int left = max(0, str_len(in) - cnt);
  return strn(b, left);
}
static struct str
str_put(char *b, int cap, struct str in, int pos, struct str s) {
  if (pos >= str_len(in)) {
    return str_add(b, cap, in, s);
  }
  if (str_len(in) + str_len(s) < cap) {
    memmove(b + pos + str_len(s), b + pos, castsz(str_len(in) - pos));
    mcpy(b + pos, str_beg(s), str_len(s));
    return strn(b, str_len(in) + str_len(s));
  }
  int nn = 0;
  unsigned rune = 0;
  int n = cap - str_len(s);
  for utf_loop(&rune, it, _, s) {
    int cnt = casti(str_end(it) - str_beg(s));
    if (cnt >= -n) {
      break;
    }
    nn = cnt;
  }
  memmove(b + pos + str_len(s), b + pos, cast(size_t, str_len(in) - pos));
  mcpy(b + pos, str_beg(s), str_len(s));
  return strn(b, str_len(s) + str_len(in));
}
static struct str
str_del(char *b, struct str in, int pos, int len) {
  assert(b);
  if (pos >= str_len(in)) {
    return str_rm(b, in, len);
  }
  assert(pos + len <= str_len(in));
  memmove(b + pos, b + pos + len, castsz(str_len(in) - pos));
  return strn(b, str_len(in) - len);
}
static struct str
str_fmtsn(char *buf, int n, const char *fmt, ...) {
  int ret;
  va_list va;
  va_start(va, fmt);
  ret = fmtvsn(buf, n, fmt, va);
  va_end(va);
  return strn(buf, ret);
}
static struct str
str_add_fmt(char *b, int cap, struct str in, const char *fmt, ...) {
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
#define time_inf    (9223372036854775807ll)
#define time_ninf   (-9223372036854775807ll)
#define time_ns(ns) castll((ns))
#define time_us(us) (castll((us))*1000ll)
#define time_ms(ms) (castll((ms))*1000000ll)
#define time_sec(s) (castll((s))*1000000000ll)
#define time_min(m) (castll((m))*60000000000ll)
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

#define ns_time(t) (t)
#define us_time(t) ((t)/1000ll)
#define ms_time(t) ((t)/1000000ll)
#define sec_time(t) ((t)/1000000000ll)
#define min_time(t) ((t)/60000000000ll)
#define hour_time(t) ((t)/3600000000000ll)

#define sec_flt_time(t) castf(castd(t)*(1.0/1000000000.0))
#define min_flt_time(t) castf(castd(t)*(1.0/10000000000.0))
#define hour_flt_time(t) castf(castd(t)*(1.0/3600000000000.0))

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
 *                              Ticket Mutex
 * ---------------------------------------------------------------------------
 */
struct lck {
  unsigned long long volatile ticket;
  unsigned long long volatile serving;
};
static inline void
lck_acq(struct lck *lck) {
  unsigned long long t = atom_add(&lck->ticket, 1);
  while(t != lck->serving) {
    yield();
  }
}
static inline void
lck_rel(struct lck *lck) {
  atom_add(&lck->serving, 1);
}

/* ---------------------------------------------------------------------------
 *                                  Arena
 * ---------------------------------------------------------------------------
 */
#define ARENA_ALIGNMENT 8
#define ARENA_BLOCK_SIZE KB(64)

// clang-format off
#define arena_obj(a, s, T) cast(T*, arena_alloc(a, s, szof(T)))
#define arena_arr(a, s, T, n) cast(T*, arena_alloc(a, s, szof(T) * n))
#define arena_scope(a,s,sys)\
  (int uniqid(_i_) = (arena_scope_push(s,a), 0); uniqid(_i_) < 1;\
    uniqid(_i_) = (arena_scope_pop(s,a,sys), 1))
// clang-format on

static int
arena_align_off(struct arena *a, int align) {
  intptr_t ret = (intptr_t)a->blk->base + a->blk->used;
  int msk = align - 1;
  int off = 0;
  if(ret & msk) {
    off = casti((align - (ret & msk)));
  }
  return off;
}
static int
arena_size_for(struct arena *a, int size_init) {
  int off = arena_align_off(a, ARENA_ALIGNMENT);
  return size_init + off;
}
static void*
arena_alloc(struct arena *a, struct sys *s, int size_init) {
  assert(a);
  assert(s);

  int siz = 0;
  void *ret = 0;
  if (a->blk) {
    siz = arena_size_for(a, size_init);
  }
  if (!a->blk || ((a->blk->used + siz) > a->blk->size)) {
    siz = size_init; /* allocate new block */
    int blksiz = max(!a->blksiz ? ARENA_BLOCK_SIZE : a->blksiz, siz);
    struct mem_blk *blk = s->mem.alloc(s, 0, blksiz, 0, 0);
    blk->prv = a->blk;
    a->blk = blk;
  }
  assert((a->blk->used + siz) <= a->blk->size);
  int align_off = arena_align_off(a, ARENA_ALIGNMENT);
  int off = a->blk->used + align_off;
  ret = a->blk->base + off;
  a->blk->used += siz;

  assert(siz >= size_init);
  assert(a->blk->used <= a->blk->size);
  mset(ret, 0, size_init);
  return ret;
}
static void *
arena_cpy(struct arena *a, struct sys *s, const void *src, int siz) {
  assert(a);
  assert(s);
  assert(src);
  assert(siz > 0);

  void *dst = arena_alloc(a, s, siz);
  mcpy(dst, src, siz);
  return dst;
}
static struct str
arena_fmt(struct arena *a, struct sys *s, const char *fmt, ...) {
  assert(a);
  assert(s);
  assert(fmt);

  va_list args;
  va_start(args, fmt);
  int n = fmtvsn(0, 0, fmt, args);
  va_end(args);

  char *ret = arena_alloc(a, s, n + 1);
  va_start(args, fmt);
  fmtvsn(ret, n + 1, fmt, args);
  va_end(args);
  return strn(ret, n);
}
static char *
arena_cstr_rng(struct arena *a, struct sys *s, struct str cs, struct rng r) {
  assert(a);
  assert(s);
  if (!str_len(cs) || !r.cnt) {
    return 0;
  }
  char *ret = arena_alloc(a, s, r.cnt + 1);
  mcpy(ret, str_beg(cs), str_len(cs));
  ret[r.cnt] = 0;
  return ret;
}
static inline char *
arena_cstr(struct arena *a, struct sys *s, struct str cs) {
  assert(a);
  assert(s);
  return arena_cstr_rng(a, s, cs, rngn(str_len(cs)));
}
static inline struct str
arena_str(struct arena *a, struct sys *s, struct str cs) {
  assert(a);
  assert(s);
  char *ret = arena_cstr(a, s, cs);
  return strn(ret, str_len(cs));
}
static inline struct str
arena_str_rng(struct arena *a, struct sys *s, struct str cs, struct rng r) {
  assert(a);
  assert(s);
  char *ret = arena_cstr_rng(a, s, cs, r);
  return strn(ret, r.cnt);
}
static void
arena_free_last_blk(struct arena *a, struct sys *s) {
  assert(a);
  assert(s);

  struct mem_blk *blk = a->blk;
  a->blk = blk->prv;
  s->mem.free(s, blk);
}
static void
arena_reset(struct arena *a, struct sys *s) {
  assert(a);
  assert(s);
  while (a->blk) {
    if (a->blk->prv == 0) {
      a->blk->used = 0;
      break;
    }
    arena_free_last_blk(a, s);
  }
}
static void
arena_free(struct arena *a, struct sys *s) {
  assert(a);
  assert(s);
  while (a->blk) {
    int is_last = (a->blk->prv == 0);
    arena_free_last_blk(a, s);
    if(is_last) {
      break;
    }
  }
}
static int
arena_scope_push(struct arena_scope *s, struct arena *a) {
  assert(a);
  assert(s);

  s->blk = a->blk;
  s->used = a->blk ? a->blk->used : 0;
  a->tmp_cnt++;
  return 1;
}
static int
arena_scope_pop(struct arena_scope *s, struct arena *a, struct sys *_sys) {
  assert(s);
  assert(a);
  assert(_sys);
  while (a->blk != s->blk) {
    arena_free_last_blk(a, _sys);
  }
  if (a->blk) {
    assert(a->blk->used >= s->used);
    a->blk->used = s->used;
  }
  assert(a->tmp_cnt > 0);
  a->tmp_cnt--;
  return 1;
}

/* ---------------------------------------------------------------------------
 *                                  List
 * ---------------------------------------------------------------------------
 */
// clang-format off
#define lst_init(e) (e)->prv = (e), (e)->nxt = (e)
#define lst_get(ptr,type,mem) containerof(ptr,type,mem)
static void lst__add(struct lst_elm *e, struct lst_elm *p, struct lst_elm *n)\
  {(e)->nxt = (n); (e)->prv = (p); (p)->nxt = (e); (n)->prv = (e);}
#define lst_add(lst,e) lst__add(e, lst, (lst)->nxt)
#define lst_add_tail(lst,e) lst__add(e, (lst)->prv, lst)
static void lst__del(struct lst_elm *p, struct lst_elm *n) {n->prv = p, p->nxt = n;}
#define lst_del(n) lst__del((n)->prv, (n)->nxt)
#define lst_empty(lst) ((lst)->nxt == (lst))
#define lst_any(lst) (!lst_empty(lst))
#define lst_first(lst) ((lst)->nxt)
#define lst_last(lst) ((lst)->prv)
#define lst_each(e,l) ((e) = (l)->nxt; (e) != (l); (e) = (e)->nxt)
#define lst_each_safe(a,b,l)\
  ((a) = (l)->nxt, (b) = (a)->nxt; (a) != (l); (a) = (b), (b) = (a)->nxt)
#define lst_each_rev(e,l) ((e) = (l)->prv; (e) != (l); (e) = (e)->prv)
#define lst_each_rev_safe(e,l)\
 ((a) = (l)->prv, (b) = (a)->prv; (a) != (l); (a) = (b), (b) = (a)->prv)
// clang-format on

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
 *                              STRING-BUFFER
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
  assert(*cnt >= 0);
  assert(cap >= 0);
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
  assert(str_buf_len(hdl) < cnt);
  assert(str_buf_off(hdl) < cnt);
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
#define tbl__loop(n,i,t,cap) (int n = tbl__nxt_idx(t,cap,0), i = 0; n < cap; n = tbl__nxt_idx(t,cap,n+1),++i)
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
tbl__hash(unsigned long long k) {
  unsigned long long h = k & 0x7fffffffffffffffllu;
  return h | (h == 0);
}
static void
tbl__swap(void *a, void *b, void *tmp, int siz) {
  mcpy(tmp, a, siz);
  mcpy(a, b, siz);
  mcpy(b, tmp, siz);
}
static inline long long
tbl__store(unsigned long long *keys, void *vals, int *cnt,
           unsigned long long i, unsigned long long h,
           void* val, int val_siz) {
  keys[i] = h;
  if (vals) {
    unsigned long long off = (unsigned long long)val_siz * i;
    mcpy((unsigned char*)vals + off, val, val_siz);
  }
  *cnt += 1;
  return castll(i);
}
static long long
tbl__put(unsigned long long *keys, void *vals, int *cnt, int cap,
         unsigned long long key, void *v, int val_siz) {
  unsigned long long h = tbl__hash(key);
  unsigned long long n = castull(cap);
  unsigned long long i = h % n, b = i, dist = 0;
  do {
    unsigned long long k = keys[i];
    if (!k) return tbl__store(keys, vals, cnt, i, h, v, val_siz);
    unsigned long long d = tbl__dist(k, n, i);
    if (d++ > dist++) continue;
    if (tbl__is_del(k)) {
      return tbl__store(keys, vals, cnt, i, h, v, val_siz);
    }
    iswap(h, keys[i]);
    if (vals) {
      void *tmp_val = (unsigned char*)vals + cap * val_siz;
      void *cur_val = (unsigned char*)vals + i * (unsigned long long)val_siz;
      tbl__swap(cur_val, v, tmp_val, val_siz);
    }
    dist = d;
  } while ((i = ((i + 1) % n)) != b);
  return castll(n);
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

/* ---------------------------------------------------------------------------
 *                                  Sort
 * ---------------------------------------------------------------------------
 */
// clang-format off
typedef void*(sort_access_f)(const void *data, void *usr);
#define sort__access(a,usr,access,conv,off) ((access) ? (conv)((access)(a + off, usr)) : (conv)(a + off))

/* conversion functions from type -> sortable unsigned short/integer representation */
typedef unsigned(*sort_conv_f)(const void *p);
static force_inline unsigned sort__cast_ushort(const void *p) {return *(const unsigned short*)p;}
static force_inline unsigned sort__cast_short(const void *p) {union bit_castu {short i; unsigned short u;} v = {.i = *(const short*)p}; return v.u ^ (1u << 15u);}
static force_inline unsigned sort__cast_hflt(const void *p) {unsigned short u = *(const unsigned short*)p; if ((u >> 15u) == 1u) {u *= (unsigned short)-1; u ^= (1u << 15u);} return u ^ (1u << 15u);}
static force_inline unsigned sort__cast_uint(const void *p) {return *(const unsigned*)p;}
static force_inline unsigned sort__cast_int(const void *p) {union bit_castu {int i; unsigned u;} v = {.i = *(const int*)p}; return v.u ^ (1u << 31u);}
static force_inline unsigned sort__cast_flt(const void *p) {union bit_castu {float f; unsigned u;} v = {.f = *(const float*)p}; if ((v.u >> 31u) == 1u) {v.u *= (unsigned)-1; v.u ^= (1u << 31u);} return v.u ^ (1u << 31u);}

#define sort_short(out,a,siz,n,off) sort_radix16(out,a,siz,n,off,0,0,sort__cast_short)
#define sort_ushort(out,a,siz,n,off) sort_radix16(out,a,siz,n,off,0,0,sort__cast_ushort)
#define sort_hflt(out,a,siz,n,off) sort_radix16(out,a,siz,n,off,0,0,sort__cast_hflt)
#define sort_int(out,a,siz,n,off) sort_radix32(out,a,siz,n,off,0,0,sort__cast_int)
#define sort_uint(out,a,siz,n,off) sort_radix32(out,a,siz,n,off,0,0,sort__cast_uint)
#define sort_flt(out,a,siz,n,off) sort_radix32(out,a,siz,n,off,0,0,sort__cast_flt)

#define sort_shorts(out,a,n) sort_short(out,a,szof(short),n,0)
#define sort_ushorts(out,a,n) sort_ushort(out,a,szof(unsigned short),n,0)
#define sort_hflts(out,a,n) sort_hflt(out,a,szof(unsigned short),n,0)
#define sort_ints(out,a,n) sort_int(out,a,szof(int),n,0)
#define sort_uints(out,a,n) sort_uint(out,a,szof(unsigned),n,0)
#define sort_flts(out,a,n) sort_flt(out,a,szof(float),n,0)
// clang-format on

static force_inline void
sort__radix16(unsigned *restrict out, const void *a, int siz, int n, int off,
             void *usr, sort_access_f access, sort_conv_f conv) {
  assert(a);
  assert(out);
  /* <!> out needs to be at least size: 2*n+512 <!> */
  unsigned *buf = out + 2 * n;
  unsigned *restrict h[] = {buf, buf + 256};
  const unsigned char *b = cast(const unsigned char*, a);
  if (!n) {
    return;
  }
  /* build histogram */
  int is_sorted = 1;
  mset(buf,0,512*szof(unsigned));
  unsigned lst = sort__access(b + out[0] * (unsigned)siz, usr, access, conv, off);
  for loop(i,n) {
    unsigned k = sort__access(b + out[i] * (unsigned)siz, usr, access, conv, off);
    h[0][k & 0xff]++;
    h[1][(k >> 8) & 0xff]++;
    is_sorted = (k < lst) ? 0 : is_sorted;
    lst = k;
  }
  if (is_sorted) {
    return; /* already sorted so early out */
  }
  /* convert histogram into offset table */
  unsigned sum[2] = {0};
  for loop(i,256) {
    unsigned t0 = h[0][i] + sum[0]; h[0][i] = sum[0], sum[0] = t0;
    unsigned t1 = h[1][i] + sum[1]; h[1][i] = sum[1], sum[1] = t1;
  }
  /* sort 8-bits at a time */
  unsigned *restrict idx[] = {out, out + n};
  for (int p = 0, s = 0, d = 1; p < 2; ++p, d = !d, s = !s) {
    for loop(i,n) {
      unsigned at = idx[s][i] * castu(siz);
      unsigned k = sort__access(b + at, usr, access, conv, off);
      idx[d][h[p][(k>>(8 * p))&0xff]++] = at;
    }
  }
}
static force_inline void
sort_radix16(unsigned *restrict out, const void *a, int siz, int n, int off,
             void *usr, sort_access_f access, sort_conv_f conv) {
  assert(a);
  assert(out);
  seq_rngu(out, rngn(n));
  sort__radix16(out, a, siz, n, off, usr, access, conv);
}
static force_inline void
sort__radix32(unsigned *restrict out, const void *a, int siz, int n, int off,
             void *usr, sort_access_f access, sort_conv_f conv) {
  assert(a);
  assert(out);
  /* <!> out needs to be at least size: 2*n+1024 <!> */
  unsigned *buf = out + 2 * n;
  unsigned *restrict h[] = {buf, buf + 256, buf + 512, buf + 768};
  const unsigned char *b = cast(const unsigned char*, a);
  if (!n) {
    return;
  }
  /* build histogram */
  int is_sorted = 1;
  mset(buf,0,1024*szof(unsigned));
  unsigned lst = sort__access(b + out[0] * (unsigned)siz, usr, access, conv, off);
  for loop(i,n) {
    unsigned k = sort__access(b + out[i] * (unsigned)siz, usr, access, conv, off);
    h[0][(k & 0xff)]++;
    h[1][(k >> 8) & 0xff]++;
    h[2][(k >> 16) & 0xff]++;
    h[3][(k >> 24)]++;
    is_sorted = (k < lst) ? 0 : is_sorted;
    lst = k;
  }
  if (is_sorted) {
    return; /* already sorted. early out */
  }
  /* convert histogram into offset table */
  unsigned sum[4] = {0};
  for loop(i,256) {
    unsigned t0 = h[0][i] + sum[0]; h[0][i] = sum[0], sum[0] = t0;
    unsigned t1 = h[1][i] + sum[1]; h[1][i] = sum[1], sum[1] = t1;
    unsigned t2 = h[2][i] + sum[2]; h[2][i] = sum[2], sum[2] = t2;
    unsigned t3 = h[3][i] + sum[3]; h[3][i] = sum[3], sum[3] = t3;
  }
  /* sort 8-bit at a time */
  unsigned *restrict idx[2] = {out, out + n};
  for (int p = 0, s = 0, d = 1; p < 4; ++p, d = !d, s = !s) {
    for loop(i,n) {
      unsigned at = idx[s][i] * castu(siz);
      unsigned k = sort__access(b + at, usr, access, conv, off);
      idx[d][h[p][(k>>(8*p))&0xff]++] = at;
    }
  }
}
static force_inline void
sort_radix32(unsigned *restrict out, const void *a, int siz, int n, int off,
             void *usr, sort_access_f access, sort_conv_f conv) {
  assert(a);
  assert(out);
  seq_rngu(out, rngn(n));
  sort__radix32(out, a, siz, n, off, usr, access, conv);
}
/* ---------------------------------------------------------------------------
 *                                  Search
 * ---------------------------------------------------------------------------
 */
static int
sorted_search(const void *vals, int cnt, int siz, void *val,
              int(*cmp_less)(const void *a, const void *b)) {
  assert(val);
  assert(vals);
  assert(cmp_less);

  int nleft = cnt;
  const unsigned char *base = vals;
  for (;;) {
    int half = nleft >> 1;
    if (half <= 0) {
      break;
    }
    const unsigned char *mid = base + half * siz;
    base = cmp_less(mid, val) ? mid : base;
    nleft -= half;
  }
  if (nleft == 1) {
    base += cmp_less(base, val) ? siz : 0;
  }
  return casti((base - cast(const unsigned char*, vals)))/siz;
}

/* ---------------------------------------------------------------------------
 *                            Command Arguments
 * --------------------------------------------------------------------------- */
// clang-format off
#define cmd_arg_begin(argv0, argc, argv) \
  for (argv0 = *argv, argv++, argc--; argv[0] && argv[0][1] && argv[0][0] == '-'; argc--, argv++) {\
    char argc_, **argv_; int brk_;\
    if (argv[0][1] == '-' && argv[0][2] == '\0') {argv++; argc--; break;}\
    for (brk_ = 0, argv[0]++, argv_ = argv; argv[0][0] && !brk_; argv[0]++) {\
      if (argv_ != argv) break;\
      argc_ = argv[0][0];\
      switch (argc_)
#define cmd_argc() argc_
#define cmd_arg_opt_str(argv, x) ((argv[0][1] == '\0' && argv[1] == 0)?\
  ((x), (char *)0) : (brk_ = 1, (argv[0][1] != '\0') ?\
    (&argv[0][1]) : (argc--, argv++, argv[0])))
#define cmd_arg_opt_int(argv,x) cmd_arg_int(cmd_arg_opt_str(argv,x))
#define cmd_arg_opt_flt(argv,x) cmd_arg_flt(cmd_arg_opt_str(argv,x))
#define cmd_arg_end }}
// clang-format on

/* ---------------------------------------------------------------------------
 *                                  Image
 * ---------------------------------------------------------------------------
 */
#define img_w(img) (img)[-2]
#define img_h(img) (img)[-1]
#define img_pix(img, x, y) \
  (img)[castu((y)) * img_w(img) + castu((x))]

static unsigned *
img_mk(unsigned *img, int w, int h) {
  img += 2;
  img_w(img) = castu(w);
  img_h(img) = castu(h);
  return img;
}
static unsigned *
img_new(struct arena *a, struct sys *s, int w, int h) {
  unsigned *img = arena_alloc(a, s, szof(unsigned) * (w * h + 2));
  return img_mk(img, w, h);
}

