#ifdef DEBUG_MODE
#include <assert.h>
#include <stddef.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cpu.h"
#include "../lib/fmt.h"
#include "../lib/std.h"
#include "dbg.h"
#include "ren.h"
#include "sys.h"
#include "../lib/fmt.c"
#include "../lib/std.c"
#endif

#define REN_CEL_X 80
#define REN_CEL_Y 45
#define REN_CEL_SIZ 96

#define REN_OP_TBL(REN_OP) \
  REN_OP(NOP,   0) \
  REN_OP(NXT,   2) \
  REN_OP(EOL,   1) \
  REN_OP(COL,   2) \
  REN_OP(STYL,  2) \
  REN_OP(CLIP,  3) \
  REN_OP(BOX,   3) \
  REN_OP(SBOX,  3) \
  REN_OP(IMG,   4)

enum ren_op_code {
#define REN_OPS(a, b) REN_OP_##a,
  REN_OP_TBL(REN_OPS)
#undef REN_OPS
  REN_OP_CNT
};
union ren_op {
  struct ren_op_hdr {
    int op;
    unsigned hash;
  } hdr;
  struct ren_op_pos {
    int x, y;
  } pos;
  struct ren_op_siz {
    int w, h;
  } siz;
  struct ren_op_img {
    int id;
    unsigned short sx, sy;
  } img;
  long long word;
  unsigned char buf[sizeof(long long)];
  void *p;
};
compiler_assert(sizeof(union ren_op) == sizeof(unsigned long long));
struct ren_op_buf {
  #define REN_OP_BUF_SIZE (1024)
  union ren_op ops[REN_OP_BUF_SIZE];
};
#define ren_clip(x, y, w, h) (struct ren_clip) { x, y, (x) + (w), (y) + (h) }
struct ren_clip {
  int min_x, min_y, max_x, max_y;
};
struct ren_state {
  unsigned col;
  int thick;
  struct ren_clip clip;
};
struct ren_cmd_buf {
  struct ren_state state;
  struct arena *mem;
  struct sys *sys;

  int cur_op_idx;
  struct ren_op_buf *ops;
  struct ren_op_buf *cur_op_buf;
  unsigned hash;
};
#define REN_MAX_CMD_BUF_CNT 4
struct ren_cmd_que {
  struct arena *mem;
  struct sys *sys;
  int cnt;
  struct ren_cmd_buf bufs[REN_MAX_CMD_BUF_CNT];
};
struct ren_tex {
  int act;
  const unsigned *mem;
  int w, h;
};
struct ren {
  struct scope scp;
  struct arena mem;
  struct ren_cmd_que que;

  int tex_cnt;
  struct ren_tex tex[REN_TEX_MAX];

  unsigned hash;
  unsigned prev_hash;

  struct ren_state state;
  int cel_act;
  unsigned cel[2][REN_CEL_X * REN_CEL_Y];
};
extern void dlInit(struct sys *sys);
extern void dlBegin(struct sys *sys);
extern void dlEnd(struct sys *sys);
extern void dlShutdown(struct sys *sys);

/* ---------------------------------------------------------------------------
 *                                  Renderer
 * ---------------------------------------------------------------------------
 */
