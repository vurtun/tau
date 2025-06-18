/* ---------------------------------------------------------------------------
 *                                Utility
 * ---------------------------------------------------------------------------
 */
#define GUI_INIT_HASH_LO FNV1A32_HASH_INITIAL
#define GUI_INIT_HASH_HI (0xC0E785u)
#define GUI_ID_INIT (struct gui_id){.lo = GUI_INIT_HASH_LO, .hi = GUI_INIT_HASH_HI}

#define gui_inbox(px, py, box) \
  (between(px, (box)->x.min, (box)->x.max) && between(py, (box)->y.min, (box)->y.max))
#define gui_contains(a, b) \
  (gui_inbox((b)->x.min, (b)->y.min, a) && gui_inbox((b)->x.max, (b)->y.max, a))
#define gui_intersect(bnd, x0,y0,x1,y1)           \
  ((bnd)->x.min < (x1) && (bnd)->x.max > (x0) &&  \
   (bnd)->y.min < (y1) && (bnd)->y.max > (y0))

static inline struct gui_id
gui__hash(void *data, int len, struct gui_id hash) {
  struct gui_id ret;
  ret.lo = fnv1a32(data, len, hash.hi);
  ret.hi = fnv1a32(&ret.lo, szof(ret.lo), hash.lo);
  return ret;
}
static inline struct gui_id
gui_gen_id(struct gui_id uid, struct gui_id pid) {
  return gui__hash(&uid, szof(uid), pid);
}
static inline int
gui_id_eq(struct gui_id lhs, struct gui_id rhs) {
  return lhs.lo == rhs.lo && lhs.hi == rhs.hi;
}
static inline int
gui_id_neq(struct gui_id lhs, struct gui_id rhs) {
  return gui_id_eq(lhs, rhs) == 0;
}
static struct gui_bnd
gui_min_max(int vmin, int vmax) {
  struct gui_bnd ret;
  ret.min = min(vmin,vmax);
  ret.max = max(vmin,vmax);
  ret.ext = max(ret.max - ret.min, 0);
  ret.mid = ret.min + ret.ext / 2;
  return ret;
}
static struct gui_bnd
gui_min_ext(int vmin, int vext) {
  struct gui_bnd ret;
  ret.min = vmin;
  ret.max = vmin + vext;
  ret.ext = vext;
  ret.mid = vmin + vext / 2;
  return ret;
}
static struct gui_bnd
gui_max_ext(int vmax, int vext) {
  struct gui_bnd ret;
  ret.min = vmax - vext;
  ret.max = vmax;
  ret.ext = vext;
  ret.mid = vmax - vext / 2;
  return ret;
}
static struct gui_bnd
gui_mid_min(int mid, int vmin) {
  struct gui_bnd ret;
  ret.ext = (mid - vmin) * 2;
  ret.max = vmin + ret.ext;
  ret.mid = mid;
  ret.min = vmin;
  return ret;
}
static struct gui_bnd
gui_mid_max(int mid, int vmax) {
  struct gui_bnd ret;
  ret.ext = (vmax - mid) * 2;
  ret.min = vmax - ret.ext;
  ret.mid = mid;
  ret.max = vmax;
  return ret;
}
static struct gui_bnd
gui_mid_ext(int mid, int vext) {
  struct gui_bnd ret;
  ret.mid = mid;
  ret.ext = vext;
  ret.max = mid + vext / 2;
  ret.min = mid - vext / 2;
  return ret;
}
static struct gui_bnd
gui_shrink(const struct gui_bnd *bnd, int pad) {
  return gui_min_max(bnd->min + pad, bnd->max - pad);
}
static struct gui_bnd
gui_div(const struct gui_bnd *bnd, int gap, int cnt, int idx) {
  /* Divide box axis into 'cnt' space and return space at 'idx'.
   * Helper for grid layout like space allocation */
  int lft = max(0, bnd->ext - (gap * cnt));
  int ext = lft / cnt;
  idx = clamp(0, idx, max(0, cnt - 1));
  int off = bnd->min + ((ext + gap) * idx);
  return gui_min_ext(off, ext);
}
static struct gui_box
gui_box_div_x(const struct gui_box *box, int gap, int cnt, int idx) {
  struct gui_box ret = {.y = box->y};
  ret.x = gui_div(&box->x, gap, cnt, idx);
  return ret;
}
static struct gui_box
gui_box_div_y(const struct gui_box *box, int gap, int cnt, int idx) {
  struct gui_box ret = {.x = box->x};
  ret.y = gui_div(&box->y, gap, cnt, idx);
  return ret;
}
static struct gui_box
gui_box_div(const struct gui_box *box, int *gap, int cntx, int cnty,
            int posx, int posy) {
  struct gui_box ret;
  ret.x = gui_div(&box->x, gap[0], cntx, posx);
  ret.y = gui_div(&box->y, gap[1], cnty, posy);
  return ret;
}
static struct gui_box
gui_box_mid_ext(const struct gui_box *box, int width, int height) {
  struct gui_box ret = {0};
  ret.x = gui_mid_ext(box->x.mid, width);
  ret.y = gui_mid_ext(box->y.mid, height);
  return ret;
}
static struct gui_box
gui_box(int posx, int posy, int width, int height) {
  struct gui_box ret;
  ret.x = gui_min_ext(posx, width);
  ret.y = gui_min_ext(posy, height);
  return ret;
}
static struct gui_box
gui_box_pos(const struct gui_box *box, int posx, int posy) {
  struct gui_box ret = {0};
  ret.x = gui_min_ext(posx, box->x.ext);
  ret.y = gui_min_ext(posy, box->y.ext);
  return ret;
}
static struct gui_box
gui_box_mov(const struct gui_box *box, int movx, int movy) {
  struct gui_box ret = {0};
  ret.x = gui_min_ext(box->x.min + movx, box->x.ext);
  ret.y = gui_min_ext(box->y.min + movy, box->y.ext);
  return ret;
}
static struct gui_box
gui_clip(int tlx, int tly, int brx, int bry) {
  struct gui_box ret;
  ret.x = gui_min_max(tlx, brx);
  ret.y = gui_min_max(tly, bry);
  return ret;
}
static struct gui_box
gui_pad(const struct gui_box *box, int padx, int pady) {
  struct gui_box ret;
  ret.x = gui_shrink(&box->x, padx);
  ret.y = gui_shrink(&box->y, pady);
  return ret;
}
static struct gui_box
gui_padv(const struct gui_box *box, int *pad) {
  return gui_pad(box, pad[0], pad[1]);
}
static struct gui_box
gui_box_posv(const struct gui_box *box, int *pos) {
  return gui_box_pos(box,pos[0],pos[1]);
}
static struct gui_box
gui_cut_lhs(struct gui_box *box, int cut, int gap) {
  int minx = box->x.min;
  int valx = min(box->x.max, box->x.min + (cut + gap));
  box->x = gui_min_max(valx, box->x.max);
  return gui_box(minx, box->y.min, cut, box->y.ext);
}
static struct gui_box
gui_cut_top(struct gui_box *box, int cut, int gap) {
  int miny = box->y.min;
  int valy = min(box->y.max, box->y.min + (cut + gap));
  box->y = gui_min_max(valy, box->y.max);
  return gui_box(box->x.min, miny, box->x.ext, cut);
}
static struct gui_box
gui_cut_rhs(struct gui_box *box, int cut, int gap) {
  int maxx = box->x.max - cut;
  int valx = max(box->x.min, box->x.max - (cut + gap));
  box->x = gui_min_max(box->x.min, valx);
  return gui_box(maxx, box->y.min, cut, box->y.ext);
}
static struct gui_box
gui_cut_bot(struct gui_box *box, int cut, int gap) {
  int maxy = box->y.max - cut;
  int valy = max(box->y.min, box->y.max - (cut + gap));
  box->y = gui_min_max(box->y.min, valy);
  return gui_box(box->x.min, maxy, box->x.ext, cut);
}
static struct gui_box
gui_cut_box(struct gui_box_cut *cut, int val) {
  switch(cut->side) {
  default: assert(0); break;
  case GUI_BOX_CUT_LHS:
    return gui_cut_lhs(cut->box, val, cut->gap);
  case GUI_BOX_CUT_RHS:
    return gui_cut_rhs(cut->box, val, cut->gap);
  case GUI_BOX_CUT_TOP:
    return gui_cut_top(cut->box, val, cut->gap);
  case GUI_BOX_CUT_BOT:
    return gui_cut_bot(cut->box, val, cut->gap);
  }
  return *cut->box;
}
static void
gui_solve(int *ret, int ext, const int *slots, int cnt, int gap,
          const int *con, struct gui_lay_sol *sol) {

  assert(ret);
  assert(slots);
  assert(ext >= 0);
  assert(cnt >= 0);

  struct gui_lay_sol dummy;
  sol = !sol ? &dummy : sol;
  mset(sol,0,szof(*sol));

  for loop(i,cnt) {
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
    for loop(i,cnt) {
      if (slots[i] < 0) {
        ret[i] = con ? con[i*2+0] : 0;
      }
    }
  }
  sol->weight = 0;
  sol->dyn_siz = max(0, total - sol->fix_siz);
  for loop(i, cnt) {
    if (slots[i] >= 0) {
      continue;
    }
    sol->weight += castu(-slots[i]);
  }
  int def_dyn_siz = 0;
  for loop(i,cnt) {
    if (slots[i] < 0) {
      unsigned slot = castu(-slots[i]);
      unsigned ratio16_16 = (slot << 16u) / sol->weight;

      ret[i] = casti((ratio16_16 * castu(sol->dyn_siz)) >> 16u);
      ret[i] = con ? clamp(con[i*2+0], ret[i], con[i*2+1]) : ret[i];
      def_dyn_siz += ret[i];
    }
  }
  if (def_dyn_siz < sol->dyn_siz) {
    int grow_cnt = 0;
    unsigned weight = 0u;
    int grow_siz = def_dyn_siz - sol->dyn_siz;
    for loop(i,cnt) {
      if (slots[i] < 0 && (!con || ret[i] < con[i*2+1])) {
        weight += castu(-slots[i]);
        grow_cnt++;
      }
    }
    while (grow_cnt > 0 && grow_siz > 0) {
      int nxt_siz = 0;
      int nxt_cnt = 0;
      unsigned nxt_weight = 0u;
      for loop(i, cnt) {
        if (slots[i] >= 0) {
          continue;
        }
        if (!con || ret[i] < con[i*2+1]) {
          unsigned slot = castu(-slots[i]);
          unsigned ratio16_16 = (slot << 16u) / weight;
          int siz = casti((ratio16_16 * castu(grow_siz)) >> 16u);
          if (con && ret[i] + siz > con[i*2+1]) {
            nxt_siz += ret[i] + siz - con[i*2+1];
            ret[i] = con[i*2+1];
          } else {
            nxt_weight += slot;
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
  assert(ctx);
  assert(lay);
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

  assert(ctx);
  assert(lay);
  assert(def);
  assert(items);

  gui__lay_init(ctx, lay, GUI_LAY_ROW, cnt, row_h, row_gap, col_gap);
  gui_solve(items, lay->box.x.ext, def, cnt, col_gap, con, sol);
}
static void
gui__vlay(struct gui_ctx *ctx, struct gui_lay *lay, int *items,
          const int *def, int cnt, int col_w, int row_gap, int col_gap,
          const int *con, struct gui_lay_sol *sol) {

  assert(ctx);
  assert(lay);
  assert(items);
  assert(def);

  gui__lay_init(ctx, lay, GUI_LAY_COL, cnt, col_w, row_gap, col_gap);
  gui_solve(items, lay->box.y.ext, def, cnt, col_gap, con, sol);
}
static struct gui_box
gui_hlay_cut(struct gui_lay *lay, int row_h) {
  assert(lay);
  lay->idx = lay->cnt;
  return gui_cut_top(&lay->box, row_h, lay->gap[1]);
}
static struct gui_box
gui_vlay_cut(struct gui_lay *lay, int col_w) {
  assert(lay);
  lay->idx = lay->cnt;
  return gui_cut_lhs(&lay->box, col_w, lay->gap[0]);
}
static struct gui_box
gui_hlay_item(struct gui_lay *lay, const int *items) {
  assert(lay);
  assert(items);
  if (lay->idx >= lay->cnt) {
    lay->sub = gui_cut_top(&lay->box, lay->item, lay->gap[1]);
    lay->idx = 0;
  }
  return gui_cut_lhs(&lay->sub, items[lay->idx++], lay->gap[0]);
}
static struct gui_box
gui_vlay_item(struct gui_lay *lay, const int *items) {
  assert(lay);
  assert(items);
  if (lay->idx >= lay->cnt) {
    lay->sub = gui_cut_lhs(&lay->box, lay->item, lay->gap[0]);
    lay->idx = 0;
  }
  return gui_cut_top(&lay->sub, items[lay->idx++], lay->gap[1]);
}
static struct gui_box
gui_lay_item(struct gui_lay *lay, const int *items) {
  assert(lay);
  assert(items);
  switch (lay->type) {
  case GUI_LAY_ROW:
    return gui_hlay_item(lay, items);
  case GUI_LAY_COL:
    return gui_vlay_item(lay, items);
  }
}

/* ---------------------------------------------------------------------------
 *                                  Draw
 * ---------------------------------------------------------------------------
 */
static int
gui_clip_begin(struct gui_clip *tmp, struct gui_ctx *ctx, int lhs, int top,
               int rhs, int bot) {
  assert(ctx);
  assert(tmp);

  *tmp = ctx->clip;
  int px0 = max(tmp->box.x.min, lhs);
  int py0 = max(tmp->box.y.min, top);
  int px1 = min(tmp->box.x.max, rhs);
  int py1 = min(tmp->box.y.max, bot);

  struct sys *sys = ctx->sys;
  ctx->clip.box = gui_clip(px0, py0, px1, py1);
  if (ctx->pass == GUI_RENDER) {
    ctx->clip.hdl = sys->gfx.d2d.clip(&sys->gfx.buf2d, px0, py0, px1, py1);
  } else {
    ctx->clip.hdl = 0;
  }
  return casti(ctx->clip.hdl);
}
static int
gui_clip_end(struct gui_ctx *ctx, struct gui_clip *clip) {
  assert(ctx);
  assert(clip);
  ctx->clip = *clip;
  return casti(ctx->clip.hdl);
}
static int
gui__drw_resv(struct gui_ctx *ctx, const struct gfx_buf2d_cost *cost) {
  assert(ctx);
  assert(cost);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);
  if (ctx->pass != GUI_RENDER) {
    return 0;
  }
  struct sys *sys = ctx->sys;
  ctx->vtx_buf_req_siz += cost->vbytes;
  ctx->idx_buf_req_siz += cost->icnt * szof(unsigned);
  if (ctx->vtx_buf_siz < sys->gfx.buf2d.vbytes + cost->vbytes) {
    ctx->oom_vtx_buf = 1;
    ctx->oom = 1;
    return 0;
  }
  int icost = cost->icnt * szof(unsigned);
  if (ctx->idx_buf_siz < sys->gfx.buf2d.icnt * szof(unsigned) + icost) {
    ctx->oom_idx_buf = 1;
    ctx->oom = 1;
    return 0;
  }
  return 1;
}
static void
gui_drw_col(struct gui_ctx *ctx, unsigned col) {
  assert(ctx);
  ctx->drw_col = col;
}
static void
gui_drw_line_style(struct gui_ctx *ctx, int line_size) {
  assert(ctx);
  ctx->line_size = line_size;
}
static void
gui_drw_box(struct gui_ctx *ctx, int px0, int py0, int px1, int py1) {
  assert(ctx);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);
  if (ctx->pass != GUI_RENDER ||
    !gui_intersect(&ctx->clip.box, px0,py0,px1,py1)) {
    return;
  }
  struct sys *sys = ctx->sys;
  if (gui__drw_resv(ctx, &sys->gfx.d2d.cost.box)) {
    sys->gfx.d2d.box(&sys->gfx.buf2d, px0, py0, px1, py1, ctx->drw_col, ctx->clip.hdl);
  }
}
static void
gui_drw_ln(struct gui_ctx *ctx, int px0, int py0, int px1, int py1) {
  assert(ctx);
  assert(ctx->sys);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);
  if (ctx->pass != GUI_RENDER) {
    return;
  }
  struct sys *sys = ctx->sys;
  if (gui__drw_resv(ctx, &sys->gfx.d2d.cost.line)) {
    sys->gfx.d2d.ln(&sys->gfx.buf2d, px0, py0, px1, py1, ctx->line_size,
      ctx->drw_col, ctx->clip.hdl);
  }
}
static void
gui_drw_hln(struct gui_ctx *ctx, int posy, int px0, int px1) {
  assert(ctx);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);
  gui_drw_ln(ctx, px0, posy, px1, posy);
}
static void
gui_drw_vln(struct gui_ctx *ctx, int posx, int py0, int py1) {
  assert(ctx);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);
  gui_drw_ln(ctx, posx, py0, posx, py1);
}
static void
gui_drw_sbox(struct gui_ctx *ctx, int px0, int py0, int px1, int py1) {
  assert(ctx);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);
  if (ctx->pass != GUI_RENDER) {
    return;
  }
  gui_drw_hln(ctx, py0, px0, px1);
  gui_drw_hln(ctx, py1, px0, px1);
  gui_drw_vln(ctx, px0, py0, py1);
  gui_drw_vln(ctx, px1, py0, py1);
}
static void
gui_drw_circle(struct gui_ctx *ctx, int posx, int posy, int radius) {
  assert(ctx);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);
  if (ctx->pass != GUI_RENDER) {
    return;
  }
  struct sys *sys = ctx->sys;
  if (gui__drw_resv(ctx, &sys->gfx.d2d.cost.circle)) {
    sys->gfx.d2d.circle(&sys->gfx.buf2d, posx, posy, radius, ctx->drw_col, ctx->clip.hdl);
  }
}
static void
gui_drw_tri(struct gui_ctx *ctx, int px0, int py0, int px1, int py1, int px2, int py2) {
  assert(ctx);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);
  if (ctx->pass != GUI_RENDER) {
    return;
  }
  if (!gui_inbox(px0, py0, &ctx->clip.box) &&
      !gui_inbox(px1, py1, &ctx->clip.box) &&
      !gui_inbox(px2, py2, &ctx->clip.box)) {
    return;
  }
  struct sys *sys = ctx->sys;
  if (gui__drw_resv(ctx, &sys->gfx.d2d.cost.tri)) {
    sys->gfx.d2d.tri(&sys->gfx.buf2d, px0, py0, px1, py1, px2, py2,
      ctx->drw_col, ctx->clip.hdl);
  }
}
static void
gui_drw_glyph(struct gui_ctx *ctx, const struct res_glyph *gly) {
  assert(ctx);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);
  if (ctx->pass != GUI_RENDER ||
    !gui_intersect(&ctx->clip.box, gly->x0, gly->y0, gly->x1, gly->y1)) {
    return;
  }
  struct sys *sys = ctx->sys;
  if (gui__drw_resv(ctx, &sys->gfx.d2d.cost.ico)) {
    sys->gfx.d2d.ico(&sys->gfx.buf2d, gly->x0, gly->y0, gly->x1, gly->y1,
        gly->sx, gly->sy, ctx->drw_col, ctx->clip.hdl);
  }
}
static void
gui_drw_rune(struct gui_ctx *ctx, int posx, int posy, int rune) {
  assert(ctx);
  struct res_glyph gly;
  res.fnt.glyph(&gly, &ctx->res->fnt, posx, posy, rune);
  gui_drw_glyph(ctx, &gly);
}
static void
gui_drw_ico(struct gui_ctx *ctx, int posx, int posy, enum res_ico_id icon) {
  assert(ctx);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);

  int rune = casti(icon);
  gui_drw_rune(ctx, posx, posy, rune) ;
}
static void
gui_drw_txt(struct gui_ctx *ctx, int dstx, int dsty, struct str txt) {
  assert(ctx);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);
  if (ctx->pass != GUI_RENDER) {
    return;
  }
  int posx = dstx;
  struct res_fnt_run_it itr;
  for res_run_loop(run, &itr, &res, ctx->res, txt) {
    int run_x = posx;
    for loopn(i, run->len, RES_FNT_MAX_RUN) {
      struct res_glyph gly = {0};
      res.run.glyph(&gly, run, i, run_x, dsty);
      gui_drw_glyph(ctx, &gly);
      run_x = posx + run->adv[i];
    }
    posx += run->adv[run->len-1] + ctx->res->fnt.space_adv * (!!str_len(itr.rest));
  }
}
static void
gui_drw_sprite(struct gui_ctx *ctx, int tex, int dstx, int dsty, int dstw, int dsth,
               int srcx, int srcy, int srcw, int srch) {
  assert(ctx);
  assert(ctx->sys);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);
  if (ctx->pass != GUI_RENDER ||
    !gui_intersect(&ctx->clip.box, dstx,dsty,dstx+dstw,dsty+dsth)) {
    return;
  }
  struct sys *sys = ctx->sys;
  if (gui__drw_resv(ctx, &sys->gfx.d2d.cost.img)) {
    sys->gfx.d2d.img(&sys->gfx.buf2d, tex, dstx, dsty, dstw, dsth,
      srcx, srcy, srcw, srch, ctx->clip.hdl);
  }
}
static void
gui_drw_img(struct gui_ctx *ctx, int tex, int dstx, int dsty, int dstw, int dsth) {
  assert(ctx);
  assert(ctx->sys);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);

  int img_siz[2];
  struct sys *sys = ctx->sys;
  sys->gfx.tex.info(img_siz, ctx->sys, tex);
  gui_drw_sprite(ctx, tex, dstx, dsty, dstw, dsth, 0, 0, img_siz[0], img_siz[1]);
}
static void
gui_drw_blit(struct gui_ctx *ctx, int tex, int posx, int posy) {
  assert(ctx);
  assert(ctx->sys);
  assert(ctx->vtx_buf);
  assert(ctx->idx_buf);

  int img_siz[2];
  struct sys *sys = ctx->sys;
  sys->gfx.tex.info(img_siz, ctx->sys, tex);
  gui_drw_sprite(ctx, tex, posx, posy, img_siz[0], img_siz[1], 0, 0,
    img_siz[0], img_siz[1]);
}

/* ---------------------------------------------------------------------------
 *                                  Panel
 * ---------------------------------------------------------------------------
 */
