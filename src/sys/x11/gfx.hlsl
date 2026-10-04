#define GFX_FLAG_NONE 0u
#define GFX_FLAG_LN   1u   // 1 << 0
#define GFX_FLAG_TRI  2u   // 1 << 1
#define GFX_FLAG_CIR  4u   // 1 << 2
#define GFX_FLAG_TEX  8u   // 1 << 3
#define GFX_FLAG_MASK 16u  // 1 << 4

// 64-byte aligned primitive layout matching struct gfx_prim in gfx_shdr.h
struct GfxPrim {
  float2 p0;          // Offset 0:  (x0, y0) or (l, t)
  float2 p1;          // Offset 8:  (x1, y1) or (r, b)
  float2 p2;          // Offset 16: (x2, y2) for TRI
  float2 uv0;         // Offset 24: Top-left normalized UV
  float2 uv1;         // Offset 32: Bottom-right normalized UV
  float  thickness;   // Offset 40: Line thickness
  float  circle_mask; // Offset 44: 1.0 for CIR, 0.0 otherwise
  uint   col;         // Offset 48: RGBA8 packed color
  uint   clip;        // Offset 52: Clip byte offset in vertex buffer
  uint   tex_flags;   // Offset 56: Low 16 bits = tex, High 16 bits = flags
  float2 _pad;        // Offset 60: Pad to 64 bytes
};
struct GfxUniform {
  uint2 viewport;
  uint2 tex_siz;
};

/* ---------------------------------------------------------------------------
 * Resource Bindings (Vulkan Descriptor Sets & Push Constants)
 * --------------------------------------------------------------------------- */
[[vk::push_constant]]
ConstantBuffer<GfxUniform> g_uniform;

// Set 0: Memory buffers for index & vertex pulling
[[vk::binding(0, 0)]] ByteAddressBuffer g_idx_buffer : register(t0, space0);
[[vk::binding(1, 0)]] ByteAddressBuffer g_vtx_buffer : register(t1, space0);

// Set 1: Bindless texture table & static sampler
[[vk::binding(0, 1)]] Texture2D    g_textures[] : register(t0, space1);
[[vk::binding(1, 1)]] SamplerState g_sampler    : register(s0, space1);

