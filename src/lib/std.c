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
 *                                  Math
 * ---------------------------------------------------------------------------
 */
// clang-format off
#define lerp(r,a,b,t) ((r)=(a)+(t)*((b)-(a)))
#define rad(x) ((x)*3.141592653f/180.0f)
#define sincosa(a,s,c) *s = sina(a), *c = cosa(a)

#define op(r,e,a,p,b,i,s) ((r) e (a) p ((b) i (s)))
#define opN(r,e,a,p,b,i,s,N)\
  for (int uniqid(_i_) = 0; uniqid(_i_) < N; ++uniqid(_i_))\
    op((r)[uniqid(_i_)],e,(a)[uniqid(_i_)],p,(b)[uniqid(_i_)],i,s)
#define opNs(r,e,a,p,b,i,s,N)\
  for (int uniqid(_i_) = 0; uniqid(_i_) < N; ++uniqid(_i_))\
    op((r)[uniqid(_i_)],e,(0),+,(a)[uniqid(_i_)],p,s)
#define lerpN(r,a,b,t)\
  for (int uniqid(_i_) = 0; uniqid(_i_) < N; ++uniqid(_i_))\
    lerp(r[uniqid(_i_)],a[uniqid(_i_)],b[uniqid(_i_)],t)

#define op2(r,e,a,p,b,i,s) opN(r,e,a,p,b,i,s,2)
#define op2s(r,e,a,p,s) opNs(r,e,a,p,b,i,s,2)
#define op3(r,e,a,p,b,i,s) opN(r,e,a,p,b,i,s,3)
#define op3s(r,e,a,p,s) opNs(r,e,a,p,b,i,s,3)
#define op4(r,e,a,p,b,i,s) opN(r,e,a,p,b,i,s,4)
#define op4s(r,e,a,p,s) opNs(r,e,a,p,b,i,s,4)

#define set2(d,x,y)     (d)[0] = x, (d)[1] = y
#define zero2(d)        set2(d,0,0)
#define dup2(d,f)       set2(d,f,f)
#define cpy2(d, s)      (d)[0] = (s)[0], (d)[1] = (s)[1]
#define add2(d,a,b)     op2(d,=,a,+,b,+,0)
#define sub2(d,a,b)     op2(d,=,a,-,b,+,0)
#define mul2(d,a,s)     op2s(d,=,a,*,s)
#define dot2(a,b)       ((a)[0]*(b)[0]+(a)[1]*(b)[1])
#define len2(v)         sqrta(dot2(v,v))
#define adds2(d,a,b,s)  op2(d,=,a,+,b,*,s)
#define subs2(d,a,b,s)  op2(d,=,a,-,b,*,s)
#define norm2(n,v)      mul2(n,v,rsqrt(dot2(v, v)))
#define normeq2(v)      norm2(v,v)
#define lerp2(r,a,b,t)  lerpN(r,a,b,t,2)

#define set3(v,x,y,z)   (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define dup3(d,f)       set3(d,f,f,f)
#define zero3(v)        set3(0,0,0)
#define cpy3(d,s)       (d)[0]=(s)[0],(d)[1]=(s)[1],(d)[2]=(s)[2]
#define add3(d,a,b)     op3(d,=,a,+,b,+,0)
#define sub3(d,a,b)     op3(d,=,a,-,b,+,0)
#define mul3(d,a,s)     op3s(d,=,a,*,s)
#define dot3(a,b)       ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define len3(v)         sqrta(dot3(v,v))
#define adds3(d,a,b,s)  op3(d,=,a,+,b,*,s)
#define subs3(d,a,b,s)  op3(d,=,a,-,b,*,s)
#define norm3(n,v)      mul3(n,v,rsqrt(dot3(v, v)))
#define normeq3(v)      norm3(v,v)
#define lerp3(r,a,b,t)  lerpN(r,a,b,t,3)
#define cross3(d,a,b) do {\
    (d)[0] = ((a)[1]*(b)[2]) - ((a)[2]*(b)[1]),\
    (d)[1] = ((a)[2]*(b)[0]) - ((a)[0]*(b)[2]),\
    (d)[2] = ((a)[0]*(b)[1]) - ((a)[1]*(b)[0]);}while(0)

