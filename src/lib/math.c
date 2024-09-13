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
math_flteq(float a, float b, float epsilon) {
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
  float flt_quad = castf(quad);
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
static const int nil2[2];
static const int nil3[3];
static const int nil4[4];

#define op(r,e,a,p,b,i,s) ((r) e (a) p ((b) i (s)))
#define opN(r,e,a,p,b,i,s,N)\
  for (int uniqid(_i_) = 0; uniqid(_i_) < N; ++uniqid(_i_))\
    op((r)[uniqid(_i_)],e,(a)[uniqid(_i_)],p,(b)[uniqid(_i_)],i,s)
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
  } while(0)
#define lenN(r,a,N)\
  do {float uniqid(_l_); dotN(uniqid(_l_),a,a,N);\
    (r) = math_sqrt(uniqid(_l_));} while(0)

#define minN(r,a,b,N) map2N(r,min,a,b,N)
#define maxN(r,a,b,N) map2N(r,max,a,b,N)
#define clampN(r,i,v,x,N) map3N(r,clamp,i,v,x,N)

#define op2(r,e,a,p,b,i,s) opN(r,e,a,p,b,i,s,2)
#define op3(r,e,a,p,b,i,s) opN(r,e,a,p,b,i,s,3)
#define op4(r,e,a,p,b,i,s) opN(r,e,a,p,b,i,s,4)

/* vector 2D */
#define set2(d,x,y)     (d)[0] = x, (d)[1] = y
#define zero2(d)        set2(d,0,0)
#define dup2(d,f)       set2(d,f,f)
#define cpy2(d, s)      (d)[0] = (s)[0], (d)[1] = (s)[1]
#define add2(d,a,b)     op2(d,=,a,+,b,+,0)
#define sub2(d,a,b)     op2(d,=,a,-,b,+,0)
#define mul2(d,a,s)     op2(d,=,nil2,+,a,*,s)
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
#define map2(r,fn,a)    mapN(r,fn,a,2)

/* vector 3D */
#define set3(v,x,y,z)   (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define dup3(d,f)       set3(d,f,f,f)
#define zero3(v)        set3(v,0,0,0)
#define cpy3(d,s)       (d)[0]=(s)[0],(d)[1]=(s)[1],(d)[2]=(s)[2]
#define add3(d,a,b)     op3(d,=,a,+,b,+,0)
#define sub3(d,a,b)     op3(d,=,a,-,b,+,0)
#define mul3(d,a,s)     op3(d,=,nil3,+,a,*,s)
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
#define map3(r,fn,a)    mapN(r,fn,a,3)
#define clamp3(r,i,v,x) clampN(r,i,v,x,3)
#define box3(r,a,b,c,cross) do {cross3(cross, a, b); (r) = dot3(n,cross);} while(0)
#define cross3(d,a,b) do {\
  (d)[0] = ((a)[1]*(b)[2]) - ((a)[2]*(b)[1]),\
  (d)[1] = ((a)[2]*(b)[0]) - ((a)[0]*(b)[2]),\
  (d)[2] = ((a)[0]*(b)[1]) - ((a)[1]*(b)[0]);\
} while(0)

/* vector 4D */
#define set4(v,x,y,z,w) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z),(v)[3] =(w)
#define set4w(d,s,w)    cpy3(d,s), (d)[3]=w
#define dup4(d,f)       set4(d,f,f,f,f)
#define cpy4(d,s)       (d)[0]=(s)[0],(d)[1]=(s)[1],(d)[2]=(s)[2],(d)[3]=(s)[3]
#define add4(d,a,b)     op4(d,=,a,+,b,+,0)
#define sub4(d,a,b)     op4(d,=,a,-,b,+,0)
#define mul4(d,a,s)     op4(d,=,nil4,+,a,*,s)
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
#define map4(r,fn,a)    mapN(r,fn,a,4)
#define clamp4(r,i,v,x) clampN(r,i,v,x,4)
#define swzl4(r,f,a,b,c,e) set4(r,(f)[a],(f)[b],(f)[c],(f)[e])
#define qid(q)          set4(q,0,0,0,1)
#define qconj(d,s)      do{mul3(d,s,-1.0f);(d)[3]=(s)[3];}while(0)
#define qinv(d,s)       qconj(d,s)
#define quat(q,n,x)     quatf(q,n,(x)[0],(x)[1],(x)[2])
#define qmul(q,a,b)\
  (q)[0] = (a)[3]*(b)[0] + (a)[0]*(b)[3] + (a)[1]*b[2] - (a)[2]*(b)[1],\
  (q)[1] = (a)[3]*(b)[1] + (a)[1]*(b)[3] + (a)[2]*b[0] - (a)[0]*(b)[2],\
  (q)[2] = (a)[3]*(b)[2] + (a)[2]*(b)[3] + (a)[0]*b[1] - (a)[1]*(b)[0],\
  (q)[3] = (a)[3]*(b)[3] - (a)[0]*(b)[0] - (a)[1]*b[1] - (a)[2]*(b)[2]
