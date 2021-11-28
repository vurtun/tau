#ifdef DEBUG_MODE
/* std */
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

/* sys */
#include "sys/cpu.h"
#include "lib/fmt.h"
#include "lib/fmt.c"
#include "lib/std.h"
#include "sys/dbg.h"
#include "sys/ren.h"
#include "sys/sys.h"
#include "lib/std.c"

/* app */
#include "res.h"
#include "gui.h"

static struct res_api res;
#endif

/* ---------------------------------------------------------------------------
 *                                Data
 * ---------------------------------------------------------------------------
 */
struct gui_img_def {
  int w, h;
  const unsigned *cmap;
  const char **data;
};
// clang-format off
static const unsigned gui_img_check_cmap[] = {[' '] = 0x00000000u, ['.'] = 0xFFFFFFFFu};
static const char *gui_img_check_data[] = {"       .","      ..","     ...",".   ... ",".. ...  ",".....   "," ...    ","  .     "};
static const char *gui_img_times_data[] = {".      .","..    .."," ..  .. ","  ....  ","  ....  "," ..  .. ","..    ..",".      ."};
static const char *gui_img_no_data[] = {"  ....  "," .    . ",".    . .",".   .  .",".  .   .",". .    ."," .    . ","  ....  "};
static const unsigned gui_img_minus_cmap[] = {[' '] = 0x00000000u, ['+'] = 0xFFFFFFFFu, ['.'] = 0xFFFFFFFFu};
static const char *gui_img_minus_data[] = {".........",".       .",".       .",".       .",". +++++ .",".       .",".       .","........."};
static const unsigned gui_img_plus_cmap[] = {[' '] = 0x00000000u, ['+'] = 0xFFFFFFFFu, ['.'] = 0xFFFFFFFFu};
static const char *gui_img_plus_data[] = {".........",".       .",".   +   .",".   +   .",". +++++ .",".   +   .",".   +   .",".       .","........."};
static const char *gui_img_chevron_left_data[] = {"    ..  ","   ..   ","  ..    "," ..     ","  ..    ","   ..   ","    ..  ","        "};
static const char *gui_img_chevron_right_data[] = {"  ..    ","   ..   ","    ..  ","     .. ","    ..  ","   ..   ","  ..    ","        "};
static const struct gui_img_def gui_img_def[GUI_IMG_MAX] = {
  [GUI_IMG_CHECK]     = {.w =  8, .h =  8, .cmap = gui_img_check_cmap,  .data = gui_img_check_data},
  [GUI_IMG_TIMES]     = {.w =  8, .h =  8, .cmap = gui_img_check_cmap,  .data = gui_img_times_data},
  [GUI_IMG_NO]        = {.w =  8, .h =  8, .cmap = gui_img_check_cmap,  .data = gui_img_no_data},
  [GUI_IMG_MINUS]     = {.w =  9, .h =  9, .cmap = gui_img_minus_cmap,  .data = gui_img_minus_data},
  [GUI_IMG_PLUS]      = {.w =  9, .h =  9, .cmap = gui_img_plus_cmap,   .data = gui_img_plus_data},
  [GUI_IMG_CHEVRONL]  = {.w =  8, .h =  8, .cmap = gui_img_check_cmap,  .data = gui_img_chevron_left_data},
  [GUI_IMG_CHEVRONR]  = {.w =  8, .h =  8, .cmap = gui_img_check_cmap,  .data = gui_img_chevron_right_data},
};
// clang-format on

/* ---------------------------------------------------------------------------
 *                                Utility
 * ---------------------------------------------------------------------------
 */
#define gui_inbox(px, py, b) \
  (between(px, (b)->x.min, (b)->x.max) && between(py, (b)->y.min, (b)->y.max))
#define gui_contains(a, b) \
  (gui_inbox((b)->x.min, (b)->y.min, a) && gui_inbox((b)->x.max, (b)->y.max, a))
#define gui_intersect(a, b)                                \
  (((a)->x.min < (b)->x.max) && (a)->x.max > (b)->x.min && \
   (a)->y.min < (b)->y.max && (a)->y.max > (b)->y.min)

static struct gui_bnd
gui_min_max(int a, int b) {
  struct gui_bnd ret;
  ret.min = a;
  ret.max = b;
  ret.ext = max(b - a, 0);
  ret.mid = a + ret.ext / 2;
  return ret;
}
static struct gui_bnd
gui_min_ext(int m, int e) {
  struct gui_bnd ret;
  ret.min = m;
  ret.max = m + e;
  ret.ext = e;
  ret.mid = m + e / 2;
  return ret;
}
static struct gui_bnd
gui_max_ext(int m, int e) {
  struct gui_bnd ret;
  ret.min = m - e;
  ret.max = m;
  ret.ext = e;
  ret.mid = m - e / 2;
  return ret;
}
static struct gui_bnd
gui_mid_min(int c, int m) {
  struct gui_bnd ret;
  ret.ext = (c - m) * 2;
  ret.max = m + ret.ext;
  ret.mid = c;
  ret.min = m;
  return ret;
}
static struct gui_bnd
gui_mid_max(int c, int m) {
  struct gui_bnd ret;
  ret.ext = (m - c) * 2;
  ret.min = m - ret.ext;
  ret.mid = c;
  ret.max = m;
  return ret;
}
static struct gui_bnd
gui_mid_ext(int c, int e) {
  struct gui_bnd ret;
  ret.mid = c;
  ret.ext = e;
  ret.max = c + e / 2;
  ret.min = c - e / 2;
  return ret;
}
static struct gui_bnd
gui_shrink(const struct gui_bnd *x, int p) {
  return gui_min_max((x)->min + (p), (x)->max - (p));
}
static struct gui_bnd
gui_div(const struct gui_bnd *b, int gap, int cnt, int idx) {
  int space = max(0, b->ext - (gap * cnt));
  int s = floori(cast(float, space) / cast(float, cnt));
  idx = clamp(0, idx, max(0, cnt - 1));
  int at = b->min + (s + gap) * idx;
  return gui_min_ext(at, s);
}
static struct gui_box
gui_box_div_x(const struct gui_box *b, int gap, int cnt, int idx) {
  struct gui_box r = {.y = b->y};
  r.x = gui_div(&b->x, gap, cnt, idx);
  return r;
}
static struct gui_box
gui_box_div_y(const struct gui_box *b, int gap, int cnt, int idx) {
  struct gui_box r = {.x = b->x};
  r.y = gui_div(&b->y, gap, cnt, idx);
  return r;
}
static struct gui_box
gui_box_div(const struct gui_box *b, int *gap, int cntx, int cnty, int x, int y) {
  struct gui_box ret;
  ret.x = gui_div(&b->x, gap[0], cntx, x);
  ret.y = gui_div(&b->y, gap[1], cnty, y);
  return ret;
}
static struct gui_box
gui_box_mid_ext(const struct gui_box *b, int w, int h) {
  struct gui_box r = {0};
  r.x = gui_mid_ext(b->x.mid, w);
  r.y = gui_mid_ext(b->y.mid, h);
  return r;
}
static struct gui_box
gui_box(int x, int y, int w, int h) {
  struct gui_box box;
  box.x = gui_min_ext(x, w);
  box.y = gui_min_ext(y, h);
  return box;
}
static struct gui_box
gui_box_pos(const struct gui_box *b, int x, int y) {
  struct gui_box box = {0};
  box.x = gui_min_ext(x, b->x.ext);
  box.y = gui_min_ext(y, b->y.ext);
  return box;
}
static struct gui_box
gui_box_mov(const struct gui_box *b, int x, int y) {
  struct gui_box box = {0};
  box.x = gui_min_ext(b->x.min + x, b->x.ext);
  box.y = gui_min_ext(b->y.min + y, b->y.ext);
  return box;
}
static struct gui_box
gui_clip(int x0, int y0, int x1, int y1) {
  struct gui_box r;
  r.x = gui_min_max(x0, x1);
  r.y = gui_min_max(y0, y1);
  return r;
}
static struct gui_box
gui_pad(const struct gui_box *b, int padx, int pady) {
  struct gui_box r;
  r.x = gui_shrink(&b->x, padx);
  r.y = gui_shrink(&b->y, pady);
  return r;
}
static struct gui_box
gui_padv(const struct gui_box *b, int *pad) {
  return gui_pad(b, pad[0], pad[1]);
}
static struct gui_box
gui_box_posv(const struct gui_box *b, int *p) {
  return gui_box_pos(b,p[0],p[1]);
}
static struct gui_box
gui_cut_lhs(struct gui_box *b, int a, int gap) {
  int x = b->x.min;
  b->x = gui_min_max(min(b->x.max, b->x.min + a + gap), b->x.max);
  return gui_box(x, b->y.min, a, b->y.ext);
}
static struct gui_box
gui_cut_top(struct gui_box *b, int a, int gap) {
  int y = b->y.min;
  b->y = gui_min_max(min(b->y.max, b->y.min + a + gap), b->y.max);
  return gui_box(b->x.min, y, b->x.ext, a);
}
static struct gui_box
gui_cut_rhs(struct gui_box *b, int a, int gap) {
  int x = b->x.max - a;
  b->x = gui_min_max(b->x.min, max(b->x.min, b->x.max - (a + gap)));
  return gui_box(x, b->y.min, a, b->y.ext);
}
static struct gui_box
gui_cut_bot(struct gui_box *b, int a, int gap) {
  int y = b->y.max - a;
  b->y = gui_min_max(b->y.min, max(b->y.min, b->y.max - (a + gap)));
  return gui_box(b->x.min, y, b->x.ext, a);
}
static struct gui_box
gui_cut_box(struct gui_box_cut *cut, int a) {
  switch(cut->side) {
  default: assert(0); break;
  case GUI_BOX_CUT_LHS:
    return gui_cut_lhs(cut->box, a, cut->gap);
  case GUI_BOX_CUT_RHS:
    return gui_cut_rhs(cut->box, a, cut->gap);
  case GUI_BOX_CUT_TOP:
    return gui_cut_top(cut->box, a, cut->gap);
  case GUI_BOX_CUT_BOT:
    return gui_cut_bot(cut->box, a, cut->gap);
  }
  return *cut->box;
}
static void
gui_solve(int *ret, int ext, const int *slots, int cnt, int gap,
          const int *con, struct gui_lay_sol *sol) {
  assert(ret);
  assert(slots);

  struct gui_lay_sol dummy;
  sol = !sol ? &dummy : sol;
  memset(sol, 0, sizeof(*sol));

  for_cnt(i,cnt) {
    if (slots[i] < 0) {
      sol->dyn_cnt++;
      continue;
    }
    int siz = slots[i];
    ret[i] = con ? clamp(con[i*2+0], siz, con[i*2+1]) : siz;
    sol->fix_siz += ret[i];
    sol->fix_cnt++;
  }
  int total = max(0, ext - (gap * cnt));
  if (sol->fix_siz >= total || !sol->dyn_cnt) {
    for_cnt(i,cnt) {
      if (slots[i] < 0) {
        ret[i] = con ? con[i*2+0] : 0;
      }
    }
  }
  sol->weight = 0.0f;
  sol->dyn_siz = max(0, total - sol->fix_siz);
  for_cnt(i, cnt) {
    if (slots[i] >= 0) {
      continue;
    }
    sol->weight += cast(float, -slots[i]);
  }
  int def_dyn_siz = 0;
  for_cnt(i,cnt) {
    if (slots[i] >= 0) {
      continue;
    }
    const float s = cast(float, -slots[i]);
    ret[i] = floori((s / sol->weight) * cast(float, sol->dyn_siz));
    ret[i] = con ? clamp(con[i*2+0], ret[i], con[i*2+1]) : ret[i];
    def_dyn_siz += ret[i];
  }
  if (def_dyn_siz < sol->dyn_siz) {
    int grow_cnt = 0;
    float weight = 0.0f;
    int grow_siz = def_dyn_siz - sol->dyn_siz;
    for_cnt(i,cnt) {
      if (slots[i] >= 0) {
        continue;
      }
      if (!con || ret[i] < con[i*2+1]) {
        weight += cast(float, -slots[i]);
        grow_cnt++;
      }
    }
    while (grow_cnt > 0 && grow_siz > 0) {
      int nxt_siz = 0;
      int nxt_cnt = 0;
      float nxt_weight = 0.0f;
      for_cnt(i, cnt) {
        if (slots[i] >= 0) {
          continue;
        }
        if (!con || ret[i] < con[i*2+1]) {
          float s = cast(float, -slots[i]);
          int siz = floori((s / weight) * cast(float, grow_siz));
          if (con && ret[i] + siz > con[i*2+1]) {
            nxt_siz += ret[i] + siz - con[i*2+1];
            ret[i] = con[i*2+1];
          } else {
            nxt_weight += cast(float, -slots[i]);
            ret[i] += siz;
            nxt_cnt++;
          }
        }
      }
      grow_siz = nxt_siz;
      grow_cnt = nxt_cnt;
      weight = nxt_weight;
    }
  }
}
/* ---------------------------------------------------------------------------
 *                                  Layout
 * ---------------------------------------------------------------------------
 */
static void
gui__lay_init(struct gui_ctx *ctx, struct gui_lay *lay,
              enum gui_lay_type type, int cnt, int item,
              int row_gap, int col_gap) {
  lay->gap[0] = (col_gap < 0) ? ctx->cfg.gap[0] : col_gap;
  lay->gap[1] = (row_gap < 0) ? ctx->cfg.gap[1] : row_gap;
  lay->item = item < 0 ? ctx->cfg.item : item;
  lay->idx = lay->cnt = cnt;
  lay->type = type;
}
static void
gui__hlay(struct gui_ctx *ctx, struct gui_lay *lay, int *items,
          const int *def, int cnt, int row_h, int row_gap, int col_gap,
          const int *con, struct gui_lay_sol *sol) {
  gui__lay_init(ctx, lay, GUI_LAY_ROW, cnt, row_h, row_gap, col_gap);
  gui_solve(items, lay->box.x.ext, def, cnt, col_gap, con, sol);
}
static void
gui__vlay(struct gui_ctx *ctx, struct gui_lay *lay, int *items,
          const int *def, int cnt, int col_w, int row_gap, int col_gap,
          const int *con, struct gui_lay_sol *sol) {
  gui__lay_init(ctx, lay, GUI_LAY_COL, cnt, col_w, row_gap, col_gap);
  gui_solve(items, lay->box.y.ext, def, cnt, col_gap, con, sol);
}
static struct gui_box
gui_hlay_cut(struct gui_lay *lay, int row_h) {
  lay->idx = lay->cnt;
  return gui_cut_top(&lay->box, row_h, lay->gap[1]);
}
static struct gui_box
gui_vlay_cut(struct gui_lay *lay, int col_w) {
  lay->idx = lay->cnt;
  return gui_cut_lhs(&lay->box, col_w, lay->gap[0]);
}
static struct gui_box
gui_hlay_item(struct gui_lay *lay, const int *items) {
  if (lay->idx >= lay->cnt) {
    lay->sub = gui_cut_top(&lay->box, lay->item, lay->gap[1]);
    lay->idx = 0;
  }
  return gui_cut_lhs(&lay->sub, items[lay->idx++], lay->gap[0]);
}
static struct gui_box
gui_vlay_item(struct gui_lay *lay, const int *items) {
  if (lay->idx >= lay->cnt) {
    lay->sub = gui_cut_lhs(&lay->box, lay->item, lay->gap[0]);
    lay->idx = 0;
  }
  return gui_cut_top(&lay->sub, items[lay->idx++], lay->gap[1]);
}
static struct gui_box
gui_lay_item(struct gui_lay *lay, const int *items) {
  switch (lay->type) {
  case GUI_LAY_ROW:
    return gui_hlay_item(lay, items);
  case GUI_LAY_COL:
    return gui_vlay_item(lay, items);
  }
}

/* ---------------------------------------------------------------------------
 *                                  Panel
 * ---------------------------------------------------------------------------
 */
static void
gui_panel_hot(struct gui_ctx *ctx, struct gui_panel *p, struct gui_panel *parent) {
  assert(ctx && p);
  const struct gui_box *b = &p->box;
  p->is_focused = p->id == ctx->focused;
  if (p->focusable && ctx->first_id == ctx->root.id) {
    ctx->first_id = p->id;
  }
  const struct sys *s = ctx->sys;
  int mx = s->mouse.pos[0];
  int my = s->mouse.pos[1];
  int in_box = gui_inbox(mx, my, b);
  p->is_hov = !parent || (parent->is_hov && in_box);
  if ((parent && !parent->is_hot) || !in_box) {
    return;
  }
  p->is_hot = 1;
  if (p->focusable) {
    ctx->focusable = p->id;
  }
  ctx->hot = p->is_hot ? p->id : ctx->hot;
}
static void
gui_focus(struct gui_ctx *ctx, struct gui_panel *p) {
  assert(p);
  assert(ctx);

  ctx->focused = p->id;
  p->is_focused = 1;
  p->has_focus = 1;
}
static void
gui_input(struct gui_input *in, struct gui_ctx *ctx,
                const struct gui_panel *p, unsigned mask) {
  assert(in);
  assert(ctx && p);
  assert(ctx->sys);

  struct sys *sys = ctx->sys;
  memset(in, 0, sizeof(*in));

  /* enter and left */
  in->is_hot = p->is_hot;
  in->entered = p->id == ctx->hot && p->id != ctx->prev_hot;
  in->exited = p->id != ctx->hot && p->id == ctx->prev_hot;

  /* (un)focus */
  in->is_focused = p->id == ctx->focused;
  in->gained_focus = (p->id == ctx->focused && p->id != ctx->prev_focused);
  in->lost_focus = p->id != ctx->focused && p->id == ctx->prev_focused;

  /* mouse pointer */
  struct sys_mouse *mouse = &sys->mouse;
  cpy2(in->mouse.pos, mouse->pos);
  cpy2(in->mouse.pos_last, mouse->pos_last);
  cpy2(in->mouse.pos_delta, mouse->pos_delta);

  in->mouse.pos_rel[0] = in->mouse.pos[0] - p->box.x.min;
  in->mouse.pos_rel[1] = in->mouse.pos[1] - p->box.y.min;
  if (ctx->pass != GUI_INPUT || ctx->disabled) {
    return;
  }
  /* keyboard */
  in->txt = sys->txt;
  in->keys = sys->keys;
  if (p->id == ctx->focused) {
    ctx->focus_next = bit_tst_clr(ctx->keys, GUI_KEY_NEXT_WIDGET) ? 1 : 0;
    if (bit_tst_clr(ctx->keys, GUI_KEY_PREV_WIDGET)) {
      ctx->focused = ctx->prev_id;
      if (ctx->prev_id == ctx->root.id) {
        ctx->focus_last = 1;
      }
    }
  }
  if (p->focusable && !ctx->disabled) {
    ctx->prev_id = p->id;
  }
  /* mouse button */
  for_cnt(i, GUI_MOUSE_BTN_CNT) {
    struct gui_mouse_btn *btn = &in->mouse.btns[i];
    if (p->is_hot) {
      btn->down = mouse->btns[i].down;
      btn->pressed = mouse->btns[i].pressed;
      btn->released = mouse->btns[i].released;
      btn->doubled = mouse->btns[i].doubled;
    }
    if (!(mask & (1 << i))) {
      continue;
    }
    /* (un)grab */
    btn->grabbed = p->id == ctx->btn[i].active && p->id == ctx->btn[i].origin;
    btn->gained_grab =
        p->id == ctx->btn[i].active && p->id != ctx->btn[i].prev_active;
    btn->lost_grab =
        p->id != ctx->btn[i].active && p->id == ctx->btn[i].prev_active;

    /* dragging */
    btn->drag_begin = btn->pressed;
    if (mouse->pos_delta[0] || mouse->pos_delta[1]) {
      if (p->id == ctx->btn[i].active &&
          ctx->btn[i].active == ctx->btn[i].origin) {
        btn->drag_pos[0] = ctx->btn[i].drag_pos[0];
        btn->drag_pos[1] = ctx->btn[i].drag_pos[1];
        btn->dragged = 1;
      }
    }
    /* click */
    if (btn->released) {
      const struct gui_box *b = &p->box;
      int dx = ctx->btn[i].drag_pos[0];
      int dy = ctx->btn[i].drag_pos[1];
      btn->clk = gui_inbox(dx, dy, b) && p->is_hot;
      if (ctx->btn[i].origin == p->id) {
        ctx->btn[i].origin = ctx->root.id;
        btn->drag_end = 1;
      }
    }
  }
}
static enum gui_state
gui_panel_state(const struct gui_ctx *ctx, const struct gui_panel *p) {
  assert(p);
  assert(ctx);

  enum gui_state ret;
  int clk = (ctx->sys->mouse.btn.left.down || ctx->sys->mouse.btn.right.down);
  const struct gui_box *b = &p->box;
  if (gui_intersect(&ctx->clip, b) == 0) {
    ret = GUI_HIDDEN;
  } else if (ctx->disabled) {
    ret = GUI_DISABLED;
  } else if (p->is_hot && clk) {
    ret = GUI_PRESSED;
  } else if ((p->id == ctx->focused) && !ctx->focus_next) {
    ret = GUI_FOCUSED;
  } else if (p->is_hot) {
    ret = GUI_HOVERED;
  } else {
    ret = GUI_NORMAL;
  }
  return ret;
}
static void
gui_panel_drw(struct gui_ctx *ctx, const struct gui_box *b) {
  assert(b);
  assert(ctx);
  struct sys *sys = ctx->sys;

  sys->ren.drw.line_style(ctx->ren, 1);
  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_BG]);
  sys->ren.drw.box(ctx->ren, gui_unbox(b));

  sys->ren.drw.hln(ctx->ren, b->y.min, b->x.min, b->x.max - 1);
  sys->ren.drw.vln(ctx->ren, b->x.min, b->y.min, b->y.max - 1);

  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_LIGHT]);
  sys->ren.drw.hln(ctx->ren, b->y.min, b->x.min + 1, b->x.max - 1);
  sys->ren.drw.vln(ctx->ren, b->x.min, b->y.min + 1, b->y.max - 1);

  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_SHADOW]);
  sys->ren.drw.hln(ctx->ren, b->y.max - 1, b->x.min, b->x.max - 1);
  sys->ren.drw.vln(ctx->ren, b->x.max - 1, b->y.min, b->y.max - 1);
}
static void
gui_focus_drw(struct gui_ctx *ctx, const struct gui_box *b, int pad) {
  struct gui_box box;
  box.x = gui_shrink(&b->x, pad);
  box.y = gui_shrink(&b->y, pad);

  struct sys *sys = ctx->sys;
  sys->ren.drw.line_style(ctx->ren, 1);
  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_TXT]);
  sys->ren.drw.sbox(ctx->ren, gui_unbox(&box));
}
static void
gui_panel_open(struct gui_ctx *ctx, struct gui_panel *pan,
               struct gui_panel *parent, unsigned long long id) {
  assert(pan);
  assert(ctx);

  pan->id = parent ? fnv1au64(id, parent->id) : id;
  pan->max[0] = pan->max[1] = 0;
  pan->is_hot = pan->is_hov = 0;
  pan->is_focused = 0;

  memcpy(pan->mouse_pos, ctx->sys->mouse.pos, sizeof(pan->mouse_pos));
  pan->rel_mouse_pos[0] = pan->mouse_pos[0] - pan->box.x.min;
  pan->rel_mouse_pos[1] = pan->mouse_pos[1] - pan->box.y.min;

  ctx->cur_id = pan->id;
  if (ctx->focus_next && pan->focusable && !ctx->disabled) {
    ctx->focused = pan->id;
    ctx->focus_next = 0;
  }
  gui_panel_hot(ctx, pan, parent);
  pan->state = gui_panel_state(ctx, pan);
  pan->has_focus = pan->id == ctx->focused;
  pan->is_focused = pan->has_focus;
}
static void
gui_panel_close(struct gui_ctx *ctx, struct gui_panel *pan,
                struct gui_panel *p) {
  assert(ctx);
  assert(pan);
  unused(ctx);

  if (p) {
    p->max[0] = max(p->max[0], pan->box.x.max);
    p->max[1] = max(p->max[1], pan->box.y.max);
    if (pan->is_focused) {
      p->is_focused = 1;
    }
  }
}
#define gui_panel_end(ctx,pan,p) gui_panel_close(ctx,pan,p)

static void
gui_panel_begin(struct gui_ctx *ctx, struct gui_panel *pan,
                struct gui_panel *parent) {
  gui_panel_open(ctx, pan, parent, (ctx)->id++);
}
static void
gui_panel_id(struct gui_ctx *ctx, struct gui_panel *pan,
             struct gui_panel *parent, unsigned long long id) {
  assert(pan);
  assert(ctx);
  assert(parent);

  gui_panel_open(ctx, pan, parent, id);
  gui_panel_close(ctx, pan, parent);
}
static void
gui_focus_on(struct gui_ctx *ctx, int cond) {
  assert(ctx);
  ctx->focus_next = cond ? 1 : ctx->focus_next;
}
/* ---------------------------------------------------------------------------
 *                                  Context
 * ---------------------------------------------------------------------------
 */
