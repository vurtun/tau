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
 *                              Half-Float
 * ---------------------------------------------------------------------------
 */
static unsigned short
hflt(float in) {
  union flt f = {.f = in};
  union flt f32infty = { 255 << 23 };
  union flt f16max   = { (127 + 16) << 23 };
  union flt denorm_magic = { ((127 - 15) + (23 - 10) + 1) << 23 };
  uint sign_mask = 0x80000000u;
  union hflt o = { 0u };
  unsigned sign = f.u & sign_mask;
  f.u ^= sign;
  // NOTE all the integer compares in this function can be safely
  // compiled into signed compares since all operands are below
  // 0x80000000. Important if you want fast straight SSE2 code
  // (since there's no unsigned PCMPGTD).
  if (f.u >= f16max.u) { // result is Inf or NaN (all exponent bits set)
      o.u = (f.u > f32infty.u) ? 0x7e00 : 0x7c00; // NaN->qNaN and Inf->Inf
  } else { // (De)normalized number or zero
    if (f.u < (113u << 23u)) { // resulting FP16 is subnormal or zero
      // use a magic value to align our 10 mantissa bits at the bottom of
      // the float. as long as FP addition is round-to-nearest-even this just works.
      f.f += denorm_magic.f;
      // and one integer subtract of the bias later, we have our final float!
      o.u = cast(unsigned short, f.u - denorm_magic.u);
    } else {
      unsigned mant_odd = (f.u >> 13u) & 1u; // resulting mantissa is odd
      // update exponent, rounding bias part 1
      f.u += ((15 - 127) << 23) + 0xfff; // rounding bias part 2
      f.u += mant_odd;
      o.u = cast(unsigned short, f.u >> 13u); // take the bits!
    }
  }
  o.u |= sign >> 16;
  return o.u;
}
static int4
hflt4(flt4 f) {
  int4 mask_sign = int4_uint(0x80000000u);
  int4 c_f16max = int4_int((127 + 16) << 23);
  int4 c_nanbit = int4_int(0x200);
  int4 c_infty_as_fp16 = int4_int(0x7c00);
  int4 c_min_normal = int4_int((127 - 14) << 23); // smallest FP32 that yields a normalized FP16
  int4 c_subnorm_magic = int4_int(((127 - 15) + (23 - 10) + 1) << 23);
  int4 c_normal_bias = int4_int(0xfff - ((127 - 15) << 23)); // adjust exponent and add mantissa rounding

  flt4 msign = flt4_int4(mask_sign);
  flt4 justsign = flt4_and(msign, f);
  flt4 absf = flt4_xor(f, justsign);
  int4 absf_int = int4_flt4(absf);// the cast is "free" (extra bypass latency, but no thruput hit)
  int4 f16max = c_f16max;
  flt4 b_isnan = flt4_cmpu(absf, absf);
  int4 b_isregular = int4_cmp_gt(f16max, absf_int);
  int4 nanbit = int4_and(int4_flt4(b_isnan), c_nanbit);
  int4 inf_or_nan = int4_or(nanbit, c_infty_as_fp16);

  int4 min_normal = c_min_normal;
  int4 b_issub = int4_cmp_gt(min_normal, absf_int);
  // "result is subnormal" path
  flt4 subnorm1 = flt4_add(absf, flt4_int4(c_subnorm_magic)); // magic value to round output mantissa
  int4 subnorm2 = int4_sub(int4_flt4(subnorm1), c_subnorm_magic); // subtract out bias
  // "result is normal" path
  int4 mantoddbit = int4_sll(absf_int, 31 - 13);
  int4 mantodd = int4_sra(mantoddbit, 31);

  int4 round1 = int4_add(absf_int, c_normal_bias);
  int4 round2 = int4_sub(round1, mantodd); // if mantissa LSB odd, bias towards rounding up (RTNE)
  int4 normal = int4_srl(round2, 13); // rounded result
  // combine the two non-specials
  int4 nonspecial = int4_or(int4_and(subnorm2, b_issub), int4_andnot(b_issub, normal));
  // merge in specials as well
  int4 joined = int4_or(int4_and(nonspecial, b_isregular), int4_andnot(b_isregular, inf_or_nan));
  int4 sign_shift = int4_srl(int4_flt4(justsign), 16);
  int4 final = int4_or(joined, sign_shift);
  return final;
}
/* ---------------------------------------------------------------------------
 *                                  Math
 * ---------------------------------------------------------------------------
 */