#define set4(v,x,y,z,w) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z),(v)[3] =(w)
#define set4w(d,s,w)    (d)[0]=(s)[0],(d)[1]=(s)[1],(d)[2]=(s)[2],(d)[3]=w
#define dup4(d,f)       set4(d,f,f,f,f)
#define cpy4(d,s)       (d)[0]=(s)[0],(d)[1]=(s)[1],(d)[2]=(s)[2],(d)[3]=(s)[3]
#define add4(d,a,b)     op4(d,=,a,+,b,+,0)
#define sub4(d,a,b)     op4(d,=,a,-,b,+,0)
#define mul4(d,a,s)     op4s(d,=,a,*,s)
#define dot4(a,b)       ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2]+(a)[3]*(b)[3])
#define len4(v)         sqrta(dot4(v,v))
#define adds4(d,a,b,s)  op4(d,=,a,+,b,*,s)
#define subs4(d,a,b,s)  op4(d,=,a,-,b,*,s)
#define norm4(n,v)      mul4(n,v,rsqrt(dot4(v, v)))
#define normaleq4(v)    norm4(v,v)
#define lerp4(r,a,b,t)  lerpN(r,a,b,t,4)
#define qid(q)          set4(q,0,0,0,1)
#define qconj(d,s)      do{mul3(d,s,-1.0f);(d)[3]=(s)[3];}while(0)
#define quatf(q,angle,x,y,z)\
    q[0] = (x) * sina(angle * 0.5f),\
    q[1] = (y) * sina(angle * 0.5f),\
    q[2] = (z) * sina(angle * 0.5f),\
    q[3] = cosa(angle * 0.5f)
#define quat(q,angle,axis) quatf(q,axis[0],axis[1],axis[2],angle)
#define qmul(q,a,b)\
    (q)[0] = (a)[3]*(b)[0] + (a)[0]*(b)[3] + (a)[1]*b[2] - (a)[2]*(b)[1],\
    (q)[1] = (a)[3]*(b)[1] + (a)[1]*(b)[3] + (a)[2]*b[0] - (a)[0]*(b)[2],\
    (q)[2] = (a)[3]*(b)[2] + (a)[2]*(b)[3] + (a)[0]*b[1] - (a)[1]*(b)[0],\
    (q)[3] = (a)[3]*(b)[3] - (a)[0]*(b)[0] - (a)[1]*b[1] - (a)[2]*(b)[2];
// clang-format on

