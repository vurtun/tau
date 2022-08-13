/* ---------------------------------------------------------------------------
 *                                Foreach
 * ---------------------------------------------------------------------------
 */
static inline int
rng__bnd(int i, int n) {
  int l = max(n, 1) - 1;
  int v = (i < 0) ? (n - i) : i;
  return clamp(v, 0, l);
}
static inline struct rng
rng__mk(int lo, int hi, int s) {
  struct rng r = {.lo = lo, .hi = hi, .step = s};
  assert((lo <= hi && s > 0) || (lo >= hi && s < 0));
  r.cnt = abs(r.hi - r.lo);
  return r;
}
#define arrv(b) (b), cntof(b)
#define arr(b) (b), dyn_cnt((b))
#define rng(b,e,s,n) rng__mk(rng__bnd(b,n), rng__bnd(e,n), s)
#define intvl(b,s,n) rng(b,n,s,n)

#define forever while(1)
#define for_nstep(i,n,s) for (int i = 0; i < (n); i += (s))
#define for_cnt(i,n) for_nstep(i,n,1)
#define fori_cnt(i,n) for (i = 0; i < (n); i += 1)
#define for_rng(i,l,r)\
  for (int i = (r).lo, l = 0; i != (r).hi; i += (r).step, ++l)

#define for_arrp(it,a,e) for ((it) = (a); (it) < (e); ++(it))
#define for_arr(it,a,n) for_arrp(it,a,(a)+(n))
#define for_arrv(it,a) for_arr(it,a,cntof(a))
#define fori_arrv(i,a) for (int i = 0; i < cntof(a); ++i)
#define for_arr_rng(it,a,r)\
  for ((it) = (a) + (r).lo; (it) != (a) + (r).hi; (it) += (r).step)
#define for_arrv_rng(it,a, b,e,s)       \
  for ((it) = (a) + rng(b,e,s,cntof(a)).lo;\
       (it) != (a) + rng(b,e,s,cntof(a)).hi;\
       (it) += rng(b,e,s,cntof(a)).step)

/* ---------------------------------------------------------------------------
 *                                Memory
 * ---------------------------------------------------------------------------
 */
static void
mcpy(void* restrict dstp, void const *restrict srcp, int bytes) {
  #define PERLOOP (4*szof(bigreg))
  assert(dstp);
  assert(srcp);
  assert(bytes >= 0);
  assert((((uintptr_t)srcp) & 15)==0);
  assert((((uintptr_t)dstp) & 15)==0);
  assert(srcp != dstp);

  int s = bytes/PERLOOP;
  unsigned char const *src = (unsigned char const*)srcp;
  unsigned char *dst = (unsigned char*)dstp;
  int rest = bytes & 15;

  bytes = (bytes & 63) / 16;
  while(s) { // 64-byte chunks
#ifdef __clang__
  __asm__ __volatile__("");
#endif
    bigreg a; bigreg_ld(a,src+0);
    bigreg b; bigreg_ld(b,src+1);
    bigreg c; bigreg_ld(c,src+2);
    bigreg d; bigreg_ld(d,src+3);

    bigreg_str(((bigreg*)(void*)dst)+0,a);
    bigreg_str(((bigreg*)(void*)dst)+1,b);
    bigreg_str(((bigreg*)(void*)dst)+2,c);
    bigreg_str(((bigreg*)(void*)dst)+3,d);

    src+=PERLOOP;
    dst+=PERLOOP;
    --s;
  };
  #undef PERLOOP
  while (bytes) { // 16-byte chunks
    bigreg a;
    bigreg_ld(a,src);
    bigreg_str(dst,a);

    src+=16;
    dst+=16;
    --bytes;
  }
  while (rest) {
    *dst = *src;
    src+=1;
    dst+=1;
    --rest;
  }
}
static void
mset(void *addr, unsigned char c, int n) {
  assert(addr);
  assert(n >= 0);
  unsigned char *dst = addr;

  bigreg z = bigreg_u8(c);
  #define PERLOOP (2*szof(bigreg))
  int s = n/PERLOOP;
  int bytes = (n & 63) / 16;
  int rest = n & 15;
  while(s) { // 64-byte chunks
#ifdef __clang__
  __asm__ __volatile__("");
#endif
    bigreg_str(((bigreg*)(void*)dst)+0,z);
    bigreg_str(((bigreg*)(void*)dst)+1,z);
    dst+=PERLOOP;
    --s;
  };
  while (bytes) { // 16-byte chunks
    bigreg_str(dst,z);
    dst+=16;
    --bytes;
  }
  while (rest) {
    *dst = 0;
    dst+=1;
    --rest;
  }
  #undef PERLOOP
}

/* ---------------------------------------------------------------------------
 *                                  Hash
 * ---------------------------------------------------------------------------
 */