static void
gui_color_scheme(struct gui_ctx *ctx, enum gui_col_scheme scm) {
  switch (scm) {
    case GUI_COL_SCHEME_SYS: {
      const struct sys *s = ctx->sys;
      ctx->cfg.col[GUI_COL_BG] = s->col[SYS_COL_WIN];
      ctx->cfg.col[GUI_COL_CONTENT] = s->col[SYS_COL_BG];
      ctx->cfg.col[GUI_COL_CONTENT_HOV] = s->col[SYS_COL_HOV];
      ctx->cfg.col[GUI_COL_SEL] = s->col[SYS_COL_SEL];
      ctx->cfg.col[GUI_COL_LIGHT] = s->col[SYS_COL_LIGHT];
      ctx->cfg.col[GUI_COL_SHADOW_SEAM] = s->col[SYS_COL_SHADOW];
      ctx->cfg.col[GUI_COL_SHADOW] = s->col[SYS_COL_SHADOW];
      ctx->cfg.col[GUI_COL_ICO] = s->col[SYS_COL_ICO];
      ctx->cfg.col[GUI_COL_TXT] = s->col[SYS_COL_TXT];
      ctx->cfg.col[GUI_COL_TXT_DISABLED] = s->col[SYS_COL_TXT_DISABLED];
      ctx->cfg.col[GUI_COL_TXT_SELECTED] = s->col[SYS_COL_TXT_SEL];
    } break;

    case GUI_COL_SCHEME_DARK: {
      ctx->cfg.col[GUI_COL_BG] = col_rgb_hex(0x4b4b4b);
      ctx->cfg.col[GUI_COL_CONTENT] = col_rgb_hex(0x313131);
      ctx->cfg.col[GUI_COL_CONTENT_HOV] = col_rgb_hex(0x202020);
      ctx->cfg.col[GUI_COL_SEL] = col_rgb_hex(0x2973CA);
      ctx->cfg.col[GUI_COL_LIGHT] = col_rgb_hex(0x6D6D6D);
      ctx->cfg.col[GUI_COL_SHADOW_SEAM] = col_rgb_hex(0x2F2F2F);
      ctx->cfg.col[GUI_COL_SHADOW] = col_rgb_hex(0x202020);
      ctx->cfg.col[GUI_COL_ICO] = col_rgb_hex(0xE0E0E0);
      ctx->cfg.col[GUI_COL_TXT] = col_rgb_hex(0xE0E0E0);
      ctx->cfg.col[GUI_COL_TXT_DISABLED] = col_rgb_hex(0x747474);
      ctx->cfg.col[GUI_COL_TXT_SELECTED] = col_rgb_hex(0xFFFFFF);
    } break;

    case GUI_COL_SCHEME_STEAM: {
      ctx->cfg.col[GUI_COL_BG] = col_rgb_hex(0x4D5945);
      ctx->cfg.col[GUI_COL_CONTENT] = col_rgb_hex(0x3F4537);
      ctx->cfg.col[GUI_COL_CONTENT_HOV] = col_rgb_hex(0x363A2E);
      ctx->cfg.col[GUI_COL_SEL] = col_rgb_hex(0xB37A00);
      ctx->cfg.col[GUI_COL_LIGHT] = col_rgb_hex(0x7d8877);
      ctx->cfg.col[GUI_COL_SHADOW_SEAM] = col_rgb_hex(0x3E4938);
      ctx->cfg.col[GUI_COL_SHADOW] = col_rgb_hex(0x313C2B);
      ctx->cfg.col[GUI_COL_ICO] = col_rgb_hex(0xF0F0F0);
      ctx->cfg.col[GUI_COL_TXT] = col_rgb_hex(0xF0F0F0);
      ctx->cfg.col[GUI_COL_TXT_DISABLED] = col_rgb_hex(0x606060);
      ctx->cfg.col[GUI_COL_TXT_SELECTED] = col_rgb_hex(0xFFFFFF);
    } break;

    case GUI_COL_SCHEME_GRAY: {
      ctx->cfg.col[GUI_COL_BG] = col_rgb_hex(0xD4D0C8);
      ctx->cfg.col[GUI_COL_CONTENT] = col_rgb_hex(0xEDEDED);
      ctx->cfg.col[GUI_COL_CONTENT_HOV] = col_rgb_hex(0xB9E3F1);
      ctx->cfg.col[GUI_COL_SEL] = col_rgb_hex(0x4cbfff);
      ctx->cfg.col[GUI_COL_LIGHT] = col_rgb_hex(0xF0F0F0);
      ctx->cfg.col[GUI_COL_SHADOW_SEAM] = col_rgb_hex(0x808080);
      ctx->cfg.col[GUI_COL_SHADOW] = col_rgb_hex(0x404040);
      ctx->cfg.col[GUI_COL_ICO] = col_rgb_hex(0x707070);
      ctx->cfg.col[GUI_COL_TXT] = col_rgb_hex(0x202020);
      ctx->cfg.col[GUI_COL_TXT_DISABLED] = col_rgb_hex(0xFFFFFF);
      ctx->cfg.col[GUI_COL_TXT_SELECTED] = col_rgb_hex(0x179bd7);
    } break;
  }
}
static int
gui__blit_ico(const struct gui_img_def *img, struct sys *sys, struct arena *mem) {
  int siz = align_up(img->w * img->h, 4);
  unsigned *dst = arena_alloc(mem, sys, siz * 4);
  for_cnt(y, img->h) {
    for_cnt(x, img->w) {
      unsigned char sym = (unsigned char)img->data[y][x];
      dst[x + y * img->w] = col_rgba_hex(img->cmap[sym]);
    }
  }
  return sys->ren.tex.mk(sys->renderer, dst, img->w, img->h);
}
static void
gui_init(struct gui_ctx *ctx, struct arena *mem, enum gui_col_scheme scm) {
  assert(ctx);
  gui_color_scheme(ctx, scm);

  ctx->cfg.sep = 6;
  ctx->cfg.item = 22;
  ctx->cfg.elm = 18;
  ctx->cfg.tab = 120;
  ctx->cfg.depth = 22;
  ctx->cfg.btn_pad = 20;
  ctx->cfg.grid = 40;

  ctx->cfg.pan_pad[0] = 4;
  ctx->cfg.pan_pad[1] = 4;
  ctx->cfg.pan_gap[0] = 4;
  ctx->cfg.pan_gap[1] = 4;

  ctx->cfg.grp_off = 10;
  ctx->cfg.grp_pad = 7;

  ctx->cfg.lay_pad[0] = 0;
  ctx->cfg.lay_pad[1] = 2;

  ctx->cfg.lst_pad[0] = 4;
  ctx->cfg.lst_pad[1] = 3;

  ctx->cfg.gap[0] = 4;
  ctx->cfg.gap[1] = 3;
  ctx->cfg.pad[0] = 5;
  ctx->cfg.pad[1] = 2;

  ctx->cfg.ico = 14;
  ctx->cfg.scrl = 14;
  ctx->txt_state.buf = arena_dyn(mem, ctx->sys, char, 2 * 1024);
  for_cnt(i, GUI_IMG_MAX) {
    ctx->ico[i] = gui__blit_ico(gui_img_def + i, ctx->sys, mem);
  }
}
static void
gui_free(struct gui_ctx *ctx) {
  dyn_free(ctx->txt_state.buf ,ctx->sys);
}
static void
gui_cfg_pushi(struct gui_cfg_stk *stk, void *ptr, int val) {
  assert(stk);
  assert(ptr);

  stk->ptr = ptr;
  memcpy(&stk->val, ptr, sizeof(val));
  memcpy(ptr, &val, sizeof(val));
}
static void
gui_cfg_pushi_on(struct gui_cfg_stk *stk, void *ptr, int val, int cond) {
  if (cond) {
    gui_cfg_pushi(stk, ptr, val);
  }
}
static void
gui_cfg_pushu(struct gui_cfg_stk *stk, void *ptr, unsigned val) {
  assert(stk);
  assert(ptr);

  stk->ptr = ptr;
  memcpy(&stk->val, ptr, sizeof(val));
  memcpy(ptr, &val, sizeof(val));
}
static void
gui_cfg_pushu_on(struct gui_cfg_stk *stk, void *ptr, unsigned val, int cond) {
  if (cond) {
    gui_cfg_pushu(stk, ptr, val);
  }
}
static void
gui_cfg_pop(const struct gui_cfg_stk *stk) {
  assert(stk);
  memcpy(stk->ptr, &stk->val, sizeof(stk->val));
}
static void
gui_cfg_pop_on(const struct gui_cfg_stk *stk, int cond) {
  if (cond) {
    gui_cfg_pop(stk);
  }
}
static void
gui_input_begin(struct gui_ctx *ctx, struct sys_mouse *mouse) {
  assert(ctx);
  assert(mouse);
  for_cnt(i, GUI_MOUSE_BTN_CNT) {
    if (!mouse->btns[i].pressed) {
      continue;
    }
    ctx->btn[i].origin = ctx->hot;
    ctx->btn[i].active = ctx->btn[i].origin;
    ctx->btn[i].drag_pos[0] = mouse->pos[0];
    ctx->btn[i].drag_pos[1] = mouse->pos[1];
    ctx->focused = ctx->focusable;
  }
}
static void
gui_input_end(struct gui_ctx *ctx, struct sys_mouse *mouse) {
  assert(ctx);
  assert(mouse);
  for_cnt(i, GUI_MOUSE_BTN_CNT) {
    ctx->btn[i].prev_active = ctx->btn[i].active;
    if (mouse->btns[i].released) {
      ctx->btn[i].origin = ctx->root.id;
    }
  }
}
static void
gui_input_consume(struct gui_ctx *ctx) {
  for_cnt(i, GUI_MOUSE_BTN_CNT) {
    memset(&ctx->btn, 0, sizeof(ctx->btn));
  }
  memset(ctx->keys, 0, sizeof(ctx->keys));
}
static struct gui_panel *
gui_begin(struct gui_ctx *ctx) {
  assert(ctx);
  if (ctx->pass == GUI_FINISHED) {
    ctx->pass = GUI_INPUT;
    return 0;
  }
  struct sys *sys = ctx->sys;
  ctx->disabled = 0;

  /* tree */
  struct gui_panel *pan = &ctx->root;
  pan->id = 14695981039346656037llu;
  if (ctx->pass == GUI_INPUT) {
    /* skip input pass if no drag mouse movement */
    struct sys *s = ctx->sys;
    int no_move = !s->mouse_mod || (s->mouse_mod && !s->mouse_grap);
    if (!s->key_mod && !s->btn_mod && !s->txt_mod && !s->scrl_mod && no_move) {
      ctx->pass = GUI_RENDER;
    }
  }
  /* setup pass */
  switch (ctx->pass) {
    case GUI_INPUT: {
      ctx->prev_id = pan->id;
      ctx->cur_id = pan->id;
      ctx->prev_hot = ctx->hot;
      ctx->prev_focused = ctx->focused;
      ctx->first_id = pan->id;
      gui_input_begin(ctx, &sys->mouse);
      ctx->focusable = ctx->root.id;
    } break;
    case GUI_RENDER: {
      sys->cursor = SYS_CUR_ARROW;
      ctx->ren = sys->ren.que.mk(sys->ren.que.dev);
      sys->ren.drw.line_style(ctx->ren, 1);
      sys->ren.drw.clip(ctx->ren, 0, 0, sys->win.w, sys->win.h);
      sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_BG]);
      sys->ren.drw.box(ctx->ren, 0, 0, sys->win.w, sys->win.h);
    } break;
  }
  /* root */
  pan->box = gui_box(0, 0, sys->win.w, sys->win.h);
  ctx->clip = gui_box(0, 0, sys->win.w, sys->win.h);
  gui_panel_hot(ctx, pan, 0);
  ctx->box = gui_padv(&pan->box, ctx->cfg.pan_pad);
  return &ctx->root;
}
static void
gui_end(struct gui_ctx *ctx) {
  assert(ctx);
  assert(ctx->disabled == 0);
  ctx->id = 0;
  switch (ctx->pass) {
    case GUI_INPUT: {
      if (ctx->focus_last || bit_tst_clr(ctx->keys, GUI_KEY_PREV_WIDGET)) {
        ctx->focused = ctx->prev_id;
      } else if (ctx->focus_next ||
                 bit_tst_clr(ctx->keys, GUI_KEY_NEXT_WIDGET)) {
        ctx->focused = ctx->first_id;
      }
      ctx->pass = GUI_RENDER;
      ctx->first_id = ctx->root.id;
      ctx->prev_focused = ctx->focused;
      ctx->focus_last = 0;
      ctx->focus_next = 0;

      gui_input_end(ctx, &ctx->sys->mouse);
      memset(ctx->keys, 0, sizeof(ctx->keys));
    } break;
    case GUI_RENDER: {
      ctx->pass = GUI_FINISHED;
      ctx->ren = 0;
    } break;
  }
}
static void
gui_scissor(struct gui_ctx *ctx, int lhs, int top, int rhs, int bot) {
  assert(ctx);
  struct sys *sys = ctx->sys;
  sys->ren.drw.clip(ctx->ren, lhs, top, rhs - lhs, bot - top);
  ctx->clip = gui_clip(lhs, top, rhs, bot);
}
static void
gui_clip_begin(struct gui_box *tmp, struct gui_ctx *ctx, int lhs, int top,
               int rhs, int bot) {
  assert(ctx);
  assert(tmp);
  if (ctx->pass != GUI_RENDER) {
    return;
  }
  *tmp = ctx->clip;
  int x0 = max(tmp->x.min, lhs);
  int y0 = max(tmp->y.min, top);
  int x1 = min(tmp->x.max, rhs);
  int y1 = min(tmp->y.max, bot);
  gui_scissor(ctx, x0, y0, x1, y1);
}
static void
gui_clip_end(struct gui_ctx *ctx, struct gui_box *clip) {
  assert(ctx);
  assert(clip);
  if (ctx->pass == GUI_RENDER) {
    gui_scissor(ctx, gui_unbox(clip));
  }
}
static void
gui_disable(struct gui_ctx *ctx, int cond) {
  assert(ctx);
  if (cond) ctx->disabled++;
}
static void
gui_enable(struct gui_ctx *ctx, int cond) {
  assert(ctx);
  if (cond) {
    assert(ctx->disabled > 0);
    ctx->disabled--;
  }
}

/* ---------------------------------------------------------------------------
 *                                  Label
 * ---------------------------------------------------------------------------
 */
struct gui_txt_bnd {
  int len, width;
  const char *end;
};
static void
gui_txt_ext(int *ext, struct gui_ctx *ctx, struct str txt) {
  assert(ext);
  assert(ctx);
  struct res_fnt *fnt = ctx->res->fnt;
  res.fnt_ext(ext, ctx->res, fnt, txt);
}
static int
gui_txt_width(struct gui_ctx *ctx, struct str txt) {
  assert(ctx);

  int ext[2];
  gui_txt_ext(ext, ctx, txt);
  return ext[0];
}
static void
gui_txt_fit(struct gui_txt_bnd *bnd, int space, struct gui_ctx *ctx,
            struct str txt) {
  memset(bnd, 0, sizeof(*bnd));
  bnd->end = txt.str;
  if (!space) {
    return;
  }
  unsigned rune = 0;
  for_utf(&rune, it, _, txt) {
    int w = gui_txt_width(ctx, strp(txt.str, it.end));
    if (w > space) {
      return;
    }
    bnd->len += it.len;
    bnd->end = it.end;
    bnd->width = w;
  }
}
static void
gui_drw_txt_uln(struct gui_ctx *ctx, struct gui_panel *pan,
                struct str txt, int uln_pos, int uln_cnt) {
  struct sys *sys = ctx->sys;
  int n = txt.len;
  uln_pos = clamp(0, uln_pos, n);
  uln_cnt = min(uln_cnt, max(0, n - uln_pos));

  struct str uln_min = utf_at(0, txt, uln_pos);
  struct str uln_max = utf_at(0, strp(uln_min.end, txt.end), uln_cnt);

  int off[2], len[2];
  gui_txt_ext(off, ctx, strp(txt.str, uln_min.str));
  gui_txt_ext(len, ctx, strp(uln_min.str, uln_max.str));
  sys->ren.drw.hln(ctx->ren, pan->box.y.max - 1, pan->box.x.min + off[0],
            pan->box.x.min + off[0] + len[0]);
}
static void
gui_align_txt(struct gui_box *b, const struct gui_align *align, int *ext) {
  assert(b);
  assert(ext);
  assert(align);
  switch (align->h) {
    case GUI_HALIGN_MID:
      b->x = gui_mid_ext(b->x.mid, ext[0]);
      break;
    case GUI_HALIGN_RIGHT:
      b->x = gui_max_ext(b->x.max, ext[0]);
      break;
  }
  switch (align->v) {
    case GUI_VALIGN_MID:
      b->y = gui_mid_ext(b->y.mid, ext[1]);
      break;
    case GUI_VALIGN_BOT:
      b->y = gui_max_ext(b->y.max, ext[1]);
      break;
  }
}
static void
gui_txt_drw(struct gui_ctx *ctx, struct gui_panel *pan, struct str txt,
            int uln_pos, int uln_cnt) {
  assert(ctx);
  assert(pan);
  struct sys *sys = ctx->sys;
  switch (pan->state) {
    case GUI_DISABLED: {
      sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_TXT_DISABLED]);
      res.print(ctx->ren, ctx->res, pan->box.x.min, pan->box.y.min, txt);
    } break;
    case GUI_HOVERED:
    case GUI_PRESSED:
    case GUI_FOCUSED:
    case GUI_NORMAL: {
      sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_TXT]);
      res.print(ctx->ren, ctx->res, pan->box.x.min, pan->box.y.min, txt);
    } break;
  }
  if (uln_pos >= 0) {
    gui_drw_txt_uln(ctx, pan, txt, uln_pos, uln_cnt);
  }
}
static void
gui_txt_uln(struct gui_ctx *ctx, struct gui_panel *pan,
            struct gui_panel *parent, struct str txt,
            const struct gui_align *align, int uln_pos, int uln_cnt) {
  assert(ctx);
  assert(pan);
  assert(parent);

  /* calculate text extend */
  int ext[2] = {0};
  gui_txt_ext(ext, ctx, txt);

  /* fit text into space */
  if (ext[0] > pan->box.x.ext) {
    struct gui_txt_bnd bnd;
    gui_txt_fit(&bnd, pan->box.x.ext, ctx, txt);
    ext[0] = bnd.width;
    txt = strp(txt.str, bnd.end);
  }
  /* align text */
  static const struct gui_align def_align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
  align = !align ? &def_align : align;
  gui_align_txt(&pan->box, align, ext);

  /* draw text */
  gui_panel_begin(ctx, pan, parent);
  if (ctx->pass == GUI_RENDER && pan->state != GUI_HIDDEN) {
    gui_txt_drw(ctx, pan, txt, uln_pos, uln_cnt);
  }
  gui_panel_end(ctx, pan, parent);
}
static void
gui_txt(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
        struct str txt, const struct gui_align *align) {
  assert(ctx);
  assert(pan);
  assert(parent);
  gui_txt_uln(ctx, pan, parent, txt, align, -1, 0);
}
static void
gui_txtvf(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
          const struct gui_align *align, const char *fmt, va_list args) {
  assert(ctx);
  assert(fmt);
  assert(pan);
  assert(parent);

  char buf[2 * 1024];
  int n = fmtvsn(buf, cntof(buf), fmt, args);
  if (n < cntof(buf)) {
    gui_txt(ctx, pan, parent, str0(buf), align);
  }
}
static void
gui_txtf(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
         const struct gui_align *align, const char *fmt, ...) {
  assert(ctx);
  assert(fmt);
  assert(pan);
  assert(parent);

  va_list args;
  va_start(args, fmt);
  gui_txtvf(ctx, pan, parent, align, fmt, args);
}
static void
gui_lbl(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
        struct gui_box_cut *cut, struct str txt) {
  assert(ctx);
  assert(cut);
  assert(pan);
  assert(parent);

  int tw = gui_txt_width(ctx, txt);
  pan->box = gui_cut_box(cut, tw);
  gui_txt(ctx, pan, parent, txt, 0);
}
static void
gui_lblvf(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
          struct gui_box_cut *cut, const char *fmt, va_list args) {
  assert(ctx);
  assert(fmt);
  assert(pan);
  assert(parent);

  char buf[2 * 1024];
  const int n = fmtvsn(buf, cntof(buf), fmt, args);
  if (n < cntof(buf)) {
    gui_lbl(ctx, pan, parent, cut, str0(buf));
  }
}
static void
gui_lblf(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
         struct gui_box_cut *cut, const char *fmt, ...) {
  assert(ctx);
  assert(fmt);
  assert(pan);
  assert(parent);

  va_list args;
  va_start(args, fmt);
  gui_lblvf(ctx, pan, parent, cut, fmt, args);
  va_end(args);
}
static void
gui_tm(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
       const char *fmt, struct tm *tm) {
  assert(tm);
  assert(fmt);
  assert(ctx);
  assert(pan);
  assert(parent);

  char buf[2 * 1024];
  strftime(buf, cntof(buf), fmt, tm);
  gui_txt(ctx, pan, parent, str0(buf), 0);
}
/* ---------------------------------------------------------------------------
 *                                  Icon
 * ---------------------------------------------------------------------------
 */
static void
gui_icon(struct gui_ctx *ctx, struct gui_icon *icn, struct gui_panel *parent,
         const char *icon) {
  assert(ctx);
  assert(icn);
  assert(parent);
  struct sys *sys = ctx->sys;

  icn->pan.box = icn->box;
  gui_panel_begin(ctx, &icn->pan, parent);
  if (ctx->pass == GUI_RENDER && icn->pan.state != GUI_HIDDEN) {
    struct gui_box ico = {0};
    ico = gui_box_mid_ext(&icn->pan.box, ctx->cfg.ico, ctx->cfg.ico);
    if (icn->pan.state == GUI_DISABLED) {
      sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_TXT_DISABLED]);
    } else sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_ICO]);
    res.ico(ctx->ren, ctx->res, ico.x.min, ico.y.min, icon);
  }
  gui_panel_end(ctx, &icn->pan, parent);

  gui_input(&icn->in, ctx, &icn->pan, GUI_BTN_LEFT);
  icn->pressed = icn->in.mouse.btn.left.pressed;
  icn->released = icn->in.mouse.btn.left.released;
  icn->clk = icn->in.mouse.btn.left.clk;
  if (icn->pan.state == GUI_FOCUSED) {
    icn->clk = bit_tst_clr(ctx->keys, GUI_KEY_ACT) ? 1 : icn->clk;
  }
}
static void
gui_icon_box(struct gui_ctx *ctx, struct gui_panel *pan,
             struct gui_panel *parent, const char *icon,
             struct str txt) {
  assert(ctx);
  assert(pan);
  assert(parent);

  int tw = gui_txt_width(ctx, txt);
  int w = tw + ctx->cfg.item + ctx->cfg.gap[0];
  pan->box.x = gui_min_ext(pan->box.x.min, min(pan->box.x.ext, w));
  gui_panel_begin(ctx, pan, parent);
  {
    struct gui_icon ico = {.box = pan->box};
    ico.box.x = gui_min_ext(pan->box.x.min, ctx->cfg.item);
    gui_icon(ctx, &ico, pan, icon);

    struct gui_panel lbl = {.box = pan->box};
    static const struct gui_align def_align = {GUI_HALIGN_MID, GUI_VALIGN_MID};
    lbl.box.x = gui_min_max(ico.box.x.max + ctx->cfg.gap[0], pan->box.x.max);
    gui_txt(ctx, &lbl, pan, txt, &def_align);
  }
  gui_panel_end(ctx, pan, parent);
}
static void
gui__img(struct gui_ctx *ctx, struct sys *sys, int x, int y, int img_id) {
  assert(sys);
  assert(img_id < GUI_IMG_MAX);
  const struct gui_img_def *def = gui_img_def + img_id;
  sys->ren.drw.col(ctx->ren, col_rgb(0xff, 0xff, 0xff));
  sys->ren.drw.img(ctx->ren, x, y, 0, 0, def->w, def->h, ctx->ico[img_id]);
}

/* ---------------------------------------------------------------------------
 *                                  Button
 * ---------------------------------------------------------------------------
 */
static void
gui_btn_drw(struct gui_ctx *ctx, const struct gui_panel *btn) {
  assert(ctx);
  assert(btn);
  const struct gui_box *b = &btn->box;
  switch (btn->state) {
    case GUI_FOCUSED:
    case GUI_DISABLED:
    case GUI_HOVERED:
    case GUI_NORMAL: {
      gui_panel_drw(ctx, &btn->box);
      if (btn->state == GUI_FOCUSED) {
        gui_focus_drw(ctx, b, 0);
      }
    } break;
    case GUI_PRESSED: {
      struct sys *sys = ctx->sys;
      sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_BG]);
      sys->ren.drw.box(ctx->ren, gui_unbox(b));

      sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_SHADOW]);
      sys->ren.drw.hln(ctx->ren, b->y.min + 1, b->x.min + 1, b->x.max - 2);
      sys->ren.drw.vln(ctx->ren, b->x.min + 1, b->y.min + 1, b->y.max - 2);

      sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_LIGHT]);
      sys->ren.drw.hln(ctx->ren, b->y.max-1, b->x.min, b->x.max);
      sys->ren.drw.vln(ctx->ren, b->x.max, b->y.min, b->y.max-1);
    } break;
  }
}
static void
gui_btn_begin(struct gui_ctx *ctx, struct gui_btn *btn,
              struct gui_panel *parent) {
  assert(btn);
  assert(ctx);
  assert(parent);

  btn->pan.box = btn->box;
  gui_panel_begin(ctx, &btn->pan, parent);
  if (ctx->pass == GUI_RENDER) {
    gui_btn_drw(ctx, &btn->pan);
  }
}
static void
gui_btn_end(struct gui_ctx *ctx, struct gui_btn *btn, struct gui_panel *parent) {
  assert(ctx);
  assert(btn);
  assert(parent);

  struct gui_panel *pan = &btn->pan;
  gui_panel_hot(ctx, pan, parent);
  gui_panel_end(ctx, pan, parent);
  gui_input(&btn->in, ctx, pan, GUI_BTN_LEFT|GUI_BTN_RIGHT|GUI_BTN_MIDDLE);

  btn->pressed = btn->in.mouse.btn.left.pressed;
  btn->released = btn->in.mouse.btn.left.released;
  btn->clk = btn->in.mouse.btn.left.clk;
  if (pan->state == GUI_FOCUSED) {
    btn->clk = bit_tst_clr(ctx->keys, GUI_KEY_ACT) ? 1 : btn->clk;
  }
}
static int
gui_btn_txt(struct gui_ctx *ctx, struct gui_btn *btn, struct gui_panel *parent,
            struct str s, const struct gui_align *align) {
  assert(btn);
  assert(ctx);
  assert(parent);

  static const struct gui_align def_align = {GUI_HALIGN_MID, GUI_VALIGN_MID};
  gui_btn_begin(ctx, btn, parent);
  {
    struct gui_panel txt = {.box = btn->pan.box};
    txt.box.x = gui_shrink(&btn->pan.box.x, ctx->cfg.pad[0]);
    gui_txt(ctx, &txt, &btn->pan, s, align ? align : &def_align);
  }
  gui_btn_end(ctx, btn, parent);
  return btn->clk;
}
static int
gui_btn_lbl(struct gui_ctx *ctx, struct gui_btn *btn, struct gui_panel *parent,
            struct gui_box_cut *cut, struct str txt,
            const struct gui_align *align) {
  assert(btn);
  assert(ctx);
  assert(parent);

  /* calculate desired space */
  int tw = gui_txt_width(ctx, txt);
  int w = tw + ctx->cfg.btn_pad * 2;
  btn->box = gui_cut_box(cut, w);
  return gui_btn_txt(ctx, btn, parent, txt, align);
}
static int
gui_btn_ico(struct gui_ctx *ctx, struct gui_btn *btn, struct gui_panel *parent,
            const char *icon) {
  assert(ctx);
  assert(btn);
  assert(parent);
  gui_btn_begin(ctx, btn, parent);
  {
    struct gui_icon ico = {0};
    ico.box = gui_box_mid_ext(&btn->box, ctx->cfg.ico, ctx->cfg.ico);
    gui_icon(ctx, &ico, &btn->pan, icon);
  }
  gui_btn_end(ctx, btn, parent);
  return btn->clk;
}
/* ---------------------------------------------------------------------------
 *                                  Check
 * ---------------------------------------------------------------------------
 */
#define gui_chk_box_bool(chkd) ((chkd) == GUI_CHK_UNSELECTED ? 0 : 1)
#define gui_chk_box_state(b) ((b) ? GUI_CHK_SELECTED : GUI_CHK_UNSELECTED)