union bit_castf {
  float f;
  unsigned i;
};
union bit_castd {
  double f;
  unsigned long long i;
};
union bit_casti {
  unsigned i;
  float f;
};
static int
floori(double x) {
  x = cast(double, (cast(int, x) - ((x < 0.0f) ? 1 : 0)));
  return cast(int, x);
}
static int
ceili(double x) {
  if (x < 0) {
    int t = cast(int, x);
    double r = x - cast(double, t);
    return (r > 0.0f) ? (t + 1) : t;
  } else {
    int i = cast(int, x);
    return (x > i) ? (i + 1) : i;
  }
}
static double
fround(double x) {
  double y;
  static const double toint = 1.0 / 2.22044604925031308085e-16;
  union bit_castd u = {x};
  int e = u.i >> 52 & 0x7ff;
  if (e >= 0x3ff + 52) {
    return x;
  }
  if (u.i >> 63) {
    x = -x;
  }
  if (e < 0x3ff - 1) {
    return 0 * u.f;
  }
  y = x + toint - toint - x;
  if (y > 0.5) {
    y = y + x - 1;
  } else if (y <= -0.5) {
    y = y + x + 1;
  } else {
    y = y + x;
  }
  if (u.i >> 63) {
    y = -y;
  }
  return y;
}
static int
roundi(double x) {
  double f = fround(x);
  return cast(int, f);
}
static double
absf(double x) {
  union bit_castd u = {x};
  u.i &= -1ULL / 2;
  return u.f;
}
static float
log2a(float x) {
  union bit_castf v = {x};
  union bit_casti m = {(v.i & 0x007FFFFF) | 0x3f000000};
  float y = (float)v.i;
  y *= 1.1920928955078125e-7f;
  return y - 124.22551499f - 1.498030302f * m.f -
         1.72587999f / (0.3520887068f + m.f);
}
static float
loga(float x) {
  return 0.69314718f * log2a(x);
}
static float
pow2a(float p) {
  float o = (p < 0) ? 1.0f : 0.0f;
  float c = (p < -126) ? -126.0f : p;
  int w = (int)c;
  float t, z = c - (float)w + o;
  t = (c + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z);
  union bit_casti v = {(unsigned)((1 << 23) * t)};
  return v.f;
}
static float
expa(float p) {
  return pow2a(1.442695040f * p);
}
static float
powa(float x, float p) {
  return pow2a(p * log2a(x));
}
static float
sinapi(float x) {
  union bit_castf p = {0.20363937680730309f};
  union bit_castf r = {0.015124940802184233f};
  union bit_castf s = {-0.0032225901625579573f};
  union bit_castf vx = {x};
  unsigned sign = vx.i & 0x80000000;
  vx.i = vx.i & 0x7FFFFFFF;

  float qp = 1.2732395447351627f * x - 0.40528473456935109f * x * vx.f;
  float qpsq = qp * qp;

  p.i |= sign;
  r.i |= sign;
  s.i ^= sign;
  return 0.78444488374548933f * qp + qpsq * (p.f + qpsq * (r.f + qpsq * s.f));
}
static float
sina(float x) {
  int k = (int)(x * 0.15915494309189534f);
  float half = (x < 0) ? -0.5f : 0.5f;
  return sinapi((half + (float)k) * 6.2831853071795865f - x);
}
static float
cosa(float x) {
  return sina(x + 1.5707963267948966f);
}
static float
rsqrt(float n) {
  union bit_castf conv = {n};
  conv.i = 0x5f375A84 - (conv.i >> 1);
  conv.f = conv.f * (1.5f - ((n * 0.5f) * conv.f * conv.f));
  return conv.f;
}
static float
sqrta(float x) {
  return x * rsqrt(x);
}
static void
qrot3(float *out, const float *q, const float *v) {
  float a[3]; mul3(a, q, 2.0f * dot3(q,v));
  float b[3]; mul3(b, v, q[3]*q[3] - dot3(q,q));
  float c[3]; cross3(c,q,v);
  float d[3]; mul3(d,c, 2.0f * q[3]);
  float e[3]; add3(e, a, b);
  add3(out, e, d);
}
static void
qrotv3(float *q, const float *u, const float *v) {
  float w[3] = {0};
  float norm_u_norm_v = sqrta(dot3(u,u) * dot3(v,v));
  float real_part = norm_u_norm_v + dot3(u,v);
  if (real_part < (1.e-6f * norm_u_norm_v)) {
    if (absf(u[0]) > absf(u[2])) {
      set3(w, -u[1], u[0], 0.0f);
    } else {
      set3(w, 0.0f, -u[2], u[1]);
    }
    real_part = 0;
  } else{
    cross3(w, u,v);
  }
  set4(q, w[0],w[1],w[2], real_part);
  normaleq4(q);
}
static void
qeuler(float *qout, float yaw, float pitch, float roll) {
  float sr, cr; sincosa(roll * 0.5f, &sr, &cr);
  float sp, cp; sincosa(pitch * 0.5f, &sp, &cp);
  float sy, cy; sincosa(yaw * 0.5f, &sy, &cy);

  qout[0] = cy * sp * cr + sy * cp * sr;
  qout[1] = sy * cp * cr - cy * sp * sr;
  qout[2] = cy * cp * sr - sy * sp * cr;
  qout[3] = cy * cp * cr + sy * sp * sr;
}
static void
transformq(float *o3, const float *v3, const float *q, const float *t3) {
  float tmp[3];
  qrot3(tmp, q, v3);
  add3(o3, tmp, t3);
}
static void
transformqI(float *o3, const float *v3, const float *q, const float *t3) {
  float tmp[3]; sub3(tmp, v3, t3);
  float inv[4]; qconj(inv, q);
  qrot3(o3, inv, tmp);
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
  int ret = cast(int, *p &m);
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
#define is_space(c) (((c) == 0x20) || ((c) == 0x09) || ((c) == 0x0a) || ((c) == 0x0b) || ((c) == 0x0c) || ((c) == 0x0d))
#define is_alpha(c) (is_lower(c) || is_upper(c))
// clang-format on

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
#define str_cut_lhs(s, n) *s = str_rhs(*s, n)
#define str_cut_rhs(s, n) *s = str_lhs(*s, n)

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
str_hash(struct str s) {
  return fnv1a64(s.str, s.len, FNV1A64_HASH_INITIAL);
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
  if (s.len < 2) return;
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
str_fnd(struct str hay, struct str needle) {
  struct str_fnd_tbl fnd;
  str_fnd_tbl(&fnd, needle);
  return str_fnd_tbl_str(hay, needle, &fnd);
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
    case 0xf0: res = *p & 0x07, n = 3; break;
    case 0xe0: res = *p & 0x0f, n = 2; break;
    case 0xc0: res = *p & 0x1f, n = 1; break;
    case 0xd0: res = *p & 0x1f, n = 1; break;
    default:   res = *p & 0xff, n = 0; break;
    // clang-format on
  }
  if (p + n + 1 > s->end) {
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
  *s = strp(p + n + 1, s->end);
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

#define arena_set(a, s, n) arena_dyn(a, s, unsigned long long, n)
#define arena_tbl(a, s, n) arena_dyn(a, s, unsigned long long, (n << 1))
#define arena_arr(a, s, T, n) cast(T *, arena_alloc(a, s, szof(T) * n))
#define arena_obj(a, s, T) cast(T*, arena_alloc(a, s, szof(T)))
#define arena_dyn(a, s, T, n) \
  cast(T *, dyn__static(arena_alloc(a, s, dyn_req_siz(sizeof(T) * (n))), (n)))

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
static void
scope_begin(struct scope *s, struct arena *a) {
  assert(a);
  assert(s);

  s->blk = a->blk;
  s->used = a->blk ? a->blk->used : 0;
  a->tmp_cnt++;
}
static void
scope_end(struct scope *s, struct arena *a, struct sys *sys) {
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
#define dyn_del(b, i) (dyn_cnt(b) ? (b)[i] = (b)[--dyn_hdr(b)->cnt] : 0);
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

#define dyn_asn(b, s, x, n)                       \
  do {                                            \
    dyn_clr(b);                                   \
    dyn_fit(b, s, n);                             \
    memcpy(b, x, sizeof((b)[0]) * cast(size_t, (n))); \
    dyn__hdr(b)->cnt = (n);                       \
  } while (0)

#define dyn_put(b, s, i, x, n)                                            \
  do {                                                                    \
    dyn_fit(b, s, n);                                                     \
    assert((i) <= dyn_cnt(b));                                            \
    assert((i) < dyn_cap(b));                                             \
    memmove((b) + (i) + (n), (b) + (i), sizeof((b)[0]) * (size_t)max(0, dyn_cnt(b) - (i))); \
    memcpy((b) + (i), (x), sizeof((b)[0]) * cast(size_t, (n)));           \
    dyn__hdr(b)->cnt += (n);                                              \
  } while (0)

#define dyn_cut(b, i, n)                                                  \
  do {                                                                    \
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
static void set__swap(unsigned long long *a, unsigned long long *b) {unsigned long long t = *a; *a = *b, *b = t;}

#define set_cnt(s) dyn_cnt(s)
#define set_cap(s) dyn_cap(s)
#define set_fnd(s,k) (set__fnd(s, k, set_cap(s)) < set_cap(s))
#define set_put(t,s,k) ((!(t) || !set_fnd(t, k)) ? set__add(t, s, k) : set_cap(t))
#define set_del(s,k) set__del(s, k, set_cap(s))
#define set_free(s,sys) dyn_free(s, sys)
#define set_clr(s) \
  do {((s) ? memset(s, 0, sizeof(unsigned long long) * (size_t)dyn_cap(s)) : 0); dyn_clr(s); } while(0)
// clang-format on

static inline unsigned long long
set__hash(unsigned long long k) {
  unsigned long long h = k & 0x7fffffffffffffffllu;
  return h | (h == 0);
}
static unsigned long long*
set_new(int old, int req, struct sys *sys) {
  int nn = max(SET_MIN_SIZE, (int)((float)req * SET_MIN_GROW_PERCENT));
  int cap = max(nn, cast(int, SET_GROW_FACTOR * cast(float, old)));
  return dyn__grow(0, sys, cap, sizeof(unsigned long long));
}
static inline long long
set__store(unsigned long long *keys, unsigned long long *vals,
           unsigned long long i, unsigned long long h, unsigned long long v) {
  keys[i] = h;
  if (vals) vals[i] = v;
  return cast(long long, i);
}
static long long
set__slot(unsigned long long *keys, unsigned long long *vals, int cap,
          unsigned long long h, unsigned long long v) {
  unsigned long long n = cast(unsigned long long, cap);
  unsigned long long i = h % n, b = i, dist = 0;
  do {
    assert(cast(int, i) < set_cap(keys));
    unsigned long long k = keys[i];
    if (!k) return set__store(keys, vals, i, h, v);
    unsigned long long d = set__dist(k, n, i);
    if (d++ > dist++) continue;
    if (set__is_del(k)) {
      return set__store(keys, vals, i, h, v);
    }
    set__swap(&h, &keys[i]);
    if (vals) set__swap(&v, &vals[i]);
    dist = d;
  } while ((i = ((i + 1) % n)) != b);
  return cast(long long, n);
}
static intptr_t
set__put(unsigned long long *set, unsigned long long key) {
  assert(set);
  unsigned long long h = set__hash(key);
  long long i = set__slot(set, 0, set_cap(set), h, 0);
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
    assert(cast(int, i) < set_cap(set));
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
  if (!set_cnt(set)) {
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
#define tbl__fits(t,n) ((n) < (int)((float)tbl_cap(t) * SET_FULL_PERCENT))
#define tbl__fit(t,s,n) (tbl__fits(t, n) ? 0 : ((t) = set__grow(t, s, n)))
#define tbl__add(t,s,k,v) (tbl__fit(t,s,tbl_cnt(t) + 1), tbl__put(t, k, v))
#define tbl__key(k) set__key(k)

#define tbl_free(t,s) set_free(t,s)
#define tbl_clr(t) set_clr(t)
#define tbl_cnt(t) set_cnt(t)
#define tbl_cap(t) (set_cap(t) >> 1)
#define tbl_val(t,i) cast(long long,((t)[tbl_cap(t) + i]))
#define tbl_del(t,k) tbl__del(t, k, 0)
#define tbl_has(t,k,e) (tbl_fnd(t, k, e) != (e))
#define tbl_any(t) (tbl_cnt(t) > 0)
#define tbl_empty(t) (tbl_cnt(t) == 0)
#define tbl_put(t,s,k,v)                                                  \
  ((!(t) || set__fnd(t,k,tbl_cap(t)) >= tbl_cap(t)) ? tbl__add(t,s,k,v) : tbl_cap(t))
#define for_tbl(i,n,v,t)\
  for (int n = tbl__nxt_idx(t,v,0), i = 0; n < tbl_cap(t); n = tbl__nxt_idx(t,v,n+1), i = i + 1)
// clang-format on

static int
tbl__nxt_idx(unsigned long long *tbl, long long *val, int i) {
  int cnt = tbl_cap(tbl);
  for (; i < cnt; ++i) {
    if (tbl__key(tbl[i])) {
      *val = tbl_val(tbl,i);
      return i;
    }
  }
  return cnt;
}
static long long
tbl__put(unsigned long long *tbl, unsigned long long key, long long ival) {
  assert(tbl);
  unsigned long long h = set__hash(key);
  unsigned long long v = cast(unsigned long long, ival);
  long long i = set__slot(tbl, tbl + tbl_cap(tbl), tbl_cap(tbl), h, v);
  if (i < tbl_cap(tbl)) {
    assert(i < tbl_cap(tbl));
    dyn__hdr(tbl)->cnt++;
  }
  return i;
}
static unsigned long long *
tbl__grow(unsigned long long *old, struct sys *s, int n) {
  unsigned long long *tbl = set_new(set_cap(old), n << 1, s);
  for (int i = 0; i < tbl_cap(old); ++i) {
    if (tbl__key(old[i])) {
      assert(i < tbl_cap(old));
      tbl__put(tbl, old[i], tbl_val(old, i));
    }
  }
  dyn_free(old, s);
  return tbl;
}
static long long
tbl__del(unsigned long long *tbl, unsigned long long key, intptr_t not_found) {
  assert(tbl);
  long long i = set__del(tbl, key, tbl_cap(tbl));
  if (i >= tbl_cap(tbl)) {
    return not_found;
  }
  return tbl_val(tbl, i);
}
static long long
tbl_fnd(unsigned long long *tbl, unsigned long long key, long long not_found) {
  assert(tbl);
  if (!tbl_cnt(tbl)) {
    return not_found;
  }
  long long i = set__fnd(tbl, key, tbl_cap(tbl));
  return (i >= tbl_cap(tbl)) ? not_found : (long long)tbl_val(tbl, i);
}
static void
ut_tbl(struct sys *sys) {
  unsigned long long *tbl = 0;
  unsigned long long h = STR_HASH8("Command");
  long long val = 1337;

  long long at = tbl_put(tbl, sys, h, val);
  assert(tbl != 0);
  assert(tbl_cnt(tbl) == 1);
  assert(tbl_cap(tbl) == 32);
  assert(at == 28);
  assert(tbl[at] == set__hash(h));
  assert(tbl[32 + 28] == 1337);

  long long v = tbl_fnd(tbl, h, 0);
  assert(v == 1337);

  v = tbl_fnd(tbl, h + 1, 0);
  assert(v == 0);

  tbl_del(tbl, h);
  assert(tbl_cnt(tbl) == 0);
  assert(tbl_cap(tbl) > 0);
  assert(tbl[at] == (set__hash(h)|0x8000000000000000llu));

  v = tbl_fnd(tbl, h, 0);
  assert(v == 0);

  long long at2 = tbl_put(tbl, sys, h, val);
  assert(tbl_cnt(tbl) == 1);
  assert(tbl_cap(tbl) > 0);
  assert(at == at2);
  assert(tbl[at] == set__hash(h));
  assert(tbl[32 + 28] == 1337);

  tbl_free(tbl, sys);
  assert(tbl == 0);
  assert(tbl_cnt(tbl) == 0);
  assert(tbl_cap(tbl) == 0);
}

/* ---------------------------------------------------------------------------
 *                                  Image
 * ---------------------------------------------------------------------------
 */
#define col_get(u) \
  (struct color) { col_r(u), col_g(u), col_b(u), col_a(u) }
#define col_paq(c) col_rgba((c)->r, (c)->g, (c)->b, (c)->a)

#define col_msk(c, i) (((c) >> i) & 0xFF)
#define col_r(c) col_msk(c, COL_R)
#define col_g(c) col_msk(c, COL_G)
#define col_b(c) col_msk(c, COL_B)
#define col_a(c) col_msk(c, COL_A)

#define col_shl(c, i) ((unsigned)(((c) & 0xFFu) << (i)))
#define col_clr(c, i) ((c) & ~(0xFFu << i))
#define col_set_r(c, r) (col_clr(c, COL_R) | (col_shl(r, COL_R))
#define col_set_g(c, g) (col_clr(c, COL_G) | (col_shl(g, COL_G))
#define col_set_b(c, b) (col_clr(c, COL_B) | (col_shl(b, COL_B))
#define col_set_a(c, a) (col_clr(c, COL_A) | (col_shl(a, COL_A))

#define col_rgba(r,g,b,a)\
  (col_shl(r, COL_R) | col_shl(g, COL_G) | col_shl(b, COL_B) | col_shl(a, COL_A))
#define col_rgb(r, g, b) col_rgba(r, g, b, 0xFFu)

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

