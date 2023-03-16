#include "ttf.c"
/* ---------------------------------------------------------------------------
 *
 *                                  Images
 *
 * --------------------------------------------------------------------------- */
static unsigned char *res__barrier;
static unsigned char *res__barrier2;
static unsigned char *res__barrier3;
static unsigned char *res__barrier4;
static unsigned char *res__dout;

static unsigned
res__decompress_len(unsigned char *input) {
    return (unsigned int)((input[8] << 24) +
            (input[9] << 16) + (input[10] << 8) + input[11]);
}
static void
res__match(unsigned char *data, unsigned int length) {
  /* INVERSE of memmove... write each byte before copying the next...*/
  assert(res__dout + length <= res__barrier);
  if (res__dout + length > res__barrier) {
    res__dout += length; return;
  }
  if (data < res__barrier4) {
    res__dout = res__barrier+1;
    return;
  }
  while (length--) {
    *res__dout++ = *data++;
  }
}
static void
res__lit(unsigned char *data, unsigned int length) {
  assert(res__dout + length <= res__barrier);
  if (res__dout + length > res__barrier) {
    res__dout += length; return;
  }
  if (data < res__barrier2) {
    res__dout = res__barrier+1;
    return;
  }
  memcpy(res__dout, data, length);
  res__dout += length;
}
static unsigned char*
res__decompress_token(unsigned char *i) {
  #define res__in2(x)   ((i[x] << 8) + i[(x)+1])
  #define res__in3(x)   ((i[x] << 16) + res__in2((x)+1))
  #define res__in4(x)   ((i[x] << 24) + res__in3((x)+1))
  if (*i >= 0x20) {       /* use fewer if's for cases that expand small */
    if (*i >= 0x80)       res__match(res__dout-i[1]-1, (unsigned int)i[0] - 0x80 + 1), i += 2;
    else if (*i >= 0x40)  res__match(res__dout-(res__in2(0) - 0x4000 + 1), (unsigned int)i[2]+1), i += 3;
    else /* *i >= 0x20 */ res__lit(i+1, (unsigned int)i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
  } else { /* more ifs for cases that expand large, since overhead is amortized */
    if (*i >= 0x18)       res__match(res__dout-(unsigned int)(res__in3(0) - 0x180000 + 1), (unsigned int)i[3]+1), i += 4;
    else if (*i >= 0x10)  res__match(res__dout-(unsigned int)(res__in3(0) - 0x100000 + 1), (unsigned int)res__in2(3)+1), i += 5;
    else if (*i >= 0x08)  res__lit(i+2, (unsigned int)res__in2(0) - 0x0800 + 1), i += 2 + (res__in2(0) - 0x0800 + 1);
    else if (*i == 0x07)  res__lit(i+3, (unsigned int)res__in2(1) + 1), i += 3 + (res__in2(1) + 1);
    else if (*i == 0x06)  res__match(res__dout-(unsigned int)(res__in3(1)+1), i[4]+1u), i += 5;
    else if (*i == 0x04)  res__match(res__dout-(unsigned int)(res__in3(1)+1), (unsigned int)res__in2(4)+1u), i += 6;
  }
  return i;
}
static unsigned
res__adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen) {
  const unsigned long ADLER_MOD = 65521;
  unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
  unsigned long i, blocklen = buflen % 5552;
  while (buflen) {
    for (i=0; i + 7 < blocklen; i += 8) {
      s1 += buffer[0]; s2 += s1;
      s1 += buffer[1]; s2 += s1;
      s1 += buffer[2]; s2 += s1;
      s1 += buffer[3]; s2 += s1;
      s1 += buffer[4]; s2 += s1;
      s1 += buffer[5]; s2 += s1;
      s1 += buffer[6]; s2 += s1;
      s1 += buffer[7]; s2 += s1;
      buffer += 8;
    }
    for (; i < blocklen; ++i) {
      s1 += *buffer++; s2 += s1;
    }
    s1 %= ADLER_MOD; s2 %= ADLER_MOD;
    buflen -= (unsigned int)blocklen;
    blocklen = 5552;
  }
  return (unsigned int)(s2 << 16) + (unsigned int)s1;
}
static unsigned
res__decompress(unsigned char *output, unsigned char *i, unsigned int length) {
  unsigned int olen;
  if (res__in4(0) != 0x57bC0000) return 0;
  if (res__in4(4) != 0)          return 0; /* error! stream is > 4GB */
  olen = res__decompress_len(i);

  res__barrier2 = i;
  res__barrier3 = i+length;
  res__barrier = output + olen;
  res__barrier4 = output;
  i += 16;

  res__dout = output;
  for (;;) {
    unsigned char *old_i = i;
    i = res__decompress_token(i);
    if (i == old_i) {
      if (*i == 0x05 && i[1] == 0xfa) {
        assert(res__dout == output + olen);
        if (res__dout != output + olen) return 0;
        if (res__adler32(1, output, olen) != (unsigned) res__in4(2))
          return 0;
        return olen;
      } else {
        assert(0); /* NOTREACHED */
        return 0;
      }
    }
    assert(res__dout <= output + olen);
    if (res__dout > output + olen)
      return 0;
  }
}
static unsigned
res__decode_85_byte(char c) {
  return (unsigned int)((c >= '\\') ? c-36 : c-35);
}
static void
res__decode_85(unsigned char* dst, const unsigned char* src) {
  while (*src) {
    unsigned int tmp =
         1 * res__decode_85_byte((char)src[0]) +
        85 * (res__decode_85_byte((char)src[1]) +
        85 * (res__decode_85_byte((char)src[2]) +
        85 * (res__decode_85_byte((char)src[3]) +
        85 * res__decode_85_byte((char)src[4]))));

    /* we can't assume little-endianess. */
    dst[0] = (unsigned char)((tmp >> 0) & 0xFF);
    dst[1] = (unsigned char)((tmp >> 8) & 0xFF);
    dst[2] = (unsigned char)((tmp >> 16) & 0xFF);
    dst[3] = (unsigned char)((tmp >> 24) & 0xFF);

    src += 5;
    dst += 4;
  }
}
/* clang-format on */