// clang-format off
#undef FLT_EPSILON
#define FLT_TAU 6.28318530717959f
#define FLT_PI 3.14159265359f
#define FLT_HALF_PI (0.5f * 3.14159265359f)
#define FLT_E 2.71828182846f
#define FLTR_SQRT2 1.41421356237f
#define FLT_EPSILON (1.192092896e-07f)

#define rad(x) ((x)*FLT_TAU/360.0f)
#define deg(x) ((x)*360.0f/FLT_TAU)
#define lerp(r,a,b,t) ((r)=(a)+(t)*((b)-(a)))
#define lerp_smooth(r,a,b,t) lerp(r,a,b,smooth01(t))
#define lerp_inv(r,a,b,v) (((v)-(a))/((b)-(a)))
#define eerp(r,a,b,t) powa(a,1-t) * powa(b,t) // useful for scaling or zooming
#define eerp_inv(a,b,v) (logf((a)/(v))/logf((a)/(b)))
#define sincosa(a,s,c) *s = sina(a), *c = cosa(a)
#define repeat(v,len) clamp(0.0f, (v) - floorf((v)/(len)) * (len), len) // repeats the given value in the interval specified by len
#define pingpong(t,len) len - absf(repat(t,len * 2f)-len) // repeats a value within a range, going back and forth
#define triwave(t,period) (1.0f-absf(2.0f*(((t)/(period))-floorf(((t)/(period))))-1.0f))
#define smooth01(x) ((x)*(x)*(3-2*(x)))
#define smoother01(x) ((x)*(x)*(x)*((x)*((x)*6-15)+10))
#define remap(r,imin,imax,omin,omax,v) lerp(r,omin,omax, inv_lerp(imin,imax,v))

#define op(r,e,a,p,b,i,s) ((r) e (a) p ((b) i (s)))
#define opN(r,e,a,p,b,i,s,N)\
  for (int uniqid(_i_) = 0; uniqid(_i_) < N; ++uniqid(_i_))\
    op((r)[uniqid(_i_)],e,(a)[uniqid(_i_)],p,(b)[uniqid(_i_)],i,s)
#define opNs(r,e,a,p,b,i,s,N)\
  for (int uniqid(_i_) = 0; uniqid(_i_) < N; ++uniqid(_i_))\
    op((r)[uniqid(_i_)],e,(0),+,(a)[uniqid(_i_)],p,s)
#define lerpN(r,a,b,t,N)\
  for (int uniqid(_i_) = 0; uniqid(_i_) < N; ++uniqid(_i_))\
    lerp((r)[uniqid(_i_)],(a)[uniqid(_i_)],(b)[uniqid(_i_)],t)
#define mapN(r,fn,a,N)\
  for (int uniqid(_i_) = 0; uniqid(_i_) < N; ++uniqid(_i_))\
    (r)[uniqid(_i_)] = fn((a)[uniqid(_i_)])
#define map2N(r,fn,a,b,N)\
  for (int uniqid(_i_) = 0; uniqid(_i_) < N; ++uniqid(_i_))\
    (r)[uniqid(_i_)] = fn((a)[uniqid(_i_)],(b)[uniqid(_i_)])
#define map3N(r,fn,a,b,c,N)\
  for (int uniqid(_i_) = 0; uniqid(_i_) < N; ++uniqid(_i_))\
    (r)[uniqid(_i_)] = fn((a)[uniqid(_i_)],(b)[uniqid(_i_)],(c)[uniqid(_i_)])

#define minN(r,a,b,N) map2N(r,min,a,b,N)
#define maxN(r,a,b,N) map2N(r,max,a,b,N)
#define clampN(r,i,v,x,N) map3N(r,clamp,i,v,x,N)

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
#define cmp2(a,b,e)     cmpN(a,b,e,2)
#define swzl2(r,f,a,b)  set2(r,(f)[a],(f)[b])
#define min2(r,a,b)     minN(r,a,b,2)
#define max2(r,a,b)     maxN(r,a,b,2)
#define clamp2(r,i,v,x) clampN(r,i,v,x,2)
#define map2(r,fn,a)    mapN(r,fn,a,2);

