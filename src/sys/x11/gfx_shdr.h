#ifdef __METAL_VERSION__
#include <metal_stdlib>
using namespace metal;
#define GFX_FLOAT2 float2
#define GFX_UINT2  uint2
#else
#include <simd/simd.h>
#define GFX_FLOAT2 simd_float2
#define GFX_UINT2  simd_uint2
#endif

#define GFX_MAX_TEX_CNT (16 * 1024)

enum gfx_flags {
  GFX_FLAG_NONE = 0,
  GFX_FLAG_LN   = 1 << 0,
  GFX_FLAG_TRI  = 1 << 1,
  GFX_FLAG_CIR  = 1 << 2,
  GFX_FLAG_TEX  = 1 << 3,
  GFX_FLAG_MASK = 1 << 4,
};
struct gfx_clip {
  short l, t, r, b;
};
// 64-byte cacheline-aligned unified primitive
struct gfx_prim {
  GFX_FLOAT2 p0;          // (x0, y0) or (l, t)
  GFX_FLOAT2 p1;          // (x1, y1) or (r, b)
  GFX_FLOAT2 p2;          // (x2, y2) for TRI
  GFX_FLOAT2 uv0;         // Normalized UV top-left (0..1)
  GFX_FLOAT2 uv1;         // Normalized UV bottom-right (0..1)
  float      thickness;   // Line thickness
  float      circle_mask; // 1.0f for CIR, 0.0f otherwise
  unsigned   col;         // RGBA8 color
  unsigned   clip;        // Clip byte offset in buffer(1)
  unsigned short tex;     // Texture slot
  unsigned short flags;   // GFX_FLAG_*
  float      _pad[2];     // Alignment padding to 64 bytes
};
struct gfx_uniform {
  GFX_UINT2 viewport;
  GFX_UINT2 tex_siz;
};