static enum gui_chk_state
gui__chk_tog(enum gui_chk_state state) {
  switch (state) {
    case GUI_CHK_UNSELECTED:
      state = GUI_CHK_SELECTED;
      break;
    case GUI_CHK_PARTIAL:
      state = GUI_CHK_UNSELECTED;
      break;
    case GUI_CHK_SELECTED:
      state = GUI_CHK_UNSELECTED;
      break;
  }
  return state;
}
static void
gui__chk_cur(struct gui_ctx *ctx, const struct gui_panel *pan,
             enum gui_chk_state chkd) {
  assert(ctx);
  assert(pan);
  struct sys *sys = ctx->sys;
  int cur_siz = gui_img_def[GUI_IMG_CHECK].w;
  struct gui_box cur = gui_box_mid_ext(&pan->box, cur_siz, cur_siz);
  if (ctx->pass == GUI_RENDER) {
    if (chkd == GUI_CHK_SELECTED) {
      gui__img(ctx, sys, cur.x.min, cur.y.min, GUI_IMG_CHECK);
    } else if (chkd == GUI_CHK_PARTIAL) {
      sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_TXT]);
      sys->ren.drw.box(ctx->ren, gui_unbox(&cur));
    }
  }
}
static void
gui__chk_drw(struct gui_ctx *ctx, const struct gui_panel *pan) {
  assert(ctx);
  assert(pan);
  struct sys *sys = ctx->sys;

  /* background */
  const struct gui_box *b = &pan->box;
  sys->ren.drw.line_style(ctx->ren, 1);

  int bg = (pan->state == GUI_DISABLED) ? GUI_COL_BG : GUI_COL_CONTENT;
  sys->ren.drw.col(ctx->ren, ctx->cfg.col[bg]);
  sys->ren.drw.box(ctx->ren, gui_unbox(b));

  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_SHADOW]);
  sys->ren.drw.hln(ctx->ren, b->y.min + 1, b->x.min + 1, b->x.max - 1);
  sys->ren.drw.vln(ctx->ren, b->x.min + 1, b->y.min + 1, b->y.max - 2);

  int c = (pan->state == GUI_DISABLED) ? GUI_COL_LIGHT : GUI_COL_BG;
  sys->ren.drw.col(ctx->ren, ctx->cfg.col[c]);
  sys->ren.drw.hln(ctx->ren, b->y.max - 2, b->x.min + 1, b->x.max - 2);
  sys->ren.drw.vln(ctx->ren, b->x.max - 2, b->y.min + 2, b->y.max - 2);
}
static enum gui_chk_state
gui_chk(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
        enum gui_chk_state chkd) {
  assert(ctx);
  assert(pan);
  assert(parent);

  pan->box = gui_box_mid_ext(&pan->box, ctx->cfg.item, ctx->cfg.item);
  gui_panel_begin(ctx, pan, parent);
  switch (ctx->pass) {
    case GUI_RENDER: {
      if (pan->state != GUI_HIDDEN) {
        gui__chk_drw(ctx, pan);
        gui__chk_cur(ctx, pan, chkd);
      }
    } break;
  }
  gui_panel_end(ctx, pan, parent);

  struct gui_input in = {0};
  gui_input(&in, ctx, pan, GUI_BTN_LEFT);
  if (in.mouse.btn.left.clk) {
    chkd = gui__chk_tog(chkd);
  }
  return chkd;
}
static int
gui_chk_box(struct gui_ctx *ctx, struct gui_panel *pan,
            struct gui_panel *parent, struct gui_box_cut *cut,
            struct str txt, enum gui_chk_state *chkd) {
  assert(ctx);
  assert(pan);
  assert(chkd);
  assert(parent);

  int tw = gui_txt_width(ctx, txt);
  int w = tw + ctx->cfg.item + ctx->cfg.gap[0];
  pan->box = gui_cut_box(cut, w);

  gui_panel_begin(ctx, pan, parent);
  {
    struct gui_panel chk = {.box = pan->box};
    gui_chk(ctx, &chk, pan, *chkd);

    struct gui_panel lbl = {.box = pan->box};
    lbl.box.x = gui_min_max(chk.box.x.max + ctx->cfg.gap[0], pan->box.x.max);
    gui_txt(ctx, &lbl, pan, txt, 0);
    if (ctx->pass == GUI_RENDER && pan->state == GUI_FOCUSED) {
      gui_focus_drw(ctx, &pan->box, 0);
    }
  }
  gui_panel_hot(ctx, pan, parent);
  gui_panel_end(ctx, pan, parent);

  struct gui_input in = {0};
  gui_input(&in, ctx, pan, GUI_BTN_LEFT);
  int act = pan->state == GUI_FOCUSED && bit_tst_clr(ctx->keys, GUI_KEY_ACT);
  if (act || in.mouse.btn.left.clk) {
    *chkd = gui__chk_tog(*chkd);
  }
  return 0;
}
static int
gui_chk_boxi(struct gui_ctx *ctx, struct gui_panel *pan,
             struct gui_panel *parent, struct gui_box_cut *cut,
             struct str txt, int *chkd) {
  assert(ctx);
  assert(pan);
  assert(parent);

  enum gui_chk_state st = gui_chk_box_state(*chkd);
  int ret = gui_chk_box(ctx, pan, parent, cut, txt, &st);
  *chkd = gui_chk_box_bool(st);
  return ret;
}

/* ---------------------------------------------------------------------------
 *                                  Toggle
 * ---------------------------------------------------------------------------
 */
static void
gui__tog_drw(struct gui_ctx *ctx, struct gui_panel *pan, int act) {
  assert(ctx);
  assert(pan);

  int cur_siz = gui_img_def[GUI_IMG_CHECK].w;
  struct sys *sys = ctx->sys;
  const struct gui_box *b = &pan->box;
  gui__chk_drw(ctx, pan);

  struct gui_box tog;
  tog.x = gui_shrink(&b->x, 2);
  tog.y = gui_shrink(&b->y, 2);

  struct gui_box c = tog;
  int min = !act ? tog.x.min : tog.x.mid;
  int max = !act ? tog.x.mid : tog.x.max;
  c.x = gui_min_max(min, max);
  gui_panel_drw(ctx, &c);

  if (act) {
    struct gui_box cur = {0};
    cur.x = gui_mid_ext(tog.x.mid - (tog.x.ext >> 2), cur_siz);
    cur.y = gui_mid_ext(tog.y.mid, cur_siz);
    gui__img(ctx, sys, cur.x.min, cur.y.min, GUI_IMG_CHECK);
  } else {
    struct gui_box cur = {0};
    cur.x = gui_mid_ext(tog.x.mid + (tog.x.ext >> 2), cur_siz);
    cur.y = gui_mid_ext(tog.y.mid, cur_siz);
    gui__img(ctx, sys, cur.x.min, cur.y.min, GUI_IMG_NO);
  }
}
static int
gui_tog(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
        int *act) {
  assert(act);
  assert(ctx);
  assert(pan);
  assert(parent);

  int w = min(pan->box.x.ext, ctx->cfg.item << 1);
  pan->box.y = gui_mid_ext(pan->box.y.mid, ctx->cfg.item);
  pan->box.x = gui_min_ext(pan->box.x.min, w);

  gui_panel_begin(ctx, pan, parent);
  if (ctx->pass == GUI_RENDER && pan->state != GUI_HIDDEN) {
    gui__tog_drw(ctx, pan, *act);
  }
  gui_panel_end(ctx, pan, parent);

  struct gui_input in = {0};
  gui_input(&in, ctx, pan, GUI_BTN_LEFT);
  if (in.mouse.btn.left.clk) {
    *act = !*act;
    return 1;
  }
  return 0;
}
static int
gui_tog_box(struct gui_ctx *ctx, struct gui_panel *pan,
            struct gui_panel *parent, struct gui_box_cut *cut,
            struct str txt, int *is_act) {
  assert(ctx);
  assert(pan);
  assert(is_act);
  assert(parent);

  int tw = gui_txt_width(ctx, txt);
  int w = tw + ctx->cfg.gap[0] + (ctx->cfg.item << 1);
  pan->box = gui_cut_box(cut, w);
  gui_panel_begin(ctx, pan, parent);
  {
    int active = *is_act;
    struct gui_panel tog = {.box = pan->box};
    gui_tog(ctx, &tog, pan, &active);

    struct gui_panel lbl = {.box = pan->box};
    lbl.box.x = gui_min_max(tog.box.x.max + ctx->cfg.gap[0], pan->box.x.max);
    gui_txt(ctx, &lbl, pan, txt, 0);
    if (ctx->pass == GUI_RENDER && pan->state == GUI_FOCUSED) {
      gui_focus_drw(ctx, &pan->box, 0);
    }
  }
  gui_panel_hot(ctx, pan, parent);
  gui_panel_end(ctx, pan, parent);

  struct gui_input in = {0};
  gui_input(&in, ctx, pan, GUI_BTN_LEFT);
  int act = pan->state == GUI_FOCUSED && bit_tst_clr(ctx->keys, GUI_KEY_ACT);
  if (act || in.mouse.btn.left.clk) {
    *is_act = !*is_act;
  }
  return 0;
}

/* ---------------------------------------------------------------------------
 *                                  Scroll
 * ---------------------------------------------------------------------------
 */
static void
gui__scrl_cur_lay(int *off, struct gui_panel *cur, const struct gui_scrl *s,
                  struct gui_panel *pan) {
  assert(s);
  assert(off);
  assert(cur);
  assert(pan);

  const struct gui_box *p = &pan->box;
  int w = ceili((s->size[0] / s->total[0]) * (double)p->x.ext);
  if (w > 0 && w < s->min[0]) {
    w = off[0] = s->min[0];
  }
  int h = ceili((s->size[1] / s->total[1]) * (double)p->y.ext);
  if (h > 0 && h < s->min[1]) {
    h = off[1] = s->min[1];
  }
  double ratio[2];
  ratio[0] = s->off[0] / s->total[0];
  ratio[1] = s->off[1] / s->total[1];

  double space[2];
  space[0] = (double)(p->x.ext - off[0]);
  space[1] = (double)(p->y.ext - off[1]);

  int x = floori((double)p->x.min + ratio[0] * space[0]);
  int y = floori((double)p->y.min + ratio[1] * space[1]);
  cur->box = gui_box(x, y, w, h);
}
static void
gui__scrl_cur_drag(double *off, const struct gui_ctx *ctx,
                   const struct gui_scrl *s, struct gui_box *b,
                   const struct gui_input *in) {
  assert(off);
  assert(ctx);
  assert(in);
  assert(s);
  assert(b);

  double dpx = cast(double, in->mouse.btn.left.drag_pos[0]);
  double dpy = cast(double, in->mouse.btn.left.drag_pos[1]);
  double dx = cast(double, in->mouse.pos[0]) - dpx;
  double dy = cast(double, in->mouse.pos[1]) - dpy;
  double rx = dx / cast(double, b->x.ext);
  double ry = dy / cast(double, b->y.ext);
  double off_x = ctx->drag_state[0] + rx * s->total[0];
  double off_y = ctx->drag_state[1] + ry * s->total[1];

  off[0] = clamp(0, off_x, s->total[0] - s->size[0]);
  off[1] = clamp(0, off_y, s->total[1] - s->size[1]);
}
static void
gui__scrl_cur(struct gui_ctx *ctx, struct gui_scrl *s, struct gui_panel *pan,
              struct gui_panel *parent, struct gui_input *in) {
  assert(s);
  assert(in);
  assert(ctx);
  assert(pan);
  assert(parent);

  int off[2] = {0, 0};
  gui__scrl_cur_lay(off, pan, s, parent);
  gui_panel_begin(ctx, pan, parent);
  {
    /* draw */
    if (ctx->pass == GUI_RENDER && pan->state != GUI_DISABLED) {
      gui_panel_drw(ctx, &pan->box);
    }
    /* input */
    gui_input(in, ctx, pan, GUI_BTN_LEFT);
    if (in->mouse.btn.left.drag_begin) {
      ctx->drag_state[0] = s->off[0];
      ctx->drag_state[1] = s->off[1];
    }
    if (in->mouse.btn.left.dragged) {
      gui__scrl_cur_drag(s->off, ctx, s, &parent->box, in);
      gui__scrl_cur_lay(off, pan, s, parent);
      s->scrolled = 1;
    }
  }
  gui_panel_end(ctx, pan, parent);
}
static void
gui__scrl_chk(struct gui_scrl *s) {
  assert(s);
  s->size[0] = max(s->size[0], 1);
  s->size[1] = max(s->size[1], 1);

  s->total[0] = max(s->total[0], 1);
  s->total[1] = max(s->total[1], 1);
  s->total[0] = max(s->total[0], s->size[0]);
  s->total[1] = max(s->total[1], s->size[1]);

  s->off[0] = clamp(0, s->off[0], s->total[0] - s->size[0]);
  s->off[1] = clamp(0, s->off[1], s->total[1] - s->size[1]);
}
static void
gui__scrl_jmp(double *off, const struct gui_scrl *s,
              const struct gui_panel *cur, int px, int py) {
  assert(s);
  assert(off);
  assert(cur);

  double mpx = cast(double, px);
  double mpy = cast(double, py);
  double dx = mpx - s->box.x.min - (cur->box.x.ext >> 1);
  double dy = mpy - s->box.y.min - (cur->box.y.ext >> 1);
  double rx = dx / cast(double, s->box.x.ext);
  double ry = dy / cast(double, s->box.y.ext);

  off[0] = clamp(0, rx * s->total[0], s->total[0] - s->size[0]);
  off[1] = clamp(0, ry * s->total[1], s->total[1] - s->size[1]);
}
static void
gui_scrl(struct gui_ctx *ctx, struct gui_scrl *s, struct gui_panel *parent) {
  assert(s);
  assert(ctx);
  assert(parent);
  struct gui_panel cur = {0};
  struct gui_input cur_in = {0};

  s->pan.box = s->box;
  gui_panel_begin(ctx, &s->pan, parent);
  {
    gui__scrl_chk(s);
    if (ctx->pass == GUI_RENDER && s->pan.state != GUI_HIDDEN) {
      /* draw background */
      const struct gui_box *b = &s->pan.box;
      struct color c = col_get(ctx->cfg.col[GUI_COL_CONTENT]);
      c.a = 0x7f;

      struct sys *sys = ctx->sys;
      sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_BG]);
      sys->ren.drw.box(ctx->ren, gui_unbox(b));
      sys->ren.drw.col(ctx->ren, col_paq(&c));
      sys->ren.drw.box(ctx->ren, gui_unbox(b));
    }
    gui__scrl_cur(ctx, s, &cur, &s->pan, &cur_in);
  }
  gui_panel_end(ctx, &s->pan, parent);

  struct gui_input in = {0};
  gui_input(&in, ctx, &s->pan, GUI_BTN_LEFT);
  if (in.mouse.btn.left.pressed && !cur_in.mouse.btn.left.pressed) {
    const struct sys *sys = ctx->sys;
    if (!(sys->keymod & SYS_KEYMOD_SHIFT)) {
      gui__scrl_jmp(s->off, s, &cur, in.mouse.pos[0], in.mouse.pos[1]);
      s->scrolled = 1;
    } else {
      /* handle page up/down click shortcut */
      double pgx = (in.mouse.pos[0] < cur.box.x.min) ? -s->size[0] : s->size[0];
      double pgy = (in.mouse.pos[1] < cur.box.y.min) ? -s->size[1] : s->size[1];

      s->off[0] = clamp(0, s->off[0] + pgx, s->total[0] - s->size[0]);
      s->off[1] = clamp(0, s->off[1] + pgy, s->total[1] - s->size[1]);
      s->scrolled = 1;
    }
  }
}

/* ---------------------------------------------------------------------------
 *                                  Arrow
 * ---------------------------------------------------------------------------
 */