#define FNV1A32_HASH_INITIAL 2166136261u
#define FNV1A64_HASH_INITIAL 14695981039346656037llu
static const unsigned char sse_align aes_seed[16] = {
  178, 201, 95, 240, 40, 41, 143, 216,
  2, 209, 178, 114, 232, 4, 176, 188
};
static unsigned
fnv1a32(const void *ptr, int size, unsigned h) {
  const unsigned char *p = ptr;
  for (int i = 0; i < size; ++i) {
     h = (h ^ p[i]) * 16777619u;
  }
  return h;
}
static unsigned long long
fnv1a64(const void *ptr, int len, unsigned long long h) {
  const unsigned char *p = ptr;
  for (int i = 0; i < len; ++i) {
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
static aes128
aes128_hash(const void *src, int len, aes128 seedx16) {
  static const char unsigned msk[32] = {
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0
  };
  const unsigned char *at = src;
  aes128 h = aes128_int(cast(unsigned,len));
  h = aes128_xor(h, seedx16);
  int n = len >> 4;
  while (n--) {
    aes128 in = aes128_load(at);
    h = aes128_xor(h, in);
    h = aes128_dec(h, aes128_zero());
    h = aes128_dec(h, aes128_zero());
    h = aes128_dec(h, aes128_zero());
    h = aes128_dec(h, aes128_zero());
    at += 16;
  }
  int over = len & 15;
  aes128 in = aes128_load(at);
  in = aes128_and(in, aes128_load((msk + 16 - over)));
  h = aes128_xor(h, in);
  h = aes128_dec(h, aes128_zero());
  h = aes128_dec(h, aes128_zero());
  h = aes128_dec(h, aes128_zero());
  h = aes128_dec(h, aes128_zero());
  return h;
}
static unsigned long long
rnd_gen(unsigned long long x, int n) {
  return x + cast(unsigned long long, n) * 0x9E3779B97F4A7C15llu;
}
static unsigned long long
rnd_mix(unsigned long long z) {
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9llu;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBllu;
  return z ^ (z >> 31llu);
}
static unsigned long long
rnd_split_mix(unsigned long long *x, int i) {
  *x = rnd_gen(*x, i);
  return rnd_mix(*x);
}
static unsigned long long
rnd(unsigned long long *x) {
  return rnd_split_mix(x, 1);
}
static unsigned
rndu(unsigned long long *x) {
  unsigned long long z = rnd(x);
  return cast(unsigned, z & 0xffffffffu);
}
static int
rndi(unsigned long long *x) {
  unsigned z = rndu(x);
  long long n = cast(long long, z) - (UINT_MAX/2);
  assert(n >= INT_MIN && n <= INT_MAX);
  return cast(int, n);
}
static double
rndn(unsigned long long *x) {
  unsigned n = rndu(x);
  return cast(double,n) / cast(double,UINT_MAX);
}

/* ---------------------------------------------------------------------------
 *                                  Bitset
 * ---------------------------------------------------------------------------
 */
#define for_bitset(i,x,s,n) \
  for (int i = bit_ffs(s,n,0), x = 0; i < n; i = bit_ffs(s,n,i+1), x = x + 1)

static int
bit_set(unsigned long *addr, int nr) {
  unsigned long m = bit_mask(nr);
  unsigned long *p = addr + bit_word(nr);
  int ret = cast(int, *p &m);
  *p |= m;
  return ret;
}
static void
bit_set_on(unsigned long *addr, int nr, int cond) {
  if (cond) {
    bit_set(addr, nr);
  }
}
static int
bit_clr(unsigned long *addr, int nr) {
  assert(addr);
  unsigned long m = bit_mask(nr);
  unsigned long *p = addr + bit_word(nr);
  int ret = cast(int, (*p & m));
  *p &= ~m;
  return ret;
}
static void
bit_clr_on(unsigned long *addr, int nr, int cond) {
  if (cond) {
    bit_clr(addr, nr);
  }
}
static int
bit_xor(unsigned long *addr, int nr) {
  assert(addr);
  unsigned long m = bit_mask(nr);
  unsigned long *p = addr + bit_word(nr);
  *p ^= m;
  return (*p & m) ? 1 : 0;
}
static void
bit_fill(unsigned long *addr, int byte, int nbits) {
  assert(addr);
  int n = bits_to_long(nbits);
  unsigned long len = (unsigned long)n * sizeof(long);
  memset(addr, byte, len);
}
static int
bit_tst(const unsigned long *addr, int nr) {
  assert(addr);
  unsigned long msk = (unsigned long)nr & (BITS_PER_LONG - 1);
  return (1ul & (addr[bit_word(nr)] >> msk)) != 0;
}
static int
bit_tst_clr(unsigned long *addr, int nr) {
  assert(addr);
  if (bit_tst(addr, nr)) {
    bit_xor(addr, nr);
    return 1;
  }
  return 0;
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
    int pos = __builtin_ctzl(c);
    return (int)(i * BITS_PER_LONG) + pos;
  }
  return nbits;
}
static int
bit_ffz(const unsigned long *addr, int nbits, int idx) {
  assert(addr);
  unsigned long off = bit_word_idx(idx);
  unsigned long long n = (unsigned long long)bits_to_long(nbits);
  for (unsigned long i = bit_word(idx); i < n; ++i) {
    unsigned long cmsk = bit_mask(off) - 1u;
    unsigned long c = (~addr[i]) & (unsigned long)~cmsk;
    if (!c) {
      off = bit_word_nxt(off);
      continue;
    }
    int pos = __builtin_ctzl(c);
    return (int)(i * BITS_PER_LONG) + pos;
  }
  return nbits;
}
static int
bit_cnt_set(const unsigned long *addr, int nbits, int idx) {
  assert(addr);
  unsigned long widx = bit_word(idx);
  unsigned long cmsk = max(1u, bit_mask(idx)) - 1u;
  if ((unsigned long)nbits < BITS_PER_LONG) {
    cmsk |= ~(max(1u, bit_mask(nbits)) - 1u);
  }
  unsigned long w = addr[widx] & ~cmsk;
  unsigned long long n = (unsigned long long)bits_to_long(nbits);
  int cnt = __builtin_popcountll(w);
  for (unsigned long i = widx + 1; i < n; ++i) {
    w = addr[i];
    if ((unsigned long)nbits - BITS_PER_LONG * i < BITS_PER_LONG) {
      cmsk |= ~(bit_mask(nbits) - 1u);
      w = w & ~cmsk;
    }
    cnt += __builtin_popcountll(w);
  }
  return cnt;
}
static int
bit_cnt_zero(const unsigned long *addr, int nbits, int idx) {
  assert(addr);
  unsigned long widx = bit_word(idx);
  unsigned long cmsk = max(1u, bit_mask(idx)) - 1u;
  if ((unsigned long)nbits < BITS_PER_LONG) {
    cmsk |= ~(max(1u, bit_mask(nbits)) - 1u);
  }
  unsigned long long n = (unsigned long long)bits_to_long(nbits);
  unsigned long w = addr[widx] & ~cmsk;
  int cnt = __builtin_popcountll(~w);
  for (unsigned long i = widx + 1; i < n; ++i) {
    w = addr[i];
    if ((unsigned long)nbits - BITS_PER_LONG * i < BITS_PER_LONG) {
      cmsk |= ~(bit_mask(nbits) - 1u);
      w = w & ~cmsk;
    }
    cnt += __builtin_popcountll(~w);
  }
  return min(nbits, cnt);
}
static int
bit_set_at(const unsigned long *addr, int nbits, int off, int idx) {
  assert(addr);
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
 *                                  Char
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
static int str__match_here(struct str reg, struct str txt);
static int str__match_star(int c, struct str reg, struct str txt);

// clang-format off
#define H1(s,i,x)   (x*65599u+(unsigned char)s[((i)<(cntof(s)-1))?cntof(s)-2-(i):(cntof(s)-1)])
#define H4(s,i,x)   H1(s,i,H1(s,i+1,H1(s,i+2,H1(s,i+3,x))))
#define H8(s,i,x)   H4(s,i,H4(s,i+4,x))
#define H16(s,i,x)  H4(s,i,H4(s,i+4,H4(s,i+8,H4(s,i+12,x))))
#define H32(s,i,x)  H16(s,i,H16(s,i+16,x))
#define H64(s,i,x)  H16(s,i,H16(s,i+16,H16(s,i+32,H16(s,i+48,x))))

#define STR_HASH4(s)    cast(unsigned,(H4(s,0,0)^(H4(s,0,0)>>16)))
#define STR_HASH8(s)    cast(unsigned,(H8(s,0,0)^(H8(s,0,0)>>16)))
#define STR_HASH16(s)   cast(unsigned,(H16(s,0,0)^(H16(s,0,0)>>16)))
#define STR_HASH32(s)   cast(unsigned,(H32(s,0,0)^(H32(s,0,0)>>16)))
#define STR_HASH64(s)   cast(unsigned,(H64(s,0,0)^(H64(s,0,0)>>16)))

static inline unsigned
str__match_hash(struct str s) {
  unsigned int h = 0;
  for (int i = 0; s.len; ++i) {
    h = 65599u * h + (unsigned char)s.str[i];
  }
  return h ^ (h >> 16);
}
#define match(s) switch(str__match_hash(s))
#define with4(s) case STR_HASH4(s)
#define with8(s) case STR_HASH8(s)
#define with16(s) case STR_HASH16(s)
#define with32(s) case STR_HASH32(s)
#define with64(s) case STR_HASH64(s)

#define cstrn(s) cast(int, strlen(s))
#define str(s,n) (struct str){s, (s) + (n), (n)}
#define strp(b,e) (struct str){.str = (b), .end = (e), .len = cast(int, (e) - (b))}
#define str0(s) (struct str){(s), (s) + cstrn(s), cstrn(s)}
#define strv(s) str(s, cntof(s)-1)
#define strf(s) (s).len, (s).str
#define str_inv (struct str){0,0,-1}
#define str_nil (struct str){0,0,0}
#define str_eq(a,b) (str_cmp(a,b) == 0)
#define str_neq(a,b) (str_cmp(a,b) != 0)
#define str_sub(s,b,e) str((s).str + (b), (e) - (b))
#define str_rhs(s, n) str_sub(s, min((s).len, n), (s).len)
#define str_lhs(s, n) str_sub(s, 0, min((s).len, n))
#define str_cut_lhs(s, n) *(s) = str_rhs(*(s), n)
#define str_cut_rhs(s, n) *(s) = str_lhs(*(s), n)

#define for_str(it,c,s)\
  for (const char *it = (c)->str; it < (c)->end; it += (s))
#define fori_str(i,c,s)\
  for (int i = 0; i < (c)->len; i += (s))
#define for_str_rng(it,a, b,e,s)\
  for (const char *it = (a)->str + rng(b,e,s,(a)->len).lo;\
      (it) != (a)->str + rng(b,e,s,(a)->len).hi;\
      (it) += rng(b,e,s,(a)->len).step)
#define for_str_tok(it, rest, src, delim)                       \
  for (struct str rest = src, it = str_split_cut(&rest, delim); \
       it.len; it = str_split_cut(&rest, delim))
// clang-format on

static unsigned long long
str__hash(struct str s, unsigned long long id) {
  return fnv1a64(s.str, s.len, id);
}
static unsigned long long
str_hash(struct str s) {
  return str__hash(s, FNV1A64_HASH_INITIAL);
}
static int
str_cmp(struct str a, struct str b) {
  int n = min(a.len, b.len);
  for_cnt(i, n) {
    if (a.str[i] < b.str[i]) {
      return -1;
    } else if (a.str[i] > b.str[i]) {
      return +1;
    }
  }
  if (a.len > b.len) {
    return +1;
  } else if (b.len > a.len) {
    return -1;
  }
  return 0;
}
static void
str_fnd_tbl(struct str_fnd_tbl *fnd, struct str s) {
  if (s.len <= 1) return;
  for (int i = 0; i < UCHAR_MAX + 1; ++i) {
    fnd->tbl[i] = s.len;
  }
  if (s.len > 0) {
    int n = s.len - 1;
    for (int i = 0; i < n; ++i) {
      unsigned char at = cast(unsigned char, s.str[i]);
      fnd->tbl[at] = n - i;
    }
  }
}
static int
str_fnd_tbl_str(struct str hay, struct str needle, struct str_fnd_tbl *fnd) {
  if (needle.len > hay.len) {
    return hay.len;
  }
  if (needle.len == 1) {
    char *res = memchr(hay.str, needle.str[0], cast(size_t, hay.len));
    return res ? cast(int, res - hay.str) : hay.len;
  }
  int hpos = 0;
  int n = needle.len - 1;
  char last_char = needle.str[n];
  while (hpos <= hay.len - needle.len) {
    int c = hay.str[hpos + n];
    if (last_char == c &&
        !memcmp(needle.str, hay.str + hpos, cast(size_t, n))) {
      return hpos;
    }
    hpos += fnd->tbl[c];
  }
  return hay.len;
}
static int
str_fnd_tbl_has(struct str hay, struct str needle, struct str_fnd_tbl *fnd) {
  return str_fnd_tbl_str(hay, needle, fnd) >= hay.len;
}
static int
str__fnd_sse(struct str hay, struct str needle) {
  static const char unsigned ovr_msk[32] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
  };
  chr16 ndl = chr16_ld(needle.str);
  chr16 ovr = chr16_ld(ovr_msk + 16 - (needle.len & 15));
  for (int i = 0; i + needle.len <= hay.len; i++) {
    chr16 txt = chr16_ld(hay.str + i);
    chr16 ceq = chr16_eq(ndl,txt);
    chr16 msk = chr16_or(ceq,ovr);
    if (chr16_tst_all_ones(msk)) {
      return i;
    }
  }
  return hay.len;
}
static int
str_fnd(struct str hay, struct str needle) {
  if (needle.len == 1) {
    char *res = memchr(hay.str, needle.str[0], cast(size_t, hay.len));
    return res ? cast(int, res - hay.str) : hay.len;
  } else if (needle.len <= 16) {
    return str__fnd_sse(hay, needle);
  } else {
    struct str_fnd_tbl fnd;
    str_fnd_tbl(&fnd, needle);
    return str_fnd_tbl_str(hay, needle, &fnd);
  }
}
static int
str_has(struct str hay, struct str needle) {
  return str_fnd(hay, needle) >= hay.len;
}
static struct str
str_split_cut(struct str *s, struct str delim) {
  int p = str_fnd(*s, delim);
  if (p < s->len) {
    struct str res = str_lhs(*s, p);
    str_cut_lhs(s, p + 1);
    return res;
  } else {
    struct str res = *s;
    *s = str_nil;
    return res;
  }
}
static int
str_fzy(struct str s, struct str p) {
  const char *pat = p.str;
  const char *str = s.str;

  int run = 1;
  int score = 0;
  int remain = 0;
  for (; str < s.end && pat < p.end; str++) {
    // clang-format off
    while (*str == ' ' && str < s.end) {str++;}
    while (*pat == ' ' && pat < p.end) {pat++;}
    // clang-format on
    if (to_lower(*str) == to_lower(*pat)) {
      score += run; run++; pat++;
    } else {
      score--; run = 1;
    }
  }
  remain = cast(int, s.end - str);
  int val = score + remain + cast(int, s.str - str);
  int left = cast(int, p.end - pat);
  return cast(int, val *left - remain);
}
static int
str__match_here(struct str reg, struct str txt) {
  if (!reg.len) return 1;
  if (reg.len > 1 && reg.str[1] == '*')
    return str__match_star(reg.str[0], str_rhs(reg,2), txt);
  if (reg.str[0] == '$' && reg.len == 2)
    return txt.len == 0;
  if (reg.str[0] == '?' || reg.str[0] == txt.str[0])
    return str__match_here(str_rhs(reg,1), str_rhs(txt,1));
  return 0;
}
static int
str__match_star(int c, struct str reg, struct str txt) {
  do {/* a '* matches zero or more instances */
    if (str__match_here(reg, txt))
      return 1;
    txt = str_rhs(txt,1);
  } while (txt.len && (txt.str[0] == c || c == '?'));
  return 0;
}
static int
str_regex(struct str txt, struct str reg) {
  /*
  c    matches any literal character c
  ?    matches any single character
  ^    matches the beginning of the input string
  $    matches the end of the input string
  *    matches zero or more occurrences of the previous character */
  if (reg.len && reg.str[0] == '^') {
    return str__match_here(str_rhs(reg,1), txt);
  }
  do {/* must look even if string is empty */
    if (str__match_here(reg, txt)) {
      return 1;
    }
    txt = str_rhs(txt,1);
  } while (txt.len);
  return 0;
}
static void
ut_str(struct sys *sys) {
  unused(sys);
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
  assert(close_no == hay.len);
  int no = str_fnd(hay, strv("rock"));
  assert(no == hay.len);
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
#define for_utf(rune, it, rest, src)\
  for (struct str rest = src, it = utf_dec(rune, &rest); it.len;\
      it = utf_dec(rune, &rest))
#define for_utf_rev(rune, it, rest, src)\
  for (struct str rest = src, it = utf_dec_rev(rune, &rest); it.len;\
      it = utf_dec_rev(rune, &rest))

static struct str
utf_dec(unsigned *rune, struct str *s) {
  assert(s);
  if (!s->len) {
    if (rune) *rune = UTF_INVALID;
    return strp(s->end, s->end);
  }
  int n = 0;
  unsigned res = 0;
  const char *p = s->str;
  switch (*p & 0xf0) {
    // clang-format off
    case 0xf0: res = (*p & 0x07), n = 3; break;
    case 0xe0: res = (*p & 0x0f), n = 2; break;
    case 0xc0: res = (*p & 0x1f), n = 1; break;
    case 0xd0: res = (*p & 0x1f), n = 1; break;
    default:   res = (*p & 0xff), n = 0; break;
    // clang-format on
  }
  if (s->str + n + 1 > s->end) {
    if (rune) *rune = UTF_INVALID;
    *s = strp(s->end, s->end);
    return *s;
  }
  struct str view = str(p, n + 1);
  for (int i = 0; i < n; ++i) {
    res = (res << 6) | (*(++p) & 0x3f);
  }
  if (rune) {
    *rune = res;
  }
  *s = strp(s->str + n + 1, s->end);
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
  const char *p = s->end;
  while (p > s->str) {
    char c = *(--p);
    if (utf_tst(c)) {
      struct str r = strp(p, s->end);
      struct str it = utf_dec(rune, &r);
      *s = strp(s->str, p);
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
  for_utf(&glyph, it, _, s) {
    if (i >= idx) {
      if (rune) *rune = glyph;
      return it;
    }
    i++;
  }
  if (rune)
    *rune = UTF_INVALID;
  return strp(s.end, s.end);
}
static int
utf_at_idx(struct str s, int idx) {
  struct str view = utf_at(0, s, idx);
  if (view.len) {
    return cast(int, view.str - s.str);
  }
  return s.len;
}
static int
utf_len(struct str s) {
  int i = 0;
  unsigned rune = 0;
  for_utf(&rune, _, __, s) {
    i++;
  }
  return i;
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
#define KB(n) (1024 * n)
#define MB(n) (1024 * KB(n))
#define GB(n) (1024 * MB(n))

#define ARENA_ALIGNMENT 8
#define ARENA_BLOCK_SIZE KB(64)

#define arena_obj(a, s, T) cast(T*, arena_alloc(a, s, szof(T)))
#define arena_arr(a, s, T, n) cast(T*, arena_alloc(a, s, szof(T) * n))
#define arena_dyn(a, s, T, n) \
  cast(T*, dyn__static(arena_alloc(a, s, dyn_req_siz(sizeof(T) * (n))), (n)))
#define arena_set(a, s, n) arena_dyn(a, s, unsigned long long, n)
#define arena_tbl(a, s, T, n)\
  cast(T*, tbl__setup(arena_alloc(a, s, tbl__resv(szof(T), n)), 0, n))

#define scp_mem(a,s,sys)\
  for (int uniqid(_i_) = (mem_scp_begin(s,a), 0); uniqid(_i_) < 1;\
    uniqid(_i_) = (mem_scp_end(s,a,sys), 1))

static int
arena_align_off(struct arena *a, int align) {
  intptr_t res = (intptr_t)a->blk->base + a->blk->used;
  int msk = align - 1;
  int off = 0;
  if(res & msk) {
    off = cast(int, (align - (res & msk)));
  }
  return off;
}
static int
arena_size_for(struct arena *a, int size_init) {
  int off = arena_align_off(a, ARENA_ALIGNMENT);
  return size_init + off;
}
static void*
arena_alloc(struct arena *a, struct sys *sys, int size_init) {
  assert(a);
  assert(sys);

  int siz = 0;
  void *res = 0;
  if (a->blk) {
    siz = arena_size_for(a, size_init);
  }
  if (!a->blk || ((a->blk->used + siz) > a->blk->size)) {
    siz = size_init; /* allocate new block */
    int blksiz = max(!a->blksiz ? ARENA_BLOCK_SIZE : a->blksiz, siz);
    struct mem_blk *blk = sys->mem.alloc(0, blksiz, 0, 0);
    blk->prv = a->blk;
    a->blk = blk;
  }
  assert((a->blk->used + siz) <= a->blk->size);
  int align_off = arena_align_off(a, ARENA_ALIGNMENT);
  int off = a->blk->used + align_off;
  res = a->blk->base + off;
  a->blk->used += siz;

  assert(siz >= size_init);
  assert(a->blk->used <= a->blk->size);
  memset(res, 0, cast(size_t, size_init));
  return res;
}
static void *
arena_cpy(struct arena *a, struct sys *sys, const void *src, int siz) {
  assert(a);
  assert(src);
  assert(siz > 0);

  void *dst = arena_alloc(a, sys, siz);
  memcpy(dst, src, cast(size_t, siz));
  return dst;
}
static struct str
arena_fmt(struct arena *a, struct sys *sys, const char *fmt, ...) {
  assert(a);
  assert(fmt);

  va_list args;
  va_start(args, fmt);
  int n = fmtvsn(0, 0, fmt, args);
  va_end(args);

  char *res = arena_alloc(a, sys, n + 1);
  va_start(args, fmt);
  fmtvsn(res, n + 1, fmt, args);
  va_end(args);
  return str(res, n);
}
static char *
arena_cstr(struct arena *a, struct sys *sys, struct str cs) {
  assert(a);
  assert(sys);

  char *s = arena_alloc(a, sys, cs.len + 1);
  memcpy(s, cs.str, cast(size_t, cs.len));
  s[cs.len] = 0;
  return s;
}
static struct str
arena_str(struct arena *a, struct sys *sys, struct str cs) {
  assert(a);
  char *s = arena_cstr(a, sys, cs);
  return str(s, cs.len);
}
static void
arena_free_last_blk(struct arena *a, struct sys *s) {
  assert(a);
  assert(s);

  struct mem_blk *blk = a->blk;
  a->blk = blk->prv;
  s->mem.free(blk);
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
static void*
arena_boot(struct arena *a, struct sys *s, int obj_siz, int arena_off) {
  assert(a);
  assert(s);

  struct arena boot = {0};
  void *obj = arena_alloc(a, s, obj_siz);
  *(struct arena*)(void*)((unsigned char*)obj + arena_off) = boot;
  return obj;
}
static int
mem_scp_begin(struct mem_scp *s, struct arena *a) {
  assert(a);
  assert(s);

  s->blk = a->blk;
  s->used = a->blk ? a->blk->used : 0;
  a->tmp_cnt++;
  return 1;
}
static int
mem_scp_end(struct mem_scp *s, struct arena *a, struct sys *sys) {
  assert(s);
  assert(a);
  assert(sys);
  while (a->blk != s->blk) {
    arena_free_last_blk(a, sys);
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
#define for_lst(e,l)  for((e) = (l)->nxt; (e) != (l); (e) = (e)->nxt)
#define for_lst_safe(a,b,l) \
    for((a) = (l)->nxt, (b) = (a)->nxt; (a) != (l); (a) = (b), (b) = (a)->nxt)
#define for_lst_rev(e,l) for((e) = (l)->prv; (e) != (l); (e) = (e)->prv)
#define for_lst_rev_safe(a,b,l) \
    for((a) = (l)->prv, (b) = (a)->prv; (a) != (l); (a) = (b), (b) = (a)->prv)
// clang-format on

/* ---------------------------------------------------------------------------
 *                                  Array
 * ---------------------------------------------------------------------------
 */
#define DYN_ALIGN 16
struct dyn_hdr {
  struct mem_blk *blk;
  int cnt, cap;
  char buf[];
};
// clang-format off
#define dyn__hdr(b) ((struct dyn_hdr *)(void *)((char *)(b)-offsetof(struct dyn_hdr, buf)))
#define dyn__fits(b, n) (dyn_cnt(b) + (n) <= dyn_cap(b))
#define dyn_req_siz(type_size) (szof(struct dyn_hdr) + (type_size) + DYN_ALIGN)
#define dyn_fix_siz(type,cnt) dyn_req_siz(sizeof(type) * cnt)
#define dyn__add(b, s, x) (dyn_fit((b), (s), 1), (b)[dyn__hdr(b)->cnt++] = x)

#define dyn_cnt(b) ((b) ? dyn__hdr(b)->cnt : 0)
#define dyn_cap(b) ((b) ? abs(dyn__hdr(b)->cap) : 0)
#define dyn_begin(b) ((b) + 0)
#define dyn_empty(b) (dyn_cnt(b) == 0)
#define dyn_any(b) (dyn_cnt(b) > 0)
#define dyn_end(b) ((b) + dyn_cnt(b))
#define dyn_fit(b, s, n) (dyn__fits((b), (n)) ? 0:((b) = dyn__grow((b), (s), dyn_cnt(b) + (n), szof(*(b)))))
#define dyn_add(b, s, ...) dyn__add(b, s, (__VA_ARGS__))
#define dyn_pop(b) ((b) ? dyn__hdr(b)->cnt = max(0, dyn__hdr(b)->cnt - 1): 0)
#define dyn_clr(b) ((b) ? dyn__hdr(b)->cnt = 0 : 0)
#define dyn_del(b, i) (dyn_cnt(b) ? (b)[i] = (b)[--dyn__hdr(b)->cnt] : 0);
#define dyn_rm(a, i) (dyn_cnt(a) ? memmove(&((a)[i]), &((a)[i+1]),(size_t)((--(dyn__hdr(a)->cnt)) - i) * sizeof((a)[0])) :0)
#define dyn_fmt(b, s, fmt, ...) ((b) = dyn__fmt((b), (s), (fmt), __VA_ARGS__))
#define dyn_free(b,s) ((!(b))?0:(dyn__hdr(b)->cap <= 0) ? (b) = 0 : ((s)->mem.free(dyn__hdr(b)->blk), (b) = 0))
#define dyn_str(b) str(dyn_begin(b), dyn_cnt(b))
#define dyn_sort(b,f) ((b) ? qsort(b, cast(size_t, dyn_cnt(b)), sizeof(b[0]), f), 0 : 0)
#define dyn_asn_str(b,sys, s) dyn_asn(b,sys,(s).str,(s).len)
#define dyn_val(b,i) assert(i < dyn_cnt(b))

#define fori_dyn(i,c)\
  for (int i = 0; i < dyn_cnt(c); ++i)
#define for_dyn(it,c) for_arr(it, c, dyn_cnt(c))
#define for_dyn_rng(it,a, b,e,s)                \
  for ((it) = (a) + rng(b,e,s,dyn_cnt(a)).lo;   \
       (it) != (a) + rng(b,e,s,dyn_cnt(a)).hi;  \
       (it) += rng(b,e,s,dyn_cnt(a)).step)

#define dyn_asn(b, s, x, n) do {                  \
  dyn_clr(b);                                   \
  dyn_fit(b, s, n);                             \
  memcpy(b, x, sizeof((b)[0]) * cast(size_t, (n))); \
  dyn__hdr(b)->cnt = (n);                       \
} while (0)

#define dyn_put(b, s, i, x, n) do {                                       \
  dyn_fit(b, s, n);                                                     \
  assert((i) <= dyn_cnt(b));                                            \
  assert((i) < dyn_cap(b));                                             \
  memmove((b) + (i) + (n), (b) + (i), sizeof((b)[0]) * (size_t)max(0, dyn_cnt(b) - (i))); \
  memcpy((b) + (i), (x), sizeof((b)[0]) * cast(size_t, (n)));           \
  dyn__hdr(b)->cnt += (n);                                              \
} while (0)

#define dyn_cut(b, i, n) do {                                             \
  assert((i) < dyn_cnt(b));                                             \
  assert((i) + (n) <= dyn_cnt(b));                                      \
  memmove((b) + (i), b + (i) + (n), sizeof((b)[0]) * (size_t)max(0, dyn_cnt(b) - ((i) + (n)))); \
  dyn__hdr(b)->cnt -= (n);                                              \
} while (0)
// clang-format on

static void *
dyn__grow(void *buf, struct sys *sys, int new_len, int elem_size) {
  struct dyn_hdr *hdr = 0;
  int cap = dyn_cap(buf);
  int new_cap = max(32, max(2 * cap + 1, new_len));
  int new_size = offsetof(struct dyn_hdr, buf) + new_cap * elem_size;
  assert(new_len <= new_cap);
  if (!buf) {
    /* allocate new array */
    struct mem_blk *blk = sys->mem.alloc(0, new_size, SYS_MEM_GROWABLE, 0);
    hdr = cast(struct dyn_hdr*, (void*)blk->base);
    hdr->blk = blk;
    hdr->cnt = 0;
  } else if (dyn__hdr(buf)->cap < 0) {
    /* static memory so allocate and copy to heap */
    struct mem_blk *blk = sys->mem.alloc(0, new_size, SYS_MEM_GROWABLE, 0);
    hdr = cast(struct dyn_hdr*, (void*)blk->base);
    hdr->blk = blk;
    hdr->cnt = dyn_cnt(buf);
    memcpy(hdr->buf, dyn__hdr(buf)->buf, (size_t)(cap * elem_size));
  } else {
    /* grow array */
    int n = dyn_cnt(buf);
    hdr = dyn__hdr(buf);

    struct mem_blk *blk = sys->mem.alloc(hdr->blk, new_size, SYS_MEM_GROWABLE, 0);
    hdr = cast(struct dyn_hdr*, (void*)blk->base);
    hdr->blk = blk;
    hdr->cnt = n;
  }
  hdr->cap = new_cap;
  return hdr->buf;
}
static void *
dyn__static(void *buf, int n) {
  void *u = (char *)buf + DYN_ALIGN + sizeof(struct dyn_hdr) - 1;
  void *a = align_down_ptr(u, DYN_ALIGN);
  void *h = (char *)a - sizeof(struct dyn_hdr);

  struct dyn_hdr *hdr = h;
  hdr->cap = -n, hdr->cnt = 0;
  return hdr->buf;
}
static char *
dyn__fmt(char *buf, struct sys *sys, const char *fmt, ...) {
  assert(buf);
  assert(fmt);

  int cap, n;
  va_list args;
  va_start(args, fmt);
  cap = dyn_cap(buf) - dyn_cnt(buf);
  n = 1 + fmtvsn(dyn_end(buf), cap, fmt, args);
  va_end(args);

  if (n > cap) {
    dyn_fit(buf, sys, n + dyn_cnt(buf));
    va_start(args, fmt);
    int new_cap = dyn_cap(buf) - dyn_cnt(buf);
    n = 1 + fmtvsn(dyn_end(buf), new_cap, fmt, args);
    assert(n <= new_cap);
    va_end(args);
  }
  dyn__hdr(buf)->cnt += n - 1;
  return buf;
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
static dyn(char)
path_push(dyn(char) path, struct sys *sys, struct str src) {
  char *p = dyn_end(path);
  while (p != path && p[-1] == '/') {
    dyn_pop(path);
  }
  if (src.str[0] == '/') {
    src = str_rhs(src, 1);
  }
  dyn_fmt(path, sys, "/%.*s", strf(src));
  return path;
}
static struct str
path_file(struct str path) {
  for (const char *p = path.end; p > path.str; --p) {
    if (p[-1] == '/') {
      return strp(p, path.end);
    }
  }
  return path;
}
static struct str
path_ext(struct str path) {
  for (const char *p = path.end; p > path.str; --p) {
    if (p[-1] == '.') {
      return strp(p, path.end);
    }
  }
  return str_nil;
}

/* ---------------------------------------------------------------------------
 *                                  Set
 * ---------------------------------------------------------------------------
 */
#define SET_MIN_SIZE 64
#define SET_GROW_FACTOR 2.8f
#define SET_FULL_PERCENT 0.85f
#define SET_MIN_GROW_PERCENT 2.0f

// clang-format off
#define set__is_del(k) (((k) >> 63) != 0)
#define set__dist(h,n,i) (((i) + (n) - ((h) % n)) % (n))
#define set__fits(t,cnt) ((cnt) < (int)((float)set_cap(t) * SET_FULL_PERCENT))
#define set__fit(t,s,n) (set__fits(t, n) ? 0 : ((t) = set__grow(t, s, n)))
#define set__add(t,s,k) (set__fit(t, s, set_cnt(t) + 1), set__put(t, k))
#define set__key(k) ((k) != 0u && !set__is_del(k))
static void set__swapi(unsigned long long *a, unsigned long long *b) {unsigned long long t = *a; *a = *b, *b = t;}

#define set_cnt(s) dyn_cnt(s)
#define set_cap(s) dyn_cap(s)
#define set_fnd(s,k) (set__fnd(s, k, set_cap(s)) < set_cap(s))
#define set_put(t,s,k) ((!(t) || !set_fnd(t, k)) ? set__add(t, s, k) : set_cap(t))
#define set_del(s,k) set__del(s, k, set_cap(s))
#define set_free(s,sys) dyn_free(s, sys)
#define set_clr(s) do {((s) ? memset(s,0,sizeof(unsigned long long) * (size_t)dyn_cap(s)) : 0); dyn_clr(s);} while(0)
// clang-format on

static inline unsigned long long
set__hash(unsigned long long k) {
  unsigned long long h = k & 0x7fffffffffffffffllu;
  return h | (h == 0);
}
static int
set__new_cap(int old, int req) {
  int nn = max(SET_MIN_SIZE, (int)((float)req * SET_MIN_GROW_PERCENT));
  return max(nn, cast(int, SET_GROW_FACTOR * cast(float, old)));
}
static unsigned long long*
set_new(int old, int req, struct sys *sys) {
  int cap = set__new_cap(old, req);
  return dyn__grow(0, sys, cap, sizeof(unsigned long long));
}
static void
set__swap(void *a, void *b, void *tmp, int siz) {
  memcpy(tmp, a, siz);
  memcpy(a, b, siz);
  memcpy(b, tmp, siz);
}
static inline long long
set__store(unsigned long long *keys, void *vals,
           unsigned long long i, unsigned long long h,
           void* val, int val_siz) {
  keys[i] = h;
  if (vals) {
    unsigned long long off = (unsigned long long)val_siz * i;
    memcpy((unsigned char*)vals + off, val, val_siz);
  }
  return cast(long long, i);
}
static long long
set__slot(unsigned long long *keys, void *vals, int cap,
          unsigned long long h, void *v, int val_siz) {
  unsigned long long n = cast(unsigned long long, cap);
  unsigned long long i = h % n, b = i, dist = 0;
  do {
    unsigned long long k = keys[i];
    if (!k) return set__store(keys, vals, i, h, v, val_siz);
    unsigned long long d = set__dist(k, n, i);
    if (d++ > dist++) continue;
    if (set__is_del(k)) {
      return set__store(keys, vals, i, h, v, val_siz);
    }
    set__swapi(&h, &keys[i]);
    if (vals) {
      void *tmp_val = (unsigned char*)vals + cap * val_siz;
      void *cur_val = (unsigned char*)vals + i * (unsigned long long)val_siz;
      set__swap(cur_val, v, tmp_val, val_siz);
    }
    dist = d;
  } while ((i = ((i + 1) % n)) != b);
  return cast(long long, n);
}
static intptr_t
set__put(unsigned long long *set, unsigned long long key) {
  assert(set);
  unsigned long long h = set__hash(key);
  long long i = set__slot(set, 0, set_cap(set), h, 0, 0);
  if (i < set_cap(set)) {
    assert(i < set_cap(set));
    dyn__hdr(set)->cnt++;
  }
  return cast(intptr_t, i);
}
static unsigned long long*
set__grow(unsigned long long *old, struct sys *sys, int n) {
  unsigned long long *set = set_new(set_cap(old), n, sys);
  for (int i = 0; i < set_cap(old); ++i) {
    assert(i < set_cap(old));
    if (set__key(old[i])) {
      set__put(set, old[i]);
    }
  }
  dyn_free(old, sys);
  return set;
}
static long long
set__fnd(unsigned long long *set, unsigned long long key, int cap) {
  assert(set);
  unsigned long long h = set__hash(key);
  unsigned long long n = cast(unsigned long long, cap);
  unsigned long long i = h % n, b = i, dist = 0;
  do {
    if (!set[i] || dist > set__dist(set[i],n,i)) {
      return cap;
    } else if(set[i] == h) {
      return cast(long long, i);
    }
    dist++;
  } while ((i = ((i + 1) % n)) != b);
  return cap;
}
static long long
set__del(unsigned long long* set, unsigned long long key, int cap) {
  assert(set);
  if (!cap) {
    return cap;
  }
  long long i = set__fnd(set, key, cap);
  if (i < cap) {
    assert(i < set_cap(set));
    dyn__hdr(set)->cnt--;
    set[i] |= 0x8000000000000000llu;
  }
  return i;
}
static void
ut_set(struct sys *sys) {
  {
    unsigned long long *set = 0;
    unsigned long long h = STR_HASH8("Command");

    long long at = set_put(set, sys, h);
    assert(set != 0);
    assert(set_cnt(set) == 1);
    assert(set_cap(set) > 0);
    assert(set[at] == set__hash(h));

    int has = set_fnd(set, h);
    assert(has == 1);

    has = set_fnd(set, h ^ 0x1235);
    assert(has == 0);

    has = set_fnd(set, h + 1);
    assert(has == 0);

    set_del(set, h);
    assert(set_cnt(set) == 0);
    assert(set_cap(set) > 0);
    assert(set[at] == (set__hash(h)|0x8000000000000000llu));

    has = set_fnd(set, h);
    assert(has == 0);

    long long at2 = set_put(set, sys, h);
    assert(set_cnt(set) == 1);
    assert(set_cap(set) > 0);
    assert(at == at2);
    assert(set[at2] == set__hash(h));

    set_free(set, sys);
    assert(set == 0);
    assert(set_cnt(set) == 0);
    assert(set_cap(set) == 0);
  }
  struct arena a = {0};
  {
    unsigned long long *set = arena_set(&a, sys, 64);

    unsigned long long h = STR_HASH8("Command");
    long long at = set_put(set, sys, h);
    assert(set != 0);
    assert(set_cnt(set) == 1);
    assert(set_cap(set) > 0);
    assert(set[at] == set__hash(h));

    int has = set_fnd(set, h);
    assert(has == 1);

    has = set_fnd(set, h ^ 0x1235);
    assert(has == 0);

    has = set_fnd(set, h + 1);
    assert(has == 0);

    set_del(set, h);
    assert(set_cnt(set) == 0);
    assert(set_cap(set) > 0);
    assert(set[at] == (set__hash(h)|0x8000000000000000llu));

    long long at2 = set_put(set, sys, h);
    assert(set_cnt(set) == 1);
    assert(set_cap(set) > 0);
    assert(at == at2);
    assert(set[at2] == set__hash(h));

    has = set_fnd(set, h);
    assert(has == 1);

    set_free(set, sys);
    assert(set == 0);
    assert(set_cnt(set) == 0);
    assert(set_cap(set) == 0);
  }
  arena_free(&a, sys);
}

/* ---------------------------------------------------------------------------
 *                                  Table
 * ---------------------------------------------------------------------------
 */
// clang-format off
#define tbl__fits(t,n) ((n) < (int)(((float)tbl_cap(t)) * SET_FULL_PERCENT))
#define tbl__fit(t,s,n) (tbl__fits(t, n) ? 0 : ((t) = tbl__grow((t),s,szof(*(t)),n)))
#define tbl__add(t,s,k,v) (tbl__fit(t,s,tbl_cnt(t) + 1), tbl__put((t),k,v,szof(*(t))))
#define tbl__key(k) set__key(k)
#define tbl__val(vals,i,s) (cast(unsigned char*, vals) + s * i)
#define tbl__is_uniq(t,k) (!tbl_cap(t)|| !tbl__fnd(t,k,szof(*(t))))
#define tbl__chk_val(t,v) if (0){(t)[tbl_cap(t)]=*(v);}
#define tbl__keys(t,s) ((unsigned long long*)align_down_ptr(((unsigned char*)(void*)(t)) + ((dyn__hdr(t))->cap)*(s) + 16,16))

#define tbl_cnt(t) dyn_cnt(t)
#define tbl_cap(t) dyn_cap(t)
#define tbl_any(t) (tbl_cnt(t) > 0)
#define tbl_empty(t) (tbl_cnt(t) == 0)
#define tbl_put(t,s,k,v) do {if (tbl__is_uniq(t,k)) {tbl__add(t,s,k,v); tbl__chk_val(t,v);}} while(0)
#define tbl_fnd(t,k) (!tbl_cnt(t)) ? 0: tbl__fnd(t, k, szof(*(t)))
#define tbl_has(t,k) (tbl_fnd(t, k) != 0)
#define tbl_del(t,k) tbl__del(t, k, szof(*(t)))
#define tbl_clr(t) do{dyn__hdr(t)->cnt = 0; memset(tbl__keys(t,szof(*(t))), 0, tbl_cap(t) * szof(unsigned long long));} while(0)
#define tbl_free(t,s) do {if(tbl_cap(t)){(s)->mem.free(dyn__hdr(t)->blk); (t) = 0;}} while(0)
#define for_tbl(n,i,t) for (int n = tbl__nxt_idx(t,0,szof(*(t))), i = 0; n < tbl_cap(t); n = tbl__nxt_idx(t,n+1,szof(*(t))),++i)
// clang-format on

static int
tbl__nxt_idx(void *tbl, int i, int type_siz) {
  int cnt = tbl_cap(tbl);
  unsigned long long *keys = tbl__keys(tbl,type_siz);
  for (; i < cnt; ++i) {
    if (tbl__key(keys[i])) {
      return i;
    }
  }
  return cnt;
}
static void*
tbl__put(void *tbl, unsigned long long key, void *val, int val_siz) {
  unsigned long long h = set__hash(key);
  unsigned long long *keys = tbl__keys(tbl, val_siz);
  long long i = set__slot(keys, tbl, tbl_cap(tbl), h, val, val_siz);
  if (i < tbl_cap(tbl)) {
    dyn__hdr(tbl)->cnt++;
    return tbl__val(tbl,i,val_siz);
  }
  return 0;
}
static void*
tbl__fnd(void *tbl, unsigned long long key, int val_siz) {
  unsigned long long *keys = tbl__keys(tbl, val_siz);
  long long i = set__fnd(keys, key, tbl_cap(tbl));
  if (i >= tbl_cap(tbl)) {
    return 0;
  }
  return cast(unsigned char*, tbl) + i * val_siz;
}
static void
tbl__del(void *tbl, unsigned long long key, int val_siz) {
  if (!tbl_cap(tbl)) return;
  unsigned long long *keys = tbl__keys(tbl, val_siz);
  long long i = set__fnd(keys, key, tbl_cap(tbl));
  if(i < tbl_cap(tbl)) {
    keys[i] |= 0x8000000000000000llu;
    dyn__hdr(tbl)->cnt--;
  }
}
static int
tbl__resv(int val_siz, int cap) {
  int siz = szof(unsigned long long) * cap + val_siz * (cap + 1);
  return siz + szof(struct dyn_hdr) + 16;
}
static void*
tbl__setup(void *mem, struct mem_blk *blk, int cap) {
  struct dyn_hdr *hdr = cast(struct dyn_hdr*, mem);
  hdr->blk = blk;
  hdr->cap = cap;
  hdr->cnt = 0;
  return hdr + 1;
}
static void*
tbl__grow(void *tbl, struct sys *sys, int val_siz, int n) {
  int cap = set__new_cap(tbl_cap(tbl), n);
  int siz = tbl__resv(val_siz, cap);
  struct mem_blk *blk = sys->mem.alloc(0, siz, SYS_MEM_GROWABLE, 0);
  void *new_tbl = tbl__setup(blk->base, blk, cap);

  unsigned long long *keys = tbl__keys(new_tbl,val_siz);
  for (int i = 0; i < tbl_cap(tbl); ++i) {
    if (set__key(keys[i])) {
      void *val = tbl__val(tbl,i,val_siz);
      tbl__put(new_tbl, keys[i], val, val_siz);
    }
  }
  if (tbl && dyn__hdr(tbl)->blk) {
    sys->mem.free(dyn__hdr(tbl)->blk);
  }
  return new_tbl;
}
static void
ut_tbl(struct sys *sys) {
  static const unsigned long long h = STR_HASH8("Command");
  int val = 1337;

  tbl(int) t = 0;
  int *v = tbl_fnd(t, h);
  assert(v == 0);

  tbl_put(t, sys, h, &val);
  assert(tbl_cnt(t) == 1);
  assert(tbl_cap(t) == 64);

  v = tbl_fnd(t, h);
  assert(v);
  assert(*v == val);

  v = tbl_fnd(t, h + 1);
  assert(v == 0);

  tbl_del(t, h);
  assert(tbl_cnt(t) == 0);
  assert(tbl_cap(t) > 0);

  tbl_put(t, sys, h, &val);
  assert(tbl_cnt(t) == 1);
  assert(tbl_cap(t) > 0);

  tbl_free(t, sys);
  assert(tbl_cnt(t) == 0);
  assert(tbl_cap(t) == 0);
}

/* ---------------------------------------------------------------------------
 *                                  Sort
 * ---------------------------------------------------------------------------
 */
// clang-format off
typedef unsigned(*sort_conv_f)(const void *p);
typedef void*(sort_access_f)(const void *data, void *usr);

#define sort_int(rnk, s, mem, a, siz, n, off) sort__base_mem(rnk, s, mem, a, siz, n, off, 0, 0, sort__cast_int)
#define sort_uint(rnk, s, mem, a, siz, n, off) sort__base_mem(rnk, s, mem, a, siz, n, off, 0, 0, sort__cast_uint)
#define sort_flt(rnk, s, mem, a, siz, n, off) sort__base_mem(rnk, s, mem, a, siz, n, off, 0, 0, sort__cast_flt)
#define sort_str(tmp, tmp2, tmp3, a, n, siz, off) sort__str(tmp, tmp2, tmp3, a, n, siz, off, 0, 0)

#define sort_int_cb(rnk, s, mem, a, siz, n, off, cb, usr) sort__base_mem(rnk, s, mem, a, siz, n, off, usr, cb, sort__cast_int)
#define sort_uint_cb(rnk, s, mem, a, siz, n, off, cb, usr) sort__base_mem(rnk, s, mem, a, siz, n, off, usr, cb, sort__cast_uint)
#define sort_flt_cb(rnk, s, mem, a, siz, n, off, cb, usr) sort__base_mem(rnk, s,, mem, a, siz, n, off, usr, cb, sort__cast_flt)
#define sort_str_cb(tmp, tmp2, tmp3, a, n, siz, off, cb, usr) sort__str(tmp, tmp2, tmp3, a, n, siz, off, usr, cb)

#define sort__access(a,usr,access,conv,off) ((access) ? (conv)((access)(a + off, usr)) : (conv)(a + off))
#define sort__char_at(s,d) (((d) < (s)->len) ? (s)->str[d] : -1)
#define sort__str_get(a,access,usr) (struct str*)((access) ? (access(a, usr)) : (a))
static inline unsigned sort__cast_uint(const void *p) {return *(const unsigned*)p;}
static inline unsigned sort__cast_int(const void *p) {union bit_castu {int i; unsigned u;} v = {.i = *(const int*)p}; return v.u ^ (1u << 31u);}
static inline unsigned sort__cast_flt(const void *p) {union bit_castu {float f; unsigned u;} v = {.f = *(const float*)p}; if ((v.u >> 31u) == 1u) {v.u *= (unsigned)-1; v.u ^= (1u << 31u);}return v.u ^ (1u << 31u);}
// clang-format on

static void
sort_q3(int *rnk, void *arr, int siz, int n, int off, int lo, int hi,
        void *usr, sort_access_f access, sort_conv_f conv) {
  int i, j, l = lo, h = hi;
  unsigned char *a = arr;
  if (lo >= hi) return;
  if (hi - lo <= 1) {
    unsigned alo = sort__access(a + rnk[lo] * siz, usr, access, conv, off);
    unsigned ahi = sort__access(a + rnk[hi] * siz, usr, access, conv, off);
    if (ahi < alo) {
      int tmp = rnk[lo];
      rnk[lo] = rnk[hi];
      rnk[hi] = tmp;
    }
    i = lo, j = hi;
  } else {
    int mid = lo;
    unsigned pivot = sort__access(a + rnk[hi] * siz, usr, access, conv, off);
    while (mid <= hi) {
      unsigned amid = sort__access(a + rnk[mid] * siz, usr, access, conv, off);
      if (amid < pivot) {
        int tmp = rnk[lo]; rnk[lo++] = rnk[mid]; rnk[mid++] = tmp;
      } else if (amid == pivot) {
        mid++;
      } else if (amid > pivot) {
        int tmp = rnk[mid]; rnk[mid] = rnk[hi]; rnk[hi--] = tmp;
      }
    }
    i = lo - 1, j = mid;
  }
  sort_q3(rnk, arr, siz, n, off, l, i, usr, access, conv);
  sort_q3(rnk, arr, siz, n, off, j, h, usr, access, conv);
}
static inline int*
sort_radix(int *rnk, int *rnk2, void *a, int siz, int n, int off,
           void *usr, sort_access_f access, sort_conv_f conv) {
  enum sort_defs {
    SORT_RADIX_BITS = 8,
    SORT_RADIX_SIZE = (1 << SORT_RADIX_BITS),
    SORT_RADIX_MASK = (SORT_RADIX_SIZE-1),
    SORT_MAX_PASSES = 4,
    SORT_H0_OFF     = (SORT_RADIX_SIZE*0),
    SORT_H1_OFF     = (SORT_RADIX_SIZE*1),
    SORT_H2_OFF     = (SORT_RADIX_SIZE*2),
    SORT_H3_OFF     = (SORT_RADIX_SIZE*3),
  };
  unsigned h[SORT_RADIX_SIZE * SORT_MAX_PASSES] = {0};
  unsigned *lnk[SORT_RADIX_SIZE];
  {
    /* build histogram for all passes */
    unsigned char *p = a;
    unsigned *h0 = &h[SORT_H0_OFF];
    unsigned *h1 = &h[SORT_H1_OFF];
    unsigned *h2 = &h[SORT_H2_OFF];
    unsigned *h3 = &h[SORT_H3_OFF];

    int is_sorted = 1;
    unsigned last = sort__access(p, usr, access, conv, off);
    for (int i = 0; i < n; ++i) {
      unsigned v = sort__access(p + rnk[i] * siz, usr, access, conv, off);
      if (v < last) is_sorted = 0;
      h0[(v >> (SORT_RADIX_BITS * 0)) & SORT_RADIX_MASK]++;
      h1[(v >> (SORT_RADIX_BITS * 1)) & SORT_RADIX_MASK]++;
      h2[(v >> (SORT_RADIX_BITS * 2)) & SORT_RADIX_MASK]++;
      h3[(v >> (SORT_RADIX_BITS * 3)) & SORT_RADIX_MASK]++;
      last = v;
    }
    if (is_sorted) {
      return rnk; /* already sorted so early out */
    }
  }
  unsigned char *p = a;
  for (unsigned i = 0; i < SORT_MAX_PASSES; ++i) {
    unsigned* pass_cnt = &h[i << SORT_RADIX_BITS];
    unsigned uniq_val = sort__access(p, usr, access, conv, off);
    unsigned slot = (uniq_val >> (i * SORT_RADIX_BITS)) & SORT_RADIX_MASK;
    if (pass_cnt[slot] == (unsigned)n) {
      continue; /* all values in pass are the same, so skip pass */
    }
    /* create offsets */
    lnk[0] = (unsigned*)rnk2;
    for (int j = 1; j < SORT_RADIX_SIZE; ++j) {
      lnk[j] = lnk[j-1] + pass_cnt[j-1];
    }
    /* perform radix sort */
    unsigned shift = i * SORT_RADIX_BITS;
    for (unsigned k = 0; k < cast(unsigned,n); ++k) {
      int id = rnk[k];
      unsigned v = sort__access(p + id * siz, usr, access, conv, off);
      unsigned d = (v >> shift) & SORT_RADIX_MASK;
      *lnk[d]++ = cast(unsigned,id);
    }
    int *tmp = rnk;
    rnk = rnk2;
    rnk2 = tmp;
  }
  return rnk;
}
static inline int*
sort__base(int *rnk, int *rnk2, void *a, int siz, int n, int off,
           void *usr, sort_access_f access, sort_conv_f conv) {
  for (int i = 0; i < n; ++i) rnk[i] = i;
  if (n < 64) {
    sort_q3(rnk, a, siz, n, off, 0, n-1, usr, access, conv);
    return rnk;
  } else return sort_radix(rnk, rnk2, a, siz, n, off, usr, access, conv);
}
static inline void
sort__base_mem(int *rnk, struct sys *s, struct arena *mem,
               void *a, int siz, int n, int off,
               void *usr, sort_access_f access, sort_conv_f conv) {
  struct mem_scp scp = {0};
  scp_mem(mem, &scp, s) {
    int *tmp = arena_arr(mem, s, int, n);
    if (sort__base(rnk, tmp, a, siz, n, off, usr, access, conv) != rnk) {
      for (int i = 0; i < n; ++i) {
        rnk[i] = tmp[i];
      }
    }
  }
}
static char
sort__str_at(unsigned char *p, int d, sort_access_f access, void *usr) {
  struct str * s = sort__str_get(p, access, usr);
  return sort__char_at(s,d);
}
static void
sort_str_q3s(int *rnk, void *a, int lo, int hi, int d, int siz, int off,
              sort_access_f access, void *usr) {
  if (hi <= lo) return;
  unsigned char *p = a;
  int lt = lo, gt = hi, i = lo + 1;
  int v = sort__str_at(p + rnk[lo] * siz + off, d, access, usr);
  while (i <= gt) {
    int t = sort__str_at(p + rnk[i] * siz + off, d, access, usr);
    if (t < v) {int tmp = rnk[lt]; rnk[lt++] = rnk[i]; rnk[i++] = tmp;}
    else if(t > v) {int tmp = rnk[i]; rnk[i] = rnk[gt]; rnk[gt--] = tmp;}
    else i++;
  }
  sort_str_q3s(rnk, a, lo, lt-1, d, siz, off, access, usr);
  if (v >= 0)  sort_str_q3s(rnk, a, lt, gt, d + 1, siz, off, access, usr);
  sort_str_q3s(rnk, a, gt+1, hi, d, siz, off, access, usr);
}
static int*
sort__str_base(int *r, int *r2, short *o, void *a, int n, int siz, int off,
               int lo, int hi, sort_access_f fn, void *u, int d) {
  unsigned char * p = a;
  if (n < 32) {
    sort_str_q3s(r, a, lo, hi, d, siz, off, fn, u);
    return r;
  }
  int c[257] = {0};
  for (int i = 0; i < n; ++i)
    o[i] = sort__str_at(p + r[i] * siz + off, d, fn, u);
  for (int i = 0; i < n; ++i) {
    ++c[o[i] + 1];
  }
  int idx[257];
  idx[0] = idx[1] = 0;
  for (int i = 1; i < 256; ++i)
    idx[i+1] = idx[i] + c[i];
  for (int i = 0; i < n; ++i)
    r2[idx[o[i]+1]++] = r[i];
  int *tmp = r; r = r2; r2 = tmp;

  int bsum = c[1];
  for (int i = 1; i < 256; ++i) {
    if (c[i + 1] == 0) continue;
    lo = bsum, hi = bsum + c[i+1]-1;
    int *res = sort__str_base(r, r2, o, a, c[i+1], siz, off, lo, hi, fn, u, d+1);
    if (res != r) tmp = r, r = r2, r2 = tmp;
    bsum += c[i+1];
  }
  return r;
}
static int*
sort__str(int *r, int *r2, short *o, void *a, int n, int siz, int off,
          sort_access_f fn, void *u, int d) {
  for (int i = 0; i < n; ++i) r[i] = i;
  return sort__str_base(r, r2, o, a, n, siz, off, 0, n-1, fn, u, d);
}
/* ---------------------------------------------------------------------------
 *                                  Search
 * ---------------------------------------------------------------------------
 */
static int
sorted_search(const void *vals, int cnt, int siz, void *val,
              int(*cmp_less)(const void *a, const void *b)) {
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
  return cast(int, (base - cast(const unsigned char*, vals)))/siz;
}

/* ---------------------------------------------------------------------------
 *                                  Image
 * ---------------------------------------------------------------------------
 */
#define img_w(img) (img)[-2]
#define img_h(img) (img)[-1]
#define img_pix(img, x, y) \
  (img)[cast(unsigned, (y)) * img_w(img) + cast(unsigned, (x))]

static unsigned *
img_mk(unsigned *img, int w, int h) {
  img += 2;
  img_w(img) = cast(unsigned, w);
  img_h(img) = cast(unsigned, h);
  return img;
}
static unsigned *
img_new(struct arena *a, struct sys *s, int w, int h) {
  unsigned *img = arena_alloc(a, s, szof(unsigned) * (w * h + 2));
  return img_mk(img, w, h);
}