static void
gui_panel_hot(struct gui_ctx *ctx, struct gui_panel *pan,
              struct gui_panel *parent) {
  assert(ctx);
  assert(pan);

  const struct sys *sys = ctx->sys;
  const struct gui_box *box = &pan->box;
  pan->is_focused = !!gui_id_eq(pan->id, ctx->focused);
  if (pan->focusable && gui_id_eq(ctx->first_id, ctx->root.id)) {
    ctx->first_id = pan->id;
  }
  int in_box = gui_inbox(sys->mouse.pos[0], sys->mouse.pos[1], box);
  pan->is_hov = !parent || (parent->is_hov && in_box);
  if ((parent && !parent->is_hot) || !in_box) {
    return;
  }
  pan->is_hot = 1;
  if (pan->focusable) {
    ctx->focusable = pan->id;
  }
  ctx->hot = pan->is_hot ? pan->id : ctx->hot;
}
static void
gui_focus(struct gui_ctx *ctx, struct gui_panel *pan) {
  assert(pan);
  assert(ctx);

  ctx->focused = pan->id;
  pan->is_focused = 1;
  pan->has_focus = 1;
}
static void
gui_input(struct gui_input *pin, struct gui_ctx *ctx,
                const struct gui_panel *pan, unsigned mask) {
  assert(pin);
  assert(pan);
  assert(ctx);
  assert(ctx->sys);

  struct sys *sys = ctx->sys;
  mset(pin, 0, sizeof(*pin));

  /* enter and left */
  pin->is_hot = pan->is_hot;
  pin->entered = gui_id_eq(pan->id, ctx->hot) && gui_id_neq(pan->id, ctx->prev_hot);
  pin->exited = gui_id_neq(pan->id, ctx->hot) && gui_id_eq(pan->id, ctx->prev_hot);

  /* (un)focus */
  pin->is_focused = !!gui_id_eq(pan->id, ctx->focused);
  pin->gained_focus = !!gui_id_eq(pan->id, ctx->focused) && gui_id_neq(pan->id, ctx->prev_focused);
  pin->lost_focus = !!gui_id_neq(pan->id, ctx->focused) && gui_id_eq(pan->id, ctx->prev_focused);

  /* mouse pointer */
  struct sys_mouse *mouse = &sys->mouse;
  cpy2(pin->mouse.pos, mouse->pos);
  cpy2(pin->mouse.pos_last, mouse->pos_last);
  cpy2(pin->mouse.pos_delta, mouse->pos_delta);

  pin->mouse.pos_rel[0] = pin->mouse.pos[0] - pan->box.x.min;
  pin->mouse.pos_rel[1] = pin->mouse.pos[1] - pan->box.y.min;
  if (ctx->pass != GUI_INPUT || ctx->disabled) {
    return;
  }
  /* keyboard */
  pin->txt = sys->txt;
  pin->keys = sys->keys;
  if (gui_id_eq(pan->id, ctx->focused)) {
    ctx->focus_next = bit_tst_clr(ctx->keys, GUI_KEY_NEXT_WIDGET) ? 1 : 0;
    if (bit_tst_clr(ctx->keys, GUI_KEY_PREV_WIDGET)) {
      ctx->focused = ctx->prev_id;
      if (gui_id_eq(ctx->prev_id, ctx->root.id)) {
        ctx->focus_last = 1;
      }
    }
  }
  if (pan->focusable && !ctx->disabled) {
    ctx->prev_id = pan->id;
  }
  /* mouse button */
  for loop(i, GUI_MOUSE_BTN_CNT) {
    struct gui_mouse_btn *btn = &pin->mouse.btns[i];
    if (pan->is_hot) {
      btn->down = mouse->btns[i].down;
      btn->pressed = mouse->btns[i].pressed;
      btn->released = mouse->btns[i].released;
      btn->doubled = mouse->btns[i].doubled;
      if (!pan->dbl_clk && mouse->btns[i].doubled) {
        btn->pressed = 1; /* handle win32 double-click */
      }
    }
    if (!(mask & (1 << i))) {
      continue;
    }
    /* (un)grab */
    btn->grabbed = gui_id_eq(pan->id, ctx->btn[i].active) && gui_id_eq(pan->id, ctx->btn[i].origin);
    btn->gained_grab = gui_id_eq(pan->id, ctx->btn[i].active) && gui_id_neq(pan->id, ctx->btn[i].prev_active);
    btn->lost_grab = gui_id_eq(pan->id, ctx->btn[i].active) && gui_id_eq(pan->id, ctx->btn[i].prev_active);

    /* dragging */
    btn->drag_begin = btn->pressed;
    if (mouse->pos_delta[0] || mouse->pos_delta[1]) {
      if (gui_id_eq(pan->id, ctx->btn[i].active) &&
          gui_id_eq(ctx->btn[i].active, ctx->btn[i].origin)) {

        btn->drag_pos[0] = ctx->btn[i].drag_pos[0];
        btn->drag_pos[1] = ctx->btn[i].drag_pos[1];
        btn->dragged = 1;
      }
    }
    /* click */
    if (btn->released) {
      const struct gui_box *box = &pan->box;
      int dx = ctx->btn[i].drag_pos[0];
      int dy = ctx->btn[i].drag_pos[1];

      btn->clk = gui_inbox(dx, dy, box) && pan->is_hot;
      if (gui_id_eq(ctx->btn[i].origin, pan->id)) {
        ctx->btn[i].origin = ctx->root.id;
        btn->drag_end = 1;
      }
    }
  }
}
static enum gui_state
gui_panel_state(const struct gui_ctx *ctx, const struct gui_panel *pan) {
  assert(pan);
  assert(ctx);
  enum gui_state ret;
  const struct gui_box *box = &pan->box;
  if (!gui_intersect(&ctx->clip.box, box->x.min, box->y.min, box->x.max, box->y.max)) {
    ret = GUI_HIDDEN;
  } else if (ctx->disabled) {
    ret = GUI_DISABLED;
  } else if ((gui_id_eq(pan->id, ctx->focused)) && !ctx->focus_next) {
    ret = GUI_FOCUSED;
  } else {
    ret = GUI_NORMAL;
  }
  return ret;
}
static void
gui_panel_drw(struct gui_ctx *ctx, const struct gui_box *box) {
  assert(box);
  assert(ctx);

  gui_drw_line_style(ctx, 1);
  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_BG]);
  gui_drw_box(ctx, gui_unbox(box));

  gui_drw_hln(ctx, box->y.min, box->x.min, box->x.max - 1);
  gui_drw_vln(ctx, box->x.min, box->y.min, box->y.max - 1);

  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_LIGHT]);
  gui_drw_hln(ctx, box->y.min, box->x.min, box->x.max - 1);
  gui_drw_vln(ctx, box->x.min, box->y.min + 1, box->y.max - 1);

  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SHADOW]);
  gui_drw_hln(ctx, box->y.max - 1, box->x.min, box->x.max - 1);
  gui_drw_vln(ctx, box->x.max - 1, box->y.min, box->y.max - 1);
}
static void
gui_focus_drw(struct gui_ctx *ctx, const struct gui_box *box, int pad) {
  struct gui_box foc;
  foc.x = gui_shrink(&box->x, pad);
  foc.y = gui_shrink(&box->y, pad);

  gui_drw_line_style(ctx, 1);
  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_TXT]);
  gui_drw_sbox(ctx, gui_unbox(&foc));
}
static void
gui_panel_open(struct gui_ctx *ctx, struct gui_panel *pan,
               struct gui_panel *parent, struct gui_id uid) {
  assert(pan);
  assert(ctx);

  pan->id = parent ? gui_gen_id(uid, parent->id) : uid;
  pan->max[0] = pan->max[1] = 0;
  pan->is_hot = pan->is_hov = 0;
  pan->is_focused = 0;

  mcpy(pan->mouse_pos, ctx->sys->mouse.pos, sizeof(pan->mouse_pos));
  pan->rel_mouse_pos[0] = pan->mouse_pos[0] - pan->box.x.min;
  pan->rel_mouse_pos[1] = pan->mouse_pos[1] - pan->box.y.min;

  ctx->cur_id = pan->id;
  if (ctx->focus_next && pan->focusable && !ctx->disabled) {
    ctx->focused = pan->id;
    ctx->focus_next = 0;
  }
  gui_panel_hot(ctx, pan, parent);
  pan->state = gui_panel_state(ctx, pan);
  pan->has_focus = !!gui_id_eq(pan->id, ctx->focused);
  pan->is_focused = pan->has_focus;
}
static void
gui_panel_close(struct gui_ctx *ctx, struct gui_panel *pan,
                struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  unused(ctx);

  if (parent) {
    parent->max[0] = max(parent->max[0], pan->box.x.max);
    parent->max[1] = max(parent->max[1], pan->box.y.max);
    if (pan->is_focused) {
      parent->is_focused = 1;
    }
  }
}
#define gui_panel_end(ctx,pan,p) gui_panel_close(ctx,pan,p)

static void
gui_panel_begin(struct gui_ctx *ctx, struct gui_panel *pan,
                struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(parent);

  gui_panel_open(ctx, pan, parent, ctx->id);
  ctx->id.lo++;
}
static void
gui_panel_id(struct gui_ctx *ctx, struct gui_panel *pan,
             struct gui_panel *parent, struct gui_id uid) {
  assert(pan);
  assert(ctx);
  assert(parent);

  gui_panel_open(ctx, pan, parent, uid);
  gui_panel_close(ctx, pan, parent);
}
static void
gui_panel_cur_hov(struct gui_ctx *ctx, const struct gui_panel *pan,
                  enum sys_cur_style cur) {
  assert(pan);
  assert(ctx);
  if (pan->is_hov) {
    struct sys *sys = ctx->sys;
    sys->cursor = cur;
  }
}
static void
gui_tooltip(struct gui_ctx *ctx, const struct gui_panel *pan, struct str str) {
  assert(pan);
  assert(ctx);
  if (pan->is_hov) {
    struct sys *sys = ctx->sys;
    if (str_len(str) < szof(sys->tooltip.buf)) {
      mcpy(sys->tooltip.buf, str_beg(str), str_len(str));
      sys->tooltip.str = strn(sys->tooltip.buf, str_len(str));
    } else {
      mcpy(sys->tooltip.buf, str_beg(str), sizeof(sys->tooltip.buf));
      sys->tooltip.str = strn(sys->tooltip.buf, szof(sys->tooltip.buf));
    }
  }
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
      const struct sys *sys = ctx->sys;
      ctx->cfg.col[GUI_COL_BG] = sys->col[SYS_COL_WIN];
      ctx->cfg.col[GUI_COL_CONTENT] = sys->col[SYS_COL_BG];
      ctx->cfg.col[GUI_COL_CONTENT_HOV] = sys->col[SYS_COL_HOV];
      ctx->cfg.col[GUI_COL_SEL] = sys->col[SYS_COL_SEL];
      ctx->cfg.col[GUI_COL_LIGHT] = sys->col[SYS_COL_LIGHT];
      ctx->cfg.col[GUI_COL_SHADOW_SEAM] = sys->col[SYS_COL_SHADOW];
      ctx->cfg.col[GUI_COL_SHADOW] = sys->col[SYS_COL_SHADOW];
      ctx->cfg.col[GUI_COL_ICO] = sys->col[SYS_COL_ICO];
      ctx->cfg.col[GUI_COL_TXT] = sys->col[SYS_COL_TXT];
      ctx->cfg.col[GUI_COL_TXT_DISABLED] = sys->col[SYS_COL_TXT_DISABLED];
      ctx->cfg.col[GUI_COL_TXT_SELECTED] = sys->col[SYS_COL_TXT_SEL];
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
static void
gui_dflt_cfg(struct gui_cfg *cfg) {
  cfg->sep = GUI_CFG_SEP;
  cfg->item = GUI_CFG_ITEM;
  cfg->elm = GUI_CFG_ELM;
  cfg->tab = GUI_CFG_TAB;
  cfg->depth = GUI_CFG_DEPTH;
  cfg->btn_pad = GUI_CFG_BTN_PAD;
  cfg->grid = GUI_CFG_GRID;

  cfg->pan_pad[0] = GUI_CFG_PAN_PADX;
  cfg->pan_pad[1] = GUI_CFG_PAN_PADY;
  cfg->pan_gap[0] = GUI_CFG_PAN_GAPX;
  cfg->pan_gap[1] = GUI_CFG_PAN_GAPY;

  cfg->grp_off = GUI_CFG_GRP_OFF;
  cfg->grp_pad = GUI_CFG_GRP_PAD;

  cfg->lay_pad[0] = GUI_CFG_LAY_PADX;
  cfg->lay_pad[1] = GUI_CFG_LAY_PADY;
  cfg->lst_pad[0] = GUI_CFG_LST_PADX;
  cfg->lst_pad[1] = GUI_CFG_LST_PADY;

  cfg->gap[0] = GUI_CFG_GAPX;
  cfg->gap[1] = GUI_CFG_GAPY;
  cfg->pad[0] = GUI_CFG_PADX;
  cfg->pad[1] = GUI_CFG_PADY;

  cfg->ico = GUI_CFG_ICO;
  cfg->scrl = GUI_CFG_SCRL;
}
static inline int
gui__scale_val(int val, unsigned dpi_scale) {
  unsigned scaled = (castu(val) * dpi_scale);
  unsigned rounded = scaled + (1U << 15U);
  rounded &= ~((1U << 16U) - 1u);
  return casti(rounded >> 16U);
}
static void
gui_cfg_scale(struct gui_cfg *out, const struct gui_cfg *old, unsigned dpi_scale) {
  out->sep = gui__scale_val(old->sep, dpi_scale);
  out->item = gui__scale_val(old->item, dpi_scale);
  out->elm = gui__scale_val(old->elm, dpi_scale);
  out->tab = gui__scale_val(old->tab, dpi_scale);
  out->depth = gui__scale_val(old->depth, dpi_scale);
  out->btn_pad = gui__scale_val(old->btn_pad, dpi_scale);
  out->grid = gui__scale_val(old->grid, dpi_scale);
  out->pan_pad[0] = gui__scale_val(old->pan_pad[0], dpi_scale);
  out->pan_pad[1] = gui__scale_val(old->pan_pad[1], dpi_scale);
  out->pan_gap[0] = gui__scale_val(old->pan_gap[0], dpi_scale);
  out->pan_gap[1] = gui__scale_val(old->pan_gap[1], dpi_scale);
  out->grp_off = gui__scale_val(old->grp_off, dpi_scale);
  out->grp_pad = gui__scale_val(old->grp_pad, dpi_scale);
  out->lay_pad[0] = gui__scale_val(old->lay_pad[0], dpi_scale);
  out->lay_pad[1] = gui__scale_val(old->lay_pad[1], dpi_scale);
  out->lst_pad[0] = gui__scale_val(old->lst_pad[0], dpi_scale);
  out->lst_pad[1] = gui__scale_val(old->lst_pad[1], dpi_scale);
  out->gap[0] = gui__scale_val(old->gap[0], dpi_scale);
  out->gap[1] = gui__scale_val(old->gap[1], dpi_scale);
  out->pad[0] = gui__scale_val(old->pad[0], dpi_scale);
  out->pad[1] = gui__scale_val(old->pad[1], dpi_scale);
  out->ico = gui__scale_val(old->ico, dpi_scale);
  out->scrl = gui__scale_val(old->scrl, dpi_scale);
}
static void
gui_init(struct gui_ctx *ctx, struct gui_args *args) {
  assert(ctx);
  gui_color_scheme(ctx, args->scm);
  gui_dflt_cfg(&ctx->cfg);
  if (args->scale != 0) {
    gui_cfg_scale(&ctx->cfg, &ctx->cfg, args->scale);
    ctx->dpi_scale = args->scale;
  } else {
    ctx->dpi_scale = 1U << 16U;
  }
}
static int
gui_cfg_pushi(struct gui_cfg_stk *stk, void *ptr, int val) {
  assert(stk);
  assert(ptr);

  stk->ptr = ptr;
  mcpy(&stk->val, ptr, sizeof(val));
  mcpy(ptr, &val, sizeof(val));
  return 1;
}
static int
gui_cfg_pushi_on(struct gui_cfg_stk *stk, void *ptr, int val, int cond) {
  assert(stk);
  assert(ptr);
  if (cond) {
    gui_cfg_pushi(stk, ptr, val);
    return 1;
  }
  return 0;
}
static int
gui_cfg_pushu(struct gui_cfg_stk *stk, void *ptr, unsigned val) {
  assert(stk);
  assert(ptr);

  stk->ptr = ptr;
  mcpy(&stk->val, ptr, sizeof(val));
  mcpy(ptr, &val, sizeof(val));
  return 1;
}
static int
gui_cfg_pushu_on(struct gui_cfg_stk *stk, void *ptr, unsigned val, int cond) {
  assert(stk);
  assert(ptr);
  if (cond) {
    gui_cfg_pushu(stk, ptr, val);
    return 1;
  }
  return 0;
}
static int
gui_cfg_pop(const struct gui_cfg_stk *stk) {
  assert(stk);
  mcpy(stk->ptr, &stk->val, sizeof(stk->val));
  return 1;
}
static int
gui_cfg_pop_on(const struct gui_cfg_stk *stk, int cond) {
  assert(stk);
  if (cond) {
    gui_cfg_pop(stk);
    return 1;
  }
  return 0;
}
static void
gui_input_begin(struct gui_ctx *ctx, struct sys_mouse *mouse) {
  assert(ctx);
  assert(mouse);
  for loop(i, GUI_MOUSE_BTN_CNT) {
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
  for loop(i, GUI_MOUSE_BTN_CNT) {
    ctx->btn[i].prev_active = ctx->btn[i].active;
    if (mouse->btns[i].released) {
      ctx->btn[i].origin = ctx->root.id;
    }
  }
}
static void
gui_input_eat(struct gui_ctx *ctx) {
  assert(ctx);
  for loop(i, GUI_MOUSE_BTN_CNT) {
    mset(&ctx->btn, 0, sizeof(ctx->btn));
  }
  mset(ctx->keys, 0, sizeof(ctx->keys));
}
static void
gui_sys_dnd_begin(struct gui_ctx *ctx, struct sys *sys) {
  assert(sys);
  assert(ctx);

  ctx->dnd_act = 1;
  ctx->dnd_set = 1;
  ctx->dnd_clr = 1;
  ctx->dnd_btn = GUI_MOUSE_LEFT;

  switch (sys->dnd.state) {
  case SYS_DND_NONE: return;
  case SYS_DND_PREVIEW:
  case SYS_DND_ENTER:
  case SYS_DND_LEFT: break;
  case SYS_DND_DELIVERY:
    ctx->dnd_paq.state = GUI_DND_DELIVERY;
    sys->mouse.btn.left.released = 1;
    break;
  }
  ctx->dnd_paq.src = GUI_DND_EXTERN;
  ctx->dnd_paq.response = GUI_DND_REJECT;
  ctx->dnd_paq.src_id = ctx->root.id;

  switch (sys->dnd.type) {
  case SYS_DND_FILE: {
    ctx->dnd_paq.type = GUI_DND_SYS_FILES;
    ctx->dnd_paq.data = sys->dnd.files;
    ctx->dnd_paq.size = sys->dnd.file_cnt;
  } break;
  case SYS_DND_STR: {
    ctx->dnd_paq.type = GUI_DND_SYS_STRING;
    ctx->dnd_paq.data = &sys->dnd.str;
    ctx->dnd_paq.size = str_len(sys->dnd.str);
  } break;}
}
static void
gui_sys_dnd_end(struct gui_ctx *ctx, struct sys *sys) {
  switch (ctx->dnd_paq.response) {
  case GUI_DND_REJECT:
    sys->dnd.response = SYS_DND_REJECT; break;
  case GUI_DND_ACCEPT:
    sys->dnd.response = SYS_DND_ACCEPT; break;
  }
}
static int
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
  pan->id = GUI_ID_INIT;
  if (ctx->pass == GUI_INPUT) {
    /* skip input pass when only mouse movement happend and not dragging */
    int no_move = !sys->mouse_mod || (sys->mouse_mod && !sys->mouse_grap);
    if (sys->drw && !sys->key_mod && !sys->btn_mod && !sys->dnd_mod &&
        !sys->txt_mod && !sys->scrl_mod && no_move) {
      ctx->pass = GUI_RENDER;
    }
    if (!sys->drw && (sys->key_mod || sys->btn_mod || sys->dnd_mod || sys->txt_mod ||
         sys->scrl_mod || (sys->mouse_mod && sys->mouse_grap))) {
      sys->repaint = 1;
    }
  }
  /* skip render pass when system does not request it */
  if (ctx->pass == GUI_RENDER) {
    if (!sys->drw) {
      ctx->pass = GUI_INPUT;
      return 0;
    }
  }
  /* setup pass */
  switch (ctx->pass) {
    case GUI_FINISHED: assert(0); break;
    case GUI_INPUT: {
      ctx->prev_id = pan->id;
      ctx->cur_id = pan->id;
      ctx->prev_hot = ctx->hot;
      ctx->prev_focused = ctx->focused;
      ctx->first_id = pan->id;
      sys->cursor = SYS_CUR_ARROW;

      gui_input_begin(ctx, &sys->mouse);
      ctx->focusable = ctx->root.id;
      /* drag & drop */
      if (sys->dnd.state != SYS_DND_NONE) {
        gui_sys_dnd_begin(ctx, sys);
      }
    } break;
    case GUI_RENDER: {
      sys->tooltip.str = str_nil;
      sys->gfx.buf2d.vtx = ctx->vtx_buf;
      sys->gfx.buf2d.idx = recast(unsigned*, ctx->idx_buf);

      ctx->oom = 0;
      ctx->oom_vtx_buf = 0;
      ctx->oom_idx_buf = 0;
      ctx->vtx_buf_req_siz = 0;
      ctx->idx_buf_req_siz = 0;

      struct gui_clip clp = {0};
      gui_drw_line_style(ctx, 1);
      ctx->clip.box = gui_clip(0, 0, sys->win.w, sys->win.h);

      gui_clip_begin(&clp, ctx, 0, 0, sys->win.w, sys->win.h);
      gui_drw_col(ctx, ctx->cfg.col[GUI_COL_BG]);
      gui_drw_box(ctx, 0, 0, sys->win.w, sys->win.h);
    } break;
  }
  /* root */
  pan->box = gui_box(0, 0, sys->win.w, sys->win.h);
  gui_panel_hot(ctx, pan, 0);
  ctx->box = gui_padv(&pan->box, ctx->cfg.pan_pad);
  return 1;
}
static void
gui_dnd_clr(struct gui_ctx *ctx) {
  assert(ctx);
  mset(&ctx->dnd_paq, 0, sizeof(ctx->dnd_paq));
  ctx->dnd_act = 0;
  ctx->dnd_set = 0;
  ctx->dnd_in = 0;
}
static int
gui_end(struct gui_ctx *ctx) {
  assert(ctx);
  assert(ctx->disabled == 0);

  ctx->id.lo = 0;
  ctx->id.hi = GUI_INIT_HASH_LO;
  switch (ctx->pass) {
    case GUI_FINISHED: break;
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

      struct sys *sys = ctx->sys;
      if (sys->dnd.state != SYS_DND_NONE) {
        gui_sys_dnd_end(ctx, sys);
      }
     if (ctx->dnd_clr) {
        gui_dnd_clr(ctx);
      }
      gui_input_end(ctx, &sys->mouse);
      mset(ctx->keys, 0, sizeof(ctx->keys));
    } break;
    case GUI_RENDER: {
      ctx->pass = GUI_FINISHED;
    } break;
  }
  return 0;
}
static int
gui_disable(struct gui_ctx *ctx, int cond) {
  assert(ctx);
  if (cond) {
    ctx->disabled++;
  }
  return casti(ctx->disabled);
}
static int
gui_enable(struct gui_ctx *ctx, int cond) {
  assert(ctx);
  if (cond) {
    assert(ctx->disabled > 0);
    ctx->disabled--;
  }
  return casti(ctx->disabled);
}

/* ---------------------------------------------------------------------------
 *                                  Drag & Drop
 * ---------------------------------------------------------------------------
 */
static int
gui_dnd_src_begin(struct gui_dnd_src *ret, struct gui_ctx *ctx,
                  struct gui_panel *pan, struct gui_dnd_src_arg *arg) {
  assert(ret);
  assert(ctx);
  assert(pan);
  assert(arg);

  struct gui_input dummy = {0};
  struct gui_input *pin = arg->in;
  if (!arg->in) {
    pin = &dummy;
    gui_input(pin, ctx, pan, (1U << arg->drag_btn));
  }
  ret->activated = pin->mouse.btns[arg->drag_btn].drag_begin;
  ret->drag_begin = pin->mouse.btns[arg->drag_btn].drag_begin;
  ret->dragged = pin->mouse.btns[arg->drag_btn].dragged;
  ret->drag_end = pin->mouse.btns[arg->drag_btn].drag_end;
  if (ret->drag_end) {
    ctx->dnd_clr = 1;
  }
  ret->active = ret->drag_begin|| ret->dragged || ret->drag_end;
  if (!ret->active) {
    return 0;
  }
  ctx->dnd_paq.src_id = pan->id;
  ctx->dnd_paq.src = GUI_DND_INTERN;
  ctx->dnd_btn = arg->drag_btn;
  ctx->dnd_act = 1;
  ctx->dnd_in = 1;
  return 1;
}
static void
gui_dnd_src_set(struct gui_ctx *ctx, struct gui_id type, const void *data,
                int len, int cond) {
  assert(ctx);
  assert(ctx->dnd_in);
  assert(ctx->dnd_act);
  if (!cond) {
    return;
  }
  ctx->dnd_set = 1;
  ctx->dnd_paq.type = type;
  ctx->dnd_paq.data = data;
  ctx->dnd_paq.size = len;
}
static void
gui_dnd_src_end(struct gui_ctx *ctx) {
  assert(ctx);
  assert(ctx->dnd_in);
  assert(ctx->dnd_act);
  if (!ctx->dnd_set) {
    ctx->dnd_act = 0;
  }
}
static int
gui_dnd_dst_begin(struct gui_ctx *ctx, struct gui_panel *pan) {
  assert(ctx);
  assert(pan);
  if (!ctx->dnd_act || !pan->is_hov) {
    return 0;
  }
  assert(ctx->dnd_act);
  assert(ctx->dnd_set);

  struct gui_input pin;
  gui_input(&pin, ctx, pan, 0);
  if (pin.entered) {
    ctx->dnd_paq.state = GUI_DND_ENTER;
  } else if (pin.exited) {
    ctx->dnd_paq.state = GUI_DND_LEFT;
  } else {
    struct sys *sys = ctx->sys;
    if (sys->mouse.btns[ctx->dnd_btn].released) {
      ctx->dnd_paq.state = GUI_DND_DELIVERY;
    } else {
      ctx->dnd_paq.state = GUI_DND_PREVIEW;
    }
  }
  ctx->dnd_in = 1;
  return 1;
}
static struct gui_dnd_paq*
gui_dnd_dst_get(struct gui_ctx *ctx, struct gui_id type) {
  assert(ctx);
  assert(ctx->dnd_in);
  assert(ctx->dnd_act);
  assert(ctx->dnd_set);
  if (gui_id_neq(ctx->dnd_paq.type, type)) {
    return 0;
  }
  return &ctx->dnd_paq;
}
static void
gui_dnd_dst_end(struct gui_ctx *ctx) {
  assert(ctx);
  assert(ctx->dnd_in);
  assert(ctx->dnd_act);
  assert(ctx->dnd_set);
  ctx->dnd_in = 0;
}
/* ---------------------------------------------------------------------------
 *                                  Label
 * ---------------------------------------------------------------------------
 */