enum gui_direction {
  GUI_NORTH = 0x01,
  GUI_WEST = 0x02,
  GUI_SOUTH = 0x04,
  GUI_EAST = 0x08
};
enum gui_dir {
  GUI_DIR_HORIZONTAL = GUI_NORTH | GUI_SOUTH,
  GUI_DIR_VERTICAL = GUI_WEST | GUI_EAST,
};
static void
gui_arrow(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
          enum gui_direction orient) {
  assert(ctx);
  assert(pan);
  assert(parent);

  gui_panel_begin(ctx, pan, parent);
  if (ctx->pass == GUI_RENDER && pan->state != GUI_HIDDEN) {
    struct sys *sys = ctx->sys;
    int c = pan->state == GUI_DISABLED ? GUI_COL_TXT_DISABLED : GUI_COL_TXT;
    sys->ren.drw.col(ctx->ren, ctx->cfg.col[c]);
    sys->ren.drw.line_style(ctx->ren, 1);
    switch (orient) {
      case GUI_NORTH: {
        for_cnt(i, pan->box.y.ext) {
          sys->ren.drw.hln(ctx->ren, pan->box.y.max - 1 - i, pan->box.x.min + i - 1,
                    pan->box.x.max - i - 1);
        }
      } break;
      case GUI_WEST: {
        for_cnt(i, pan->box.x.ext) {
          sys->ren.drw.vln(ctx->ren, pan->box.x.max - 1 - i, pan->box.y.min + i - 1,
                    pan->box.y.min + pan->box.y.ext - i);
        }
      } break;
      case GUI_SOUTH: {
        for_cnt(i, pan->box.y.ext) {
          sys->ren.drw.hln(ctx->ren, pan->box.y.min + i, pan->box.x.min + i - 1,
                    pan->box.x.max - i - 1);
        }
      } break;
      case GUI_EAST: {
        for_cnt(i, pan->box.x.ext) {
          sys->ren.drw.vln(ctx->ren, pan->box.x.min + i, pan->box.y.min + i - 1,
                    pan->box.y.min + pan->box.y.ext - i - 1);
        }
      } break;
    }
  }
  gui_panel_end(ctx, pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  Separator
 * ---------------------------------------------------------------------------
 */
static int
gui_sep(struct gui_ctx *ctx, struct gui_btn *btn, struct gui_panel *parent,
        enum gui_orient dir, int *val) {
  assert(val);
  assert(ctx);
  assert(parent);

  gui_btn_begin(ctx, btn, parent);
  gui_btn_end(ctx, btn, parent);
  if (btn->pan.is_hot || btn->in.mouse.btn.left.dragged) {
    switch (dir) {
      case GUI_HORIZONTAL:
        ctx->sys->cursor = SYS_CUR_SIZE_WE;
        break;
      case GUI_VERTICAL:
        ctx->sys->cursor = SYS_CUR_SIZE_NS;
        break;
    }
  }
  if (btn->in.mouse.btn.left.drag_begin) {
    ctx->drag_state[0] = cast(double, *val);
  }
  if (btn->in.mouse.btn.left.dragged) {
    int d = 0;
    switch (dir) {
      case GUI_HORIZONTAL:
        d = ctx->sys->mouse.pos[0] - btn->in.mouse.btn.left.drag_pos[0];
        break;
      case GUI_VERTICAL:
        d = ctx->sys->mouse.pos[1] - btn->in.mouse.btn.left.drag_pos[1];
        break;
    }
    *val = max(0, cast(int, ctx->drag_state[0]) + d);
    return 1;
  }
  return 0;
}

/* ---------------------------------------------------------------------------
 *                                  Scrollbar
 * ---------------------------------------------------------------------------
 */
static int
gui__scrl_btn(struct gui_ctx *ctx, struct gui_btn *btn,
              struct gui_panel *parent, enum gui_direction d) {
  assert(ctx);
  assert(btn);
  assert(parent);
  btn->pan.focusable = 0;
  gui_btn_begin(ctx, btn, parent);
  {
    int w = (d == GUI_SOUTH || d == GUI_NORTH) ? 5 : 3;
    int h = (d == GUI_EAST || d == GUI_WEST) ? 5 : 3;

    struct gui_panel sym = {0};
    sym.box = gui_box_mid_ext(&btn->pan.box, w, h);
    gui_arrow(ctx, &sym, &btn->pan, d);
  }
  gui_btn_end(ctx, btn, parent);
  return btn->clk;
}
static void
gui_hscrl(struct gui_ctx *ctx, struct gui_scrl_bar *s,
          struct gui_panel *parent) {
  assert(s);
  assert(ctx);
  assert(parent);

  s->pan.box = s->box;
  gui_panel_begin(ctx, &s->pan, parent);
  {
    s->step = s->step <= 0 ? s->size * 0.1f : s->step;
    s->scrolled = 0;

    /* decrement button */
    s->btn_dec.box.x = gui_min_ext(s->box.x.min, s->box.y.ext);
    s->btn_dec.box.y = gui_max_ext(s->box.y.max, s->box.y.ext);
    gui__scrl_btn(ctx, &s->btn_dec, &s->pan, GUI_WEST);
    if (s->btn_dec.pan.is_hot) ctx->sys->cursor = SYS_CUR_LEFT_ARROW;
    if (s->btn_dec.pressed) {
      s->off -= s->step;
      s->scrolled = 1;
    }
    /* increment button */
    s->btn_inc.box.x = gui_max_ext(s->box.x.max, s->box.y.ext);
    s->btn_inc.box.y = gui_max_ext(s->box.y.max, s->box.y.ext);
    gui__scrl_btn(ctx, &s->btn_inc, &s->pan, GUI_EAST);
    if (s->btn_inc.pan.is_hot) ctx->sys->cursor = SYS_CUR_RIGHT_ARROW;
    if (s->btn_inc.pressed) {
      s->off += s->step;
      s->scrolled = 1;
    }
    /* scroll */
    struct gui_scrl scrl = {0};
    scrl.total[1] = scrl.size[1] = 1;
    scrl.total[0] = s->total;
    scrl.size[0] = s->size;
    scrl.min[0] = s->min_size;
    scrl.off[0] = s->off;
    scrl.off[1] = 0;

    scrl.box.y = gui_max_ext(s->box.y.max, s->box.y.ext);
    scrl.box.x = gui_min_max(s->btn_dec.box.x.max, s->btn_inc.box.x.min);
    gui_scrl(ctx, &scrl, &s->pan);

    /* mouse cursor */
    struct gui_input in = {0};
    gui_input(&in, ctx, &scrl.pan, GUI_BTN_LEFT);
    if (scrl.pan.is_hot || in.mouse.btn.left.grabbed) {
      ctx->sys->cursor = SYS_CUR_SIZE_WE;
    }
    s->scrolled = s->scrolled || scrl.scrolled;
    s->off = scrl.off[0];
  }
  gui_panel_end(ctx, &s->pan, parent);
}
static void
gui_vscrl(struct gui_ctx *ctx, struct gui_scrl_bar *s,
          struct gui_panel *parent) {
  assert(s);
  assert(ctx);
  assert(parent);

  s->pan.box = s->box;
  gui_panel_begin(ctx, &s->pan, parent);
  {
    s->step = s->step <= 0 ? s->size * 0.1f : s->step;
    s->scrolled = 0;

    /* decrement button */
    s->btn_dec.box.x = gui_min_ext(s->box.x.min, s->box.x.ext);
    s->btn_dec.box.y = gui_min_ext(s->box.y.min, s->box.x.ext);
    gui__scrl_btn(ctx, &s->btn_dec, &s->pan, GUI_NORTH);
    if (s->btn_dec.pan.is_hot) ctx->sys->cursor = SYS_CUR_UP_ARROW;
    if (s->btn_dec.pressed) {
      s->off -= s->step;
      s->scrolled = 1;
    }
    /* increment button */
    s->btn_inc.box.x = gui_min_ext(s->box.x.min, s->box.x.ext);
    s->btn_inc.box.y = gui_max_ext(s->box.y.max, s->box.x.ext);
    gui__scrl_btn(ctx, &s->btn_inc, &s->pan, GUI_SOUTH);
    if (s->btn_inc.pressed) {
      s->off += s->step;
      s->scrolled = 1;
    }
    /* scroll */
    struct gui_scrl scrl = {0};
    scrl.total[0] = scrl.size[0] = 1;
    scrl.total[1] = s->total;
    scrl.size[1] = s->size;
    scrl.min[1] = s->min_size;
    scrl.off[1] = s->off;
    scrl.off[0] = 0;

    scrl.box.x = s->btn_dec.box.x;
    scrl.box.y = gui_min_max(s->btn_dec.box.y.max, s->btn_inc.box.y.min);
    gui_scrl(ctx, &scrl, &s->pan);
    if (s->btn_inc.pan.is_hot) {
      ctx->sys->cursor = SYS_CUR_DOWN_ARROW;
    }
    /* mouse cursor */
    struct gui_input in = {0};
    gui_input(&in, ctx, &scrl.pan, GUI_BTN_LEFT);
    if (scrl.pan.is_hot || in.mouse.btn.left.grabbed) {
      ctx->sys->cursor = SYS_CUR_SIZE_NS;
    }
    s->scrolled = s->scrolled || scrl.scrolled;
    s->off = scrl.off[1];
  }
  gui_panel_end(ctx, &s->pan, parent);
}

/* ---------------------------------------------------------------------------
 *                              Text Editor
 * ---------------------------------------------------------------------------
 */
static int
gui_txt_ed_has_sel(struct gui_txt_ed *edt) {
  return edt->sel[0] != edt->sel[1];
}
static void
gui_txt_ed_reset(struct gui_txt_ed *edt) {
  assert(edt);
  edt->undo.undo_pnt = 0;
  edt->undo.undo_char_pnt = 0;
  edt->undo.redo_pnt = GUI_EDT_UNDO_CNT;
  edt->undo.redo_char_pnt = GUI_EDT_UNDO_CHAR_CNT;

  edt->mode = GUI_EDT_MODE_INSERT;
  edt->sel[0] = edt->sel[1] = 0;
  edt->init = 1;
  edt->off = 0;
  edt->cur = 0;
}
static void
gui_txt_ed_clear(struct gui_txt_ed *edt, char **buf) {
  assert(edt);
  gui_txt_ed_reset(edt);
  dyn_clr(*buf);
}
static void
gui_txt_ed_init(struct gui_txt_ed *edt) {
  assert(edt);
  gui_txt_ed_reset(edt);
}
static void
gui_txt_ed_redo_flush(struct gui_txt_ed_undo *s) {
  assert(s);
  s->redo_char_pnt = GUI_EDT_UNDO_CHAR_CNT;
  s->redo_pnt = GUI_EDT_UNDO_CNT;
}
static void
gui_txt_ed_undo_discard(struct gui_txt_ed_undo *s) {
  assert(s);
  if (s->undo_pnt <= 0) {
    return;
  }
  /* discard oldest entry in undo list */
  if (s->stk[0].char_at >= 0) {
    /* if the 0th undo state has characters, clean them up  */
    int n = s->stk[0].in_len;
    s->undo_char_pnt = (short)(s->undo_char_pnt - n);
    memcpy(s->buf, s->buf + n,
           cast(size_t, s->undo_char_pnt) + sizeof(s->buf[0]));
    for_cnt(i, s->undo_pnt) {
      if (s->stk[i].char_at < 0) continue;
      s->stk[i].char_at = cast(short, s->stk[i].char_at - n);
    }
  }
  s->undo_pnt--;
  memcpy(s->stk, s->stk + 1, cast(size_t, s->undo_pnt) * sizeof(s->buf[0]));
}
static void
gui_txt_ed_redo_discard(struct gui_txt_ed_undo *s) {
  assert(s);
  int k = GUI_EDT_UNDO_CNT - 1;
  if (s->redo_pnt > k) {
    return;
  }
  /* discard the oldest entry in the redo list--it's bad if this
  ever happens, but because undo & redo have to store the actual
  characters in different cases, the redo character buffer can
  fill up even though the undo buffer didn't */
  if (s->stk[k].char_at >= 0) {
    /* if the k'th undo state has chars, clean up */
    int n = s->stk[k].in_len;
    s->redo_char_pnt = cast(short, s->redo_char_pnt + n);
    int cnt = GUI_EDT_UNDO_CHAR_CNT - s->redo_char_pnt;
    memcpy(s->buf + s->redo_char_pnt, s->buf + s->redo_char_pnt - n,
           cast(size_t, cnt) * sizeof(char));

    for (int i = s->redo_pnt; i < k; ++i) {
      if (s->stk[i].char_at < 0) continue;
      s->stk[i].char_at = cast(short, s->stk[i].char_at + n);
    }
  }
  s->redo_pnt++;
  int cnt = GUI_EDT_UNDO_CNT - s->redo_pnt;
  if (cnt) {
    memcpy(s->stk + s->redo_pnt - 1, s->stk + s->redo_pnt,
           cast(size_t, cnt) * sizeof(s->stk[0]));
  }
}
static struct gui_txt_ed_undo_entry *
gui_txt_ed_undo_entry(struct gui_txt_ed_undo *s, int char_cnt) {
  assert(s);
  gui_txt_ed_redo_flush(s); /* discard redo on new undo record */
  if (s->undo_pnt >= GUI_EDT_UNDO_CNT) {
    gui_txt_ed_undo_discard(s); /* freeup record if needed */
  }
  /* no undo if does not fit into buffer */
  if (char_cnt > GUI_EDT_UNDO_CHAR_CNT) {
    s->undo_char_pnt = 0;
    s->undo_pnt = 0;
    return 0;
  }
  while (s->undo_char_pnt + char_cnt > GUI_EDT_UNDO_CHAR_CNT) {
    gui_txt_ed_undo_discard(s);
  }
  return s->stk + s->undo_pnt++;
}
static char *
gui_txt_ed_undo_make(struct gui_txt_ed_undo *s, int pos, int in_len,
                     int del_len) {
  assert(s);
  struct gui_txt_ed_undo_entry *rdo = gui_txt_ed_undo_entry(s, in_len);
  if (rdo == 0) {
    return 0;
  }
  rdo->where = pos;
  rdo->in_len = cast(short, in_len);
  rdo->del_len = cast(short, del_len);
  if (in_len == 0) {
    rdo->char_at = -1;
    return 0;
  }
  rdo->char_at = s->undo_char_pnt;
  s->undo_char_pnt = cast(short, s->undo_char_pnt + in_len);
  return s->buf + rdo->char_at;
}
static void
gui_txt_ed_undo(struct gui_txt_ed *edt, struct sys *sys, char **buf) {
  assert(edt);
  struct gui_txt_ed_undo *s = &edt->undo;
  if (s->undo_pnt == 0) {
    return;
  }
  struct gui_txt_ed_undo_entry udo = s->stk[s->undo_pnt - 1];
  struct gui_txt_ed_undo_entry *rdo = s->stk + s->undo_pnt - 1;
  rdo->in_len = udo.del_len;
  rdo->del_len = udo.in_len;
  rdo->where = udo.where;
  rdo->char_at = -1;
  /* if the undo record says to delete characters, then the redo record will
     need to re-insert the characters that get deleted, so we need to store
     them. There are three cases:
         - there's enough room to store the characters
         - characters stored for *redoing* don't leave room for redo
         - characters stored for *undoing* don't leave room for redo
     if the last is true, we have to bail */
  if (udo.del_len) {
    if (s->undo_char_pnt + udo.del_len < GUI_EDT_UNDO_CHAR_CNT) {
      while (s->undo_char_pnt + udo.del_len > s->redo_char_pnt) {
        gui_txt_ed_redo_discard(s);
        if (s->redo_pnt == GUI_EDT_UNDO_CHAR_CNT) {
          return;
        }
      }
      rdo = s->stk + s->redo_pnt - 1;
      rdo->char_at = cast(short, s->redo_char_pnt - udo.del_len);
      s->redo_char_pnt = cast(short, s->redo_char_pnt - udo.del_len);
      for_cnt(i, udo.del_len) {
        s->buf[rdo->char_at + i] = (*buf)[udo.where + i];
      }
    } else {
      rdo->in_len = 0;
    }
    dyn_cut(*buf, udo.where, udo.del_len);
  }
  if (udo.in_len) {
    dyn_put(*buf, sys, udo.where, s->buf + udo.char_at, udo.in_len);
    s->undo_char_pnt = cast(short, s->undo_char_pnt - udo.in_len);
  }
  edt->cur = utf_len(strp(*buf, *buf + udo.where + udo.in_len));

  s->undo_pnt--;
  s->redo_pnt--;
}
static void
gui_txt_ed_redo(struct gui_txt_ed *edt, struct sys *sys, char **buf) {
  assert(edt);
  struct gui_txt_ed_undo *s = &edt->undo;
  if (s->redo_pnt == GUI_EDT_UNDO_CNT) {
    return;
  }
  /* we KNOW there must be room for the undo record,
   * because the redo record was derived from an undo record */
  struct gui_txt_ed_undo_entry rdo = s->stk[s->undo_pnt];
  struct gui_txt_ed_undo_entry *udo = s->stk + s->undo_pnt;
  udo->del_len = rdo.in_len;
  udo->in_len = rdo.del_len;
  udo->where = rdo.where;
  udo->char_at = -1;

  if (rdo.del_len) {
    if (s->undo_char_pnt + udo->in_len <= s->redo_char_pnt) {
      udo->char_at = s->undo_char_pnt;
      s->undo_char_pnt = cast(short, s->undo_char_pnt + udo->in_len);
      /* now save the characters */
      for_cnt(i, udo->in_len)
        s->buf[udo->char_at + i] = (*buf)[udo->where + i];
    } else {
      udo->in_len = udo->del_len = 0;
    }
    dyn_cut(*buf, rdo.where, rdo.del_len);
  }
  if (rdo.in_len) {
    dyn_put(*buf, sys, rdo.where, s->buf + rdo.char_at, rdo.in_len);
  }
  edt->cur = utf_len(strp(*buf, *buf + rdo.where + rdo.in_len));

  s->undo_pnt++;
  s->redo_pnt++;
}
static void
gui_txt_ed_undo_in(struct gui_txt_ed *edt, int where, int len) {
  assert(edt);
  gui_txt_ed_undo_make(&edt->undo, where, 0, len);
}
static void
gui_txt_ed_undo_del(struct gui_txt_ed *edt, char **buf, int where, int len) {
  assert(edt);
  char *p = gui_txt_ed_undo_make(&edt->undo, where, len, 0);
  if (p) {
    memcpy(p, *buf + where, cast(size_t, len));
  }
}
static void
gui_txt_ed_undo_repl(struct gui_txt_ed *edt, char **buf, int where,
                     int old_len, int new_len) {
  assert(edt);
  char *p = gui_txt_ed_undo_make(&edt->undo, where, old_len, new_len);
  if (p) {
    memcpy(p, *buf + where, cast(size_t, old_len));
  }
}
static int
gui_txt_ed_ext(char *buf, int line_begin, int char_idx, struct res *r,
               struct res_fnt *fnt) {
  assert(buf);
  assert(fnt);
  assert(r);

  int ext[2];
  struct str view = utf_at(0, dyn_str(buf), line_begin + char_idx);
  res.fnt_ext(ext, r, fnt, view);
  return ext[0];
}
struct gui_txt_row {
  int x[2], y[2];
  int baseline_y_dt;
  int char_cnt;
};
static void
gui_txt_ed_lay_row(struct gui_txt_row *row, char *buf, int line_begin,
                   int row_h, struct res *r, struct res_fnt *fnt) {
  assert(buf);
  assert(fnt);
  assert(row);

  struct str begin = utf_at(0, dyn_str(buf), line_begin);
  struct str txt = strp(begin.str, dyn_end(buf));
  struct str end  = begin;

  int cnt = 0;
  unsigned rune = 0;
  for_utf(&rune, it, _, txt) {
    end = it;
    if (rune == '\n') {
      break;
    }
    cnt++;
  }
  int ext[2];
  struct str ln = strp(begin.str, end.end);
  res.fnt_ext(ext, r, fnt, ln);

  row->char_cnt = cnt;
  row->baseline_y_dt = row_h;
  row->x[0] = row->y[0] = 0;
  row->x[1] = ext[0];
  row->y[1] = row_h;
}
static int
gui_txt_ed_loc_coord(char *buf, int x, int y, int row_h, struct res *r,
                     struct res_fnt *fnt) {
  assert(buf);
  assert(fnt);
  assert(r);

  int i = 0;
  int base_y = 0;
  struct gui_txt_row row = {0};
  while (i < dyn_cnt(buf)) {
    gui_txt_ed_lay_row(&row, buf, i, row_h, r, fnt);
    if (row.char_cnt <= 0) {
      return dyn_cnt(buf);
    } else if (i == 0 && y < base_y + row.y[0]) {
      return 0;
    } else if (y < base_y + row.y[1]) {
      break;
    }
    base_y += row.baseline_y_dt;
    i += row.char_cnt;
  }
  if (i >= dyn_cnt(buf)) {
    return dyn_cnt(buf);
  } else if (x < row.x[0]) {
    return i;
  } else if (x < row.x[1]) {
    int k = i, prev_x = row.x[0];
    for (i = 0; i < row.char_cnt; ++i) {
      int w = gui_txt_ed_ext(buf, k, i, r, fnt);
      if (x < prev_x + w) {
        if (x < prev_x + w / 2) {
          return k + i;
        } else {
          return k + i + 1;
        }
      } else {
        prev_x += w;
      }
    }
  }
  unsigned rune = 0;
  utf_at(&rune, dyn_str(buf), i + row.char_cnt - 1);
  if (rune == '\n') {
    return i + row.char_cnt - 1;
  } else {
    return i + row.char_cnt;
  }
}
static void
gui_txt_ed_clk(struct gui_txt_ed *edt, char *buf, int x, int y, int row_h,
               struct res *r, struct res_fnt *fnt) {
  assert(edt);
  assert(fnt);
  edt->cur = gui_txt_ed_loc_coord(buf, x, y, row_h, r, fnt);
  edt->sel[0] = edt->sel[1] = edt->cur;
}
static void
gui_txt_ed_drag(struct gui_txt_ed *edt, char *buf, int x, int y, int row_h,
                struct res *r, struct res_fnt *fnt) {
  assert(edt);
  assert(fnt);
  int p = gui_txt_ed_loc_coord(buf, x, y, row_h, r, fnt);
  if (edt->sel[0] == edt->sel[1]) {
    edt->sel[0] = edt->cur;
  }
  edt->cur = edt->sel[1] = p;
}
static void
gui_txt_ed_clamp(struct gui_txt_ed *edt, char *buf) {
  assert(edt);
  int n = utf_len(dyn_str(buf));
  if (gui_txt_ed_has_sel(edt)) {
    edt->sel[0] = (edt->sel[0] > n) ? n : edt->sel[0];
    edt->sel[1] = (edt->sel[1] > n) ? n : edt->sel[1];
    edt->cur = (edt->sel[0] == edt->sel[1]) ? edt->sel[0] : edt->cur;
  }
  if (edt->cur > n) {
    edt->cur = n;
  }
}
static void
gui__txt_edt_del(struct gui_txt_ed *edt, char **buf, int where, int len) {
  assert(edt);
  gui_txt_ed_undo_del(edt, buf, where, len);
  dyn_cut(*buf, where, len);
  gui_txt_ed_clamp(edt, *buf);
}
static void
gui_txt_ed_del(struct gui_txt_ed *edt, char **buf, int where, int len) {
  assert(edt);
  struct str begin = utf_at(0, dyn_str(*buf), where);
  struct str end = utf_at(0, strp(begin.str, dyn_end(*buf)), len);
  if (!begin.len) {
    return;
  }
  int w = cast(int, begin.str - *buf);
  int n = cast(int, end.str - begin.str);
  gui__txt_edt_del(edt, buf, w, n);
}
static void
gui_txt_ed_del_sel(struct gui_txt_ed *edt, char **buf) {
  assert(edt);
  gui_txt_ed_clamp(edt, *buf);
  if (!gui_txt_ed_has_sel(edt)) {
    return;
  }
  if (edt->sel[0] < edt->sel[1]) {
    gui_txt_ed_del(edt, buf, edt->sel[0], edt->sel[1] - edt->sel[0]);
    edt->sel[1] = edt->cur = edt->sel[0];
  } else {
    gui_txt_ed_del(edt, buf, edt->sel[1], edt->sel[0] - edt->sel[1]);
    edt->sel[0] = edt->cur = edt->sel[1];
  }
}
static void
gui_txt_ed_sort_sel(struct gui_txt_ed *edt) {
  assert(edt);
  if (edt->sel[1] >= edt->sel[0]) {
    return;
  }
  int tmp = edt->sel[1];
  edt->sel[1] = edt->sel[0];
  edt->sel[0] = tmp;
}
static void
gui_txt_ed_move_sel_first(struct gui_txt_ed *edt) {
  assert(edt);
  if (!gui_txt_ed_has_sel(edt)) {
    return;
  }
  gui_txt_ed_sort_sel(edt);
  edt->cur = edt->sel[0];
  edt->sel[1] = edt->sel[0];
}
static void
gui_txt_ed_move_sel_last(struct gui_txt_ed *edt, char *buf) {
  assert(edt);
  if (!gui_txt_ed_has_sel(edt)) {
    return;
  }
  gui_txt_ed_sort_sel(edt);
  gui_txt_ed_clamp(edt, buf);

  edt->cur = edt->sel[1];
  edt->sel[0] = edt->sel[1];
}
static inline int
gui_rune_is_word_boundary(long c) {
  switch (c) {
    default:
      return 0;
    case '\t':
    case ',':
    case ' ':
    case '.':
    case ';':
    case '(':
    case ')':
    case '{':
    case '}':
    case '[':
    case ']':
    case '<':
    case '>':
    case '|':
    case '/':
    case '?':
    case '#':
    case '~':
    case '@':
    case '\'':
    case '=':
    case '+':
    case '-':
    case '_':
    case '*':
    case '&':
    case '^':
    case '%':
    case '$':
    case '\"':
    case '!':
    case '`':
    case '\\':
    case ':':
      return 1;
  }
}
static int
gui_txt_ed_move_to_prev_word(struct gui_txt_ed *edt, char *buf) {
  unsigned rune = 0;
  assert(edt);
  if (!edt->cur) {
    return 0;
  }
  /* skip all trailing word boundary runes */
  int c = edt->cur - 1;
  struct str at = utf_at(0, dyn_str(buf), c);
  for_utf_rev(&rune, it, rest, strp(buf, at.str)) {
    if (!gui_rune_is_word_boundary(rune)) {
      at = rest;
      break;
    }
    c--;
  }
  /* find first word boundary rune */
  for_utf_rev(&rune, it, rest, at) {
    if (gui_rune_is_word_boundary(rune)) {
      break;
    }
    c--;
  }
  return c;
}
static int
gui_txt_ed_move_to_next_word(struct gui_txt_ed *edt, char *buf) {
  assert(edt);
  unsigned rune = 0;
  int c = edt->cur + 1;
  struct str at = utf_at(0, dyn_str(buf), c);
  for_utf(&rune, it, rest, strp(buf, at.str)) {
    if (!gui_rune_is_word_boundary(rune)) {
      at = rest;
      break;
    }
    c++;
  }
  for_utf(&rune, it, rest, at) {
    if (gui_rune_is_word_boundary(rune)) {
      break;
    }
    c++;
  }
  return c;
}
static void
gui_txt_ed_prep_sel_at_cur(struct gui_txt_ed *edt) {
  assert(edt);
  /* update selection and cursor to match each other */
  if (!gui_txt_ed_has_sel(edt)) {
    edt->sel[0] = edt->sel[1] = edt->cur;
  } else {
    edt->cur = edt->sel[1];
  }
}
static int
gui_txt_ed_cut(struct gui_txt_ed *edt, char **buf) {
  assert(edt);
  if (gui_txt_ed_has_sel(edt)) {
    gui_txt_ed_del_sel(edt, buf); /* implicitly clamps */
    return 1;
  }
  return 0;
}
static int
gui_txt_ed_paste(struct gui_txt_ed *edt, struct sys *sys, char **buf, struct str txt) {
  assert(edt);
  assert(buf);

  /* if there's a selection, the paste should delete it */
  gui_txt_ed_clamp(edt, *buf);
  gui_txt_ed_del_sel(edt, buf);

  const int cur = utf_at_idx(dyn_str(*buf), edt->cur);
  dyn_put(*buf, sys, cur, txt.str, txt.len);
  gui_txt_ed_undo_in(edt, cur, txt.len);
  return 1;
}
static void
gui_txt_ed_sel_all(struct gui_txt_ed *edt, char *buf) {
  assert(edt);
  edt->sel[0] = 0;
  edt->sel[1] = utf_len(dyn_str(buf));
  edt->cur = edt->sel[1];
}
static void
gui_txt_ed_txt(struct gui_txt_ed *edt, struct sys *sys, char **buf, struct str txt) {
  assert(edt);
  assert(buf);

  unsigned rune = 0;
  for_utf(&rune, it, _, txt) {
    if (rune == 127 || rune == '\n') {
      continue;
    }
    struct str cur = utf_at(0, dyn_str(*buf), edt->cur);
    int at = cast(int, cur.str - *buf);
    if (dyn_empty(*buf)) {
      dyn_asn(*buf, sys, it.str, it.len);
      edt->cur += 1;
    } else if (!gui_txt_ed_has_sel(edt) && edt->cur < utf_len(dyn_str(*buf))) {
      if (edt->mode == GUI_EDT_MODE_REPLACE) {
        gui_txt_ed_undo_repl(edt, buf, at, cur.len, it.len);
        dyn_cut(*buf, at, cur.len);
      }
      dyn_put(*buf, sys, at, it.str, it.len);
      edt->cur += 1;
    } else {
      gui_txt_ed_del_sel(edt, buf); /* implicitly clamps */
      cur = utf_at(0, dyn_str(*buf), edt->cur);
      at = cast(int, cur.str - *buf);

      dyn_put(*buf, sys, at, it.str, it.len);
      gui_txt_ed_undo_in(edt, at, it.len);
      edt->cur += 1;
    }
  }
}
static void
gui_txt_ed_clip(struct gui_txt_ed *edt, char *buf, struct sys *sys) {
  assert(edt);
  assert(sys);
  if (!gui_txt_ed_has_sel(edt)) {
    return;
  }
  /* selected text */
  int idx0 = min(edt->sel[0], edt->sel[1]);
  int idx1 = max(edt->sel[1], edt->sel[0]);
  int sel0 = utf_at_idx(dyn_str(buf), idx0);
  int sel1 = utf_at_idx(dyn_str(buf), idx1);
  sys->clipboard.set(str(buf + sel0, sel1 - sel0), sys->mem.tmp);
}
static int
gui_txt_ed_on_key(int *ret, struct gui_txt_ed *edt, char **buf, struct gui_ctx *ctx) {
  assert(edt);
  assert(ctx);

  /* mode */
  int mod = 0;
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_IN_MODE)) {
    edt->mode = GUI_EDT_MODE_INSERT;
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_REPL_MODE)) {
    edt->mode = GUI_EDT_MODE_REPLACE;
    *ret = 1;
  }
  /* selection */
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_ALL)) {
    gui_txt_ed_sel_all(edt, *buf);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_CUR_UP)) {
    bit_set(ctx->keys, GUI_KEY_EDIT_SEL_CUR_RIGHT);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_CUR_DOWN)) {
    bit_set(ctx->keys, GUI_KEY_EDIT_SEL_CUR_LEFT);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_CUR_LEFT)) {
    gui_txt_ed_clamp(edt, *buf);
    gui_txt_ed_prep_sel_at_cur(edt);
    if (edt->sel[1] > 0) edt->sel[1]--;
    edt->cur = edt->sel[1];
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_CUR_RIGHT)) {
    gui_txt_ed_prep_sel_at_cur(edt);
    edt->sel[1]++;
    gui_txt_ed_clamp(edt, *buf);
    edt->cur = edt->sel[1];
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_WORD_LEFT)) {
    if (!gui_txt_ed_has_sel(edt)) gui_txt_ed_prep_sel_at_cur(edt);
    edt->cur = gui_txt_ed_move_to_prev_word(edt, *buf);
    edt->sel[1] = edt->cur;
    gui_txt_ed_clamp(edt, *buf);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_WORD_RIGHT)) {
    if (!gui_txt_ed_has_sel(edt)) gui_txt_ed_prep_sel_at_cur(edt);
    edt->cur = gui_txt_ed_move_to_next_word(edt, *buf);
    edt->sel[1] = edt->cur;
    gui_txt_ed_clamp(edt, *buf);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_START)) {
    gui_txt_ed_prep_sel_at_cur(edt);
    edt->cur = edt->sel[1] = 0;
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_END)) {
    gui_txt_ed_prep_sel_at_cur(edt);
    edt->cur = edt->sel[1] = utf_len(dyn_str(*buf));
    *ret = 1;
  }
  /* movement */
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_CUR_UP)) {
    bit_set(ctx->keys, GUI_KEY_EDIT_CUR_LEFT);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_CUR_DOWN)) {
    bit_set(ctx->keys, GUI_KEY_EDIT_CUR_RIGHT);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_CUR_LEFT)) {
    if (gui_txt_ed_has_sel(edt))
      gui_txt_ed_move_sel_first(edt);
    else if (edt->cur > 0)
      edt->cur--;
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_CUR_RIGHT)) {
    if (gui_txt_ed_has_sel(edt)) {
      gui_txt_ed_move_sel_last(edt, *buf);
    } else {
      edt->cur++;
    }
    gui_txt_ed_clamp(edt, *buf);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_WORD_LEFT)) {
    if (!gui_txt_ed_has_sel(edt)) {
      edt->cur = gui_txt_ed_move_to_prev_word(edt, *buf);
      gui_txt_ed_clamp(edt, *buf);
    } else {
      gui_txt_ed_move_sel_first(edt);
    }
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_WORD_RIGHT)) {
    if (!gui_txt_ed_has_sel(edt)) {
      edt->cur = gui_txt_ed_move_to_next_word(edt, *buf);
      gui_txt_ed_clamp(edt, *buf);
    } else {
      gui_txt_ed_move_sel_last(edt, *buf);
    }
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_START)) {
    edt->cur = edt->sel[0] = edt->sel[1] = 0;
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_END)) {
    edt->cur = utf_len(dyn_str(*buf));
    edt->sel[0] = edt->sel[1] = 0;
    *ret = 1;
  }
  /* modification */
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_DELETE)) {
    if (!gui_txt_ed_has_sel(edt)) {
      int n = utf_len(dyn_str(*buf));
      if (edt->cur < n) {
        gui_txt_ed_del(edt, buf, edt->cur, 1);
      } else if (n) {
        gui_txt_ed_del(edt, buf, n-1, 1);
      }
    } else {
      gui_txt_ed_del_sel(edt, buf);
    }
    mod = 1;
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_REMOVE)) {
    if (!gui_txt_ed_has_sel(edt)) {
      if (edt->cur > 0) {
        gui_txt_ed_del(edt, buf, edt->cur - 1, 1);
        edt->cur = max(0, edt->cur - 1);
      } else if (utf_len(dyn_str(*buf))) {
        gui_txt_ed_del(edt, buf, 0, 1);
      }
    } else {
      gui_txt_ed_del_sel(edt, buf);
    }
    mod = 1;
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_UNDO)) {
    gui_txt_ed_undo(edt, ctx->sys, buf);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_REDO)) {
    gui_txt_ed_redo(edt, ctx->sys, buf);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_COPY)) {
    gui_txt_ed_clip(edt, *buf, ctx->sys);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_CUT)) {
    if (gui_txt_ed_has_sel(edt)) {
      gui_txt_ed_clip(edt, *buf, ctx->sys);
      gui_txt_ed_cut(edt, buf);
      mod = 1;
    }
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_PASTE)) {
    struct scope scp;
    struct sys *sys = ctx->sys;
    scope_begin(&scp, sys->mem.tmp);
    struct str p = sys->clipboard.get(sys->mem.tmp);
    gui_txt_ed_paste(edt, ctx->sys, buf, p);
    scope_end(&scp, sys->mem.tmp, sys);
    mod = 1;
  }
  return mod;
}

/* ---------------------------------------------------------------------------
 *                                  Edit
 * ---------------------------------------------------------------------------
 */
static int
gui_calc_edit_off(int *cur_ext, const struct gui_txt_ed *edt, const char *slc,
                  struct res *r, struct res_fnt *fnt, const int *txt_ext,
                  int width) {
  assert(edt);
  assert(fnt);
  assert(slc);
  assert(cur_ext);
  assert(txt_ext);

  res.fnt_ext(cur_ext, r, fnt, str(slc, edt->cur));
  int off = cur_ext[0] - (width >> 1);
  off = clamp(0, off, max(0, txt_ext[0] - width));
  return off;
}
static void
gui_edit_field_drw_txt_sel(struct gui_ctx *ctx, struct gui_panel *pan,
                           struct gui_txt_ed *edt, char *buf, int cur_off,
                           int *txt_ext) {
  assert(ctx);
  assert(pan);
  assert(edt);
  assert(txt_ext);

  /* calculate selection begin/end pixel offset */
  int sel0 = utf_at_idx(dyn_str(buf), min(edt->sel[0], edt->sel[1]));
  int sel1 = utf_at_idx(dyn_str(buf), max(edt->sel[1], edt->sel[0]));

  int sel_end_off[2] = {0}, sel_begin_off[2] = {0};
  gui_txt_ext(sel_begin_off, ctx, str(buf, sel0));
  gui_txt_ext(sel_end_off, ctx, str(buf, sel1));

  static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
  if (sel0 > 0) {
    /* text before selection */
    struct gui_panel lbl = {.box = pan->box, .state = pan->state};
    lbl.box.x = gui_min_ext(pan->box.x.min - cur_off, sel_begin_off[0]);
    gui_align_txt(&lbl.box, &align, txt_ext);
    gui_txt_drw(ctx, &lbl, str(buf, sel0), -1, 0);
  }
  if (gui_txt_ed_has_sel(edt)) {
    /* draw text in selection */
    struct gui_box b = {0};
    int sel_w = sel_end_off[0] - sel_begin_off[0];
    b.x = gui_min_ext(pan->box.x.min - cur_off + sel_begin_off[0], sel_w);
    b.y = gui_min_ext(pan->box.y.min, pan->box.y.ext);

    struct sys *sys = ctx->sys;
    sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_SEL]);
    sys->ren.drw.box(ctx->ren, gui_unbox(&b));

    struct gui_panel lbl = {.box = b, .state = pan->state};
    gui_align_txt(&lbl.box, &align, txt_ext);
    gui_txt_drw(ctx, &lbl, strp(buf + sel0, buf + sel1), -1, 0);
  }
  const int buf_len = utf_len(dyn_str(buf));
  if (sel1 < buf_len) {
    /* text after selection */
    int sel_w = txt_ext[0] - sel_end_off[0];
    int off = sel_end_off[0] - cur_off;

    struct gui_panel lbl = {.box = pan->box, .state = pan->state};
    lbl.box.x = gui_min_ext(pan->box.x.min + off, sel_w);

    gui_align_txt(&lbl.box, &align, txt_ext);
    gui_txt_drw(ctx, &lbl, strp(buf + sel1, dyn_end(buf)), -1, 0);
  }
}
static void
gui_edit_field_drw(struct gui_ctx *ctx, struct gui_edit_box *box,
                   struct gui_panel *pan, struct gui_txt_ed *edt,
                   char *buf, int cur_off, int *cur_ext, int *txt_ext) {
  assert(ctx);
  assert(box);
  assert(pan);
  assert(edt);
  assert(cur_ext);
  assert(txt_ext);

  /* draw text  */
  struct gui_box clip = {0};
  gui_clip_begin(&clip, ctx, gui_unbox(&pan->box));
  {
    box->active = pan->id == ctx->focused;
    if (box->active && gui_txt_ed_has_sel(edt)) {
      gui_edit_field_drw_txt_sel(ctx, pan, edt, buf, cur_off, txt_ext);
    } else {
      static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
      struct gui_panel lbl = {.box = pan->box, .state = pan->state};
      gui_align_txt(&lbl.box, &align, txt_ext);
      gui_txt_drw(ctx, &lbl, dyn_str(buf), -1, 0);
    }
  }
  gui_clip_end(ctx, &clip);

  /* draw cursor */
  if (box->active && !gui_txt_ed_has_sel(edt)) {
    struct sys *sys = ctx->sys;
    sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_TXT]);
    sys->ren.drw.vln(ctx->ren, pan->box.x.min + cur_ext[0], pan->box.y.min + 1,
              pan->box.y.max);
  }
  /* draw focus */
  if (pan->state == GUI_FOCUSED) {
    gui_focus_drw(ctx, &pan->box, 1);
  }
}
static void
gui_edit_field_input(struct gui_ctx *ctx, struct gui_edit_box *box,
                     struct gui_panel *pan, struct gui_txt_ed *edt, char **buf,
                     struct gui_input *in, struct res_fnt *fnt, int cur_off) {
  assert(in);
  assert(ctx);
  assert(box);
  assert(pan);
  assert(edt);
  assert(fnt);
  struct sys *sys = ctx->sys;

  /* focus change handling */
  if (in->gained_focus) {
    box->focused = 1;
    edt->mode = GUI_EDT_MODE_INSERT;
    if (box->flags & GUI_EDIT_SEL_ON_ACT) {
      gui_txt_ed_sel_all(edt, *buf);
    } else if (box->flags & GUI_EDIT_GOTO_END_ON_ACT) {
      int n = utf_len(dyn_str(*buf));
      edt->cur = edt->sel[0] = edt->sel[1] = n;
    }
    if (box->flags & GUI_EDIT_CLR_ON_ACT) {
      dyn_clr(*buf);
      edt->cur = edt->sel[0] = edt->sel[1] = 0;
    }
  }
  if (in->lost_focus) {
    edt->mode = GUI_EDT_MODE_INSERT;
    box->unfocused = 1;
  }
  /* input handling */
  box->active = pan->id == ctx->focused;
  if (box->active) {
    int key = 0;
    box->mod = !!gui_txt_ed_on_key(&key, edt, buf, ctx);
    if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_COMMIT)) {
      ctx->focused = ctx->root.id;
      box->unfocused = 1;
      box->commited = 1;
    }
    if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_ABORT)) {
      ctx->focused = ctx->root.id;
      box->aborted = 1;
    }
    if (!key && sys->txt_len) {
      gui_txt_ed_txt(edt, ctx->sys, buf, str(sys->txt, sys->txt_len));
      box->mod = 1;
    }
  }
  int mouse_pos = sys->mouse.pos[0] - pan->box.x.min + cur_off;
  if (in->mouse.btn.left.pressed) {
    gui_txt_ed_clk(edt, *buf, mouse_pos, 0, pan->box.y.ext, ctx->res, fnt);
  } else if (in->mouse.btn.left.dragged) {
    gui_txt_ed_drag(edt, *buf, mouse_pos, 0, pan->box.y.ext, ctx->res, fnt);
  }
}
static void
gui_edit_field(struct gui_ctx *ctx, struct gui_edit_box *box,
               struct gui_panel *pan, struct gui_panel *parent,
               struct gui_txt_ed *edt, char **buf) {
  assert(edt);
  assert(ctx);
  assert(box);
  assert(pan);
  assert(parent);
  pan->focusable = 1;
  gui_panel_begin(ctx, pan, parent);
  {
    /* calculate cursor offset  */
    int cur_ext[2], txt_ext[2];
    struct res_fnt *fnt = ctx->res->fnt;
    if (pan->id == ctx->focused) {
      gui_txt_ed_clamp(edt, *buf);
    }
    res.fnt_ext(txt_ext, ctx->res, fnt, dyn_str(*buf));
    const int cur = gui_calc_edit_off(cur_ext, edt, *buf, ctx->res, fnt,
                                      txt_ext, pan->box.x.ext);
    if (pan->is_hot) {
      struct sys *sys = ctx->sys;
      sys->cursor = SYS_CUR_IBEAM;
    }
    switch (ctx->pass) {
      case GUI_INPUT: {
        struct gui_input in = {0};
        gui_input(&in, ctx, pan, GUI_BTN_LEFT);
        gui_edit_field_input(ctx, box, pan, edt, buf, &in, fnt, cur);
      } break;
      case GUI_RENDER: {
        if (pan->state != GUI_HIDDEN)
          gui_edit_field_drw(ctx, box, pan, edt, *buf, cur, cur_ext, txt_ext);
      } break;
      case GUI_FINISHED:
        break;
    }
  }
  gui_panel_end(ctx, pan, parent);
}
static void
gui_edit_drw(struct gui_ctx *ctx, const struct gui_panel *pan) {
  assert(ctx);
  assert(pan);
  struct sys *sys = ctx->sys;

  const struct gui_box *b = &pan->box;
  sys->ren.drw.line_style(ctx->ren, 1);
  int c = pan->state == GUI_DISABLED ? GUI_COL_BG : GUI_COL_CONTENT;
  sys->ren.drw.col(ctx->ren, ctx->cfg.col[c]);
  sys->ren.drw.box(ctx->ren, gui_unbox(b));

  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_BG]);
  sys->ren.drw.hln(ctx->ren, b->y.max - 1, b->x.min + 1, b->x.max);
  sys->ren.drw.vln(ctx->ren, b->x.max - 1, b->y.min + 1, b->y.max - 1);

  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_SHADOW]);
  sys->ren.drw.hln(ctx->ren, b->y.min, b->x.min, b->x.max - 1);
  sys->ren.drw.vln(ctx->ren, b->x.min, b->y.min, b->y.max - 1);

  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_LIGHT]);
  sys->ren.drw.hln(ctx->ren, b->y.max, b->x.min, b->x.max);
  sys->ren.drw.vln(ctx->ren, b->x.max, b->y.min + 1, b->y.max);
}
static void
gui_edit(struct gui_ctx *ctx, struct gui_edit_box *edt,
         struct gui_panel *parent, struct gui_txt_ed *ed, char **buf) {
  assert(ed);
  assert(ctx);
  assert(edt);
  assert(parent);

  edt->pan.box = edt->box;
  gui_panel_begin(ctx, &edt->pan, parent);
  {
    struct gui_box *b = &edt->box;
    if (ctx->pass == GUI_RENDER && edt->pan.state != GUI_HIDDEN) {
      gui_edit_drw(ctx, &edt->pan);
    }
    /* content */
    static const int pad[2] = {3, 3};
    struct gui_panel content = {.focusable = 1};
    content.box.x = gui_shrink(&b->x, pad[0]);
    content.box.y = gui_shrink(&b->y, pad[1]);
    gui_edit_field(ctx, edt, &content, &edt->pan, ed, buf);
  }
  gui_panel_end(ctx, &edt->pan, parent);
}
static void
gui_edit_box(struct gui_ctx *ctx, struct gui_edit_box *box,
             struct gui_panel *parent, char **buf) {
  assert(buf);
  assert(ctx);
  assert(box);
  assert(parent);

  struct gui_txt_ed *edt = &ctx->txt_ed_tmp_state;
  const unsigned long long id = fnv1au64(ctx->id, parent->id);
  if (ctx->focused == id) {
    edt = &ctx->txt_ed_state;
    if (ctx->prev_focused != id) {
      gui_txt_ed_init(edt);
    }
  }
  gui_edit(ctx, box, parent, edt, buf);
}