static void *
res_unpack(int *data_siz, const char *src, struct sys *sys,
           struct arena *a, struct arena *tmp) {
  unsigned char *data = 0;
  {
    const int com_size = (((int)strlen(src) + 4) / 5) * 4;
    unsigned char *com_buf = arena_alloc(tmp, sys, com_size);
    res__decode_85(com_buf, cast(const unsigned char *, src));
    {
      unsigned un_siz = res__decompress_len(com_buf);
      data = arena_alloc(a, sys, cast(int, un_siz));
      res__decompress(data, com_buf, un_siz);
      *data_siz = cast(int, un_siz);
    }
  }
  return data;
}
static void *
res_default_fnt(int *data_siz, struct sys *sys, struct arena *a, struct arena *tmp) {
  return res_unpack(data_siz, res__default_fnt, sys, a, tmp);
}
static void *
res_icon_fnt(int *data_siz, struct sys *sys, struct arena *a, struct arena *tmp) {
  return res_unpack(data_siz, res__icon_fnt, sys, a, tmp);
}

/* ---------------------------------------------------------------------------
 *                                Cache
 * ---------------------------------------------------------------------------
 */
static int*
res__run_cache_slot(struct res_run_cache *c, hkey h) {
  int hidx = hkey32(h);
  int slot = (hidx & c->hmsk);
  assert(slot < c->hcnt);
  return &c->htbl[slot];
}
static struct res_fnt_run*
res__run_cache_get(struct res_run_cache *c, int i) {
  assert(i < c->run_cnt);
  return &c->runs[i];
}
static struct res_fnt_run*
res__run_cache_sen(struct res_run_cache *c) {
  return c->runs;
}
#ifdef DEBUG_MODE
static void
res__run_cache_val_lru(struct res_run_cache *c, int expct_cnt_chng) {
  int i, run_cnt = 0;
  struct res_fnt_run *sen = res__run_cache_sen(c);
  int last_ordering = sen->ordering;
  for (i = sen->lru_nxt; i != 0; ) {
    struct res_fnt_run *run = res__run_cache_get(c, i);
    assert(run->ordering < last_ordering);
    last_ordering = run->ordering;
    i = run->lru_nxt;
    run_cnt++;
  }
  if((c->last_lru_cnt + expct_cnt_chng) != run_cnt) {
    assert(0);
  }
  c->last_lru_cnt = run_cnt;
}
#else
#define res__run_cache_val_lru(...)
#endif
static void
res__run_cache_recycle_lru(struct res_run_cache *c) {
  struct res_fnt_run *sen = res__run_cache_sen(c);
  assert(sen->lru_prv);

  int idx = sen->lru_prv;
  struct res_fnt_run *run = res__run_cache_get(c, idx);
  struct res_fnt_run *prv = res__run_cache_get(c, run->lru_prv);
  prv->lru_nxt = 0;
  sen->lru_prv = run->lru_prv;
  res__run_cache_val_lru(c, -1);

  /* find location of entry in hash chain */
  int *nxt_idx = res__run_cache_slot(c, run->hash);
  while (*nxt_idx != idx) {
    assert(*nxt_idx);
    struct res_fnt_run *nxt_run = res__run_cache_get(c, *nxt_idx);
    nxt_idx = &nxt_run->nxt;
  }
  /* remove lru run from hash chain and place into free chain */
  assert(*nxt_idx == idx);
  *nxt_idx = run->nxt;
  run->nxt = sen->nxt;
  sen->nxt = idx;
  c->stats.recycle_cnt++;
}
static int
res__run_cache_free_entry(struct res_run_cache *c) {
  struct res_fnt_run *sen = res__run_cache_sen(c);
  if (!sen->nxt) {
    res__run_cache_recycle_lru(c);
  }
  int ret = sen->nxt;
  assert(ret);

  struct res_fnt_run *run = res__run_cache_get(c, ret);
  sen->nxt = run->nxt;
  run->nxt = 0;

  assert(run);
  assert(run != sen);
  assert(run->nxt == 0);
  return ret;
}
struct res_run_cache_tbl_fnd_res {
  struct res_fnt_run *run;
  int *slot;
  int idx;
};
static struct res_run_cache_tbl_fnd_res
res__run_cache_tbl_fnd(struct res_run_cache *c, hkey hash) {
  struct res_run_cache_tbl_fnd_res ret = {0};
  ret.slot = res__run_cache_slot(c, hash);
  ret.idx = *ret.slot;
  while (ret.idx) {
    struct res_fnt_run *it = res__run_cache_get(c, ret.idx);
    if (hkey_eq(it->hash, hash)) {
      ret.run = it;
      break;
    }
    ret.idx = it->nxt;
  }
  return ret;
}
struct res_run_cache_fnd_res {
  int is_new;
  struct res_fnt_run *run;
};
static struct res_run_cache_fnd_res
res_run_cache_fnd(struct res_run_cache *c, hkey h) {
  struct res_run_cache_fnd_res ret = {0};
  struct res_run_cache_tbl_fnd_res fnd = res__run_cache_tbl_fnd(c, h);
  if (fnd.run) {
    struct res_fnt_run *prv = res__run_cache_get(c, fnd.run->lru_prv);
    struct res_fnt_run *nxt = res__run_cache_get(c, fnd.run->lru_nxt);
    prv->lru_nxt = fnd.run->lru_nxt;
    nxt->lru_prv = fnd.run->lru_prv;
    res__run_cache_val_lru(c, -1);
    c->stats.hit_cnt++;
  } else {
    fnd.idx = res__run_cache_free_entry(c);
    assert(fnd.idx);
    fnd.run = res__run_cache_get(c, fnd.idx);
    fnd.run->nxt = *fnd.slot;
    fnd.run->hash = h;
    *fnd.slot = fnd.idx;
    c->stats.miss_cnt++;
    ret.is_new = 1;
  }
  struct res_fnt_run *sen = res__run_cache_sen(c);
  assert(fnd.run != sen);
  fnd.run->lru_nxt = sen->lru_nxt;
  fnd.run->lru_prv = 0;

  struct res_fnt_run *lru_nxt = res__run_cache_get(c, sen->lru_nxt);
  lru_nxt->lru_prv = fnd.idx;
  sen->lru_nxt = fnd.idx;
#ifdef DEBUG_MODE
  fnd.run->ordering = sen->ordering++;
  res__run_cache_val_lru(c, 1);
#endif
  ret.run = fnd.run;
  return ret;
}
static void
res_run_cache_init(struct res_run_cache *c, struct sys *sys,
                   const struct res_args *args) {
  assert(ispow2(args->hash_cnt));
  c->hcnt = args->hash_cnt;
  c->hmsk = c->hcnt - 1;
  c->run_cnt = args->run_cnt;
  c->htbl = arena_arr(sys->mem.arena, sys, int, c->hcnt);
  c->runs = arena_arr(sys->mem.arena, sys, struct res_fnt_run, c->run_cnt);
  for_cnt(i, args->run_cnt) {
    struct res_fnt_run *run = res__run_cache_get(c, i);
    run->nxt = ((i + 1) < args->run_cnt) ? run->nxt = i + 1 : 0;
  }
}