struct VSOutput {
  float4 pos         : SV_Position;
  float4 col         : COLOR0;
  float2 uv          : TEXCOORD0;
  float2 local_pos   : TEXCOORD1;
  float4 clip_rect   : CLIPRECT;
  nointerpolation uint  tex_id      : TEXID;
  nointerpolation uint  flags       : FLAGS;
  nointerpolation float circle_mask : CIRMASK;
};
VSOutput ren_vtx(uint vtx_id : SV_VertexID) {
  VSOutput output;

  // 1. Fetch packed index (lower 24 bits: prim byte offset; upper 8 bits: corner index)
  uint packed_idx  = g_idx_buffer.Load(vtx_id * 4u);
  uint prim_offset = packed_idx & 0x00FFFFFFu;
  uint corner_idx  = (packed_idx >> 24u) & 0xFFu;

  // 2. Pull the 64-byte primitive structure
  GfxPrim prim = g_vtx_buffer.Load<GfxPrim>(prim_offset);

  uint tex_slot = prim.tex_flags & 0xFFFFu;
  uint flags    = (prim.tex_flags >> 16u) & 0xFFFFu;

  // 3. Pull 8-byte clip rect (short l, t, r, b) and sign-extend branchlessly
  uint2 raw_clip = g_vtx_buffer.Load2(prim.clip);
  int clip_l = int(raw_clip.x << 16) >> 16;
  int clip_t = int(raw_clip.x) >> 16;
  int clip_r = int(raw_clip.y << 16) >> 16;
  int clip_b = int(raw_clip.y) >> 16;
  output.clip_rect = float4(float(clip_l), float(clip_t), float(clip_r), float(clip_b));

  // 4. Branchless geometry generation:
  // Decode corner bits: bit 0 controls X (left/right), bit 1 controls Y (top/bottom)
  float use_p1_x = float(corner_idx & 1u);
  float use_p1_y = float((corner_idx >> 1u) & 1u);

  // [Type A: Quad (Box, Circle, Icon, Image)]
  float2 quad_pos   = float2(lerp(prim.p0.x, prim.p1.x, use_p1_x),
                 lerp(prim.p0.y, prim.p1.y, use_p1_y));
  float2 quad_uv    = float2(lerp(prim.uv0.x, prim.uv1.x, use_p1_x),
                 lerp(prim.uv0.y, prim.uv1.y, use_p1_y));
  float2 quad_local = float2(lerp(-1.0f, 1.0f, use_p1_x),
                 lerp(-1.0f, 1.0f, use_p1_y));

  // [Type B: Line]
  // Compute line direction and perpendicular normal vector branchlessly
  float2 line_delta = prim.p1 - prim.p0;
  float  line_len_sq = dot(line_delta, line_delta);
  float  inv_len     = rsqrt(max(line_len_sq, 1e-6f));
  float2 line_dir    = line_delta * inv_len;
  float2 line_perp   = float2(-line_dir.y, line_dir.x) * (prim.thickness * 0.5f);

  float2 line_base = lerp(prim.p0, prim.p1, use_p1_y);
  float  perp_sign = lerp(-1.0f, 1.0f, use_p1_x);
  float2 line_pos  = line_base + (line_perp * perp_sign);

  // [Type C: Triangle]
  float is_c0 = float(corner_idx == 0u);
  float is_c1 = float(corner_idx == 1u);
  float is_c2 = float(corner_idx == 2u);
  float2 tri_pos = (prim.p0 * is_c0) + (prim.p1 * is_c1) + (prim.p2 * is_c2);

  // 5. Select final position branchlessly based on primitive flags
  float is_ln   = float((flags & GFX_FLAG_LN) != 0u);
  float is_tri  = float((flags & GFX_FLAG_TRI) != 0u);
  float is_quad = 1.0f - max(is_ln, is_tri);
  float2 final_pos = (quad_pos * is_quad) + (line_pos * is_ln) + (tri_pos * is_tri);

  // 6. Viewport transform to Vulkan NDC [-1, 1]
  // Vulkan NDC: Top-Left (-1, -1) to Bottom-Right (1, 1) matching screen coordinates
  float2 ndc_pos = (final_pos / float2(g_uniform.viewport)) * 2.0f - 1.0f;
  output.pos = float4(ndc_pos, 0.0f, 1.0f);

  // 7. Unpack RGBA8 vertex color branchlessly
  uint4 raw_col = (prim.col >> uint4(0u, 8u, 16u, 24u)) & 0xFFu;
  output.col = float4(raw_col) / 255.0f;

  output.uv          = quad_uv;
  output.local_pos   = quad_local;
  output.tex_id      = tex_slot;
  output.flags       = flags;
  output.circle_mask = prim.circle_mask;
  return output;
}
float4 ren_frag(VSOutput input) : SV_Target {
  // 1. Branchless hardware scissor / clip evaluation
  // SV_Position provides window pixel coordinates (x: [0..w], y: [0..h])
  float2 in_clip_min = step(input.clip_rect.xy, input.pos.xy);
  float2 in_clip_max = step(input.pos.xy, input.clip_rect.zw);
  float clip_factor  = in_clip_min.x * in_clip_min.y * in_clip_max.x * in_clip_max.y;
  clip(clip_factor - 0.5f); // Discards fragment if out of bounds; no 'if' branch

  // 2. Texture fetch from bindless descriptor table
  float4 tex_sample = g_textures[NonUniformResourceIndex(input.tex_id)].Sample(g_sampler, input.uv);

  // 3. Branchless texturing / icon mask blending
  float is_tex  = float((input.flags & GFX_FLAG_TEX)  != 0u);
  float is_mask = float((input.flags & GFX_FLAG_MASK) != 0u);

  // If mask: use R channel as alpha coverage (e.g. font glyphs / icons)
  float4 mask_tex     = float4(1.0f, 1.0f, 1.0f, tex_sample.r);
  float4 textured_col = lerp(tex_sample, mask_tex, is_mask);
  float4 surface_col  = lerp(float4(1.0f, 1.0f, 1.0f, 1.0f), textured_col, is_tex);
  float4 out_col = input.col * surface_col;

  // 4. Branchless analytic circle anti-aliasing (SDF)
  float  dist         = length(input.local_pos);
  float  delta        = fwidth(dist);
  float  circle_alpha = 1.0f - smoothstep(1.0f - delta, 1.0f, dist);
  float  alpha_factor = lerp(1.0f, circle_alpha, input.circle_mask);
  out_col.a *= alpha_factor;
  return out_col;
}