/* ---------------------------------------------------------------------------
 *                                  Spinner
 * ---------------------------------------------------------------------------
 */
static float
gui_spin_cur_ratio(const struct gui_spin_val *spin){
  assert(spin);
  float ret = 0.0f;
  switch (spin->typ) {
    case GUI_SPIN_INT:
      ret = cast(float, spin->val.i - spin->min.i) / cast(float, spin->max.i);
      break;
    case GUI_SPIN_FLT:
      ret = (spin->val.f - spin->min.f) / spin->max.f;
      break;
    default:
      assert(0);
      break;
  }
  return ret;
}
static void
gui_spin_cur(struct gui_box *cur, const struct gui_spin_val *spin,
             const struct gui_spin *s) {
  assert(cur);
  assert(spin);
  assert(s);

  float r = gui_spin_cur_ratio(spin);
  cur->x = gui_shrink(&s->pan.box.x, 2);
  cur->x = gui_min_ext(cur->x.min, floori(r * cast(float, cur->x.ext)));
  cur->y = gui_shrink(&s->pan.box.y, 2);
}
static int
gui_spin_commit(struct gui_ctx *ctx, struct gui_spin_val *spin) {
  assert(ctx);
  assert(spin);

  int mod = 0;
  if (dyn_empty(ctx->txt_state.buf)) {
    dyn_add(ctx->txt_state.buf, ctx->sys, '0');
  }
  dyn_add(ctx->txt_state.buf, ctx->sys, 0);
  switch (spin->typ) {
    case GUI_SPIN_INT: {
      char *ep = 0;
      long d = strtol(ctx->txt_state.buf, &ep, 10);
      if (*ep == '\0' && ep != ctx->txt_state.buf) {
        spin->val.i = clamp(spin->min.i, (int)d, spin->max.i);
        mod = 1;
      }
    } break;
    case GUI_SPIN_FLT: {
      char *ep = 0;
      double d = strtod(ctx->txt_state.buf, &ep);
      if (*ep == '\0' && ep != ctx->txt_state.buf) {
        spin->val.f = clamp(spin->min.f, cast(float, d), spin->max.f);
        mod = 1;
      }
    } break;
    default:
      assert(0);
      break;
  }
  dyn_pop(ctx->txt_state.buf);
  return mod;
}
static void
gui_spin_focus(struct gui_ctx *ctx, struct gui_spin_val *spin,
               struct gui_txt_ed *ed) {
  assert(ed);
  assert(ctx);
  assert(spin);

  dyn_clr(ctx->txt_state.buf);
  switch (spin->typ) {
    case GUI_SPIN_INT:
      dyn_fmt(ctx->txt_state.buf, ctx->sys, "%d", spin->val.i);
      break;
    case GUI_SPIN_FLT:
      dyn_fmt(ctx->txt_state.buf, ctx->sys, "%.2f", spin->val.f);
      break;
    default:
      assert(0);
      break;
  }
  gui_txt_ed_init(ed);
  gui_txt_ed_sel_all(ed, ctx->txt_state.buf);
}
static int
gui_spin_key(struct gui_ctx *ctx, struct gui_spin_val *spin) {
  int mod = 0;
  assert(ctx);
  assert(spin);

  if (bit_tst_clr(ctx->keys, GUI_KEY_SPIN_INC)) {
    switch (spin->typ) {
      case GUI_SPIN_INT:
        spin->val.i = min(spin->val.i + spin->inc.i, spin->max.i);
        break;
      case GUI_SPIN_FLT:
        spin->val.f = min(spin->val.f + spin->inc.f, spin->max.f);
        break;
      default:
        assert(0);
        break;
    }
    mod = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_SPIN_DEC)) {
    switch (spin->typ) {
      case GUI_SPIN_INT:
        spin->val.i = max(spin->val.i - spin->inc.i, spin->min.i);
        break;
      case GUI_SPIN_FLT:
        spin->val.f = max(spin->val.f - spin->inc.f, spin->min.f);
        break;
      default:
        assert(0);
        break;
    }
    mod = 1;
  }
  return mod;
}
static void
gui_spin_scrl(struct sys *sys, struct gui_spin_val *spin) {
  assert(sys);
  assert(spin);
  switch (spin->typ) {
    case GUI_SPIN_INT: {
      int i = spin->val.i + spin->inc.i * sys->mouse.scrl[1];
      spin->val.i = clamp(spin->min.i, i, spin->max.i);
    } break;
    case GUI_SPIN_FLT: {
      float delta = cast(float, sys->mouse.scrl[1]);
      float v = spin->val.f + spin->inc.f * delta;
      spin->val.f = clamp(spin->min.f, v, spin->max.f);
    } break;
    default:
      assert(0);
      break;
  }
  sys->mouse.scrl[1] = 0;
}
static void
gui_spin_rel_drag(struct sys *sys, struct gui_spin_val *spin) {
  assert(sys);
  assert(spin);
  switch (spin->typ) {
    case GUI_SPIN_INT: {
      int val = sys->mouse.pos_delta[0] * spin->inc.i;
      spin->val.i = clamp(spin->min.i, spin->val.i + val, spin->max.i);
    } break;
    case GUI_SPIN_FLT: {
      float val = cast(float, sys->mouse.pos_delta[0]) * spin->inc.f;
      spin->val.f = clamp(spin->min.f, spin->val.f + val, spin->max.f);
    } break;
    default:
      assert(0);
      break;
  }
}
static void
gui_spin_abs_drag(struct sys *sys, struct gui_spin_val *spin,
                  const struct gui_spin *s) {
  assert(sys);
  assert(spin);
  assert(s);

  struct gui_box cur = {0};
  gui_spin_cur(&cur, spin, s);
  int dt = sys->mouse.pos[0] - cur.x.min;
  float r = cast(float, dt) /cast(float, s->pan.box.x.ext - 4);
  switch (spin->typ) {
    case GUI_SPIN_INT: {
      float off = cast(float, spin->max.i - spin->min.i) * r;
      int val = roundi(off) + spin->min.i;
      spin->val.i = clamp(spin->min.i, val, spin->max.i);
    } break;
    case GUI_SPIN_FLT: {
      float val = (r * (spin->max.f - spin->min.f)) + spin->min.f;
      spin->val.f = clamp(spin->min.f, val, spin->max.f);
    } break;
    default:
      assert(0);
      break;
  }
}
static void
gui_spin_str(char *buf, int cap, const struct gui_spin_val *spin) {
  assert(buf);
  assert(spin);
  switch (spin->typ) {
    case GUI_SPIN_INT:
      fmtsn(buf, cap, "%d", spin->val.i);
      break;
    case GUI_SPIN_FLT:
      fmtsn(buf, cap, "%.2f", spin->val.f);
      break;
    default:
      assert(0);
      break;
  }
}
static void
gui_spin_drw(struct gui_ctx *ctx, struct gui_spin_val *spin,
             const struct gui_spin *s) {
  assert(ctx);
  assert(spin);
  assert(s);

  gui_edit_drw(ctx, &s->pan);
  if (s->flags & GUI_SPIN_SLIDER) {
    struct gui_box cur = {0};
    gui_spin_cur(&cur, spin, s);
    cur.y.max += 1;

    struct sys *sys = ctx->sys;
    sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_SEL]);
    sys->ren.drw.box(ctx->ren, gui_unbox(&cur));
    sys->ren.drw.line_style(ctx->ren, 1);
    sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_TXT]);
    sys->ren.drw.vln(ctx->ren, cur.x.max, cur.y.min, cur.y.max - 1);
  }
}
static int
gui_spin(struct gui_ctx *ctx, struct gui_spin *s, struct gui_panel *parent,
         struct gui_spin_val *spin) {
  assert(ctx);
  assert(spin);
  assert(parent);

  int modified = 0;
  s->pan.box = s->box;
  gui_panel_begin(ctx, &s->pan, parent);
  {
    static const int edt_flags = GUI_EDIT_SEL_ON_ACT | GUI_EDIT_ENABLE_COMMIT;
    struct gui_edit_box edt = {.flags = edt_flags};
    if (ctx->pass == GUI_RENDER && s->pan.state != GUI_HIDDEN) {
      gui_spin_drw(ctx, spin, s);
    }
    int padx = ctx->cfg.pad[0];
    struct gui_box *b = &s->box;
    struct gui_panel edit_pan = {.focusable = 1};
    edit_pan.box.x = gui_min_max(b->x.min + padx, b->x.max - padx);
    edit_pan.box.y = gui_min_max(b->y.min + 2, b->y.max - 2);

    const unsigned long long id = fnv1au64(ctx->id, s->pan.id);
    if (ctx->focused == id) {
      /* focused edit field */
      struct gui_txt_ed *ed = &ctx->txt_ed_state;
      if (ctx->prev_focused != id) {
        gui_spin_focus(ctx, spin, ed);
      }
      struct gui_txt_buf *txt = &ctx->txt_state;
      gui_edit_field(ctx, &edt, &edit_pan, &s->pan, ed, &txt->buf);
      if (edt.commited) {
        modified = gui_spin_commit(ctx, spin) || modified;
      }
    } else {
      /* convert number to string */
      char buf[dyn_fix_siz(char, 256)];
      char *dum = dyn__static(buf, 256);
      gui_spin_str(dum, 256, spin);

      /* edit field */
      struct gui_txt_ed *ed = &ctx->txt_ed_tmp_state;
      gui_txt_ed_init(ed);

      gui_edit_field(ctx, &edt, &edit_pan, &s->pan, ed, &dum);
      if (edt.focused) {
        gui_spin_focus(ctx, spin, &ctx->txt_ed_state);
      } else if (edt.unfocused) {
        modified = gui_spin_commit(ctx, spin) || modified;
      }
    }
    /* input handling */
    struct gui_input in;
    gui_input(&in, ctx, &edit_pan, GUI_BTN_RIGHT);
    if (in.mouse.btn.left.grabbed) {
      ctx->sys->cursor = SYS_CUR_SIZE_WE;
    }
    modified = modified || edt.mod;
    if (edit_pan.state == GUI_FOCUSED) {
      /* focus input */
      struct sys *sys = ctx->sys;
      modified = gui_spin_key(ctx, spin) || modified;
      if (sys->mouse.scrl[1]) {
        gui_spin_scrl(sys, spin);
        modified = 1;
      }
    }
    if (in.mouse.btn.right.dragged || in.mouse.btn.right.pressed) {
      if (s->flags & GUI_SPIN_SLIDER) {
        gui_spin_abs_drag(ctx->sys, spin, s);
      } else {
        gui_spin_rel_drag(ctx->sys, spin);
      }
      modified = 1;
    }
    s->drag_begin = s->drag_begin || in.mouse.btn.right.drag_begin;
    s->drag_end = s->drag_end || in.mouse.btn.right.drag_end;
    s->dragged = s->dragged || in.mouse.btn.right.dragged;
  }
  gui_panel_end(ctx, &s->pan, parent);
  s->mod = s->mod || modified;
  return s->mod;
}
static int
gui_spin_flt(struct gui_ctx *ctx, struct gui_spin *ctl,
             struct gui_panel *parent, float *n, float min, float max,
             float inc) {
  assert(n);
  assert(ctx);
  assert(parent);

  struct gui_spin_val spin;
  spin.typ = GUI_SPIN_FLT;
  spin.inc.f = inc == 0.0f ? 1.0f : inc;
  spin.val.f = *n;

  if ((min == max) && min == 0.0f) {
    spin.min.f = -(1 << FLT_MANT_DIG);
    spin.max.f = (1 << FLT_MANT_DIG);
    spin.inc.f = 1.0f;
  } else {
    spin.min.f = min;
    spin.max.f = max;
  }
  if (gui_spin(ctx, ctl, parent, &spin)) {
    *n = spin.val.f;
    return 1;
  }
  return 0;
}
static int
gui_spin_f(struct gui_ctx *ctx, struct gui_spin *ctl, struct gui_panel *parent,
           float *n) {
  assert(n);
  assert(ctx);
  assert(parent);
  return gui_spin_flt(ctx, ctl, parent, n, 0.0f, 0.0f, 0.0f);
}
static int
gui_spin_int(struct gui_ctx *ctx, struct gui_spin *ctl,
             struct gui_panel *parent, int *n, int min, int max, int inc) {
  assert(n);
  assert(ctx);
  assert(parent);

  struct gui_spin_val spin = {0};
  spin.typ = GUI_SPIN_INT;
  spin.inc.i = inc == 0 ? 1 : inc;
  spin.val.i = *n;
  spin.min.i = min;
  spin.max.i = max;

  if ((min == max) && min == 0) {
    spin.min.i = INT_MIN;
    spin.max.i = INT_MAX;
    spin.inc.i = 1;
  } else {
    spin.min.i = min;
    spin.max.i = max;
  }
  if (gui_spin(ctx, ctl, parent, &spin)) {
    *n = spin.val.i;
    return 1;
  }
  return 0;
}
static int
gui_spin_i(struct gui_ctx *ctx, struct gui_spin *ctl, struct gui_panel *parent,
           int *n) {
  assert(n);
  assert(ctx);
  assert(parent);
  return gui_spin_int(ctx, ctl, parent, n, 0, 0, 0);
}

/* ---------------------------------------------------------------------------
 *                                  Group
 * ---------------------------------------------------------------------------
 */
static void
gui_grp_drw_hdr(struct gui_ctx *ctx, const struct gui_grp *grp,
                const struct gui_panel *pan, struct str txt) {
  assert(ctx);
  assert(grp);
  assert(pan);

  struct sys *sys = ctx->sys;
  const struct gui_box *b = &pan->box;
  int top = b->y.min + (grp->ext[1] >> 1);

  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_TXT]);
  res.print(ctx->ren, ctx->res, b->x.min + ctx->cfg.grp_off, b->y.min, txt);

  sys->ren.drw.line_style(ctx->ren, 1);
  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_SHADOW]);
  sys->ren.drw.hln(ctx->ren, top, b->x.min, b->x.min + ctx->cfg.grp_pad);

  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_LIGHT]);
  sys->ren.drw.hln(ctx->ren, top + 1, b->x.min + 1, b->x.min + ctx->cfg.grp_pad);
}
static void
gui_grp_drw(struct gui_ctx *ctx, const struct gui_grp *grp,
            const struct gui_panel *pan) {
  assert(ctx);
  assert(grp);
  assert(pan);

  const struct gui_box *b = &pan->box;
  int top = b->y.min + (grp->ext[1] >> 1);
  int ext_off = 2 * ctx->cfg.grp_off + grp->ext[0];

  struct sys *sys = ctx->sys;
  sys->ren.drw.line_style(ctx->ren, 1);
  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_SHADOW]);
  sys->ren.drw.hln(ctx->ren, top, b->x.min + ext_off, b->x.max - 1);
  sys->ren.drw.hln(ctx->ren, b->y.max - 1, b->x.min, b->x.max - 1);
  sys->ren.drw.vln(ctx->ren, b->x.min, b->y.min, b->y.max - 1);
  sys->ren.drw.vln(ctx->ren, b->x.max - 1, top, b->y.max - 1);

  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_LIGHT]);
  sys->ren.drw.hln(ctx->ren, top + 1, b->x.min + ext_off, b->x.max - 2);
  sys->ren.drw.hln(ctx->ren, b->y.max, b->x.min, b->x.max);
  sys->ren.drw.vln(ctx->ren, b->x.min + 1, top + 1, b->y.max - 2);
  sys->ren.drw.vln(ctx->ren, b->x.max, top, b->y.max);
}
static void
gui_grp_begin(struct gui_ctx *ctx, struct gui_grp *grp,
              struct gui_panel *parent, struct str txt) {
  assert(ctx);
  assert(grp);
  assert(parent);

  grp->pan.box = grp->box;
  gui_panel_begin(ctx, &grp->pan, parent);

  gui_txt_ext(grp->ext, ctx, txt);
  grp->content.y = gui_min_max(grp->box.y.min + grp->ext[1], grp->box.y.max);
  grp->content.x = grp->box.x;
  if (ctx->pass == GUI_RENDER && grp->pan.state != GUI_HIDDEN) {
    gui_grp_drw_hdr(ctx, grp, &grp->pan, txt);
  }
}
static void
gui_grp_end(struct gui_ctx *ctx, struct gui_grp *grp, struct gui_panel *parent) {
  assert(ctx);
  assert(grp);

  int ext[2];
  ext[0] = grp->pan.max[0] - grp->pan.box.x.min;
  ext[1] = grp->pan.max[1] - grp->pan.box.y.min;

  grp->pan.box.x = gui_min_ext(grp->pan.box.x.min, ext[0]);
  grp->pan.box.y = gui_min_ext(grp->pan.box.y.min, ext[1]);
  if (ctx->pass == GUI_RENDER && grp->pan.state != GUI_HIDDEN) {
    gui_grp_drw(ctx, grp, &grp->pan);
  }
  gui_panel_end(ctx, &grp->pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  Region
 * ---------------------------------------------------------------------------
 */
static void
gui_reg_drw(struct gui_ctx *ctx, const struct gui_box *b) {
  assert(ctx);
  assert(b);

  /* background */
  struct sys *sys = ctx->sys;
  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_CONTENT]);
  sys->ren.drw.box(ctx->ren, gui_unbox(b));

  /* border */
  sys->ren.drw.line_style(ctx->ren, 1);
  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_SHADOW]);
  sys->ren.drw.hln(ctx->ren, b->y.min, b->x.min, b->x.max);
  sys->ren.drw.vln(ctx->ren, b->x.min, b->y.min, b->y.max);
}
static void
gui_reg_begin(struct gui_ctx *ctx, struct gui_reg *d,
               struct gui_panel *parent, const double *off) {
  assert(d);
  assert(ctx);
  assert(off);
  assert(parent);

  d->pan.box = d->box;
  gui_panel_begin(ctx, &d->pan, parent);
  memcpy(d->off, off, sizeof(d->off));

  /* calculate clip area */
  struct gui_box *b = &d->pan.box;
  d->space.x = gui_min_max(b->x.min, b->x.max - ctx->cfg.scrl);
  d->space.y = gui_min_max(b->y.min, b->y.max - ctx->cfg.scrl);
  if (ctx->pass == GUI_RENDER && d->pan.state != GUI_HIDDEN) {
    gui_reg_drw(ctx, &d->box);
    gui_clip_begin(&d->clip_rect, ctx, d->space.x.min, d->space.y.min,
                   d->space.x.max, d->space.y.max);
  }
  /* apply scroll offset to area body */
  int off_x = floori(d->off[0]);
  int off_y = floori(d->off[1]);

  b->x = gui_min_max(d->space.x.min - off_x, d->space.x.max - off_x);
  b->y = gui_min_max(d->space.y.min - off_y, d->space.y.max - off_y);
}
static void
gui_reg_apply_lst(struct gui_reg *d, const struct gui_lst *lst,
                   int row_mult) {
  assert(d);
  assert(lst);

  d->vscrl.step = cast(double, lst->lay.slot[1]);
  d->scrl_wheel = lst->lay.slot[1] * row_mult;

  struct gui_panel *pan = &d->pan;
  pan->max[0] = max(pan->max[0], lst->view.max[0]);
  pan->max[1] = max(pan->max[1], lst->view.max[1]);
}
static void
gui_reg_end(struct gui_ctx *ctx, struct gui_reg *d, struct gui_panel *parent,
            double *off) {
  assert(d);
  assert(off);
  assert(ctx);
  assert(ctx->sys);
  d->scrolled = 0;

  struct sys *sys = ctx->sys;
  struct gui_panel *pan = &d->pan;
  gui_panel_end(ctx, pan, parent);
  if (ctx->pass == GUI_RENDER && pan->state != GUI_HIDDEN) {
    gui_clip_end(ctx, &d->clip_rect);
  }
  int off_x = floori(d->off[0]);
  int off_y = floori(d->off[1]);

  /* do mouse wheel scrolling */
  if (sys->mouse.scrl[1]) {
    if (ctx->pass == GUI_INPUT && pan->is_hov) {
      d->scrl_wheel = !d->scrl_wheel ? 1 : d->scrl_wheel;
      d->off[1] -= cast(double, sys->mouse.scrl[1] * d->scrl_wheel);
      sys->mouse.scrl[1] = 0;
      d->scrolled = 1;
    }
  }
  /* setup vertical scrollbar */
  d->vscrl.min_size = ctx->cfg.scrl;
  d->vscrl.total = cast(double, pan->max[1] - pan->box.y.min);
  d->vscrl.size = cast(double, pan->box.y.max - pan->box.y.min - ctx->cfg.scrl);

  /* handle keyboard list begin/end action */
  if (pan->is_hot && bit_tst_clr(ctx->keys, GUI_KEY_SCRL_BEGIN)) {
    d->off[1] = 0;
  } else if (pan->is_hot && bit_tst_clr(ctx->keys, GUI_KEY_SCRL_END)) {
    d->off[1] = d->vscrl.total - d->vscrl.size;
  }
  /* handle list page up/down action */
  if (pan->is_hot && bit_tst_clr(ctx->keys, GUI_KEY_SCRL_PGUP)) {
    d->off[1] = d->off[1] - d->vscrl.size;
    d->off[1] = clamp(0, d->off[1], d->vscrl.total - d->vscrl.size);
  }
  if (pan->is_hot && bit_tst_clr(ctx->keys, GUI_KEY_SCRL_PGDN)) {
    d->off[1] = d->off[1] + d->vscrl.size;
    d->off[1] = clamp(0, d->off[1], d->vscrl.total - d->vscrl.size);
  }
  d->vscrl.off = d->off[1];

  /* do vertical scrollbar */
  if (d->vscrl.total > d->vscrl.size + ctx->cfg.scrl) {
    int top = pan->box.y.min + 2 + off_y;
    int right = pan->box.x.max + ctx->cfg.scrl + off_x;
    d->vscrl.box.x = gui_min_max(pan->box.x.max + off_x, right);
    d->vscrl.box.y = gui_min_max(top, pan->box.y.max + off_y);

    gui_vscrl(ctx, &d->vscrl, pan);
    if (absf(d->off[1] - d->vscrl.off) >= 1.0f) {
      d->off[1] = d->vscrl.off;
      d->scrolled = 1;
    }
  } else {
    d->off[1] = 0;
  }
  /* setup horizontal scrollbar */
  d->hscrl.off = d->off[0];
  d->hscrl.min_size = ctx->cfg.scrl;
  d->hscrl.total = cast(double, pan->max[0] - pan->box.x.min);
  d->hscrl.size = cast(double, pan->box.x.max - pan->box.x.min + ctx->cfg.scrl);

  /* do horizontal scrollbar */
  if (d->hscrl.total > d->hscrl.size) {
    int bot = pan->box.y.max + ctx->cfg.scrl + off_y;
    int left = pan->box.x.min + 2 + off_x;
    d->hscrl.box.x = gui_min_max(left, pan->box.x.max + off_x);
    d->hscrl.box.y = gui_min_max(pan->box.y.max + off_y, bot);

    gui_hscrl(ctx, &d->hscrl, pan);
    if (absf(d->off[0] - d->hscrl.off) >= 1.0f) {
      d->off[0] = d->hscrl.off;
      d->scrolled = 1;
    }
  } else {
    d->off[0] = 0;
  }
  if (off) {
    memcpy(off, d->off, sizeof(d->off));
  }
  d->max_off[0] = d->hscrl.total - d->hscrl.size;
  d->max_off[1] = d->vscrl.total - d->vscrl.size;
}

/* ---------------------------------------------------------------------------
 *                              List Layout
 * ---------------------------------------------------------------------------
 */
