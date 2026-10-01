#include <metal_stdlib>
#include "gfx_shdr.h"

using namespace metal;

// Output struct for the Vertex Shader (includes hardware clip distance)
struct vtx_out {
  float4 pos [[position]];
  float4 col;
  float2 tex_coord;
  float2 local_pos;
  ushort tex;
  ushort is_mask;
  ushort is_tex;
  float clip_dist [[clip_distance]] [4]; // Attribute placed before array size
};
// Input struct for the Fragment Shader (omits vertex-only clip distance)
struct frag_in {
  float4 pos [[position]];
  float4 col;
  float2 tex_coord;
  float2 local_pos;
  ushort tex;
  ushort is_mask;
  ushort is_tex;
};

// ============================================================================
// VERTEX SHADER
// ============================================================================
vertex vtx_out
ren_vtx(uint vid [[vertex_id]],
        const device uint *idxs      [[buffer(0)]],
        const device char *prims     [[buffer(1)]],
        constant gfx_uniform *uni    [[buffer(2)]]) {

  const uint idx    = idxs[vid];
  const uint off    = idx & 0x00ffffff;
  const uint corner = (idx >> 24) & 0x03;

  const device gfx_prim &p = *(const device gfx_prim*)(prims + off);
  const float2 corner_uv = float2(float(corner & 1), float(corner >> 1));

  // 1. Geometry evaluation
  float2 pos_quad = mix(p.p0, p.p1, corner_uv);

  float2 d = p.p1 - p.p0;
  float2 n = float2(-d.y, d.x) / max(length(d), 0.0001f);
  float2 pos_ln = mix(p.p0, p.p1, corner_uv.x) + n * (p.thickness * (corner_uv.y - 0.5f));

  float2 pts[3] = { p.p0, p.p1, p.p2 };
  float2 pos_tri = pts[min(corner, 2u)];

  // Branchless selection
  float2 pos_px = select(pos_quad, pos_ln,  (p.flags & GFX_FLAG_LN) != 0);
  pos_px        = select(pos_px,   pos_tri, (p.flags & GFX_FLAG_TRI) != 0);

  // 2. Vertex outputs
  vtx_out vo;
  vo.col       = unpack_unorm4x8_to_float(p.col);
  vo.tex_coord = mix(p.uv0, p.uv1, corner_uv);
  vo.local_pos = mix(float2(-1.0f), float2(1.0f), corner_uv) * p.circle_mask;
  vo.tex       = p.tex;
  vo.is_mask   = (p.flags & GFX_FLAG_MASK) ? 1 : 0;
  vo.is_tex    = (p.flags & GFX_FLAG_TEX)  ? 1 : 0;

  // 3. Hardware clipping (4 planes)
  const device gfx_clip &c = *(const device gfx_clip*)(prims + p.clip);
  vo.clip_dist[0] = pos_px.x - float(c.l);
  vo.clip_dist[1] = float(c.r) - pos_px.x;
  vo.clip_dist[2] = pos_px.y - float(c.t);
  vo.clip_dist[3] = float(c.b) - pos_px.y;

  // 4. Viewport NDC transform
  float2 view = float2(uni->viewport);
  vo.pos.x = (pos_px.x / view.x) * 2.0f - 1.0f;
  vo.pos.y = -1.0f * ((pos_px.y / view.y) * 2.0f - 1.0f);
  vo.pos.zw = float2(0.0f, 1.0f);
  return vo;
}

// ============================================================================
// FRAGMENT SHADER
// ============================================================================
struct frag_arg {
  array<texture2d<float>, GFX_MAX_TEX_CNT> tex;
};
fragment float4
ren_frag(frag_in vi [[stage_in]], device frag_arg &in [[buffer(0)]]) {
  constexpr sampler smp(coord::normalized, min_filter::linear, mag_filter::linear, mip_filter::linear);

  float4 tex_sample = in.tex[vi.tex].sample(smp, vi.tex_coord);
  float4 mask_col   = float4(1.0f, 1.0f, 1.0f, tex_sample.r);
  float4 tex_col    = select(tex_sample, mask_col, vi.is_mask != 0);
  tex_col           = select(float4(1.0f), tex_col, vi.is_tex != 0);

  float dist = length(vi.local_pos);
  float circle_mask = saturate((1.0f - dist) / max(fwidth(dist), 0.0001f));

  float4 out_col = vi.col * tex_col;
  out_col.a *= circle_mask;
  return out_col;
}