#define set3(v,x,y,z)   (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define dup3(d,f)       set3(d,f,f,f)
#define zero3(v)        set3(v,0,0,0)
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
#define proj3(p,v,n)    mul3(p,n,dot3(v,n))
#define projn3(p,v,to)  mul3(to,dot3(v,to)/Dot(to,to));
#define rej3(p,a,b)     do {proj3(p,a,b); sub3(p,a,p);}while(0)
#define rejn3(p,a,b)    do {projn3(p,a,b); sub3(p,a,p);}while(0)
#define lerp3(r,a,b,t)  lerpN(r,a,b,t,3)
#define cmp3(a,b,e)     cmpN(a,b,e,3)
#define swzl3(r,f,a,b,c) set3(r,(f)[a],(f)[b],(f)[c])
#define min3(r,a,b)     minN(r,a,b,3)
#define max3(r,a,b)     maxN(r,a,b,3)
#define map3(r,fn,a)    mapN(r,fn,a,3);
#define clamp3(r,i,v,x) clampN(r,i,v,x,3)
#define cross3(d,a,b) do {\
  (d)[0] = ((a)[1]*(b)[2]) - ((a)[2]*(b)[1]),\
  (d)[1] = ((a)[2]*(b)[0]) - ((a)[0]*(b)[2]),\
  (d)[2] = ((a)[0]*(b)[1]) - ((a)[1]*(b)[0]);\
} while(0)

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
#define cmp4(a,b,e)     cmpN(a,b,e,4)
#define min4(r,a,b)     minN(r,a,b,3)
#define max4(r,a,b)     maxN(r,a,b,3)
#define map4(r,fn,a)    mapN(r,fn,a,4);
#define clamp4(r,i,v,x) clampN(r,i,v,x,4)
#define swzl4(r,f,a,b,c,e) set4(r,(f)[a],(f)[b],(f)[c],(f)[e])
#define qid(q)          set4(q,0,0,0,1)
#define qconj(d,s)      do{mul3(d,s,-1.0f);(d)[3]=(s)[3];}while(0)
#define quatf(q,angle,x,y,z)\
  (q)[3] = sina((angle) * 0.5f),\
  (q)[0] = (x) * (q)[3], (q)[1] = (y) * (q)[3],\
  (q)[2] = (z) * (q)[3], (q)[3] = cosa((angle) * 0.5f)