static void
gui_lst_lay_init(struct gui_lst_lay *lst) {
  assert(lst);
  lst->idx = 0;
  lst->slot[0] = max(1, lst->item[0] + lst->gap[0]);
  lst->slot[1] = max(1, lst->item[1] + lst->gap[1]);

  lst->lay = gui_padv(&lst->box, lst->pad);
  if (lst->orient == GUI_VERTICAL) {
    float space = cast(float, lst->space[0]);
    float slot = cast(float, lst->slot[0]);

    lst->space[0] = max(0, lst->box.x.ext - (lst->pad[0] << 1));
    lst->space[1] = max(0, lst->box.y.ext - lst->pad[1]);
    lst->cnt[0] = max(1, floori(space / slot));
    lst->off = roundi(lst->offset / cast(float, lst->slot[1]));
    lst->off_idx = lst->off * lst->cnt[0];
    lst->lay.y = gui_min_max(lst->lay.y.min, INT_MAX);

    lst->cnt[1] = 0;
    lst->view_cnt = lst->cnt[0];
    lst->page = roundi(cast(float, lst->space[1]) / cast(float, lst->slot[1]));
    lst->page_cnt = lst->page * lst->cnt[0];
  } else {
    float space = cast(float, lst->space[1]);
    float slot = cast(float, lst->slot[1]);

    lst->space[0] = max(0, lst->box.x.ext - lst->pad[0]);
    lst->space[1] = max(0, lst->box.y.ext - (lst->pad[1] << 1));
    lst->cnt[1] = max(1, floori(space / slot));
    lst->off = roundi(lst->offset / cast(float, lst->slot[0]));
    lst->off_idx = lst->off * lst->cnt[1];
    lst->lay.x = gui_min_max(lst->lay.x.min, INT_MAX);

    lst->cnt[0] = 0;
    lst->view_cnt = lst->cnt[1];
    lst->page = roundi(cast(float, lst->space[0]) / cast(float, lst->slot[0]));
    lst->page_cnt = lst->page * lst->cnt[1];
  }
  lst->total = lst->lay;
}
static void
gui_lst_lay_gen(struct gui_box *box, struct gui_lst_lay *lst) {
  assert(box);
  assert(lst);
  switch (lst->orient) {
    case GUI_VERTICAL: {
      *box = gui_cut_top(&lst->lay, lst->item[1], lst->gap[1]);
    } break;
    case GUI_HORIZONTAL: {
      *box = gui_cut_lhs(&lst->lay, lst->item[0], lst->gap[0]);
    } break;
  }
  lst->idx++;
}
static void
gui_lst_lay_apply_view(struct gui_lst_lay *lst, const struct gui_lst_view *v) {
  assert(v);
  assert(lst);

  lst->idx = v->begin;
  lst->lay.x = gui_min_max(v->at[0], lst->lay.x.max);
  lst->lay.y = gui_min_max(v->at[1], lst->lay.y.max);
}
static double
gui_lst_lay_center(struct gui_lst_lay *lay, int idx) {
  assert(lay);
  switch (lay->orient) {
    case GUI_HORIZONTAL: {
      int mid_off = lay->slot[0] * idx + (lay->slot[0] >> 1);
      return cast(double, max(0, mid_off - (lay->space[0] >> 1)));
    }
    case GUI_VERTICAL: {
      int mid_off = lay->slot[1] * idx + (lay->slot[1] >> 1);
      return cast(double, max(0, mid_off - (lay->space[1] >> 1)));
    }
  }
  return 0.0f;
}
static double
gui_lst_lay_fit_start(struct gui_lst_lay *lay, int idx) {
  assert(lay);
  switch (lay->orient) {
    case GUI_HORIZONTAL:
      return cast(double, lay->slot[0] * idx);
    case GUI_VERTICAL:
      return cast(double, lay->slot[1] * idx);
  }
  return 0.0f;
}
static double
gui_lst_lay_fit_end(struct gui_lst_lay *lay, int idx) {
  assert(lay);
  switch (lay->orient) {
    case GUI_HORIZONTAL:
      return cast(double, lay->slot[0] * (idx + 1) - lay->space[0]);
    case GUI_VERTICAL:
      return cast(double, lay->slot[1] * (idx + 1) - lay->space[1]);
  }
  return 0.0f;
}
static double
gui_lst_lay_clamp(struct gui_lst_lay *lay, int idx) {
  assert(lay);
  if (idx <= lay->off_idx) {
    return gui_lst_lay_fit_start(lay, idx);
  } else if (lay->off_idx + lay->page_cnt <= idx) {
    return gui_lst_lay_fit_end(lay, idx);
  } else {
    return lay->offset;
  }
}

/* ---------------------------------------------------------------------------
 *                              List-Control
 * ---------------------------------------------------------------------------
 */
static void
gui_lst_ctl_focus(struct gui_ctx *ctx, struct gui_lst_ctl *ctl,
                  struct gui_panel *parent, int item_idx, int was_focused) {
  ctl->gained_focus = !was_focused;
  ctl->item_idx = item_idx;
  ctl->has_focus = 1;
  ctl->mod = 1;

  parent->is_focused = 1;
  gui_focus(ctx, parent);

  ctx->focus_next = 0;
  ctx->lst_state.focused = 1;
  ctx->lst_state.cur_idx = item_idx;
  ctx->lst_state.owner = parent->id;
}
static void
gui_lst_ctl_elm(struct gui_ctx *ctx, struct gui_lst_ctl *ctl,
                const struct gui_lst_lay *lst, const struct gui_box *box,
                struct gui_panel *parent, unsigned long long id) {
  assert(ctx);
  assert(ctl);
  assert(lst);
  assert(box);
  assert(parent);

  struct gui_panel item = {.box = *box};
  gui_panel_id(ctx, &item, parent, id);
  ctl->owner_id = parent->id;

  int item_idx = lst->idx - 1;
  int focused = parent->id == ctx->lst_state.owner && ctx->lst_state.focused;
  if (ctl->idx++ == 0) {
    ctl->has_focus = parent->is_focused;
    if (!ctl->has_focus) {
      ctx->prev_id = parent->id;
      if (ctx->focus_next) { /* keyboard list focus */
        gui_lst_ctl_focus(ctx, ctl, parent, item_idx, focused);
      }
    } else if (bit_tst_clr(ctx->keys, GUI_KEY_PREV_WIDGET)) {
      ctx->focused = ctx->prev_id;
      if (ctx->prev_id == ctx->root.id) {
        ctx->focus_last = 1;
      }
    }
  }
  /* mouse list focus */
  struct gui_input in;
  gui_input(&in, ctx, &item, GUI_BTN_LEFT);
  int pressed = in.mouse.btn.left.pressed && ctl->focus == GUI_LST_FOCUS_ON_CLK;
  int hovered = item.is_hot && ctl->focus == GUI_LST_FOCUS_ON_HOV;
  if (((pressed || hovered))) {
    gui_lst_ctl_focus(ctx, ctl, parent, item_idx, 0);
  }
  /* store cursor box for later drawing of cursor */
  ctl->activated = 0;
  if (ctl->has_focus && ctx->lst_state.cur_idx == lst->idx - 1 &&
      item_idx >= lst->off_idx && item_idx <= lst->off_idx + lst->page_cnt) {
    ctl->cur_vis = 1;
    ctl->cur_box = *box;
    if (bit_tst_clr(ctx->keys, GUI_KEY_ACT)) {
      ctl->activated = 1;
    }
  }
}
static void
gui_lst_ctl_proc(struct gui_ctx *ctx, struct gui_lst_ctl *ctl,
                 const struct gui_lst_lay *lay, int total) {
  assert(ctx);
  assert(lay);
  assert(ctl);
  total = max(0, total - 1);

  struct gui_lst_state *s = &ctx->lst_state;
  if (s->focused && !ctl->has_focus && ctl->owner_id == s->owner) {
    ctl->lost_focus = 1;
    ctl->has_focus = 0;

    s->owner = ctx->root.id;
    s->focused = 0;
    s->cur_idx = 0;
  }
  if (!ctl->has_focus) {
    return;
  }
  int cur_inc = 0, cur_dec = 0;
  int view_inc = 0, view_dec = 0;
  if (ctx->pass == GUI_RENDER && ctl->show_cursor && ctl->cur_vis) {
    /* draw list item cursor */
    struct sys *sys = ctx->sys;
    const struct gui_box *b = &ctl->cur_box;
    sys->ren.drw.line_style(ctx->ren, 1);
    sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_TXT]);
    sys->ren.drw.sbox(ctx->ren, b->x.min - 1, b->y.min - 1, b->x.max - 1, b->y.max - 1);
  }
  /* map keys to operations */
  switch (lay->orient) {
    case GUI_HORIZONTAL: {
      cur_inc = bit_tst_clr(ctx->keys, GUI_KEY_LST_RIGHT);
      cur_dec = bit_tst_clr(ctx->keys, GUI_KEY_LST_LEFT);
    } break;
    case GUI_VERTICAL: {
      cur_inc = bit_tst_clr(ctx->keys, GUI_KEY_LST_DN);
      cur_dec = bit_tst_clr(ctx->keys, GUI_KEY_LST_UP);
    } break;
  }
  /* cursor movement */
  if (cur_inc) {
    ctl->scrl = GUI_LST_CLAMP_ITEM;
    ctl->item_idx = clamp(0, s->cur_idx + 1, total);
    ctl->mod = ctl->was_scrolled = 1;
    ctl->key_mod = ctl->cur_mod = 1;
  } else if (cur_dec) {
    ctl->scrl = GUI_LST_CLAMP_ITEM;
    ctl->item_idx = clamp(0, s->cur_idx - 1, total);
    ctl->mod = ctl->was_scrolled = 1;
    ctl->key_mod = ctl->cur_mod = 1;
  } else if (view_inc) {
    ctl->scrl = GUI_LST_FIT_ITEM_END;
    ctl->item_idx = clamp(0, lay->idx + lay->view_cnt, total);
    ctl->mod = ctl->was_scrolled = ctl->key_mod = 1;
  } else if (view_dec) {
    ctl->scrl = GUI_LST_FIT_ITEM_START;
    ctl->item_idx = clamp(0, lay->idx - lay->view_cnt, total);
    ctl->mod = ctl->was_scrolled = ctl->key_mod = 1;
  }
  /* handle keyboard list begin/end action */
  if (bit_tst(ctx->keys, GUI_KEY_SCRL_BEGIN)) {
    ctl->item_idx = 0;
    ctl->scrl = GUI_LST_FIT_ITEM_START;
    ctl->mod = ctl->was_scrolled = 1;
    ctl->cur_mod = ctl->key_mod = ctl->jmp_mod = 1;
  } else if (bit_tst(ctx->keys, GUI_KEY_SCRL_END)) {
    ctl->item_idx = total;
    ctl->scrl = GUI_LST_FIT_ITEM_END;
    ctl->mod = ctl->was_scrolled = 1;
    ctl->cur_mod = ctl->key_mod = ctl->jmp_mod = 1;
  }
  /* handle list page up action */
  if (bit_tst_clr(ctx->keys, GUI_KEY_SCRL_PGUP)) {
    ctl->mod = ctl->cur_mod = 1;
    ctl->key_mod = ctl->jmp_mod = 1;
    if (ctx->lst_state.cur_idx == lay->off_idx) {
      ctl->item_idx = max(0, ctx->lst_state.cur_idx - lay->page_cnt);
      ctl->scrl = GUI_LST_FIT_ITEM_START;
      ctl->was_scrolled = 1;
    } else {
      ctl->item_idx = lay->off_idx;
    }
  } else if (bit_tst_clr(ctx->keys, GUI_KEY_SCRL_PGDN)) {
    /* handle list page down action */
    int page_end_idx = lay->off_idx + lay->page_cnt;
    ctl->key_mod = ctl->jmp_mod = 1;
    ctl->mod = ctl->cur_mod = 1;
    if (ctx->lst_state.cur_idx == max(0, page_end_idx - 1)) {
      ctl->item_idx = min(ctx->lst_state.cur_idx + lay->page_cnt, total);
      ctl->scrl = GUI_LST_FIT_ITEM_END;
      ctl->was_scrolled = 1;
    } else {
      ctl->item_idx = max(0, page_end_idx - 1);
    }
  }
  if (ctl->mod) {
    ctx->lst_state.cur_idx = ctl->item_idx;
  }
}

/* ---------------------------------------------------------------------------
 *                              List-Selection
 * ---------------------------------------------------------------------------
 */
struct gui_lst_elm_state {
  struct gui_input in;
  unsigned hov : 1;
  unsigned down : 1;
  unsigned clk : 1;
};
static void
gui_lst_sel_elm_input(struct gui_lst_elm_state *s, struct gui_ctx *ctx,
                      struct gui_lst_sel *sel, const struct gui_lst_lay *lay,
                      const struct gui_lst_ctl *ctl, struct sys *sys,
                      struct gui_panel *item, int is_sel) {
  /* mouse item selection */
  gui_input(&s->in, ctx, item, GUI_BTN_LEFT);
  s->hov = (sel->on == GUI_LST_SEL_ON_HOV) && s->in.is_hot;
  s->down = ctl->has_focus && ctx->lst_state.cur_idx == lay->idx - 1 &&
            bit_tst_clr(ctx->keys, GUI_KEY_LST_SEL);
  s->clk = 0;
  if (sel->on == GUI_LST_SEL_ON_CLK) {
    int ctrl = (sys->keymod & SYS_KEYMOD_CTRL);
    int shift = (sys->keymod & SYS_KEYMOD_SHIFT);
    if (sel->mode == GUI_LST_SEL_SINGLE || sel->bhv == GUI_LST_SEL_BHV_TOGGLE) {
      s->clk = s->in.mouse.btn.left.pressed;
    } else if (is_sel && !ctrl && !shift) {
      s->clk = s->in.mouse.btn.left.clk;
    } else if ((sys->keymod & SYS_KEYMOD_CTRL)) {
      s->clk = s->in.mouse.btn.left.clk;
    } else if (sys->keymod & SYS_KEYMOD_SHIFT) {
      s->clk = s->in.mouse.btn.left.pressed;
    } else {
      s->clk = s->in.mouse.btn.left.pressed;
    }
  }
}
static void
gui_lst_on_sel(struct gui_ctx *ctx, struct gui_lst_sel *sel,
               const struct gui_lst_lay *lay, int is_sel) {
  struct sys *sys = ctx->sys;
  switch (sel->bhv) {
    case GUI_LST_SEL_BHV_TOGGLE: {
      sel->mut = GUI_LST_SEL_MOD_EMPLACE;
    } break;
    case GUI_LST_SEL_BHV_FLEETING: {
      if (sys->keymod & SYS_KEYMOD_CTRL) {
        sel->mut = GUI_LST_SEL_MOD_EMPLACE;
      } else {
        sel->mut = GUI_LST_SEL_MOD_REPLACE;
      }
    } break;
  }
  struct gui_lst_state *s = &ctx->lst_state;
  if ((sys->keymod & SYS_KEYMOD_SHIFT) && sel->bhv != GUI_LST_SEL_BHV_TOGGLE) {
    sel->op = is_sel ? GUI_LST_SEL_OP_CLR : GUI_LST_SEL_OP_SET;
    if (s->sel_idx > lay->idx - 1) {
      sel->begin_idx = lay->idx - 1;
      sel->end_idx = s->sel_idx + 1;
    } else {
      sel->begin_idx = s->sel_idx;
      sel->end_idx = lay->idx;
    }
  } else {
    int ctrl = (sys->keymod & SYS_KEYMOD_CTRL);
    sel->begin_idx = sel->idx;
    sel->end_idx = sel->idx + 1;
    if (!ctrl && sel->bhv != GUI_LST_SEL_BHV_TOGGLE) {
      sel->op = GUI_LST_SEL_OP_SET;
    } else {
      sel->op = is_sel ? GUI_LST_SEL_OP_CLR : GUI_LST_SEL_OP_SET;
    }
    s->sel_op = is_sel ? GUI_LST_SEL_OP_CLR : GUI_LST_SEL_OP_SET;
    s->sel_idx = sel->idx;
  }
}
static void
gui_lst_sel_elm(struct gui_ctx *ctx, struct gui_lst_sel *sel,
                const struct gui_lst_lay *lay, const struct gui_lst_ctl *ctl,
                struct gui_box *box, struct gui_panel *parent, int is_sel,
                unsigned long long id) {
  assert(ctx);
  assert(sel);
  assert(lay);
  assert(ctl);
  assert(box);
  assert(parent);

  struct gui_panel item = {.box = *box};
  gui_panel_id(ctx, &item, parent, id);
  if (sel->bitset && sel->mode == GUI_LST_SEL_MULTI) {
    is_sel = bit_tst(sel->bitset, lay->idx - 1);
  } else if (sel->mode == GUI_LST_SEL_SINGLE && sel->src == GUI_LST_SEL_SRC_INT) {
    is_sel = is_sel || (ctl->has_focus && ctx->lst_state.cur_idx == ctl->idx - 1);
  }
  struct gui_lst_elm_state state;
  gui_lst_sel_elm_input(&state, ctx, sel, lay, ctl, ctx->sys, &item, is_sel);
  if (state.clk || state.hov || state.down) {
    sel->mod = 1;
    sel->idx = lay->idx - 1;
    if (sel->mode == GUI_LST_SEL_MULTI && (state.clk || state.down)) {
      gui_lst_on_sel(ctx, sel, lay, is_sel);
    } else if (sel->mode == GUI_LST_SEL_SINGLE &&
               ctl->focus == GUI_LST_FOCUS_ON_HOV) {
      is_sel = state.hov;
    }
  }
  if (ctx->pass == GUI_RENDER) {
    /* draw selection background */
    struct sys *sys = ctx->sys;
    if (is_sel) {
      sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_SEL]);
      sys->ren.drw.box(ctx->ren, gui_unbox(box));
    } else if (sel->hov == GUI_LST_SEL_HOV_YES && item.is_hot) {
      if (ctx->cfg.col[GUI_COL_CONTENT_HOV] != ctx->cfg.col[GUI_COL_CONTENT]) {
        sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_CONTENT_HOV]);
        sys->ren.drw.box(ctx->ren, gui_unbox(box));
      }
    }
  }
}
static void
gui_lst_sel_proc(struct gui_ctx *ctx, struct gui_lst_sel *sel,
                 const struct gui_lst_ctl *ctl, int total) {
  assert(ctx);
  assert(ctl);
  assert(sel);

  struct sys *sys = ctx->sys;
  struct gui_lst_state *s = &ctx->lst_state;
  if (sel->mode != GUI_LST_SEL_MULTI) {
    if (ctl->cur_mod) {
      sel->mod = 1;
      sel->op = GUI_LST_SEL_OP_SET;
      sel->idx = ctl->item_idx;
      sel->mut = GUI_LST_SEL_MOD_REPLACE;
      sel->begin_idx = ctl->item_idx;
      sel->end_idx = ctl->item_idx + 1;
      s->sel_idx = sel->idx;
    }
  }
  if (ctl->has_focus && bit_tst_clr(ctx->keys, GUI_KEY_LST_SEL_ALL)) {
    /* handle select all shortcut */
    sel->op = GUI_LST_SEL_OP_SET;
    sel->mod = 1;
    sel->begin_idx = 0;
    sel->end_idx = total;
    sel->idx = 0;
    sel->mut = GUI_LST_SEL_MOD_EMPLACE;
  }
  /* handle cursor selection change */
  if (ctl->key_mod) {
    if ((sys->keymod & SYS_KEYMOD_SHIFT)) {
      /* handle range selection by cursor jump */
      int item = clamp(0, ctl->item_idx, total);
      sel->begin_idx = min(s->sel_idx, item);
      sel->end_idx = max(s->sel_idx, item) + 1;
      sel->op = (enum gui_lst_sel_op)s->sel_op;
      sel->mod = 1;
    }
    if (!(sys->keymod & SYS_KEYMOD_CTRL) && ctl->cur_mod) {
      /* handle cursor single selection on movement */
      sel->mod = 1;
      sel->op = GUI_LST_SEL_OP_SET;
      sel->idx = ctl->item_idx;
      sel->mut = GUI_LST_SEL_MOD_REPLACE;

      if (ctl->jmp_mod && (sys->keymod & SYS_KEYMOD_SHIFT)) {
        int item = clamp(0, ctl->item_idx, total);
        sel->begin_idx = min(s->sel_idx, item);
        sel->end_idx = max(s->sel_idx, item) + 1;
      } else {
        sel->begin_idx = ctl->item_idx;
        sel->end_idx = ctl->item_idx + 1;
        s->sel_op = GUI_LST_SEL_OP_SET;
        s->sel_idx = ctl->item_idx;
      }
    }
    if ((sys->keymod & SYS_KEYMOD_SHIFT)) {
      sel->mut = GUI_LST_SEL_MOD_EMPLACE;
    }
  }
  sel->begin_idx = clamp(0, sel->begin_idx, total);
  sel->end_idx = clamp(0, sel->end_idx, total);

  /* modify bitset by selection set */
  if (sel->bitset && sel->mod) {
    if (sel->mut == GUI_LST_SEL_MOD_REPLACE) {
      bit_fill(sel->bitset, 0, total);
      sel->cnt = 0;
    }
    for (int i = sel->begin_idx; i < sel->end_idx; ++i) {
      switch (sel->op) {
        case GUI_LST_SEL_OP_SET:
          if (!bit_set(sel->bitset, i)) {
            sel->cnt++;
          }
          break;
        case GUI_LST_SEL_OP_CLR:
          if (bit_clr(sel->bitset, i)) {
            sel->cnt--;
          }
          break;
      }
    }
  }
  /* key handler */
  if (ctl->has_focus) {
    sel->cpy = !!bit_tst_clr(ctx->keys, GUI_KEY_LST_CPY);
    sel->cut = !!bit_tst_clr(ctx->keys, GUI_KEY_LST_CUT);
    sel->del = !!bit_tst_clr(ctx->keys, GUI_KEY_LST_DEL);
    sel->ret = !!bit_tst_clr(ctx->keys, GUI_KEY_LST_RET);
    sel->paste = !!bit_tst_clr(ctx->keys, GUI_KEY_LST_PASTE);
  }
}
/* ---------------------------------------------------------------------------
 *                                  View
 * ---------------------------------------------------------------------------
 */
static void
gui_lst_view(struct gui_lst_view *v, const struct gui_lst_lay *lay) {
  assert(v);
  assert(lay);
  const int total = max(1, v->total_cnt) - 1;
  if (lay->orient == GUI_VERTICAL) {
    double cnt = cast(double, max(1, lay->cnt[0]));
    v->cnt[1] = max(1, ceili(cast(double, total) / cnt));
    v->cnt[0] = lay->cnt[0];

    v->begin = lay->off_idx;
    v->end = v->begin + v->cnt[0] * (lay->page + 1);
    v->end = min(v->end, v->total_cnt);

    v->at[0] = lay->lay.x.min;
    v->at[1] = lay->lay.y.min + lay->off * lay->slot[1];
  } else {
    double cnt = cast(double, max(1, lay->cnt[1]));
    v->cnt[0] = max(1, ceili(cast(double, total) / cnt));
    v->cnt[1] = lay->cnt[1];

    v->begin = lay->off_idx;
    v->end = v->begin + v->cnt[1] * (lay->page + 1);
    v->end = min(v->end, v->total_cnt);

    v->at[0] = lay->lay.x.min + lay->off * lay->slot[0];
    v->at[1] = lay->lay.y.min;
  }
  v->total[0] = max(1, v->cnt[0]) * lay->slot[0] + lay->pad[0] * 2;
  v->total[1] = max(1, v->cnt[1]) * lay->slot[1] + lay->pad[1] * 2;

  v->max[0] = lay->lay.x.min + v->total[0];
  v->max[1] = lay->lay.y.min + v->total[1];
}

/* ---------------------------------------------------------------------------
 *                                  List
 * ---------------------------------------------------------------------------
 */
static void
gui_lst_cfg_init(struct gui_lst_cfg *cfg, int total_cnt, double off) {
  assert(cfg);
  memset(cfg, 0, sizeof(*cfg));
  cfg->total_cnt = total_cnt;

  cfg->lay.orient = GUI_VERTICAL;
  cfg->lay.scrl_mult = 3;
  cfg->lay.offset = off;

  cfg->ctl.focus = GUI_LST_FOCUS_ON_CLK;
  cfg->ctl.show_cursor = 1;

  cfg->sel.src = GUI_LST_SEL_SRC_INT;
  cfg->sel.bhv = GUI_LST_SEL_BHV_FLEETING;
  cfg->sel.mode = GUI_LST_SEL_SINGLE;
  cfg->sel.on = GUI_LST_SEL_ON_CLK;
  cfg->sel.hov = GUI_LST_SEL_HOV_YES;

  cfg->lay.pad[0] = cfg->lay.pad[1] = -1;
  cfg->lay.item[0] = cfg->lay.item[1] = -1;

  cfg->fltr.on = GUI_LST_FLTR_ON_ZERO;
  cfg->fltr.bitset = 0;
}
static void
gui_lst_begin(struct gui_ctx *ctx, struct gui_lst *lst,
              const struct gui_lst_cfg *cfg) {
  assert(ctx);
  assert(lst);
  assert(cfg);

  lst->id = ctx->id;
  lst->fltr_on = cfg->fltr.on;
  lst->fltr_bitset = cfg->fltr.bitset;

  lst->lay.box = lst->box;
  lst->lay.orient = cfg->lay.orient;
  lst->lay.offset = cfg->lay.offset;
  lst->lay.pad[0] = cfg->lay.pad[0] < 0 ? ctx->cfg.lst_pad[0] : cfg->lay.pad[0];
  lst->lay.pad[1] = cfg->lay.pad[1] < 0 ? ctx->cfg.lst_pad[1] : cfg->lay.pad[1];
  lst->lay.item[1] = cfg->lay.item[1] < 0 ? ctx->cfg.elm : cfg->lay.item[1];
  lst->lay.item[0] = cfg->lay.item[0] < 0 ? lst->box.x.ext : cfg->lay.item[0];

  lst->ctl.show_cursor = cfg->ctl.show_cursor;
  lst->ctl.focus = cfg->ctl.focus;

  lst->sel.src = cfg->sel.src;
  lst->sel.bhv = cfg->sel.bhv;
  lst->sel.mode = cfg->sel.mode;
  lst->sel.on = cfg->sel.on;
  lst->sel.hov = cfg->sel.hov;
  lst->sel.bitset = cfg->sel.bitset;
  lst->sel.cnt = cfg->sel.cnt;

  lst->view.total_cnt = cfg->total_cnt;
  if (lst->fltr_bitset) {
    switch (lst->fltr_on) {
      case GUI_LST_FLTR_ON_ZERO:
        lst->view.total_cnt = bit_cnt_set(lst->fltr_bitset, cfg->total_cnt, 0);
        break;
      case GUI_LST_FLTR_ON_ONE:
        lst->view.total_cnt = bit_cnt_zero(lst->fltr_bitset, cfg->total_cnt, 0);
        break;
    }
  }
  gui_lst_lay_init(&lst->lay);
  gui_lst_view(&lst->view, &lst->lay);

  lst->fltr_cnt = cfg->total_cnt;
  if (lst->fltr_bitset) {
    int win = lst->view.end - lst->view.begin;
    switch (lst->fltr_on) {
      case GUI_LST_FLTR_ON_ZERO:
        lst->view.begin =
            bit_set_at(lst->fltr_bitset, lst->fltr_cnt, 0, lst->view.begin);
        lst->view.end =
            bit_set_at(lst->fltr_bitset, lst->fltr_cnt, lst->view.begin, win);
        break;
      case GUI_LST_FLTR_ON_ONE:
        lst->view.begin =
            bit_zero_at(lst->fltr_bitset, lst->fltr_cnt, 0, lst->view.begin);
        lst->view.end =
            bit_zero_at(lst->fltr_bitset, lst->fltr_cnt, lst->view.begin, win);
        break;
    }
  }
  gui_lst_lay_apply_view(&lst->lay, &lst->view);
  lst->begin = lst->view.begin;
  lst->end = lst->view.end;
  lst->cnt = lst->end - lst->begin;
}
static void
gui_lst_begin_def(struct gui_ctx *ctx, struct gui_lst *lst, int total_cnt,
                  double off) {
  struct gui_lst_cfg cfg;
  gui_lst_cfg_init(&cfg, total_cnt, off);
  gui_lst_begin(ctx, lst, &cfg);
}
static int
gui_lst_nxt(const struct gui_lst *lst, int idx) {
  assert(lst);
  idx = idx + 1;
  if (lst->fltr_bitset) {
    switch (lst->fltr_on) {
      case GUI_LST_FLTR_ON_ZERO:
        return bit_ffs(lst->fltr_bitset, lst->fltr_cnt, idx);
      case GUI_LST_FLTR_ON_ONE:
        return bit_ffz(lst->fltr_bitset, lst->fltr_cnt, idx);
    }
  }
  return idx;
}
static void
gui_lst_elm_begin(struct gui_ctx *ctx, struct gui_lst *lst,
                  struct gui_panel *pan, struct gui_panel *p,
                  unsigned long long id, int sel) {
  assert(ctx);
  assert(pan);
  assert(lst);
  assert(p);

  ctx->id = id;
  gui_lst_lay_gen(&pan->box, &lst->lay);
  gui_lst_ctl_elm(ctx, &lst->ctl, &lst->lay, &pan->box, p, id);
  gui_lst_sel_elm(ctx, &lst->sel, &lst->lay, &lst->ctl, &pan->box, p, sel, id);
  gui_panel_begin(ctx, pan, p);
}
static void
gui_lst_elm_end(struct gui_ctx *ctx, struct gui_lst *lst,
                struct gui_panel *pan, struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(lst);
  unused(lst);
  gui_panel_end(ctx, pan, parent);
}
static void
gui_lst_end(struct gui_ctx *ctx, struct gui_lst *lst) {
  assert(ctx);
  assert(lst);

  const int act = lst->ctl.has_focus;
  const struct gui_lst_state *s = &ctx->lst_state;
  if (act && (s->cur_idx < lst->view.begin || s->cur_idx >= lst->view.end)) {
    /* dummy focus list element */
    struct gui_panel elm = {0};
    lst->lay.idx = s->cur_idx;
    unsigned long *tmp = lst->sel.bitset;
    lst->sel.bitset = 0;
    gui_lst_elm_begin(ctx, lst, &elm, &elm, 0xdeadbeefdeadbeef, 0);
    gui_lst_elm_end(ctx, lst, &elm, &elm);
    lst->sel.bitset = tmp;
  }
  gui_lst_ctl_proc(ctx, &lst->ctl, &lst->lay, lst->view.total_cnt);
  gui_lst_sel_proc(ctx, &lst->sel, &lst->ctl, lst->view.total_cnt);
  ctx->id = lst->id;
}
static void
gui_lst_set_sel_idx(struct gui_ctx *ctx, struct gui_lst *lst, int idx) {
  assert(ctx);
  assert(lst);
  if (!lst->ctl.has_focus) {
    return;
  }
  ctx->lst_state.sel_idx = idx;
  lst->sel.idx = idx;
  lst->sel.mod = 1;
}
static void
gui_lst_set_cur_idx(struct gui_ctx *ctx, struct gui_lst *lst, int idx) {
  assert(ctx);
  assert(lst);
  if (!lst->ctl.has_focus) {
    return;
  }
  ctx->lst_state.cur_idx = idx;
  lst->ctl.idx = idx;
  lst->ctl.mod = 1;
}

