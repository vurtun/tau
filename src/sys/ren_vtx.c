struct ren_drw_cmd {
  unsigned elm_cnt;
  struct sys_rect clip;
  unsigned long long tex;
};
struct ren_nil_tex {
  unsigned long long tex;
  float uv[2];
};
struct ren_buf {
  void *mem;
  int at, siz;
  int req;
};
struct ren_vtx {
  float pos[2];
  float uv[2];
  unsigned char col[4];
};
enum ren_conv_res {
  REN_CONV_OK = 0,
  REN_CONV_VTX_BUF_FULL = 0x02,
  REN_CONV_IDX_BUF_FULL = 0x04,
  REN_CONV_CMD_BUF_FULL = 0x08,
  REN_CONV_PATH_BUF_FULL = 0x10,
};
struct ren_drw_buf {
  /* in: */
  float alpha;
  struct ren_nil_tex nil;
  struct ren_buf cmd;
  struct ren_buf vtx;
  struct ren_buf elm;
  struct ren_buf path;

  /* internal: */
  int elm_cnt;
  int vtx_cnt;
  int cmd_cnt;
  int cmd_off;
  int path_cnt;

  struct sys_rect clip;
  unsigned long long prev_tex;
  struct ren_drw_cmd *cur_cmd;
};
static void
ren_drw_buf_init(struct ren_drw_buf *buf) {
  assert(buf);
  assert(buf->cmd.mem);
  assert(buf->cmd.siz);
  assert(buf->vtx.mem);
  assert(buf->vtx.siz);
  assert(buf->elm.mem);
  assert(buf->elm.siz);
  assert(buf->path.mem);
  assert(buf->path.siz);
  assert(buf->alpha > 0.0f);

  buf->cmd_off = 0;
  buf->elm_cnt = 0;
  buf->vtx_cnt = 0;
  buf->cmd_cnt = 0;
  buf->path_cnt = 0;
  buf->prev_tex = buf->nil.tex;

  buf->clip.x = 0;
  buf->clip.y = 0;
  buf->clip.w = 16*1024.0f;
  buf->clip.h = 16*1024.0f;

  buf->cmd.req = 0;
  buf->vtx.req = 0;
  buf->elm.req = 0;
  buf->path.req = 0;
}
static void*
ren_buf_alloc(struct ren_buf *buf, int siz) {
  assert(buf);
  assert(buf->mem);
  assert(buf->siz);
  assert(siz > 0);

  unsigned char *mem = buf->mem;
  unsigned char *at = mem + buf->at;
  int space = buf->siz - buf->at;
  if (space < siz) {
    buf->at = buf->siz;
    buf->req += siz;
    return 0;
  }
  buf->at += siz;
  buf->req += siz;
  return at;
}
static float*
ren_drw_buf_path_new(struct ren_drw_buf *buf, int cnt) {
  assert(buf);
  assert(cnt > 0);
  float *pnts = ren_buf_alloc(&buf->path, 2 * szof(float));
  if (!pnts) {
    return 0;
  }
  buf->path_cnt += cnt;
  return pnts;
}
static struct ren_drw_cmd*
ren_drw_buf_cmd(struct ren_drw_buf *buf, struct sys_rect clip,
                unsigned long long tex) {
  assert(buf);
  struct ren_drw_cmd *cmd = ren_buf_alloc(&buf->cmd, szof(struct ren_drw_cmd));
  if (!cmd) {
    return 0;
  }
  cmd->elm_cnt = 0;
  cmd->clip = clip;
  cmd->tex = tex;

  buf->cur_cmd = cmd;
  buf->cmd_cnt++;
  buf->clip = clip;
  buf->prev_tex = buf->nil.tex;
  return cmd;
}
static struct ren_drw_cmd*
ren_drw_buf_scissor(struct ren_drw_buf *buf, struct sys_rect clip) {
  assert(buf);
  return ren_drw_buf_cmd(buf, clip, buf->prev_tex);
}
static struct ren_drw_cmd*
ren_drw_buf_tex(struct ren_drw_buf *buf, unsigned long long tex) {
  assert(buf);
  if (buf->prev_tex == tex) {
    return buf->cur_cmd;
  }
  return ren_drw_buf_cmd(buf, buf->clip, tex);
}
static struct ren_vtx*
ren_drw_buf_vtx(struct ren_drw_buf *buf, int cnt) {
  assert(buf);
  assert(cnt > 0);
  int siz = szof(struct ren_vtx) * cnt;
  struct ren_vtx *vtx = ren_buf_alloc(&buf->vtx, siz);
  if (!vtx) {
    return 0;
  }
  buf->vtx_cnt += cnt;
  assert(buf->vtx_cnt < KB(64));
  return vtx;
}
static unsigned short*
ren_drw_buf_elm(struct ren_drw_buf *buf, int cnt) {
  assert(buf);
  assert(cnt > 0);
  if (!buf->cur_cmd) {
    assert(buf->cur_cmd);
    return 0;
  }
  struct ren_drw_cmd *cmd = buf->cur_cmd;
  int siz = szof(unsigned short) * cnt;
  unsigned short *idx = ren_buf_alloc(&buf->elm, siz);
  if (!idx) {
    return 0;
  }
  buf->elm_cnt += cnt;
  cmd->elm_cnt += cnt;
  return idx;
}
static void
ren_drw_buf_poly(struct ren_drw_buf *buf, const float *pnts, int cnt,
                 unsigned color) {
  assert(buf);
  assert(pnts);
  assert(cnt > 1);

  unsigned char ucol[4]; col_unpaq(ucol, color);
  float colf[4]; map4(colf, flt_byte, ucol);
  colf[3] = colf[3] * buf->alpha;
  unsigned colb[4]; map4(colb, byte_flt, colf);

  int idx_at = buf->vtx_cnt;
  int idx_cnt = (cnt-1)*3;
  int vtx_cnt = cnt;

  struct ren_vtx *vtx = ren_drw_buf_vtx(buf, vtx_cnt);
  unsigned short *idx = ren_drw_buf_elm(buf, idx_cnt);
  if (!vtx || !idx) {
    return;
  }
  for (int i = 0; i < vtx_cnt; ++i) {
    cpy2(vtx[i].pos, pnts + i * 2);
    cpy2(vtx[i].uv, buf->nil.uv);
    cpy4(vtx[i].col, colb);
  }
  for (int i = 2; i < cnt; ++i) {
    idx[0] = cast(unsigned short, idx_at);
    idx[1] = cast(unsigned short, idx_at + i - 1);
    idx[2] = cast(unsigned short, idx_at + i);
    idx_at += 3;
  }
}
static void
ren_drw_buf_path_clr(struct ren_drw_buf *buf) {
  assert(buf);
  buf->path.at = 0;
  buf->path_cnt = 0;
}
static void
ren_drw_buf_path_line_to(struct ren_drw_buf *buf, const float *pos) {
  assert(buf);
  assert(pos);
  if (!buf->cmd_cnt) {
    ren_drw_buf_cmd(buf, buf->clip, buf->nil.tex);
  }
  struct ren_drw_cmd *cmd = buf->cur_cmd;
  if (cmd->tex != buf->nil.tex) {
    ren_drw_buf_tex(buf, buf->nil.tex);
  }
  float *pnt = ren_drw_buf_path_new(buf, 1);
  if (!pnt) {
    return;
  }
  cpy2(pnt, pos);
}
static void
ren_drw_buf_path_rect_to(struct ren_drw_buf *buf, const float *a,
                         const float *b) {
  assert(a);
  assert(b);
  assert(buf);

  ren_drw_buf_path_line_to(buf, a);
  ren_drw_buf_path_line_to(buf, (float[]){b[0],a[1]});
  ren_drw_buf_path_line_to(buf, b);
  ren_drw_buf_path_line_to(buf, (float[]){a[0],b[1]});
}
static void
ren_daw_buf_path_fill(struct ren_drw_buf *buf, unsigned col) {
  assert(buf);
  if (!buf->path.at) {
    return;
  }
  ren_drw_buf_poly(buf, buf->path.mem, buf->path_cnt, col);
  ren_drw_buf_path_clr(buf);
}
static void
ren_drw_buf_fill_rect(struct ren_drw_buf *buf, struct sys_rect r, unsigned col) {
  assert(buf);
  if (!col_a(col)) {
    return;
  }
  ren_drw_buf_path_rect_to(buf, &r.x, (float[]){r.x + r.w, r.y + r.h});
  ren_daw_buf_path_fill(buf, col);
}
static void
ren_drw_buf_rect_uv(struct ren_drw_buf *buf, const int *restrict a,
                    int float *restrict c, const float *restrict uva,
                    const float *uvc, int img_id, unsigned col) {
  assert(a);
  assert(c);
  assert(buf);
  assert(uva);
  assert(uvc);

  int vidx = cast(unsigned short, buf->vtx_cnt);
  struct ren_vtx *vtx = ren_drw_buf_vtx(buf, 4);
  unsigned short *idx = ren_drw_buf_elm(buf, 6);
  if (!vtx || !idx) {
    return;
  }
  int b[2]; set2(b, c[0], a[1]);
  int d[2]; set2(d, a[0], c[1]);

  idx[0] = cast(unsigned short, vidx+0); idx[1] = cast(unsigned short, vidx+1);
  idx[2] = cast(unsigned short, vidx+2); idx[3] = cast(unsigned short, vidx+0);
  idx[4] = cast(unsigned short, vidx+2); idx[5] = cast(unsigned short, vidx+3);

  vtx[0] = (struct ren_vtx){.pos = {a[0],a[1]}, .uv = {uva[0],uva[1]}, col};
  vtx[1] = (struct ren_vtx){.pos = {b[0],b[1]}, .uv = {uvb[0],uvb[1]}, col};
  vtx[2] = (struct ren_vtx){.pos = {c[0],c[1]}, .uv = {uvc[0],uvc[1]}, col};
  vtx[3] = (struct ren_vtx){.pos = {d[0],d[1]}, .uv = {uvd[0],uvd[1]}, col};
}
static void
ren_drw_buf_img(struct ren_drw_buf *buf, unsigned long long tex,
                struct sys_rect d, struct sys_rect s, const int *img_siz,
                unsigned col) {
  assert(buf);
  assert(buf);
  ren_drw_buf_tex(buf, tex);

  float uv[4];
  uv[0] = s.x/cast(float, img_siz[0]);
  uv[1] = s.y/cast(float, img_siz[1]);
  uv[2] = (s->x + s->w)/cast(float, img_siz[0]);
  uv[3] = (s->y + s->h)/cast(float, img_siz[1]);
  ren_drw_buf_rect_uv(buf, &d.x, (float[]){d.x+d.w, d.y+d.h}, uv, uv+2, col);
}
static unsigned
ren_conv(struct ren_drw_buf *buf, struct ren *ren, int w, int h) {
  unsigned res = REN_CONV_OK;
  for_cnt(i, ren->que.cnt) {
    struct ren_cmd_buf *cmd = ren->que.bufs + i;
    const struct ren_clip clip = ren_clip(0, 0, w, h);
    struct ren_state s = {.clip = clip, .thick = 1};
    for (union ren_op *p = ren_op_begin(cmd->ops); p; p = ren_op_next(p)) {
      switch (p[0].hdr.op) {
      case REN_OP_STYL: s.thick = cast(int, p[1].word); break;
      case REN_OP_COL: s.col = cast(unsigned, p[1].word); break;
      case REN_OP_CLIP: {
        struct ren_clip c = ren_clip(p[1].pos.x, p[1].pos.y, p[2].siz.w, p[2].siz.h);
        s.clip.min_x = max(dirty->min_x, c.min_x);
        s.clip.min_y = max(dirty->min_y, c.min_y);
        s.clip.max_x = min(dirty->max_x, c.max_x);
        s.clip.max_y = min(dirty->max_y, c.max_y);
      } break;
      case REN_OP_BOX: {
        struct sys_rect r = sys_rect(p[1].pos.x, p[1].pos.y, p[2].siz.w, p[2].siz.h);
        ren_drw_buf_fill_rect(buf, r, s.col);
      } break;
      case REN_OP_SBOX: {
        struct sys_rect r = sys_rect(p[1].pos.x, p[1].pos.y, p[2].siz.w, p[2].siz.h);
        struct sys_rect a = sys_rect(r.x, r.y, r.w + 1, thick);
        struct sys_rect b = sys_rect(r.x + r.w, r.y, thick, r.h + 1);
        struct sys_rect c = sys_rect(r.x, r.y + r.h, r.w + 1, thick);
        struct sys_rect d = sys_rect(r.x, r.y, thick, r.h + 1);

        ren_drw_buf_fill_rect(buf, a, s.col);
        ren_drw_buf_fill_rect(buf, b, s.col);
        ren_drw_buf_fill_rect(buf, c, s.col);
        ren_drw_buf_fill_rect(buf, d, s.col);
      } break;
      case REN_OP_IMG: {
        int img_id = p[3].img.id;
        assert (img_id < REN_TEX_MAX && img_id >= 0);
        if (img_id >= REN_TEX_MAX || img_id < 0) {
          break;
        }
        assert(ren->tex[img_id].act);
        const struct ren_tex *tex = ren->tex + img_id;
        struct sys_rect s = sys_rect(p[3].img.sx, p[3].img.sy, p[2].siz.w, p[2].siz.h);
        struct sys_rect d = sys_rect(p[1].pos.x, p[1].pos.y, p[2].siz.w, p[2].siz.h);
        ren_drw_buf_img(buf, img_id, d, s, &tex->w, s.col);
      } break;}
    }
  }
  res = (buf->cmd.req > buf->cmd.siz) ? res|REN_CONV_CMD_BUF_FULL : res;
  res = (buf->vtx.req > buf->vtx.siz) ? res|REN_CONV_VTX_BUF_FULL : res;
  res = (buf->elm.req > buf->elm.siz) ? res|REN_CONV_ELM_BUF_FULL : res;
  res = (buf->path.req > buf->path.siz) ? res|REN_CONV_PATH_BUF_FULL : res;
  return res;
}