static void
gui_txt_ext(int *ext, struct gui_ctx *ctx, struct str txt) {
  assert(ext);
  assert(ctx);
  res.fnt.ext(ext, ctx->res, txt);
}
static int
gui_txt_width(struct gui_ctx *ctx, struct str txt) {
  assert(ctx);
  int ext[2] = {0};
  gui_txt_ext(ext, ctx, txt);
  return ext[0];
}
static void
gui_txt_fit(struct res_txt_bnd *bnd, int space, struct gui_ctx *ctx,
            struct str txt) {
  assert(bnd);
  assert(ctx);
  res.fnt.fit(bnd, ctx->res, space, txt);
}
static void
gui_drw_txt_uln(struct gui_ctx *ctx, struct gui_panel *pan,
                struct str txt, int uln_pos, int uln_cnt) {
  assert(ctx);
  assert(pan);

  int cnt = str_len(txt);
  uln_pos = clamp(0, uln_pos, cnt);
  uln_cnt = min(uln_cnt, max(0, cnt - uln_pos));

  struct str uln_min = utf_at(0, txt, uln_pos);
  struct str uln_max = utf_at(0, strp(str_end(uln_min), str_end(txt)), uln_cnt);

  int off[2];
  int len[2];

  gui_txt_ext(off, ctx, strp(str_beg(txt), str_beg(uln_min)));
  gui_txt_ext(len, ctx, strp(str_beg(uln_min), str_beg(uln_max)));
  gui_drw_hln(ctx, pan->box.y.max - 1, pan->box.x.min + off[0],
              pan->box.x.min + off[0] + len[0]);
}
static void
gui_align_txt(struct gui_box *box, const struct gui_align *align, int *ext) {
  assert(box);
  assert(ext);
  assert(align);

  switch (align->h) {
    case GUI_HALIGN_LEFT:
      break;
    case GUI_HALIGN_MID:
      box->x = gui_mid_ext(box->x.mid, ext[0]);
      break;
    case GUI_HALIGN_RIGHT:
      box->x = gui_max_ext(box->x.max, ext[0]);
      break;
  }
  switch (align->v) {
    case GUI_VALIGN_TOP:
      break;
    case GUI_VALIGN_MID:
      box->y = gui_mid_ext(box->y.mid, ext[1]);
      break;
    case GUI_VALIGN_BOT:
      box->y = gui_max_ext(box->y.max, ext[1]);
      break;
  }
}
static void
gui_txt_drw(struct gui_ctx *ctx, struct gui_panel *pan, struct str txt,
            int uln_pos, int uln_cnt) {

  assert(ctx);
  assert(pan);
  switch (pan->state) {
    case GUI_HIDDEN:
      return;
    case GUI_DISABLED: {
      gui_drw_col(ctx, ctx->cfg.col[GUI_COL_TXT_DISABLED]);
      gui_drw_txt(ctx, pan->box.x.min, pan->box.y.min, txt);
    } break;
    case GUI_FOCUSED:
    case GUI_NORMAL: {
      gui_drw_col(ctx, ctx->cfg.col[GUI_COL_TXT]);
      gui_drw_txt(ctx, pan->box.x.min, pan->box.y.min, txt);
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
    struct res_txt_bnd bnd;
    gui_txt_fit(&bnd, pan->box.x.ext, ctx, txt);
    ext[0] = bnd.width;
    txt = strp(str_beg(txt), bnd.end);
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

  char buf[GUI_MAX_FMT_BUF];
  int cnt = fmtvsn(buf, cntof(buf), fmt, args);
  if (cnt < cntof(buf)) {
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
  va_end(args);
}
static void
gui_lbl(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
        struct gui_box_cut *cut, struct str txt) {

  assert(ctx);
  assert(cut);
  assert(pan);
  assert(parent);

  int txtw = gui_txt_width(ctx, txt);
  pan->box = gui_cut_box(cut, txtw);
  gui_txt(ctx, pan, parent, txt, 0);
}
static void
gui_lblvf(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
          struct gui_box_cut *cut, const char *fmt, va_list args) {

  assert(ctx);
  assert(fmt);
  assert(pan);
  assert(parent);

  char buf[GUI_MAX_FMT_BUF];
  int cnt = fmtvsn(buf, cntof(buf), fmt, args);
  if (cnt < cntof(buf)) {
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

  char buf[GUI_MAX_TM_BUF];
  strftime(buf, cntof(buf), fmt, tm);
  gui_txt(ctx, pan, parent, str0(buf), 0);
}
/* ---------------------------------------------------------------------------
 *                                  Icon
 * ---------------------------------------------------------------------------
 */
static void
gui_ico(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
         enum res_ico_id ico_id) {

  assert(ctx);
  assert(pan);
  assert(parent);

  gui_panel_begin(ctx, pan, parent);
  if (ctx->pass == GUI_RENDER && pan->state != GUI_HIDDEN) {
    struct gui_box ico = {0};
    ico = gui_box_mid_ext(&pan->box, ctx->cfg.ico, ctx->cfg.ico);
    if (pan->state == GUI_DISABLED) {
      gui_drw_col(ctx, ctx->cfg.col[GUI_COL_TXT_DISABLED]);
    } else {
      gui_drw_col(ctx, ctx->cfg.col[GUI_COL_ICO]);
    }
    gui_drw_ico(ctx, ico.x.min, ico.y.min, ico_id);
  }
  gui_panel_end(ctx, pan, parent);
}
static void
gui_icon(struct gui_ctx *ctx, struct gui_icon *icn, struct gui_panel *parent,
         enum res_ico_id ico_id) {

  assert(ctx);
  assert(icn);
  assert(parent);

  icn->pan.box = icn->box;
  gui_ico(ctx, &icn->pan, parent, ico_id);
  gui_panel_cur_hov(ctx, &icn->pan, SYS_CUR_HAND);

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
             struct gui_panel *parent, enum res_ico_id ico_id,
             const struct str txt) {

  assert(ctx);
  assert(pan);
  assert(parent);

  int txtw = gui_txt_width(ctx, txt);
  int ext = txtw + ctx->cfg.item + ctx->cfg.gap[0];
  int width = min(pan->box.x.ext, ext);

  pan->box.x = gui_min_ext(pan->box.x.min, width);
  gui_panel_begin(ctx, pan, parent);
  {
    struct gui_panel ico = {.box = pan->box};
    ico.box.x = gui_min_ext(pan->box.x.min, ctx->cfg.item);
    gui_ico(ctx, &ico, pan, ico_id);

    struct gui_panel lbl = {.box = pan->box};
    static const struct gui_align def_align = {GUI_HALIGN_MID, GUI_VALIGN_MID};
    lbl.box.x = gui_min_max(ico.box.x.max + ctx->cfg.gap[0], pan->box.x.max);
    gui_txt(ctx, &lbl, pan, txt, &def_align);
  }
  gui_panel_end(ctx, pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  Button
 * ---------------------------------------------------------------------------
 */
static void
gui_btn_drw(struct gui_ctx *ctx, const struct gui_panel *btn) {
  assert(ctx);
  assert(btn);
  const struct gui_box *box = &btn->box;

  int clk = (ctx->sys->mouse.btn.left.down || ctx->sys->mouse.btn.right.down);
  if (btn->is_hot && clk) {
    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_BG]);
    gui_drw_box(ctx, gui_unbox(box));

    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SHADOW]);
    gui_drw_hln(ctx, box->y.min + 1, box->x.min + 1, box->x.max - 2);
    gui_drw_vln(ctx, box->x.min + 1, box->y.min + 1, box->y.max - 2);

    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_LIGHT]);
    gui_drw_hln(ctx, box->y.max-1, box->x.min, box->x.max);
    gui_drw_vln(ctx, box->x.max, box->y.min, box->y.max-1);

  } else {
    gui_panel_drw(ctx, &btn->box);
    if (btn->state == GUI_FOCUSED) {
      gui_focus_drw(ctx, box, 0);
    }
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
  gui_input(&btn->in, ctx, pan, GUI_BTN_LEFT);

  btn->pressed = btn->in.mouse.btn.left.pressed;
  btn->released = btn->in.mouse.btn.left.released;
  btn->clk = btn->in.mouse.btn.left.clk;
  if (pan->state == GUI_FOCUSED) {
    btn->clk = bit_tst_clr(ctx->keys, GUI_KEY_ACT) ? 1 : btn->clk;
  }
}
static int
gui_btn_txt(struct gui_ctx *ctx, struct gui_btn *btn, struct gui_panel *parent,
            struct str str, const struct gui_align *align) {

  assert(btn);
  assert(ctx);
  assert(parent);

  static const struct gui_align def_align = {GUI_HALIGN_MID, GUI_VALIGN_MID};
  gui_btn_begin(ctx, btn, parent);
  {
    struct gui_panel txt = {.box = btn->pan.box};
    txt.box.x = gui_shrink(&btn->pan.box.x, ctx->cfg.pad[0]);
    gui_txt(ctx, &txt, &btn->pan, str, align ? align : &def_align);
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
  int txtw = gui_txt_width(ctx, txt);
  int ext = txtw + (ctx->cfg.btn_pad * 2);
  btn->box = gui_cut_box(cut, ext);
  return gui_btn_txt(ctx, btn, parent, txt, align);
}
static int
gui_btn_ico(struct gui_ctx *ctx, struct gui_btn *btn, struct gui_panel *parent,
            enum res_ico_id icon) {

  assert(ctx);
  assert(btn);
  assert(parent);

  gui_btn_begin(ctx, btn, parent);
  {
    struct gui_panel ico = {0};
    ico.box = gui_box_mid_ext(&btn->box, ctx->cfg.ico, ctx->cfg.ico);
    gui_ico(ctx, &ico, &btn->pan, icon);
  }
  gui_btn_end(ctx, btn, parent);
  return btn->clk;
}
static int
gui_btn_ico_txt(struct gui_ctx *ctx, struct gui_btn *btn, struct gui_panel *parent,
                struct str txt, enum res_ico_id icon, int uline) {

  assert(ctx);
  assert(btn);
  assert(parent);
  assert(icon);

  static const struct gui_align align = {GUI_HALIGN_MID, GUI_VALIGN_MID};
  gui_btn_begin(ctx, btn, parent);
  {
    struct gui_panel lbl = {.box = btn->pan.box};
    lbl.box.x = gui_shrink(&btn->pan.box.x, ctx->cfg.pad[0]);
    gui_txt_uln(ctx, &lbl, &btn->pan, txt, &align, uline, 1);

    struct gui_panel ico = {0};
    ico.box.x = gui_max_ext(lbl.box.x.min - ctx->cfg.gap[0], ctx->cfg.ico);
    ico.box.y = gui_mid_ext(btn->box.y.mid, ctx->cfg.ico);
    gui_ico(ctx, &ico, &btn->pan, icon);
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

  int ext[2]; res.ico.ext(ext, ctx->res, RES_ICO_CHECK);
  struct gui_box cur = gui_box_mid_ext(&pan->box, ext[0], ext[1]);
  if (ctx->pass == GUI_RENDER) {
    if (chkd == GUI_CHK_SELECTED) {
      gui_drw_col(ctx, ctx->cfg.col[GUI_COL_ICO]);
      gui_drw_ico(ctx, cur.x.min, cur.y.min, RES_ICO_CHECK);
    } else if (chkd == GUI_CHK_PARTIAL) {
      gui_drw_col(ctx, ctx->cfg.col[GUI_COL_TXT]);
      gui_drw_box(ctx, gui_unbox(&cur));
    }
  }
}
static void
gui__chk_drw(struct gui_ctx *ctx, const struct gui_panel *pan) {
  assert(ctx);
  assert(pan);

  /* background */
  const struct gui_box *box = &pan->box;
  gui_drw_line_style(ctx, 1);

  int bgc = (pan->state == GUI_DISABLED) ? GUI_COL_BG : GUI_COL_CONTENT;
  gui_drw_col(ctx, ctx->cfg.col[bgc]);
  gui_drw_box(ctx, gui_unbox(box));

  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SHADOW]);
  gui_drw_hln(ctx, box->y.min + 1, box->x.min + 1, box->x.max - 1);
  gui_drw_vln(ctx, box->x.min + 1, box->y.min + 1, box->y.max - 2);

  int col = (pan->state == GUI_DISABLED) ? GUI_COL_LIGHT : GUI_COL_BG;
  gui_drw_col(ctx, ctx->cfg.col[col]);
  gui_drw_hln(ctx, box->y.max - 2, box->x.min + 1, box->x.max - 2);
  gui_drw_vln(ctx, box->x.max - 2, box->y.min + 2, box->y.max - 2);
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
    case GUI_INPUT:
      break;
    case GUI_RENDER: {
      if (pan->state != GUI_HIDDEN) {
        gui__chk_drw(ctx, pan);
        gui__chk_cur(ctx, pan, chkd);
      }
    } break;
    case GUI_FINISHED:
      break;
  }
  gui_panel_end(ctx, pan, parent);

  struct gui_input pin = {0};
  gui_input(&pin, ctx, pan, GUI_BTN_LEFT);
  if (pin.mouse.btn.left.clk) {
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

  int txtw = gui_txt_width(ctx, txt);
  int ext = txtw + ctx->cfg.item + ctx->cfg.gap[0];
  pan->box = gui_cut_box(cut, ext);

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

  struct gui_input pin = {0};
  gui_input(&pin, ctx, pan, GUI_BTN_LEFT);
  int act = pan->state == GUI_FOCUSED && bit_tst_clr(ctx->keys, GUI_KEY_ACT);
  if (act || pin.mouse.btn.left.clk) {
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

  enum gui_chk_state state = gui_chk_box_state(*chkd);
  int ret = gui_chk_box(ctx, pan, parent, cut, txt, &state);
  *chkd = gui_chk_box_bool(state);
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

  const struct gui_box *box = &pan->box;
  int ext[2]; res.ico.ext(ext, ctx->res, RES_ICO_CHECK);
  gui__chk_drw(ctx, pan);

  struct gui_box tog;
  tog.x = gui_shrink(&box->x, 2);
  tog.y = gui_shrink(&box->y, 2);
  {
    struct gui_box slot = tog;
    int min = !act ? tog.x.min : tog.x.mid;
    int max = !act ? tog.x.mid : tog.x.max;
    slot.x = gui_min_max(min, max);
    gui_panel_drw(ctx, &slot);
  }
  if (act) {
    struct gui_box cur = {0};
    cur.x = gui_mid_ext(tog.x.mid - (tog.x.ext >> 2), ext[0]);
    cur.y = gui_mid_ext(tog.y.mid, ext[1]);
    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_ICO]);
    gui_drw_ico(ctx, cur.x.min, cur.y.min, RES_ICO_CHECK);
  } else {
    struct gui_box cur = {0};
    cur.x = gui_mid_ext(tog.x.mid + (tog.x.ext >> 2), ext[0]);
    cur.y = gui_mid_ext(tog.y.mid, ext[1]);
    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_ICO]);
    gui_drw_ico(ctx, cur.x.min, cur.y.min, RES_ICO_NO);
  }
}
static int
gui_tog(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
        int *act) {

  assert(act);
  assert(ctx);
  assert(pan);
  assert(parent);

  int ext = min(pan->box.x.ext, ctx->cfg.item << 1);
  pan->box.y = gui_mid_ext(pan->box.y.mid, ctx->cfg.item);
  pan->box.x = gui_min_ext(pan->box.x.min, ext);

  gui_panel_begin(ctx, pan, parent);
  if (ctx->pass == GUI_RENDER && pan->state != GUI_HIDDEN) {
    gui__tog_drw(ctx, pan, *act);
  }
  gui_panel_end(ctx, pan, parent);

  struct gui_input pin = {0};
  gui_input(&pin, ctx, pan, GUI_BTN_LEFT);
  if (pin.mouse.btn.left.clk) {
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

  int txtw = gui_txt_width(ctx, txt);
  int ext = txtw + ctx->cfg.gap[0] + (ctx->cfg.item << 1);
  pan->box = gui_cut_box(cut, ext);

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

  struct gui_input pin = {0};
  gui_input(&pin, ctx, pan, GUI_BTN_LEFT);
  int act = pan->state == GUI_FOCUSED && bit_tst_clr(ctx->keys, GUI_KEY_ACT);
  if (act || pin.mouse.btn.left.clk) {
    *is_act = !*is_act;
  }
  return 0;
}

/* ---------------------------------------------------------------------------
 *                                  Scroll
 * ---------------------------------------------------------------------------
 */
static void
gui__scrl_cur_lay(struct gui_panel *cur, const struct gui_scrl *scl,
                  struct gui_panel *pan) {
  assert(cur);
  assert(scl);
  assert(pan);
  struct gui_box *box = &pan->box;

  assert(box->x.ext >= 0);
  assert(box->y.ext >= 0);

  assert(scl->size[0] > 0);
  assert(scl->size[1] > 0);

  assert(scl->total[0] > 0);
  assert(scl->total[1] > 0);

  assert(scl->size[0] <= scl->total[0]);
  assert(scl->size[1] <= scl->total[1]);

  assert(scl->off[0] >= 0);
  assert(scl->off[1] >= 0);

  int pad[2] = {0,0};
  int siz[2];
  {
    unsigned ratio[2];
    ratio[0] = ((castu(scl->size[0]) << 8U) / castu(scl->total[0]));
    ratio[1] = ((castu(scl->size[1]) << 8U) / castu(scl->total[1]));

    unsigned scaled[2];
    scaled[0] = (ratio[0] * castu(box->x.ext));
    scaled[1] = (ratio[1] * castu(box->y.ext));

    siz[0] = casti(scaled[0] >> 8U) + !!(scaled[0] & 0xffU);
    if (siz[0] > 0 && siz[0] < scl->min[0]) {
      siz[0] = pad[0] = scl->min[0];
    }
    siz[1] = casti(scaled[1] >> 8U) + !!(scaled[1] & 0xffU);
    if (siz[1] > 0 && siz[1] < scl->min[1]) {
      siz[1] = pad[1] = scl->min[1];
    }
  }
  int pos[2];
  {
    int space[2];
    space[0] = box->x.ext - min(box->x.ext, pad[0]);
    space[1] = box->y.ext - min(box->y.ext, pad[1]);

    unsigned ratio[2];
    ratio[0] = ((castu(scl->off[0]) << 8U) / castu(scl->total[0]));
    ratio[1] = ((castu(scl->off[1]) << 8U) / castu(scl->total[1]));

    unsigned mov[2];
    mov[0] = ratio[0] * castu(space[0]);
    mov[1] = ratio[1] * castu(space[1]);

    pos[0] = box->x.min + casti(mov[0] >> 8U);
    pos[1] = box->y.min + casti(mov[1] >> 8U);
  }
  cur->box = gui_box(pos[0], pos[1], siz[0], siz[1]);
}
static void
gui__scrl_cur_drag(int *off, const struct gui_ctx *ctx,
                   const struct gui_scrl *scl, struct gui_box *box,
                   const struct gui_input *pin) {
  assert(off);
  assert(ctx);
  assert(pin);

  assert(scl);
  assert(box);

  int start[2];
  start[0] = pin->mouse.btn.left.drag_pos[0];
  start[1] = pin->mouse.btn.left.drag_pos[1];

  int area[2]; sub2(area, pin->mouse.pos, start);
  area[0] = clamp(-box->x.ext, area[0], box->x.ext);
  area[1] = clamp(-box->y.ext, area[1], box->y.ext);

  int sign[2];
  sign[0] = area[0] < 0 ? -1 : 1;
  sign[1] = area[1] < 0 ? -1 : 1;

  unsigned ratio[2];
  ratio[0] = ((castu(abs(area[0])) << 7U) / castu(box->x.ext));
  ratio[1] = ((castu(abs(area[1])) << 7U) / castu(box->y.ext));

  int dta[2];
  dta[0] = casti((ratio[0] * castu(scl->total[0])) >> 7U) * sign[0];
  dta[1] = casti((ratio[1] * castu(scl->total[1])) >> 7U) * sign[1];

  int space[2];
  space[0] = scl->total[0] - scl->size[0];
  space[1] = scl->total[1] - scl->size[1];

  int new_off[2]; add2(new_off, ctx->drag_state, dta);
  off[0] = clamp(0, new_off[0], space[0]);
  off[1] = clamp(0, new_off[1], space[1]);
}
static void
gui__scrl_cur(struct gui_ctx *ctx, struct gui_scrl *scl, struct gui_panel *pan,
              struct gui_panel *parent, struct gui_input *pin) {

  assert(scl);
  assert(pin);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui__scrl_cur_lay(pan, scl, parent);
  gui_panel_begin(ctx, pan, parent);
  {
    /* draw */
    if (ctx->pass == GUI_RENDER &&
        pan->state != GUI_DISABLED) {
      gui_panel_drw(ctx, &pan->box);
    }
    /* input */
    gui_input(pin, ctx, pan, GUI_BTN_LEFT);
    if (pin->mouse.btn.left.drag_begin) {
      ctx->drag_state[0] = scl->off[0];
      ctx->drag_state[1] = scl->off[1];
    }
    if (pin->mouse.btn.left.dragged) {
      gui__scrl_cur_drag(scl->off, ctx, scl, &parent->box, pin);
      gui__scrl_cur_lay(pan, scl, parent);
      scl->scrolled = 1;
    }
  }
  gui_panel_end(ctx, pan, parent);
}
static void
gui__scrl_chk(struct gui_scrl *scl) {
  assert(scl);
  scl->size[0] = max(scl->size[0], 1);
  scl->size[1] = max(scl->size[1], 1);

  scl->total[0] = max(scl->total[0], 1);
  scl->total[1] = max(scl->total[1], 1);

  scl->total[0] = max(scl->total[0], scl->size[0]);
  scl->total[1] = max(scl->total[1], scl->size[1]);

  scl->off[0] = clamp(0, scl->off[0], scl->total[0] - scl->size[0]);
  scl->off[1] = clamp(0, scl->off[1], scl->total[1] - scl->size[1]);
}
static void
gui__scrl_jmp(int *off, const struct gui_scrl *scl,
              const struct gui_panel *cur, int posx, int posy) {
  assert(scl);
  assert(off);
  assert(cur);

  int tar[2];
  tar[0] = posx - scl->box.x.min - (cur->box.x.ext >> 1);
  tar[1] = posy - scl->box.y.min - (cur->box.y.ext >> 1);

  int dst[2];
  dst[0] = clamp(0, tar[0], scl->box.x.ext);
  dst[1] = clamp(0, tar[1], scl->box.y.ext);

  unsigned ratio[2];
  ratio[0] = ((castu(dst[0]) << 8U) / castu(scl->box.x.ext));
  ratio[1] = ((castu(dst[1]) << 8U) / castu(scl->box.y.ext));

  off[0] = casti((ratio[0] * castu(scl->total[0])) >> 8U);
  off[1] = casti((ratio[1] * castu(scl->total[1])) >> 8U);

  off[0] = clamp(0, off[0], scl->total[0] - scl->size[0]);
  off[1] = clamp(0, off[1], scl->total[1] - scl->size[1]);
}
static void
gui__scrl_drw(struct gui_ctx *ctx, struct gui_scrl *scl) {
  const struct gui_box *box = &scl->pan.box;
  struct color col = col_get(ctx->cfg.col[GUI_COL_CONTENT]);
  col.a = 0x7f;

  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_BG]);
  gui_drw_box(ctx, gui_unbox(box));
  gui_drw_col(ctx, col_paq(&col));
  gui_drw_box(ctx, gui_unbox(box));
}
static void
gui_scrl(struct gui_ctx *ctx, struct gui_scrl *scl, struct gui_panel *parent) {
  assert(scl);
  assert(ctx);
  assert(parent);

  struct gui_panel cur = {0};
  struct gui_input cur_in = {0};

  scl->pan.box = scl->box;
  gui_panel_begin(ctx, &scl->pan, parent);
  {
    gui__scrl_chk(scl);
    if (ctx->pass == GUI_RENDER &&
        scl->pan.state != GUI_HIDDEN) {
      gui__scrl_drw(ctx, scl);
    }
    gui__scrl_cur(ctx, scl, &cur, &scl->pan, &cur_in);
  }
  gui_panel_end(ctx, &scl->pan, parent);

  struct gui_input pin = {0};
  gui_input(&pin, ctx, &scl->pan, GUI_BTN_LEFT);
  if (pin.mouse.btn.left.pressed && !cur_in.mouse.btn.left.pressed) {
    if (!(ctx->sys->keymod & SYS_KEYMOD_SHIFT)) {
      gui__scrl_jmp(scl->off, scl, &cur, pin.mouse.pos[0], pin.mouse.pos[1]);
      scl->scrolled = 1;
    } else {
      /* handle page up/down click shortcut */
      int pgx = (pin.mouse.pos[0] < cur.box.x.min) ? -scl->size[0] : scl->size[0];
      int pgy = (pin.mouse.pos[1] < cur.box.y.min) ? -scl->size[1] : scl->size[1];
      scl->off[0] = clamp(0, scl->off[0] + pgx, scl->total[0] - scl->size[0]);
      scl->off[1] = clamp(0, scl->off[1] + pgy, scl->total[1] - scl->size[1]);
      scl->scrolled = 1;
    }
  }
}

