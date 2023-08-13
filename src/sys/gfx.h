#define GFX_VERSION 1

#define GFX_TEX_MAX (1*1024)

struct sys;
struct gfx_buf2d {
  void *intern;
  unsigned char *vtx;
  int vbytes;
  unsigned *idx;
  int icnt;
};
struct gfx_buf2d_cost {
  int vbytes;
  int icnt;
};
struct gfx_buf2d_cost_info {
  struct gfx_buf2d_cost clip;
  struct gfx_buf2d_cost box;
  struct gfx_buf2d_cost line;
  struct gfx_buf2d_cost circle;
  struct gfx_buf2d_cost tri;
  struct gfx_buf2d_cost ico;
  struct gfx_buf2d_cost img;
};
struct gfx_d2d_api {
  int tex;
  struct gfx_buf2d_cost_info cost;
  unsigned(*clip)(struct gfx_buf2d*, int x0, int y0, int x1, int y1);
  void(*box)(struct gfx_buf2d*, int x0, int y0, int x1, int y1, unsigned col, unsigned clip);
  void(*ln)(struct gfx_buf2d*, int x0, int y0, int x1, int y1, int thickness, unsigned col, unsigned clip);
  void(*circle)(struct gfx_buf2d *buf, int x, int y, int r, unsigned col, unsigned clp);
  void(*tri)(struct gfx_buf2d *buf, int x0, int y0, int x1, int y1, int x2, int y2, unsigned col, unsigned clp);
  void(*ico)(struct gfx_buf2d *buf, int x0, int y0, int x1, int y1, int u, int v, unsigned col, unsigned clp);
  void(*img)(struct gfx_buf2d *buf, int tex, int dx, int dy, int dw, int dh, int sx, int sy, int sw, int sh, unsigned clp);
};
enum gfx_pix_fmt_type {
  GFX_PIX_FMT_R8,
  GFX_PIX_FMT_R8G8B8A8,
  GFX_PIX_FMT_TYPE_CNT,
};
struct gfx_tex_api {
  int(*load)(struct sys *s, enum gfx_pix_fmt_type type, void *data, int w, int h);
  void(*info)(int *siz, struct sys *s, int id);
  void(*del)(struct sys *s, int tex);
};
struct gfx_api {
  int version;

  unsigned clear_color;
  int(*init)(struct sys *s, void *view);
  void(*begin)(struct sys *s, int w, int h);
  void(*end)(struct sys *s, void *view);
  void(*shutdown)(struct sys *s);
  void(*resize)(struct sys *s, int w, int h);

  struct gfx_buf2d buf2d;
  struct gfx_d2d_api d2d;
  struct gfx_tex_api tex;
};

