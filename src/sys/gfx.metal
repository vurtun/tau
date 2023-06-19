#include <metal_stdlib>
#include "gfx_com.h"

using namespace metal;

struct vtx_out {
  float4 pos [[position]];
  float4 col;
  float4 crl;
  float4 clp;
  float2 tex_coord;
};
vertex vtx_out
ren_vtx(uint vid                    [[vertex_id]],
        const device uint *idxs     [[buffer(0)]],
        const device char *prims    [[buffer(1)]],
        constant gfx_uniform *uni   [[buffer(2)]]) {

  const uint idx = idxs[vid];
  const uint off = idx & 0x00ffffff;
  const uint corner = (idx >> 24) & 0x03;
  const uint prim = (idx >> 26);

  vtx_out vo;
  if (prim == GFX_PRIM_BOX) {
    device const gfx_box *r = (device const gfx_box*)(prims + off);
    device const gfx_clip *c = (device const gfx_clip*)(prims + r->clip);

    const float2 ba = float2(float(r->l), float(r->t));
    const float2 bb = float2(float(r->r), float(r->b));
    const float2 ca = float2(float(c->l), float(c->t));
    const float2 cb = float2(float(c->r), float(c->b));
    const float2 a = max(ba, ca);
    const float2 b = min(bb, cb);

    vo.col = unpack_unorm4x8_to_float(r->col);
    vo.pos.x = mix(a.x, b.x, float(corner & 0x01));
    vo.pos.y = mix(a.y, b.y, float(corner >> 1));

  } else if (prim == GFX_PRIM_LN) {
    device const gfx_ln *ln = (device const gfx_ln*)(prims + off);
    device const gfx_clip *c = (device const gfx_clip*)(prims + ln->clip);

    const float2 la = float2(float(ln->x0), float(ln->y0));
    const float2 lb = float2(float(ln->x1), float(ln->y1));
    const float2 ca = float2(float(c->l), float(c->t));
    const float2 cb = float2(float(c->r), float(c->b));

    const float2 a = min(cb, max(la, ca));
    const float2 b = min(cb, max(lb, ca));
    const float2 d = normalize(b - a);
    const float2 n = float2(-d.y, d.x);

    vo.col = unpack_unorm4x8_to_float(ln->col);
    vo.pos.xy = mix(a, b, float(corner & 0x01));
    vo.pos.xy += n * (0.5f * float(ln->thickness * (1 - 2 * int(((3-corner) >> 1)))));

  } else if (prim == GFX_PRIM_CIR) {
    device const gfx_cir *r = (device const gfx_cir*)(prims + off);
    device const gfx_clip *c = (device const gfx_clip*)(prims + r->clip);

    const float2 a = float2(float(r->x0), float(r->y0));
    const float2 b = float2(float(r->x1), float(r->y1));

    vo.col = unpack_unorm4x8_to_float(r->col);
    vo.pos.x = mix(a.x, b.x, float(corner & 0x01));
    vo.pos.y = mix(a.y, b.y, float(corner >> 1));

    const float2 d = (b - a) * 0.5f;
    vo.crl.xy = a + d;
    vo.crl.z = (b.x - a.x) * 0.5f;
    vo.clp.xy = float2(float(c->l), float(c->t));
    vo.clp.zw = float2(float(c->r), float(c->b));

  } else if (prim == GFX_PRIM_TRI) {
    device const gfx_tri *t = (device const gfx_tri*)(prims + off);
    device const gfx_clip *p = (device const gfx_clip*)(prims + t->clip);

    const float2 a = float2(float(t->x0), float(t->y0));
    const float2 b = float2(float(t->x1), float(t->y1));
    const float2 c = float2(float(t->x2), float(t->y2));

    vo.col = unpack_unorm4x8_to_float(t->col);
    vo.pos.xy = corner == 1 ? a : corner == 2 ? b : c;
    vo.clp.xy = float2(float(p->l), float(p->t));
    vo.clp.zw = float2(float(p->r), float(p->b));

  } else if (prim == GFX_PRIM_IMG) {
    device const gfx_img *r = (device const gfx_img*)(prims + off);
    device const gfx_clip *c = (device const gfx_clip*)(prims + r->clip);

    const float2 a = float2(float(r->l), float(r->t));
    const float2 b = float2(float(r->r), float(r->b));

    vo.col = unpack_unorm4x8_to_float(r->col);
    vo.pos.x = mix(a.x, b.x, float(corner & 0x01));
    vo.pos.y = mix(a.y, b.y, float(corner >> 1));

    const float2 uva = float2(float(r->u), float(r->v));
    const float2 uvb = float2(float(r->u + r->r - r->l), float(r->v + r->b - r->t));

    const float u = mix(uva.x, uvb.x, float(corner & 0x01));
    const float v = mix(uva.y, uvb.y, float(corner >> 1));

    float2 tex_siz = vector_float2(uni->tex_siz);
    vo.tex_coord = float2(u/tex_siz.x, v/tex_siz.y);

    vo.clp.xy = float2(float(c->l), float(c->t));
    vo.clp.zw = float2(float(c->r), float(c->b));
  }
  vector_float2 view = vector_float2(uni->viewport);
  vo.pos.x = vo.pos.x / view.x * 2.0f - 1.0f;
  vo.pos.y = -1.0f * (vo.pos.y / view.y * 2.0f - 1.0f);
  vo.pos.zw = float2(0.0, 1.0);
  vo.crl.w = float(prim);
  return vo;
}
fragment float4
ren_frag(vtx_out vi             [[stage_in]],
         texture2d<float> tex   [[texture(0)]]) {
  constexpr sampler smp(coord::normalized, min_filter::linear, mag_filter::linear, mip_filter::linear);
  const uint prim = uint(vi.crl.w);
  if (prim == GFX_PRIM_CIR) {
    if (vi.pos.x < vi.clp.x || vi.pos.y < vi.clp.y)
      discard_fragment();
    if (vi.pos.x > vi.clp.z || vi.pos.y > vi.clp.w)
      discard_fragment();
    if (distance(vi.pos.xy, vi.crl.xy) > vi.crl.z)
      discard_fragment();

  } else if (prim == GFX_PRIM_TRI) {
    if (vi.pos.x < vi.clp.x || vi.pos.y < vi.clp.y)
      discard_fragment();
    if (vi.pos.x > vi.clp.z || vi.pos.y > vi.clp.w)
      discard_fragment();

  } else if (prim ==  GFX_PRIM_IMG) {
    if (vi.pos.x < vi.clp.x || vi.pos.y < vi.clp.y)
      discard_fragment();
    if (vi.pos.x > vi.clp.z || vi.pos.y > vi.clp.w)
      discard_fragment();
    float r = tex.sample(smp, vi.tex_coord).r;
    vi.col.a = r;
  }
  return vi.col;
}