/* ---------------------------------------------------------------------------
 *                                  Arrow
 * ---------------------------------------------------------------------------
 */
static void
gui_arrow(struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent,
          enum gui_direction orient) {

  assert(ctx);
  assert(pan);
  assert(parent);

  gui_panel_begin(ctx, pan, parent);
  if (ctx->pass == GUI_RENDER && pan->state != GUI_HIDDEN) {
    int cfg = pan->state == GUI_DISABLED ? GUI_COL_TXT_DISABLED : GUI_COL_TXT;
    gui_drw_col(ctx, ctx->cfg.col[cfg]);
    switch (orient) {
      case GUI_NORTH: {
        gui_drw_tri(ctx, pan->box.x.mid, pan->box.y.min, pan->box.x.min,
          pan->box.y.max, pan->box.x.max, pan->box.y.max);
      } break;
      case GUI_WEST: {
        gui_drw_tri(ctx, pan->box.x.max, pan->box.y.min, pan->box.x.max,
          pan->box.y.max, pan->box.x.min, pan->box.y.mid);
      } break;
      case GUI_SOUTH: {
        gui_drw_tri(ctx, pan->box.x.min, pan->box.y.min, pan->box.x.mid,
          pan->box.y.max, pan->box.x.max, pan->box.y.min);
      } break;
      case GUI_EAST: {
        gui_drw_tri(ctx, pan->box.x.min, pan->box.y.min, pan->box.x.max,
          pan->box.y.mid, pan->box.x.min, pan->box.y.max);
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
        enum gui_orient orient, int *val) {

  assert(val);
  assert(ctx);
  assert(parent);

  gui_btn_begin(ctx, btn, parent);
  gui_btn_end(ctx, btn, parent);
  if (btn->pan.is_hot || btn->in.mouse.btn.left.dragged) {
    switch (orient) {
      case GUI_HORIZONTAL:
        ctx->sys->cursor = SYS_CUR_SIZE_WE;
        break;
      case GUI_VERTICAL:
        ctx->sys->cursor = SYS_CUR_SIZE_NS;
        break;
    }
  }
  if (btn->in.mouse.btn.left.drag_begin) {
    ctx->drag_state[0] = *val;
  }
  if (btn->in.mouse.btn.left.dragged) {
    int dir = 0;
    switch (orient) {
    case GUI_HORIZONTAL:
      dir = ctx->sys->mouse.pos[0] - btn->in.mouse.btn.left.drag_pos[0];
      break;
    case GUI_VERTICAL:
      dir = ctx->sys->mouse.pos[1] - btn->in.mouse.btn.left.drag_pos[1];
      break;
    }
    *val = max(0, casti(ctx->drag_state[0]) + dir);
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
              struct gui_panel *parent, enum gui_direction dir) {

  assert(ctx);
  assert(btn);
  assert(parent);

  btn->pan.focusable = 0;
  gui_btn_begin(ctx, btn, parent);
  {
    int extx = (dir == GUI_SOUTH || dir == GUI_NORTH) ? 5 : 3;
    int exty = (dir == GUI_EAST || dir == GUI_WEST) ? 5 : 3;

    int dpi_w = gui__scale_val(extx, ctx->dpi_scale);
    int dpi_h = gui__scale_val(exty, ctx->dpi_scale);

    struct gui_panel sym = {0};
    sym.box = gui_box_mid_ext(&btn->pan.box, dpi_w, dpi_h);
    gui_arrow(ctx, &sym, &btn->pan, dir);
  }
  gui_btn_end(ctx, btn, parent);
  return btn->clk;
}
static void
gui_hscrl(struct gui_ctx *ctx, struct gui_scrl_bar *bar,
          struct gui_panel *parent) {

  assert(bar);
  assert(ctx);
  assert(parent);

  bar->pan.box = bar->box;
  gui_panel_begin(ctx, &bar->pan, parent);
  {
    bar->scrolled = 0;
    bar->step = (bar->step <= 0) ? (bar->size >> 3U) : bar->step;

    /* decrement button */
    bar->btn_dec.box.x = gui_min_ext(bar->box.x.min, bar->box.y.ext);
    bar->btn_dec.box.y = gui_max_ext(bar->box.y.max, bar->box.y.ext);
    gui__scrl_btn(ctx, &bar->btn_dec, &bar->pan, GUI_WEST);
    if (bar->btn_dec.pressed) {
      bar->off -= bar->step;
      bar->scrolled = 1;
    }
    /* increment button */
    bar->btn_inc.box.x = gui_max_ext(bar->box.x.max, bar->box.y.ext);
    bar->btn_inc.box.y = gui_max_ext(bar->box.y.max, bar->box.y.ext);
    gui__scrl_btn(ctx, &bar->btn_inc, &bar->pan, GUI_EAST);
    if (bar->btn_inc.pressed) {
      bar->off += bar->step;
      bar->scrolled = 1;
    }
    /* scroll */
    struct gui_scrl scrl = {0};
    scrl.total[1] = scrl.size[1] = 1;
    scrl.total[0] = bar->total;
    scrl.size[0] = bar->size;
    scrl.min[0] = bar->min_size;
    scrl.off[0] = bar->off;
    scrl.off[1] = 0;

    scrl.box.y = gui_max_ext(bar->box.y.max, bar->box.y.ext);
    scrl.box.x = gui_min_max(bar->btn_dec.box.x.max, bar->btn_inc.box.x.min);
    gui_scrl(ctx, &scrl, &bar->pan);

    /* mouse cursor */
    struct gui_input pin = {0};
    gui_input(&pin, ctx, &scrl.pan, GUI_BTN_LEFT);
    if (scrl.pan.is_hot || pin.mouse.btn.left.grabbed) {
      ctx->sys->cursor = SYS_CUR_SIZE_WE;
    }
    bar->scrolled = bar->scrolled || scrl.scrolled;
    bar->off = scrl.off[0];
  }
  gui_panel_end(ctx, &bar->pan, parent);
}
static void
gui_vscrl(struct gui_ctx *ctx, struct gui_scrl_bar *bar,
          struct gui_panel *parent) {

  assert(bar);
  assert(ctx);
  assert(parent);

  bar->pan.box = bar->box;
  gui_panel_begin(ctx, &bar->pan, parent);
  {
    bar->scrolled = 0;
    bar->step = (bar->step <= 0) ? (bar->size >> 3u): bar->step;

    /* decrement button */
    bar->btn_dec.box.x = gui_min_ext(bar->box.x.min, bar->box.x.ext);
    bar->btn_dec.box.y = gui_min_ext(bar->box.y.min, bar->box.x.ext);
    gui__scrl_btn(ctx, &bar->btn_dec, &bar->pan, GUI_NORTH);
    if (bar->btn_dec.pressed) {
      bar->off -= bar->step;
      bar->scrolled = 1;
    }
    /* increment button */
    bar->btn_inc.box.x = gui_min_ext(bar->box.x.min, bar->box.x.ext);
    bar->btn_inc.box.y = gui_max_ext(bar->box.y.max, bar->box.x.ext);
    gui__scrl_btn(ctx, &bar->btn_inc, &bar->pan, GUI_SOUTH);
    if (bar->btn_inc.pressed) {
      bar->off += bar->step;
      bar->scrolled = 1;
    }
    /* scroll */
    struct gui_scrl scrl = {0};
    scrl.total[0] = scrl.size[0] = 1;
    scrl.total[1] = bar->total;
    scrl.size[1] = bar->size;
    scrl.min[1] = bar->min_size;
    scrl.off[1] = bar->off;
    scrl.off[0] = 0;

    scrl.box.x = bar->btn_dec.box.x;
    scrl.box.y = gui_min_max(bar->btn_dec.box.y.max, bar->btn_inc.box.y.min);
    gui_scrl(ctx, &scrl, &bar->pan);

    /* mouse cursor */
    struct gui_input pin = {0};
    gui_input(&pin, ctx, &scrl.pan, GUI_BTN_LEFT);
    if (scrl.pan.is_hot || pin.mouse.btn.left.grabbed) {
      ctx->sys->cursor = SYS_CUR_SIZE_NS;
    }
    bar->scrolled = bar->scrolled || scrl.scrolled;
    bar->off = scrl.off[1];
  }
  gui_panel_end(ctx, &bar->pan, parent);
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
  edt->sel[0] = 0;
  edt->sel[1] = 0;
  edt->init = 1;
  edt->off = 0;
  edt->cur = 0;
}
static void
gui_txt_ed_clear(struct gui_txt_ed *edt) {
  assert(edt);
  gui_txt_ed_reset(edt);
  edt->str = str_nil;
  edt->buf[0] = 0;
}
static void
gui_txt_ed_init(struct gui_txt_ed *edt, char *buf, int cap, struct str str) {
  assert(edt);
  assert(buf);

  gui_txt_ed_reset(edt);
  edt->buf = buf;
  edt->cap = cap;
  edt->str = str;
}
static void
gui_txt_ed_assign(struct gui_txt_ed *edt, struct str str){
  assert(edt);
  edt->str = str_sqz(edt->buf, edt->cap, str);
}
static void
gui_txt_ed_redo_flush(struct gui_txt_ed_undo *undo) {
  assert(undo);
  undo->redo_char_pnt = GUI_EDT_UNDO_CHAR_CNT;
  undo->redo_pnt = GUI_EDT_UNDO_CNT;
}
static void
gui_txt_ed_undo_discard(struct gui_txt_ed_undo *undo) {
  assert(undo);
  if (undo->undo_pnt <= 0) {
    return;
  }
  /* discard oldest entry in undo list */
  if (undo->stk[0].char_at >= 0) {
    /* if the 0th undo state has characters, clean them up  */
    int cnt = undo->stk[0].in_len;
    undo->undo_char_pnt = casts(undo->undo_char_pnt - cnt);
    mcpy(undo->buf, undo->buf + cnt, undo->undo_char_pnt + szof(undo->buf[0]));
    for loop(i, undo->undo_pnt) {
      if (undo->stk[i].char_at < 0) {
        continue;
      }
      undo->stk[i].char_at = casts(undo->stk[i].char_at - cnt);
    }
  }
  undo->undo_pnt--;
  mcpy(undo->stk, undo->stk + 1, undo->undo_pnt * szof(undo->buf[0]));
}
static void
gui_txt_ed_redo_discard(struct gui_txt_ed_undo *undo) {
  assert(undo);
  int total = GUI_EDT_UNDO_CNT - 1;
  if (undo->redo_pnt > total) {
    return;
  }
  /* discard the oldest entry in the redo list--it's bad if this
  ever happens, but because undo & redo have to store the actual
  characters in different cases, the redo character buffer can
  fill up even though the undo buffer didn't */
  if (undo->stk[total].char_at >= 0) {
    /* if the k'th undo state has chars, clean up */
    int len = undo->stk[total].in_len;
    undo->redo_char_pnt = casts(undo->redo_char_pnt + len);
    int cnt = GUI_EDT_UNDO_CHAR_CNT - undo->redo_char_pnt;
    mcpy(undo->buf + undo->redo_char_pnt, undo->buf + undo->redo_char_pnt - len, cnt * szof(char));
    for (int i = undo->redo_pnt; i < total; ++i) {
      if (undo->stk[i].char_at < 0) {
        continue;
      }
      undo->stk[i].char_at = casts(undo->stk[i].char_at + len);
    }
  }
  undo->redo_pnt++;
  int cnt = GUI_EDT_UNDO_CNT - undo->redo_pnt;
  if (cnt) {
    mcpy(undo->stk + undo->redo_pnt - 1, undo->stk + undo->redo_pnt, cnt * szof(undo->stk[0]));
  }
}
static struct gui_txt_ed_undo_entry*
gui_txt_ed_undo_entry(struct gui_txt_ed_undo *undo, int char_cnt) {
  assert(undo);
  gui_txt_ed_redo_flush(undo); /* discard redo on new undo record */
  if (undo->undo_pnt >= GUI_EDT_UNDO_CNT) {
    gui_txt_ed_undo_discard(undo); /* freeup record if needed */
  }
  /* no undo if does not fit into buffer */
  if (char_cnt > GUI_EDT_UNDO_CHAR_CNT) {
    undo->undo_char_pnt = 0;
    undo->undo_pnt = 0;
    return 0;
  }
  while (undo->undo_char_pnt + char_cnt > GUI_EDT_UNDO_CHAR_CNT) {
    gui_txt_ed_undo_discard(undo);
  }
  return undo->stk + undo->undo_pnt++;
}
static char *
gui_txt_ed_undo_make(struct gui_txt_ed_undo *undo, int pos, int in_len,
                     int del_len) {
  assert(undo);
  struct gui_txt_ed_undo_entry *elm = gui_txt_ed_undo_entry(undo, in_len);
  if (elm == 0) {
    return 0;
  }
  elm->where = pos;
  elm->in_len = casts(in_len);
  elm->del_len = casts(del_len);
  if (in_len == 0) {
    elm->char_at = -1;
    return 0;
  }
  elm->char_at = undo->undo_char_pnt;
  undo->undo_char_pnt = casts(undo->undo_char_pnt + in_len);
  return undo->buf + elm->char_at;
}
static void
gui_txt_ed_undo(struct gui_txt_ed *edt) {
  assert(edt);
  struct gui_txt_ed_undo *undo = &edt->undo;
  if (undo->undo_pnt == 0) {
    return;
  }
  struct gui_txt_ed_undo_entry ude = undo->stk[undo->undo_pnt - 1];
  struct gui_txt_ed_undo_entry *rde = &undo->stk[undo->undo_pnt - 1];
  rde->in_len = ude.del_len;
  rde->del_len = ude.in_len;
  rde->where = ude.where;
  rde->char_at = -1;
  /* if the undo record says to delete characters, then the redo record will
     need to re-insert the characters that get deleted, so we need to store
     them. There are three cases:
         - there's enough room to store the characters
         - characters stored for *redoing* don't leave room for redo
         - characters stored for *undoing* don't leave room for redo
     if the last is true, we have to bail */
  if (ude.del_len) {
    if (undo->undo_char_pnt + ude.del_len < GUI_EDT_UNDO_CHAR_CNT) {
      while (undo->undo_char_pnt + ude.del_len > undo->redo_char_pnt) {
        gui_txt_ed_redo_discard(undo);
        if (undo->redo_pnt == GUI_EDT_UNDO_CHAR_CNT) {
          return;
        }
      }
      rde = undo->stk + undo->redo_pnt - 1;
      rde->char_at = casts(undo->redo_char_pnt - ude.del_len);
      undo->redo_char_pnt = casts(undo->redo_char_pnt - ude.del_len);
      for loop(i, ude.del_len) {
        undo->buf[rde->char_at + i] = edt->buf[ude.where + i];
      }
    } else {
      rde->in_len = 0;
    }
    edt->str = str_del(edt->buf, edt->str, ude.where, ude.del_len);
  }
  if (ude.in_len) {
    struct str src = strn(edt->buf + ude.char_at, ude.in_len);
    edt->str = str_put(edt->buf, edt->cap, edt->str, ude.where, src);
    undo->undo_char_pnt = cast(short, undo->undo_char_pnt - ude.in_len);
  }
  edt->cur = utf_len(strn(edt->buf, ude.where + ude.in_len));

  undo->undo_pnt--;
  undo->redo_pnt--;
}
static void
gui_txt_ed_redo(struct gui_txt_ed *edt) {
  assert(edt);
  struct gui_txt_ed_undo *undo = &edt->undo;
  if (undo->redo_pnt == GUI_EDT_UNDO_CNT) {
    return;
  }
  /* we KNOW there must be room for the undo record,
   * because the redo record was derived from an undo record */
  struct gui_txt_ed_undo_entry rde = undo->stk[undo->undo_pnt];
  struct gui_txt_ed_undo_entry *ude = undo->stk + undo->undo_pnt;
  ude->del_len = rde.in_len;
  ude->in_len = rde.del_len;
  ude->where = rde.where;
  ude->char_at = -1;

  if (rde.del_len) {
    if (undo->undo_char_pnt + ude->in_len <= undo->redo_char_pnt) {
      ude->char_at = undo->undo_char_pnt;
      undo->undo_char_pnt = cast(short, undo->undo_char_pnt + ude->in_len);
      /* now save the characters */
      for (int i = 0; i < ude->in_len; ++i) {
        undo->buf[ude->char_at + i] = edt->buf[ude->where + i];
      }
    } else {
      ude->in_len = ude->del_len = 0;
    }
    edt->str = str_del(edt->buf, edt->str, rde.where, rde.del_len);
  }
  if (rde.in_len) {
    struct str src = strn(undo->buf + rde.char_at, rde.in_len);
    edt->str = str_put(edt->buf, edt->cap, edt->str, rde.where, src);
  }
  edt->cur = utf_len(strn(edt->buf, rde.where + rde.in_len));

  undo->undo_pnt++;
  undo->redo_pnt++;
}
static void
gui_txt_ed_undo_in(struct gui_txt_ed *edt, int where, int len) {
  assert(edt);
  gui_txt_ed_undo_make(&edt->undo, where, 0, len);
}
static void
gui_txt_ed_undo_del(struct gui_txt_ed *edt, int where, int len) {
  assert(edt);
  char *ptr = gui_txt_ed_undo_make(&edt->undo, where, len, 0);
  if (ptr) {
    mcpy(ptr, edt->buf + where, len);
  }
}
static void
gui_txt_ed_undo_repl(struct gui_txt_ed *edt, int where,
                     int old_len, int new_len) {
  assert(edt);
  char *ptr = gui_txt_ed_undo_make(&edt->undo, where, old_len, new_len);
  if (ptr) {
    mcpy(ptr, edt->buf + where, old_len);
  }
}
static int
gui_txt_ed_ext(const struct gui_txt_ed *edt, int line_begin, int char_idx,
               struct res *fnt_res) {
  assert(edt);
  assert(fnt_res);

  int ext[2];
  struct str view = utf_at(0, edt->str, line_begin + char_idx);
  res.fnt.ext(ext, fnt_res, view);
  return ext[0];
}
struct gui_txt_row {
  int x[2], y[2];
  int baseline_y_dt;
  int char_cnt;
};
static void
gui_txt_ed_lay_row(struct gui_txt_row *row, const struct gui_txt_ed *edt,
                   int line_begin, int row_h, struct res *fnt_res) {
  assert(edt);
  assert(row);
  assert(fnt_res);

  struct str begin = utf_at(0, edt->str, line_begin);
  struct str txt = strp(str_beg(begin), str_end(edt->str));
  struct str end = begin;

  int cnt = 0;
  unsigned rune = 0;
  for utf_loop(&rune, it, _, txt) {
    end = it;
    if (rune == '\n') {
      break;
    }
    cnt++;
  }
  int ext[2];
  res.fnt.ext(ext, fnt_res, strp(str_beg(begin), str_end(end)));

  row->char_cnt = cnt;
  row->baseline_y_dt = row_h;
  row->x[0] = row->y[0] = 0;
  row->x[1] = ext[0];
  row->y[1] = row_h;
}
static int
gui_txt_ed_loc_coord(const struct gui_txt_ed *edt, int posx, int posy, int row_h,
                     struct res *fnt_res) {
  assert(fnt_res);
  assert(edt);

  int idx = 0;
  int base_y = 0;
  struct gui_txt_row row = {0};
  while (idx < str_len(edt->str)) {
    gui_txt_ed_lay_row(&row, edt, idx, row_h, fnt_res);
    if (row.char_cnt <= 0) {
      return str_len(edt->str);
    }
    if (idx == 0 && posy < base_y + row.y[0]) {
      return 0;
    }
    if (posy < base_y + row.y[1]) {
      break;
    }
    base_y += row.baseline_y_dt;
    idx += row.char_cnt;
  }
  if (idx >= str_len(edt->str)) {
    return str_len(edt->str);
  }
  if (posx < row.x[0]) {
    return idx;
  }
  if (posx < row.x[1]) {
    int off = idx;
    int prev_x = row.x[0];
    for (idx = 0; idx < row.char_cnt; ++idx) {
      int width = gui_txt_ed_ext(edt, off, idx, fnt_res);
      if (posx < prev_x + width) {
        if (posx < prev_x + width / 2) {
          return off + idx;
        }
        return off + idx + 1;
      }
      prev_x += width;
    }
  }
  unsigned rune = 0;
  utf_at(&rune, edt->str, idx + row.char_cnt - 1);
  if (rune == '\n') {
    return idx + row.char_cnt - 1;
  }
  return idx + row.char_cnt;
}
static void
gui_txt_ed_clk(struct gui_txt_ed *edt, int posx, int posy, int row_h,
               struct res *fnt_res) {
  assert(edt);
  edt->cur = gui_txt_ed_loc_coord(edt, posx, posy, row_h, fnt_res);
  edt->sel[0] = edt->sel[1] = edt->cur;
}
static void
gui_txt_ed_drag(struct gui_txt_ed *edt, int posx, int posy, int row_h,
                struct res *fnt_res) {
  assert(edt);
  int ptr = gui_txt_ed_loc_coord(edt, posx, posy, row_h, fnt_res);
  if (edt->sel[0] == edt->sel[1]) {
    edt->sel[0] = edt->cur;
  }
  edt->cur = ptr;
  edt->sel[1] = ptr;
}
static void
gui_txt_ed_clamp(struct gui_txt_ed *edt) {
  assert(edt);
  int cnt = utf_len(edt->str);
  if (gui_txt_ed_has_sel(edt)) {
    edt->sel[0] = (edt->sel[0] > cnt) ? cnt : edt->sel[0];
    edt->sel[1] = (edt->sel[1] > cnt) ? cnt : edt->sel[1];
    edt->cur = (edt->sel[0] == edt->sel[1]) ? edt->sel[0] : edt->cur;
  }
  if (edt->cur > cnt) {
    edt->cur = cnt;
  }
}
static void
gui__txt_edt_del(struct gui_txt_ed *edt, int where, int len) {
  assert(edt);
  gui_txt_ed_undo_del(edt, where, len);
  edt->str = str_del(edt->buf, edt->str, where, len);
  gui_txt_ed_clamp(edt);
}
static void
gui_txt_ed_del(struct gui_txt_ed *edt, int where, int len) {
  assert(edt);
  struct str begin = utf_at(0, edt->str, where);
  struct str end = utf_at(0, strp(str_beg(begin), str_end(edt->str)), len);
  if (!str_len(begin)) {
    return;
  }
  int width = casti(str_beg(begin) - edt->buf);
  int cnt = casti(str_beg(end) - str_beg(begin));
  gui__txt_edt_del(edt, width, cnt);
}
static void
gui_txt_ed_del_sel(struct gui_txt_ed *edt) {
  assert(edt);
  gui_txt_ed_clamp(edt);
  if (!gui_txt_ed_has_sel(edt)) {
    return;
  }
  if (edt->sel[0] < edt->sel[1]) {
    gui_txt_ed_del(edt, edt->sel[0], edt->sel[1] - edt->sel[0]);
    edt->sel[1] = edt->cur = edt->sel[0];
  } else {
    gui_txt_ed_del(edt, edt->sel[1], edt->sel[0] - edt->sel[1]);
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
gui_txt_ed_move_sel_last(struct gui_txt_ed *edt) {
  assert(edt);
  if (!gui_txt_ed_has_sel(edt)) {
    return;
  }
  gui_txt_ed_sort_sel(edt);
  gui_txt_ed_clamp(edt);

  edt->cur = edt->sel[1];
  edt->sel[0] = edt->sel[1];
}
static inline int
gui_rune_is_word_boundary(long rune) {
  if (is_space(rune) || is_punct(rune)) {
    return 1;
  }
  return 0;
}
static int
gui_txt_ed_move_to_prev_word(struct gui_txt_ed *edt) {
  unsigned rune = 0;
  assert(edt);
  if (!edt->cur) {
    return 0;
  }
  /* skip all trailing word boundary runes */
  int cnt = edt->cur - 1;
  struct str off = utf_at(0, edt->str, cnt);
  for utf_loop_rev(&rune, _, rest, strp(edt->buf, str_beg(off))) {
    if (!gui_rune_is_word_boundary(rune)) {
      off = rest;
      break;
    }
    cnt--;
  }
  /* find first word boundary rune */
  for utf_loop_rev(&rune, _, rest, off) {
    if (gui_rune_is_word_boundary(rune)) {
      break;
    }
    cnt--;
  }
  return cnt;
}
static int
gui_txt_ed_move_to_next_word(struct gui_txt_ed *edt) {
  assert(edt);
  unsigned rune = 0;
  int cnt = edt->cur + 1;
  struct str off = utf_at(0, edt->str, cnt);
  for utf_loop(&rune, _, rest, strp(edt->buf, str_beg(off))) {
    if (!gui_rune_is_word_boundary(rune)) {
      off = rest;
      break;
    }
    cnt++;
  }
  for utf_loop(&rune, _, rest, off) {
    if (gui_rune_is_word_boundary(rune)) {
      break;
    }
    cnt++;
  }
  return cnt;
}
static void
gui_txt_ed_prep_sel_at_cur(struct gui_txt_ed *edt) {
  assert(edt);
  /* update selection and cursor to match each other */
  if (!gui_txt_ed_has_sel(edt)) {
    edt->sel[0] = edt->cur;
    edt->sel[1] = edt->cur;
  } else {
    edt->cur = edt->sel[1];
  }
}
static int
gui_txt_ed_cut(struct gui_txt_ed *edt) {
  assert(edt);
  if (gui_txt_ed_has_sel(edt)) {
    gui_txt_ed_del_sel(edt); /* implicitly clamps */
    return 1;
  }
  return 0;
}
static int
gui_txt_ed_paste(struct gui_txt_ed *edt, struct str txt) {
  assert(edt);
  /* if there's a selection, the paste should delete it */
  gui_txt_ed_clamp(edt);
  gui_txt_ed_del_sel(edt);

  const int cur = utf_at_idx(edt->str, edt->cur);
  edt->str = str_put(edt->buf, edt->cap, edt->str, cur, txt);
  if (str_len(edt->str)) {
    gui_txt_ed_undo_in(edt, cur, str_len(txt));
    edt->cur += utf_len(txt);
    return 1;
  }
  gui_txt_ed_undo_in(edt, cur, str_len(txt));
  return 1;
}
static void
gui_txt_ed_sel_all(struct gui_txt_ed *edt) {
  assert(edt);
  edt->sel[0] = 0;
  edt->sel[1] = utf_len(edt->str);
  edt->cur = edt->sel[1];
}
static void
gui_txt_ed_txt(struct gui_txt_ed *edt, struct str txt) {
  assert(edt);
  unsigned rune = 0;
  for utf_loop(&rune, itr, _, txt) {
    if (rune == 127 || rune == '\n') {
      continue;
    }
    struct str cur = utf_at(0, edt->str, edt->cur);
    int idx = casti(str_beg(cur) - edt->buf);
    if (!str_len(edt->str)) {
      edt->str = str_sqz(edt->buf, edt->cap, itr);
      edt->cur += 1;
    } else if (!gui_txt_ed_has_sel(edt) && edt->cur < utf_len(edt->str)) {
      if (edt->mode == GUI_EDT_MODE_REPLACE) {
        gui_txt_ed_undo_repl(edt, idx, str_len(cur), str_len(itr));
        edt->str = str_del(edt->buf, edt->str, idx, str_len(cur));
      }
      edt->str = str_put(edt->buf, edt->cap, edt->str, idx, itr);
      if (str_len(edt->str)) {
        edt->cur += 1;
      }
    } else {
      gui_txt_ed_del_sel(edt); /* implicitly clamps */
      cur = utf_at(0, edt->str, edt->cur);
      idx = casti(str_beg(cur) - edt->buf);
      edt->str = str_put(edt->buf, edt->cap, edt->str, idx, itr);
      if (str_len(edt->str)) {
        gui_txt_ed_undo_in(edt, idx, str_len(itr));
        edt->cur += 1;
      }
    }
  }
}
static void
gui_txt_ed_clip(struct gui_txt_ed *edt, struct sys *sys) {
  assert(edt);
  assert(sys);
  if (!gui_txt_ed_has_sel(edt)) {
    return;
  }
  /* selected text */
  int idx0 = min(edt->sel[0], edt->sel[1]);
  int idx1 = max(edt->sel[1], edt->sel[0]);
  int sel0 = utf_at_idx(edt->str, idx0);
  int sel1 = utf_at_idx(edt->str, idx1);
  sys->clipboard.set(sys, strn(edt->buf + sel0, sel1 - sel0));
}
static int
gui_txt_ed_on_key(int *ret, struct gui_txt_ed *edt, struct gui_ctx *ctx) {
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
    gui_txt_ed_sel_all(edt);
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
    gui_txt_ed_clamp(edt);
    gui_txt_ed_prep_sel_at_cur(edt);
    if (edt->sel[1] > 0) {
      edt->sel[1]--;
    }
    edt->cur = edt->sel[1];
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_CUR_RIGHT)) {
    gui_txt_ed_prep_sel_at_cur(edt);
    edt->sel[1]++;
    gui_txt_ed_clamp(edt);
    edt->cur = edt->sel[1];
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_WORD_LEFT)) {
    if (!gui_txt_ed_has_sel(edt)) {
      gui_txt_ed_prep_sel_at_cur(edt);
    }
    edt->cur = gui_txt_ed_move_to_prev_word(edt);
    edt->sel[1] = edt->cur;
    gui_txt_ed_clamp(edt);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_WORD_RIGHT)) {
    if (!gui_txt_ed_has_sel(edt)) {
      gui_txt_ed_prep_sel_at_cur(edt);
    }
    edt->cur = gui_txt_ed_move_to_next_word(edt);
    edt->sel[1] = edt->cur;
    gui_txt_ed_clamp(edt);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_START)) {
    gui_txt_ed_prep_sel_at_cur(edt);
    edt->cur = edt->sel[1] = 0;
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_SEL_END)) {
    gui_txt_ed_prep_sel_at_cur(edt);
    edt->cur = edt->sel[1] = utf_len(edt->str);
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
    if (gui_txt_ed_has_sel(edt)) {
      gui_txt_ed_move_sel_first(edt);
    } else if (edt->cur > 0) {
      edt->cur--;
    }
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_CUR_RIGHT)) {
    if (gui_txt_ed_has_sel(edt)) {
      gui_txt_ed_move_sel_last(edt);
    } else {
      edt->cur++;
    }
    gui_txt_ed_clamp(edt);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_WORD_LEFT)) {
    if (!gui_txt_ed_has_sel(edt)) {
      edt->cur = gui_txt_ed_move_to_prev_word(edt);
      gui_txt_ed_clamp(edt);
    } else {
      gui_txt_ed_move_sel_first(edt);
    }
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_WORD_RIGHT)) {
    if (!gui_txt_ed_has_sel(edt)) {
      edt->cur = gui_txt_ed_move_to_next_word(edt);
      gui_txt_ed_clamp(edt);
    } else {
      gui_txt_ed_move_sel_last(edt);
    }
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_START)) {
    edt->cur = edt->sel[0] = edt->sel[1] = 0;
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_END)) {
    edt->cur = utf_len(edt->str);
    edt->sel[0] = edt->sel[1] = 0;
    *ret = 1;
  }
  /* modification */
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_DELETE)) {
    if (!gui_txt_ed_has_sel(edt)) {
      int cnt = utf_len(edt->str);
      if (edt->cur < cnt) {
        gui_txt_ed_del(edt, edt->cur, 1);
      } else if (cnt) {
        gui_txt_ed_del(edt, cnt-1, 1);
      }
    } else {
      gui_txt_ed_del_sel(edt);
    }
    mod = 1;
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_REMOVE)) {
    if (!gui_txt_ed_has_sel(edt)) {
      if (edt->cur > 0) {
        if (edt->cur < utf_len(edt->str)) {
          edt->cur = max(0, edt->cur - 1);
          gui_txt_ed_del(edt, edt->cur, 1);
        } else {
          gui_txt_ed_del(edt, edt->cur - 1, 1);
        }
      } else if (utf_len(edt->str)) {
        gui_txt_ed_del(edt, 0, 1);
      }
    } else {
      gui_txt_ed_del_sel(edt);
    }
    mod = 1;
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_UNDO)) {
    gui_txt_ed_undo(edt);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_REDO)) {
    gui_txt_ed_redo(edt);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_COPY)) {
    gui_txt_ed_clip(edt, ctx->sys);
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_CUT)) {
    if (gui_txt_ed_has_sel(edt)) {
      gui_txt_ed_clip(edt, ctx->sys);
      gui_txt_ed_cut(edt);
      mod = 1;
    }
    *ret = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_EDIT_PASTE)) {
    struct str ptr = ctx->sys->clipboard.get(ctx->sys);
    gui_txt_ed_paste(edt, ptr);
    mod = 1;
  }
  return mod;
}