static inline unsigned
ren__blend(unsigned dst, unsigned src) {
  struct color s = col_get(src);
  struct color d = col_get(dst);
  unsigned ia = 0xFF - s.a;
  unsigned r = ((s.r * s.a) + (d.r * ia)) >> 8;
  unsigned g = ((s.g * s.a) + (d.g * ia)) >> 8;
  unsigned b = ((s.b * s.a) + (d.b * ia)) >> 8;
  return col_rgb(r, g, b);
}
static void
ren__intersect(struct ren_clip *res, const struct sys_rect *a,
               const struct ren_clip *b) {
  res->min_x = max(a->x, max(0, b->min_x));
  res->min_y = max(a->y, max(0, b->min_y));
  res->max_x = min(a->x + a->w, b->max_x);
  res->max_y = min(a->y + a->h, b->max_y);
}
static int
ren__area(const struct ren_clip *r) {
  int w = r->max_x - r->min_x;
  int h = r->max_y - r->min_y;
  return w * h;
}
static void
ren__rect_slow(struct sys *sys, struct sys_ren_target *tar,
               const struct sys_rect *rect, const struct ren_clip *clip,
               unsigned col) {
  if (col_a(col) == 0 ) return;
  struct ren_clip fill = {0};
  ren__intersect(&fill, rect, clip);
  if (ren__area(&fill) <= 0) {
    return;
  }
  dbg_blk_begin(sys, "ren:rect_slow");
  unsigned *d = tar->pixels + fill.min_x + fill.min_y * tar->w;
  const int dr = tar->w - (fill.max_x - fill.min_x);
  if (col_a(col) == 0xFF) {
    for (int j = fill.min_y; j < fill.max_y; j++) {
      for (int i = fill.min_x; i < fill.max_x; i++) {
        *d = col;
        d++;
      }
      d += dr;
    }
  } else {
    for (int j = fill.min_y; j < fill.max_y; j++) {
      for (int i = fill.min_x; i < fill.max_x; i++) {
        *d = ren__blend(*d, col);
        d++;
      }
      d += dr;
    }
  }
  dbg_blk_end(sys);
}
static void
ren__rect(struct sys *sys, struct sys_ren_target *tar,
          const struct sys_rect *rect, const struct ren_clip *clip,
          unsigned col) {
  if (rect->w < 4) {
    ren__rect_slow(sys, tar, rect, clip, col);
    return;
  }
#ifdef CPU_SIMD_128
  if (col_a(col) == 0) return;
  struct ren_clip fill = {0};
  ren__intersect(&fill, rect, clip);
  if (ren__area(&fill) <= 0) {
    return;
  }
  /* setup clipping masks for start and end of row */
  dbg_blk_begin(sys, "ren:rect");
  static const unsigned char x1[] = {
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  };
  static const unsigned char x2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  };
  static const unsigned char x3[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
  };
  static const unsigned char y1[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y2[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y3[] = {
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  int4 clip_msk_start = int4_char(-1);
  int4 clip_msk_end = int4_char(-1);
  const int4 all_clip_start_msks[] = {
    int4_char(-1), int4_ld(x1), int4_ld(x2), int4_ld(x3),
  };
  const int4 all_clip_end_msks[] = {
    int4_char(-1), int4_ld(y3), int4_ld(y2), int4_ld(y1),
  };
  if (fill.min_x & 3){
    clip_msk_start = all_clip_start_msks[fill.min_x & 3];
    fill.min_x = fill.min_x & ~3;
  }
  if (fill.max_x & 3){
    clip_msk_end = all_clip_end_msks[fill.max_x & 3];
    fill.max_x = (fill.max_x & ~3) + 4;
  }
  if ((fill.max_x - fill.min_x) == 4) {
    clip_msk_start = int4_and(clip_msk_start, clip_msk_end);
  }
  /* setup constants */
  int4 msk_ff = int4_int(0xff);
  int4 cola = int4_int(col_a(col));
  int4 col_ia = int4_sub(msk_ff, cola);

  int4 colr = int4_int(col_r(col));
  int4 colg = int4_int(col_g(col));
  int4 colb = int4_int(col_b(col));

  int4 col_ir = int4_mul(colr, cola);
  int4 col_ig = int4_mul(colg, cola);
  int4 col_ib = int4_mul(colb, cola);

  for (int y = fill.min_y; y < fill.max_y; ++y) {
    int4 clip_msk = clip_msk_start;
    unsigned *pix = tar->pixels + fill.min_x + y * tar->w;
    for (int x = fill.min_x; x < fill.max_x; x += 4) {
      /* load destination color */
      int4 d = int4_ld(pix);
      int4 dr = int4_and(int4_srl(d, COL_R), msk_ff);
      int4 dg = int4_and(int4_srl(d, COL_G), msk_ff);
      int4 db = int4_and(int4_srl(d, COL_B), msk_ff);

      /* blend multiply by inverse color alpha */
      int4 tr = int4_mul(dr, col_ia);
      int4 tg = int4_mul(dg, col_ia);
      int4 tb = int4_mul(db, col_ia);

      /* add source and destination color */
      int4 br = int4_add(col_ir, tr);
      int4 bg = int4_add(col_ig, tg);
      int4 bb = int4_add(col_ib, tb);

      /* repack rgb values */
      int4 sr = int4_sll(int4_srl(br, 8), COL_R);
      int4 sg = int4_sll(int4_srl(bg, 8), COL_G);
      int4 sb = int4_sll(int4_srl(bb, 8), COL_B);
      int4 rgb = int4_or(int4_or(sr, sg), sb);

      /* apply clip mask */
      int4 dst_msk = int4_and(clip_msk, rgb);
      int4 src_msk = int4_andnot(clip_msk, d);
      int4 out = int4_or(dst_msk, src_msk);
      int4_str(pix, out);

      if ((x + 8) < fill.max_x) {
        clip_msk = int4_char(-1);
      } else clip_msk = clip_msk_end;
      pix += 4;
    }
  }
  dbg_blk_end(sys);
#else
  ren__rect_slow(sys, tar, rect, clip, col);
#endif
}
static void
ren__rect_fast(struct sys *sys, struct sys_ren_target *tar,
               const struct sys_rect *rect, const struct ren_clip *clip,
              unsigned col) {
  if (rect->w < 8) {
    ren__rect(sys, tar, rect, clip, col);
    return;
  }
#ifdef CPU_SIMD_256
  if (col_a(col) == 0) return;
  struct ren_clip fill = {0};
  ren__intersect(&fill, rect, clip);
  if (ren__area(&fill) <= 0) {
    return;
  }
  dbg_blk_begin(sys, "ren:rect_fast");
  static const unsigned char x1[] = {
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  };
  static const unsigned char x2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  };
  static const unsigned char x3[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  };
  static const unsigned char x4[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  };
  static const unsigned char x5[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  };
  static const unsigned char x6[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  };
  static const unsigned char x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
  };
  static const unsigned char y1[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y2[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y3[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y4[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y5[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y6[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y7[] = {
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  /* setup row start clipping mask */
  int8 clip_msk_start = int8_char(-1);
  int8 clip_msk_end = int8_char(-1);
  const int8 all_clip_start_msks[] = {
    int8_char(-1), int8_ld(x1), int8_ld(x2),
    int8_ld(x3), int8_ld(x4), int8_ld(x5),
    int8_ld(x6), int8_ld(x7),
  };
  const int8 all_clip_end_msks[] = {
    int8_char(-1), int8_ld(y7), int8_ld(y6),
    int8_ld(y5), int8_ld(y4), int8_ld(y3),
    int8_ld(y2), int8_ld(y1),
  };
  if (fill.min_x & 7){
    clip_msk_start = all_clip_start_msks[fill.min_x & 7];
    fill.min_x = fill.min_x & ~7;
  }
  if (fill.max_x & 7){
    clip_msk_end = all_clip_end_msks[fill.max_x & 7];
    fill.max_x = (fill.max_x & ~7) + 8;
  }
  if ((fill.max_x - fill.min_x) == 8) {
    clip_msk_start = int8_and(clip_msk_start, clip_msk_end);
  }
  /* setup constants */
  int8 msk_ff = int8_int(0xff);
  int8 cola = int8_int(col_a(col));
  int8 col_ia = int8_sub(msk_ff, cola);

  int8 colr = int8_int(col_r(col));
  int8 colg = int8_int(col_g(col));
  int8 colb = int8_int(col_b(col));

  int8 col_ir = int8_mul(colr, cola);
  int8 col_ig = int8_mul(colg, cola);
  int8 col_ib = int8_mul(colb, cola);

  for (int y = fill.min_y; y < fill.max_y; ++y) {
    int8 clip_msk;
    clip_msk = clip_msk_start;
    unsigned *pix = tar->pixels + fill.min_x + y * tar->w;
    for (int x = fill.min_x; x < fill.max_x; x += 8) {
      /* load destination color */
      int8 d = int8_ld(pix);
      int8 dr = int8_and(int8_srl(d, COL_R), msk_ff);
      int8 dg = int8_and(int8_srl(d, COL_G), msk_ff);
      int8 db = int8_and(int8_srl(d, COL_B), msk_ff);

      /* blend multiply by inverse color alpha */
      int8 tr = int8_mul(dr, col_ia);
      int8 tg = int8_mul(dg, col_ia);
      int8 tb = int8_mul(db, col_ia);

      /* add source and destination color */
      int8 br = int8_add(col_ir, tr);
      int8 bg = int8_add(col_ig, tg);
      int8 bb = int8_add(col_ib, tb);

      /* repack rgb values */
      int8 sr = int8_sll(int8_srl(br, 8), COL_R);
      int8 sg = int8_sll(int8_srl(bg, 8), COL_G);
      int8 sb = int8_sll(int8_srl(bb, 8), COL_B);
      int8 rgb = int8_or(int8_or(sr, sg), sb);

      /* apply clip mask */
      int8 dst_msk = int8_and(clip_msk, rgb);
      int8 src_msk = int8_andnot(clip_msk, d);
      int8 out = int8_or(dst_msk, src_msk);
      int8_store(pix, out);

      if ((x + 16) < fill.max_x) {
        clip_msk = int8_char(-1);
      } else clip_msk = clip_msk_end;
      pix += 8;
    }
  }
  dbg_blk_end(sys);
#else
  ren__rect(sys, tar, rect, clip, col);
#endif
}
static void
ren__srect(struct sys *sys, struct sys_ren_target *tar, const struct sys_rect *rect,
           const struct ren_clip *clip, unsigned col, int thick) {
  unused(thick);
  struct sys_rect a = sys_rect(rect->x, rect->y, rect->w + 1, thick);
  struct sys_rect b = sys_rect(rect->x + rect->w, rect->y, thick, rect->h + 1);
  struct sys_rect c = sys_rect(rect->x, rect->y + rect->h, rect->w + 1, thick);
  struct sys_rect d = sys_rect(rect->x, rect->y, thick, rect->h + 1);

  ren__rect(sys, tar, &a, clip, col);
  ren__rect_slow(sys, tar, &b, clip, col);
  ren__rect(sys, tar, &c, clip, col);
  ren__rect_slow(sys, tar, &d, clip, col);
}
static inline unsigned
ren__blend2(unsigned dst, unsigned src, unsigned col) {
  struct color s = col_get(src);
  struct color d = col_get(dst);
  struct color c = col_get(col);

  s.a = ((s.a * c.a) >> 8u);
  unsigned ia = 0xff - s.a;
  unsigned r = ((s.r * c.r * s.a) >> 16u) + ((d.r * ia) >> 8u);
  unsigned g = ((s.g * c.g * s.a) >> 16u) + ((d.g * ia) >> 8u);
  unsigned b = ((s.b * c.b * s.a) >> 16u) + ((d.b * ia) >> 8u);
  return col_rgb(r, g, b);
}
static void
ren__blit_img_slow(struct sys *sys, struct sys_ren_target *tar,
                   const unsigned *img, int x, int y,
                   const struct sys_rect *sub_img, int stride,
                   const struct ren_clip *clip, unsigned col) {
  /* clip */
  if (col_a(col) == 0) return;
  int n = clip->min_x - x;
  struct sys_rect sub = *sub_img;
  if (n > 0) {sub.w -= n; sub.x += n; x += n;}
  if ((n = clip->min_y - y) > 0) {sub.h -= n;sub.y += n;y += n;}
  if ((n = x + sub.w - clip->max_x) > 0) sub.w -= n;
  if ((n = y + sub.h - clip->max_y) > 0) sub.h -= n;
  if (sub.w <= 0 || sub.h <= 0) return;

  /* draw */
  dbg_blk_begin(sys, "ren:blit_img_slow");
  const unsigned *s = img + sub.x + sub.y * stride;
  unsigned *d = tar->pixels + x + y * tar->w;
  int sr = stride - sub.w;
  int dr = tar->w - sub.w;
  for (int j = 0; j < sub.h; j++) {
    for (int i = 0; i < sub.w; i++) {
      *d = ren__blend2(*d, *s, col);
      d++, s++;
    }
    d += dr, s += sr;
  }
  dbg_blk_end(sys);
}
static void
ren__blit_img(struct sys *sys, struct sys_ren_target *tar,
              const unsigned *src_img, int x, int y,
              const struct sys_rect *sub_img, int stride,
              const struct ren_clip *clip, unsigned col) {
#ifdef CPU_SIMD_128
  /* clip */
  if (col_a(col) == 0) return;
  int n = clip->min_x - x;
  struct sys_rect sub = *sub_img;
  if (n > 0) {sub.w -= n; sub.x += n; x += n;}
  if ((n = clip->min_y - y) > 0) {sub.h -= n;sub.y += n;y += n;}
  if ((n = x + sub.w - clip->max_x) > 0) sub.w -= n;
  if ((n = y + sub.h - clip->max_y) > 0) sub.h -= n;
  if (sub.w <= 0 || sub.h <= 0) return;

  /* setup clipping masks for start and end of row */
  dbg_blk_begin(sys, "ren:blit_img");
  static const unsigned char y1[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y2[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y3[] = {
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  int4 clip_msk_start = int4_char(-1);
  int4 clip_msk_end = int4_char(-1);
  const int4 all_clip_msks[] = {
    int4_char(-1), int4_ld(y3), int4_ld(y2), int4_ld(y1),
  };
  if (sub.w & 3){
    clip_msk_end = all_clip_msks[sub.w & 3];
    sub.w = (sub.w & ~3) + 4;
  }
  if (sub.w == 4) {
    clip_msk_start = clip_msk_end;
  }
  int4 msk_ff = int4_int(0xff);
  int4 cr = int4_int(col_r(col));
  int4 cg = int4_int(col_g(col));
  int4 cb = int4_int(col_b(col));
  int4 ca = int4_int(col_a(col));

  for (int j = 0; j < sub.h; j++) {
    int4 clip_msk = clip_msk_start;
    const unsigned *src_pix = src_img + sub.x + (sub.y + j) * stride;
    unsigned *dst_pix = tar->pixels + x + (y + j) * tar->w;
    for (int i = 0; i < sub.w; i += 4) {
      /* load destination color */
      int4 d = int4_ld(dst_pix);
      int4 dr = int4_and(int4_srl(d, COL_R), msk_ff);
      int4 dg = int4_and(int4_srl(d, COL_G), msk_ff);
      int4 db = int4_and(int4_srl(d, COL_B), msk_ff);

      /* load source color */
      int4 s = int4_ld(src_pix);
      int4 sr = int4_and(int4_srl(s, COL_R), msk_ff);
      int4 sg = int4_and(int4_srl(s, COL_G), msk_ff);
      int4 sb = int4_and(int4_srl(s, COL_B), msk_ff);
      int4 sa = int4_and(int4_srl(s, COL_A), msk_ff);

      /* calculate combined alpha */
      sa = int4_srl(int4_mul(sa, ca), 8);
      int4 ia = int4_sub(msk_ff, sa);

      /* blend red component */
      int4 rsc = int4_mul(int4_mul(sr, cr), sa);
      int4 rdc = int4_mul(dr, ia);
      int4 r = int4_add(int4_srl(rsc, 16), int4_srl(rdc, 8));

      /* blend green component */
      int4 gsc = int4_mul(int4_mul(sg, cg), sa);
      int4 gdc = int4_mul(dg, ia);
      int4 g = int4_add(int4_srl(gsc, 16), int4_srl(gdc, 8));

      /* blend blue component */
      int4 bsc = int4_mul(int4_mul(sb, cb), sa);
      int4 bdc = int4_mul(db, ia);
      int4 b = int4_add(int4_srl(bsc, 16), int4_srl(bdc, 8));

      /* repack rgb values */
      int4 pr = int4_sll(r, COL_R);
      int4 pg = int4_sll(g, COL_G);
      int4 pb = int4_sll(b, COL_B);
      int4 rgb = int4_or(int4_or(pr, pg), pb);

      /* apply clip mask */
      int4 dst_msk = int4_and(clip_msk, rgb);
      int4 src_msk = int4_andnot(clip_msk, d);
      int4 out = int4_or(dst_msk, src_msk);
      int4_str(dst_pix, out);

      if ((i + 8) < sub.w) {
        clip_msk = int4_char(-1);
      } else clip_msk = clip_msk_end;

      src_pix += 4;
      dst_pix += 4;
    }
  }
  dbg_blk_end(sys);
#else
  ren__blit_img_slow(sys, tar, src_img, x, y, sub_img, stride, clip, col);
#endif
}
static void
ren__blit_img_fast(struct sys *sys, struct sys_ren_target *tar,
                   const unsigned *src_img, int x, int y,
                   const struct sys_rect *sub_img, int stride,
                   const struct ren_clip *clip, unsigned col) {
#ifdef CPU_SIMD_256
  /* clip */
  if (col_a(col) == 0) return;
  int n = clip->min_x - x;
  struct sys_rect sub = *sub_img;
  if (n > 0) {sub.w -= n; sub.x += n; x += n;}
  if ((n = clip->min_y - y) > 0) {sub.h -= n;sub.y += n;y += n;}
  if ((n = x + sub.w - clip->max_x) > 0) sub.w -= n;
  if ((n = y + sub.h - clip->max_y) > 0) sub.h -= n;
  if (sub.w <= 0 || sub.h <= 0) return;

  /* setup clipping masks */
  dbg_blk_begin(sys, "ren:blit_img_fast");
  static const unsigned char y1[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y2[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y3[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y4[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y5[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y6[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  static const unsigned char y7[] = {
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  int8 clip_msk_start = int8_char(-1);
  int8 clip_msk_end = int8_char(-1);
  const int8 all_clip_msks[] = {
    int8_char(-1), int8_ld(y7), int8_ld(y6),
    int8_ld(y5), int8_ld(y4), int8_ld(y3),
    int8_ld(y2), int8_ld(y1),
  };
  if (sub.w & 7){
    clip_msk_end = all_clip_msks[sub.w & 7];
    sub.w = (sub.w & ~7) + 8;
  }
  if (sub.w == 8) {
    clip_msk_start = clip_msk_end;
  }
  int8 msk_ff = int8_int(0xff);
  int8 cr = int8_int(col_r(col));
  int8 cg = int8_int(col_g(col));
  int8 cb = int8_int(col_b(col));
  int8 ca = int8_int(col_a(col));

  for (int j = 0; j < sub.h; j++) {
    int8 clip_msk = clip_msk_start;
    const unsigned *src_pix = src_img + sub.x + (sub.y + j) * stride;
    unsigned *dst_pix = tar->pixels + x + (y + j) * tar->w;
    for (int i = 0; i < sub.w; i += 8) {
      /* load destination color */
      int8 d = int8_ld(dst_pix);
      int8 dr = int8_and(int8_srl(d, COL_R), msk_ff);
      int8 dg = int8_and(int8_srl(d, COL_G), msk_ff);
      int8 db = int8_and(int8_srl(d, COL_B), msk_ff);

      /* load source color */
      int8 s = int8_ld(src_pix);
      int8 sr = int8_and(int8_srl(s, COL_R), msk_ff);
      int8 sg = int8_and(int8_srl(s, COL_G), msk_ff);
      int8 sb = int8_and(int8_srl(s, COL_B), msk_ff);
      int8 sa = int8_and(int8_srl(s, COL_A), msk_ff);

      /* calculate combined alpha */
      sa = int8_srl(int8_mul(sa, ca), 8);
      int8 ia = int8_sub(msk_ff, sa);

      /* blend red component */
      int8 rsc = int8_mul(int8_mul(sr, cr), sa);
      int8 rdc = int8_mul(dr, ia);
      int8 r = int8_add(int8_srl(rsc, 16), int8_srl(rdc, 8));

      /* blend green component */
      int8 gsc = int8_mul(int8_mul(sg, cg), sa);
      int8 gdc = int8_mul(dg, ia);
      int8 g = int8_add(int8_srl(gsc, 16), int8_srl(gdc, 8));

      /* blend blue component */
      int8 bsc = int8_mul(int8_mul(sb, cb), sa);
      int8 bdc = int8_mul(db, ia);
      int8 b = int8_add(int8_srl(bsc, 16), int8_srl(bdc, 8));

      /* repack rgb values */
      int8 pr = int8_sll(r, COL_R);
      int8 pg = int8_sll(g, COL_G);
      int8 pb = int8_sll(b, COL_B);
      int8 rgb = int8_or(int8_or(pr, pg), pb);

      /* apply clip mask */
      int8 dst_msk = int8_and(clip_msk, rgb);
      int8 src_msk = int8_andnot(clip_msk, d);
      int8 out = int8_or(dst_msk, src_msk);
      int8_store(dst_pix, out);

      if ((i + 16) < sub.w) {
        clip_msk = int8_char(-1);
      } else clip_msk = clip_msk_end;

      src_pix += 8;
      dst_pix += 8;
    }
  }
  dbg_blk_end(sys);
#else
  ren__blit_img(sys, tar, src_img, x, y, sub_img, stride, clip, col);
#endif
}

/* ---------------------------------------------------------------------------
 *                              Commands
 * ---------------------------------------------------------------------------
 */
static const int ren__op_tbl_size[REN_OP_CNT] = {
#define REN_OPS(a, b) b,
  REN_OP_TBL(REN_OPS)
#undef REN_OPS
};
static const union ren_op *
ren_op_begin(const struct ren_op_buf *ob) {
  const union ren_op *p = ob->ops;
  assert(p[0].hdr.op != REN_OP_NOP);
  if (p[0].hdr.op == REN_OP_EOL) {
    return 0;
  } else {
    return p;
  }
}
static const union ren_op *
ren_op_next(const union ren_op *p) {
  p += ren__op_tbl_size[p->hdr.op];
  if (p[0].hdr.op == REN_OP_NXT) {
    p = (union ren_op *)p[1].p;
  }
  if (p[0].hdr.op == REN_OP_EOL) {
    return 0;
  }
  return p;
}
static union ren_op *
ren__push_op(struct ren_cmd_buf *buf, int cnt) {
  int idx = buf->cur_op_idx;
  struct ren_op_buf *ob = buf->cur_op_buf;
  if (idx + cnt >= REN_OP_BUF_SIZE - 2) {
    struct ren_op_buf *obn = arena_alloc(buf->mem, buf->sys, szof(struct ren_op_buf));
    ob->ops[idx + 0].hdr = (struct ren_op_hdr){REN_OP_NXT, 0};
    ob->ops[idx + 1].p = cast(void*, obn);
    buf->cur_op_buf = obn;
    ob = obn;
    idx = 0;
  }
  memset(ob->ops + idx, 0, cast(size_t, cnt) * sizeof(union ren_op));
  buf->cur_op_idx = idx + cnt;
  return ob->ops + idx;
}
static union ren_op *
ren_push_op(struct ren_cmd_buf *buf, int cnt) {
  if (!buf->cur_op_buf) {
    buf->ops = arena_alloc(buf->mem, buf->sys, szof(struct ren_op_buf));
    buf->hash = FNV1A32_HASH_INITIAL;
    buf->cur_op_buf = buf->ops;
    buf->cur_op_idx = 0;
  }
  return ren__push_op(buf, cnt);
}
static void
ren_reg_op(struct ren_cmd_buf *buf, unsigned h) {
  buf->hash = fnv1au32(buf->hash, h);
}
static void
ren_col(struct ren_cmd_buf *buf, unsigned col) {
  if (buf->state.col != col) {
    buf->state.col = col;
    union ren_op *p = ren_push_op(buf, ren__op_tbl_size[REN_OP_COL]);
    p[0].hdr = (struct ren_op_hdr){REN_OP_COL};
    p[1].word = col;
  }
}
static void
ren_line_style(struct ren_cmd_buf *buf, int thickness) {
  if (buf->state.thick != thickness) {
    union ren_op *p = ren_push_op(buf, ren__op_tbl_size[REN_OP_STYL]);
    p[0].hdr = (struct ren_op_hdr){.op = REN_OP_STYL};
    p[1].word = thickness;
    buf->state.thick = thickness;
  }
}
static void
ren_scissor(struct ren_cmd_buf *buf, int x, int y, int w, int h) {
  union ren_op *p = ren_push_op(buf, ren__op_tbl_size[REN_OP_CLIP]);
  p[0].hdr = (struct ren_op_hdr){.op = REN_OP_CLIP};
  p[1].pos = (struct ren_op_pos){x, y};
  p[2].siz = (struct ren_op_siz){w, h};
  p[0].hdr.hash = fnv1a32(p, 3 * szof(union ren_op), FNV1A32_HASH_INITIAL);
  ren_reg_op(buf, p[0].hdr.hash);
}
static void
ren_box(struct ren_cmd_buf *buf, int x0, int y0, int x1, int y1) {
  union ren_op *p = ren_push_op(buf, ren__op_tbl_size[REN_OP_BOX]);
  p[0].hdr = (struct ren_op_hdr){.op = REN_OP_BOX};
  p[1].pos = (struct ren_op_pos){x0, y0};
  p[2].siz = (struct ren_op_siz){x1 - x0, y1 - y0};
  p[0].hdr.hash = fnv1a32(p, 3 * szof(union ren_op), FNV1A32_HASH_INITIAL);
  p[0].hdr.hash = fnv1a32(&buf->state.col, szof(buf->state.col), p[0].hdr.hash);
  ren_reg_op(buf, p[0].hdr.hash);
}
static void
ren_hline(struct ren_cmd_buf *buf, int y, int x0, int x1) {
  ren_box(buf, x0, y, x1 + 1, y + buf->state.thick);
}
static void
ren_vline(struct ren_cmd_buf *buf, int x, int y0, int y1) {
  ren_box(buf, x, y0, x + buf->state.thick, y1 + 1);
}
static void
ren_srect(struct ren_cmd_buf *buf, int x0, int y0, int x1, int y1) {
  union ren_op *p = ren_push_op(buf, ren__op_tbl_size[REN_OP_SBOX]);
  p[0].hdr = (struct ren_op_hdr){.op = REN_OP_SBOX};
  p[1].pos = (struct ren_op_pos){x0, y0};
  p[2].siz = (struct ren_op_siz){x1 - x0, y1 - y0};
  p[0].hdr.hash = fnv1a32(p, 3 * szof(union ren_op), FNV1A32_HASH_INITIAL);
  p[0].hdr.hash = fnv1a32(&buf->state.col, szof(buf->state.col), p[0].hdr.hash);
  p[0].hdr.hash = fnv1a32(&buf->state.thick, szof(buf->state.thick), p[0].hdr.hash);
  ren_reg_op(buf, p[0].hdr.hash);
}
static void
ren_img(struct ren_cmd_buf *buf, int dx, int dy, int sx, int sy,
        int w, int h, int img_id) {
  union ren_op *p = ren_push_op(buf, ren__op_tbl_size[REN_OP_IMG]);
  p[0].hdr = (struct ren_op_hdr){.op = REN_OP_IMG};
  p[1].pos = (struct ren_op_pos){dx, dy};
  p[2].siz = (struct ren_op_siz){w, h};
  p[3].img = (struct ren_op_img){img_id, (unsigned short)sx, (unsigned short)sy};
  p[0].hdr.hash = fnv1a32(p, 4 * szof(union ren_op), FNV1A32_HASH_INITIAL);
  ren_reg_op(buf, p[0].hdr.hash);
}

/* ---------------------------------------------------------------------------
 *                                  System
 * ---------------------------------------------------------------------------
 */
#define ren__cell_idx(x, y) ((x) + (y)*REN_CEL_X)
static inline int
ren__rects_overlap(const struct sys_rect *a, const struct sys_rect *b) {
  return b->x + b->w >= a->x && b->x <= a->x + a->w && b->y + b->h >= a->y &&
         b->y <= a->y + a->h;
}
static inline void
ren__intersect_rects(struct sys_rect *res, const struct sys_rect *a,
                     const struct sys_rect *b) {
  int x1 = max(a->x, b->x);
  int y1 = max(a->y, b->y);
  int x2 = min(a->x + a->w, b->x + b->w);
  int y2 = min(a->y + a->h, b->y + b->h);
  *res = (struct sys_rect){x1, y1, max(0, x2 - x1), max(0, y2 - y1)};
}
static void
ren__update_overlapping_cells(struct ren *ren, struct sys_rect r,
                              unsigned h) {
  int x1 = r.x / REN_CEL_SIZ;
  int y1 = r.y / REN_CEL_SIZ;
  int x2 = (r.x + r.w) / REN_CEL_SIZ;
  int y2 = (r.y + r.h) / REN_CEL_SIZ;

  for (int y = y1; y <= y2; y++) {
    for (int x = x1; x <= x2; x++) {
      int idx = ren__cell_idx(x, y);
      unsigned s = ren->cel[ren->cel_act][idx];
      ren->cel[ren->cel_act][idx] = fnv1au32(h, s);
    }
  }
}
static inline void
ren__merge_rects(struct sys_rect *res, const struct sys_rect *a,
                 const struct sys_rect *b) {
  int x1 = min(a->x, b->x);
  int y1 = min(a->y, b->y);
  int x2 = max(a->x + a->w, b->x + b->w);
  int y2 = max(a->y + a->h, b->y + b->h);
  *res = (struct sys_rect){x1, y1, x2 - x1, y2 - y1};
}
static void
ren__push_rect(struct sys_ren_target *ren_tar, struct sys *s,
               const struct sys_rect *r) {
  for (int i = dyn_cnt(ren_tar->dirty_rects) - 1; i >= 0; i--) {
    struct sys_rect *rp = ren_tar->dirty_rects + i;
    if (ren__rects_overlap(rp, r)) {
      ren__merge_rects(rp, rp, r);
      return;
    }
  }
  dyn_add(ren_tar->dirty_rects, s, *r);
}
static void
ren__drw_cmd(struct sys *sys, struct ren *ren, struct sys_ren_target *tar,
             struct ren_state *s, const union ren_op *p,
             const struct ren_clip *dirty) {
  switch (p[0].hdr.op) {
  case REN_OP_STYL:
    s->thick = cast(int, p[1].word);
    break;
  case REN_OP_COL:
    s->col = cast(unsigned, p[1].word);
    break;
  case REN_OP_CLIP: {
    struct ren_clip c = ren_clip(p[1].pos.x, p[1].pos.y, p[2].siz.w, p[2].siz.h);
    s->clip.min_x = max(dirty->min_x, c.min_x);
    s->clip.min_y = max(dirty->min_y, c.min_y);
    s->clip.max_x = min(dirty->max_x, c.max_x);
    s->clip.max_y = min(dirty->max_y, c.max_y);
  } break;
  case REN_OP_BOX: {
    struct sys_rect r = sys_rect(p[1].pos.x, p[1].pos.y, p[2].siz.w, p[2].siz.h);
    ren__rect_fast(sys, tar, &r, &s->clip, s->col);
  } break;
  case REN_OP_SBOX: {
    struct sys_rect r = sys_rect(p[1].pos.x, p[1].pos.y, p[2].siz.w, p[2].siz.h);
    ren__srect(sys, tar, &r, &s->clip, s->col, s->thick);
  } break;
  case REN_OP_IMG: {
    int img_id = p[3].img.id;
    assert (img_id < REN_TEX_MAX && img_id >= 0);
    if (img_id >= REN_TEX_MAX || img_id < 0) {
      break;
    }
    assert(ren->tex[img_id].act);
    const struct ren_tex *tex = ren->tex + img_id;
    struct sys_rect r = sys_rect(p[3].img.sx, p[3].img.sy, p[2].siz.w, p[2].siz.h);
    ren__blit_img_fast(sys, tar, tex->mem, p[1].pos.x, p[1].pos.y, &r, tex->w, &s->clip, s->col);
  } break;
  }
}
static struct ren_cmd_buf*
ren_mk_buf(struct ren_cmd_que *q) {
  assert(q);
  assert(q->mem);
  assert(q->cnt < cntof(q->bufs));

  struct ren_cmd_buf *buf = q->bufs + q->cnt++;
  buf->mem = q->mem;
  buf->sys = q->sys;

  buf->hash = FNV1A32_HASH_INITIAL;
  buf->cur_op_buf = 0;
  buf->cur_op_idx = 0;
  buf->state.thick = 1;
  buf->state.col = 0;
  buf->ops = 0;
  return buf;
}
static int
ren_tex_mk(void *ren_hdl, const unsigned *mem, int w, int h) {
  assert(mem);
  assert(ren_hdl);
  struct ren *ren = cast(struct ren*, ren_hdl);
  for (int i = 0; i < REN_TEX_MAX; ++i) {
    if (!ren->tex[i].act) {
      struct ren_tex *tex = ren->tex + i;
      tex->w = w, tex->h = h;
      tex->mem = mem;
      tex->act = 1;
      return i;
    }
  }
  return REN_TEX_MAX;
}
static void
ren_tex_del(void *ren_hdl, int id) {
  assert(ren_hdl);
  assert(id >= 0 && id < REN_TEX_MAX);
  struct ren *ren = cast(struct ren*, ren_hdl);

  assert(ren->tex[id].act);
  struct ren_tex *tex = ren->tex + id;
  memset(tex, 0, sizeof(*tex));
}
static void
ren_tex_siz(int *siz, void *ren_hdl, int id) {
  assert(siz);
  assert(ren_hdl);
  assert(id >= 0 && id < REN_TEX_MAX);
  struct ren *ren = cast(struct ren*, ren_hdl);

  assert(ren->tex[id].act);
  struct ren_tex *tex = ren->tex + id;
  siz[0] = tex->w;
  siz[1] = tex->h;
}
static const struct ren_api ren_api = {
  .version = REN_VERSION,
  .que = {
    .mk = ren_mk_buf,
  },
  .drw = {
    .col = ren_col,
    .line_style = ren_line_style,
    .clip = ren_scissor,
    .box = ren_box,
    .hln = ren_hline,
    .vln = ren_vline,
    .sbox = ren_srect,
    .img = ren_img,
  },
  .tex = {
    .mk = ren_tex_mk,
    .del = ren_tex_del,
    .siz = ren_tex_siz,
  },
};
static void
ren_init(struct sys *sys) {
  struct ren *ren = arena_obj(sys->mem.arena, sys, struct ren);
  for (int i = 0; i < REN_CEL_X * REN_CEL_Y; i++) {
    ren->cel[0][i] = FNV1A32_HASH_INITIAL;
  }
  ren->mem.blksiz = KB(128);
  ren->hash = FNV1A32_HASH_INITIAL;
  ren->que.mem = &ren->mem;
  ren->que.sys = sys;
  sys->renderer = ren;

  sys->ren = ren_api;
  sys->ren.que.dev = &ren->que;
}
static void
ren_begin(struct sys *sys) {
  unused(sys);
}
static void
ren__cleanup(struct ren *ren, struct sys *sys) {
  arena_reset(&ren->mem, sys);
  ren->que.cnt = 0;
}
static void
ren_end(struct sys *sys) {
  dbg_blk_begin(sys, "render");
  struct ren *ren = sys->renderer;

  ren->prev_hash = ren->hash;
  ren->hash = FNV1A32_HASH_INITIAL;
  for (int i = 0; i < ren->que.cnt; ++i) {
    struct ren_cmd_buf *buf = ren->que.bufs + i;
    union ren_op *p = ren__push_op(buf, ren__op_tbl_size[REN_OP_EOL]);
    p[0].hdr = (struct ren_op_hdr){.op = REN_OP_EOL};
    ren->hash = fnv1au32(ren->hash, buf->hash);
  }
  struct sys_ren_target *ren_tar = &sys->ren_target;
  if (ren_tar->resized) {
    memset(ren->cel[!ren->cel_act], 0xff, sizeof(ren->cel[!ren->cel_act]));
    ren->prev_hash = FNV1A32_HASH_INITIAL;
  }
  if (ren->hash == ren->prev_hash) {
    /* coarse grained change detection */
    dbg_blk_end(sys);
    ren__cleanup(ren, sys);
    return;
  }
  dbg_blk_begin(sys, "ren:hash_grid");
  for (int bi = 0; bi < ren->que.cnt; ++bi) {
    struct ren_cmd_buf *buf = ren->que.bufs + bi;
    /* use hash grid for fine grained change detection */
    const union ren_op *p = 0;
    struct sys_rect c = sys_rect(-10000, -10000, 20000, 20000);
    for (p = ren_op_begin(buf->ops); p; p = ren_op_next(p)) {
      switch (p[0].hdr.op) {
        default: {
          struct sys_rect r = sys_rect(p[1].pos.x, p[1].pos.y, p[2].siz.w, p[2].siz.h);
          ren__intersect_rects(&r, &r, &c);
          if (r.w != 0 && r.h != 0) {
            ren__update_overlapping_cells(ren, r, p[0].hdr.hash);
          }
        } break;
        case REN_OP_STYL:
        case REN_OP_COL:
        case REN_OP_EOL:
          break;
      }
    }
    dbg_blk_end(sys);

    /* push rects for all cells changed from last frame and reset cells */
    int max_x = max(1, ren_tar->w / REN_CEL_SIZ + 1);
    int max_y = max(1, ren_tar->h / REN_CEL_SIZ + 1);
    for (int y = 0; y < max_y; y++) {
      for (int x = 0; x < max_x; x++) {
        /* compare previous and current cell for change */
        int cidx = ren__cell_idx(x, y);
        unsigned cur_cel = ren->cel[ren->cel_act][cidx];
        unsigned *prv_cel = &ren->cel[!ren->cel_act][cidx];
        if (cur_cel != *prv_cel) {
          struct sys_rect r = sys_rect(x, y, 1, 1);
          ren__push_rect(ren_tar, sys, &r);
        }
        *prv_cel = FNV1A32_HASH_INITIAL;
      }
    }
    /* expand rects from cells to pixel dirty rectangles */
    struct sys_rect tar_rect = {0, 0, ren_tar->w, ren_tar->h};
    for (int i = 0; i < dyn_cnt(ren_tar->dirty_rects); i++) {
      struct sys_rect *r = &ren_tar->dirty_rects[i];
      r->x *= REN_CEL_SIZ, r->y *= REN_CEL_SIZ;
      r->w *= REN_CEL_SIZ, r->h *= REN_CEL_SIZ;
      ren__intersect_rects(r, r, &tar_rect);
    }
    /* paint over all dirty rects in window via render cmds */
    dbg_blk_begin(sys, "ren:paint");
    for (int i = 0; i < dyn_cnt(ren_tar->dirty_rects); ++i) {
      const struct sys_rect *d = ren_tar->dirty_rects + i;
      const struct ren_clip dclip = ren_clip(d->x, d->y, d->w, d->h);
      struct ren_state state = {.clip = dclip, .thick = 1};
      for (p = ren_op_begin(buf->ops); p; p = ren_op_next(p)) {
        ren__drw_cmd(sys, ren, ren_tar, &state, p, &dclip);
      }
    }
    dbg_blk_end(sys);
  }
  ren->cel_act = !ren->cel_act;
  ren__cleanup(ren, sys);
  dbg_blk_end(sys);
}
static void
ren_shutdown(struct sys *sys) {
  struct ren *ren = sys->renderer;
  arena_free(&ren->mem, sys);
}
/* -----------------------------------------------------------------------------
 *                                    API
 * -----------------------------------------------------------------------------
 */
#ifdef DEBUG_MODE
extern void
dlInit(struct sys *sys) {
  ren_init(sys);
}
extern void
dlBegin(struct sys *sys) {
  ren_begin(sys);
}
extern void
dlEnd(struct sys *sys) {
  ren_end(sys);
}
extern void
dlShutdown(struct sys *sys) {
  ren_shutdown(sys);
}
#endif

