enum gfx_prim_type {
  GFX_PRIM_BOX,
  GFX_PRIM_LN,
  GFX_PRIM_CIR,
  GFX_PRIM_TRI,
  GFX_PRIM_IMG,
  GFX_PRIM_CNT
};
struct gfx_clip {
  short l,t,r,b;
};
struct gfx_box {
  short l,t,r,b;
  unsigned col;
  unsigned clip;
};
struct gfx_ln {
  short x0,y0,x1,y1;
  unsigned col;
  unsigned clip;
  float thickness;
};
struct gfx_cir {
  short x0,y0,x1,y1;
  unsigned col;
  unsigned clip;
};
struct gfx_tri {
  short x0,y0;
  short x1,y1;
  short x2,y2;
  unsigned col;
  unsigned clip;
};
struct gfx_img {
  short l,t,r,b;
  unsigned col;
  unsigned clip;
  unsigned short u,v;
};
struct gfx_uniform {
  vector_uint2 viewport;
  vector_uint2 tex_siz;
};

