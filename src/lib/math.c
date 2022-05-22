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

/* ---------------------------------------------------------------------------
 *                                Standart
 * ---------------------------------------------------------------------------
 */
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
static double
absf(double x) {
  union bit_castd u = {x};
  u.i &= -1ULL / 2;
  return u.f;
}
static inline int
cmpN(const float *a, const float *b, float e, int N) {
  int r = 1;
  for (int i = 0; i < N; ++i)
    r &= (absf(a[i] - b[i]) > e);
  return r;
}
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
static inline int
fequal(float a, float b) {
  /* http://realtimecollisiondetection.net/blog/?p=89 */
  static const float epsilon = 0.0001f;
  return absf(a - b) < epsilon * max(1.0f, max(absf(a), absf(b)));
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
/* ---------------------------------------------------------------------------
 *                              Half-Float
 * ---------------------------------------------------------------------------
 */
union flt {
  unsigned u;
  float f;
  struct {
    unsigned mant : 23;
    unsigned expo : 8;
    unsigned sign : 1;
  };
};
union hflt {
  unsigned short u;
  struct {
    unsigned mant : 10;
    unsigned Expo : 5;
    unsigned sign : 1;
  };
};
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
 *                                Vector
 * ---------------------------------------------------------------------------
 */
/* vector */
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

/* vector 2D */
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

/* vector 3D */
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

/* vector 4D */
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

/* ---------------------------------------------------------------------------
 *                                Quaternion
 * ---------------------------------------------------------------------------
 */
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

/* ---------------------------------------------------------------------------
 *                                Swing-Twist
 * ---------------------------------------------------------------------------
 */
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
    /* singularity: rotation by 180 degree */
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
  /* formula & proof: */
  /* http://www.euclideanspace.com/maths/geometry/rotations/for/decomposition */
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