#define quat(q,n,x) quatf(q,n,(x)[0],(x)[1],(x)[2])
#define qmul(q,a,b)\
  (q)[0] = (a)[3]*(b)[0] + (a)[0]*(b)[3] + (a)[1]*b[2] - (a)[2]*(b)[1],\
  (q)[1] = (a)[3]*(b)[1] + (a)[1]*(b)[3] + (a)[2]*b[0] - (a)[0]*(b)[2],\
  (q)[2] = (a)[3]*(b)[2] + (a)[2]*(b)[3] + (a)[0]*b[1] - (a)[1]*(b)[0],\
  (q)[3] = (a)[3]*(b)[3] - (a)[0]*(b)[0] - (a)[1]*b[1] - (a)[2]*(b)[2]
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
fmoda(float x, float y) {
  union {float f; unsigned i;} ux = {x}, uy = {y};
  int ex = (ux.i >> 23) & 0xff;
  int ey = (uy.i >> 23) & 0xff;
  unsigned sx = ux.i & 0x80000000;
  unsigned i, uxi = ux.i;

  if (uy.i<<1 == 0 || ((uy.i & 0x7fffffff) > 0x7f800000) || ex == 0xff){
    return (x*y)/(x*y);
  }
  if (uxi<<1 <= uy.i<<1) {
    if (uxi<<1 == uy.i<<1) {
      return 0*x;
    }
    return x;
  }
  /* normalize x and y */
  if (!ex) {
    for (i = uxi<<9; i>>31 == 0; ex--, i <<= 1);
    uxi <<= -ex + 1;
  } else {
    uxi &= -1U >> 9;
    uxi |= 1U << 23;
  }
  if (!ey) {
    for (i = uy.i<<9; i>>31 == 0; ey--, i <<= 1);
    uy.i <<= -ey + 1;
  } else {
    uy.i &= -1U >> 9;
    uy.i |= 1U << 23;
  }
  /* x mod y */
  for (; ex > ey; ex--) {
    i = uxi - uy.i;
    if (i >> 31 == 0) {
      if (i == 0)
        return 0*x;
      uxi = i;
    }
    uxi <<= 1;
  }
  i = uxi - uy.i;
  if (i >> 31 == 0) {
    if (i == 0)
      return 0*x;
    uxi = i;
  }
  for (; uxi>>23 == 0; uxi <<= 1, ex--);

  /* scale result up */
  if (ex > 0) {
    uxi -= 1U << 23;
    uxi |= (unsigned)ex << 23;
  } else {
    uxi >>= -ex + 1;
  }
  uxi |= sx;
  ux.i = uxi;
  return ux.f;
}
static inline int
cmpN(const float *a, const float *b, float e, int N) {
  int r = 1;
  for (int i = 0; i < N; ++i)
    r &= (absf(a[i] - b[i]) > e);
  return r;
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
  flt4_strs(&n, flt4_rsqrt(flt4_flt(n)));
  return n;
}
static float
sqrta(float x) {
  return x * rsqrt(x);
}
static float
acosa(float a) {
  if (a < 0.0f) {
    if (a <= -1.0f) {
      return FLT_PI;
    }
    a = cast(float, absf(a));
    float t0 = (-0.0187293f * a + 0.0742610f);
    float t1 = (t0 * a - 0.2121144f);
    return FLT_PI - (t1 * a + 1.5707288f) * sqrta(1.0f-a);
  } else {
    if (a >= 1.0f) {
      return 0.0f;
    }
    float t0 = (-0.0187293f * a + 0.0742610f);
    float t1 = (t0 * a - 0.2121144f);
    return (t1 * a + 1.5707288f) * sqrta(1.0f - a);
  }
}
static float
asina(float a) {
  if (a < 0.0f) {
    if (a <= -1.0f) {
      return -FLT_HALF_PI;
    }
    a = cast(float, absf(a));
    float t0 = (-0.0187293f * a + 0.0742610f);
    float t1 = (t0 * a - 0.2121144f);
    return (t1 * a + 1.5707288f) * sqrta(1.0f - a) - FLT_HALF_PI;
  } else {
    if (a >= 1.0f) {
     return FLT_HALF_PI;
    }
    float t0 = (-0.0187293f * a + 0.0742610f);
    float t1 = (t0 * a - 0.2121144f);
    return FLT_HALF_PI - (t1 * a + 1.5707288f) * sqrta(1.0f - a);
  }
}
static float
atana(float a) {
  float s;
  if (absf(a) > 1.0f) {
    a = 1.0f / a;
    s = a * a;
    float t0 = (0.0028662257f * s - 0.0161657367f);
    float t1 = (t0 * s + 0.0429096138f);
    float t2 = (t1 * s - 0.0752896400f);
    float t3 = (t2 * s + 0.1065626393f);
    float t4 = (t3 * s - 0.1420889944f);
    float t5 = (t4 * s + 0.1999355085f);
    float t6 = (t5 * s - 0.3333314528f);
    s = - ((t6 * s) + 1.0f) * a;
    if ( a < 0.0f ) {
      return s - FLT_HALF_PI;
    } else {
      return s + FLT_HALF_PI;
    }
  } else {
    s = a * a;
    float t0 = (0.0028662257f * s - 0.0161657367f);
    float t1 = (t0 * s + 0.0429096138f);
    float t2 = (t1 * s - 0.0752896400f);
    float t3 = (t2 * s + 0.1065626393f);
    float t4 = (t3 * s - 0.1420889944f);
    float t5 = (t4 * s + 0.1999355085f);
    return (((t5 * s - 0.3333314528f) * s) + 1.0f) * a;
  }
}
static float
atan2a(float y, float x) {
  float a, s;
  if (absf(y) > absf(x)) {
    a = x / y;
    s = a * a;
    float t0 = (0.0028662257f * s - 0.0161657367f);
    float t1 = (t0 * s + 0.0429096138f);
    float t2 = (t1 * s - 0.0752896400f);
    float t3 = (t2 * s + 0.1065626393f);
    float t4 = (t3 * s - 0.1420889944f);
    float t5 = (t4 * s + 0.1999355085f);
    s = - (((t5 * s - 0.3333314528f) * s) + 1.0f) * a;
    if (a < 0.0f) {
      return s - FLT_HALF_PI;
    } else {
      return s + FLT_HALF_PI;
    }
  } else {
    a = y / x;
    s = a * a;
    float t0 = (0.0028662257f * s - 0.0161657367f);
    float t1 = (t0 * s + 0.0429096138f);
    float t2 = (t1 * s - 0.0752896400f);
    float t3 = (t2 * s + 0.1065626393f);
    float t4 = (t3 * s - 0.1420889944f);
    float t5 = (t4 * s + 0.1999355085f);
    return (((t5 * s - 0.3333314528f) * s) + 1.0f) * a;
  }
}
static float
angle3(const float *restrict a3, const float *restrict b3) {
  float c[3]; cpy3(c, a3);
  float d[3]; cpy3(d, b3);
  float len1 = dot3(c,c);
  float len2 = dot3(d,d);
  float n = len1 * len2;
  if (n < 1.e-6f) return 0.0f;
  float dot = dot3(c,d);
  return acosa(dot/sqrta(n));
}
static void
qrot3(float *restrict out, const float *restrict qrot,
      const float *restrict vec) {
  float q[4]; cpy4(q,qrot);
  float v[3]; cpy3(v,vec);
  float a[3]; mul3(a, q, 2.0f * dot3(q,v));
  float b[3]; mul3(b, v, q[3]*q[3] - dot3(q,q));
  float c[3]; cross3(c,q,v);
  float d[3]; mul3(d,c, 2.0f * q[3]);
  float e[3]; add3(e, a, b);
  float r[3]; add3(r, e, d);
  cpy3(out, r);
}
static void
qalign3(float *restrict q, const float *restrict from3,
        const float *restrict to3) {
  float w[3] = {0};
  float u[3]; cpy3(u,from3);
  float v[3]; cpy3(v,to3);
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

  float ret[4];
  ret[0] = cy * sp * cr + sy * cp * sr;
  ret[1] = sy * cp * cr - cy * sp * sr;
  ret[2] = cy * cp * sr - sy * sp * cr;
  ret[3] = cy * cp * cr + sy * sp * sr;
  cpy4(qout, ret);
}
static void
eulerq(float *restrict out3, const float *restrict qin) {
  float q[4]; cpy4(q, qin);
  float y_sq = q[1] * q[1];
  float t0 = 2.0f * (q[3] * q[0] + q[1] * q[2]);
  float t1 = 1.0f - 2.0f * (q[0] * q[0] + y_sq);
  float t2 = 2.0f * (q[3] * q[1] - q[2] * q[0]);
  t2 = t2 > 1.0f ? 1.0f : t2;
  t2 = t2 < -1.0f ? -1.0f : t2;
  float t3 = 2.0f * (q[3] * q[2] + q[0] * q[1]);
  float t4 = 1.0f - 2.0f * (y_sq + q[2] * q[2]);
  float v[3]; set3(v, atan2a(t0, t1), asina(t2), atan2a(t3, t4));
  cpy3(out3, v);
}
static void
qtransform(float *restrict o3, const float *restrict in3,
           const float *restrict qrot, const float *restrict mov3) {
  float v3[3]; cpy3(v3, in3);
  float q[4]; cpy4(q, qrot);
  float t3[3]; cpy3(t3, mov3);

  float tmp[3], ret[3];
  qrot3(tmp, q, v3);
  add3(ret, tmp, t3);
  cpy3(o3, ret);
}
static void
qtransformI(float *restrict o3, const float *restrict in3,
            const float *restrict qrot, const float *restrict mov3) {
  float v3[3]; cpy3(v3, in3);
  float q[4]; cpy4(q, qrot);
  float t3[3]; cpy3(t3, mov3);

  float tmp[3]; sub3(tmp, v3, t3);
  float inv[4]; qconj(inv, q);
  float ret[3]; qrot3(ret, inv, tmp);
  cpy3(o3,ret);
}
static void
qaxis(float *restrict o3, const float *restrict q) {
  float n = len3(q);
  if (n < 1.0e-9f) {
    set3(o3,-1, 0, 0);
    return;
  }
  float tmp[3];
  mul3(tmp, q, n);
  cpy3(o3, tmp);
}
static float
qangle(const float *q) {
  return 2.0f * acosa(clamp(-1.0f, q[3], 1.0f));
}
static void
qpow(float *restrict qo, const float *restrict q, float exp) {
  float axis[3]; qaxis(axis, q);
  float angle = qangle(q) * exp;
  float ret[4]; quat(ret, angle, axis);
  cpy4(qo, ret);
}
static void
qintegrate(float *restrict qo, const float *restrict qq,
           const float *restrict qv, float dt) {
  float p[4]; qpow(p, qv, dt);
  float r[4]; qmul(r, p, qq);
  cpy4(qo, r);
}
static void
qintegratev(float *restrict qo, const float *restrict qrot,
            float *restrict angle_vel3, float dt) {
  // omega: angular velocity (direction is axis, magnitude is angle)
  // https://fgiesen.wordpress.com/2012/08/24/quaternion-differentiation/
  // https://www.ashwinnarayan.com/post/how-to-integrate-quaternions/
  // https://gafferongames.com/post/physics_in_3d/
  float q[4]; cpy4(q, qrot);
  float av[3]; cpy3(av, angle_vel3);
  float t[4] = {0,0,0,1};
  mul3(t, av, dt * 0.5f);
  float r[4]; qmul(r, t, q); normaleq4(r);
  cpy4(qo, r);
}
static void
qslerp(float *restrict qres, const float *restrict qfrom,
       const float *restrict qto, float t) {
  if (t <= 0.0f) {
    cpy4(qres,qfrom);
    return;
  } else if (t >= 1.0f || cmp4(qfrom, qto, FLT_EPSILON)) {
    cpy4(qres, qto);
    return;
  }
  float tmp[4];
  float cosom = dot4(qfrom,qto);
  if (cosom < 0.0f) {
    mul4(tmp,qto,-1.0f);
    cosom = -cosom;
  } else {
    cpy4(tmp,qto);
  }
  float scale0, scale1;
  if ((1.0f - cosom) > 1e-6f) {
    scale0 = 1.0f - cosom * cosom;
    float sinom = rsqrt(scale0);
    float omega = atan2a(scale0 * sinom, cosom);
    scale0 = sina((1.0f - t) * omega) * sinom;
    scale1 = sina(t * omega) * sinom;
  } else {
    scale0 = 1.0f - t;
    scale1 = t;
  }
  float a[4]; mul4(a,qfrom,scale0);
  float b[4]; mul4(b,tmp,scale1);
  add4(qres,a,b);
}
static void
qtwist(float *restrict qtwist, const float *restrict qin,
       const float *restrict axis) {
  float q[4]; cpy4(q, qin);
  float d = dot3(q,axis);
  float t[4]; mul3(t,axis,d); t[3] = q[3];
  float l2 = dot4(t,t);
  if (l2 == 0.0f) {
    set4(qtwist, 0,0,0,1);
    return;
  }
  float l = rsqrt(l2);
  mul4(t,t,l);
  cpy4(qtwist,t);
}
static void
qst_get(float *restrict qswing, float *restrict qtwist,
        const float *restrict qin){
  float q[4]; cpy4(q,qin);
  float s = sqrta((q[3]*q[3]) + (q[0]*q[0]));
  if (s != 0.0f) {
    float y = (q[3] * q[1] - q[0] * q[2]);
    float z = (q[3] * q[2] + q[0] * q[1]);
    float qs[4]; set4(qs, 0, y/s, z/s, s);
    float qt[4]; set4(qtwist, q[0]/s, 0, 0, q[3]/s);
    cpy4(qswing, qs);
    cpy4(qtwist, qt);
  } else {
    /* 180 degree rotation around either y or z */
    set4(qtwist, 0,0,0,1);
    cpy4(qswing, q);
  }
}
static void
qst__decomp(float *restrict qswing, float *restrict qtwist,
            const float *restrict q, const float *restrict axis3) {
  if (dot3(q,q) < 1.0e-9f) {
     // singularity: rotation by 180 degree
    float rot_twist_axis[3]; qrot3(rot_twist_axis, q, axis3);
    float swing_axis[3]; cross3(swing_axis, axis3, rot_twist_axis);
    if (dot3(swing_axis,swing_axis) < 1e-9f) {
      float swing_angle = angle3(axis3, rot_twist_axis);
      quat(qswing, swing_angle, swing_axis);
    } else {
      set4(qswing, 0,0,0,1);
    }
    // always twist 180 degree on singularity
    quat(qswing, rad(180.0f), axis3);
    return;
  }
  // formula & proof:
  // http://www.euclideanspace.com/maths/geometry/rotations/for/decomposition/
  proj3(qtwist, q, axis3);
  normaleq4(qtwist);
  float inv[4]; qconj(inv, qtwist);
  qmul(qswing, q, inv);
}
static void
qst_decomp(float *restrict qfull_swing, float *restrict qfull_twist,
           const float *restrict qa, const float *restrict qb,
           const float *restrict twist_axis3) {
  float inv[4]; qconj(inv, qa);
  float q[4]; qmul(q, qb, inv);
  qst__decomp(qfull_swing, qfull_twist, q, twist_axis3);
}
static void
qst__nlerp(float *restrict qres, float *restrict qswing, float *restrict qtwist,
           const float *restrict qfull_swing, const float *restrict qfull_twist,
           float t_swing, float t_twist) {
  static const float qid[4] = {0,0,0,1};
  lerp4(qswing, qid, qfull_swing, t_swing);
  lerp4(qtwist, qid, qfull_twist, t_twist);
  qmul(qres, qtwist, qswing);
}
static void
qst_nlerp(float *restrict qres, const float *restrict qfull_swing,
          const float *restrict qfull_twist, float t_swing, float t_twist) {
  float swing[4], twist[4];
  qst__nlerp(qres, swing, twist, qfull_swing, qfull_twist, t_swing, t_twist);
}
static void
qst__slerp(float *restrict qres, float *restrict qswing, float *restrict qtwist,
           const float *restrict qfull_swing, const float *restrict qfull_twist,
           float t_swing, float t_twist) {
  static const float qid[4] = {0,0,0,1};
  qslerp(qswing, qid, qfull_swing, t_swing);
  qslerp(qtwist, qid, qfull_twist, t_twist);
  qmul(qres, qtwist, qswing);
}
static void
qst_slerp(float *restrict qres, const float *restrict qfull_swing,
          const float *restrict qfull_twist, float t_swing, float t_twist) {
  float swing[4], twist[4];
  qst__slerp(qres, swing, twist, qfull_swing, qfull_twist, t_swing, t_twist) ;
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

#define sort_int(rnk, rnk2, a, siz, n, off) sort__base(rnk, rnk2, a, siz, n, off, 0, 0, sort__cast_int)
#define sort_uint(rnk, rnk2, a, siz, n, off) sort__base(rnk, rnk2, a, siz, n, off, 0, 0, sort__cast_uint)
#define sort_flt(rnk, rnk2, a, siz, n, off) sort__base(rnk, rnk2, a, siz, n, off, 0, 0, sort__cast_flt)
#define sort_str(tmp, tmp2, tmp3, a, n, siz, off) sort__str(tmp, tmp2, tmp3, a, n, siz, off, 0, 0)

#define sort_int_cb(rnk, rnk2, a, siz, n, off, cb, usr) sort__base(rnk, rnk2, a, siz, n, off, usr, cb, sort__cast_int)
#define sort_uint_cb(rnk, rnk2, a, siz, n, off, cb, usr) sort__base(rnk, rnk2, a, siz, n, off, usr, cb, sort__cast_uint)
#define sort_flt_cb(rnk, rnk2, a, siz, n, off, cb, usr) sort__base(rnk, rnk2, a, siz, n, off, usr, cb, sort__cast_flt)
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
  unsigned char *a = (unsigned char*)arr;
  int i = lo - 1, j = hi;
  if (hi <= lo) {
    return;
  }
  int p = lo - 1, q = hi;
  unsigned v = sort__access(a + rnk[hi] * siz, usr, access, conv, off);
  while (1) {
    while (sort__access(a + rnk[++i] * siz, usr, access, conv, off) < v);
    while (v < sort__access(a + rnk[--j] * siz, usr, access, conv, off) && j != lo);
    if (i >= j) break;
    {int tmp = rnk[i]; rnk[i] = rnk[j]; rnk[j] = tmp;}
    if (sort__access(a + rnk[i] * siz, usr, access, conv, off) == v) {
      int tmp = rnk[p]; rnk[p] = rnk[i]; rnk[p] = tmp;
      p++;
    }
    if (sort__access(a + rnk[j] * siz, usr, access, conv, off) == v) {
      int tmp = rnk[q]; rnk[q] = rnk[j]; rnk[q] = tmp;
      q--;
    }
  }
  j = i - 1;
  {int tmp = rnk[i]; rnk[i] = rnk[hi]; rnk[hi] = tmp;}
  for (int k = lo; k < p; k++, j--) {
    int tmp = rnk[k]; rnk[k] = rnk[j]; rnk[j] = tmp;
  }
  i = i + 1;
  for (int k = hi - 1; k > q; k--, i++) {
    int tmp = rnk[k]; rnk[k] = rnk[i]; rnk[i] = tmp;
  }
  sort_q3(rnk, a, siz, n, off, lo, j, usr, access, conv);
  sort_q3(rnk, a, siz, n, off, i, hi, usr, access, conv);
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
    // create offsets
    lnk[0] = (unsigned*)rnk2;
    for (int j = 1; j < SORT_RADIX_SIZE; ++j) {
      lnk[j] = lnk[j-1] + pass_cnt[j-1];
    }
    // perform radix sort
    unsigned shift = i * SORT_RADIX_BITS;
    for (unsigned j = 0; i < cast(unsigned,n); ++i) {
      int id = rnk[j];
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
 *                                  Color
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

#define byte_flt(x) clamp(0u, cast(unsigned char, 0.5f + 255.0f * (x)), 255u)
#define flt_byte(x) (cast(float,(x)) * (1.0f/255.0f))

static float
col_srgb_linear(float x) {
  float v = 0.0f;
  if (x <= 0.0031308f) {
    v = 12.92f * x;
  } else {
    v = (1.0f + 0.055f) * powa(x, 1.0f / 2.0f) - 0.055f;
  }
  return clamp(0, v, 1.0f);
}
static void
col_srgb_rgb(float *restrict srgb_out, const float *restrict rgb_in) {
  float rgb[3]; cpy3(rgb, rgb_in);
  float srgb[3]; map3(srgb, col_srgb_linear, rgb);
  cpy3(srgb_out, srgb);
}
static void
col_srgba_rgba(float *restrict srgba_out, const float *restrict rgba_in) {
  float rgba[4]; cpy4(rgba, rgba_in);
  float srgba[4]; map4(srgba, col_srgb_linear, rgba);
  cpy4(srgba_out, srgba);
}
static float
col_linear_srgb(float x) {
  if (x <= 0.04045f) {
    return x / 12.98f;
  } else {
    return powa((x + 0.055f )/(1.0f + 0.055f), 2.4f);
  }
}
static void
col_rgb_srgb(float *restrict rgb_out, const float *restrict srgb_in) {
  float srgb[3]; cpy3(srgb, srgb_in);
  float rgb[3]; map3(rgb, col_linear_srgb, srgb);
  cpy3(rgb_out, rgb);
}
static void
col_rgba_srgba(float *restrict rgba_out, const float *restrict srgba_in) {
  float srgba[4]; cpy4(srgba, srgba_in);
  float rgba[4]; map4(rgba, col_linear_srgb, srgba);
  cpy4(rgba_out, rgba);
}
static unsigned
col_paq_flt(const float *restrict rgba_in) {
  float rgba[4]; cpy4(rgba, rgba_in);
  unsigned char col[4]; map4(col, byte_flt, rgba);
  return col_rgba(col[0], col[1], col[2], col[3]);
}
static void
col_flt_paq(float *out, unsigned in) {
  unsigned char col[4] = {col_r(in), col_g(in), col_b(in), col_a(in)};
  float rgba[4]; map4(rgba, flt_byte, col);
  cpy4(out, rgba);
}
static void
col_hsv_rgb(float *restrict hsv_out, const float *restrict rgb_in) {
  float hsv[3] = {0};
  float rgb[3]; cpy3(rgb, rgb_in);
  float m = min3i(rgb[0], rgb[1], rgb[2]);
  hsv[2] = max3i(rgb[0], rgb[1], rgb[2]);
  float c = hsv[2] - m;
  if (c != 0.0f) {
    hsv[1] = c / hsv[2];
    float d[3] = {(hsv[2] - rgb[0])/c, (hsv[2] - rgb[1])/c, (hsv[2] - rgb[2])/c};
    float dzxy[3]; swzl3(dzxy,d,2,0,1);
    static const float off[3] = {2.0f, 4.0f, 6.0f};
    float t[3]; sub3(t, d, dzxy);
    add3(d, t, off);
    if (rgb[0] >= hsv[2]) hsv[0] = d[2];
    else if (rgb[1] >= hsv[2]) hsv[0] = d[0];
    else hsv[0] = d[1];
    hsv[0] = fmoda(hsv[0] / 6.0f, 1.0f);
  }
  cpy3(hsv_out, hsv);
}
static void
col_rgb_hue(float *restrict rgb_out, float hue) {
  const float rgb[3] = {
    clamp01((float)absf(hue * 6.0f - 3.0f) - 1.0f),
    clamp01(2.0f - (float)absf(hue * 6.0f - 2.0f)),
    clamp01(2.0f - (float)absf(hue * 6.0f - 4.0f))
  };
  cpy3(rgb_out, rgb);
}
static void
col_rgb_hsv(float *restrict rgb_out, const float *restrict hsv_in) {
  float hsv[3]; cpy3(hsv, hsv_in);
  float rgb[3]; col_rgb_hue(rgb, hsv[0]);
  const float ret[3] = {
    ((rgb[0] - 1.0f) * hsv[1] + 1.0f) * hsv[2],
    ((rgb[1] - 1.0f) * hsv[1] + 1.0f) * hsv[2],
    ((rgb[2] - 1.0f) * hsv[1] + 1.0f) * hsv[2],
  };
  cpy3(rgb_out, ret);
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