/* ---------------------------------------------------------------------------
 *                                  Edit
 * ---------------------------------------------------------------------------
 */
static int
gui_calc_edit_off(int *cur_ext, struct res *ret, const struct gui_txt_ed *edt,
                  const int *txt_ext, int width) {
  assert(edt);
  assert(ret);
  assert(cur_ext);
  assert(txt_ext);

  struct str cur = utf_at(0, edt->str, edt->cur);
  struct str str = strp(str_beg(edt->str), str_beg(cur));

  res.fnt.ext(cur_ext, ret, str);
  int off = cur_ext[0] - (width >> 1);
  off = clamp(0, off, max(0, txt_ext[0] - width));
  return off;
}
static void
gui_edit_field_drw_txt_sel(struct gui_ctx *ctx, struct gui_panel *pan,
                           struct gui_txt_ed *edt, int cur_off,
                           int *txt_ext) {
  assert(ctx);
  assert(pan);
  assert(edt);
  assert(txt_ext);

  /* calculate selection begin/end pixel offset */
  int sel0 = utf_at_idx(edt->str, min(edt->sel[0], edt->sel[1]));
  int sel1 = utf_at_idx(edt->str, max(edt->sel[1], edt->sel[0]));

  int sel_end_off[2] = {0};
  int sel_begin_off[2] = {0};
  gui_txt_ext(sel_begin_off, ctx, strn(edt->buf, sel0));
  gui_txt_ext(sel_end_off, ctx, strn(edt->buf, sel1));

  static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
  if (sel0 > 0) {
    /* text before selection */
    struct gui_panel lbl = {.box = pan->box, .state = pan->state};
    lbl.box.x = gui_min_ext(pan->box.x.min - cur_off, sel_begin_off[0]);
    gui_align_txt(&lbl.box, &align, txt_ext);
    gui_txt_drw(ctx, &lbl, strn(edt->buf, sel0), -1, 0);
  }
  if (gui_txt_ed_has_sel(edt)) {
    /* draw text in selection */
    struct gui_box box = {0};
    int sel_w = sel_end_off[0] - sel_begin_off[0];
    box.x = gui_min_ext(pan->box.x.min - cur_off + sel_begin_off[0], sel_w);
    box.y = gui_min_ext(pan->box.y.min, pan->box.y.ext);

    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SEL]);
    gui_drw_box(ctx, gui_unbox(&box));

    struct gui_panel lbl = {.box = box, .state = pan->state};
    gui_align_txt(&lbl.box, &align, txt_ext);
    gui_txt_drw(ctx, &lbl, strp(edt->buf + sel0, edt->buf + sel1), -1, 0);
  }
  const int buf_len = utf_len(edt->str);
  if (sel1 < buf_len) {
    /* text after selection */
    int sel_w = txt_ext[0] - sel_end_off[0];
    int off = sel_end_off[0] - cur_off;

    struct gui_panel lbl = {.box = pan->box, .state = pan->state};
    lbl.box.x = gui_min_ext(pan->box.x.min + off, sel_w);

    gui_align_txt(&lbl.box, &align, txt_ext);
    gui_txt_drw(ctx, &lbl, strp(edt->buf + sel1, str_end(edt->str)), -1, 0);
  }
}
static void
gui_edit_field_drw(struct gui_ctx *ctx, struct gui_edit_box *box,
                   struct gui_panel *pan, struct gui_txt_ed *edt,
                   int cur_off, int *cur_ext, int *txt_ext) {
  assert(ctx);
  assert(box);
  assert(pan);
  assert(edt);
  assert(cur_ext);
  assert(txt_ext);

  /* draw text  */
  struct gui_clip clip = {0};
  gui_clip_begin(&clip, ctx, gui_unbox(&pan->box));
  {
    box->active = !!gui_id_eq(pan->id, ctx->focused);
    if (box->active && gui_txt_ed_has_sel(edt)) {
      gui_edit_field_drw_txt_sel(ctx, pan, edt, cur_off, txt_ext);
    } else {
      static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
      struct gui_panel lbl = {.box = pan->box, .state = pan->state};
      gui_align_txt(&lbl.box, &align, txt_ext);
      gui_txt_drw(ctx, &lbl, edt->str, -1, 0);
    }
  }
  gui_clip_end(ctx, &clip);

  /* draw cursor */
  if (box->active && !gui_txt_ed_has_sel(edt)) {
    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_TXT]);
    gui_drw_vln(ctx, pan->box.x.min + cur_ext[0], pan->box.y.min + 1, pan->box.y.max);
  }
  /* draw focus */
  if (pan->state == GUI_FOCUSED) {
    gui_focus_drw(ctx, &pan->box, 1);
  }
}
static void
gui_edit_field_input(struct gui_ctx *ctx, struct gui_edit_box *box,
                     struct gui_panel *pan, struct gui_txt_ed *edt,
                     struct gui_input *pin, int cur_off) {
  assert(pin);
  assert(ctx);
  assert(box);
  assert(pan);
  assert(edt);
  struct sys *sys = ctx->sys;

  /* focus change handling */
  if (pin->gained_focus) {
    box->focused = 1;
    edt->mode = GUI_EDT_MODE_INSERT;
    if (box->flags & GUI_EDIT_SEL_ON_ACT) {
      gui_txt_ed_sel_all(edt);
    } else if (box->flags & GUI_EDIT_GOTO_END_ON_ACT) {
      int cnt = utf_len(edt->str);
      edt->cur = edt->sel[0] = edt->sel[1] = cnt;
    }
    if (box->flags & GUI_EDIT_CLR_ON_ACT) {
      edt->cur = edt->sel[0] = edt->sel[1] = 0;
      edt->str = str_nil;
    }
  }
  if (pin->lost_focus) {
    edt->mode = GUI_EDT_MODE_INSERT;
    box->unfocused = 1;
  }
  /* input handling */
  box->active = !!gui_id_eq(pan->id, ctx->focused);
  if (box->active) {
    int key = 0;
    box->mod = !!gui_txt_ed_on_key(&key, edt, ctx);
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
      gui_txt_ed_txt(edt, strn(sys->txt, sys->txt_len));
      box->mod = 1;
    }
  }
  int mouse_pos = sys->mouse.pos[0] - pan->box.x.min + cur_off;
  if (pin->mouse.btn.left.pressed) {
    gui_txt_ed_clk(edt, mouse_pos, 0, pan->box.y.ext, ctx->res);
  } else if (pin->mouse.btn.left.dragged) {
    gui_txt_ed_drag(edt, mouse_pos, 0, pan->box.y.ext, ctx->res);
  }
}
static struct str
gui_edit_field(struct gui_ctx *ctx, struct gui_edit_box *box,
               struct gui_panel *pan, struct gui_panel *parent,
               struct gui_txt_ed *edt) {
  assert(edt);
  assert(ctx);
  assert(box);
  assert(pan);
  assert(parent);

  pan->focusable = 1;
  gui_panel_begin(ctx, pan, parent);
  {
    /* calculate cursor offset  */
    if (gui_id_eq(pan->id, ctx->focused)) {
      gui_txt_ed_clamp(edt);
    }
    int cur_ext[2];
    int txt_ext[2];
    res.fnt.ext(txt_ext, ctx->res, edt->str);
    int cur = gui_calc_edit_off(cur_ext, ctx->res, edt, txt_ext, pan->box.x.ext);
    if (pan->is_hot) {
      struct sys *sys = ctx->sys;
      sys->cursor = SYS_CUR_IBEAM;
    }
    switch (ctx->pass) {
    case GUI_INPUT: {
      struct gui_input pin = {0};
      gui_input(&pin, ctx, pan, GUI_BTN_LEFT);
      gui_edit_field_input(ctx, box, pan, edt, &pin, cur);
    } break;
    case GUI_RENDER: {
      if (pan->state != GUI_HIDDEN) {
        gui_edit_field_drw(ctx, box, pan, edt, cur, cur_ext, txt_ext);
      }
    } break;
    case GUI_FINISHED:
      break;
    }
  }
  gui_panel_end(ctx, pan, parent);

  if (gui_dnd_dst_begin(ctx, pan)) {
    struct gui_dnd_paq *paq = gui_dnd_dst_get(ctx, GUI_DND_SYS_STRING);
    if (paq) { /* string drag & drop */
      const struct str *str = paq->data;
      switch (paq->state) {
      case GUI_DND_LEFT:
      case GUI_DND_ENTER:
      case GUI_DND_PREVIEW: break;
      case GUI_DND_DELIVERY: {
        edt->str = str_sqz(edt->buf, edt->cap, *str);
      } break;}
      paq->response = GUI_DND_ACCEPT;
    }
    gui_dnd_dst_end(ctx);
  }
  return edt->str;
}
static void
gui_edit_drw(struct gui_ctx *ctx, const struct gui_panel *pan) {
  assert(ctx);
  assert(pan);

  const struct gui_box *box = &pan->box;
  gui_drw_line_style(ctx, 1);
  int col = pan->state == GUI_DISABLED ? GUI_COL_BG : GUI_COL_CONTENT;
  gui_drw_col(ctx, ctx->cfg.col[col]);
  gui_drw_box(ctx, gui_unbox(box));

  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_BG]);
  gui_drw_hln(ctx, box->y.max - 1, box->x.min + 1, box->x.max);
  gui_drw_vln(ctx, box->x.max - 1, box->y.min + 1, box->y.max - 1);

  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SHADOW]);
  gui_drw_hln(ctx, box->y.min, box->x.min, box->x.max - 1);
  gui_drw_vln(ctx, box->x.min, box->y.min, box->y.max - 1);

  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_LIGHT]);
  gui_drw_hln(ctx, box->y.max, box->x.min, box->x.max);
  gui_drw_vln(ctx, box->x.max, box->y.min + 1, box->y.max);
}
static struct str
gui_edit(struct gui_ctx *ctx, struct gui_edit_box *edt,
         struct gui_panel *parent, struct gui_txt_ed *ted) {

  assert(ted);
  assert(ctx);
  assert(edt);
  assert(parent);

  edt->pan.box = edt->box;
  if (ctx->pass == GUI_RENDER) {
    gui_edit_drw(ctx, &edt->pan);
  }
  struct gui_box content = {0};
  static const int pad[2] = {3, 3};
  content.x = gui_shrink(&edt->box.x, pad[0]);
  content.y = gui_shrink(&edt->box.y, pad[1]);

  edt->pan.focusable = 1;
  edt->pan.box = content;
  gui_edit_field(ctx, edt, &edt->pan, parent, ted);
  return ted->str;
}
static struct str
gui_edit_box(struct gui_ctx *ctx, struct gui_edit_box *box,
             struct gui_panel *parent, char *buf, int cap, struct str str) {

  assert(buf);
  assert(ctx);
  assert(box);
  assert(parent);

  struct gui_txt_ed *edt = &ctx->txt_ed_tmp_state;
  struct gui_id uid = gui_gen_id(ctx->id, parent->id);
  if (gui_id_eq(ctx->focused, uid)) {
    edt = &ctx->txt_ed_state;
    if (gui_id_neq(ctx->prev_focused, uid)) {
      gui_txt_ed_init(edt, buf, cap, str);
    }
  } else {
    edt->str = str;
  }
  return gui_edit(ctx, box, parent, edt);
}

/* ---------------------------------------------------------------------------
 *                                  Spinner
 * ---------------------------------------------------------------------------
 */