/* ---------------------------------------------------------------------------
 *                                  Font
 * ---------------------------------------------------------------------------
 */
static struct res_glyph_set *
res__load_glyphset(struct res *r, struct res_fnt *fnt, struct arena *a, int idx) {
  struct sys *sys = r->sys;

  /* init image */
  int w = 256;
  int h = 256;
  struct res_glyph_set *set = arena_alloc(a, sys, szof(struct res_glyph_set));

retry:;
  /* load glyphs */
  struct mem_scp scp;
  mem_scp_begin(&scp, a);

  set->img = img_new(a, sys, w, h);
  float s = fnt_scale_for_mapping_em_to_pixels(&fnt->stbfont, 1) /
            fnt_scale_for_pixel_height(&fnt->stbfont, 1);
  int ret = fnt_bake_bitmap(fnt->data, 0, fnt->size * s, (void *)set->img,
                            w, h, idx * 256, 256, set->glyphs);

  /* retry with a larger image buffer if the buffer wasn't large enough */
  if (ret < 0) {
    w *= 2, h *= 2;
    mem_scp_end(&scp, a, sys);
    goto retry;
  }
  /* adjust glyph yoffsets and xadvance */
  int ascent, descent, linegap;
  fnt_get_vmetrics(&fnt->stbfont, &ascent, &descent, &linegap);
  float scale = fnt_scale_for_mapping_em_to_pixels(&fnt->stbfont, fnt->size);
  int scaled_ascent = cast(int, cast(float, ascent) * scale + 0.5f);
  for (int i = 0; i < 256; i++) {
    set->glyphs[i].yoff += cast(float, scaled_ascent);
    set->glyphs[i].xadvance = cast(float, math_roundi(set->glyphs[i].xadvance));
  }
  /* convert 8bit data to 32bit */
  for (int i = w * h - 1; i >= 0; i--) {
    unsigned char n = *((unsigned char *)set->img + i);
    set->img[i] = col_rgba(0xFF, 0xFF, 0xFF, n);
  }
  set->texid = sys->ren.tex.mk(sys->renderer, set->img, w, h);
  return set;
}
static struct res_glyph_set *
res_fnt_get_glyphset(struct res *r, struct res_fnt *fnt, int codepoint) {
  struct sys *sys = r->sys;
  int idx = (codepoint >> 8) % RES_MAX_GLYPHSET;
  if (!fnt->sets[idx]) {
    fnt->sets[idx] = res__load_glyphset(r, fnt, sys->mem.arena, idx);
  }
  return fnt->sets[idx];
}
static struct res_fnt *
res_fnt_new(struct res *r, struct arena *a, void *data, float pntsiz) {
  struct sys *sys = r->sys;

  /* init font */
  struct mem_scp scp;
  mem_scp_begin(&scp, a);

  struct res_fnt *fnt = arena_alloc(a, sys, szof(struct res_fnt));
  fnt->size = pntsiz;
  fnt->data = data;

  /* init stbfont */
  const int ok = fnt_init(&fnt->stbfont, fnt->data, 0);
  if (!ok) {
    goto fail;
  }
  /* get height and scale */
  int ascent, descent, linegap;
  fnt_get_vmetrics(&fnt->stbfont, &ascent, &descent, &linegap);
  fnt->scale = fnt_scale_for_mapping_em_to_pixels(&fnt->stbfont, pntsiz);

  float total_h = cast(float, ascent - descent + linegap);
  fnt->height = math_ceili(total_h * fnt->scale);
  return fnt;

fail:
  mem_scp_end(&scp, a, sys);
  return 0;
}
static void
res_fnt_fill_run(struct res *r, struct res_fnt_run *run, struct str txt) {
  run->len = 0;
  int n = 0, ext = 0;
  struct res_fnt *fnt = r->fnt;

  unsigned rune = 0;
  for_utf(&rune, it, rest, txt) {
    assert(run->len < RES_FNT_MAX_RUN);
    struct res_glyph_set *set = res_fnt_get_glyphset(r, fnt, cast(int, rune));
    struct fnt_baked_char *g = &set->glyphs[rune & 0xFF];
    n += it.len;

    assert(g->x1 >= g->x0 && g->x1 - g->x0 < UCHAR_MAX);
    assert(g->y1 >= g->y0 && g->y1 - g->y0 < UCHAR_MAX);
    assert(g->xoff >= SCHAR_MIN && g->xoff <= SCHAR_MAX);
    assert(g->yoff >= SCHAR_MIN && g->yoff <= SCHAR_MAX);

    run->off[run->len] = cast(unsigned char, n);
    ext += math_ceili(g->xadvance);
    run->ext[run->len * 2 + 0] = cast(unsigned char, g->x1 - g->x0);
    run->ext[run->len * 2 + 1] = cast(unsigned char, g->y1 - g->y0);
    run->coord[run->len * 2 + 0] = g->x0;
    run->coord[run->len * 2 + 1] = g->y0;
    run->pad[run->len * 2 + 0] = cast(signed char, math_roundi(g->xoff));
    run->pad[run->len * 2 + 1] = cast(signed char, math_roundi(g->yoff));
    run->tex_id[run->len] = set->texid;

    if (rest.len) {
      unsigned nxt = utf_get(rest);
      int k = fnt_get_codepoint_kern_advance(&fnt->stbfont,
        cast(int, rune), cast(int, nxt));
      ext += math_ceili(fnt->scale * cast(float, k));
    }
    run->adv[run->len++] = cast(unsigned short, ext);
    if (run->len >= RES_FNT_MAX_RUN) {
      break;
    }
  }
}
static void
res_fnt_ext(int *ext, struct res *r, struct str txt) {
  assert(ext);

  ext[0] = 0;
  ext[1] = r->fnt->height;
  if (!txt.len) {
    return;
  }
  hkey h = hkey_init;
  int n = div_round_up(txt.len, 16);
  for_cnt(i,n) {
    struct str seg = str_lhs(txt, 16);
    h = cpu_hash(seg.str, seg.len, h);

    struct res_run_cache_fnd_res ret = res_run_cache_fnd(&r->run_cache, h);
    struct res_fnt_run *run = ret.run;
    if (ret.is_new) {
      res_fnt_fill_run(r, run, txt);
    }
    ext[0] += run->adv[run->len-1];
    txt = str_cut_lhs(&txt, run->off[run->len-1]);
  }
}
static void
res_fnt_fit_run(struct res_txt_bnd *bnd, struct res_fnt_run *run, int space,
                int ext) {
  int width = 0, len = 0;
  assert(run->len <= RES_FNT_MAX_RUN);
  for_cnt(i, run->len) {
    assert(i < RES_FNT_MAX_RUN);
    if (ext + run->adv[i] > space){
      break;
    } else {
      len = run->off[i];
      width = run->adv[i];
    }
  }
  bnd->len += len;
  bnd->width += width;
}
static void
res_fnt_fit(struct res_txt_bnd *bnd, struct res *r, int space, struct str txt) {
  assert(r);
  assert(bnd);
  memset(bnd, 0, sizeof(*bnd));
  bnd->end = txt.end;
  if (!space) {
    return;
  }
  int ext = 0;
  hkey h = hkey_init;
  int n = div_round_up(txt.len, 16);
  for_cnt(i,n) {
    struct str seg = str_lhs(txt, 16);
    h = cpu_hash(seg.str, seg.len, h);

    struct res_run_cache_fnd_res ret = res_run_cache_fnd(&r->run_cache, h);
    struct res_fnt_run *run = ret.run;
    if (ret.is_new) {
      res_fnt_fill_run(r, run, txt);
    }
    if (ext + run->adv[run->len-1] < space) {
      bnd->len += run->off[run->len-1];
      bnd->width += run->adv[run->len-1];
      ext += run->adv[run->len-1];
    } else {
      if (ext + run->adv[0] < space) {
        res_fnt_fit_run(bnd, run, space, ext);
      }
      break;
    }
    txt = str_cut_lhs(&txt, run->off[run->len-1]);
  }
  bnd->end = txt.str + bnd->len;
}
static struct fnt_baked_char *
res__glyph(struct ren_cmd_buf *buf, struct res *r, struct res_fnt *fnt,
           int x, int y, int rune) {
  struct res_glyph_set *set = res_fnt_get_glyphset(r, fnt, rune);
  struct fnt_baked_char *g = &set->glyphs[rune & 0xFF];
  struct sys *sys = r->sys;

  int sx = g->x0;
  int sy = g->y0;
  int w = g->x1 - g->x0;
  int h = g->y1 - g->y0;

  int at_x = x + math_roundi(g->xoff);
  int at_y = y + math_roundi(g->yoff);
  sys->ren.img(buf, at_x, at_y, sx, sy, w, h, set->texid);
  return g;
}
static int
res__ren_run(struct sys *sys, struct ren_cmd_buf *buf, struct res_fnt_run *run,
             int dx, int y) {
  int x = dx;
  for_cnt(i, run->len) {
    int sx = run->coord[i * 2 + 0];
    int sy = run->coord[i * 2 + 1];
    int w = run->ext[i * 2 + 0];
    int h = run->ext[i * 2 + 1];
    int at_x = x + run->pad[i * 2 + 0];
    int at_y = y + run->pad[i * 2 + 1];
    sys->ren.img(buf, at_x, at_y, sx, sy, w, h, run->tex_id[i]);
    x = dx + run->adv[i];
  }
  return dx + run->adv[run->len-1];
}
static void
ren_print(struct ren_cmd_buf *buf, struct res *r, int x, int y, struct str txt) {
  hkey h = hkey_init;
  int n = div_round_up(txt.len, 16);
  for_cnt(i,n) {
    struct str seg = str_lhs(txt, 16);
    h = cpu_hash(seg.str, seg.len, h);

    struct res_run_cache_fnd_res ret = res_run_cache_fnd(&r->run_cache, h);
    struct res_fnt_run *run = ret.run;
    if (ret.is_new) {
      res_fnt_fill_run(r, run, txt);
    }
    x = res__ren_run(r->sys, buf, run, x, y);
    txt = str_cut_lhs(&txt, run->off[run->len-1]);
  }
}
static void
ren_ico_siz(int *siz, struct res *r, const char *ico) {
  siz[0] = siz[1] = 0;
  unsigned rune = 0;
  struct str utf8 = str0(ico);
  utf_dec(&rune, &utf8);
  if (rune != UTF_INVALID) {
    struct res_glyph_set *set = res_fnt_get_glyphset(r, r->ico, cast(int, rune));
    struct fnt_baked_char *g = &set->glyphs[rune & 0xFF];
    siz[0] = g->x1 - g->x0;
    siz[1] = g->y1 - g->y0;
  }
}
static void
ren_ico(struct ren_cmd_buf *buf, struct res *r, int x, int y, const char *ico) {
  unsigned rune = 0;
  struct str utf8 = str0(ico);
  utf_dec(&rune, &utf8);
  if (rune != UTF_INVALID) {
    res__glyph(buf, r, r->ico, x, y, cast(int, rune));
  }
}
static void
res_init(struct res *r, const struct res_args *args) {
  struct sys *sys = r->sys;
  static const float fnt_pnt_siz[] = {8.0f, 10.0f, 12.0f, 14.0f, 16.0f, 20.0f};
  float pnt_siz = math_floori(sys->fnt_pnt_size * sys->ui_scale);

  double best_d = 10000.0;
  r->fnt_pnt_size = 16.0f;
  fori_arrv(i, fnt_pnt_siz) {
    double d = math_abs(pnt_siz - fnt_pnt_siz[i]);
    if (d < best_d) {
      r->fnt_pnt_size = fnt_pnt_siz[i];
      best_d = d;
    }
  }
  scp_mem(sys->mem.tmp, sys) {
    int fnt_siz = 0;
    void *mem = res_default_fnt(&fnt_siz, sys, sys->mem.arena, sys->mem.tmp);
    r->fnt = res_fnt_new(r, sys->mem.arena, mem, r->fnt_pnt_size);
    assert(r->fnt);
  }
  int ico_siz = 0;
  scp_mem(sys->mem.tmp, sys) {
    void *mem = res_icon_fnt(&ico_siz, sys, sys->mem.arena, sys->mem.tmp);
    r->ico = res_fnt_new(r, sys->mem.arena, mem, 16.0f);
    assert(r->ico);
  }
  res_run_cache_init(&r->run_cache, sys, args);
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
extern void dlExport(void *export, void *import);
static const struct res_api res_api = {
  .version = RES_VERSION,
  .init = res_init,
  .ico_siz = ren_ico_siz,
  .ico = ren_ico,
  .print = ren_print,
  .fnt = {
    .ext = res_fnt_ext,
    .fit = res_fnt_fit,
  }
};
static void
res_get_api(void *export, void *import) {
  unused(import);
  struct res_api *r = (struct res_api*)export;
  *r = res_api;
}

