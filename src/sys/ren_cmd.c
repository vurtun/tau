#define ren_clip(x, y, w, h) (struct ren_clip) { x, y, (x) + (w), (y) + (h) }
struct ren_clip {
  int min_x, min_y;
  int max_x, max_y;
};
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
struct ren_state {
  unsigned col;
  int thick, img;
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
  int vtx_cnt;
  int idx_cnt;
  int cmd_cnt;
};
#define REN_MAX_CMD_BUF_CNT 8
struct ren_cmd_que {
  struct arena *mem;
  struct sys *sys;
  int cnt;
  struct ren_cmd_buf bufs[REN_MAX_CMD_BUF_CNT];
};
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
    struct ren_op_buf *obn = arena_obj(buf->mem, buf->sys, struct ren_op_buf);
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
ren_reg_op(struct ren_cmd_buf *buf, unsigned h, int vtx_cnt, int idx_cnt,
           int cmd_cnt) {
  buf->hash = fnv1au32(buf->hash, h);
  buf->vtx_cnt += vtx_cnt;
  buf->idx_cnt += idx_cnt;
  buf->cmd_cnt += cmd_cnt;
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
  ren_reg_op(buf, p[0].hdr.hash, 0, 0, 1);
}
static void
ren_box(struct ren_cmd_buf *buf, int x0, int y0, int x1, int y1) {
  union ren_op *p = ren_push_op(buf, ren__op_tbl_size[REN_OP_BOX]);
  p[0].hdr = (struct ren_op_hdr){.op = REN_OP_BOX};
  p[1].pos = (struct ren_op_pos){x0, y0};
  p[2].siz = (struct ren_op_siz){x1 - x0, y1 - y0};
  p[0].hdr.hash = fnv1a32(p, 3 * szof(union ren_op), FNV1A32_HASH_INITIAL);
  p[0].hdr.hash = fnv1a32(&buf->state.col, szof(buf->state.col), p[0].hdr.hash);
  ren_reg_op(buf, p[0].hdr.hash, 4, 6, 0);
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
  ren_reg_op(buf, p[0].hdr.hash, 16, 24, 0);
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
  ren_reg_op(buf, p[0].hdr.hash, 4, 6, 1);
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
  buf->state.img = REN_TEX_MAX;
  buf->vtx_cnt = 0;
  buf->idx_cnt = 0;
  buf->cmd_cnt = 0;
  buf->ops = 0;
  return buf;
}