static int
gui_spin_cur_ext(const struct gui_spin_val *val, int width) {
  int ret = 0;
  switch(val->typ) {
  case GUI_SPIN_INT: {
    int delta = val->num.i - val->min.i;
    ret = delta ? (width / (val->max.i / delta)) : 0;
  } break;
  case GUI_SPIN_UINT: {
    unsigned delta = val->num.u - val->min.u;
    ret = casti(delta ? (castu(width) / (val->max.u / delta)) : 0);
  } break;
#ifdef GUI_USE_FLT
  case GUI_SPIN_FLT: {
    float delta = val->num.f - val->min.f;
    ret = math_floori((delta / val->max.f) * castf(width));
  } break;
#endif
  default:
    assert(0);
    break;
  }
  return ret;
}
static void
gui_spin_cur(struct gui_box *cur, const struct gui_spin_val *val,
             const struct gui_spin *spin) {
  assert(cur);
  assert(val);
  assert(spin);

  int width = gui_spin_cur_ext(val, cur->x.ext);
  cur->x = gui_shrink(&spin->pan.box.x, 2);
  cur->x = gui_min_ext(cur->x.min, width);
  cur->y = gui_shrink(&spin->pan.box.y, 2);
}
static int
gui_spin_commit(struct gui_ctx *ctx, struct gui_spin_val *val) {
  assert(ctx);
  assert(val);

  int mod = 0;
  if (!str_len(ctx->txt_state.str)) {
    ctx->txt_state.buf[0] = '0';
    ctx->txt_state.str = strn(ctx->txt_state.buf, 1);
  }
  int last = min(str_len(ctx->txt_state.str), cntof(ctx->txt_state.buf)-1);
  ctx->txt_state.buf[last] = 0;
  switch (val->typ) {
    case GUI_SPIN_INT: {
      char *eptr = 0;
      long num = strtol(ctx->txt_state.buf, &eptr, 10);
      if (*eptr == '\0' && eptr != ctx->txt_state.buf) {
        val->num.i = clamp(val->min.i, casti(num), val->max.i);
        mod = 1;
      }
    } break;
    case GUI_SPIN_UINT: {
      char *eptr = 0;
      long num = strtol(ctx->txt_state.buf, &eptr, 10);
      if (*eptr == '\0' && eptr != ctx->txt_state.buf) {
        val->num.u = clamp(val->min.u, castu(num), val->max.u);
        mod = 1;
      }
    } break;
#ifdef GUI_USE_FLT
    case GUI_SPIN_FLT: {
      char *eptr = 0;
      double num = strtod(ctx->txt_state.buf, &eptr);
      if (*eptr == '\0' && eptr != ctx->txt_state.buf) {
        val->num.f = clamp(val->min.f, castf(num), val->max.f);
        mod = 1;
      }
    } break;
#endif
    default:
      assert(0);
      break;
  }
  return mod;
}
static void
gui_spin_focus(struct gui_ctx *ctx, struct gui_spin_val *val,
               struct gui_txt_ed *ted) {
  assert(ted);
  assert(ctx);
  assert(val);

  ctx->txt_state.buf[0] = 0;
  ctx->txt_state.str = str_nil;
  switch (val->typ) {
  case GUI_SPIN_INT:
    ctx->txt_state.str = str_fmtsn(ctx->txt_state.buf, cntof(ctx->txt_state.buf), "%d", val->num.i);
    break;
  case GUI_SPIN_UINT:
    ctx->txt_state.str = str_fmtsn(ctx->txt_state.buf, cntof(ctx->txt_state.buf), "%u", val->num.u);
    break;
#ifdef GUI_USE_FLT
  case GUI_SPIN_FLT:
    ctx->txt_state.str = str_fmtsn(ctx->txt_state.buf, cntof(ctx->txt_state.buf), "%.2f", val->num.f);
    break;
#endif
  default:
    assert(0);
    break;
  }
  gui_txt_ed_init(ted, ctx->txt_state.buf, cntof(ctx->txt_state.buf), ctx->txt_state.str);
  gui_txt_ed_sel_all(ted);
}
static int
gui_spin_key(struct gui_ctx *ctx, struct gui_spin_val *val) {
  assert(ctx);
  assert(val);

  int mod = 0;
  if (bit_tst_clr(ctx->keys, GUI_KEY_SPIN_INC)) {
    switch (val->typ) {
    case GUI_SPIN_INT:
      val->num.i = min(val->num.i + val->inc.i, val->max.i);
      break;
    case GUI_SPIN_UINT:
      val->num.u = min(val->num.u + val->inc.u, val->max.u);
      break;
#ifdef GUI_USE_FLT
    case GUI_SPIN_FLT:
      val->num.f = min(val->num.f + val->inc.f, val->max.f);
      break;
#endif
    default:
      assert(0);
      break;
    }
    mod = 1;
  }
  if (bit_tst_clr(ctx->keys, GUI_KEY_SPIN_DEC)) {
    switch (val->typ) {
    case GUI_SPIN_INT:
      val->num.i = max(val->num.i - val->inc.i, val->min.i);
      break;
    case GUI_SPIN_UINT:
      val->num.u = max(val->num.u - val->inc.u, val->min.u);
      break;
#ifdef GUI_USE_FLT
    case GUI_SPIN_FLT:
      val->num.f = max(val->num.f - val->inc.f, val->min.f);
      break;
#endif
    default:
      assert(0);
      break;
    }
    mod = 1;
  }
  return mod;
}
static void
gui_spin_scrl(struct sys *sys, struct gui_spin_val *val) {
  assert(sys);
  assert(val);

  switch (val->typ) {
  case GUI_SPIN_INT: {
    int idx = val->num.i + (val->inc.i * sys->mouse.scrl[1]);
    val->num.i = clamp(val->min.i, idx, val->max.i);
  } break;
  case GUI_SPIN_UINT: {
    int idx = casti(val->num.u) + casti(val->inc.u * castu(sys->mouse.scrl[1]));
    val->num.u = clamp(val->min.u, castu(idx), val->max.u);
  } break;
#ifdef GUI_USE_FLT
  case GUI_SPIN_FLT: {
    float delta = castf(sys->mouse.scrl[1]);
    float new_val = val->num.f + (val->inc.f * delta);
    val->num.f = clamp(val->min.f, new_val, val->max.f);
  } break;
#endif
  default:
    assert(0);
    break;
  }
  sys->mouse.scrl[1] = 0;
}
static void
gui_spin_rel_drag(struct sys *sys, struct gui_spin_val *val) {
  assert(sys);
  assert(val);

  switch (val->typ) {
  case GUI_SPIN_INT: {
    int new_val = sys->mouse.pos_delta[0] * val->inc.i;
    val->num.i = clamp(val->min.i, val->num.i + new_val, val->max.i);
  } break;
  case GUI_SPIN_UINT: {
    unsigned new_val = castu(sys->mouse.pos_delta[0]) * val->inc.u;
    val->num.u = clamp(val->min.u, val->num.u + new_val, val->max.u);
  } break;
#ifdef GUI_USE_FLT
  case GUI_SPIN_FLT: {
    float new_val = castf(sys->mouse.pos_delta[0]) * val->inc.f;
    val->num.f = clamp(val->min.f, val->num.f + new_val, val->max.f);
  } break;
#endif
  default:
    assert(0);
    break;
  }
}
static void
gui_spin_abs_drag(struct sys *sys, struct gui_spin_val *val,
                  const struct gui_spin *spin) {
  assert(sys);
  assert(val);
  assert(spin);

  struct gui_box cur = {0};
  gui_spin_cur(&cur, val, spin);

  int ext = spin->pan.box.x.ext - 4;
  int tar = (sys->mouse.pos[0] - cur.x.min);
  int dst = clamp(0, tar, ext);

  switch (val->typ) {
  case GUI_SPIN_INT: {
    unsigned ratio = (castu(dst) << 7U) / castu(ext);
    unsigned total = castu(val->max.i - val->min.i);

    unsigned scaled = ratio * total;
    unsigned rounded = scaled + (1U << 6U);
    rounded &= ~((1U << 7U) - 1U);
    int off = casti(rounded >> 7U);

    int new_val = off + val->min.i;
    val->num.i = clamp(val->min.i, new_val, val->max.i);
  } break;
  case GUI_SPIN_UINT: {
    unsigned ratio = (castu(dst) << 7U) / castu(ext);
    unsigned total = castu(val->max.i - val->min.i);

    unsigned scaled = ratio * total;
    unsigned rounded = scaled + (1U << 6U);
    rounded &= ~((1U << 7U) - 1U);
    unsigned off = rounded >> 7U;

    unsigned new_val = off + val->min.u;
    val->num.u = clamp(val->min.u, new_val, val->max.u);
  } break;
#ifdef GUI_USE_FLT
  case GUI_SPIN_FLT: {
    float ratio = castf(dst) / castf(spin->pan.box.x.ext - 4);
    float new_val = (ratio * (val->max.f - val->min.f)) + val->min.f;
    val->num.f = clamp(val->min.f, new_val, val->max.f);
  } break;
#endif
  default:
    assert(0);
    break;
  }
}
static struct str
gui_spin_str(char *buf, int cap, const struct gui_spin_val *val) {
  assert(buf);
  assert(val);

  switch (val->typ) {
  case GUI_SPIN_INT:
    return str_fmtsn(buf, cap, "%d", val->num.i);
  case GUI_SPIN_UINT:
    return str_fmtsn(buf, cap, "%u", val->num.u);
#ifdef GUI_USE_FLT
  case GUI_SPIN_FLT:
    return str_fmtsn(buf, cap, "%.2f", val->num.f);
#endif
  default:
    assert(0);
    return strv("0");
  }
}
static void
gui_spin_drw(struct gui_ctx *ctx, struct gui_spin_val *val,
             const struct gui_spin *spin) {
  assert(ctx);
  assert(val);
  assert(spin);

  gui_edit_drw(ctx, &spin->pan);
  if (spin->flags & GUI_SPIN_SLIDER) {
    struct gui_box cur = {0};
    gui_spin_cur(&cur, val, spin);
    cur.y.max += 1;

    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SEL]);
    gui_drw_box(ctx, gui_unbox(&cur));
    gui_drw_line_style(ctx, 1);
    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_TXT]);
    gui_drw_vln(ctx, cur.x.max, cur.y.min, cur.y.max - 1);
  }
}
static int
gui_spin(struct gui_ctx *ctx, struct gui_spin *spin, struct gui_panel *parent,
         struct gui_spin_val *val) {

  assert(ctx);
  assert(val);
  assert(spin);
  assert(parent);

  int modified = 0;
  spin->pan.box = spin->box;
  gui_panel_begin(ctx, &spin->pan, parent);
  {
    static const int edt_flags = GUI_EDIT_SEL_ON_ACT | GUI_EDIT_ENABLE_COMMIT;
    struct gui_edit_box edt = {.flags = edt_flags};
    if (ctx->pass == GUI_RENDER &&
        spin->pan.state != GUI_HIDDEN) {
      gui_spin_drw(ctx, val, spin);
    }
    int pad = ctx->cfg.pad[0];
    struct gui_panel edit_pan = {.focusable = 1};
    edit_pan.box.x = gui_min_max(spin->box.x.min + pad, spin->box.x.max - pad);
    edit_pan.box.y = gui_min_max(spin->box.y.min + 2, spin->box.y.max - 2);

    struct gui_id uid = gui_gen_id(ctx->id, spin->pan.id);
    if (gui_id_eq(ctx->focused, uid)) {
      /* focused edit field */
      struct gui_txt_ed *ted = &ctx->txt_ed_state;
      if (gui_id_neq(ctx->prev_focused, uid)) {
        gui_spin_focus(ctx, val, ted);
      }
      gui_edit_field(ctx, &edt, &edit_pan, &spin->pan, ted);
      if (edt.commited) {
        modified = gui_spin_commit(ctx, val) || modified;
      }
    } else {
      /* convert number to string */
      char buf[GUI_MAX_NUM_BUF];
      struct str str = gui_spin_str(buf, cntof(buf), val);

      /* edit field */
      struct gui_txt_ed *ted = &ctx->txt_ed_tmp_state;
      gui_txt_ed_init(ted, buf, cntof(buf), str);
      gui_edit_field(ctx, &edt, &edit_pan, &spin->pan, ted);
      if (edt.focused) {
        gui_spin_focus(ctx, val, &ctx->txt_ed_state);
      } else if (edt.unfocused) {
        modified = gui_spin_commit(ctx, val) || modified;
      }
    }
    /* input handling */
    struct gui_input pin;
    gui_input(&pin, ctx, &edit_pan, GUI_BTN_RIGHT);
    if (pin.mouse.btn.left.grabbed) {
      ctx->sys->cursor = SYS_CUR_SIZE_WE;
    }
    modified = modified || edt.mod;
    if (edit_pan.state == GUI_FOCUSED) {
      /* focus input */
      modified = gui_spin_key(ctx, val) || modified;
      if (ctx->sys->mouse.scrl[1]) {
        gui_spin_scrl(ctx->sys, val);
        modified = 1;
      }
    }
    if (pin.mouse.btn.right.dragged ||
        pin.mouse.btn.right.pressed) {
      if (spin->flags & GUI_SPIN_SLIDER) {
        gui_spin_abs_drag(ctx->sys, val, spin);
      } else {
        gui_spin_rel_drag(ctx->sys, val);
      }
      modified = 1;
    }
    spin->drag_begin = spin->drag_begin || pin.mouse.btn.right.drag_begin;
    spin->drag_end = spin->drag_end || pin.mouse.btn.right.drag_end;
    spin->dragged = spin->dragged || pin.mouse.btn.right.dragged;
  }
  gui_panel_end(ctx, &spin->pan, parent);
  spin->mod = spin->mod || modified;
  return spin->mod;
}
#ifdef GUI_USE_FLT
static int
gui_spin_flt(struct gui_ctx *ctx, struct gui_spin *ctl,
             struct gui_panel *parent, float *num, float min, float max,
             float inc) {

  assert(num);
  assert(ctx);
  assert(parent);

  struct gui_spin_val val;
  val.typ = GUI_SPIN_FLT;
  val.inc.f = inc == 0.0F ? 1.0F : inc;
  val.num.f = *num;

  if ((min == max) && min == 0.0F) {
    val.min.f = -(1 << FLT_MANT_DIG);
    val.max.f = (1 << FLT_MANT_DIG);
    val.inc.f = 1.0F;
  } else {
    val.min.f = min;
    val.max.f = max;
  }
  if (gui_spin(ctx, ctl, parent, &val)) {
    *num = val.num.f;
    return 1;
  }
  return 0;
}
static int
gui_spin_f(struct gui_ctx *ctx, struct gui_spin *ctl, struct gui_panel *parent,
           float *num) {
  assert(num);
  assert(ctx);
  assert(parent);
  return gui_spin_flt(ctx, ctl, parent, num, 0.0F, 0.0F, 0.0F);
}
#endif