// clang-format on

static float
angle3(const float *restrict a3, const float *restrict b3) {
  float c[3]; cpy3(c,a3);
  float d[3]; cpy3(d,b3);
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

#define col_black col_rgb(0,0,0)
#define col_white col_rgb(255,255,255)
#define col_red col_rgb(255,0,0)
#define col_green col_rgb(0,255,0)
#define col_blue col_rgb(0,0,255)

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
m3x3id(float *m) {
  m[0*3+0] = 1.0f, m[0*3+1] = 0.0f, m[0*3+2] = 0.0f;
  m[1*3+0] = 0.0f; m[1*3+1] = 1.0f; m[1*3+2] = 0.0f;
  m[2*3+0] = 0.0f; m[2*3+1] = 0.0f; m[2*3+2] = 1.0f;
}
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
static inline void
qrotX(float *restrict out, const float *restrict qrot) {
  assert(out);
  assert(qrot);
  /* rotate vector (1, 0, 0) by quat */
  float x = qrot[0], y = qrot[1], z = qrot[2], w = qrot[3];
  float tx = 2.0f * x, tw = 2.0f * w;
  out[0] = tx * x + tw * w - 1.0f;
  out[1] = tx * y + z * tw;
  out[2] = tx * z - y * tw;
}
static inline void
qrotY(float *restrict out, const float *restrict qrot) {
  assert(out);
  assert(qrot);
  /* rotate vector (0, 1, 0) by quat */
  float x = qrot[0], y = qrot[1], z = qrot[2], w = qrot[3];
  float ty = 2.0f * y, tw = 2.0f * w;
  out[0] = x * ty - z * tw;
  out[1] = tw * w + ty * y - 1.0f;
  out[2] = x * tw + ty * z;
}
static inline void
qrotZ(float *restrict out, const float *restrict qrot) {
  assert(out);
  assert(qrot);
  /* rotate vector (0, 0, 1) by quat */
  float x = qrot[0], y = qrot[1], z = qrot[2], w = qrot[3];
  float tz = 2.0f * z, tw = 2.0f * w;
  out[0] = x * tz + y * tw;
  out[1] = y * tz - x * tw;
  out[2] = tw * w + tz * z - 1.0f;
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
  qst__slerp(qres, swing, twist, qfull_swing, qfull_twist, t_swing, t_twist);
}
/* ---------------------------------------------------------------------------
 *                                  Eigen
 * ---------------------------------------------------------------------------
 */
static void
qdiag3(float *restrict qres, const float *restrict A) {
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
eigensym3(float *restrict e_val, float *restrict e_vec33,
          const float *restrict a33) {
  assert(e_val);
  assert(e_vec33);
  assert(a33);

  /* calculate eigensystem from symmetric 3x3 matrix */
  float q[4]; qdiag3(q, a33);
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
static void
mean3(float *restrict mean, const float *restrict pnts, int n) {
  assert(mean);
  assert(pnts);
  zero3(mean);
  if (!n) {
    return;
  }
  for loop(i, n) {
    add3(mean, mean, pnts + i * 3);
  }
  float div = 1.0f / castf(n);
  mul3(mean, mean, div);
}
static void
covar3(float *restrict mat33, const float *restrict pnts, int n) {
  assert(mat33);
  assert(pnts);
  m3x3id(mat33);
  if (n <= 0) {
    return;
  }
  float mean[3] = {0};
  mean3(mean, pnts, n);

  float div = 1.0f / castf(n);
  float xx = 0.0f, yy = 0.0f, zz = 0.0f;
  float xy = 0.0f, xz = 0.0f, yz = 0.0f;
  for loop(i,n) {
    float p[3]; sub3(p, pnts+i*3, mean);
    xx += p[0] * p[0], xy += p[0] * p[1];
    xz += p[0] * p[2], yy += p[1] * p[1];
    yz += p[1] * p[2], zz += p[2] * p[2];
  }
  xx *= div, xy *= div;
  xz *= div, yy *= div;
  yz *= div, zz *= div;

  mat33[0*3+0] = xx;
  mat33[0*3+1] = xy;
  mat33[0*3+2] = xz;

  mat33[1*3+0] = xy;
  mat33[1*3+1] = yy;
  mat33[1*3+2] = yz;

  mat33[2*3+0] = xz;
  mat33[2*3+1] = yz;
  mat33[2*3+2] = zz;
}