/* ---------------------------------------------------------------------------
 *                                  List-Area
 * ---------------------------------------------------------------------------
 */
static void
gui_lst_reg_begin(struct gui_ctx *ctx, struct gui_lst_reg *la,
                   struct gui_panel *parent, const struct gui_lst_cfg *cfg,
                   const double *off) {
  assert(la);
  assert(off);
  assert(cfg);
  assert(ctx);
  assert(parent);

  la->reg.box = la->box;
  gui_reg_begin(ctx, &la->reg, parent, off);

  la->lst.box = la->reg.pan.box;
  gui_lst_begin(ctx, &la->lst, cfg);
  gui_reg_apply_lst(&la->reg, &la->lst, cfg->lay.scrl_mult);
}
static int
gui_lst_reg_nxt(const struct gui_lst_reg *la, int idx) {
  return gui_lst_nxt(&la->lst, idx);
}
static void
gui_lst_reg_elm_begin(struct gui_ctx *ctx, struct gui_lst_reg *la,
                        struct gui_panel *pan, unsigned long long id, int sel) {
  assert(la);
  assert(ctx);
  assert(pan);
  gui_lst_elm_begin(ctx, &la->lst, pan, &la->reg.pan, id, sel);
}
static void
gui_lst_reg_elm_end(struct gui_ctx *ctx, struct gui_lst_reg *la,
                      struct gui_panel *pan) {
  assert(la);
  assert(ctx);
  assert(pan);
  gui_lst_elm_end(ctx, &la->lst, pan, &la->reg.pan);
}
static void
gui_lst_reg_elm_txt(struct gui_ctx *ctx, struct gui_lst_reg *reg,
                     struct gui_panel *elm, unsigned long long id, int is_sel,
                     struct str txt, const char *icon,
                     const struct gui_align *align) {
  assert(ctx);
  assert(reg);
  assert(elm);
  gui_lst_reg_elm_begin(ctx, reg, elm, id, is_sel);
  {
    struct gui_panel item = {.box = elm->box};
    if (icon) {
      gui_icon_box(ctx, &item, elm, icon, txt);
    } else {
      gui_txt(ctx, &item, elm, txt, align);
    }
  }
  gui_lst_reg_elm_end(ctx, reg, elm);
}
static void
gui_lst_reg_center(struct gui_lst_reg *la, int idx) {
  assert(la);
  const struct gui_lst_lay *lst = &la->lst.lay;
  if (lst->orient == GUI_VERTICAL) {
    la->reg.off[1] = gui_lst_lay_center(&la->lst.lay, idx);
  } else {
    la->reg.off[0] = gui_lst_lay_center(&la->lst.lay, idx);
  }
}
static void
gui_lst_reg_fit_start(struct gui_lst_reg *la, int idx) {
  assert(la);
  const struct gui_lst_lay *lst = &la->lst.lay;
  if (lst->orient == GUI_VERTICAL) {
    la->reg.off[1] = gui_lst_lay_fit_start(&la->lst.lay, idx);
  } else {
    la->reg.off[0] = gui_lst_lay_fit_start(&la->lst.lay, idx);
  }
}
static void
gui_lst_reg_fit_end(struct gui_lst_reg *la, int idx) {
  assert(la);
  const struct gui_lst_lay *lst = &la->lst.lay;
  if (lst->orient == GUI_VERTICAL) {
    la->reg.off[1] = gui_lst_lay_fit_end(&la->lst.lay, idx);
  } else {
    la->reg.off[0] = gui_lst_lay_fit_end(&la->lst.lay, idx);
  }
}
static void
gui_lst_reg_clamp(struct gui_lst_reg *la, int idx) {
  assert(la);
  const struct gui_lst_lay *lst = &la->lst.lay;
  if (lst->orient == GUI_VERTICAL) {
    la->reg.off[1] = gui_lst_lay_clamp(&la->lst.lay, idx);
  } else {
    la->reg.off[0] = gui_lst_lay_clamp(&la->lst.lay, idx);
  }
}
static void
gui_lst_reg_end(struct gui_ctx *ctx, struct gui_lst_reg *la,
                struct gui_panel *parent, double *off) {
  assert(la);
  assert(ctx);
  assert(off);
  gui_lst_end(ctx, &la->lst);
  if (la->lst.ctl.was_scrolled) {
    switch (la->lst.ctl.scrl) {
      case GUI_LST_FIT_ITEM_START:
        gui_lst_reg_fit_start(la, la->lst.ctl.item_idx);
        break;
      case GUI_LST_FIT_ITEM_END:
        gui_lst_reg_fit_end(la, la->lst.ctl.item_idx);
        break;
      case GUI_LST_CLAMP_ITEM:
        gui_lst_reg_clamp(la, la->lst.ctl.item_idx);
        break;
    }
  }
  gui_reg_end(ctx, &la->reg, parent, off);
}
/* ---------------------------------------------------------------------------
 *                                  Tree-Node
 * ---------------------------------------------------------------------------
 */
static void
gui_tree_node_icon_drw(struct gui_ctx *ctx, const struct gui_panel *pan,
                       int is_open) {
  assert(ctx);
  assert(pan);
  const int img = is_open ? GUI_IMG_MINUS : GUI_IMG_PLUS;
  int cur_siz = gui_img_def[img].w;

  struct gui_box cur = {0};
  cur.x = gui_mid_ext(pan->box.x.mid, cur_siz);
  cur.y = gui_mid_ext(pan->box.y.mid, cur_siz);
  gui__img(ctx, ctx->sys, cur.x.min, cur.y.min, img);
}
static int
gui_tree_node_icon(struct gui_ctx *ctx, struct gui_panel *pan,
                   struct gui_panel *parent, int is_open) {
  assert(ctx);
  assert(pan);
  assert(parent);

  gui_panel_begin(ctx, pan, parent);
  if (ctx->pass == GUI_RENDER && pan->state != GUI_HIDDEN) {
    gui_tree_node_icon_drw(ctx, pan, is_open);
  }
  gui_panel_end(ctx, pan, parent);

  struct gui_input in = {0};
  gui_input(&in, ctx, pan, GUI_BTN_LEFT);
  return (in.mouse.btn.left.pressed);
}
static void
gui_tree_node_begin(struct gui_ctx *ctx, struct gui_tree_node *node,
                    struct gui_panel *parent, int depth) {
  assert(ctx);
  assert(node);
  assert(parent);

  struct gui_panel *pan = &node->pan;
  const int off = depth * ctx->cfg.depth;
  pan->box.x = gui_min_max(parent->box.x.min + off, parent->box.x.max);
  pan->box.y = gui_min_max(parent->box.y.min, parent->box.y.max);
  gui_panel_begin(ctx, pan, parent);

  struct gui_panel ico = {.box = pan->box};
  int cur_siz = gui_img_def[GUI_IMG_PLUS].w;
  ico.box.x = gui_min_ext(pan->box.x.min, ctx->cfg.item);
  ico.box.x = gui_mid_ext(ico.box.x.mid, cur_siz);
  ico.box.y = gui_mid_ext(ico.box.y.mid, cur_siz);

  switch (node->type) {
    case GUI_TREE_NODE: {
      if (gui_tree_node_icon(ctx, &ico, parent, node->open)) {
        node->open = !node->open;
        node->changed = 1;
      }
    } break;
  }
  node->box.x = gui_min_max(ico.box.x.max + ctx->cfg.gap[0], pan->box.x.max);
  node->box.y = ico.box.y;
}
static void
gui_tree_node_end(struct gui_ctx *ctx, struct gui_tree_node *node,
                  struct gui_panel *parent) {
  assert(ctx);
  assert(node);

  gui_panel_end(ctx, &node->pan, parent);
  if (!node->changed) {
    struct gui_input in = {0};
    gui_input(&in, ctx, &node->pan, GUI_BTN_LEFT);
    if (in.mouse.btn.left.doubled) {
      node->open = !node->open;
      node->changed = 1;
    }
  }
}
static void
gui_tree_node(struct gui_ctx *ctx, struct gui_tree_node *node,
              struct gui_panel *parent, int depth, struct str txt) {
  assert(ctx);
  assert(node);
  assert(parent);
  static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};

  gui_tree_node_begin(ctx, node, parent, depth);
  struct gui_panel lbl = {.box = node->box};
  gui_txt(ctx, &lbl, &node->pan, txt, &align);
  gui_tree_node_end(ctx, node, parent);
}

/* ---------------------------------------------------------------------------
 *                                  Splitter
 * ---------------------------------------------------------------------------
 */
static void
gui__split_toc(struct gui_split_toc *s, int *state) {
  assert(s);
  assert(state);

  s->col_cnt = state[0];
  s->lay_cnt = state[1];
  s->slot = state + 2;
  s->cons = s->slot + (s->col_cnt << 1);
  s->seps = s->cons + (s->col_cnt << 2);
}
static void
gui_split_lay_begin(struct gui_split_lay *bld, int *state, int cnt,
                    int sep_size) {
  assert(bld);
  assert(state);

  bld->idx = 0;
  bld->cnt = cnt;
  bld->sep_size = sep_size;

  state[0] = cnt;
  state[1] = cnt << 1;
  gui__split_toc(&bld->toc, state);
}
static void
gui_split_lay_add(struct gui_split_lay *bld, int type, int val, const int *con) {
  assert(bld);
  assert(con);
  assert(bld->idx < bld->cnt);

  int dst = bld->idx++;
  bld->toc.slot[(dst << 1) + 0] = type == GUI_LAY_SLOT_DYN ? -val : val;
  bld->toc.slot[(dst << 1) + 1] = bld->sep_size;
  bld->toc.cons[(dst << 2) + 0] = max(0, con[0]);
  bld->toc.cons[(dst << 2) + 1] = con[1] < 0 ? 16 * 1024 : con[1];
  bld->toc.cons[(dst << 2) + 2] = bld->sep_size;
  bld->toc.cons[(dst << 2) + 3] = bld->sep_size;
}
static void
gui_split_lay_end(struct gui_split_lay *bld) {
  assert(bld);
  memset(bld->toc.seps, 0, sizeof(int) * cast(size_t, bld->idx << 1));
}
static void
gui_split_lay(int *state, const struct gui_ctx *ctx,
              const struct gui_split_lay_cfg *cfg) {
  int cnt = !cfg->total ? cfg->cnt : cfg->total;
  int stride = cfg->size ? cfg->size : szof(struct gui_split_lay_slot);
  const unsigned char *p = cast(const unsigned char*, cfg->slots) + cfg->off;

  struct gui_split_lay bld = {0};
  gui_split_lay_begin(&bld, state, cnt, ctx->cfg.sep);
  for_cnt(i, cnt) {
    int idx = cfg->sort ? cfg->sort[i] : i;
    if (cfg->fltr && bit_tst(cfg->fltr, idx) == 0) {
      continue;
    }
    const struct gui_split_lay_slot *s = cast(const void*, p + stride * idx);
    gui_split_lay_add(&bld, s->type, s->size, s->con);
  }
  gui_split_lay_end(&bld);
}
static void
gui__split_gen(struct gui_box *item, struct gui_split *spt, const int *siz) {
  switch (spt->orient) {
    case GUI_VERTICAL:
      *item = gui_cut_top(&spt->lay, siz[spt->idx], 0);
      break;
    case GUI_HORIZONTAL:
      *item = gui_cut_lhs(&spt->lay, siz[spt->idx], 0);
      break;
  }
  spt->idx++;
}
static void
gui_split_begin(struct gui_ctx *ctx, struct gui_split *spt,
                struct gui_panel *parent, enum gui_split_type typ,
                enum gui_orient orient, int *items, int *state) {
  assert(ctx);
  assert(spt);
  assert(items);
  assert(state);
  assert(parent);
  assert(state[0] > 0);

  int siz = 0;
  struct gui_split_toc toc = {0};
  gui__split_toc(&toc, state);

  spt->sidx = 2;
  spt->typ = typ;
  spt->orient = orient;
  spt->pan.box = spt->box;
  spt->lay = spt->box;

  switch (orient) {
    case GUI_VERTICAL: {
      siz = spt->box.y.ext;
      if (spt->typ == GUI_SPLIT_FIT) {
        spt->lay.y = gui_min_max(spt->lay.y.min, INT_MAX);
      }
    } break;
    case GUI_HORIZONTAL: {
      siz = spt->box.x.ext;
      if (spt->typ == GUI_SPLIT_FIT) {
        spt->lay.x = gui_min_max(spt->lay.x.min, INT_MAX);
      }
    } break;
  }
  spt->cnt = toc.lay_cnt;
  const int cnt_half = toc.col_cnt;
  if ((spt->typ == GUI_SPLIT_FIT && siz != toc.seps[1]) ||
      spt->cnt != toc.seps[0]) {
    /* calculate separator positions from slot definitions */
    gui_solve(items, siz, toc.slot, toc.lay_cnt, 0, toc.cons, &spt->sol);
    int *def = toc.seps + 1;
    toc.seps[0] = spt->cnt;
    toc.seps[1] = siz,
    toc.seps[2] = items[0];
    for (int i = 1; i < cnt_half; i++) {
      def[i + 1] = def[i] + items[(i - 1) * 2 + 1] + items[i * 2];
    }
  } else {
    /* restore panel size from separator positions */
    const int *def = toc.seps + 1;
    for (int i = 0, off = 0; i < cnt_half; ++i) {
      items[i * 2] = def[i + 1] - off;
      items[i * 2 + 1] = toc.slot[i * 2 + 1];
      off = def[i + 1] + items[i * 2 + 1];
    }
  }
  const int lay_siz = toc.seps[cnt_half + 1];
  if (parent) {
    switch (orient) {
      case GUI_VERTICAL:
        parent->max[1] = max(parent->max[1], parent->box.y.min + lay_siz);
        break;
      case GUI_HORIZONTAL:
        parent->max[0] = max(parent->max[0], parent->box.x.min + lay_siz);
        break;
    }
  }
  gui__split_gen(&spt->item, spt, items);
  gui_panel_begin(ctx, &spt->pan, parent);
}
static void
gui_split_sep(struct gui_ctx *ctx, struct gui_split *spt,
              const int *lay, int *split_state) {
  assert(ctx);
  assert(spt);
  assert(split_state);
  struct gui_split_toc toc = {0};
  gui__split_toc(&toc, split_state);

  struct gui_btn sep = {0};
  gui__split_gen(&sep.box, spt, lay);

  const int sep_pos = toc.seps[spt->sidx];
  const int cnt_half = (spt->cnt >> 1) + (spt->cnt & 0x01);
  if (gui_sep(ctx, &sep, &spt->pan, spt->orient, &toc.seps[spt->sidx])) {
    if (toc.cons) { /* apply (min,max) constraints for both panes */
      int prv = spt->sidx > 2 ? toc.seps[spt->sidx - 1] : 0;
      int fst = toc.seps[spt->sidx] - prv;
      int fci = (spt->sidx - 2) * 2;
      const int *fc = toc.cons + fci * 2;

      int nxt = spt->sidx + 1 < (cnt_half) + 2 ? toc.seps[spt->sidx + 1] : sep_pos;
      int sec = nxt - toc.seps[spt->sidx];
      int sci = (spt->sidx - 2) * 2 + 2;
      const int *sc = toc.cons + sci * 2;
      if (!between(fst, fc[0], fc[1]) || !between(sec, sc[0], sc[1])) {
        toc.seps[spt->sidx] = sep_pos;
      }
    }
    switch (spt->typ) {
      case GUI_SPLIT_EXP: {
        int delta = toc.seps[spt->sidx] - sep_pos;
        for (int i = spt->sidx + 1; i < cnt_half + 2; ++i) {
          toc.seps[i] += delta;
        }
      } break;
      case GUI_SPLIT_FIT: {
        int val = toc.seps[spt->sidx];
        int min = spt->sidx > 2 ? toc.seps[spt->sidx - 1] : 0;
        int max = spt->sidx + ((1 < cnt_half) ? toc.seps[spt->sidx + 1] : sep_pos);
        toc.seps[spt->sidx] = clamp(min, val, max);
      } break;
    }
  }
  gui__split_gen(&spt->item, spt, lay);
  spt->sidx++;
}
static void
gui_split_end(struct gui_ctx *ctx, struct gui_split *spt, struct gui_panel *parent) {
  assert(ctx);
  assert(spt);

  gui_panel_end(ctx, &spt->pan, parent);
  switch (spt->orient) {
    case GUI_VERTICAL:
      spt->pan.box.y.max = spt->pan.max[1];
      break;
    case GUI_HORIZONTAL:
      spt->pan.box.x.max = spt->pan.max[0];
      break;
  }
}

/* ---------------------------------------------------------------------------
 *                                  Table
 * ---------------------------------------------------------------------------
 */
static void
gui_tbl_begin(struct gui_ctx *ctx, struct gui_tbl *tbl,
              struct gui_panel *parent, const double *off,
              const struct gui_tbl_sort *sort) {
  assert(ctx);
  assert(tbl);
  assert(off);
  assert(parent);

  tbl->idx = 0;
  if (sort) {
    tbl->sort = *sort;
  }
  tbl->reg.box = tbl->box;
  gui_reg_begin(ctx, &tbl->reg, parent, off);

  tbl->pan = &tbl->reg.pan;
  tbl->hdr = &tbl->spt.pan;
}
static void
gui_tbl_hdr_begin(struct gui_ctx *ctx, struct gui_tbl *tbl, int *items, int *s) {
  assert(ctx);
  assert(tbl);

  struct gui_panel *pan = tbl->pan;
  struct gui_split_toc toc = {0};
  gui__split_toc(&toc, s);

  tbl->cnt = toc.col_cnt;
  tbl->hdr_h = ctx->cfg.item;

  int offx = floori(tbl->reg.off[0]);
  int offy = floori(tbl->reg.off[1]);
  tbl->spt.box.x = gui_min_ext(pan->box.x.min, pan->box.x.ext + offx);
  tbl->spt.box.y = gui_min_ext(pan->box.y.min + offy, ctx->cfg.item);
  gui_split_begin(ctx, &tbl->spt, pan, GUI_SPLIT_EXP, GUI_HORIZONTAL, items, s);
}
static void
gui_tbl_hdr_slot_begin(struct gui_ctx *ctx, struct gui_tbl *tbl,
                       struct gui_btn *slot) {
  assert(ctx);
  assert(tbl);
  assert(slot);

  slot->box = tbl->spt.item;
  gui_btn_begin(ctx, slot, &tbl->spt.pan);
  slot->box.x = gui_shrink(&slot->box.x, ctx->cfg.pan_pad[0]);
}
static void
gui_tbl_hdr_slot_end(struct gui_ctx *ctx, struct gui_tbl *tbl,
                     const int *lay, struct gui_btn *slot, int *state) {
  assert(ctx);
  assert(tbl);
  assert(slot);

  gui_btn_end(ctx, slot, &tbl->spt.pan);
  if (tbl->idx < tbl->cnt) {
    gui_split_sep(ctx, &tbl->spt, lay, state);
  }
  if (slot->clk) {
    tbl->resort = 1;
    if (tbl->sort.col == tbl->idx) {
      tbl->sort.order = !tbl->sort.order;
    } else {
      tbl->sort.col = tbl->idx, tbl->sort.order = 0;
    }
  }
  tbl->idx++;
}
static void
gui_tbl_hdr_slot(struct gui_ctx *ctx, struct gui_tbl *tbl, const int *lay,
                int *state, struct str txt) {
  assert(ctx);
  assert(tbl);
  struct gui_btn slot = {0};
  gui_tbl_hdr_slot_begin(ctx, tbl, &slot);
  {
    static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
    struct gui_panel pan = {.box = slot.box};
    gui_txt(ctx, &pan, &slot.pan, txt, &align);
  }
  gui_tbl_hdr_slot_end(ctx, tbl, lay, &slot, state);
}
static void
gui_tbl_hdr_end(struct gui_ctx *ctx, struct gui_tbl *tbl) {
  assert(ctx);
  assert(tbl);
  gui_split_end(ctx, &tbl->spt, tbl->pan);
}
static void
gui_tbl_lst_cfg_init(struct gui_ctx *ctx, struct gui_tbl_lst_cfg *cfg,
                     int total_cnt) {
  assert(ctx);
  assert(cfg);
  memset(cfg, 0, sizeof(*cfg));
  cfg->total_cnt = total_cnt;
  cfg->ctl.show_cursor = 1;

  cfg->lay.pad[0] = cfg->lay.pad[1] = -1;
  cfg->lay.gap[0] = cfg->lay.gap[1] = -1;
  cfg->lay.item_size = ctx->cfg.elm;
  cfg->lay.scrl_mult = 3;

  cfg->sel.src = GUI_LST_SEL_SRC_INT;
  cfg->sel.mode = GUI_LST_SEL_SINGLE;
  cfg->sel.bhv = GUI_LST_SEL_BHV_FLEETING;
  cfg->sel.on = GUI_LST_SEL_ON_CLK;
  cfg->sel.hov = GUI_LST_SEL_HOV_YES;

  cfg->fltr.on = GUI_LST_FLTR_ON_ZERO;
  cfg->fltr.bitset = 0;
}
static void
gui__tbl_lst_cfg_init(struct gui_lst_cfg *ret, const struct gui_ctx *ctx,
                      const struct gui_tbl_lst_cfg *cfg, int w, double off) {
  gui_lst_cfg_init(ret, cfg->total_cnt, off);
  int lst_pad = cfg->lay.pad[0] < 0 ? ctx->cfg.lst_pad[0] : ret->lay.pad[0];

  ret->lay.orient = GUI_VERTICAL;
  ret->lay.pad[0] = cfg->lay.pad[0];
  ret->lay.pad[1] = cfg->lay.pad[1];
  ret->lay.gap[0] = cfg->lay.gap[0];
  ret->lay.gap[1] = cfg->lay.gap[1];
  ret->lay.item[0] = w;
  ret->lay.item[0] -= lst_pad * 2;
  ret->lay.item[1] = cfg->lay.item_size;
  ret->lay.scrl_mult = cfg->lay.scrl_mult;

  ret->ctl.focus = cfg->ctl.focus;
  ret->ctl.show_cursor = cfg->ctl.show_cursor;

  ret->sel.src = cfg->sel.src;
  ret->sel.mode = cfg->sel.mode;
  ret->sel.bhv = cfg->sel.bhv;
  ret->sel.on = cfg->sel.on;
  ret->sel.cnt = cfg->sel.cnt;
  ret->sel.hov = cfg->sel.hov;
  ret->sel.bitset = cfg->sel.bitset;

  ret->fltr.bitset = cfg->fltr.bitset;
  ret->fltr.on = cfg->fltr.on;
}
static void
gui_tbl_lst_begin(struct gui_ctx *ctx, struct gui_tbl *tbl,
                  const struct gui_tbl_lst_cfg *cfg) {
  assert(ctx);
  assert(tbl);
  assert(cfg);
  struct gui_panel *pan = tbl->pan;

  struct gui_box lst = {0};
  int lst_top = pan->box.y.min + tbl->hdr_h;
  lst.x = gui_min_max(pan->box.x.min, tbl->spt.pan.max[0]);
  lst.y = gui_min_max(lst_top, pan->box.y.max - ctx->cfg.scrl);

  struct gui_lst_cfg lst_cfg = {0};
  gui__tbl_lst_cfg_init(&lst_cfg, ctx, cfg, lst.x.ext, tbl->reg.off[1]);

  tbl->lst.box = lst;
  gui_lst_begin(ctx, &tbl->lst, &lst_cfg);
  gui_reg_apply_lst(&tbl->reg, &tbl->lst, cfg->lay.scrl_mult);

  int cx = lst.x.min + floori(tbl->reg.off[0]);
  int cy = tbl->lst.box.y.min + floori(tbl->lst.lay.offset) + cfg->lay.pad[1];
  gui_clip_begin(&tbl->clip, ctx, cx, cy, cx + lst.x.ext, cy + lst.y.ext);

  tbl->lst.begin = tbl->lst.view.begin;
  tbl->lst.end = tbl->lst.view.end;
  tbl->lst.cnt = tbl->lst.end - tbl->lst.begin;
}
static void
gui_tbl_lst_begin_def(struct gui_ctx *ctx, struct gui_tbl *tbl, int total_cnt) {
  struct gui_tbl_lst_cfg cfg;
  gui_tbl_lst_cfg_init(ctx, &cfg, total_cnt);
  gui_tbl_lst_begin(ctx, tbl, &cfg);
}
static void
gui_tbl_lst_elm_begin(struct gui_ctx *ctx, struct gui_tbl *tbl,
                      struct gui_panel *elm, unsigned long long id, int sel) {
  assert(ctx);
  assert(tbl);
  assert(elm);

  tbl->idx = 0;
  struct gui_panel *pan = tbl->pan;
  gui_lst_elm_begin(ctx, &tbl->lst, elm, pan, id, sel);
  tbl->col_lay = elm->box;
}
static void
gui_tbl_lst_elm_col(struct gui_box *box, struct gui_ctx *ctx,
                    struct gui_tbl *tbl, const int *lay) {
  assert(box);
  assert(ctx);
  assert(tbl);
  unused(ctx);

  struct gui_box sep = {0};
  if (tbl->idx) {
    gui_clip_end(ctx, &tbl->col_clip);
    sep = gui_cut_lhs(&tbl->col_lay, lay[((tbl->idx - 1) << 1) + 1], 0);
  }
  *box = gui_cut_lhs(&tbl->col_lay, lay[(tbl->idx << 1)], 0);
  gui_clip_begin(&tbl->col_clip, ctx, gui_unbox(box));
  tbl->idx++;
}
static void
gui_tbl_lst_elm_end(struct gui_ctx *ctx, struct gui_tbl *tbl,
                    struct gui_panel *elm) {
  assert(ctx);
  assert(tbl);
  assert(elm);
  if (tbl->idx) {
    gui_clip_end(ctx, &tbl->col_clip);
  }
  gui_lst_elm_end(ctx, &tbl->lst, elm, tbl->pan);
}
static void
gui_tbl_lst_end(struct gui_ctx *ctx, struct gui_tbl *tbl) {
  assert(ctx);
  assert(tbl);

  gui_clip_end(ctx, &tbl->clip);
  gui_lst_end(ctx, &tbl->lst);
  if (tbl->lst.ctl.was_scrolled) {
    switch (tbl->lst.ctl.scrl) {
      case GUI_LST_FIT_ITEM_START: {
        double off = gui_lst_lay_fit_start(&tbl->lst.lay, tbl->lst.ctl.item_idx);
        tbl->reg.off[1] = off;
      } break;
      case GUI_LST_FIT_ITEM_END: {
        double off = gui_lst_lay_fit_end(&tbl->lst.lay, tbl->lst.ctl.item_idx);
        tbl->reg.off[1] = off;
      } break;
      case GUI_LST_CLAMP_ITEM: {
        double off = gui_lst_lay_clamp(&tbl->lst.lay, tbl->lst.ctl.item_idx);
        tbl->reg.off[1] = off;
      }break;
    }
  }
}
static void
gui_tbl_end(struct gui_ctx *ctx, struct gui_tbl *tbl,
            struct gui_panel *parent, double *off) {
  assert(ctx);
  assert(tbl);
  assert(off);
  gui_reg_end(ctx, &tbl->reg, parent, off);
}
static void
gui_tbl_lst_txt(struct gui_ctx *ctx, struct gui_tbl *tbl, const int *lay,
                struct gui_panel *elm, struct str txt,
                const char *icon, const struct gui_align *align) {
  assert(ctx);
  assert(tbl);
  assert(elm);

  struct gui_panel item = {0};
  gui_tbl_lst_elm_col(&item.box, ctx, tbl, lay);
  if (icon) {
    gui_icon_box(ctx, &item, elm, icon, txt);
  } else {
    gui_txt(ctx, &item, elm, txt, align);
  }
}
static void
gui_tbl_lst_txtf(struct gui_ctx *ctx, struct gui_tbl *tbl, const int *lay,
                 struct gui_panel *elm, const struct gui_align *align,
                 const char *fmt, ...) {
  assert(ctx);
  assert(tbl);
  assert(elm);

  struct gui_panel item = {0};
  gui_tbl_lst_elm_col(&item.box, ctx, tbl, lay);

  va_list args;
  va_start(args, fmt);
  gui_txtvf(ctx, &item, elm, align, fmt, args);
  va_end(args);
}
static void
gui_tbl_lst_tm(struct gui_ctx *ctx, struct gui_tbl *tbl, const int *lay,
               struct gui_panel *elm, const char *fmt, struct tm *tm) {
  assert(ctx);
  assert(tbl);
  assert(elm);
  assert(fmt);

  struct gui_panel item = {0};
  gui_tbl_lst_elm_col(&item.box, ctx, tbl, lay);
  gui_tm(ctx, &item, elm, fmt, tm);
}