static int
gui_spin_int(struct gui_ctx *ctx, struct gui_spin *ctl,
             struct gui_panel *parent, int *num, int min, int max, int inc) {

  assert(num);
  assert(ctx);
  assert(parent);

  struct gui_spin_val val = {0};
  val.typ = GUI_SPIN_INT;
  val.inc.i = inc == 0 ? 1 : inc;
  val.num.i = *num;
  val.min.i = min;
  val.max.i = max;

  if ((min == max) && min == 0) {
    val.min.i = INT_MIN;
    val.max.i = INT_MAX;
    val.inc.i = 1;
  } else {
    val.min.i = min;
    val.max.i = max;
  }
  if (gui_spin(ctx, ctl, parent, &val)) {
    *num = val.num.i;
    return 1;
  }
  return 0;
}
static int
gui_spin_i(struct gui_ctx *ctx, struct gui_spin *ctl, struct gui_panel *parent,
           int *num) {
  assert(num);
  assert(ctx);
  assert(parent);
  return gui_spin_int(ctx, ctl, parent, num, 0, 0, 0);
}
static int
gui_spin_uint(struct gui_ctx *ctx, struct gui_spin *ctl,
             struct gui_panel *parent, unsigned *num, unsigned min,
             unsigned max, unsigned inc) {

  assert(num);
  assert(ctx);
  assert(parent);

  struct gui_spin_val val = {0};
  val.typ = GUI_SPIN_INT;
  val.inc.u = inc == 0 ? 1 : inc;
  val.num.u = *num;
  val.min.u = min;
  val.max.u = max;

  if ((min == max) && min == 0) {
    val.min.u = 0;
    val.max.u = UINT_MAX;
    val.inc.u = 1;
  } else {
    val.min.u = min;
    val.max.u = max;
  }
  if (gui_spin(ctx, ctl, parent, &val)) {
    *num = val.num.u;
    return 1;
  }
  return 0;
}
static int
gui_spin_u(struct gui_ctx *ctx, struct gui_spin *ctl, struct gui_panel *parent,
           unsigned *num) {
  assert(num);
  assert(ctx);
  assert(parent);
  return gui_spin_uint(ctx, ctl, parent, num, 0, 0, 0);
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

  const struct gui_box *box = &pan->box;
  int top = box->y.min + (grp->ext[1] >> 1);

  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_TXT]);
  gui_drw_txt(ctx, box->x.min + ctx->cfg.grp_off, box->y.min, txt);

  gui_drw_line_style(ctx, 1);
  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SHADOW]);
  gui_drw_hln(ctx, top, box->x.min, box->x.min + ctx->cfg.grp_pad);

  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_LIGHT]);
  gui_drw_hln(ctx, top + 1, box->x.min + 1, box->x.min + ctx->cfg.grp_pad);
}
static void
gui_grp_drw(struct gui_ctx *ctx, const struct gui_grp *grp,
            const struct gui_panel *pan) {
  assert(ctx);
  assert(grp);
  assert(pan);

  const struct gui_box *box = &pan->box;
  int top = box->y.min + (grp->ext[1] >> 1);
  int ext_off = (2 * ctx->cfg.grp_off) + grp->ext[0];

  gui_drw_line_style(ctx, 1);
  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SHADOW]);
  gui_drw_hln(ctx, top, box->x.min + ext_off, box->x.max - 1);
  gui_drw_hln(ctx, box->y.max - 1, box->x.min, box->x.max - 1);
  gui_drw_vln(ctx, box->x.min, box->y.min, box->y.max - 1);
  gui_drw_vln(ctx, box->x.max - 1, top, box->y.max - 1);

  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_LIGHT]);
  gui_drw_hln(ctx, top + 1, box->x.min + ext_off, box->x.max - 2);
  gui_drw_hln(ctx, box->y.max, box->x.min, box->x.max);
  gui_drw_vln(ctx, box->x.min + 1, top + 1, box->y.max - 2);
  gui_drw_vln(ctx, box->x.max, top, box->y.max);
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
  if (ctx->pass == GUI_RENDER &&
      grp->pan.state != GUI_HIDDEN) {
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
  if (ctx->pass == GUI_RENDER &&
      grp->pan.state != GUI_HIDDEN) {
    gui_grp_drw(ctx, grp, &grp->pan);
  }
  gui_panel_end(ctx, &grp->pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  Region
 * ---------------------------------------------------------------------------
 */
static void
gui_reg_drw(struct gui_ctx *ctx, const struct gui_box *box) {
  assert(ctx);
  assert(box);

  /* background */
  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_CONTENT]);
  gui_drw_box(ctx, gui_unbox(box));

  /* border */
  gui_drw_line_style(ctx, 1);
  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SHADOW]);
  gui_drw_hln(ctx, box->y.min, box->x.min, box->x.max-1);
  gui_drw_vln(ctx, box->x.min, box->y.min, box->y.max);
}
static void
gui_reg_begin(struct gui_ctx *ctx, struct gui_reg *reg,
               struct gui_panel *parent, const int *off) {
  assert(reg);
  assert(ctx);
  assert(off);
  assert(parent);

  reg->pan.box = reg->box;
  gui_panel_begin(ctx, &reg->pan, parent);
  mcpy(reg->off, off, sizeof(reg->off));

  /* calculate clip area */
  struct gui_box *box = &reg->pan.box;
  reg->space.x = gui_min_max(box->x.min, box->x.max - ctx->cfg.scrl);
  reg->space.y = gui_min_max(box->y.min, box->y.max - ctx->cfg.scrl);
  if (ctx->pass == GUI_RENDER && reg->pan.state != GUI_HIDDEN) {
    gui_reg_drw(ctx, &reg->box);
    gui_clip_begin(&reg->clip_rect, ctx, reg->space.x.min, reg->space.y.min,
                   reg->space.x.max, reg->space.y.max);
  }
  /* apply scroll offset to area body */
  int off_x = math_floori(reg->off[0]);
  int off_y = math_floori(reg->off[1]);

  box->x = gui_min_max(reg->space.x.min - off_x, reg->space.x.max - off_x);
  box->y = gui_min_max(reg->space.y.min - off_y, reg->space.y.max - off_y);
}
static void
gui_reg_apply_lst(struct gui_reg *reg, const struct gui_lst *lst, int row_mult) {
  assert(reg);
  assert(lst);

  reg->vscrl.step = lst->lay.slot[1];
  reg->scrl_wheel = lst->lay.slot[1] * row_mult;

  reg->pan.max[0] = max(reg->pan.max[0], lst->view.max[0]);
  reg->pan.max[1] = max(reg->pan.max[1], lst->view.max[1]);
}
static void
gui_reg_end(struct gui_ctx *ctx, struct gui_reg *reg, struct gui_panel *parent,
            int *off) {

  assert(reg);
  assert(off);
  assert(ctx);
  reg->scrolled = 0;

  struct sys *sys = ctx->sys;
  struct gui_panel *pan = &reg->pan;
  gui_panel_end(ctx, pan, parent);
  if (ctx->pass == GUI_RENDER &&
      pan->state != GUI_HIDDEN) {
    gui_clip_end(ctx, &reg->clip_rect);
  }
  int off_x = reg->off[0];
  int off_y = reg->off[1];

  /* mouse wheel scrolling */
  if (sys->mouse.scrl[1]) {
    if (ctx->pass == GUI_INPUT && pan->is_hov) {
      if (sys->mouse.scrl[1]) {
        reg->scrl_wheel = !reg->scrl_wheel ? 1 : reg->scrl_wheel;
        reg->off[1] -= sys->mouse.scrl[1] * reg->scrl_wheel;
        sys->mouse.scrl[1] = 0;
        reg->scrolled = 1;
      }
    }
  }
  int top = pan->box.y.min + off_y;
  int right = pan->box.x.max + ctx->cfg.scrl + off_x;
  int bot = pan->box.y.max + ctx->cfg.scrl + off_y;
  int left = pan->box.x.min + off_x;

  reg->vscrl.box.x = gui_min_max(pan->box.x.max + off_x, right);
  reg->vscrl.box.y = gui_min_max(top, pan->box.y.max + off_y);
  reg->hscrl.box.x = gui_min_max(left, pan->box.x.max + off_x);
  reg->hscrl.box.y = gui_min_max(pan->box.y.max + off_y, bot);

  if (!reg->no_vscrl) {
    /* setup vertical scrollbar */
    reg->vscrl.min_size = ctx->cfg.scrl;
    reg->vscrl.total = (pan->max[1] - pan->box.y.min);
    reg->vscrl.size = (pan->box.y.max - pan->box.y.min - ctx->cfg.scrl);
    /* handle keyboard list begin/end action */
    if (pan->is_hot && bit_tst_clr(ctx->keys, GUI_KEY_SCRL_BEGIN)) {
      reg->off[1] = 0;
    } else if (pan->is_hot && bit_tst_clr(ctx->keys, GUI_KEY_SCRL_END)) {
      reg->off[1] = reg->vscrl.total - reg->vscrl.size;
    }
    /* handle list page up/down action */
    if (pan->is_hot && bit_tst_clr(ctx->keys, GUI_KEY_SCRL_PGUP)) {
      reg->off[1] = reg->off[1] - reg->vscrl.size;
      reg->off[1] = clamp(0, reg->off[1], reg->vscrl.total - reg->vscrl.size);
    }
    if (pan->is_hot && bit_tst_clr(ctx->keys, GUI_KEY_SCRL_PGDN)) {
      reg->off[1] = reg->off[1] + reg->vscrl.size;
      reg->off[1] = clamp(0, reg->off[1], reg->vscrl.total - reg->vscrl.size);
    }
    /* vertical scrollbar */
    reg->vscrl.off = reg->off[1];
    if (reg->force_vscrl || reg->vscrl.total > reg->vscrl.size + ctx->cfg.scrl) {
      gui_vscrl(ctx, &reg->vscrl, pan);
      if (abs(reg->off[1] - reg->vscrl.off) >= 1) {
        reg->off[1] = reg->vscrl.off;
        reg->scrolled = 1;
      }
    } else {
      reg->off[1] = 0;
    }
    reg->max_off[1] = reg->vscrl.total - reg->vscrl.size;
  }
  if (!reg->no_hscrl){
    /* setup horizontal scrollbar */
    reg->hscrl.off = reg->off[0];
    reg->hscrl.min_size = ctx->cfg.scrl;
    reg->hscrl.total = pan->max[0] - pan->box.x.min;
    reg->hscrl.size = pan->box.x.max - pan->box.x.min + ctx->cfg.scrl;

    if (reg->force_hscrl || reg->hscrl.total > reg->hscrl.size) {
      /* horizontal scrollbar */
      gui_hscrl(ctx, &reg->hscrl, pan);
      if (abs(reg->off[0] - reg->hscrl.off) >= 1) {
        reg->off[0] = reg->hscrl.off;
        reg->scrolled = 1;
      }
    } else {
      reg->off[0] = 0;
    }
    reg->max_off[0] = reg->hscrl.total - reg->hscrl.size;
  }
  if (off) {
    mcpy(off, reg->off, sizeof(reg->off));
  }
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
    lst->space[0] = max(0, lst->box.x.ext - (lst->pad[0] << 1));
    lst->space[1] = max(0, lst->box.y.ext - lst->pad[1]);
    lst->cnt[0] = div_round_up(lst->space[0], lst->slot[0]);
    lst->cnt[0] = max(1, lst->cnt[0]);
    lst->off = lst->offset / lst->slot[1];
    lst->off_idx = lst->off * lst->cnt[0];
    lst->lay.y = gui_min_max(lst->lay.y.min, INT_MAX/2);

    lst->cnt[1] = 0;
    lst->view_cnt = lst->cnt[0];
    lst->page = lst->space[1] / lst->slot[1];
    lst->page_cnt = lst->page * lst->cnt[0];
  } else {
    lst->space[0] = max(0, lst->box.x.ext - lst->pad[0]);
    lst->space[1] = max(0, lst->box.y.ext - (lst->pad[1] << 1));
    lst->cnt[1] = div_round_up(lst->space[1], lst->slot[1]);
    lst->cnt[1] = max(1, lst->cnt[1]);
    lst->off = lst->offset / lst->slot[0];
    lst->off_idx = lst->off * lst->cnt[1];
    lst->lay.x = gui_min_max(lst->lay.x.min, INT_MAX/2);

    lst->cnt[0] = 0;
    lst->view_cnt = lst->cnt[1];
    lst->page = lst->space[0] / lst->slot[0];
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
gui_lst_lay_apply_view(struct gui_lst_lay *lst, const struct gui_lst_view *view) {
  assert(lst);
  assert(view);

  lst->idx = view->begin;
  lst->lay.x = gui_min_max(view->at[0], lst->lay.x.max);
  lst->lay.y = gui_min_max(view->at[1], lst->lay.y.max);
}
static int
gui_lst_lay_center(struct gui_lst_lay *lay, int idx) {
  assert(lay);
  assert(idx >= 0);

  switch (lay->orient) {
    case GUI_HORIZONTAL: {
      int mid_off = (lay->slot[0] * idx) + (lay->slot[0] >> 1);
      return max(0, mid_off - (lay->space[0] >> 1));
    }
    case GUI_VERTICAL: {
      int mid_off = (lay->slot[1] * idx) + (lay->slot[1] >> 1);
      return max(0, mid_off - (lay->space[1] >> 1));
    }
  }
  return 0;
}
static int
gui_lst_lay_fit_start(struct gui_lst_lay *lay, int idx) {
  assert(lay);
  assert(idx >= 0);

  switch (lay->orient) {
    case GUI_HORIZONTAL:
      return lay->slot[0] * idx;
    case GUI_VERTICAL:
      return lay->slot[1] * idx;
  }
  return 0;
}
static int
gui_lst_lay_fit_end(struct gui_lst_lay *lay, int idx) {
  assert(lay);
  assert(idx >= 0);

  assert(lay);
  switch (lay->orient) {
    case GUI_HORIZONTAL:
      return (lay->slot[0] * (idx + 1)) - lay->space[0];
    case GUI_VERTICAL:
      return (lay->slot[1] * (idx + 1)) - lay->space[1];
  }
  return 0;
}
static int
gui_lst_lay_clamp(struct gui_lst_lay *lay, int idx) {
  assert(lay);
  assert(idx >= 0);
  if (idx <= lay->off_idx) {
    return gui_lst_lay_fit_start(lay, idx);
  }
  if (lay->off_idx + lay->page_cnt <= idx) {
    return gui_lst_lay_fit_end(lay, idx);
  }
  return lay->offset;
}

/* ---------------------------------------------------------------------------
 *                              List-Control
 * ---------------------------------------------------------------------------
 */
static void
gui_lst_ctl_focus(struct gui_ctx *ctx, struct gui_lst_ctl *ctl,
                  struct gui_panel *parent, int item_idx, int was_focused) {
  assert(ctx);
  assert(ctl);
  assert(parent);

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
                struct gui_panel *parent, struct gui_id uid) {
  assert(ctx);
  assert(ctl);
  assert(lst);
  assert(box);
  assert(parent);

  struct gui_panel item = {.box = *box};
  gui_panel_id(ctx, &item, parent, uid);
  ctl->owner_id = parent->id;

  int item_idx = lst->idx - 1;
  int focused = gui_id_eq(parent->id, ctx->lst_state.owner) && ctx->lst_state.focused;
  if (ctl->idx++ == 0) {
    ctl->has_focus = parent->is_focused;
    if (!ctl->has_focus) {
      ctx->prev_id = parent->id;
      if (ctx->focus_next) { /* keyboard list focus */
        gui_lst_ctl_focus(ctx, ctl, parent, item_idx, focused);
      }
    } else if (bit_tst_clr(ctx->keys, GUI_KEY_PREV_WIDGET)) {
      ctx->focused = ctx->prev_id;
      if (gui_id_eq(ctx->prev_id, ctx->root.id)) {
        ctx->focus_last = 1;
      }
    }
  }
  /* mouse list focus */
  struct gui_input pin;
  gui_input(&pin, ctx, &item, GUI_BTN_LEFT);
  int pressed = pin.mouse.btn.left.pressed && ctl->focus == GUI_LST_FOCUS_ON_CLK;
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

  struct gui_lst_state *lst = &ctx->lst_state;
  if (lst->focused && !ctl->has_focus &&
      gui_id_eq(ctl->owner_id, lst->owner)) {

    ctl->lost_focus = 1;
    ctl->has_focus = 0;

    lst->owner = ctx->root.id;
    lst->focused = 0;
    lst->cur_idx = 0;
  }
  if (!ctl->has_focus) {
    return;
  }

  int cur_inc = 0;
  int cur_dec = 0;
  int view_inc = 0;
  int view_dec = 0;

  if (ctx->pass == GUI_RENDER && ctl->show_cursor && ctl->cur_vis) {
    /* draw list item cursor */
    const struct gui_box *box = &ctl->cur_box;
    gui_drw_line_style(ctx, 1);
    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_TXT]);
    gui_drw_sbox(ctx, box->x.min - 1, box->y.min - 1, box->x.max - 1, box->y.max - 1);
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
    ctl->item_idx = clamp(0, lst->cur_idx + 1, total);
    ctl->mod = ctl->was_scrolled = 1;
    ctl->key_mod = ctl->cur_mod = 1;
  } else if (cur_dec) {
    ctl->scrl = GUI_LST_CLAMP_ITEM;
    ctl->item_idx = clamp(0, lst->cur_idx - 1, total);
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
gui_lst_sel_elm_input(struct gui_lst_elm_state *elm, struct gui_ctx *ctx,
                      struct gui_lst_sel *sel, const struct gui_lst_lay *lay,
                      const struct gui_lst_ctl *ctl, struct sys *sys,
                      struct gui_panel *item, int is_sel) {
  assert(elm);
  assert(sys);
  assert(ctx);
  assert(sel);
  assert(lay);
  assert(ctl);
  assert(item);

  /* mouse item selection */
  gui_input(&elm->in, ctx, item, GUI_BTN_LEFT);
  elm->hov = (sel->on == GUI_LST_SEL_ON_HOV) && elm->in.is_hot;
  elm->down = ctl->has_focus && ctx->lst_state.cur_idx == lay->idx - 1 &&
            bit_tst_clr(ctx->keys, GUI_KEY_LST_SEL);

  elm->clk = 0;
  if (sel->on == GUI_LST_SEL_ON_CLK) {
    int ctrl = (sys->keymod & SYS_KEYMOD_CTRL);
    int shift = (sys->keymod & SYS_KEYMOD_SHIFT);
    if (sel->mode == GUI_LST_SEL_SINGLE || sel->bhv == GUI_LST_SEL_BHV_TOGGLE) {
      elm->clk = elm->in.mouse.btn.left.pressed;
    } else if (is_sel && !ctrl && !shift) {
      elm->clk = elm->in.mouse.btn.left.clk;
    } else if ((sys->keymod & SYS_KEYMOD_CTRL)) {
      elm->clk = elm->in.mouse.btn.left.clk;
    } else if (sys->keymod & SYS_KEYMOD_SHIFT) {
      elm->clk = elm->in.mouse.btn.left.pressed;
    } else {
      elm->clk = elm->in.mouse.btn.left.pressed;
    }
  }
}
static void
gui_lst_on_sel(struct gui_ctx *ctx, struct gui_lst_sel *sel,
               const struct gui_lst_lay *lay, int is_sel) {
  assert(ctx);
  assert(sel);
  assert(lay);

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
  struct gui_lst_state *lst = &ctx->lst_state;
  if ((sys->keymod & SYS_KEYMOD_SHIFT) && sel->bhv != GUI_LST_SEL_BHV_TOGGLE) {
    sel->op = is_sel ? GUI_LST_SEL_OP_CLR : GUI_LST_SEL_OP_SET;
    if (lst->sel_idx > lay->idx - 1) {
      sel->begin_idx = lay->idx - 1;
      sel->end_idx = lst->sel_idx + 1;
    } else {
      sel->begin_idx = lst->sel_idx;
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
    lst->sel_op = is_sel ? GUI_LST_SEL_OP_CLR : GUI_LST_SEL_OP_SET;
    lst->sel_idx = sel->idx;
  }
  sel->sel_cnt = sel->end_idx - sel->begin_idx;
}
static void
gui_lst_sel_elm(struct gui_ctx *ctx, struct gui_lst_sel *sel,
                const struct gui_lst_lay *lay, const struct gui_lst_ctl *ctl,
                struct gui_box *box, struct gui_panel *parent, int is_sel,
                struct gui_id uid) {
  assert(ctx);
  assert(sel);
  assert(lay);
  assert(ctl);
  assert(box);
  assert(parent);

  struct gui_panel item = {.box = *box};
  gui_panel_id(ctx, &item, parent, uid);
  if (sel->mode == GUI_LST_SEL_SINGLE && sel->src == GUI_LST_SEL_SRC_INT) {
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
      ctx->sys->repaint = 1;
      is_sel = state.hov;
    }
  }
  if (ctx->pass == GUI_RENDER) {
    /* draw selection background */
    if (is_sel) {
      gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SEL]);
      gui_drw_box(ctx, gui_unbox(box));
    } else if (sel->hov == GUI_LST_SEL_HOV_YES && item.is_hot) {
      if (ctx->cfg.col[GUI_COL_CONTENT_HOV] != ctx->cfg.col[GUI_COL_CONTENT]) {
        gui_drw_col(ctx, ctx->cfg.col[GUI_COL_CONTENT_HOV]);
        gui_drw_box(ctx, gui_unbox(box));
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
  struct gui_lst_state *lst = &ctx->lst_state;
  if (sel->mode != GUI_LST_SEL_MULTI) {
    if (ctl->cur_mod) {
      sel->mod = 1;
      sel->op = GUI_LST_SEL_OP_SET;
      sel->idx = ctl->item_idx;
      sel->mut = GUI_LST_SEL_MOD_REPLACE;
      sel->begin_idx = ctl->item_idx;
      sel->end_idx = ctl->item_idx + 1;
      lst->sel_idx = sel->idx;
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
      sel->begin_idx = min(lst->sel_idx, item);
      sel->end_idx = max(lst->sel_idx, item) + 1;
      sel->op = cast(enum gui_lst_sel_op, lst->sel_op);
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
        sel->begin_idx = min(lst->sel_idx, item);
        sel->end_idx = max(lst->sel_idx, item) + 1;
      } else {
        sel->begin_idx = ctl->item_idx;
        sel->end_idx = ctl->item_idx + 1;
        lst->sel_op = GUI_LST_SEL_OP_SET;
        lst->sel_idx = ctl->item_idx;
      }
    }
    if ((sys->keymod & SYS_KEYMOD_SHIFT)) {
      sel->mut = GUI_LST_SEL_MOD_EMPLACE;
    }
  }
  sel->begin_idx = clamp(0, sel->begin_idx, total);
  sel->end_idx = clamp(0, sel->end_idx, total);

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
gui_lst_view(struct gui_lst_view *view, const struct gui_lst_lay *lay) {
  assert(lay);
  assert(view);
  const int total = max(1, view->total_cnt) - 1;
  if (lay->orient == GUI_VERTICAL) {
    int cnt = max(1, lay->cnt[0]);
    view->cnt[1] = div_round_up(total, cnt);
    view->cnt[1] = max(1, view->cnt[1]);
    view->cnt[0] = lay->cnt[0];

    view->begin = lay->off_idx;
    view->end = view->begin + view->cnt[0] * (lay->page + 1);
    view->end = min(view->end, view->total_cnt);

    view->at[0] = lay->lay.x.min;
    view->at[1] = lay->lay.y.min + lay->off * lay->slot[1];
  } else {
    int cnt = max(1, lay->cnt[1]);
    view->cnt[0] = div_round_up(total, cnt);
    view->cnt[0] = max(1, view->cnt[0]);
    view->cnt[1] = lay->cnt[1];

    view->begin = lay->off_idx;
    view->end = view->begin + view->cnt[1] * (lay->page + 1);
    view->end = min(view->end, view->total_cnt);

    view->at[0] = lay->lay.x.min + lay->off * lay->slot[0];
    view->at[1] = lay->lay.y.min;
  }
  view->total[0] = max(1, view->cnt[0]) * lay->slot[0] + lay->pad[0] * 2;
  view->total[1] = max(1, view->cnt[1]) * lay->slot[1] + lay->pad[1] * 2;

  view->max[0] = lay->lay.x.min + view->total[0];
  view->max[1] = lay->lay.y.min + view->total[1];
}

/* ---------------------------------------------------------------------------
 *                                  List
 * ---------------------------------------------------------------------------
 */
static void
gui_lst_cfg_init(struct gui_lst_cfg *cfg, int total_cnt, int off) {
  assert(cfg);
  mset(cfg, 0, sizeof(*cfg));
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
                  int off) {
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
                  struct gui_panel *pan, struct gui_panel *parent,
                  struct gui_id uid, int sel) {
  assert(ctx);
  assert(pan);
  assert(lst);
  assert(parent);

  ctx->id = uid;
  gui_lst_lay_gen(&pan->box, &lst->lay);
  gui_lst_ctl_elm(ctx, &lst->ctl, &lst->lay, &pan->box, parent, uid);
  gui_lst_sel_elm(ctx, &lst->sel, &lst->lay, &lst->ctl, &pan->box, parent, sel, uid);
  gui_panel_begin(ctx, pan, parent);
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
  const struct gui_lst_state *gbl = &ctx->lst_state;
  if (act && (gbl->cur_idx < lst->view.begin || gbl->cur_idx >= lst->view.end)) {
    /* dummy focus list element */
    struct gui_panel elm = {0};
    lst->lay.idx = gbl->cur_idx;

    gui_lst_elm_begin(ctx, lst, &elm, &elm, GUI_LST_ELM_TRAIL_ID, 0);
    gui_lst_elm_end(ctx, lst, &elm, &elm);
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
gui_lst_reg_begin(struct gui_ctx *ctx, struct gui_lst_reg *lag,
                   struct gui_panel *parent, const struct gui_lst_cfg *cfg,
                   const int *off) {
  assert(lag);
  assert(off);
  assert(cfg);
  assert(ctx);
  assert(parent);

  lag->reg.box = lag->box;
  gui_reg_begin(ctx, &lag->reg, parent, off);

  lag->lst.box = lag->reg.pan.box;
  gui_lst_begin(ctx, &lag->lst, cfg);
  gui_reg_apply_lst(&lag->reg, &lag->lst, cfg->lay.scrl_mult);
}
static int
gui_lst_reg_nxt(const struct gui_lst_reg *lag, int idx) {
  return gui_lst_nxt(&lag->lst, idx);
}
static void
gui_lst_reg_elm_begin(struct gui_ctx *ctx, struct gui_lst_reg *lag,
                      struct gui_panel *pan, struct gui_id uid, int sel) {
  assert(lag);
  assert(ctx);
  assert(pan);
  gui_lst_elm_begin(ctx, &lag->lst, pan, &lag->reg.pan, uid, sel);
}
static void
gui_lst_reg_elm_end(struct gui_ctx *ctx, struct gui_lst_reg *lag,
                      struct gui_panel *pan) {
  assert(lag);
  assert(ctx);
  assert(pan);
  gui_lst_elm_end(ctx, &lag->lst, pan, &lag->reg.pan);
}
static void
gui_lst_reg_elm_txt(struct gui_ctx *ctx, struct gui_lst_reg *lag,
                     struct gui_panel *elm, struct gui_id uid, int is_sel,
                     struct str txt, const struct gui_align *align) {
  assert(ctx);
  assert(lag);
  assert(elm);
  gui_lst_reg_elm_begin(ctx, lag, elm, uid, is_sel);
  {
    struct gui_panel item = {.box = elm->box};
    gui_txt(ctx, &item, elm, txt, align);
  }
  gui_lst_reg_elm_end(ctx, lag, elm);
}
static void
gui_lst_reg_elm_txt_ico(struct gui_ctx *ctx, struct gui_lst_reg *lag,
                     struct gui_panel *elm, struct gui_id uid, int is_sel,
                     struct str txt, enum res_ico_id ico_id) {
  assert(ctx);
  assert(lag);
  assert(elm);
  gui_lst_reg_elm_begin(ctx, lag, elm, uid, is_sel);
  {
    struct gui_panel item = {.box = elm->box};
    gui_icon_box(ctx, &item, elm, ico_id, txt);
  }
  gui_lst_reg_elm_end(ctx, lag, elm);
}
static void
gui_lst_reg_center(struct gui_lst_reg *lag, int idx) {
  assert(lag);
  const struct gui_lst_lay *lst = &lag->lst.lay;
  if (lst->orient == GUI_VERTICAL) {
    lag->reg.off[1] = gui_lst_lay_center(&lag->lst.lay, idx);
  } else {
    lag->reg.off[0] = gui_lst_lay_center(&lag->lst.lay, idx);
  }
}
static void
gui_lst_reg_fit_start(struct gui_lst_reg *lag, int idx) {
  assert(lag);
  const struct gui_lst_lay *lst = &lag->lst.lay;
  if (lst->orient == GUI_VERTICAL) {
    lag->reg.off[1] = gui_lst_lay_fit_start(&lag->lst.lay, idx);
  } else {
    lag->reg.off[0] = gui_lst_lay_fit_start(&lag->lst.lay, idx);
  }
}
static void
gui_lst_reg_fit_end(struct gui_lst_reg *lag, int idx) {
  assert(lag);
  const struct gui_lst_lay *lst = &lag->lst.lay;
  if (lst->orient == GUI_VERTICAL) {
    lag->reg.off[1] = gui_lst_lay_fit_end(&lag->lst.lay, idx);
  } else {
    lag->reg.off[0] = gui_lst_lay_fit_end(&lag->lst.lay, idx);
  }
}
static void
gui_lst_reg_clamp(struct gui_lst_reg *lag, int idx) {
  assert(lag);
  const struct gui_lst_lay *lst = &lag->lst.lay;
  if (lst->orient == GUI_VERTICAL) {
    lag->reg.off[1] = gui_lst_lay_clamp(&lag->lst.lay, idx);
  } else {
    lag->reg.off[0] = gui_lst_lay_clamp(&lag->lst.lay, idx);
  }
}
static void
gui_lst_reg_end(struct gui_ctx *ctx, struct gui_lst_reg *lag,
                struct gui_panel *parent, int *off) {
  assert(lag);
  assert(ctx);
  assert(off);

  gui_lst_end(ctx, &lag->lst);
  if (lag->lst.ctl.was_scrolled) {
    switch (lag->lst.ctl.scrl) {
      case GUI_LST_FIT_ITEM_START:
        gui_lst_reg_fit_start(lag, lag->lst.ctl.item_idx);
        break;
      case GUI_LST_FIT_ITEM_END:
        gui_lst_reg_fit_end(lag, lag->lst.ctl.item_idx);
        break;
      case GUI_LST_CLAMP_ITEM:
        gui_lst_reg_clamp(lag, lag->lst.ctl.item_idx);
        break;
    }
  }
  gui_reg_end(ctx, &lag->reg, parent, off);
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
  enum res_ico_id img = is_open ? RES_ICO_COLLAPSE : RES_ICO_EXPAND;
  int ext[2]; res.ico.ext(ext, ctx->res, img);

  struct gui_box cur = {0};
  cur.x = gui_mid_ext(pan->box.x.mid, ext[0]);
  cur.y = gui_mid_ext(pan->box.y.mid, ext[1]);
  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_ICO]);
  gui_drw_ico(ctx, cur.x.min, cur.y.min, img);
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
  gui_panel_cur_hov(ctx, pan, SYS_CUR_HAND);

  struct gui_input pin = {0};
  gui_input(&pin, ctx, pan, GUI_BTN_LEFT);
  return (pin.mouse.btn.left.pressed);
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
  int ext[2]; res.ico.ext(ext, ctx->res, RES_ICO_COLLAPSE);
  ico.box.x = gui_min_ext(pan->box.x.min, ctx->cfg.item);
  ico.box.x = gui_mid_ext(ico.box.x.mid, ext[0]);
  ico.box.y = gui_mid_ext(ico.box.y.mid, ext[1]);

  switch (node->type) {
    case GUI_TREE_LEAF:
      break;
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
    struct gui_input pin = {0};
    gui_input(&pin, ctx, &node->pan, GUI_BTN_LEFT);
    if (pin.mouse.btn.left.doubled) {
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
gui__split_toc(struct gui_split_toc *toc, int *state) {
  assert(toc);
  assert(state);

  toc->col_cnt = state[0];
  toc->lay_cnt = state[1];
  toc->slot = state + 2;
  toc->cons = toc->slot + (toc->col_cnt << 1);
  toc->seps = toc->cons + (toc->col_cnt << 2);
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
  bld->toc.cons[(dst << 2) + 1] = con[1] < 0 ? GUI_SPLIT_MAX_SIZE : con[1];
  bld->toc.cons[(dst << 2) + 2] = bld->sep_size;
  bld->toc.cons[(dst << 2) + 3] = bld->sep_size;
}
static void
gui_split_lay_end(struct gui_split_lay *bld) {
  assert(bld);
  mset(bld->toc.seps, 0, szof(int) * (bld->idx << 1));
}
static void
gui_split_lay(int *state, const struct gui_ctx *ctx,
              const struct gui_split_lay_cfg *cfg) {

  assert(ctx);
  assert(cfg);
  assert(state);

  int cnt = !cfg->total ? cfg->cnt : cfg->total;
  int stride = cfg->size ? cfg->size : szof(struct gui_split_lay_slot);
  const unsigned char *ptr = cast(const unsigned char*, cfg->slots) + cfg->off;

  struct gui_split_lay bld = {0};
  gui_split_lay_begin(&bld, state, cnt, ctx->cfg.sep);
  for loop(i, cnt) {
    int idx = cfg->sort ? cfg->sort[i] : i;
    if (cfg->fltr && bit_tst(cfg->fltr, idx) == 0) {
      continue;
    }
    const struct gui_split_lay_slot *slt = cast(const void*, ptr + (stride * idx));
    gui_split_lay_add(&bld, slt->type, slt->size, slt->con);
  }
  gui_split_lay_end(&bld);
}
static void
gui__split_gen(struct gui_box *item, struct gui_split *spt, const int *siz) {
  switch (spt->orient) {
    case GUI_VERTICAL:
      assert(spt->idx < spt->cnt);
      *item = gui_cut_top(&spt->lay, siz[spt->idx], 0);
      break;
    case GUI_HORIZONTAL:
      assert(spt->idx < spt->cnt);
      *item = gui_cut_lhs(&spt->lay, siz[spt->idx], 0);
      break;
  }
  spt->idx++;
}
static void
gui_split_begin(struct gui_ctx *ctx, struct gui_split *spt,
                struct gui_panel *parent, enum gui_split_type typ,
                enum gui_orient orient, int *items, int item_cnt,
                int *state, int state_cnt) {
  assert(ctx);
  assert(spt);
  assert(items);
  assert(state);
  assert(parent);
  assert(state[0] > 0);

  unused(item_cnt);
  unused(state_cnt);

  int siz = 0;
  struct gui_split_toc toc = {0};
  gui__split_toc(&toc, state);

  assert(item_cnt >= toc.lay_cnt);
  assert(item_cnt >= GUI_SPLIT_COL(toc.col_cnt));
  assert(state_cnt >= GUI_SPLIT_CAP(toc.col_cnt));

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
    toc.seps[0] = spt->cnt;
    toc.seps[1] = siz;
    toc.seps[2] = items[0];

    int *sep = &toc.seps[1];
    for (int i = 1; i < cnt_half; i++) {
      sep[i + 1] = sep[i] + items[((i - 1) * 2) + 1] + items[i * 2];
    }
  } else {
    /* restore panel size from separator positions */
    const int *def = toc.seps + 1;
    for (int i = 0, off = 0; i < cnt_half; ++i) {
      items[i * 2] = def[i + 1] - off;
      items[(i * 2) + 1] = toc.slot[(i * 2) + 1];
      off = def[i + 1] + items[(i * 2) + 1];
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
    if (toc.cons) {
      /* apply (min,max) constraints for both panes */
      int prv = spt->sidx > 2 ? toc.seps[spt->sidx - 1] : 0;
      int fst = toc.seps[spt->sidx] - prv;
      int fci = (spt->sidx - 2) * 2;
      const int *fcn = toc.cons + (fci * 2);

      int nxt = spt->sidx + 1 < (cnt_half) + 2 ? toc.seps[spt->sidx + 1] : sep_pos;
      int sec = nxt - toc.seps[spt->sidx];
      int sci = ((spt->sidx - 2) * 2) + 2;
      const int *scn = toc.cons + (sci * 2);
      if (!between(fst, fcn[0], fcn[1]) || !between(sec, scn[0], scn[1])) {
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
              struct gui_panel *parent, const int *off,
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
gui_tbl_hdr_begin(struct gui_ctx *ctx, struct gui_tbl *tbl, int *items,
                  int item_cnt, int *state, int state_cnt) {
  assert(ctx);
  assert(tbl);

  struct gui_panel *pan = tbl->pan;
  struct gui_split_toc toc = {0};
  gui__split_toc(&toc, state);

  tbl->cnt = toc.col_cnt;
  tbl->hdr_h = ctx->cfg.item;

  int offx = math_floori(tbl->reg.off[0]);
  int offy = math_floori(tbl->reg.off[1]);
  tbl->spt.box.x = gui_min_ext(pan->box.x.min, pan->box.x.ext + offx);
  tbl->spt.box.y = gui_min_ext(pan->box.y.min + offy, ctx->cfg.item);
  gui_split_begin(ctx, &tbl->spt, pan, GUI_SPLIT_EXP, GUI_HORIZONTAL, items,
                  item_cnt, state, state_cnt);
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
  if (tbl->idx+1 < tbl->cnt) {
    gui_split_sep(ctx, &tbl->spt, lay, state);
  }
  if (slot->clk) {
    tbl->resort = 1;
    if (tbl->sort.col == tbl->idx) {
      tbl->sort.order = !tbl->sort.order;
    } else {
      tbl->sort.col = tbl->idx;
      tbl->sort.order = 0;
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

  mset(cfg, 0, sizeof(*cfg));
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
                      const struct gui_tbl_lst_cfg *cfg, int width, int off) {

  gui_lst_cfg_init(ret, cfg->total_cnt, off);
  int lst_pad = cfg->lay.pad[0] < 0 ? ctx->cfg.lst_pad[0] : ret->lay.pad[0];

  ret->lay.orient = GUI_VERTICAL;
  ret->lay.pad[0] = cfg->lay.pad[0];
  ret->lay.pad[1] = cfg->lay.pad[1];
  ret->lay.gap[0] = cfg->lay.gap[0];
  ret->lay.gap[1] = cfg->lay.gap[1];
  ret->lay.item[0] = width;
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

  int winx = lst.x.min + tbl->reg.off[0];
  int winy = tbl->lst.box.y.min + tbl->lst.lay.offset + cfg->lay.pad[1];
  gui_clip_begin(&tbl->clip, ctx, winx, winy, winx + lst.x.ext, winy + lst.y.ext);

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
                      struct gui_panel *elm, struct gui_id uid, int sel) {
  assert(ctx);
  assert(tbl);
  assert(elm);

  tbl->idx = 0;
  struct gui_panel *pan = tbl->pan;
  gui_lst_elm_begin(ctx, &tbl->lst, elm, pan, uid, sel);
  tbl->col_lay = elm->box;
}
static void
gui_tbl_lst_elm_col(struct gui_box *box, struct gui_ctx *ctx,
                    struct gui_tbl *tbl, const int *lay) {
  assert(box);
  assert(ctx);
  assert(tbl);

  if (tbl->idx) {
    gui_clip_end(ctx, &tbl->col_clip);
    gui_cut_lhs(&tbl->col_lay, lay[((tbl->idx - 1) << 1) + 1], 0);
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
        int off = gui_lst_lay_fit_start(&tbl->lst.lay, tbl->lst.ctl.item_idx);
        tbl->reg.off[1] = off;
      } break;
      case GUI_LST_FIT_ITEM_END: {
        int off = gui_lst_lay_fit_end(&tbl->lst.lay, tbl->lst.ctl.item_idx);
        tbl->reg.off[1] = off;
      } break;
      case GUI_LST_CLAMP_ITEM: {
        int off = gui_lst_lay_clamp(&tbl->lst.lay, tbl->lst.ctl.item_idx);
        tbl->reg.off[1] = off;
      }break;
    }
  }
}
static void
gui_tbl_end(struct gui_ctx *ctx, struct gui_tbl *tbl,
            struct gui_panel *parent, int *off) {
  assert(ctx);
  assert(tbl);
  assert(off);
  gui_reg_end(ctx, &tbl->reg, parent, off);
}
static void
gui_tbl_lst_col_txt_ico(struct gui_ctx *ctx, struct gui_tbl *tbl, const int *lay,
                        struct gui_panel *elm, struct gui_panel *item,
                        struct str txt, enum res_ico_id icon) {
  assert(ctx);
  assert(tbl);
  assert(elm);
  assert(item);

  gui_tbl_lst_elm_col(&item->box, ctx, tbl, lay);
  gui_icon_box(ctx, item, elm, icon, txt);
}
static void
gui_tbl_lst_col_txt(struct gui_ctx *ctx, struct gui_tbl *tbl, const int *lay,
                    struct gui_panel *elm, struct gui_panel *item, struct str txt,
                    const struct gui_align *align) {
  assert(ctx);
  assert(tbl);
  assert(elm);
  assert(item);

  gui_tbl_lst_elm_col(&item->box, ctx, tbl, lay);
  gui_txt(ctx, item, elm, txt, align);
}
static void
gui_tbl_lst_txt(struct gui_ctx *ctx, struct gui_tbl *tbl, const int *lay,
                struct gui_panel *elm, struct str txt,
                const struct gui_align *align) {
  struct gui_panel item = {0};
  gui_tbl_lst_col_txt(ctx, tbl, lay, elm, &item, txt, align);
}
static void
gui_tbl_lst_txt_ico(struct gui_ctx *ctx, struct gui_tbl *tbl, const int *lay,
                    struct gui_panel *elm, struct str txt,
                    enum res_ico_id icon) {
  struct gui_panel item = {0};
  gui_tbl_lst_col_txt_ico(ctx, tbl, lay, elm, &item, txt, icon);
}
static void
gui_tbl_lst_col_ico(struct gui_ctx *ctx, struct gui_tbl *tbl, const int *lay,
                    struct gui_panel *elm, struct gui_icon *item,
                    enum res_ico_id ico) {
  assert(ctx);
  assert(tbl);
  assert(elm);
  assert(item);

  gui_tbl_lst_elm_col(&item->box, ctx, tbl, lay);
  gui_icon(ctx, item, elm, ico);
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
               struct gui_panel *elm, const char *fmt, struct tm *time) {
  assert(ctx);
  assert(tbl);
  assert(elm);
  assert(fmt);

  struct gui_panel item = {0};
  gui_tbl_lst_elm_col(&item.box, ctx, tbl, lay);
  gui_tm(ctx, &item, elm, fmt, time);
}
static void
gui_tbl_lst_lnk(struct gui_ctx *ctx, struct gui_tbl *tbl, const int *lay,
                struct gui_panel *elm, struct gui_panel *item, struct str txt,
                const struct gui_align *align) {
  assert(ctx);
  assert(tbl);
  assert(elm);
  assert(item);

  gui_tbl_lst_elm_col(&item->box, ctx, tbl, lay);
  gui_txt_uln(ctx, item, elm, txt, align, 0, txt.rng.cnt);
}

/* ---------------------------------------------------------------------------
 *                              Combo
 * ---------------------------------------------------------------------------
 */
static void
gui_combo_begin(struct gui_ctx *ctx, struct gui_combo *com,
               struct gui_panel *parent) {
  assert(ctx);
  assert(com);
  assert(parent);

  struct gui_panel *pan = &com->pan;
  pan->box = com->box;
  pan->focusable = 1;

  gui_panel_begin(ctx, pan, parent);
  if (ctx->pass == GUI_RENDER &&
      pan->state != GUI_HIDDEN) {
    gui_edit_drw(ctx, pan);
  }
  com->btn.box.x = gui_max_ext(pan->box.x.max, ctx->cfg.scrl);
  com->btn.box.y = gui_min_max(pan->box.y.min + 2, pan->box.y.max - 1);
  gui__scrl_btn(ctx, &com->btn, pan, GUI_EAST);

  com->hdr.y = pan->box.y;
  com->hdr.x = gui_shrink(&pan->box.x, ctx->cfg.pad[0]);
  com->hdr.x = gui_min_max(com->hdr.x.min, com->btn.box.x.min - ctx->cfg.gap[0]);
}
static void
gui_combo_end(struct gui_ctx *ctx, struct gui_combo *com,
                  struct gui_panel *parent) {
  assert(ctx);
  assert(com);
  if (ctx->pass == GUI_RENDER &&
      com->pan.state == GUI_FOCUSED) {
    struct gui_box hdr = com->hdr;
    hdr.x = gui_min_max(com->box.x.min, hdr.x.max);
    gui_focus_drw(ctx, &hdr, 1);
  }
  gui_panel_hot(ctx, &com->pan, parent);
  gui_panel_end(ctx, &com->pan, parent);

  gui_input(&com->in, ctx, &com->pan, GUI_BTN_LEFT);
  if (com->in.mouse.btn.left.clk || com->pan.state == GUI_FOCUSED) {
    if (com->in.mouse.btn.left.clk || bit_tst_clr(ctx->keys, GUI_KEY_ACT)) {
      com->opened = 1;
    }
  }
}
static int
gui_combo(struct gui_ctx *ctx, struct gui_combo *com,
          struct gui_panel *parent, struct str txt) {
  assert(ctx);
  assert(com);
  assert(parent);
  gui_combo_begin(ctx, com, parent);
  {
    static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
    struct gui_panel lbl = {.box = com->hdr};
    gui_txt(ctx, &lbl, &com->pan, txt, &align);
  }
  gui_combo_end(ctx, com, parent);
  return com->opened;
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

  tab->total = cnt;
  tab->tab_w = ctx->cfg.tab;
  tab->sel.idx = sel_idx;
  tab->sel.mod = 0;

  int gap = ctx->cfg.pan_gap[0];
  switch(tab->hdr_pos) {
  case GUI_TAB_HDR_TOP: {
    tab->hdr.x = tab->box.x;
    tab->hdr.y = gui_min_ext(tab->box.y.min, ctx->cfg.item);
    tab->at = tab->hdr.y.max;
    tab->off = tab->box.x.min;

    tab->bdy.x = gui_shrink(&tab->hdr.x, ctx->cfg.pan_pad[0]);
    tab->bdy.y = gui_min_max(tab->hdr.y.max + gap, tab->box.y.max);
    tab->bdy.y = gui_shrink(&tab->bdy.y, ctx->cfg.pan_gap[1]);
  } break;

  case GUI_TAB_HDR_BOT: {
    tab->hdr.x = tab->box.x;
    tab->hdr.y = gui_max_ext(tab->box.y.max, ctx->cfg.item);
    tab->at = tab->hdr.y.min;
    tab->off = tab->box.x.min;

    tab->bdy.x = gui_shrink(&tab->hdr.x, ctx->cfg.pan_pad[0]);
    tab->bdy.y = gui_min_max(tab->box.y.min, tab->hdr.y.min - gap);
    tab->bdy.y = gui_shrink(&tab->bdy.y, ctx->cfg.pan_gap[1]);
  } break;}
  tab->cnt = min(tab->total, (tab->hdr.x.ext - ctx->cfg.scrl) / tab->tab_w);

  tab->pan.box = tab->box;
  gui_panel_begin(ctx, &tab->pan, parent);
}
static void
gui_tab_ctl_sel(struct gui_ctx *ctx, struct gui_tab_ctl *tab, int idx) {
  unused(ctx);
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

  tab->off = hdr->box.x.min;
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

  const struct gui_box *box = &slot->box;
  switch(tab->hdr_pos) {
  case GUI_TAB_HDR_TOP: {
    int top = box->y.min + ((is_act) ? 0 : 1);
    int height = box->y.ext - ((is_act) ? 0 : 1);

    gui_drw_line_style(ctx, 1);
    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_BG]);
    gui_drw_box(ctx, box->x.min, top, box->x.max, top + height);

    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_LIGHT]);
    gui_drw_hln(ctx, top, box->x.min, box->x.max - 1);
    if (!is_act) {
      gui_drw_hln(ctx, box->y.max, box->x.min, box->x.max);
    }
    if (tab->idx == 0 || (tab->idx - 1) != tab->sel.idx) {
      gui_drw_vln(ctx, box->x.min, top, box->y.max);
    }
    if (tab->sel.idx != (tab->idx + 1)) {
      gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SHADOW]);
      gui_drw_vln(ctx, box->x.max - 1, top, box->y.max);
    }
  } break;

  case GUI_TAB_HDR_BOT: {
    int bot = box->y.max - ((is_act) ? 0 : 1);
    int height = box->y.ext - ((is_act) ? 0 : 1);

    gui_drw_line_style(ctx, 1);
    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_BG]);
    gui_drw_box(ctx, box->x.min, bot, box->x.max, bot - height);

    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SHADOW]);
    gui_drw_hln(ctx, bot, box->x.min, box->x.max - 1);
    if (!is_act) {
      gui_drw_hln(ctx, box->y.min, box->x.min, box->x.max);
    }
    if (tab->idx == 0 || (tab->idx - 1) != tab->sel.idx) {
      gui_drw_col(ctx, ctx->cfg.col[GUI_COL_LIGHT]);
      gui_drw_vln(ctx, box->x.min, bot, box->y.min);
    }
    if (tab->sel.idx != (tab->idx + 1)) {
      gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SHADOW]);
      gui_drw_vln(ctx, box->x.max - 1, box->y.min, bot);
    }
  } break;}
}
static void
gui_tab_hdr_slot_begin(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                       struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot,
                       struct gui_id uid) {
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
  hdr->id = uid;
  tab->off = slot->box.x.max;
  slot->box.x = gui_shrink(&slot->box.x, ctx->cfg.pan_pad[0]);
  hdr->slot = slot->box;
}
static void
gui_tab_hdr_slot_end(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                     struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot,
                     struct gui_input *pin) {
  assert(ctx);
  assert(tab);
  assert(slot);
  unused(hdr);

  struct gui_input ins = {0};
  pin = !pin ? &ins : pin;
  slot->id = gui_gen_id(hdr->id, tab->pan.id);
  gui_panel_hot(ctx, slot, &tab->pan);
  gui_input(pin, ctx, slot, GUI_BTN_LEFT);

  /* selection */
  if (pin->mouse.btn.left.pressed) {
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
  if (pin->mouse.btn.left.dragged) {
    int min_x = hdr->slot.x.min - (hdr->slot.x.ext >> 1);
    int max_x = hdr->slot.x.max + (hdr->slot.x.ext >> 1);
    if (tab->idx > 0 && pin->mouse.pos[0] < min_x) {
      tab->sort.mod = 1;
      tab->sort.dst = tab->idx - 1;
      tab->sort.src = tab->idx;

      tab->sel.mod = 1;
      tab->sel.idx = tab->sort.dst;
    } else if (tab->idx + 1 < tab->total && pin->mouse.pos[0] > max_x) {
      tab->sort.mod = 1;
      tab->sort.dst = tab->idx + 1;
      tab->sort.src = tab->idx;

      tab->sel.mod = 1;
      tab->sel.idx = tab->sort.dst;
    }
  }
  if (ctx->pass == GUI_RENDER &&
      gui_id_eq(slot->id, ctx->focused) &&
      !ctx->focus_next) {
    gui_focus_drw(ctx, &hdr->slot, 0);
  }
  tab->idx++;
}
static void
gui_tab_hdr_slot(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                 struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot,
                 struct gui_id uid, struct str txt) {
  assert(ctx);
  assert(tab);
  assert(hdr);
  assert(slot);

  static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
  gui_tab_hdr_slot_begin(ctx, tab, hdr, slot, uid);
  gui_txt(ctx, slot, &hdr->pan, txt, &align);
  gui_tab_hdr_slot_end(ctx, tab, hdr, slot, 0);
}
static void
gui_tab_hdr_item(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                 struct gui_tab_ctl_hdr *hdr, struct gui_id uid,
                 struct str txt) {
  assert(ctx);
  assert(tab);
  assert(hdr);

  struct gui_panel slot = {0};
  gui_tab_hdr_slot(ctx, tab, hdr, &slot, uid, txt);
}
static void
gui__tab_ctl_drw(struct gui_ctx *ctx, const struct gui_tab_ctl *tab,
                 const struct gui_panel *pan) {
  assert(ctx);
  assert(tab);
  assert(pan);
  const struct gui_box *box = &pan->box;

  switch(tab->hdr_pos) {
  case GUI_TAB_HDR_TOP: {
    gui_drw_line_style(ctx, 1);
    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_LIGHT]);
    gui_drw_hln(ctx, tab->at, tab->off, box->x.max - 2);
    gui_drw_vln(ctx, box->x.min, tab->at, box->y.max - 1);

    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SHADOW]);
    gui_drw_hln(ctx, box->y.max - 1, box->x.min + 1, box->x.max - 2);
    gui_drw_vln(ctx, box->x.max - 2, tab->at, box->y.max - 1);
  } break;

  case GUI_TAB_HDR_BOT: {
    gui_drw_line_style(ctx, 1);
    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_LIGHT]);
    gui_drw_hln(ctx, box->y.min, box->x.min, box->x.max);
    gui_drw_vln(ctx, box->x.min, box->y.min, tab->at);

    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_SHADOW]);
    gui_drw_hln(ctx, tab->at, tab->off, box->x.max - 2);
    gui_drw_vln(ctx, box->x.max - 2, box->y.min, tab->at);
  } break;}
}
static void
gui_tab_ctl_end(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                struct gui_panel *parent) {
  assert(ctx);
  assert(tab);
  assert(parent);
  if (ctx->pass == GUI_RENDER &&
      tab->pan.state != GUI_HIDDEN) {
    gui__tab_ctl_drw(ctx, tab, &tab->pan);
  }
  gui_panel_end(ctx, &tab->pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  Grid
 * ---------------------------------------------------------------------------
 */
static void
gui__grid_drw(struct gui_ctx *ctx, struct gui_box *box, unsigned flags,
              int off_x, int off_y) {
  assert(box);
  assert(ctx);

  int size = ctx->cfg.grid;
  gui_drw_line_style(ctx, 1);
  gui_drw_col(ctx, ctx->cfg.col[GUI_COL_TXT_DISABLED]);
  if (flags & GUI_GRID_X) {
    assert(size > 0);
    int posx = (off_x < 0) ? (abs(off_x) % size) : (size - (off_x % size));
    for (; posx < box->x.ext; posx += size) {
      gui_drw_vln(ctx, box->x.min + posx, box->y.min, box->y.max);
    }
  }
  if (flags & GUI_GRID_Y) {
    assert(size > 0);
    int posy = (off_y < 0) ? (abs(off_y) % size) : (size - (off_y % size));
    for (; posy < box->y.ext; posy += size) {
      gui_drw_hln(ctx, box->y.min + posy, box->x.min, box->x.max);
    }
  }
}
static void
gui_grid_begin(struct gui_ctx *ctx, struct gui_grid *grid,
               struct gui_panel *parent, const int *off) {
  assert(ctx);
  assert(off);
  assert(grid);
  assert(parent);

  grid->pan.box = grid->box;
  if (grid->flags == GUI_GRID_DFLT) {
    grid->flags = GUI_GRID_XY;
  }
  gui_panel_begin(ctx, &grid->pan, parent);
  mcpy(grid->off, off, sizeof(grid->off));

  int off_x = math_floori(grid->off[0]);
  int off_y = math_floori(grid->off[1]);

  /* calculate clip area */
  struct gui_box *box = &grid->pan.box;
  grid->space = gui_padv(box, ctx->cfg.pan_pad);
  if (ctx->pass == GUI_RENDER && grid->pan.state != GUI_HIDDEN) {
    gui_reg_drw(ctx, &grid->box);
    gui__grid_drw(ctx, &grid->space, grid->flags, off_x, off_y);
    gui_clip_begin(&grid->clip, ctx, grid->space.x.min, grid->space.y.min,
                   grid->space.x.max, grid->space.y.max);
  }
  /* apply scroll offset to area body */
  box->x = gui_min_max(grid->space.x.min - off_x, INT_MAX);
  box->y = gui_min_max(grid->space.y.min - off_y, INT_MAX);
}
static void
gui_grid_end(struct gui_ctx *ctx, struct gui_grid *grid,
             struct gui_panel *parent, int *off) {

  assert(off);
  assert(ctx);
  assert(grid);
  assert(parent);

  grid->scrolled = 0;
  struct gui_panel *pan = &grid->pan;
  gui_panel_end(ctx, pan, parent);
  if (ctx->pass == GUI_RENDER &&
      pan->state != GUI_HIDDEN) {
    gui_clip_end(ctx, &grid->clip);
  }
  struct gui_input pin = {0};
  gui_input(&pin, ctx, pan, GUI_BTN_RIGHT);
  if (pin.mouse.btn.right.dragged) {
    struct sys *sys = ctx->sys;
    grid->off[0] -= sys->mouse.pos_delta[0];
    grid->off[1] -= sys->mouse.pos_delta[1];
    grid->scrolled = 1;
  }
  mcpy(off, grid->off, sizeof(grid->off));
}

/* ---------------------------------------------------------------------------
 *                                  Node
 * ---------------------------------------------------------------------------
 */
static void
gui_graph_node_begin(struct gui_ctx *ctx, struct gui_graph_node *node,
                     struct gui_panel *parent) {
  assert(ctx);
  assert(node);
  assert(parent);

  node->pan.box = gui_box_mov(&node->box, parent->box.x.min, parent->box.y.min);
  gui_panel_begin(ctx, &node->pan, parent);
  if (ctx->pass == GUI_RENDER && node->pan.state != GUI_HIDDEN) {
    gui_panel_drw(ctx, &node->pan.box);
  }
  node->lay = gui_pad(&node->pan.box, 2, 2);
}
static void
gui_graph_node_item(struct gui_box *ret, struct gui_ctx *ctx,
                    struct gui_graph_node *node, int item_h) {
  assert(ret);
  assert(ctx);
  assert(node);

  item_h = !item_h ? ctx->cfg.item : item_h;
  *ret = gui_cut_top(&node->lay, item_h, 0);
}
static void
gui_graph_node_hdr_begin(struct gui_ctx *ctx, struct gui_graph_node *node,
                         struct gui_graph_node_hdr *hdr) {
  assert(hdr);
  assert(ctx);
  assert(node);

  hdr->pan.box = hdr->box;
  hdr->pan.box.x = gui_min_max(hdr->box.x.min, hdr->box.x.max - 2);
  gui_panel_begin(ctx, &hdr->pan, &node->pan);
  if (ctx->pass == GUI_RENDER && hdr->pan.state != GUI_HIDDEN) {
    gui_drw_col(ctx, ctx->cfg.col[GUI_COL_CONTENT]);
    gui_drw_box(ctx, gui_unbox(&hdr->pan.box));
  }
  hdr->content = hdr->pan.box;
  hdr->content.x = gui_shrink(&hdr->pan.box.x, 1);
}
static void
gui_graph_node_hdr_end(struct gui_ctx *ctx, struct gui_graph_node *node,
                       struct gui_graph_node_hdr *hdr) {
  assert(hdr);
  assert(ctx);
  assert(node);

  gui_panel_hot(ctx, &hdr->pan, &node->pan);
  gui_panel_end(ctx, &hdr->pan, &node->pan);

  gui_input(&hdr->in, ctx, &hdr->pan, GUI_BTN_LEFT);
  hdr->mov_begin = hdr->in.mouse.btn.left.drag_begin;
  hdr->mov_end = hdr->in.mouse.btn.left.drag_end;
  hdr->moved = hdr->in.mouse.btn.left.dragged;

  if (hdr->mov_begin) {
    ctx->drag_state[0] = node->box.x.min;
    ctx->drag_state[1] = node->box.y.min;
  }
  if (hdr->moved) {
    int dpx = hdr->in.mouse.btn.left.drag_pos[0];
    int dpy = hdr->in.mouse.btn.left.drag_pos[1];
    int dstx = hdr->in.mouse.pos[0] - dpx;
    int dsty = hdr->in.mouse.pos[1] - dpy;
    hdr->pos[0] = ctx->drag_state[0] + dstx;
    hdr->pos[1] = ctx->drag_state[1] + dsty;
  }
}
static void
gui_graph_node_end(struct gui_ctx *ctx, struct gui_graph_node *node,
                   struct gui_panel *parent) {
  assert(ctx);
  assert(node);
  assert(parent);
  gui_panel_end(ctx, &node->pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
static const struct gui_api gui__api = {
  .version = GUI_VERSION,
  .init = gui_init,
  .begin = gui_begin,
  .end = gui_end,
  .enable = gui_enable,
  .disable = gui_disable,
  .color_scheme = gui_color_scheme,
  .tooltip = gui_tooltip,
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
  .drw = {
    .col = gui_drw_col,
    .line_style = gui_drw_line_style,
    .box = gui_drw_box,
    .line = gui_drw_ln,
    .circle = gui_drw_circle,
    .tri = gui_drw_tri,
    .glyph = gui_drw_glyph,
    .rune = gui_drw_rune,
    .txt = gui_drw_txt,
    .ico = gui_drw_ico,
    .sprite = gui_drw_sprite,
    .img = gui_drw_img,
    .blit = gui_drw_blit,
  },
  .clip = {
    .begin = gui_clip_begin,
    .end = gui_clip_end,
  },
  .dnd = {
    .src = {
      .begin = gui_dnd_src_begin,
      .set = gui_dnd_src_set,
      .end = gui_dnd_src_end,
    },
    .dst = {
      .begin = gui_dnd_dst_begin,
      .get = gui_dnd_dst_get,
      .end = gui_dnd_dst_end,
    },
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
    .cur = {
      .hov = gui_panel_cur_hov,
    },
  },
  .cfg = {
    .init = gui_dflt_cfg,
    .scale = gui_cfg_scale,
    .pushi = gui_cfg_pushi,
    .pushi_on = gui_cfg_pushi_on,
    .pushu = gui_cfg_pushu,
    .pushu_on = gui_cfg_pushu_on,
    .pop = gui_cfg_pop,
    .pop_on = gui_cfg_pop_on,
  },
  .in = {
    .eat = gui_input_eat,
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
    .img = gui_ico,
    .clk = gui_icon,
    .box = gui_icon_box,
  },
  .btn = {
    .begin = gui_btn_begin,
    .end = gui_btn_end,
    .txt = gui_btn_txt,
    .lbl = gui_btn_lbl,
    .ico = gui_btn_ico,
    .ico_txt = gui_btn_ico_txt
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
    .num = gui_spin_int,
    .i = gui_spin_i,
    .uint = gui_spin_uint,
    .u = gui_spin_u,
#ifdef GUI_USE_FLT
    .flt = gui_spin_flt,
    .f = gui_spin_f,
#endif
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
      .ctr = gui_lst_lay_center,
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
      .ctr = gui_lst_reg_center,
      .fit_start = gui_lst_reg_fit_start,
      .fit_end = gui_lst_reg_fit_end,
      .clamp = gui_lst_reg_clamp,
      .end = gui_lst_reg_end,
      .elm = {
        .begin = gui_lst_reg_elm_begin,
        .end = gui_lst_reg_elm_end,
        .txt = gui_lst_reg_elm_txt,
        .txt_ico = gui_lst_reg_elm_txt_ico,
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
          .pan = {
            .txt = gui_tbl_lst_col_txt,
            .txt_ico = gui_tbl_lst_col_txt_ico,
          },
          .slot = gui_tbl_lst_elm_col,
          .ico = gui_tbl_lst_col_ico,
          .txt = gui_tbl_lst_txt,
          .txt_ico = gui_tbl_lst_txt_ico,
          .txtf = gui_tbl_lst_txtf,
          .tm = gui_tbl_lst_tm,
          .lnk = gui_tbl_lst_lnk,
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
        .txt = gui_tab_hdr_slot,
      },
      .item = {
        .txt = gui_tab_hdr_item,
      },
    },
  },
  .grid = {
    .begin = gui_grid_begin,
    .end = gui_grid_end,
  },
  .graph_node = {
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
gui_api(void *export, void *import) {
  unused(import);
  struct gui_api *api = (struct gui_api*)export;
  *api = gui__api;
}

