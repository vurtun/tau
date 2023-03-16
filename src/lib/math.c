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
#define eerp(r,a,b,t) math_pow(a,1-t) * math_pow(b,t) // useful for scaling or zooming
#define eerp_inv(a,b,v) (math_log((a)/(v))/math_log((a)/(b)))
#define repeat(v,len) clamp(0.0f, (v) - math_floor((v)/(len)) * (len), len) // repeats the given value in the interval specified by len
#define pingpong(t,len) len - math_abs(repat(t,len * 2f)-len) // repeats a value within a range, going back and forth
#define triwave(t,period) (1.0f-math_abs(2.0f*(((t)/(period))-math_floor(((t)/(period))))-1.0f))
#define smooth01(x) ((x)*(x)*(3-2*(x)))
#define smoother01(x) ((x)*(x)*(x)*(6*(x)*(x)-15*(x)+10))
#define remap(r,imin,imax,omin,omax,v) lerp(r,omin,omax, inv_lerp(imin,imax,v))
// clang-format on

/* ---------------------------------------------------------------------------
 *                                Standard
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
union bit_castqi {
  hflt4 hflt;
  unsigned short u16[4];
};
static float
math_log2(float x) {
  union bit_castf v = {x};
  union bit_casti m = {(v.i & 0x007FFFFF) | 0x3f000000};
  float y = (float)v.i;
  y *= 1.1920928955078125e-7f;
  return y - 124.22551499f - 1.498030302f * m.f -
    1.72587999f / (0.3520887068f + m.f);
}
static float
math_log(float x) {
  return 0.69314718f * math_log2(x);
}
static double
math_abs(double x) {
  union bit_castd u = {x};
  u.i &= -1ULL / 2;
  return u.f;
}
static inline int
cmpN(const float *a, const float *b, float e, int N) {
  int r = 1;
  for (int i = 0; i < N; ++i) {
    r &= (math_abs(a[i] - b[i]) > e);
  }
  return r;
}
static int
math_floori(double x) {
  x = castd((casti(x) - ((x < 0.0f) ? 1 : 0)));
  return casti(x);
}
static int
math_ceili(double x) {
  if (x < 0) {
    int t = casti(x);
    double r = x - castd(t);
    return (r > 0.0f) ? (t + 1) : t;
  } else {
    int i = casti(x);
    return (x > i) ? (i + 1) : i;
  }
}
static inline int
math_flteq(float a, float b, int epsilon) {
  /* http://realtimecollisiondetection.net/blog/?p=89 */
  return math_abs(a - b) < epsilon * max(1.0f, max(math_abs(a), math_abs(b)));
}
static float
math_pow2(float p) {
  float o = (p < 0) ? 1.0f : 0.0f;
  float c = (p < -126) ? -126.0f : p;
  int w = casti(c);
  float t, z = c - castf(w) + o;
  t = (c + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z);
  union bit_casti v = {castu(((1 << 23) * t))};
  return v.f;
}
static float
math_exp(float p) {
  return math_pow2(1.442695040f * p);
}
static float
math_pow(float x, float p) {
  return math_pow2(p * math_log2(x));
}
static inline float
math_rsqrt(float x) {
  return cpu_rsqrt(x);
}
static inline float
math_sqrt(float x) {
  return cpu_sqrt(x);
}
static inline float
math_rsqrtp(float x) {
  /* Exact bits: 23.62 */
  /* δ+max = 7.362378×10^−8 */
  /* δ−max = −7.754203×10^−8 */
  union { float f; int i; } v = {x};
  int k = v.i & 0x00800000;
  float y = 0.0f;
  if (k != 0) {
    v.i = 0x5ed9dbc6 - (v.i >> 1);
    y = v.f;
    y = 2.33124018f*y*cpu_fma(-x, y*y, 1.07497406f);
  } else {
    v.i = 0x5f19d200 - (v.i >> 1);
    y = v.f;
    y = 0.824212492f*y*cpu_fma(-x, y*y, 2.14996147f);
  }
  {
    float c = x*y;
    float r = cpu_fma(y, -c, 1.0f);
    return cpu_fma(0.5f*y, r, y);
  }
}
static inline float
math_sqrtp(float x) {
  /* Exact bits: 23.4 */
  /* δ+max = 8.757966×10^−8 */
  /* δ−max = −9.037992×10^−8 */
  union { float f; int i; } v = {x};
  int k = v.i & 0x00800000;
  float y, c;
  if (k != 0) {
    v.i = 0x5ed9d098 - (v.i >> 1);
    y = v.f;
    c = x*y;
    y = 2.33139729f*c*cpu_fma(y, -c, 1.07492042f);
  } else {
    v.i = 0x5f19d352 - (v.i >> 1);
    y = v.f;
    c = x*y;
    y = 0.82420468f*c*cpu_fma(y, -c, 2.14996147f);
  }
  {
    float l = x*y;
    float r = cpu_fma(y, -l, 1.0f);
    return cpu_fma(0.5f*l, r, l);
  }
}
static double
math_round(double x) {
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
math_roundi(double x) {
  double f = math_round(x);
  return casti(f);
}
static float
math_floor(float x) {
  union {float f; unsigned i;} u = {x};
  int e = (int)(u.i >> 23 & 0xff) - 0x7f;
  unsigned m;
  if (e >= 23) {
    return x;
  }
  if (e >= 0) {
    m = 0x007fffff >> e;
    if ((u.i & m) == 0) {
      return x;
    }
    if (u.i >> 31) {
      u.i += m;
    }
    u.i &= ~m;
  } else {
    if (u.i >> 31 == 0) {
      u.i = 0;
    } else if (u.i << 1) {
      u.f = -1.0;
    }
  }
  return u.f;
}
static float
math_mod(float x, float y) {
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
static inline float
math_rintf(float x) {
  return cpu_rintf(x);
}
static inline float
math_fma(float x, float y, float z) {
  return cpu_fma(x,y,z);
}
static void
math_sin_cos_pi(float *sp, float *cp, float a) {
  int i;
  float c, r, s, az = a * 0.0f; // must be evaluated with IEEE-754 semantics
  /* for |a| > 2**24, cospi(a) = 1.0f, but cospi(Inf) = NaN */
  a = (math_abs(a) < 0x1.0p24f) ? a : az;
  r = math_rintf(a + a); // must use IEEE-754 "to nearest" rounding
  i = casti(r);
  float t = math_fma(-0.5f, r, a);
  /* compute core approximations */
  s = t * t;
  /* Approximate cos(pi*x) for x in [-0.25,0.25] */
  r = 0x1.d9e000p-3f;
  r = math_fma(r, s, -0x1.55c400p+0f);
  r = math_fma(r, s,  0x1.03c1cep+2f);
  r = math_fma(r, s, -0x1.3bd3ccp+2f);
  c = math_fma(r, s,  0x1.000000p+0f);
  /* Approximate sin(pi*x) for x in [-0.25,0.25] */
  r = -0x1.310000p-1f;
  r = math_fma(r, s,  0x1.46737ep+1f);
  r = math_fma(r, s, -0x1.4abbfep+2f);
  r = (t * s) * r;
  s = math_fma(t, 0x1.921fb6p+1f, r);
  if (i & 2) {
    s = 0.0f - s; // must be evaluated with IEEE-754 semantics
    c = 0.0f - c; // must be evaluated with IEEE-754 semantics
  }
  if (i & 1) {
    t = 0.0f - s; // must be evaluated with IEEE-754 semantics
    s = c;
    c = t;
  }
  /* IEEE-754: sinPi(+n) is +0 and sinPi(-n) is -0 for positive integers n */
  if (a == math_floor(a)) {
    s = az;
  }
  *sp = s;
  *cp = c;
}
static float
math_sin_pi(float x) {
  float s = 0.0f;
  float c = 0.0f;
  math_sin_cos_pi(&s, &c, x);
  return s;
}
static float
math_cos_pi(float x) {
  float s = 0.0f;
  float c = 0.0f;
  math_sin_cos_pi(&s, &c, x);
  return c;
}
static void
math_sin_cos(float *vsin, float *vcos, float a) {
  assert(vsin);
  assert(vcos);
  /* Implementation based on sinf.c from the cephes library, combines sinf
   * and cosf in a single function. Original implementation by
   * Stephen L. Moshier (See: http://www.moshier.net/) */
  union bit_castf u = {.f = a};
  // Make arg positive and safe sign for sin (cos is symmetric around x (highest bit of a float is the sign bit)
  unsigned sin_sign = u.i & 0x80000000U;
  union bit_castf x = {.i = u.i ^ sin_sign};
  // x / (PI / 2) rounded to nearest int gives us the quadrant closest to x
  unsigned quad = castu((0.6366197723675814f * x.f + 0.5f));

  // Make x relative to the closest quadrant.
  // This does x = x - quadrant * PI / 2 using a two step Cody-Waite argument reduction.
  // This improves the accuracy of the result by avoiding loss of significant bits in the subtraction.
  // We start with x = x - quadrant * PI / 2, PI / 2 in hexadecimal notation is 0x3fc90fdb, we remove the lowest 16 bits to
  // get 0x3fc90000 (= 1.5703125) this means we can now multiply with a number of up to 2^16 without losing any bits.
  // This leaves us with: x = (x - quadrant * 1.5703125) - quadrant * (PI / 2 - 1.5703125).
  // PI / 2 - 1.5703125 in hexadecimal is 0x39fdaa22, stripping the lowest 12 bits we get 0x39fda000 (= 0.0004837512969970703125)
  // This leaves uw with: x = ((x - quadrant * 1.5703125) - quadrant * 0.0004837512969970703125) - quadrant * (PI / 2 - 1.5703125 - 0.0004837512969970703125)
  // See: https://stackoverflow.com/questions/42455143/sine-cosine-modular-extended-precision-arithmetic
  // After this we have x in the range [-PI / 4, PI / 4].
  float flt_quad = quad;
  x.f = ((x.f - flt_quad * 1.5703125f) -
        flt_quad * 0.0004837512969970703125f) -
        flt_quad * 7.549789948768648e-8f;
  // Calculate x2 = x^2
  float x2 = x.f * x.f;

  // Taylor expansion:
  // cos(x) = 1 - x^2/2! + x^4/4! - x^6/6! + x^8/8! + ... = (((x2/8!- 1/6!) * x2 + 1/4!) * x2 - 1/2!) * x2 + 1
  float tay_cos = ((2.443315711809948e-5f * x2 - 1.388731625493765e-3f) * x2 + 4.166664568298827e-2f) * x2 * x2 - 0.5f * x2 + 1.0f;
  // sin(x) = x - x^3/3! + x^5/5! - x^7/7! + ... = ((-x2/7! + 1/5!) * x2 - 1/3!) * x2 * x + x
  float tay_sin = ((-1.9515295891e-4f * x2 + 8.3321608736e-3f) * x2 - 1.6666654611e-1f) * x2 * x.f + x.f;

  // The lowest 2 bits of quadrant indicate the quadrant that we are in.
  // Let x be the original input value and x' our value that has been mapped to the range [-PI / 4, PI / 4].
  // since cos(x) = sin(x - PI / 2) and since we want to use the Taylor expansion as close as possible to 0,
  // we can alternate between using the Taylor expansion for sin and cos according to the following table:
  //
  // quadrant  sin(x)    cos(x)
  // XXX00b  sin(x')   cos(x')
  // XXX01b  cos(x')  -sin(x')
  // XXX10b -sin(x')  -cos(x')
  // XXX11b -cos(x')   sin(x')
  //
  // So: sin_sign = bit2, cos_sign = bit1 ^ bit2, bit1 determines if we use sin or cos Taylor expansion
  unsigned bit1 = quad << 31u;
  unsigned bit2 = (quad << 30u) & 0x80000000u;

  union bit_castf s = {.f = bit1 ? tay_cos : tay_sin};
  union bit_castf c = {.f = bit1 ? tay_sin : tay_cos};

  sin_sign = sin_sign ^ bit2;
  unsigned cos_sign = bit1 ^ bit2;

  s.i ^= sin_sign;
  c.i ^= cos_sign;

  *vsin = s.f;
  *vcos = c.f;
}
static float
math_sin(float x) {
  float s = 0.0f;
  float c = 0.0f;
  math_sin_cos(&s, &c, x);
  return s;
}
static float
math_cos(float x) {
  float s = 0.0f;
  float c = 0.0f;
  math_sin_cos(&s, &c, x);
  return c;
}
static float
math_tan(float a) {
  /* Implementation based on tanf.c from the cephes library,
   * original implementation by Stephen L. Moshier (See: http://www.moshier.net/) */
  union bit_castf u = {.f = a};
  unsigned tan_sign = (u.i & 0x80000000u);
  union bit_castf x = {.i = u.i ^ tan_sign};
  /* x / (PI / 2) rounded to nearest int gives us the quadrant closest to x */
  unsigned quad = castu(0.6366197723675814f * x.f + 0.5f);
  /* remap x to range [-PI / 4, PI / 4], see sin cos */
  float flt_quad = castf(quad);
  x.f = ((x.f - flt_quad * 1.5703125f) - flt_quad * 0.0004837512969970703125f) - flt_quad * 7.549789948768648e-8f;
  float x2 = x.f * x.f;
  // roughly equivalent to the Taylor expansion:
  // tan(x) = x + x^3/3 + 2*x^5/15 + 17*x^7/315 + 62*x^9/2835 + ...
  float tan = (((((9.38540185543e-3f * x2 + 3.11992232697e-3f) * x2 + 2.44301354525e-2f) * x2 +
      5.34112807005e-2f) * x2 + 1.33387994085e-1f) * x2 + 3.33331568548e-1f) * x2 * x.f + x.f;
  /* For the 2nd and 4th quadrant we need to invert the value */
  unsigned bit1 = quad << 31u;
  /* add small epsilon to prevent div by zero, works because tan is always positive */
  union bit_castf r = {.f = bit1 ? -1.0f / (tan + FLT_MIN) : tan};
  union bit_castf t = {.i = r.i ^ tan_sign};
  return t.f;
}
static float
math_asin(float v) {
  /* Implementation based on asinf.c from the cephes library
   * Original implementation by Stephen L. Moshier (See: http://www.moshier.net/) */
  // Make argument positive
  union bit_castf u = {.f = v};
  unsigned asin_sign = (u.i & 0x80000000u);
  union bit_castf a = {.i = u.i ^ asin_sign};
  // asin is not defined outside the range [-1, 1] but it often happens that a value is slightly above 1 so we just clamp here
  a.f = min(1.0f, a.f);
  // When |x| <= 0.5 we use the asin approximation as is
  float z1 = a.f * a.f;
  float x1 = a.f;
  // When |x| > 0.5 we use the identity asin(x) = PI / 2 - 2 * asin(sqrt((1 - x) / 2))
  float z2 = 0.5f * (1.0f - a.f);
  float x2 = math_sqrt(z2);
  // Select which of the two situations we have
  union bit_castf z = {.f = a.f > 0.5f ? z2 : z1};
  float x = a.f > 0.5f ? x2 : x1;
  // Polynomial approximation of asin
  z.f = ((((4.2163199048e-2f * z.f + 2.4181311049e-2f) * z.f + 4.5470025998e-2f) * z.f + 7.4953002686e-2f) * z.f + 1.6666752422e-1f) * z.f * x + x;
  // If |x| > 0.5 we need to apply the remainder of the identity above
  z.f = a.f > 0.5f ? (0.5f * FLT_PI) - (z.f + z.f) : z.f;
  // Put the sign back
  union bit_castf t = {.i = z.i ^ asin_sign};
  return t.f;
}
static float
math_acos(float x) {
  // Not the most accurate, but simple
  return (0.5f * FLT_PI) - math_asin(x);
}
static float
math_atan(float v) {
  // Implementation based on atanf.c from the cephes library
  // Original implementation by Stephen L. Moshier (See: http://www.moshier.net/)
  union bit_castf u = {.f = v};
  unsigned atan_sign = (u.i & 0x80000000u);
  union bit_castf x = {.i = u.i ^ atan_sign};
  float y = 0.0f;

  // If x > Tan(PI / 8)
  int gt1 = x.f > 0.4142135623730950f;
  float x1 = (x.f - 1.0f) / (x.f + 1.0f);

  // If x > Tan(3 * PI / 8)
  int gt2 = x.f > 2.414213562373095f;
  float x2 = -1.0f / (x.f + FLT_MIN); // Add small epsilon to prevent div by zero, works because x is always positive

  // Apply first if
  x.f =  gt1 ? x1 : x.f;
  y = gt1 ? 0.25 * FLT_PI : y;

  // Apply second if
  x.f = gt2 ? x2 : x.f;
  y = gt2 ? 0.5f * FLT_PI : y;

  // Polynomial approximation
  float z = x.f * x.f;
  y += (((8.05374449538e-2f * z - 1.38776856032e-1f) * z + 1.99777106478e-1f) * z - 3.33329491539e-1f) * z * x.f + x.f;

  // Put the sign back
  union bit_castf t = {.f = y};
  t.i ^= atan_sign;
  return t.f;
}
static float
math_atan2(float in_y, float in_x) {
  // determine absolute value and sign of y
  union bit_castf uy = {.f = in_y};
  unsigned y_sign = (uy.i & 0x80000000u);
  union bit_castf y_abs = {.i = uy.i ^ y_sign};

  union bit_castf ux = {.f = in_x};
  unsigned x_sign = (ux.i & 0x80000000u);
  union bit_castf x_abs = {.i = ux.i ^ x_sign};

  // always divide smallest / largest to avoid dividing by zero
  int x_is_numerator = x_abs.f < y_abs.f;
  float num = x_is_numerator ? x_abs.f : y_abs.f;
  float den = x_is_numerator ? y_abs.f : x_abs.f;
  float atn = math_atan(num / den);
  // if we calculated x / y instead of y / x the result is
  // PI / 2 - result (note that this is true because we know the result is
  // positive because the input was positive)
  atn = x_is_numerator ? (0.5f * FLT_PI) - atn : atn;
  // Now we need to map to the correct quadrant
  // x_sign y_sign  result
  // +1   +1    atan
  // -1   +1    -atan + PI
  // -1   -1    atan - PI
  // +1   -1    -atan
  // This can be written as: x_sign * y_sign * (atan - (x_sign < 0? PI : 0))
  union bit_castf tpi = {.f = FLT_PI};
  union bit_castf u = {.i = (x_sign >> 31) & tpi.i};
  union bit_castf s = {.i = x_sign ^ y_sign};
  union bit_castf t = {.f = atn - u.f};
  t.i = t.i ^ s.i;
  return t.f;
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
#define opNs(r,e,a,p,s,N)\
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
#define dotN(r,a,b,N)\
  do {r = 0; for (int uniqid(_i_) = 0; uniqid(_i_) < N; ++uniqid(_i_))\
      r += (a)[uniqid(_i_)] * (b)[uniqid(_i_)];\
  } while(0);
#define lenN(r,a,N)\
  do {float uniqid(_l_); dotN(uniqid(_l_),a,a,N);\
    (r) = math_sqrt(uniqid(_l_));} while(0);

#define minN(r,a,b,N) map2N(r,min,a,b,N)
#define maxN(r,a,b,N) map2N(r,max,a,b,N)
#define clampN(r,i,v,x,N) map3N(r,clamp,i,v,x,N)

#define op2(r,e,a,p,b,i,s) opN(r,e,a,p,b,i,s,2)
#define op2s(r,e,a,p,s) opNs(r,e,a,p,s,2)
#define op3(r,e,a,p,b,i,s) opN(r,e,a,p,b,i,s,3)
#define op3s(r,e,a,p,s) opNs(r,e,a,p,s,3)
#define op4(r,e,a,p,b,i,s) opN(r,e,a,p,b,i,s,4)
#define op4s(r,e,a,p,s) opNs(r,e,a,p,s,4)

/* vector 2D */
#define set2(d,x,y)     (d)[0] = x, (d)[1] = y
#define zero2(d)        set2(d,0,0)
#define dup2(d,f)       set2(d,f,f)
#define cpy2(d, s)      (d)[0] = (s)[0], (d)[1] = (s)[1]
#define add2(d,a,b)     op2(d,=,a,+,b,+,0)
#define sub2(d,a,b)     op2(d,=,a,-,b,+,0)
#define mul2(d,a,s)     op2s(d,=,a,*,s)
#define dot2(a,b)       ((a)[0]*(b)[0]+(a)[1]*(b)[1])
#define len2(v)         math_sqrt(dot2(v,v))
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
#define len3(v)         math_sqrt(dot3(v,v))
#define adds3(d,a,b,s)  op3(d,=,a,+,b,*,s)
#define subs3(d,a,b,s)  op3(d,=,a,-,b,*,s)
#define norm3(n,v)      mul3(n,v,math_rsqrt(dot3(v, v)))
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
static inline float
box3(const float *a, const float *b, const float *c) {
  float n[3];
  cross3(n, a, b);
  return dot3(n, c);
}

/* vector 4D */
#define set4(v,x,y,z,w) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z),(v)[3] =(w)
#define set4w(d,s,w)    (d)[0]=(s)[0],(d)[1]=(s)[1],(d)[2]=(s)[2],(d)[3]=w
#define dup4(d,f)       set4(d,f,f,f,f)
#define cpy4(d,s)       (d)[0]=(s)[0],(d)[1]=(s)[1],(d)[2]=(s)[2],(d)[3]=(s)[3]
#define add4(d,a,b)     op4(d,=,a,+,b,+,0)
#define sub4(d,a,b)     op4(d,=,a,-,b,+,0)
#define mul4(d,a,s)     op4s(d,=,a,*,s)
#define dot4(a,b)       ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2]+(a)[3]*(b)[3])
#define len4(v)         math_sqrt(dot4(v,v))
#define adds4(d,a,b,s)  op4(d,=,a,+,b,*,s)
#define subs4(d,a,b,s)  op4(d,=,a,-,b,*,s)
#define norm4(n,v)      mul4(n,v,math_rsqrt(dot4(v, v)))
#define normaleq4(v)    norm4(v,v)
#define lerp4(r,a,b,t)  lerpN(r,a,b,t,4)
#define cmp4(a,b,e)     cmpN(a,b,e,4)
#define min4(r,a,b)     minN(r,a,b,4)
#define max4(r,a,b)     maxN(r,a,b,4)
#define map4(r,fn,a)    mapN(r,fn,a,4);
#define clamp4(r,i,v,x) clampN(r,i,v,x,4)
#define swzl4(r,f,a,b,c,e) set4(r,(f)[a],(f)[b],(f)[c],(f)[e])
#define qid(q)          set4(q,0,0,0,1)
#define qconj(d,s)      do{mul3(d,s,-1.0f);(d)[3]=(s)[3];}while(0)
#define qinv(d,s)       qconj(d,s);
#define quat(q,n,x)     quatf(q,n,(x)[0],(x)[1],(x)[2])
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
  return math_cos(dot/math_sqrt(n));
}

/* ---------------------------------------------------------------------------
 *                                  Color
 * ---------------------------------------------------------------------------
 */
#define col_get(u) \
  (struct color) { col_r(u), col_g(u), col_b(u), col_a(u) }
#define col_unpaq(d,u) set4(d, col_r(u), col_g(u), col_b(u), col_a(u))
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
#define col_rgbav(a) col_rgba((a)[0],(a)[1],(a)[2],(a)[3])
#define col_rgbv(a) col_rgba((a)[0],(a)[1],(a)[2],0xff)

#define byte_flt(x) clamp(0u, castb(0.5f + 255.0f * (x)), 255u)
#define flt_byte(x) (castf((x)) * (1.0f/255.0f))

static float
col_srgb_linear(float x) {
  float v = 0.0f;
  if (x <= 0.0031308f) {
    v = 12.92f * x;
  } else {
    v = (1.0f + 0.055f) * math_pow(x, 1.0f / 2.0f) - 0.055f;
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
    return math_pow((x + 0.055f )/(1.0f + 0.055f), 2.4f);
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
    hsv[0] = math_mod(hsv[0] / 6.0f, 1.0f);
  }
  cpy3(hsv_out, hsv);
}
static void
col_rgb_hue(float *restrict rgb_out, float hue) {
  const float rgb[3] = {
    clamp01((float)math_abs(hue * 6.0f - 3.0f) - 1.0f),
    clamp01(2.0f - (float)math_abs(hue * 6.0f - 2.0f)),
    clamp01(2.0f - (float)math_abs(hue * 6.0f - 4.0f))
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
 *                                Matrix
 * ---------------------------------------------------------------------------
 */
static void
m3x3q(float *restrict m, const float *restrict q) {
  float x2 = q[0] + q[0];
  float y2 = q[1] + q[1];
  float z2 = q[2] + q[2];

  float xx = q[0]*x2;
  float xy = q[0]*y2;
  float xz = q[0]*z2;

  float yy = q[1]*y2;
  float yz = q[1]*z2;
  float zz = q[2]*z2;

  float wx = q[3]*x2;
  float wy = q[3]*y2;
  float wz = q[3]*z2;

  m[0*3+0] = 1.0f - (yy + zz);
  m[0*3+1] = xy - wz;
  m[0*3+2] = xz + wy;

  m[1*3+0] = xy + wz;
  m[1*3+1] = 1.0f - (xx + zz);
  m[1*3+2] = yz - wx;

  m[2*3+0] = xz - wy;
  m[2*3+1] = yz + wx;
  m[2*3+2] = 1.0f - (xx + yy);
}
static void
m3x3T(float *restrict t, const float *restrict m) {
  t[3*0+0] = m[3*0+0], t[3*0+1] = m[3*1+0], t[3*0+2] = m[3*2+0];
  t[3*1+0] = m[3*0+1], t[3*1+1] = m[3*1+1], t[3*1+2] = m[3*2+1];
  t[3*2+0] = m[3*0+2], t[3*2+1] = m[3*1+2], t[3*2+2] = m[3*2+2];
}
static void
mul3x3(float *restrict d, const float *restrict a, const float *restrict b) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      *d++ = a[0] * b[0*3+j] + a[1] * b[1*3+j] + a[2] * b[2*3+j];
    }
    a += 3;
  }
}

/* ---------------------------------------------------------------------------
 *                                Quaternion
 * ---------------------------------------------------------------------------
 */
static void
quatf(float *restrict q, float angle, float x, float y, float z) {
  assert(q);
  float s, c; math_sin_cos(&s, &c, angle * 0.5f);
  q[0] = x * s, q[1] = y * s;
  q[2] = z * s, q[3] = c;
}
static void
qrot3(float *restrict out, const float *restrict qrot,
      const float *restrict vec) {
  assert(out);
  assert(qrot);
  assert(vec);

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
  assert(q);
  assert(from3);
  assert(to3);

  float w[3] = {0};
  float u[3]; cpy3(u,from3);
  float v[3]; cpy3(v,to3);
  float norm_u_norm_v = math_sqrt(dot3(u,u) * dot3(v,v));
  float real_part = norm_u_norm_v + dot3(u,v);
  if (real_part < (1.e-6f * norm_u_norm_v)) {
    if (math_abs(u[0]) > math_abs(u[2])) {
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
  assert(qout);
  float sr, cr; math_sin_cos(&sr, &cr, roll * 0.5f);
  float sp, cp; math_sin_cos(&sp, &cp, pitch * 0.5f);
  float sy, cy; math_sin_cos(&sy, &cy, yaw * 0.5f);

  float ret[4];
  ret[0] = cy * sp * cr + sy * cp * sr;
  ret[1] = sy * cp * cr - cy * sp * sr;
  ret[2] = cy * cp * sr - sy * sp * cr;
  ret[3] = cy * cp * cr + sy * sp * sr;
  cpy4(qout, ret);
}
static void
eulerq(float *restrict out3, const float *restrict qin) {
  assert(out3);
  assert(qin);

  float q[4]; cpy4(q, qin);
  float y_sq = q[1] * q[1];
  float t0 = 2.0f * (q[3] * q[0] + q[1] * q[2]);
  float t1 = 1.0f - 2.0f * (q[0] * q[0] + y_sq);
  float t2 = 2.0f * (q[3] * q[1] - q[2] * q[0]);
  t2 = t2 > 1.0f ? 1.0f : t2;
  t2 = t2 < -1.0f ? -1.0f : t2;
  float t3 = 2.0f * (q[3] * q[2] + q[0] * q[1]);
  float t4 = 1.0f - 2.0f * (y_sq + q[2] * q[2]);
  float v[3] = {math_atan2(t0, t1), math_asin(t2), math_atan2(t3, t4)};
  cpy3(out3, v);
}
static void
qtransform(float *restrict o3, const float *restrict in3,
           const float *restrict qrot, const float *restrict mov3) {
  assert(o3);
  assert(in3);
  assert(qrot);
  assert(mov3);

  float v3[3]; cpy3(v3, in3);
  float q[4]; cpy4(q, qrot);
  float t3[3]; cpy3(t3, mov3);
  float tmp[3]; qrot3(tmp, q, v3);
  float ret[3]; add3(ret, tmp, t3);
  cpy3(o3, ret);
}
static void
qtransformI(float *restrict o3, const float *restrict in3,
            const float *restrict qrot, const float *restrict mov3) {
  assert(o3);
  assert(in3);
  assert(qrot);
  assert(mov3);

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
  assert(o3);
  assert(q);
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
  assert(q);
  return 2.0f * math_acos(clamp(-1.0f, q[3], 1.0f));
}
static void
qpow(float *restrict qo, const float *restrict q, float exp) {
  assert(qo);
  assert(q);

  float axis[3]; qaxis(axis, q);
  float angle = qangle(q) * exp;
  float ret[4]; quat(ret, angle, axis);
  cpy4(qo, ret);
}
static void
qintegrate(float *restrict qo, const float *restrict qq,
           const float *restrict qv, float dt) {
  assert(qo);
  assert(qq);
  assert(qv);

  float p[4]; qpow(p, qv, dt);
  float r[4]; qmul(r, p, qq);
  cpy4(qo, r);
}
static void
qintegratev(float *restrict qo, const float *restrict qrot,
            float *restrict angle_vel3, float dt) {
  assert(qo);
  assert(qrot);
  assert(angle_vel3);
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
qnlerp(float *restrict qres, const float *restrict qfrom,
       const float *restrict qto, float t) {
  assert(qres);
  assert(qfrom);
  assert(qto);

  float b[4]; cpy4(b, qto);
  float dot = dot4(qfrom, qto);
  float cosT = clamp(-1.0f, dot, 1.0f);
  if (cosT < 0.0f) {
    /* flip quaternions if on opposite hemispheres */
    cosT = -cosT;
    mul4(b,b,-1.0f);
  }
  float beta = 1.0f - t;
  if (cosT < (1.0f - FLT_EPSILON)) {
    /* if the quats are far apart, do spherical interpolation. */
    float theta = math_acos(cosT);
    float cosec_theta = 1.0f / math_sin(theta);
    beta = cosec_theta * math_sin(beta * theta);
    t = cosec_theta * math_sin(t * theta);
  }
  float q[4];
  q[0] = beta * qfrom[0] + t * b[0];
  q[1] = beta * qfrom[1] + t * b[1];
  q[2] = beta * qfrom[2] + t * b[2];
  q[3] = beta * qfrom[3] + t * b[3];
  norm4(qres, q);
}
static void
qslerp(float *restrict qres, const float *restrict qfrom,
       const float *restrict qto, float t) {
  assert(qres);
  assert(qfrom);
  assert(qto);
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
    float sinom = math_rsqrt(scale0);
    float omega = math_atan2(scale0 * sinom, cosom);
    scale0 = math_sin((1.0f - t) * omega) * sinom;
    scale1 = math_sin(t * omega) * sinom;
  } else {
    scale0 = 1.0f - t;
    scale1 = t;
  }
  float a[4]; mul4(a,qfrom,scale0);
  float b[4]; mul4(b,tmp,scale1);
  add4(qres,a,b);
}
static void
qdiag(float *restrict qres, const float *restrict A) {
  assert(qres);
  assert(A);
  /* Symmetric Matrix 3x3 Diagonalizer: http://melax.github.io/diag.html
   * -------------------------------------------------------------------
    'A' must be a symmetric matrix.
    Returns quaternion q such that corresponding matrix Q
    can be used to Diagonalize 'A'

    Diagonal matrix D = Q * A * Transpose(Q);  and  A = QT*D*Q
    The rows of Q are the eigenvectors, D's diagonal is the eigenvalues
    As per 'row' convention if float3x3 Q = q.getmatrix(); then v*Q = q*v*conj(q)
  */
  float q[4]; qid(q);
  for (int i = 0; i < 8; ++i) {
    float Q[9]; m3x3q(Q, q); /* v*Q == q*v*conj(q) */
    float QT[9]; m3x3T(QT,Q);
    float QA[9]; mul3x3(QA,Q,A);
    float D[9]; mul3x3(D,QA,QT); /* A = Q^T*D*Q */
    float offdiag[3]; set3(offdiag, D[1*3+2], D[0*3+2], D[0*3+1]);
    float om[3]; map3(om, (float)math_abs, offdiag);
    int k = (om[0] > om[1] && om[0] > om[2]) ? 0: (om[1] > om[2]) ? 1 : 2;
    int k1 = (k+1)%3;
    int k2 = (k+2)%3;
    if (offdiag[k]==0.0f) {
      break;  /* diagonal already */
    }
    float sthet = (D[k2*3+k2]-D[k1*3+k1])/(2.0f*offdiag[k]);
    float sgn = (sthet > 0.0f)?1.0f:-1.0f;
    float thet = sthet * sgn; /* make it positive */
    float t = sgn /(thet +((thet < 1.E6f)?math_sqrt(thet*thet+1.0f):thet));
    float c = 1.0f/math_sqrt(t*t+1.0f); /* c= 1/(t^2+1) , t=s/c */
    if (c==1.0f) {
      break;  /* no room for improvement - reached machine precision. */
    }
    float jr[4] = {0.0f};
    /* using 1/2 angle identity sin(a/2) = sqrt((1-cos(a))/2) */
    jr[k] = sgn*math_sqrt((1.0f-c)/2.0f);
    jr[k] *= -1.0f; /* since our quat-to-mat conv was for v*M instead of M*v */
    jr[3] = math_sqrt(1.0f - jr[k]*jr[k]);
    if(jr[3] == 1.0f) {
      break; /* reached limits of floating point precision */
    }
    qmul(q,q,jr);
    normaleq4(q);
  }
  cpy4(qres, q);
}
static void
eigensym(float *e_val, float *e_vec33, const float *a33) {
  assert(e_val);
  assert(e_vec33);
  assert(a33);

  /* calculate eigensystem from symmetric 3x3 matrix */
  float q[4]; qdiag(q, a33);
  float Q[9]; m3x3q(Q, q);
  float QT[9]; m3x3T(QT, Q);
  float QA[9]; mul3x3(QA, Q, a33);
  float D[9]; mul3x3(D, QA, QT);
  float ev[3]; set3(ev, D[0*3+0], D[1*3+1], D[2*3+2]);

  /* 3-way sort eigenvalues and eigenvectors */
  float aev[3]; map3(aev,(float)math_abs,ev);
  if (aev[0] >= aev[1] && aev[1] >= aev[2]) {
    cpy3(e_val, ev);
    mcpy(e_vec33, Q, szof(Q));
  } else if (aev[1] >= aev[0] && aev[0] >= aev[2]) {
    e_val[0] = ev[1];
    e_val[1] = ev[0];
    e_val[2] = ev[2];

    cpy3(e_vec33, Q+3);
    cpy3(e_vec33+3, Q+0);
    cpy3(e_vec33+6, Q+6);
  } else if (aev[2] >= aev[0] && aev[0] >= aev[1]) {
    e_val[0] = ev[2];
    e_val[1] = ev[0];
    e_val[2] = ev[1];

    cpy3(e_vec33, Q+6);
    cpy3(e_vec33+3, Q+0);
    cpy3(e_vec33+6, Q+3);
  } else if (aev[0] >= aev[1] && aev[2] >= aev[1]) {
    e_val[0] = ev[0];
    e_val[1] = ev[2];
    e_val[2] = ev[1];

    cpy3(e_vec33, Q+0);
    cpy3(e_vec33+3, Q+6);
    cpy3(e_vec33+6, Q+3);
  } else if (aev[2] >= aev[1] && aev[1] >= aev[0]) {
    e_val[0] = ev[2];
    e_val[1] = ev[1];
    e_val[2] = ev[0];

    cpy3(e_vec33, Q+6);
    cpy3(e_vec33+3, Q+3);
    cpy3(e_vec33+6, Q+0);
  } else if (aev[1] >= aev[2] && aev[2] >= aev[0]) {
    e_val[0] = ev[1];
    e_val[1] = ev[2];
    e_val[2] = ev[0];

    cpy3(e_vec33, Q+3);
    cpy3(e_vec33+3, Q+6);
    cpy3(e_vec33+6, Q+0);
  }
}

/* ---------------------------------------------------------------------------
 *                                Swing-Twist
 * ---------------------------------------------------------------------------
 */
static void
qtwist(float *restrict qtwist, const float *restrict qin,
       const float *restrict axis) {
  assert(qtwist);
  assert(qin);
  assert(axis);

  float q[4]; cpy4(q, qin);
  float d = dot3(q,axis);
  float t[4]; mul3(t,axis,d); t[3] = q[3];
  float l2 = dot4(t,t);
  if (l2 == 0.0f) {
    set4(qtwist, 0,0,0,1);
    return;
  }
  float l = math_rsqrt(l2);
  mul4(t,t,l);
  cpy4(qtwist,t);
}
static void
qst_get(float *restrict qswing, float *restrict qtwist,
        const float *restrict qin){
  assert(qswing);
  assert(qtwist);
  assert(qin);

  float q[4]; cpy4(q,qin);
  float s = math_sqrt((q[3]*q[3]) + (q[0]*q[0]));
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
  assert(qswing);
  assert(qtwist);
  assert(q);
  assert(axis3);

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
    /* always twist 180 degree on singularity */
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
  assert(qfull_swing);
  assert(qfull_twist);
  assert(qa);
  assert(qb);
  assert(twist_axis3);

  float inv[4]; qconj(inv, qa);
  float q[4]; qmul(q, qb, inv);
  qst__decomp(qfull_swing, qfull_twist, q, twist_axis3);
}
static void
qst__nlerp(float *restrict qres, float *restrict qswing, float *restrict qtwist,
           const float *restrict qfull_swing, const float *restrict qfull_twist,
           float t_swing, float t_twist) {
  assert(qres);
  assert(qswing);
  assert(qtwist);
  assert(qfull_swing);
  assert(qfull_twist);

  static const float qid[4] = {0,0,0,1};
  lerp4(qswing, qid, qfull_swing, t_swing);
  lerp4(qtwist, qid, qfull_twist, t_twist);
  qmul(qres, qtwist, qswing);
}
static void
qst_nlerp(float *restrict qres, const float *restrict qfull_swing,
          const float *restrict qfull_twist, float t_swing, float t_twist) {
  assert(qres);
  assert(qfull_swing);
  assert(qfull_twist);
  float swing[4], twist[4];
  qst__nlerp(qres, swing, twist, qfull_swing, qfull_twist, t_swing, t_twist);
}
static void
qst__slerp(float *restrict qres, float *restrict qswing, float *restrict qtwist,
           const float *restrict qfull_swing, const float *restrict qfull_twist,
           float t_swing, float t_twist) {
  assert(qres);
  assert(qswing);
  assert(qtwist);
  assert(qfull_swing);
  assert(qfull_twist);

  static const float qid[4] = {0,0,0,1};
  qslerp(qswing, qid, qfull_swing, t_swing);
  qslerp(qtwist, qid, qfull_twist, t_twist);
  qmul(qres, qtwist, qswing);
}
static void
qst_slerp(float *restrict qres, const float *restrict qfull_swing,
          const float *restrict qfull_twist, float t_swing, float t_twist) {
  assert(qres);
  assert(qfull_swing);
  assert(qfull_twist);
  float swing[4], twist[4];
  qst__slerp(qres, swing, twist, qfull_swing, qfull_twist, t_swing, t_twist) ;
}
/* ---------------------------------------------------------------------------
 *                                Transform
 * --------------------------------------------------------------------------- */
struct transform {
  float pos[3];
  float rot[4];
  float scale[3];
};
static void
transform_id(struct transform *ret) {
  zero3(ret->pos);
  qid(ret->rot);
  set3(ret->scale,1,1,1);
}
static void
transform_set(struct transform *ret, const float *restrict pos,
              const float *restrict rot, const float *restrict scale) {
  assert(ret);
  assert(pos);
  assert(rot);
  assert(scale);

  cpy3(ret->pos, pos);
  cpy4(ret->rot, rot);
  cpy3(ret->scale, scale);
}
static void
transform_pnt(float *restrict ret, const struct transform *tfm,
              const float *restrict v) {
  assert(ret);
  assert(tfm);
  assert(v);

  float sp[3]; op3(sp,=,v,*,tfm->scale,+,0);
  float tp[3]; qrot3(tp, tfm->rot, sp);
  float rp[3]; add3(rp, tp, tfm->pos);
  cpy3(ret, rp);
}
static void
transform_inv(struct transform *ret, const struct transform *src) {
  assert(ret);
  assert(src);

  float inv_scale[3];
  inv_scale[0] = (src->scale[0] == 0.0f) ? 0.0f : 1.0f / src->scale[0];
  inv_scale[1] = (src->scale[1] == 0.0f) ? 0.0f : 1.0f / src->scale[1];
  inv_scale[2] = (src->scale[2] == 0.0f) ? 0.0f : 1.0f / src->scale[2];

  float inv_rot[4]; qinv(inv_rot, src->rot);
  float sp[3]; op3(sp,=,src->pos,*,inv_scale,+,0);
  float tp[3]; qrot3(tp, inv_rot, sp);
  float inv_pos[3]; mul3(inv_pos, tp, -1.0f);
  transform_set(ret, inv_pos, inv_rot, inv_scale);
}
static void
transform_mul(struct transform *restrict ret, const struct transform *restrict a,
              const struct transform *restrict b) {
  assert(ret);
  assert(a);
  assert(b);

  float rot[4]; qmul(rot, a->rot, b->rot);
  float ps[3]; op3(ps,=,a->pos,*,b->scale,+,0);
  float pr[3]; qrot3(pr,b->rot, ps);
  float pos[3]; add3(pos,pr,b->pos);
  float scale[3]; op3(ps,=,a->scale,*,b->scale,+,0);
  transform_set(ret, pos, rot, scale);
}
static void
transform_nlerp(struct transform *ret, const struct transform *a,
                const struct transform *b, float t) {
  assert(ret);
  assert(a);
  assert(b);

  float rot[4]; qnlerp(rot, a->rot, b->rot, t);
  float pos[3]; lerp3(pos, a->pos, b->pos, t);
  float scale[3]; lerp3(scale, a->scale, b->scale, t);
  transform_set(ret, pos, rot, scale);
}
static void
transform_slerp(struct transform *ret, const struct transform *a,
                const struct transform *b, float t) {
  assert(ret);
  assert(a);
  assert(b);

  float rot[4]; qslerp(rot, a->rot, b->rot, t);
  float pos[3]; lerp3(pos, a->pos, b->pos, t);
  float scale[3]; lerp3(scale, a->scale, b->scale, t);
  transform_set(ret, pos, rot, scale);
}
static void
transform_delta(struct transform *ret, const struct transform *from,
                const struct transform *to) {
  assert(ret);
  assert(from);
  assert(to);

  float inv_scale[3];
  inv_scale[0] = (from->scale[0] == 0.0f) ? 0.0f : 1.0f / from->scale[0];
  inv_scale[1] = (from->scale[1] == 0.0f) ? 0.0f : 1.0f / from->scale[1];
  inv_scale[2] = (from->scale[2] == 0.0f) ? 0.0f : 1.0f / from->scale[2];

  float dt_scale[3]; op3(dt_scale,=,to->scale,*,inv_scale,+,0);
  float inv_rot[4]; qinv(inv_rot, from->rot);
  float dt_rot[4]; qmul(dt_rot, to->rot, inv_rot);
  float dt_delta[3]; sub3(dt_delta,to->pos,from->pos);
  float dt_rot_pos[3]; qrot3(dt_rot_pos, inv_rot, dt_delta);
  float dt_pos[3]; op3(dt_pos,=,dt_rot_pos,*,inv_scale,+,0);
  transform_set(ret, dt_pos, dt_rot, dt_scale);
}
static void
transform_compose(float *m44, struct transform *tfm) {
  assert(m44);
  assert(tfm);

  float x2 = tfm->rot[0] + tfm->rot[0];
  float y2 = tfm->rot[1] + tfm->rot[1];
  float z2 = tfm->rot[2] + tfm->rot[2];

  float xx = tfm->rot[0]*x2;
  float xy = tfm->rot[0]*y2;
  float xz = tfm->rot[0]*z2;

  float yy = tfm->rot[1]*y2;
  float yz = tfm->rot[1]*z2;
  float zz = tfm->rot[2]*z2;

  float wx = tfm->rot[3]*x2;
  float wy = tfm->rot[3]*y2;
  float wz = tfm->rot[3]*z2;

  float m[4][4] = {0};
  m[0][0] = 1.0f - (yy + zz);
  m[0][1] = xy - wz;
  m[0][2] = xz + wy;
  m[0][3] = 0.0f;

  m[1][0] = xy + wz;
  m[1][1] = 1.0f - (xx + zz);
  m[1][2] = yz - wx;
  m[1][3] = 0.0f;

  m[2][0] = xz - wy;
  m[2][1] = yz + wx;
  m[2][2] = 1.0f - (xx + yy);
  m[2][3] = 0.0f;

  mul3(m[0],m[0],tfm->scale[0]);
  mul3(m[1],m[1],tfm->scale[1]);
  mul3(m[2],m[2],tfm->scale[2]);

  m[3][0] = tfm->pos[0];
  m[3][1] = tfm->pos[1];
  m[3][2] = tfm->pos[2];
  m[3][3] = 1.0f;
  mcpy(m44,m,szof(m));
}

/* ---------------------------------------------------------------------------
 *                                Bspline
 * ---------------------------------------------------------------------------
 */
#define bspline_linear_len(ctrl_cnt) castf(((ctrl_cnt-1)/3))
#define bspline_is_key(ctrl_idx) ((ctrl_idx % 3) == 0u)
#define bspline_is_after_key(pnt_idx) ((ctrl_idx % 3) == 1u)
#define bspline_is_before_key(pnt_idx) ((ctrl_idx % 3) == 2u)

#define bspline_arc_len2(cvs,cnt)\
  bspline_arc_len(cvs,cnt,2)
#define bspline_pos2(pos, linear_pos, cvs, cnt, is_closed)\
  bspline_pos(pos, linear_pos, cvs, cnt, is_closed,2)
#define bspline_tangent2(tangetn, linear_pos, cvs, cnt, is_closed)\
  bspline_tangent3(tangetn, linear_pos, cvs, cnt, is_closed,2)

#define bspline_arc_len3(cvs,cnt)\
  bspline_arc_len(cvs,cnt,3)
#define bspline_pos3(pos, linear_pos, cvs, cnt, is_closed)\
  bspline_pos(pos, linear_pos, cvs, cnt, is_closed,3)
#define bspline_tangent3(tangetn, linear_pos, cvs, cnt, is_closed)\
  bspline_tangent3(tangetn, linear_pos, cvs, cnt, is_closed,3)

static float
bspline__arc_len(const float *restrict p0, const float *restrict p1,
                 const float *restrict p2, const float *restrict p3, int dim) {
  assert(p0);
  assert(p1);
  assert(p2);
  assert(p3);
  assert(dim > 0);

  float d[dim]; opN(d,=,p0,-,p3,+,0,dim);
  float len; lenN(len, d, dim);
  float a[dim]; opN(a,=,p0,-,p1,+,0,dim);
  float b[dim]; opN(b,=,p1,-,p2,+,0,dim);
  float c[dim]; opN(c,=,p2,-,p3,+,0,dim);

  float a_len; lenN(a_len, a, dim);
  float b_len; lenN(b_len, b, dim);
  float c_len; lenN(c_len, c, dim);
  float o_len = a_len + b_len + c_len;
  if (o_len < 0.001f || (o_len / len) <= 1.001f) {
    return (o_len + len) * 0.5f;
  }
  /* Subdivide the given spline.
    Use the following scheme :
    *---h---*
    |  / \  |
    | #-+-# |
    x/     \x
    |       |
    *       *

    * - original control vertices (p0 - p3, input)
    h - helper control vertex at half between p1 and p2
    x - at half between p0 and p1 (np1, control vertex of output) and half
        between p2 and p3 (np5, control vertex of output)
    # - at half between x and h (np2 and np4, control vertices of output)
    + - at half between np2 and np4 (np3, control vertex of output)
  */
  float h[dim]; opN(h,=,p1,+,p2,*,0.5f,dim);
  float np1[dim]; opN(np1,=,p0,+,p1,*,0.5f,dim);
  float np5[dim]; opN(np5,=,p2,+,p3,*,0.5f,dim);
  float np2[dim]; opN(np2,=,np1,+,h,*,0.5f,dim);
  float np4[dim]; opN(np4,=,h,+,np5,*,0.5f,dim);
  float np3[dim]; opN(np3,=,np2,+,np4,*,0.5f,dim);
  float lhs = bspline__arc_len(p0, np1, np2, np3,dim);
  float rhs = bspline__arc_len(np3, np4, np5, p3,dim);
  return lhs + rhs;
}
static float
bspline_arc_len(const float *cvs, int cnt, int dim) {
  float sum = 0;
  for (int i = 0; i + 1 < cnt; i += 3) {
    const float *p0 = cvs + (i + 0) * dim;
    const float *p1 = cvs + (i + 1) * dim;
    const float *p2 = cvs + (i + 2) * dim;
    const float *p3 = cvs + (i + 3) * dim;
    sum += bspline__arc_len(p0, p1, p2, p3, dim);
  }
  return sum;
}
static void
bspline_pos(float *pos, float linear_pos, const float *cvs, int cnt,
            int is_closed, int dim) {
  assert(pos);
  assert(cvs);
  assert(dim > 0);
  assert(cnt > 0);

  float val = castf(casti(linear_pos));
  int idx = casti(val) * 3;
  if (idx + 1 >= cnt) {
    /* reached the end.. */
    if (is_closed) {
      idx = idx % (cnt-1);
    } else {
      memcpy(pos, cvs + (cnt - 1) * dim, szof(float) * dim);
      return;
    }
  }
  assert(idx <= cnt);
  assert(idx + 3 <= cnt);

  /* calculate parameters */
  float u[4];
  float pos_frac = linear_pos - val;
  u[0] = ((1.0f - pos_frac) * (1.0f - pos_frac)) * (1.0f - pos_frac);
  u[1] = ((3.0f * pos_frac) * ((1.0f - pos_frac) * (1.0f - pos_frac)));
  u[2] = ((3.0f * pos_frac) * (pos_frac * (1.0f - pos_frac)));
  u[3] = pos_frac * pos_frac * pos_frac;

  /* calculate point */
  opNs(pos, =,cvs+(idx+0)*dim,*,u[0],dim);
  opNs(pos,+=,cvs+(idx+1)*dim,*,u[1],dim);
  opNs(pos,+=,cvs+(idx+2)*dim,*,u[2],dim);
  opNs(pos,+=,cvs+(idx+3)*dim,*,u[3],dim);
}
static void
bspline_tangent(float *tangent, float linear_pos, const float *cvs, int cnt,
                int is_closed, int dim) {
  assert(cvs);
  assert(dim > 0);
  assert(cnt > 0);
  assert(tangent);

  float val = castf(casti(linear_pos));
  int idx = casti(val) * 3;
  if (idx + 1 >= cnt) {
    /* reached the end.. */
    if (is_closed) {
      idx = idx % (cnt - 1);
    } else {
      assert(cnt >= 4);
      idx = max(4, cnt) - 4;
    }
  }
  assert(idx < cnt);
  assert(idx + 3 <= cnt);
  float pos_frac = linear_pos - val;

  /* now calculate tangent: -3 (a (x-1) + b (-1 + 4x - 3x) + c(-2+3x)x - dx) */
  float a = (pos_frac - 1.0f) * (pos_frac - 1.0f);
  float b = -1.0f + (4.0f * pos_frac) - ((3.0f * pos_frac * pos_frac));
  float c = ((-2.0f + (3.0f * pos_frac)) * pos_frac);
  float d = -(pos_frac * pos_frac);

  memset(tangent,0,szof(float) * dim);
  opNs(tangent,+=,cvs+((idx+0)%cnt)*dim,*,a,dim);
  opNs(tangent,+=,cvs+((idx+1)%cnt)*dim,*,b,dim);
  opNs(tangent,+=,cvs+((idx+2)%cnt)*dim,*,c,dim);
  opNs(tangent,+=,cvs+((idx+3)%cnt)*dim,*,d,dim);
  opNs(tangent, =,tangent,*,-3.0f,dim);
}
static float
bspline_nearest_cv(const float *restrict cvs, int cnt,
                   const float *restrict pos, int dim) {
  assert(cvs);
  assert(pos);
  assert(dim > 0);
  assert(cnt > 0);

  float r = 0.0f;
  float dist, best_dist = 10000.0f*10000.0f;
  for (int i = 0; i < cnt; i++) {
    float v[dim]; opN(v,=,cvs +(i*dim),+,pos,+,0,dim);
    lenN(dist, v, dim);
    if (dist < best_dist) {
      best_dist = dist;
      r = castf(i) / 3.0f;
    }
  }
  return r;
}
static float
bspline__sqr_dist(const float *restrict p0, const float *restrict p1, int dim) {
  assert(p0);
  assert(p1);

  float d[dim]; opN(d,=,p0,-,p1,+,0,dim);
  float len2; dotN(len2,d,d,dim);
  return len2;
}
static float
bspline_nearest_pnt(const float *restrict cvs, int cnt,
                    const float* restrict pos, int is_closed, int dim) {
  assert(cvs);
  assert(cnt > 0);
  assert(dim > 0);
  assert(pos);

  int it_cnt = 0;
  float off = 0.0f;
  float scale = 1.0f;
  float linear_len = bspline_linear_len(cnt);
  float near_pos = bspline_nearest_cv(cvs, cnt, pos, dim);
  do {
    float bpos[dim]; bspline_pos(bpos, near_pos, cvs, cnt, is_closed, dim);
    float tan[dim]; bspline_tangent(tan, near_pos, cvs, cnt, is_closed, dim);
    float len2; dotN(len2,tan,tan,dim);
    if (len2 <= FLT_EPSILON) {
      return bspline_nearest_cv(cvs, cnt, pos, dim);
    }
    float old_off = off;
    float to_pos[dim]; opN(to_pos,=,pos,-,bpos,+,0,dim);
    float dot; dotN(dot,tan, to_pos,dim);
    off = dot / len2;
    float off_diff = (float)math_abs(old_off - off);
    if (it_cnt > 0 && off_diff > FLT_EPSILON) {
      scale = scale * ((float)math_abs(old_off) / off_diff);
    }
    float new_pos = near_pos + off * scale;
    if (is_closed) {
      while (new_pos < 0.0f) {
        new_pos += linear_len;
      }
      while (new_pos > linear_len) {
        new_pos -= linear_len;
      }
    } else {
      new_pos = clamp( 0.0f, new_pos, linear_len);
    }
    /* take point if closer */
    float new_bpos[dim];
    bspline_pos(new_bpos, new_pos, cvs, cnt, is_closed, dim);
    if (bspline__sqr_dist(new_bpos, pos,dim) < bspline__sqr_dist(bpos, pos,dim)) {
      near_pos = new_pos;
    } else {
      break;
    }
  } while(math_abs(off) > 0.001f && ++it_cnt < 4);
  return near_pos;
}
static float
bspline_len_ratio(int idx, const float *restrict cvs, int cnt, float len, int dim) {
  assert(cvs);
  assert(cnt > 0);
  assert(dim > 0);
  if (idx >= cnt - 3) {
    return 1.0f;
  }
  const float *p0 = cvs + (idx + 0) * dim;
  const float *p1 = cvs + (idx + 1) * dim;
  const float *p2 = cvs + (idx + 2) * dim;
  const float *p3 = cvs + (idx + 3) * dim;
  return bspline__arc_len(p0, p1, p2, p3, dim) / len;
}
static float
bspline_calc_const_speed_time(const float *restrict cvs, int cnt,
                              float norm_time, int is_closed, int dim) {
  assert(cvs);
  assert(dim > 0);
  assert(cnt > 0);
  if (cnt < 4) {
    return 0.0f;
  }
  assert(norm_time >= 0.0f && norm_time <= 1.0f);
  if (norm_time <= 0.0f) {
    return 0.0f;
  }
  float linear_len = bspline_linear_len(cnt);
  float dst_arc_len = bspline_arc_len(cvs, cnt, dim) * norm_time;
  if (norm_time >= 1.0f) {
    return linear_len;
  }
  float start_time = 0.0f;
  float cur_arc_len = 0.0f;
  float sample_interval = 0.0001f;

  float v0[dim]; bspline_pos(v0, start_time, cvs, cnt, is_closed, dim);
  float cur_time = start_time;
  float end_time = linear_len + sample_interval;
  while (cur_time <= end_time) {
    float v1[dim]; bspline_pos(v1, cur_time, cvs, cnt, is_closed, dim);
    float dist[dim]; opN(dist,=,v1,-,v0,+,0,dim);

    float cur_step; lenN(cur_step, dist, dim);
    float delta = cur_arc_len + cur_step - dst_arc_len;
    if (delta >= 0.0f) {
      float lerp_fac = (math_abs(cur_step) < FLT_EPSILON) ? 0.0f : delta / cur_step;
      return cur_time + sample_interval * lerp_fac;
    }
    cur_time += sample_interval;
    cur_arc_len += cur_step;
    memcpy(v0, v1, dim * szof(float));
  }
  return linear_len;
}
static void
bspline_pos_const(float *restrict pos, float norm_time,
                  const float *restrict cvs, int cnt, int is_closed, int dim) {
  assert(cvs);
  assert(dim > 0);
  assert(cnt > 0);
  norm_time = clamp01(norm_time);
  float lin_time = bspline_calc_const_speed_time(cvs, cnt, norm_time, is_closed, dim);
  bspline_pos(pos, lin_time, cvs, cnt, is_closed, dim);
}
static void
bspline_calc_new_pnts_insert(float *restrict new_pnts, float linear_pos,
                            const float *restrict cvs, int cnt, int is_closed,
                            int dim) {
  /* calculate modification of old helper points */
  float fac = linear_pos - math_floori(linear_pos);
  float val = castf(casti(linear_pos ));
  int idx = casti(val) * 3;

  float h[dim]; opN(h,=,cvs+(1*dim),-,cvs+(0*dim),*,fac,dim);
  float len1; lenN(len1, h, dim);
  opN(new_pnts+(0*dim),=,h,+,cvs+(idx*dim),+,0,dim);

  opN(h,=,cvs+(2*dim),-,cvs+(3*dim),*,(1.0f-fac),dim);
  float len2; lenN(len2, h, dim);
  opN(new_pnts+(4*dim),=,h,+,cvs+((idx+3)*dim),+,0,dim);

  /* calculate new point at linear position */
  bspline_pos(new_pnts+(2*dim), linear_pos, cvs, cnt, is_closed, dim);
  // calculate new helper points
  bspline_tangent(h, linear_pos, cvs, cnt, is_closed, dim);
  float rlen; lenN(rlen,h,dim); rlen = math_rsqrt(rlen);
  opNs(h,=,h,*,rlen,dim);

  mcpy(new_pnts+(1*dim), new_pnts+(2*dim), szof(float) * dim);
  mcpy(new_pnts+(3*dim), new_pnts+(2*dim), szof(float) * dim);

  opNs(new_pnts+(1*dim),+=,h,*,(-len1 * 0.5f),dim);
  opNs(new_pnts+(3*dim),+=,h,*,(len2 * 0.5f),dim);
}
static void
bspline_calc_new_pnts_del(float *restrict new_pnts, int idx,
                          const float *restrict cvs, int cnt,
                          int dim) {
  assert(cvs);
  assert(cnt > 0);
  assert(new_pnts);
  if (idx < 3 || idx + 3 >= cnt || !bspline_is_key(idx)) {
    return;
  }
  const float *a = cvs + (idx - 3 * dim);
  const float *b = cvs + (idx - 2 * dim);
  const float *c = cvs + (idx - 1 * dim);
  const float *d = cvs + (idx - 0 * dim);
  const float *e = cvs + (idx + 1 * dim);
  const float *f = cvs + (idx + 2 * dim);
  const float *g = cvs + (idx + 3 * dim);

  float arc_len_1 = bspline__arc_len(a,b,c,d,dim);
  float arc_len_2 = bspline__arc_len(d,e,f,g,dim);
  float len = arc_len_1 + arc_len_2;

  float h[dim];
  opN(h,=,cvs+((idx-2)*dim),-,cvs+((idx-3)*dim),*,len/arc_len_1,dim);
  opN(new_pnts,=,h,+,cvs+((idx-3) * dim),+,0,dim);

  opN(h,=,cvs+((idx+2)*dim),-,cvs+((idx+3)*dim),*,len/arc_len_2,dim);
  opN(new_pnts+dim,=,h,+,cvs+((idx+3) * dim),+,0,dim);
}
/* ----------------------------------------------------------------------------
 *                                  GJK
 * ---------------------------------------------------------------------------- */
#define GJK_MAX_ITERATIONS 20
struct gjk_support {
  int aid, bid;
  float a[3];
  float b[3];
};
struct gjk_vertex {
  float a[3];
  float b[3];
  float p[3];
  int aid, bid;
};
struct gjk_simplex {
  int hit;
  int iter, max_iter;
  int vcnt, scnt;
  int saveA[5], saveB[5];
  struct gjk_vertex v[5];
  float bc[4], D;
};
struct gjk_result {
  int hit;
  float p0[3];
  float p1[3];
  float distance_squared;
  int iterations;
};
static int
gjk_solve(struct gjk_simplex *s, const struct gjk_support *sup, float *dv) {
  assert(s);
  assert(dv);
  assert(sup);
  if (!s || !sup || !dv) return 0;
  if (s->max_iter > 0 && s->iter >= s->max_iter)
      return 0;

  /* I.) Initialize */
  if (!s->vcnt) {
    s->scnt = 0;
    s->D = FLT_MAX;
    s->max_iter = !s->max_iter ? GJK_MAX_ITERATIONS: s->max_iter;
  }
  /* II.) Check for duplications */
  for (int i = 0; i < s->scnt; ++i) {
    if (sup->aid != s->saveA[i]) continue;
    if (sup->bid != s->saveB[i]) continue;
    return 0;
  }
  /* III.) Add vertex into simplex */
  struct gjk_vertex *vert = &s->v[s->vcnt];
  cpy3(vert->a, sup->a);
  cpy3(vert->b, sup->b);
  cpy3(vert->p, dv);
  vert->aid = sup->aid;
  vert->bid = sup->bid;
  s->bc[s->vcnt++] = 1.0f;

  /* IV.) Find closest simplex point */
  switch (s->vcnt) {
  case 1: break;
  case 2: {
      /* -------------------- Line ----------------------- */
      float a[3]; cpy3(a, s->v[0].p);
      float b[3]; cpy3(b, s->v[1].p);

      /* compute barycentric coordinates */
      float ab[3]; sub3(ab, a, b);
      float ba[3]; sub3(ba, b, a);

      float u = dot3(b, ba);
      float v = dot3(a, ab);
      if (v <= 0.0f) {
        /* region A */
        s->bc[0] = 1.0f;
        s->vcnt = 1;
        break;
      }
      if (u <= 0.0f) {
        /* region B */
        s->v[0] = s->v[1];
        s->bc[0] = 1.0f;
        s->vcnt = 1;
        break;
      }
      /* region AB */
      s->bc[0] = u;
      s->bc[1] = v;
      s->vcnt = 2;
  } break;
  case 3: {
    /* -------------------- Triangle ----------------------- */
    float a[3]; cpy3(a, s->v[0].p);
    float b[3]; cpy3(b, s->v[1].p);
    float c[3]; cpy3(c, s->v[2].p);

    float ab[3]; sub3(ab, a, b);
    float ba[3]; sub3(ba, b, a);
    float bc[3]; sub3(bc, b, c);
    float cb[3]; sub3(cb, c, b);
    float ca[3]; sub3(ca, c, a);
    float ac[3]; sub3(ac, a, c);

    /* compute barycentric coordinates */
    float u_ab = dot3(b, ba);
    float v_ab = dot3(a, ab);

    float u_bc = dot3(c, cb);
    float v_bc = dot3(b, bc);

    float u_ca = dot3(a, ac);
    float v_ca = dot3(c, ca);

    if (v_ab <= 0.0f && u_ca <= 0.0f) {
      /* region A */
      s->bc[0] = 1.0f;
      s->vcnt = 1;
      break;
    }
    if (u_ab <= 0.0f && v_bc <= 0.0f) {
      /* region B */
      s->v[0] = s->v[1];
      s->bc[0] = 1.0f;
      s->vcnt = 1;
      break;
    }
    if (u_bc <= 0.0f && v_ca <= 0.0f) {
      /* region C */
      s->v[0] = s->v[2];
      s->bc[0] = 1.0f;
      s->vcnt = 1;
      break;
    }
    /* calculate fractional area */
    float n[3]; cross3(n, ba, ca);
    float n1[3]; cross3(n1, b, c);
    float n2[3]; cross3(n2, c, a);
    float n3[3]; cross3(n3, a, b);

    float u_abc = dot3(n1, n);
    float v_abc = dot3(n2, n);
    float w_abc = dot3(n3, n);

    if (u_ab > 0.0f && v_ab > 0.0f && w_abc <= 0.0f) {
      /* region AB */
      s->bc[0] = u_ab;
      s->bc[1] = v_ab;
      s->vcnt = 2;
      break;
    }
    if (u_bc > 0.0f && v_bc > 0.0f && u_abc <= 0.0f) {
      /* region BC */
      s->v[0] = s->v[1];
      s->v[1] = s->v[2];
      s->bc[0] = u_bc;
      s->bc[1] = v_bc;
      s->vcnt = 2;
      break;
    }
    if (u_ca > 0.0f && v_ca > 0.0f && v_abc <= 0.0f) {
      /* region CA */
      s->v[1] = s->v[0];
      s->v[0] = s->v[2];
      s->bc[0] = u_ca;
      s->bc[1] = v_ca;
      s->vcnt = 2;
      break;
    }
    /* region ABC */
    assert(u_abc > 0.0f && v_abc > 0.0f && w_abc > 0.0f);
    s->bc[0] = u_abc;
    s->bc[1] = v_abc;
    s->bc[2] = w_abc;
    s->vcnt = 3;
  } break;
  case 4: {
    /* -------------------- Tetrahedron ----------------------- */
    float a[3]; cpy3(a, s->v[0].p);
    float b[3]; cpy3(b, s->v[1].p);
    float c[3]; cpy3(c, s->v[2].p);
    float d[3]; cpy3(d, s->v[3].p);

    float ab[3]; sub3(ab, a, b);
    float ba[3]; sub3(ba, b, a);
    float bc[3]; sub3(bc, b, c);
    float cb[3]; sub3(cb, c, b);
    float ca[3]; sub3(ca, c, a);
    float ac[3]; sub3(ac, a, c);

    float db[3]; sub3(db, d, b);
    float bd[3]; sub3(bd, b, d);
    float dc[3]; sub3(dc, d, c);
    float cd[3]; sub3(cd, c, d);
    float da[3]; sub3(da, d, a);
    float ad[3]; sub3(ad, a, d);

    /* compute barycentric coordinates */
    float u_ab = dot3(b, ba);
    float v_ab = dot3(a, ab);

    float u_bc = dot3(c, cb);
    float v_bc = dot3(b, bc);

    float u_ca = dot3(a, ac);
    float v_ca = dot3(c, ca);

    float u_bd = dot3(d, db);
    float v_bd = dot3(b, bd);

    float u_dc = dot3(c, cd);
    float v_dc = dot3(d, dc);

    float u_ad = dot3(d, da);
    float v_ad = dot3(a, ad);

    /* check verticies for closest point */
    if (v_ab <= 0.0f && u_ca <= 0.0f && v_ad <= 0.0f) {
      /* region A */
      s->bc[0] = 1.0f;
      s->vcnt = 1;
      break;
    }
    if (u_ab <= 0.0f && v_bc <= 0.0f && v_bd <= 0.0f) {
      /* region B */
      s->v[0] = s->v[1];
      s->bc[0] = 1.0f;
      s->vcnt = 1;
      break;
    }
    if (u_bc <= 0.0f && v_ca <= 0.0f && u_dc <= 0.0f) {
      /* region C */
      s->v[0] = s->v[2];
      s->bc[0] = 1.0f;
      s->vcnt = 1;
      break;
    }
    if (u_bd <= 0.0f && v_dc <= 0.0f && u_ad <= 0.0f) {
      /* region D */
      s->v[0] = s->v[3];
      s->bc[0] = 1.0f;
      s->vcnt = 1;
      break;
    }
    /* calculate fractional area */
    float n[3], n1[3], n2[3], n3[3];
    cross3(n, da, ba);
    cross3(n1, d, b);
    cross3(n2, b, a);
    cross3(n3, a, d);

    float u_adb = dot3(n1, n);
    float v_adb = dot3(n2, n);
    float w_adb = dot3(n3, n);

    cross3(n, ca, da);
    cross3(n1, c, d);
    cross3(n2, d, a);
    cross3(n3, a, c);

    float u_acd = dot3(n1, n);
    float v_acd = dot3(n2, n);
    float w_acd = dot3(n3, n);

    cross3(n, bc, dc);
    cross3(n1, b, d);
    cross3(n2, d, c);
    cross3(n3, c, b);

    float u_cbd = dot3(n1, n);
    float v_cbd = dot3(n2, n);
    float w_cbd = dot3(n3, n);

    cross3(n, ba, ca);
    cross3(n1, b, c);
    cross3(n2, c, a);
    cross3(n3, a, b);

    float u_abc = dot3(n1, n);
    float v_abc = dot3(n2, n);
    float w_abc = dot3(n3, n);

      /* check edges for closest point */
    if (w_abc <= 0.0f && v_adb <= 0.0f && u_ab > 0.0f && v_ab > 0.0f) {
      /* region AB */
      s->bc[0] = u_ab;
      s->bc[1] = v_ab;
      s->vcnt = 2;
      break;
    }
    if (u_abc <= 0.0f && w_cbd <= 0.0f && u_bc > 0.0f && v_bc > 0.0f) {
      /* region BC */
      s->v[0] = s->v[1];
      s->v[1] = s->v[2];
      s->bc[0] = u_bc;
      s->bc[1] = v_bc;
      s->vcnt = 2;
      break;
    }
    if (v_abc <= 0.0f && w_acd <= 0.0f && u_ca > 0.0f && v_ca > 0.0f) {
      /* region CA */
      s->v[1] = s->v[0];
      s->v[0] = s->v[2];
      s->bc[0] = u_ca;
      s->bc[1] = v_ca;
      s->vcnt = 2;
      break;
    }
    if (v_cbd <= 0.0f && u_acd <= 0.0f && u_dc > 0.0f && v_dc > 0.0f) {
      /* region DC */
      s->v[0] = s->v[3];
      s->v[1] = s->v[2];
      s->bc[0] = u_dc;
      s->bc[1] = v_dc;
      s->vcnt = 2;
      break;
    }
    if (v_acd <= 0.0f && w_adb <= 0.0f && u_ad > 0.0f && v_ad > 0.0f) {
      /* region AD */
      s->v[1] = s->v[3];
      s->bc[0] = u_ad;
      s->bc[1] = v_ad;
      s->vcnt = 2;
      break;
    }
    if (u_cbd <= 0.0f && u_adb <= 0.0f && u_bd > 0.0f && v_bd > 0.0f) {
      /* region BD */
      s->v[0] = s->v[1];
      s->v[1] = s->v[3];
      s->bc[0] = u_bd;
      s->bc[1] = v_bd;
      s->vcnt = 2;
      break;
    }
    /* calculate fractional volume (volume can be negative!) */
    float denom = box3(cb, ab, db);
    float volume = (denom == 0) ? 1.0f: 1.0f/denom;
    float u_abcd = box3(c, d, b) * volume;
    float v_abcd = box3(c, a, d) * volume;
    float w_abcd = box3(d, a, b) * volume;
    float x_abcd = box3(b, a, c) * volume;

      /* check faces for closest point */
    if (x_abcd <= 0.0f && u_abc > 0.0f && v_abc > 0.0f && w_abc > 0.0f) {
      /* region ABC */
      s->bc[0] = u_abc;
      s->bc[1] = v_abc;
      s->bc[2] = w_abc;
      s->vcnt = 3;
      break;
    }
    if (u_abcd <= 0.0f && u_cbd > 0.0f && v_cbd > 0.0f && w_cbd > 0.0f) {
      /* region CBD */
      s->v[0] = s->v[2];
      s->v[2] = s->v[3];
      s->bc[0] = u_cbd;
      s->bc[1] = v_cbd;
      s->bc[2] = w_cbd;
      s->vcnt = 3;
      break;
    }
    if (v_abcd <= 0.0f && u_acd > 0.0f && v_acd > 0.0f && w_acd > 0.0f) {
      /* region ACD */
      s->v[1] = s->v[2];
      s->v[2] = s->v[3];
      s->bc[0] = u_acd;
      s->bc[1] = v_acd;
      s->bc[2] = w_acd;
      s->vcnt = 3;
      break;
    }
    if (w_abcd <= 0.0f && u_adb > 0.0f && v_adb > 0.0f && w_adb > 0.0f) {
      /* region ADB */
      s->v[2] = s->v[1];
      s->v[1] = s->v[3];
      s->bc[0] = u_adb;
      s->bc[1] = v_adb;
      s->bc[2] = w_adb;
      s->vcnt = 3;
      break;
    }
    /* region ABCD */
    assert(u_abcd > 0.0f && v_abcd > 0.0f && w_abcd > 0.0f && x_abcd > 0.0f);
    s->bc[0] = u_abcd;
    s->bc[1] = v_abcd;
    s->bc[2] = w_abcd;
    s->bc[3] = x_abcd;
    s->vcnt = 4;
  } break;}

  /* V.) Check if origin is enclosed by tetrahedron */
  if (s->vcnt == 4) {
    s->hit = 1;
    return 0;
  }
  /* VI.) Ensure closing in on origin to prevent multi-step cycling */
  float pnt[3], denom = 0;
  for (int i = 0; i < s->vcnt; ++i) {
    denom += s->bc[i];
  }
  denom = 1.0f / denom;

  switch (s->vcnt) {
  case 1: cpy3(pnt, s->v[0].p); break;
  case 2: {
    /* --------- Line -------- */
    float a[3]; mul3(a, s->v[0].p, denom * s->bc[0]);
    float b[3]; mul3(b, s->v[1].p, denom * s->bc[1]);
    add3(pnt, a, b);
  } break;
  case 3: {
    /* ------- Triangle ------ */
    float a[3]; mul3(a, s->v[0].p, denom * s->bc[0]);
    float b[3]; mul3(b, s->v[1].p, denom * s->bc[1]);
    float c[3]; mul3(c, s->v[2].p, denom * s->bc[2]);

    add3(pnt, a, b);
    add3(pnt, pnt, c);
  } break;
  case 4: {
    /* ----- Tetrahedron ----- */
    float a[3]; mul3(a, s->v[0].p, denom * s->bc[0]);
    float b[3]; mul3(b, s->v[1].p, denom * s->bc[1]);
    float c[3]; mul3(c, s->v[2].p, denom * s->bc[2]);
    float d[3]; mul3(d, s->v[3].p, denom * s->bc[3]);

    add3(pnt, a, b);
    add3(pnt, pnt, c);
    add3(pnt, pnt, d);
  } break;}

  float d2 = dot3(pnt, pnt);
  if (d2 >= s->D) {
    return 0;
  }
  s->D = d2;

  /* VII.) New search direction */
  switch (s->vcnt) {
  default: assert(0); break;
  case 1: {
    /* --------- Point -------- */
    mul3(dv, s->v[0].p, -1);
  } break;
  case 2: {
    /* ------ Line segment ---- */
    float ba[3]; sub3(ba, s->v[1].p, s->v[0].p);
    float b0[3]; mul3(b0, s->v[1].p, -1);
    float t[3]; cross3(t, ba, b0);
    cross3(dv, t, ba);
  } break;
  case 3: {
    /* ------- Triangle ------- */
    float ab[3]; sub3(ab, s->v[1].p, s->v[0].p);
    float ac[3]; sub3(ac, s->v[2].p, s->v[0].p);
    float n[3]; cross3(n, ab, ac);
    if (dot3(n, s->v[0].p) <= 0.0f) {
      cpy3(dv, n);
    } else {
      mul3(dv, n, -1);
    }
  }}
  if (dot3(dv,dv) < FLT_EPSILON * FLT_EPSILON) {
    return 0;
  }
  /* VIII.) Save ids for next duplicate check */
  s->scnt = s->vcnt; s->iter++;
  for (int i = 0; i < s->scnt; ++i) {
    s->saveA[i] = s->v[i].aid;
    s->saveB[i] = s->v[i].bid;
  }
  return 1;
}
static struct gjk_result
gjk_analyze(const struct gjk_simplex *s) {
  struct gjk_result r = {0};
  r.iterations = s->iter;
  r.hit = s->hit;

  /* calculate normalization denominator */
  float denom = 0;
  for (int i = 0; i < s->vcnt; ++i) {
    denom += s->bc[i];
  }
  denom = 1.0f / denom;

  /* compute closest points */
  switch (s->vcnt) {
  default: assert(0); break;
  case 1: {
    /* Point */
    cpy3(r.p0, s->v[0].a);
    cpy3(r.p1, s->v[0].b);
  } break;
  case 2: {
    /* Line */
    float as = denom * s->bc[0];
    float bs = denom * s->bc[1];

    float a[3]; mul3(a, s->v[0].a, as);
    float b[3]; mul3(b, s->v[1].a, bs);
    float c[3]; mul3(c, s->v[0].b, as);
    float d[3]; mul3(d, s->v[1].b, bs);

    add3(r.p0, a, b);
    add3(r.p1, c, d);
  } break;
  case 3: {
    /* Triangle */
    float as = denom * s->bc[0];
    float bs = denom * s->bc[1];
    float cs = denom * s->bc[2];

    float a[3]; mul3(a, s->v[0].a, as);
    float b[3]; mul3(b, s->v[1].a, bs);
    float c[3]; mul3(c, s->v[2].a, cs);

    float d[3]; mul3(d, s->v[0].b, as);
    float e[3]; mul3(e, s->v[1].b, bs);
    float f[3]; mul3(f, s->v[2].b, cs);

    add3(r.p0, a, b);
    add3(r.p0, r.p0, c);

    add3(r.p1, d, e);
    add3(r.p1, r.p1, f);
  } break;
  case 4: {
    /* Tetrahedron */
    float as = denom * s->bc[0];
    float bs = denom * s->bc[1];
    float cs = denom * s->bc[2];
    float ds = denom * s->bc[3];

    float a[3]; mul3(a, s->v[0].a, as);
    float b[3]; mul3(b, s->v[1].a, bs);
    float c[3]; mul3(c, s->v[2].a, cs);
    float d[3]; mul3(d, s->v[3].a, ds);

    add3(r.p0, a, b);
    add3(r.p0, r.p0, c);
    add3(r.p0, r.p0, d);
    cpy3(r.p1, r.p0);
  } break;}

  if (!r.hit) {
    /* compute distance */
    float d[3]; sub3(d, r.p1, r.p0);
    r.distance_squared = dot3(d, d);
  } else {
    r.distance_squared = 0;
  }
  return r;
}

/* ---------------------------------------------------------------------------
 *                                Plane
 * ---------------------------------------------------------------------------
 */
  /* Plane: ax + by + cz + d
   * Equation:
   *      n * (p - p0) = 0
   *      n * p - n * p0 = 0
   *      |a b c| * p - |a b c| * p0
   *
   *      |a b c| * p + d = 0
   *          d = -1 * |a b c| * p0
   *
   *  Plane: |a b c d| d = -|a b c| * p0
   */
#define planeq(p, n, o) cpy3(p,n), p[3] = -dot3(n,o)

static inline void
plane_tri(float *restrict r, const float *restrict p0, const float *restrict p1,
          const float *restrict p2) {
  assert(r);
  assert(p0);
  assert(p1);
  assert(p2);

  float d10[3]; sub3(d10, p0, p1);
  float d20[3]; sub3(d20, p0, p2);
  float n[3]; cross3(n, d10, d20); normeq3(n);
  cpy3(r,n), r[3] = -dot3(p0, n);
}
static inline float
plane_dist_pnt(const float *restrict plane, const float *restrict pnt) {
  assert(plane);
  assert(pnt);
  float p[4] = {0,0,0,1}; cpy3(p, pnt);
  return dot4(p, plane);
}
static inline float
plane_abs_dist_pnt(const float *plane, const float *pnt) {
  assert(plane);
  assert(pnt);
  float d = plane_dist_pnt(plane, pnt);
  return castf(math_abs(d));
}
static inline int
plane_pnts_same_side(const float *restrict plane, const float *restrict a,
                     const float *restrict b) {
  assert(plane);
  assert(a);
  assert(b);

  float da = plane_dist_pnt(plane, a);
  float db = plane_dist_pnt(plane, b);
  return (da * db) >= 0.0f;
}
static inline int
plane_pnt_is_front(const float *restrict plane, const float *pnt) {
  assert(plane);
  assert(pnt);
  float d = plane_dist_pnt(plane, pnt);
  return d >= 0.0f;
}
static inline int
plane_pnt_is_behind(const float *restrict plane, const float *pnt) {
  assert(plane);
  assert(pnt);
  float d = plane_dist_pnt(plane, pnt);
  return d < 0.0f;
}
static inline void
plane_proj_pnt(float *restrict r, const float *restrict plane, const float *pnt) {
  assert(r);
  assert(plane);
  assert(pnt);

  float p[4] = {0,0,0,1}; cpy3(p, pnt);
  float n[4] = {0,0,0,0}; cpy3(n,plane); mul3(n,n,-1.0f);
  float d = dot4(p, plane);
  op4(r,=,p,+,n,*,d);
}
static inline void
plane_proj_vec(float *restrict r, const float *restrict plane, const float *v) {
  assert(r);
  assert(plane);
  assert(v);

  float d = dot3(v, plane);
  float n[4]; mul4(n,plane,-1.0f);
  op4(r,=,v,+,n,*,d);
}

/* ---------------------------------------------------------------------------
 *                                Line segment
 * ---------------------------------------------------------------------------
 */
static void
seg_get_pnt_to_pnt(float *res, const float *a, const float *b, const float *p) {
  assert(res);
  assert(a);
  assert(b);
  assert(p);

  float ab[3]; sub3(ab, b,a);
  float pa[3]; sub3(pa, p,a);
  float t = dot3(pa,ab) / dot3(ab,ab);
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  mul3(res, ab, t);
  add3(res, a, res);
}
static float
seg_get_pnt_to_pnt_sqdist(const float *restrict a, const float *restrict b,
                          const float *p) {
  assert(a);
  assert(b);
  assert(p);

  float ab[3]; sub3(ab,a,b);
  float ap[3]; sub3(ap,a,p);
  float bp[3]; sub3(bp,a,p);
  float e = dot3(ap,ab);

  /* handle cases p proj outside ab */
  if (e <= 0.0f) return dot3(ap,ap);
  float f = dot3(ab,ab);
  if (e >= f) return dot3(bp,bp);
  return dot3(ap,ap) - (e*e)/f;
}
static float
seg_get_pnt_to_seg(float *t1, float *t2, float *c1, float *c2,
                    const float *p1, const float *q1,
                    const float *p2, const float *q2) {
  assert(t1);
  assert(t2);
  assert(c1);
  assert(c2);
  assert(p1);
  assert(q1);
  assert(p2);
  assert(q2);

  float d1[3]; sub3(d1, q1, p1); /* direction vector segment s1 */
  float d2[3]; sub3(d2, q2, p2); /* direction vector segment s2 */
  float r[3]; sub3(r, p1, p2);

  float a = dot3(d1, d1);
  float e = dot3(d2, d2);
  float f = dot3(d2, r);

  if (a <= FLT_EPSILON && e <= FLT_EPSILON) {
    /* both segments degenerate into points */
    *t1 = *t2 = 0.0f;
    cpy3(c1, p1);
    cpy3(c2, p2);
    float d12[3]; sub3(d12, c1, c2);
    return dot3(d12,d12);
  }
  if (a > FLT_EPSILON) {
    float c = dot3(d1,r);
    if (e > FLT_EPSILON) {
      /* non-degenerate case */
      float b = dot3(d1,d2);
      float denom = a*e - b*b;
      /* compute closest point on L1/L2 if not parallel else pick any t2 */
      if (denom != 0.0f) {
        *t1 = clamp(0.0f, (b*f - c*e) / denom, 1.0f);
      } else {
        *t1 = 0.0f;
      }
      /* cmpute point on L2 closest to S1(s) */
      *t2 = (b*(*t1) + f) / e;
      if (*t2 < 0.0f) {
        *t2 = 0.0f;
        *t1 = clamp(0.0f, -c/a, 1.0f);
      } else if (*t2 > 1.0f) {
        *t2 = 1.0f;
        *t1 = clamp(0.0f, (b-c)/a, 1.0f);
      }
    } else {
      /* second segment degenerates into a point */
      *t1 = clamp(0.0f, -c/a, 1.0f);
      *t2 = 0.0f;
    }
  } else {
    /* first segment degenerates into a point */
    *t2 = clamp(0.0f, f / e, 1.0f);
    *t1 = 0.0f;
  }
  /* calculate closest points */
  float n[3]; mul3(n, d1, *t1);
  add3(c1, p1, n);
  mul3(n, d2, *t2);
  add3(c2, p2, n);

  /* calculate squared distance */
  float d12[3]; sub3(d12, c1, c2);
  return dot3(d12,d12);
}

/* ---------------------------------------------------------------------------
 *                                Ray
 * ---------------------------------------------------------------------------
 */
static float
ray_hit_plane(const float *ro, const float *rd, const float *plane) {
  assert(ro);
  assert(rd);
  assert(plane);
  /* Ray: P = origin + rd * t
   * Plane: plane_normal * P + d = 0
   *
   * Substitute:
   *      normal * (origin + rd*t) + d = 0
   *
   * Solve for t:
   *      plane_normal * origin + plane_normal * rd*t + d = 0
   *      -(plane_normal*rd*t) = plane_normal * origin + d
   *
   *                  plane_normal * origin + d
   *      t = -1 * -------------------------------
   *                      plane_normal * rd
   *
   * Result:
   *      Behind: t < 0
   *      Infront: t >= 0
   *      Parallel: t = 0
   *      Intersection point: ro + rd * t
   */
  float n = -(dot3(plane,ro) + plane[3]);
  float p = dot3(plane,rd);
  if (math_abs(p) < 0.0001f) {
    return 0.0f;
  }
  return n/p;
}
static float
ray_hit_tri(const float *ro, const float *rd, const float *p0,
            const float *p1, const float *p2) {
  assert(ro);
  assert(rd);
  assert(p0);
  assert(p1);
  assert(p2);

  /* calculate triangle normal */
  float d10[3]; sub3(d10, p1,p0);
  float d20[3]; sub3(d20, p2,p0);
  float d21[3]; sub3(d21, p2,p1);
  float d02[3]; sub3(d02, p0,p2);
  float n[3]; cross3(n, d10,d20);

  /* check for plane intersection */
  float p[4]; planeq(p, n, p0);
  float t = ray_hit_plane(ro, rd, p);
  if (t <= 0.0f){
    return t;
  }
  /* intersection point */
  float in[3];
  mul3(in,rd,t);
  add3(in,in,ro);

  /* check if point inside triangle in plane */
  float di0[3]; sub3(di0, in, p0);
  float di1[3]; sub3(di1, in, p1);
  float di2[3]; sub3(di2, in, p2);

  float in0[3]; cross3(in0, d10, di0);
  float in1[3]; cross3(in1, d21, di1);
  float in2[3]; cross3(in2, d02, di2);

  float din0 = dot3(in0,n);
  float din1 = dot3(in1,n);
  float din2 = dot3(in2,n);
  return (din0 < 0.0f || din1 < 0.0f || din2 < 0.0f) ? -1 : t;
}
static int
ray_hit_sphere(float *t0, float *t1, const float *ro, const float *rd,
               const float *c, float r) {
  assert(t0);
  assert(t1);
  assert(ro);
  assert(rd);
  assert(c);

  float a[3]; sub3(a,c,ro);
  float tc = dot3(rd,a);
  if (tc < 0) {
    return 0;
  }
  float r2 = r*r;
  float d2 = dot3(a,a) - tc*tc;
  if (d2 > r2) {
    return 0;
  }
  float td = math_sqrt(r2 - d2);

  *t0 = tc - td;
  *t1 = tc + td;
  return 1;
}
static int
ray_hit_aabb(float *restrict t0, float *restrict t1,
  const float *restrict ro, const float *restrict rd,
  const float *restrict aabb) {

  assert(t0);
  assert(t1);
  assert(ro);
  assert(rd);
  assert(aabb);

  const float *min = aabb;
  const float *max = aabb + 3;

  float t0x = (min[0] - ro[0]) / rd[0];
  float t0y = (min[1] - ro[1]) / rd[1];
  float t0z = (min[2] - ro[2]) / rd[2];
  float t1x = (max[0] - ro[0]) / rd[0];
  float t1y = (max[1] - ro[1]) / rd[1];
  float t1z = (max[2] - ro[2]) / rd[2];

  float tminx = min(t0x, t1x);
  float tminy = min(t0y, t1y);
  float tminz = min(t0z, t1z);
  float tmaxx = max(t0x, t1x);
  float tmaxy = max(t0y, t1y);
  float tmaxz = max(t0z, t1z);
  if (tminx > tmaxy || tminy > tmaxx) {
    return 0;
  }
  *t0 = max(tminx, tminy);
  *t1 = min(tmaxy, tmaxx);
  if (*t0 > tmaxz || tminz> *t1) {
    return 0;
  }
  *t0 = max(*t0, tminz);
  *t1 = min(*t1, tmaxz);
  return 1;
}

/* ---------------------------------------------------------------------------
 *                                Sphere
 * ---------------------------------------------------------------------------
 * 4 floats:
 *    3 center,
 *    1 radius
 */
static int aabb_hit_sphere(const float *restrict a, const float *restrict s);
static int capsule_hit_sphere(const float *c, const float *s);

static void
sphere_closest_pnt(float *restrict res, const float *restrict s,
                   const float *restrict p) {
  assert(res);
  assert(s);
  assert(p);

  float d[3]; sub3(d, p, s);
  float n[3]; norm3(n,d);
  mul3(res,n,s[3]);
  add3(res,s,res);
}
static int
sphere_hit_sphere(const float *restrict a, const float *restrict b) {
  assert(a);
  assert(b);
  float d[3]; sub3(d, b, a);
  float r = a[3] + b[3];
  if (dot3(d,d) > r*r) {
    return 0;
  }
  return 1;
}
static int
sphere_hit_aabb(const float *restrict s, const float *restrict a) {
  assert(s);
  assert(a);
  return aabb_hit_sphere(a, s);
}
static int
sphere_hit_capsule(const float *s, const float *c) {
  assert(s);
  assert(c);
  return capsule_hit_sphere(c, s);
}

/* ---------------------------------------------------------------------------
 *                                AABB
 * ---------------------------------------------------------------------------
 * 6 floats:
 *    3 min,
 *    3 max
 */
static inline float* aabb_min(float *a) { return a; }
static inline float* aabb_max(float *a) { return a + 3; }
static int capsule_hit_aabb(const float *c, const float *a);

static void
aabb_add_pnt(float *restrict a, const float *restrict pnt) {
  assert(a);
  assert(pnt);
  min3(a, a, pnt);
  max3(a + 3, a + 3, pnt);
}
static void
aabb_ext(float *restrict e, const float *restrict a) {
  assert(e);
  assert(a);
  float d[3]; sub3(d, a + 3, a);
  mul3(e, e, 0.5f);
}
static void
aabb_ctr(float *restrict ctr, const float *restrict a) {
  assert(ctr);
  assert(a);
  float e[3]; aabb_ext(e, a);
  add3(ctr, a, e);
}
static void
aabb_build(float *restrict a, const float *restrict ctr,
           const float *restrict ext) {
  assert(a);
  assert(ctr);
  assert(ext);
  sub3(a, ctr, ext);
  add3(a + 3, ctr, ext);
}
static void
aabb_mov(float *restrict a, const float *restrict ctr) {
  assert(a);
  assert(ctr);
  float e[3]; aabb_ext(e, a);
  aabb_build(a, ctr, e);
}
static void
aabb_rebalance(float *restrict b, const float *restrict a,
               const float *restrict m, const float *restrict t) {
  assert(b);
  assert(a);
  assert(m);
  assert(t);

  for (int i = 0; i < 3; ++i) {
    b[i] = b[3+i] = t[i];
    for (int j = 0; j < 3; ++j) {
      float e = m[i*3+j] * a[j];
      float f = m[i*3+j] * a[3+j];
      if (e < f) {
        b[i] += e;
        b[3+i] += f;
      } else {
        b[i] += f;
        b[3+i] += e;
      }
    }
  }
}
static void
aabb_merge(float *restrict r, const float *restrict a, const float *restrict b) {
  assert(b);
  assert(r);
  assert(a);

  min3(r, a, b);
  max3(r+3, a+3, b+3);
}
static void
aabb_intersect(float *r, const float *a, const float *b) {
  assert(r);
  assert(a);
  assert(b);

  max3(r, a, b);
  min3(r+3, a+3, b+3);
}
static void
aabb_closest_pnt(float *restrict res, const float *restrict a,
                 const float *restrict p) {
  assert(res);
  assert(a);
  assert(p);

  for (int i = 0; i < 3; ++i) {
    float v = p[i];
    if (v < a[i]) v = a[i];
    if (v > a[3+i]) v = a[3+i];
    res[i] = v;
  }
}
static float
aabb_sqdist_to_pnt(const float *a, const float *p) {
  assert(a);
  assert(p);

  float r = 0;
  for (int i = 0; i < 3; ++i) {
    float v = p[i];
    if (v < a[i])
      r += (a[i]-v) * (a[i]-v);
    if (v > a[3+i])
      r += (v-a[3+i]) * (v-a[3+i]);
  }
  return r;
}
static int
aabb_has_pnt(const float *restrict a, const float *restrict p) {
  assert(a);
  assert(p);

  if (p[0] < a[0] || p[0] > a[3]) return 0;
  if (p[1] < a[1] || p[1] > a[4]) return 0;
  if (p[2] < a[2] || p[2] > a[5]) return 0;
  return 1;
}
static int
aabb_has_aabb(const float *restrict a, const float *restrict b) {
  assert(a);
  assert(b);

  if (a[3] < b[0] || a[0] > b[3]) return 0;
  if (a[4] < b[1] || a[1] > b[4]) return 0;
  if (a[5] < b[2] || a[2] > b[5]) return 0;
  return 1;
}
static int
aabb_hit_sphere(const float *restrict a, const float *restrict s) {
  assert(a);
  assert(s);
  /* compute squared distance between sphere center and aabb */
  float d2 = aabb_sqdist_to_pnt(a, s);
  /* intersection if distance is smaller/equal sphere radius*/
  return d2 <= s[3]*s[3];
}
static int
aabb_hit_capsule(const float *restrict a, const float *restrict c) {
  assert(a);
  assert(c);
  return capsule_hit_aabb(c, a);
}

/* ---------------------------------------------------------------------------
 *                                Capsule
 * ---------------------------------------------------------------------------
 * 7 floats:
 *    3 center0,
 *    3 center1
 *    1 radius
 */
static float
capsule_pnt_sqdist(const float *c, const float *p) {
  assert(c);
  assert(p);
  float d2 = seg_get_pnt_to_pnt_sqdist(c, c+3, p);
  return d2 - (c[6]*c[6]);
}
static void
capsule_closest_pnt(float *res, const float *restrict c, const float *restrict p) {
  assert(res);
  assert(c);
  assert(p);
  /* calculate closest point to internal capsule segment */
  float pp[3]; seg_get_pnt_to_pnt(pp, c, c + 3, p);
  /* extend point out by radius in normal direction */
  float d[3]; sub3(d,p,pp); normeq3(d);
  mul3(res, d, c[6]);
  add3(res, pp, res);
}
static int
capsule_hit_capsule(const float *a, const float *b) {
  assert(a);
  assert(b);

  float t1, t2, c1[3], c2[3];
  float d2 = seg_get_pnt_to_seg(&t1, &t2, c1, c2, a, a+3, b, b+3);
  float r = a[6] + b[6];
  return d2 <= r*r;
}
static int
capsule_hit_sphere(const float *c, const float *s) {
  assert(c);
  assert(s);
  /* squared distance bwetween sphere center and capsule line segment */
  float d2 = seg_get_pnt_to_pnt_sqdist(c,c+3,s);
  float r = s[4] + c[6];
  return d2 <= r * r;
}
static int
capsule_hit_aabb(const float *c, const float *a) {
  assert(c);
  assert(a);
  /* calculate aabb center point */
  float ac[3]; sub3(ac, a, a+3);
  mul3(ac, ac, 0.5f);
  /* calculate closest point from aabb to point on capsule and check if inside aabb */
  float p[3]; capsule_closest_pnt(p, c, ac);
  return aabb_has_pnt(a, p);
}

/* ---------------------------------------------------------------------------
 *                                Camera
 * ---------------------------------------------------------------------------
 */
#define CAM_INF (-1.0f)
enum cam_orient {
  CAM_QUAT,
  CAM_MAT
};
enum cam_output_z_range {
  CAM_NEG_ONE_TO_ONE,
  CAM_NEG_ONE_TO_ZERO,
  CAM_ZERO_TO_ONE
};
enum cam_mode {
  CAM_PERSP,
  CAM_ORTHO,
};
struct cam_persp {
  float fov;
  float aspect_ratio;
};
struct cam_ortho {
  int left;
  int right;
  int bottom;
  int top;
};
enum cam_plane_id {
  PLANE_LEFT,
  PLANE_RIGHT,
  PLANE_TOP,
  PLANE_BOTTOM,
  PLANE_NEAR,
  PLANE_FAR,
  PLANE_CNT
};
enum cam_intersect {
  CAM_PLANE_OUTSIDE,
  CAM_PLANE_INSIDE,
  CAM_PLANE_INTERSECTS,
};
struct cam {
  /* in: proj */
  enum cam_mode mode;
  union {
    struct cam_persp persp;
    struct cam_ortho ortho;
  };
  enum cam_output_z_range zout;
  float near, far;
  float z_range_epsilon;

  /* in: view */
  enum cam_orient orient;
  float pos[3];
  float off[3];
  float ear[3];
  float q[4];
  float m[3][3];

  /* out: */
  float view[4][4];
  float view_inv[4][4];
  float proj[4][4];
  float proj_inv[4][4];
  float view_proj[4][4];
  union {
    struct cam_planes {
      float left[4];
      float right[4];
      float top[4];
      float bot[4];
      float near[4];
      float far[4];
    } plane;
    float planes[4*PLANE_CNT];
  };
  float forward[3];
  float backward[3];
  float right[3];
  float down[3];
  float left[3];
  float up[3];
};
#define cam_move(c,t) cam_movef(c, t[0], t[1], t[2])
#define cam_lookat(c, eye, ctr, up)\
  cam_lookatf((c), (eye)[0], (eye)[1], (eye)[2], (ctr)[0], (ctr)[1], (ctr)[2], (up)[0], (up)[1], (up)[2])

static void
cam_init(struct cam *c) {
  static const float qid[] = {0,0,0,1};
  static const float m4id[] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};

  c->mode = CAM_PERSP;
  c->persp.aspect_ratio = 3.0f/2.0f;
  c->persp.fov = FLT_PI / 4.0f;
  c->near = 0.01f;
  c->far = 10000;

  mcpy(c->q, qid, sizeof(qid));
  mcpy(c->view, m4id, sizeof(m4id));
  mcpy(c->view_inv, m4id, sizeof(m4id));
  mcpy(c->proj, m4id, sizeof(m4id));
  mcpy(c->proj_inv, m4id, sizeof(m4id));
}
static void
cam_build(struct cam *c) {
  assert(c);
  /* convert orientation matrix into quaternion */
  if (c->orient == CAM_MAT) {
    float s,t,
    trace = c->m[0][0];
    trace += c->m[1][1];
    trace += c->m[2][2];
    if (trace > 0.0f) {
      t = trace + 1.0f;
      s = math_sqrt((1.0f/t)) * 0.5f;

      c->q[3] = s * t;
      c->q[0] = (c->m[2][1] - c->m[1][2]) * s;
      c->q[1] = (c->m[0][2] - c->m[2][0]) * s;
      c->q[2] = (c->m[1][0] - c->m[0][1]) * s;
    } else {
      int i = 0, j, k;
      static const int next[] = {1,2,0};
      if (c->m[1][1] > c->m[0][0] ) i = 1;
      if (c->m[2][2] > c->m[i][i] ) i = 2;

      j = next[i]; k = next[j];
      t = (c->m[i][i] - (c->m[j][j] - c->m[k][k])) + 1.0f;
      s = math_sqrt((1.0f/t)) * 0.5f;

      c->q[i] = s*t;
      c->q[3] = (c->m[k][j] - c->m[j][k]) * s;
      c->q[j] = (c->m[j][i] + c->m[i][j]) * s;
      c->q[k] = (c->m[k][i] + c->m[i][k]) * s;
    }
    /* normalize quaternion */
    float len2 = c->q[0] * c->q[0] + c->q[1] * c->q[1];
    len2 += c->q[2] * c->q[2] + c->q[3]*c->q[3];
    if (len2 != 0.0f) {
      float len = math_sqrt(len2);
      float inv_len = 1.0f/len;
      c->q[0] *= inv_len; c->q[1] *= inv_len;
      c->q[2] *= inv_len; c->q[3] *= inv_len;
    }
  }
  /* Camera euler orientation
  It is not feasible to multiply euler angles directly together to represent the camera
  orientation because of gimbal lock (Even quaternions do not save you against
  gimbal lock under all circumstances). While it is true that it is not a problem for
  FPS style cameras, it is a problem for free cameras. To fix that issue this camera only
  takes in the relative angle rotation and does not store the absolute angle values [7].*/
  {
    float sx, cx; math_sin_cos(&sx,&cx,c->ear[0]*0.5f);
    float sy, cy; math_sin_cos(&sy,&cy,c->ear[1]*0.5f);
    float sz, cz; math_sin_cos(&sz,&cz,c->ear[2]*0.5f);

    float a[4], b[4];
    a[0] = cz*sx; a[1] = sz*sx; a[2] = sz*cx; a[3] = cz*cx;
    b[0] = c->q[0]*cy - c->q[2]*sy;
    b[1] = c->q[3]*sy + c->q[1]*cy;
    b[2] = c->q[2]*cy + c->q[0]*sy;
    b[3] = c->q[3]*cy - c->q[1]*sy;

    c->q[0] = a[3]*b[0] + a[0]*b[3] + a[1]*b[2] - a[2]*b[1];
    c->q[1] = a[3]*b[1] + a[1]*b[3] + a[2]*b[0] - a[0]*b[2];
    c->q[2] = a[3]*b[2] + a[2]*b[3] + a[0]*b[1] - a[1]*b[0];
    c->q[3] = a[3]*b[3] - a[0]*b[0] - a[1]*b[1] - a[2]*b[2];
    mset(c->ear, 0, sizeof(c->ear));
  }
  /* Convert quaternion to matrix
  Next up we want to convert our camera quaternion orientation into a 3x3 matrix
  to generate our view matrix. So to convert from quaternion to rotation
  matrix we first look at how to transform a vector quaternion by and how by matrix.
  To transform a vector by a unit quaternion you turn the vector into a zero w
  quaternion and left multiply by quaternion and right multiply by quaternion inverse:
      p2  = q * q(P1) * pi
          = q(qx,qy,qz,qw) * q(x,y,z,0) * q(-qx,-qy,-qz,qw)

  To get the same result with a rotation matrix you just multiply the vector by matrix:
      p2 = M * p1

  So to get the matrix M you first multiply out the quaternion transformation and
  group each x,y and z term into a column. The end result is: */
  {
    float x2 = c->q[0] + c->q[0];
    float y2 = c->q[1] + c->q[1];
    float z2 = c->q[2] + c->q[2];

    float xx = c->q[0]*x2;
    float xy = c->q[0]*y2;
    float xz = c->q[0]*z2;

    float yy = c->q[1]*y2;
    float yz = c->q[1]*z2;
    float zz = c->q[2]*z2;

    float wx = c->q[3]*x2;
    float wy = c->q[3]*y2;
    float wz = c->q[3]*z2;

    c->m[0][0] = 1.0f - (yy + zz);
    c->m[0][1] = xy - wz;
    c->m[0][2] = xz + wy;

    c->m[1][0] = xy + wz;
    c->m[1][1] = 1.0f - (xx + zz);
    c->m[1][2] = yz - wx;

    c->m[2][0] = xz - wy;
    c->m[2][1] = yz + wx;
    c->m[2][2] = 1.0f - (xx + yy);
  }
  /* View matrix
  The general transform pipeline is object space to world space to local camera
  space to screenspace to clipping space. While this particular matrix, the view matrix,
  transforms from world space to local camera space.

  So we start by trying to find the camera world transform a 4x3 matrix which
  can and will be in this particular implementation extended to a 4x4 matrix
  composed of camera rotation, camera translation and camera offset.
  While pure camera position and orientation is usefull for free FPS style cameras,
  allows the camera offset 3rd person cameras or tracking ball behavior.

      T.......camera position
      F.......camera offset
      R.......camera orientation 3x3 matrix
      C.......camera world transform 4x4 matrix

          |1 T|   |R 0|   |1 F|
      C = |0 1| * |0 1| * |0 1|

          |R   RF+T|
      C = |0      1|
  */
  {
    /* 1.) copy orientation matrix */
    c->view_inv[0][0] = c->m[0][0]; c->view_inv[0][1] = c->m[0][1];
    c->view_inv[0][2] = c->m[0][2]; c->view_inv[1][0] = c->m[1][0];
    c->view_inv[1][1] = c->m[1][1]; c->view_inv[1][2] = c->m[1][2];
    c->view_inv[2][0] = c->m[2][0]; c->view_inv[2][1] = c->m[2][1];
    c->view_inv[2][2] = c->m[2][2];

    /* 2.) transform offset by camera orientation and add translation */
    c->view_inv[3][0] = c->view_inv[0][0] * c->off[0];
    c->view_inv[3][1] = c->view_inv[0][1] * c->off[0];
    c->view_inv[3][2] = c->view_inv[0][2] * c->off[0];

    c->view_inv[3][0] += c->view_inv[1][0] * c->off[1];
    c->view_inv[3][1] += c->view_inv[1][1] * c->off[1];
    c->view_inv[3][2] += c->view_inv[1][2] * c->off[1];

    c->view_inv[3][0] += c->view_inv[2][0] * c->off[2];
    c->view_inv[3][1] += c->view_inv[2][1] * c->off[2];
    c->view_inv[3][2] += c->view_inv[2][2] * c->off[2];

    c->view_inv[3][0] += c->pos[0];
    c->view_inv[3][1] += c->pos[1];
    c->view_inv[3][2] += c->pos[2];

    /* 3.) fill last empty 4x4 matrix row */
    c->view_inv[0][3] = 0;
    c->view_inv[1][3] = 0;
    c->view_inv[2][3] = 0;
    c->view_inv[3][3] = 1.0f;
  }
  /* Now we have a matrix to transform from local camera space into world
  camera space. But remember we are looking for the opposite transform since we
  want to transform the world around the camera. So to get the other way
  around we have to invert our world camera transformation to transform to
  local camera space.

  Usually inverting matricies is quite a complex endeavour both in needed complexity
  as well as number of calculations required. Luckily we can use a nice property
  of orthonormal matrices (matrices with each column being unit length)
  on the more complicated matrix the rotation matrix.
  The inverse of orthonormal matrices is the same as the transpose of the same
  matrix, which is just a matrix column to row swap.

  So with inverse rotation matrix covered the only thing left is the inverted
  camera translation which just needs to be negated plus and this is the more
  important part tranformed by the inverted rotation matrix.
  Why? simply because the inverse of a matrix multiplication M = A * B
  is NOT Mi = Ai * Bi (i for inverse) but rather Mi = Bi * Ai.
  So if we put everything together we get the following view matrix:

      R.......camera orientation matrix3x3
      T.......camera translation
      F.......camera offset
      Ti......camera inverse translation
      Fi......camera inverse offset
      Ri......camera inverse orientation matrix3x3
      V.......view matrix

             (|R   Rf+T|)
      V = inv(|0      1|)

          |Ri -Ri*Ti-Fi|
      V = |0          1|

  Now we finally have our matrix composition and can fill out the view matrix:
  */
  {
    /*1.) Inverse camera orientation by transpose */
    c->view[0][0] = c->m[0][0];
    c->view[0][1] = c->m[1][0];
    c->view[0][2] = c->m[2][0];

    c->view[1][0] = c->m[0][1];
    c->view[1][1] = c->m[1][1];
    c->view[1][2] = c->m[2][1];

    c->view[2][0] = c->m[0][2];
    c->view[2][1] = c->m[1][2];
    c->view[2][2] = c->m[2][2];

    /* 2.) Transform inverted position vector by transposed orientation and subtract offset */
    float pos_inv[3];
    pos_inv[0] = -c->pos[0];
    pos_inv[1] = -c->pos[1];
    pos_inv[2] = -c->pos[2];

    c->view[3][0] = c->view[0][0] * pos_inv[0];
    c->view[3][1] = c->view[0][1] * pos_inv[0];
    c->view[3][2] = c->view[0][2] * pos_inv[0];

    c->view[3][0] += c->view[1][0] * pos_inv[1];
    c->view[3][1] += c->view[1][1] * pos_inv[1];
    c->view[3][2] += c->view[1][2] * pos_inv[1];

    c->view[3][0] += c->view[2][0] * pos_inv[2];
    c->view[3][1] += c->view[2][1] * pos_inv[2];
    c->view[3][2] += c->view[2][2] * pos_inv[2];

    c->view[3][0] -= c->off[0];
    c->view[3][1] -= c->off[1];
    c->view[3][2] -= c->off[2];

    /* 3.) fill last empty 4x4 matrix row */
    c->view[0][3] = 0; c->view[1][3] = 0;
    c->view[2][3] = 0; c->view[3][3] = 1.0f;
  }
  {
    /* fill vectors with data */
    c->right[0] = c->view_inv[0][0];
    c->right[1] = c->view_inv[0][1];
    c->right[2] = c->view_inv[0][2];

    c->left[0] = -c->view_inv[0][0];
    c->left[1] = -c->view_inv[0][1];
    c->left[2] = -c->view_inv[0][2];

    c->up[0] = c->view_inv[1][0];
    c->up[1] = c->view_inv[1][1];
    c->up[2] = c->view_inv[1][2];

    c->down[0] = -c->view_inv[1][0];
    c->down[1] = -c->view_inv[1][1];
    c->down[2] = -c->view_inv[1][2];

    c->forward[0] = c->view_inv[2][0];
    c->forward[1] = c->view_inv[2][1];
    c->forward[2] = c->view_inv[2][2];

    c->backward[0] = -c->view_inv[2][0];
    c->backward[1] = -c->view_inv[2][1];
    c->backward[2] = -c->view_inv[2][2];
  }
  /*  Projection matrix
  While the view matrix transforms from world space to local camera space,
  tranforms the perspective projection matrix from camera space to screen space.

  The actual work for the transformation is from eye coordinates camera
  frustum far plane to a cube with coordinates (-1,1), (0,1), (-1,0) depending on
  argument `out_z_range` in this particual implementation.
  */
  c->proj[0][1] = c->proj[0][2] = c->proj[0][3] = 0;
  c->proj[1][0] = c->proj[1][2] = c->proj[1][3] = 0;
  c->proj[2][0] = c->proj[2][1] = 0;
  c->proj[3][0] = c->proj[3][1] = c->proj[3][3] = 0;

  switch (c->mode) {
  case CAM_PERSP: {
    /*To actually build the perspective projection matrix we need:
      - Vertical field of view angle
      - Screen aspect ratio which controls the horizontal view angle in
          contrast to the field of view.
      - Z coordinate of the frustum near clipping plane
      - Z coordinate of the frustum far clipping plane

    While I will explain how to incooperate the near,far z clipping
    plane I would recommend reading [7] for the other values since it is quite
    hard to do understand without visual aid and he is probably better in
    explaining than I would be.*/
    float hfov = math_tan((c->persp.fov*0.5f));
    c->proj[0][0] = 1.0f/(c->persp.aspect_ratio * hfov);
    c->proj[1][1] = 1.0f/hfov;
    c->proj[2][3] = -1.0f;
  } break;
  case CAM_ORTHO: {
    c->proj[0][0] = 2.0f / (c->ortho.right - c->ortho.left);
    c->proj[1][1] = 2.0f / (c->ortho.top - c->ortho.bottom);
    c->proj[3][0] = - (c->ortho.right + c->ortho.left) / (c->ortho.right - c->ortho.left);
    c->proj[3][1] = - (c->ortho.top + c->ortho.bottom) / (c->ortho.top - c->ortho.bottom);
  } break;
  }
  if (c->far >= 0) {
    /* We are still missing A and B to map between the frustum near/far
    and the clipping cube near/far value. So we take the lower right part
    of our projection matrix and multiply by a vector containing the missing
    z and w value, which gives us the resulting clipping cube z;

        |A  B|   |z|    |Az + B |           B
        |-1 0| * |1| =  | -z    | = -A + ------
                                           -z

    So far so good but now we need to map from the frustum near,
    far clipping plane (n,f) to the clipping cube near and far plane (cn,cf).
    So we plugin the frustum near/far values into z and the resulting
    cube near/far plane we want to end up with.

               B                    B
        -A + ------ = cn and -A + ------ = cf
               n                    f

    We now have two equations with two unkown A and B since n,f as well
    as cn,cf are provided, so we can solved them by subtitution either by
    hand or, if you are like me prone to easily make small mistakes,
    with WolframAlpha which solved for:*/
    switch (c->zout) {
      default:
      case CAM_NEG_ONE_TO_ONE: {
        /* cn = -1 and cf = 1: */
        c->proj[2][2] = -(c->far + c->near) / (c->far - c->near);
        c->proj[3][2] = -(2.0f * c->far * c->near) / (c->far - c->near);
      } break;
      case CAM_NEG_ONE_TO_ZERO: {
        /* cn = -1 and cf = 0: */
        c->proj[2][2] = (c->near) / (c->near - c->far);
        c->proj[3][2] = (c->far * c->near) / (c->near - c->far);
      } break;
      case CAM_ZERO_TO_ONE: {
        /* cn = 0 and cf = 1: */
        c->proj[2][2] = -(c->far) / (c->far - c->near);
        c->proj[3][2] = -(c->far * c->near) / (c->far - c->near);
      } break;
    }
  } else {
    /* Infinite projection [1]:
    In general infinite projection matrices map direction to points on the
    infinite distant far plane, which is mainly useful for rendering:
      - skyboxes, sun, moon, stars
      - stencil shadow volume caps

    To actually calculate the infinite perspective matrix you let the
    far clip plane go to infinity. Once again I would recommend using and
    checking WolframAlpha to make sure all values are correct, while still
    doing the calculation at least once by hand.

    While it is mathematically correct to go to infinity, floating point errors
    result in a depth smaller or bigger. This in term results in
    fragment culling since the hardware thinks fragments are beyond the
    far clipping plane. The general solution is to introduce an epsilon
    to fix the calculation error and map it to the infinite far plane.

    Important:
    For 32-bit floating point epsilon should be greater than 2.4*10^-7,
    to account for floating point precision problems. */
    switch (c->zout) {
    default:
    case CAM_NEG_ONE_TO_ONE: {
      /* lim f->inf -((f+n)/(f-n)) => -((inf+n)/(inf-n)) => -(inf)/(inf) => -1.0*/
      c->proj[2][2] = c->z_range_epsilon - 1.0f;
      /* lim f->inf -(2*f*n)/(f-n) => -(2*inf*n)/(inf-n) => -(2*inf*n)/inf => -2n*/
      c->proj[3][2] = (c->z_range_epsilon - 2.0f) * c->near;
    } break;
    case CAM_NEG_ONE_TO_ZERO: {
      /* lim f->inf  n/(n-f) => n/(n-inf) => n/(n-inf) => -0 */
      c->proj[2][2] = c->z_range_epsilon;
      /* lim f->inf (f*n)/(n-f) => (inf*n)/(n-inf) => (inf*n)/-inf = -n */
      c->proj[3][2] = (c->z_range_epsilon - 1.0f) * c->near;
    } break;
    case CAM_ZERO_TO_ONE: {
      /* lim f->inf (-f)/(f-n) => (-inf)/(inf-n) => -inf/inf = -1 */
      c->proj[2][2] = c->z_range_epsilon - 1.0f;
      /* lim f->inf (-f*n)/(f-n) => (-inf*n)/(inf-n) => (-inf*n)/(inf) => -n */
      c->proj[3][2] = (c->z_range_epsilon - 1.0f) * c->near;
    } break;}
  }
  /* Invert projection [2][3]:
  Since perspective matrices have a fixed layout, it makes sense
  to calculate the specific perspective inverse instead of relying on a default
  matrix inverse function. Actually calculating the matrix for any perspective
  matrix is quite straight forward:

      I.......identity matrix
      p.......perspective matrix
      I(p)....inverse perspective matrix

  1.) Fill a variable inversion matrix and perspective layout matrix into the
      inversion formula: I(p) * p = I

      |x0  x1  x2  x3 |   |a 0 0 0|   |1 0 0 0|
      |x4  x5  x6  x7 | * |0 b 0 0| = |0 1 0 0|
      |x8  x9  x10 x11|   |0 0 c d|   |0 0 1 0|
      |x12 x13 x14 x15|   |0 0 e 1|   |0 0 0 1|

  2.) Multiply inversion matrix times our perspective matrix

      |x0*a x1*b x2*c+x3*e x2*d|      |1 0 0 0|
      |x4*a x5*b x6*c+x7*e x6*d|    = |0 1 0 0|
      |x8*a x9*b x10*c+x11*e x10*d|   |0 0 1 0|
      |x12*a x13*b x14*c+x15*e x14*d| |0 0 0 1|

  3.) Finally substitute each x value:
      e.g: x0*a = 1 => x0 = 1/a
      so I(p) at column 0, row 0 is 1/a.

              |1/a 0 0 0|
      I(p) =  |0 1/b 0 0|
              |0 0 0 1/e|
              |0 0 1/d -c/de|

  These steps basically work for any invertable matrices, but I would recommend
  using WolframAlpha for these specific kinds of matrices, since it can
  automatically generate inversion matrices without any fuss or possible
  human calculation errors. */
  memset(c->proj_inv, 0, sizeof(c->proj_inv));
  switch (c->mode) {
  case CAM_PERSP: {
    c->proj_inv[0][0] = 1.0f/c->proj[0][0];
    c->proj_inv[1][1] = 1.0f/c->proj[1][1];
    c->proj_inv[2][3] = 1.0f/c->proj[3][2];
    c->proj_inv[3][2] = 1.0f/c->proj[2][3];
    c->proj_inv[3][3] = -c->proj[2][2]/(c->proj[3][2] * c->proj[2][3]);
  } break;
  case CAM_ORTHO: {
    c->proj_inv[0][0] = 1.0f/c->proj[0][0];
    c->proj_inv[1][1] = 1.0f/c->proj[1][1];
    c->proj_inv[2][2] = 1.0f/c->proj[2][2];
    c->proj_inv[3][0] = -c->proj[3][0]/c->proj[0][0];
    c->proj_inv[3][1] = -c->proj[3][1]/c->proj[1][1];
    c->proj_inv[3][2] = -c->proj[3][2]/c->proj[2][2];
    c->proj_inv[3][3] = 1.0f;
  } break;
  }
  /* calculate combined view projection matrix */
  {
    float *m1 = cast(float*, c->view);
    float *m2 = cast(float*, c->proj);
    float *dst = cast(float*, c->view_proj);
    for_cnt(i,4) {
      for_cnt(j,4) {
        float a0 = m1[0] * m2[0*4+j];
        float b0 = m1[1] * m2[1*4+j];
        float c0 = m1[2] * m2[2*4+j];
        float d0 = m1[3] * m2[3*4+j];
        *dst++ = a0 + b0 + c0 + d0;
      }
      m1 += 4;
    }
  }
  {
    /* calculate the 6 planes enclosing the camera volume */
    c->plane.left[0] = c->view_proj[0][3] + c->view_proj[0][0];
    c->plane.left[1] = c->view_proj[1][3] + c->view_proj[1][0];
    c->plane.left[2] = c->view_proj[2][3] + c->view_proj[2][0];
    c->plane.left[3] = c->view_proj[3][3] + c->view_proj[3][0];

    c->plane.right[0] = c->view_proj[0][3] - c->view_proj[0][0];
    c->plane.right[1] = c->view_proj[1][3] - c->view_proj[1][0];
    c->plane.right[2] = c->view_proj[2][3] - c->view_proj[2][0];
    c->plane.right[3] = c->view_proj[3][3] - c->view_proj[3][0];

    c->plane.bot[0] = c->view_proj[0][3] + c->view_proj[0][1];
    c->plane.bot[1] = c->view_proj[1][3] + c->view_proj[1][1];
    c->plane.bot[2] = c->view_proj[2][3] + c->view_proj[2][1];
    c->plane.bot[3] = c->view_proj[3][3] + c->view_proj[3][1];

    c->plane.top[0] = c->view_proj[0][3] - c->view_proj[0][1];
    c->plane.top[1] = c->view_proj[1][3] - c->view_proj[1][1];
    c->plane.top[2] = c->view_proj[2][3] - c->view_proj[2][1];
    c->plane.top[3] = c->view_proj[3][3] - c->view_proj[3][1];

    c->plane.near[0] = c->view_proj[0][3] + c->view_proj[0][2];
    c->plane.near[1] = c->view_proj[1][3] + c->view_proj[1][2];
    c->plane.near[2] = c->view_proj[2][3] + c->view_proj[2][2];
    c->plane.near[3] = c->view_proj[3][3] + c->view_proj[3][2];

    c->plane.top[0] = c->view_proj[0][3] - c->view_proj[0][2];
    c->plane.top[1] = c->view_proj[1][3] - c->view_proj[1][2];
    c->plane.top[2] = c->view_proj[2][3] - c->view_proj[2][2];
    c->plane.top[3] = c->view_proj[3][3] - c->view_proj[3][2];

    normaleq4(c->plane.left);
    normaleq4(c->plane.right);
    normaleq4(c->plane.bot);
    normaleq4(c->plane.top);
    normaleq4(c->plane.near);
    normaleq4(c->plane.far);
  }
}
static void
cam_lookatf(struct cam *c,
    float eye_x, float eye_y, float eye_z,
    float ctr_x, float ctr_y, float ctr_z,
    float up_x, float up_y, float up_z) {
  float f[3], u[3], r[3];
  f[0] = ctr_x - eye_x,
  f[1] = ctr_y - eye_y,
  f[2] = ctr_z - eye_z;

  /* calculate right vector */
  r[0] = (f[1]*up_z) - (f[2]*up_y);
  r[1] = (f[2]*up_x) - (f[0]*up_z);
  r[2] = (f[0]*up_y) - (f[1]*up_x);

  /* calculate up vector */
  u[0] = (r[1]*f[2]) - (r[2]*f[1]);
  u[1] = (r[2]*f[0]) - (r[0]*f[2]);
  u[2] = (r[0]*f[1]) - (r[1]*f[0]);

  /* normlize vectors */
  float fl = f[0]*f[0]+f[1]*f[1]+f[2]*f[2];
  float rl = r[0]*r[0]+r[1]*r[1]+r[2]*r[2];
  float ul = u[0]*u[0]+u[1]*u[1]+u[2]*u[2];

  fl = (fl == 0.0f) ? 1.0f: 1/math_sqrt(fl);
  rl = (rl == 0.0f) ? 1.0f: 1/math_sqrt(rl);
  ul = (ul == 0.0f) ? 1.0f: 1/math_sqrt(ul);

  f[0] *= fl, f[1] *= fl, f[2] *= fl;
  r[0] *= rl, r[1] *= rl, r[2] *= rl;
  u[0] *= ul, u[1] *= ul, u[2] *= ul;

  /* setup camera */
  c->pos[0] = eye_x, c->pos[1] = eye_y, c->pos[2] = eye_z;
  c->m[0][0] = r[0], c->m[0][1] = r[1], c->m[0][2] = r[2];
  c->m[1][0] = u[0], c->m[1][1] = u[1], c->m[1][2] = u[2];
  c->m[2][0] = f[0], c->m[2][1] = f[1], c->m[2][2] = f[2];

  /* build camera */
  enum cam_orient orient = c->orient;
  c->orient = CAM_MAT;
  memset(c->ear, 0, sizeof(c->ear));
  cam_build(c);
  c->orient = orient;
}
static void
cam_movef(struct cam *c, float x, float y, float z) {
  c->pos[0] += c->view_inv[0][0] * x;
  c->pos[1] += c->view_inv[0][1] * x;
  c->pos[2] += c->view_inv[0][2] * x;

  c->pos[0] += c->view_inv[1][0] * y;
  c->pos[1] += c->view_inv[1][1] * y;
  c->pos[2] += c->view_inv[1][2] * y;

  c->pos[0] += c->view_inv[2][0] * z;
  c->pos[1] += c->view_inv[2][1] * z;
  c->pos[2] += c->view_inv[2][2] * z;
}
static void
cam_screen_to_world(float *res, const struct cam *c, float width, float height,
                    float screen_x, float screen_y, float cam_z) {
  /* Screen space to world space coordinates
  To convert from screen space coordinates to world coordinates we
  basically have to revert all transformations typically done to
  convert from world space to screen space:
      Viewport => NDC => Clip => View => World

  Viewport => NDC => Clip
  -----------------------
  First up is the transform from viewport to clipping space.
  To get from clipping space to viewport we calculate:

                  |((x+1)/2)*w|
      Vn = v =    |((1-y)/2)*h|
                  |((z+1)/2)  |

  Now we need to the the inverse process by solvinging for n:
              |(2*x)/w - 1|
      n =     |(2*y)/h    |
              |(2*z)-1)   |
              | 1         |
  */
  float x = (screen_x / width * 2.0f) - 1.0f;
  float y = (screen_y / height) * 2.0f - 1.0f;
  float z = 2.0f * cam_z - 1.0f;

  /* Clip => View
  -----------------------
  A vector v or position p in view space is tranform to clip
  coordinates c by being transformed by a projection matrix P:

      c = P * v

  To convert from clipping coordinates c to view coordinates we
  just have to transfrom c by the inverse projection matrix Pi:

      v = Pi * c

  The inverse projection matrix for all common projection matrices
  can be calculated by (see cam_build for more information):

              |1/a 0 0 0|
      Pi  =   |0 1/b 0 0|
              |0 0 0 1/e|
              |0 0 1/d -c/de|

  View => World
  -----------------------
  Finally we just need to convert from view coordinates to world
  coordinates w by transforming our view coordinates by the inverse view
  matrix Vi which in this context is just the camera translation and
  rotation.

      w = Vi * v

  Now we reached our goal and have our world coordinates. This implementation
  combines both the inverse projection as well as inverse view transformation
  into one since the projection layout is known we can do some optimization:*/
  float ax = c->proj_inv[0][0]*x;
  float by = c->proj_inv[1][1]*y;
  float dz = c->proj_inv[2][3]*z;
  float w = c->proj_inv[3][3] + dz;

  res[0] = c->proj_inv[3][2] * c->view_inv[2][0];
  res[0] += c->proj_inv[3][3] * c->view_inv[3][0];
  res[0] += ax * c->view_inv[0][0];
  res[0] += by * c->view_inv[1][0];
  res[0] += dz * c->view_inv[3][0];

  res[1] = c->proj_inv[3][2] * c->view_inv[2][1];
  res[1] += c->proj_inv[3][3] * c->view_inv[3][1];
  res[1] += ax * c->view_inv[0][1];
  res[1] += by * c->view_inv[1][1];
  res[1] += dz * c->view_inv[3][1];

  res[2] = c->proj_inv[3][2] * c->view_inv[2][2];
  res[2] += c->proj_inv[3][3] * c->view_inv[3][2];
  res[2] += ax * c->view_inv[0][2];
  res[2] += by * c->view_inv[1][2];
  res[2] += dz * c->view_inv[3][2];
  res[0] /= w; res[1] /= w; res[2] /= w;
}
static void
cam_ray(float *ro, float *rd, const struct cam *c,
        float w, float h, float mx, float my) {
  float world[3];
  cam_screen_to_world(world, c, w, h, mx, my, 0);
  /* calculate direction
  We generate the ray normal vector by first transforming the mouse cursor position
  from screen coordinates into world coordinates. After that we only have to
  subtract our camera position from our calculated mouse world position and
  normalize the result to make sure we have a unit vector as direction. */
  ro[0] = c->pos[0];
  ro[1] = c->pos[1];
  ro[2] = c->pos[2];

  rd[0] = world[0] - ro[0];
  rd[1] = world[1] - ro[1];
  rd[2] = world[2] - ro[2];

  /* normalize */
  float dot = rd[0]*rd[0] + rd[1]*rd[1] + rd[2]*rd[2];
  if (dot != 0.0f) {
    float len = math_sqrt(dot);
    rd[0] /= len; rd[1] /= len; rd[2] /= len;
  }
}
static enum cam_intersect
cam_intersect_pnt(const struct cam *c, const float *restrict pnt) {
  for (int i = 0; i < 6; ++i) {
    float d = plane_dist_pnt(c->planes+i*4, pnt);
    if (d < 0.0f) {
      return CAM_PLANE_OUTSIDE;
    }
  }
  return CAM_PLANE_INSIDE;
}
static enum cam_intersect
cam_intersect_sphere(const struct cam *c, const float *s) {
  for (int i = 0; i < 6; ++i) {
    float d = plane_dist_pnt(c->planes+i*4, s);
    if (d + s[3] < 0) {
      return CAM_PLANE_OUTSIDE;
    }
    if (d - s[3] < 0) {
      return CAM_PLANE_INTERSECTS;
    }
  }
  return CAM_PLANE_INSIDE;
}
static enum cam_intersect
cam_intersect_aabb(const struct cam *c, const float *aabb) {
  float ctr[3]; aabb_ctr(ctr, aabb);
  float ext[3]; aabb_ext(ext, aabb);
  for (int i = 0; i < 6; ++i) {
    float abs_plane[4]; mapN(abs_plane, (float)math_abs, c->planes+i*4,3);
    float d = plane_dist_pnt(c->planes+i*4, ctr);
    float r = dot3(ext, abs_plane);
    if ((d + r) < 0.0f) {
      return CAM_PLANE_OUTSIDE;
    }
    if ((d - r) < 0.0f) {
      return CAM_PLANE_INTERSECTS;
    }
  }
  return CAM_PLANE_INSIDE;
}