/* ---------------------------------------------------------------------------
 *                                  Tab
 * ---------------------------------------------------------------------------
 */
static void
gui_tab_ctl_begin(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                  struct gui_panel *parent, int cnt, int sel_idx) {
  assert(ctx);
  assert(tab);
  assert(parent);

  tab->tab_w = ctx->cfg.tab;
  tab->off = tab->box.x.min;
  tab->total = cnt;
  tab->sel.idx = sel_idx;
  tab->sel.mod = 0;

  tab->hdr.x = tab->box.x;
  tab->hdr.y = gui_min_ext(tab->box.y.min, ctx->cfg.item);
  tab->cnt = min(tab->total, (tab->hdr.x.ext - ctx->cfg.scrl) / tab->tab_w);
  tab->at = tab->hdr.y.max;

  int gap = ctx->cfg.pan_gap[0];
  tab->bdy.x = gui_shrink(&tab->hdr.x, ctx->cfg.pan_pad[0]);
  tab->bdy.y = gui_min_max(tab->hdr.y.max + gap, tab->box.y.max);
  tab->bdy.y = gui_shrink(&tab->bdy.y, ctx->cfg.pan_gap[1]);

  tab->pan.box = tab->box;
  gui_panel_begin(ctx, &tab->pan, parent);
  if (tab->show_btn) {
    /* open list button */
    gui_disable(ctx, cnt <= 1);
    tab->btn.box = tab->hdr;
    tab->btn.box.x = gui_min_ext(tab->off, ctx->cfg.scrl);
    gui_btn_begin(ctx, &tab->btn, &tab->pan);
    {
      struct gui_panel arr = {0};
      arr.box = gui_box_mid_ext(&tab->btn.pan.box, 5, 3);
      gui_arrow(ctx, &arr, &tab->btn.pan, GUI_SOUTH);
    }
    gui_btn_end(ctx, &tab->btn, &tab->pan);
    gui_enable(ctx, cnt <= 1);

    tab->hdr.x = gui_min_max(tab->btn.box.x.max, tab->hdr.x.max);
    tab->off = tab->hdr.x.min;
  }
}
static void
gui_tab_ctl_sel(struct gui_ctx *ctx, struct gui_tab_ctl *tab, int idx) {
  assert(ctx);
  assert(tab);
  assert(idx < tab->total);

  tab->sel.mod = 1;
  tab->sel.idx = 0;
  tab->sort.mod = 1;
  tab->sort.dst = 0;
  tab->sort.src = idx;
}
static void
gui_tab_hdr_begin(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                  struct gui_tab_ctl_hdr *hdr) {
  assert(ctx);
  assert(tab);
  assert(hdr);
  hdr->pan.box = hdr->box;
  gui_panel_begin(ctx, &hdr->pan, &tab->pan);
}
static void
gui_tab_hdr_end(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                struct gui_tab_ctl_hdr *hdr) {
  assert(ctx);
  assert(hdr);
  gui_panel_end(ctx, &hdr->pan, &tab->pan);
}
static void
gui__tab_hdr_slot_drw(struct gui_ctx *ctx, const struct gui_tab_ctl *tab,
                      struct gui_panel *slot, int is_act) {
  assert(ctx);
  assert(tab);
  assert(slot);

  const struct gui_box *p = &slot->box;
  int top = p->y.min + ((!is_act) ? 1 : 0);
  int height = p->y.ext - ((!is_act) ? 1 : 0);

  struct sys *sys = ctx->sys;
  sys->ren.drw.line_style(ctx->ren, 1);
  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_BG]);
  sys->ren.drw.box(ctx->ren, p->x.min, top, p->x.max, top + height);

  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_LIGHT]);
  sys->ren.drw.hln(ctx->ren, top, p->x.min, p->x.max - 1);
  if (!is_act) {
    sys->ren.drw.hln(ctx->ren, p->y.max, p->x.min, p->x.max);
  }
  if (tab->idx == 0 || (tab->idx - 1) != tab->sel.idx) {
    sys->ren.drw.vln(ctx->ren, p->x.min, top, p->y.max);
  }
  if (tab->sel.idx != (tab->idx + 1)) {
    sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_SHADOW]);
    sys->ren.drw.vln(ctx->ren, p->x.max - 2, top, p->y.max - 1);
  }
}
static void
gui_tab_hdr_slot_begin(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                       struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot,
                       unsigned long long id) {
  assert(ctx);
  assert(tab);
  assert(slot);

  int is_act = (tab->sel.idx == tab->idx);
  hdr->slot.x = gui_min_ext(tab->off, tab->tab_w);
  hdr->slot.y = hdr->pan.box.y;

  slot->box = hdr->slot;
  slot->focusable = is_act != 0;
  if (ctx->pass == GUI_RENDER) {
    gui__tab_hdr_slot_drw(ctx, tab, slot, is_act);
  }
  hdr->id = id;
  tab->off = slot->box.x.max;
  slot->box.x = gui_shrink(&slot->box.x, ctx->cfg.pan_pad[0]);
  hdr->slot = slot->box;
}
static void
gui_tab_hdr_slot_end(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                     struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot,
                     struct gui_input *in) {
  assert(ctx);
  assert(tab);
  assert(slot);
  unused(hdr);

  struct gui_input ins = {0};
  in = !in ? &ins : in;
  slot->id = fnv1au64(hdr->id, tab->pan.id);
  gui_panel_hot(ctx, slot, &tab->pan);
  gui_input(in, ctx, slot, GUI_BTN_LEFT);

  /* selection */
  if (in->mouse.btn.left.pressed) {
    if (tab->sel.idx != tab->idx) {
      tab->sel.mod = 1;
    }
    tab->sel.idx = tab->idx;
    gui_focus(ctx, slot);
  }
  if (slot->state == GUI_FOCUSED) {
    if (bit_tst_clr(ctx->keys, GUI_KEY_TAB_NEXT)) {
      tab->sel.idx = tab->sel.idx + 1;
      tab->sel.mod = 1;
    } else if (bit_tst_clr(ctx->keys, GUI_KEY_TAB_PREV)) {
      tab->sel.idx = max(0, tab->sel.idx - 1);
      tab->sel.mod = 1;
    }
  }
  /* resorting */
  if (in->mouse.btn.left.dragged) {
    int min_x = hdr->slot.x.min - (hdr->slot.x.ext >> 1);
    int max_x = hdr->slot.x.max + (hdr->slot.x.ext >> 1);
    if (tab->idx > 0 && in->mouse.pos[0] < min_x) {
      tab->sort.mod = 1;
      tab->sort.dst = tab->idx - 1;
      tab->sort.src = tab->idx;

      tab->sel.mod = 1;
      tab->sel.idx = tab->sort.dst;
    } else if (tab->idx + 1 < tab->total && in->mouse.pos[0] > max_x) {
      tab->sort.mod = 1;
      tab->sort.dst = tab->idx + 1;
      tab->sort.src = tab->idx;

      tab->sel.mod = 1;
      tab->sel.idx = tab->sort.dst;
    }
  }
  if (ctx->pass == GUI_RENDER &&
      slot->id == ctx->focused && !ctx->focus_next) {
    gui_focus_drw(ctx, &hdr->slot, 0);
  }
  tab->idx++;
}
static void
gui_tab_hdr_slot_id(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                    struct gui_tab_ctl_hdr *hdr, unsigned long long id,
                    struct str txt) {
  assert(ctx);
  assert(tab);
  assert(hdr);
  static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};

  struct gui_panel slot = {0};
  gui_tab_hdr_slot_begin(ctx, tab, hdr, &slot, id);
  gui_txt(ctx, &slot, &hdr->pan, txt, &align);
  gui_tab_hdr_slot_end(ctx, tab, hdr, &slot, 0);
}
static void
gui_tab_hdr_slot(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                    struct gui_tab_ctl_hdr *hdr, struct str txt) {
  assert(ctx);
  assert(tab);
  assert(hdr);
  gui_tab_hdr_slot_id(ctx, tab, hdr, str_hash(txt), txt);
}
static void
gui__tab_ctl_drw(struct gui_ctx *ctx, const struct gui_tab_ctl *tab,
                 const struct gui_panel *pan) {
  assert(ctx);
  assert(tab);
  assert(pan);

  struct sys *sys = ctx->sys;
  const struct gui_box *b = &pan->box;
  sys->ren.drw.line_style(ctx->ren, 1);

  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_LIGHT]);
  sys->ren.drw.hln(ctx->ren, tab->at, tab->off, b->x.max - 1);
  sys->ren.drw.vln(ctx->ren, b->x.min, tab->at, b->y.max - 1);

  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_SHADOW]);
  sys->ren.drw.hln(ctx->ren, b->y.max - 2, b->x.min + 1, b->x.max - 2);
  sys->ren.drw.vln(ctx->ren, b->x.max - 2, tab->at + 1, b->y.max - 2);
}
static void
gui_tab_ctl_end(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                struct gui_panel *parent) {
  assert(ctx);
  assert(tab);
  assert(parent);
  if (ctx->pass == GUI_RENDER && tab->pan.state != GUI_HIDDEN) {
    gui__tab_ctl_drw(ctx, tab, &tab->pan);
  }
  gui_panel_end(ctx, &tab->pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  Grid
 * ---------------------------------------------------------------------------
 */
static void
gui__grid_drw(struct gui_ctx *ctx, struct gui_box *b, unsigned flags,
              int off_x, int off_y) {
  assert(b);
  assert(ctx);
  struct sys *sys = ctx->sys;

  int s = ctx->cfg.grid;
  sys->ren.drw.line_style(ctx->ren, 1);
  sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_TXT_DISABLED]);
  if (flags & GUI_GRID_X) {
    int x = (off_x < 0) ? (abs(off_x) % s) : (s - (off_x % s));
    for (; x < b->x.ext; x += s) {
      sys->ren.drw.vln(ctx->ren, b->x.min + x, b->y.min, b->y.max);
    }
  }
  if (flags & GUI_GRID_Y) {
    int y = (off_y < 0) ? (abs(off_y) % s) : (s - (off_y % s));
    for (; y < b->y.ext; y += s) {
      sys->ren.drw.hln(ctx->ren, b->y.min + y, b->x.min, b->x.max);
    }
  }
}
static void
gui_grid_begin(struct gui_ctx *ctx, struct gui_grid *g,
               struct gui_panel *parent, const double *off) {
  assert(g);
  assert(ctx);
  assert(off);
  assert(parent);

  g->pan.box = g->box;
  if (g->flags == GUI_GRID_DFLT) {
    g->flags = GUI_GRID_XY;
  }
  gui_panel_begin(ctx, &g->pan, parent);
  memcpy(g->off, off, sizeof(g->off));

  int off_x = floori(g->off[0]);
  int off_y = floori(g->off[1]);

  /* calculate clip area */
  struct gui_box *b = &g->pan.box;
  g->space = gui_padv(b, ctx->cfg.pan_pad);
  if (ctx->pass == GUI_RENDER && g->pan.state != GUI_HIDDEN) {
    gui_reg_drw(ctx, &g->box);
    gui__grid_drw(ctx, &g->space, g->flags, off_x, off_y);
    gui_clip_begin(&g->clip, ctx, g->space.x.min, g->space.y.min,
                   g->space.x.max, g->space.y.max);
  }
  /* apply scroll offset to area body */
  b->x = gui_min_max(g->space.x.min - off_x, INT_MAX);
  b->y = gui_min_max(g->space.y.min - off_y, INT_MAX);
}
static void
gui_grid_end(struct gui_ctx *ctx, struct gui_grid *g,
             struct gui_panel *parent, double *off) {
  assert(g);
  assert(off);
  assert(ctx);
  assert(parent);

  g->scrolled = 0;
  struct gui_panel *pan = &g->pan;
  gui_panel_end(ctx, pan, parent);
  if (ctx->pass == GUI_RENDER &&
      pan->state != GUI_HIDDEN) {
    gui_clip_end(ctx, &g->clip);
  }
  struct gui_input in = {0};
  gui_input(&in, ctx, pan, GUI_BTN_RIGHT);
  if (in.mouse.btn.right.dragged) {
    struct sys *sys = ctx->sys;
    g->off[0] -= sys->mouse.pos_delta[0];
    g->off[1] -= sys->mouse.pos_delta[1];
    g->scrolled = 1;
  }
  memcpy(off, g->off, sizeof(g->off));
}

/* ---------------------------------------------------------------------------
 *                                  Node
 * ---------------------------------------------------------------------------
 */
static void
gui_graph_node_begin(struct gui_ctx *ctx, struct gui_graph_node *n,
                     struct gui_panel *parent) {
  assert(n);
  assert(ctx);
  assert(parent);

  n->pan.box = gui_box_mov(&n->box, parent->box.x.min, parent->box.y.min);
  gui_panel_begin(ctx, &n->pan, parent);
  if (ctx->pass == GUI_RENDER && n->pan.state != GUI_HIDDEN) {
    gui_panel_drw(ctx, &n->pan.box);
  }
  n->lay = gui_pad(&n->pan.box, 2, 2);
}
static void
gui_graph_node_item(struct gui_box *ret, struct gui_ctx *ctx,
                    struct gui_graph_node *n, int item_h) {
  assert(n);
  assert(ret);
  assert(ctx);

  item_h = !item_h ? ctx->cfg.item : item_h;
  *ret = gui_cut_top(&n->lay, item_h, 0);
}
static void
gui_graph_node_hdr_begin(struct gui_ctx *ctx, struct gui_graph_node *n,
                         struct gui_graph_node_hdr *hdr) {
  assert(n);
  assert(hdr);
  assert(ctx);

  hdr->pan.box = hdr->box;
  hdr->pan.box.x = gui_min_max(hdr->box.x.min, hdr->box.x.max - 2);
  gui_panel_begin(ctx, &hdr->pan, &n->pan);
  if (ctx->pass == GUI_RENDER && hdr->pan.state != GUI_HIDDEN) {
    struct sys *sys = ctx->sys;
    sys->ren.drw.col(ctx->ren, ctx->cfg.col[GUI_COL_CONTENT]);
    sys->ren.drw.box(ctx->ren, gui_unbox(&hdr->pan.box));
  }
  hdr->content = hdr->pan.box;
  hdr->content.x = gui_shrink(&hdr->pan.box.x, 1);
}
static void
gui_graph_node_hdr_end(struct gui_ctx *ctx, struct gui_graph_node *n,
                       struct gui_graph_node_hdr *hdr) {
  assert(n);
  assert(hdr);
  assert(ctx);

  gui_panel_hot(ctx, &hdr->pan, &n->pan);
  gui_panel_end(ctx, &hdr->pan, &n->pan);

  gui_input(&hdr->in, ctx, &hdr->pan, GUI_BTN_LEFT);
  hdr->mov_begin = hdr->in.mouse.btn.left.drag_begin;
  hdr->mov_end = hdr->in.mouse.btn.left.drag_end;
  hdr->moved = hdr->in.mouse.btn.left.dragged;

  if (hdr->mov_begin) {
    ctx->drag_state[0] = n->box.x.min;
    ctx->drag_state[1] = n->box.y.min;
  }
  if (hdr->moved) {
    int dpx = hdr->in.mouse.btn.left.drag_pos[0];
    int dpy = hdr->in.mouse.btn.left.drag_pos[1];
    int dx = hdr->in.mouse.pos[0] - dpx;
    int dy = hdr->in.mouse.pos[1] - dpy;
    hdr->pos[0] = cast(int, ctx->drag_state[0]) + dx;
    hdr->pos[1] = cast(int, ctx->drag_state[1]) + dy;
  }
}
static void
gui_graph_node_end(struct gui_ctx *ctx, struct gui_graph_node *n,
                   struct gui_panel *parent) {
  assert(n);
  assert(ctx);
  assert(parent);
  gui_panel_end(ctx, &n->pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
extern void dlExport(void *export, void *import);

static const struct gui_api gui_api = {
  .version = GUI_VERSION,
  .init = gui_init,
  .free = gui_free,
  .begin = gui_begin,
  .end = gui_end,
  .scissor = gui_scissor,
  .clip_begin = gui_clip_begin,
  .clip_end = gui_clip_end,
  .enable = gui_enable,
  .disable = gui_disable,
  .color_scheme = gui_color_scheme,
  .bnd = {
    .min_max = gui_min_max,
    .min_ext = gui_min_ext,
    .max_ext = gui_max_ext,
    .mid_min = gui_mid_min,
    .mid_max = gui_mid_max,
    .mid_ext = gui_mid_ext,
    .shrink = gui_shrink,
    .div = gui_div,
  },
  .box = {
    .div_x = gui_box_div_x,
    .div_y = gui_box_div_y,
    .div = gui_box_div,
    .mid_ext = gui_box_mid_ext,
    .box = gui_box,
    .pos = gui_box_pos,
    .mov = gui_box_mov,
    .clip = gui_clip,
    .pad = gui_pad,
    .padv = gui_padv,
    .posv = gui_box_posv,
  },
  .cut = {
    .lhs = gui_cut_lhs,
    .top = gui_cut_top,
    .rhs = gui_cut_rhs,
    .bot = gui_cut_bot,
    .box = gui_cut_box,
  },
  .lay = {
    .solve = gui_solve,
    .hcut = gui_hlay_cut,
    .vcut = gui_vlay_cut,
    .hitem = gui_hlay_item,
    .vitem = gui_vlay_item,
    .item = gui_lay_item,
    .vlay = gui__vlay,
    .hlay = gui__hlay,
  },
  .pan = {
    .hot = gui_panel_hot,
    .focus = gui_focus,
    .input = gui_input,
    .state = gui_panel_state,
    .drw = gui_panel_drw,
    .drw_focus = gui_focus_drw,
    .open = gui_panel_open,
    .close = gui_panel_close,
    .begin = gui_panel_begin,
    .end = gui_panel_close,
  },
  .cfg = {
    .pushi = gui_cfg_pushi,
    .pushi_on = gui_cfg_pushi_on,
    .pushu = gui_cfg_pushu,
    .pushu_on = gui_cfg_pushu_on,
    .pop = gui_cfg_pop,
    .pop_on = gui_cfg_pop_on,
  },
  .in = {
    .consume = gui_input_consume,
  },
  .txt = {
    .width = gui_txt_width,
    .uln = gui_txt_uln,
    .lbl = gui_txt,
    .fmtv = gui_txtvf,
    .txtf = gui_txtf,
    .tm = gui_tm,
  },
  .lbl = {
    .txt = gui_lbl,
    .fmtv = gui_lblvf,
    .fmt = gui_lblf,
  },
  .ico = {
    .icon = gui_icon,
    .box = gui_icon_box,
  },
  .btn = {
    .begin = gui_btn_begin,
    .end = gui_btn_end,
    .txt = gui_btn_txt,
    .lbl = gui_btn_lbl,
    .ico = gui_btn_ico,
  },
  .chk = {
    .ico = gui_chk,
    .box = gui_chk_box,
    .boxi = gui_chk_boxi,
  },
  .tog = {
    .ico = gui_tog,
    .box = gui_tog_box,
  },
  .scrl = {
    .map = gui_scrl,
    .barh = gui_hscrl,
    .barv = gui_vscrl,
  },
  .edt = {
    .drw = gui_edit_drw,
    .fld = gui_edit_field,
    .txt = gui_edit,
    .box = gui_edit_box,
    .buf = {
      .init = gui_txt_ed_init,
      .has_sel = gui_txt_ed_has_sel,
      .reset = gui_txt_ed_reset,
      .undo = gui_txt_ed_undo,
      .redo = gui_txt_ed_redo,
      .cut = gui_txt_ed_cut,
      .paste = gui_txt_ed_paste,
      .sel_all = gui_txt_ed_sel_all,
    },
  },
  .spin = {
    .val = gui_spin,
    .flt = gui_spin_flt,
    .f = gui_spin_f,
    .num = gui_spin_int,
    .i = gui_spin_i,
  },
  .grp = {
    .begin = gui_grp_begin,
    .end = gui_grp_end,
  },
  .reg = {
    .begin = gui_reg_begin,
    .apply_lst = gui_reg_apply_lst,
    .end = gui_reg_end,
  },
  .lst = {
    .view = gui_lst_view,
    .cfg = gui_lst_cfg_init,
    .begin = gui_lst_begin,
    .begin_def = gui_lst_begin_def,
    .nxt = gui_lst_nxt,
    .end = gui_lst_end,
    .set_sel_idx = gui_lst_set_sel_idx,
    .set_cur_idx = gui_lst_set_cur_idx,
    .lay = {
      .init = gui_lst_lay_init,
      .gen = gui_lst_lay_gen,
      .apply_view = gui_lst_lay_apply_view,
      .center = gui_lst_lay_center,
      .fit_start = gui_lst_lay_fit_start,
      .fit_end = gui_lst_lay_fit_end,
      .clamps = gui_lst_lay_clamp,
    },
    .ctl = {
      .elm = gui_lst_ctl_elm,
      .proc = gui_lst_ctl_proc,
    },
    .sel = {
      .elm = gui_lst_sel_elm,
      .proc = gui_lst_sel_proc,
    },
    .elm = {
      .begin = gui_lst_elm_begin,
      .end = gui_lst_elm_end,
    },
    .reg = {
      .begin = gui_lst_reg_begin,
      .nxt = gui_lst_reg_nxt,
      .center = gui_lst_reg_center,
      .fit_start = gui_lst_reg_fit_start,
      .fit_end = gui_lst_reg_fit_end,
      .clamp = gui_lst_reg_clamp,
      .end = gui_lst_reg_end,
      .elm = {
        .begin = gui_lst_reg_elm_begin,
        .end = gui_lst_reg_elm_end,
        .txt = gui_lst_reg_elm_txt,
      },
    },
  },
  .tree = {
    .begin = gui_tree_node_begin,
    .end = gui_tree_node_end,
    .node = gui_tree_node,
  },
  .splt = {
    .begin = gui_split_begin,
    .sep = gui_split_sep,
    .end = gui_split_end,
    .lay = {
      .begin = gui_split_lay_begin,
      .add = gui_split_lay_add,
      .end = gui_split_lay_end,
      .bld = gui_split_lay,
    },
  },
  .tbl = {
    .begin = gui_tbl_begin,
    .end = gui_tbl_end,
    .lay = gui_split_lay,
    .hdr = {
      .begin = gui_tbl_hdr_begin,
      .end = gui_tbl_hdr_end,
      .slot = {
        .begin = gui_tbl_hdr_slot_begin,
        .end = gui_tbl_hdr_slot_end,
        .txt = gui_tbl_hdr_slot,
      },
    },
    .lst = {
      .cfg = gui_tbl_lst_cfg_init,
      .begin = gui_tbl_lst_begin,
      .begin_def = gui_tbl_lst_begin_def,
      .end = gui_tbl_lst_end,
      .nxt = gui_lst_nxt,
      .elm = {
        .begin = gui_tbl_lst_elm_begin,
        .end = gui_tbl_lst_elm_end,
        .col = {
          .slot = gui_tbl_lst_elm_col,
          .txt = gui_tbl_lst_txt,
          .txtf = gui_tbl_lst_txtf,
          .tm = gui_tbl_lst_tm,
        },
      },
    },
  },
  .tab = {
    .begin = gui_tab_ctl_begin,
    .sel = gui_tab_ctl_sel,
    .end = gui_tab_ctl_end,
    .hdr = {
      .begin = gui_tab_hdr_begin,
      .end = gui_tab_hdr_end,
      .slot = {
        .begin = gui_tab_hdr_slot_begin,
        .end = gui_tab_hdr_slot_end,
        .txt_id = gui_tab_hdr_slot_id,
        .txt = gui_tab_hdr_slot,
      },
    },
  },
  .grid = {
    .begin = gui_grid_begin,
    .end = gui_grid_end,
  },
  .node = {
    .begin = gui_graph_node_begin,
    .item = gui_graph_node_item,
    .end = gui_graph_node_end,
    .hdr = {
      .begin = gui_graph_node_hdr_begin,
      .end = gui_graph_node_hdr_end,
    },
  },
};
static void
gui_get_api(void *export, void *import) {
  unused(import);
  struct gui_api *api = (struct gui_api*)export;
  *api = gui_api;
}
#ifdef DEBUG_MODE
extern void
dlExport(void *export, void *import) {
  struct res_api *im = (struct res_api*)import;
  gui_get_api(export, import);
  res = *im;
}
#endif

