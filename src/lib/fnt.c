#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wimplicit-int-float-conversion"
#endif

#ifndef FNT_MAX_OVERSAMPLE
#define FNT_MAX_OVERSAMPLE 8
#endif

#if FNT_MAX_OVERSAMPLE > 255
#error "FNT_MAX_OVERSAMPLE cannot be > 255"
#endif

typedef int fnt__test_oversample_pow2
    [(FNT_MAX_OVERSAMPLE & (FNT_MAX_OVERSAMPLE - 1)) == 0 ? 1 : -1];

#ifndef FNT_RASTERIZER_VERSION
#define FNT_RASTERIZER_VERSION 2
#endif

#ifdef _MSC_VER
#define FNT__NOTUSED(v) (void)(v)
#else
#define FNT__NOTUSED(v) (void)sizeof(v)
#endif

//////////////////////////////////////////////////////////////////////////
//
// fnt__buf helpers to parse data from file
//

static fnt_uint8
fnt__buf_get8(struct fnt__buf *b) {
  if (b->cursor >= b->size) {
    return 0;
  }
  return b->data[b->cursor++];
}
static fnt_uint8
fnt__buf_peek8(struct fnt__buf *b) {
  if (b->cursor >= b->size) {
    return 0;
  }
  return b->data[b->cursor];
}
static void
fnt__buf_seek(struct fnt__buf *b, int o) {
  FNT_assert(!(o > b->size || o < 0));
  b->cursor = (o > b->size || o < 0) ? b->size : o;
}
static void
fnt__buf_skip(struct fnt__buf *b, int o) {
  fnt__buf_seek(b, b->cursor + o);
}
static fnt_uint32
fnt__buf_get(struct fnt__buf *b, int n) {
  fnt_uint32 v = 0;
  FNT_assert(n >= 1 && n <= 4);
  for (int i = 0; i < n; i++) {
    v = (v << 8) | fnt__buf_get8(b);
  }
  return v;
}
static struct fnt__buf
fnt__new_buf(const void *p, size_t size) {
  struct fnt__buf r;
  FNT_assert(size < 0x40000000);
  r.data = (fnt_uint8 *)p;
  r.size = (int)size;
  r.cursor = 0;
  return r;
}
#define fnt__buf_get16(b) fnt__buf_get((b), 2)
#define fnt__buf_get32(b) fnt__buf_get((b), 4)

static struct fnt__buf
fnt__buf_range(const struct fnt__buf *b, int o, int s) {
  struct fnt__buf r = fnt__new_buf(NULL, 0);
  if (o < 0 || s < 0 || o > b->size || s > b->size - o) {
    return r;
  }
  r.data = b->data + o;
  r.size = s;
  return r;
}
static struct fnt__buf
fnt__cff_get_index(struct fnt__buf *b) {
  int count, start, offsize;
  start = b->cursor;
  count = fnt__buf_get16(b);
  if (count) {
    offsize = fnt__buf_get8(b);
    FNT_assert(offsize >= 1 && offsize <= 4);
    fnt__buf_skip(b, offsize * count);
    fnt__buf_skip(b, fnt__buf_get(b, offsize) - 1);
  }
  return fnt__buf_range(b, start, b->cursor - start);
}
static fnt_uint32
fnt__cff_int(struct fnt__buf *b) {
  int b0 = fnt__buf_get8(b);
  if (b0 >= 32 && b0 <= 246) {
    return b0 - 139;
  } else if (b0 >= 247 && b0 <= 250) {
    return (b0 - 247) * 256 + fnt__buf_get8(b) + 108;
  } else if (b0 >= 251 && b0 <= 254) {
    return -(b0 - 251) * 256 - fnt__buf_get8(b) - 108;
  } else if (b0 == 28) {
    return fnt__buf_get16(b);
  } else if (b0 == 29) {
    return fnt__buf_get32(b);
  }
  FNT_assert(0);
  return 0;
}
static void
fnt__cff_skip_operand(struct fnt__buf *b) {
  int v, b0 = fnt__buf_peek8(b);
  FNT_assert(b0 >= 28);
  if (b0 == 30) {
    fnt__buf_skip(b, 1);
    while (b->cursor < b->size) {
      v = fnt__buf_get8(b);
      if ((v & 0xF) == 0xF || (v >> 4) == 0xF) break;
    }
  } else {
    fnt__cff_int(b);
  }
}
static struct fnt__buf
fnt__dict_get(struct fnt__buf *b, int key) {
  fnt__buf_seek(b, 0);
  while (b->cursor < b->size) {
    int start = b->cursor, end, op;
    while (fnt__buf_peek8(b) >= 28) {
      fnt__cff_skip_operand(b);
    }
    end = b->cursor;
    op = fnt__buf_get8(b);
    if (op == 12) {
      op = fnt__buf_get8(b) | 0x100;
    }
    if (op == key) {
      return fnt__buf_range(b, start, end - start);
    }
  }
  return fnt__buf_range(b, 0, 0);
}
static void
fnt__dict_get_ints(struct fnt__buf *b, int key, int outcount, fnt_uint32 *out) {
  int i;
  struct fnt__buf operands = fnt__dict_get(b, key);
  for (i = 0; i < outcount && operands.cursor < operands.size; i++) {
    out[i] = fnt__cff_int(&operands);
  }
}
static int
fnt__cff_index_count(struct fnt__buf *b) {
  fnt__buf_seek(b, 0);
  return fnt__buf_get16(b);
}
static struct fnt__buf
fnt__cff_index_get(struct fnt__buf b, int i) {
  int count, offsize, start, end;
  fnt__buf_seek(&b, 0);
  count = fnt__buf_get16(&b);
  offsize = fnt__buf_get8(&b);
  FNT_assert(i >= 0 && i < count);
  FNT_assert(offsize >= 1 && offsize <= 4);
  fnt__buf_skip(&b, i * offsize);
  start = fnt__buf_get(&b, offsize);
  end = fnt__buf_get(&b, offsize);
  return fnt__buf_range(&b, 2 + (count + 1) * offsize + start, end - start);
}

//////////////////////////////////////////////////////////////////////////
//
// accessors to parse data from file
//

// on platforms that don't allow misaligned reads, if we want to allow
// truetype fonts that aren't padded to alignment, define
// ALLOW_UNALIGNED_TRUETYPE

#define ttBYTE(p) (*(fnt_uint8 *)(p))
#define ttCHAR(p) (*(fnt_int8 *)(p))
#define ttFixed(p) ttLONG(p)

static fnt_uint16
ttUSHORT(fnt_uint8 *p) {
  return p[0] * 256 + p[1];
}
static fnt_int16
ttSHORT(fnt_uint8 *p) {
  return p[0] * 256 + p[1];
}
static fnt_uint32
ttULONG(fnt_uint8 *p) {
  return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
}
#if 0
static fnt_int32
ttLONG(fnt_uint8 *p) {
  return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
}
#endif

#define fnt_tag4(p, c0, c1, c2, c3) \
  ((p)[0] == (c0) && (p)[1] == (c1) && (p)[2] == (c2) && (p)[3] == (c3))
#define fnt_tag(p, str) fnt_tag4(p, str[0], str[1], str[2], str[3])

#if 0
static int
fnt__isfont(fnt_uint8 *font) {
  // check the version number
  if (fnt_tag4(font, '1', 0, 0, 0)) return 1;  // TrueType 1
  if (fnt_tag(font, "typ1"))
    return 1;  // TrueType with type 1 font -- we don't support this!
  if (fnt_tag(font, "OTTO")) return 1;       // OpenType with CFF
  if (fnt_tag4(font, 0, 1, 0, 0)) return 1;  // OpenType 1.0
  if (fnt_tag(font, "true"))
    return 1;  // Apple specification for TrueType fonts
  return 0;
}
#endif

static fnt_uint32
fnt__find_table(fnt_uint8 *data, fnt_uint32 fontstart, const char *tag) {
  // @OPTIMIZE: binary search
  fnt_int32 i;
  fnt_int32 num_tables = ttUSHORT(data + fontstart + 4);
  fnt_uint32 tabledir = fontstart + 12;
  for (i = 0; i < num_tables; ++i) {
    fnt_uint32 loc = tabledir + 16 * i;
    if (fnt_tag(data + loc + 0, tag)) {
      return ttULONG(data + loc + 8);
    }
  }
  return 0;
}
#if 0
static int
fnt_get_font_offset_for_index_internal(unsigned char *font_collection,
                                       int index) {
  // if it's just a font, there's only one valid index
  if (fnt__isfont(font_collection)) {
    return index == 0 ? 0 : -1;
  }
  // check if it's a TTC
  if (fnt_tag(font_collection, "ttcf")) {
    // version 1?
    if (ttULONG(font_collection + 4) == 0x00010000 ||
        ttULONG(font_collection + 4) == 0x00020000) {
      fnt_int32 n = ttLONG(font_collection + 8);
      if (index >= n) {
        return -1;
      }
      return ttULONG(font_collection + 12 + index * 4);
    }
  }
  return -1;
}
static int
fnt_get_number_of_fonts_internal(unsigned char *font_collection) {
  // if it's just a font, there's only one valid font
  if (fnt__isfont(font_collection)) {
    return 1;
  }
  // check if it's a TTC
  if (fnt_tag(font_collection, "ttcf")) {
    // version 1?
    if (ttULONG(font_collection + 4) == 0x00010000 ||
        ttULONG(font_collection + 4) == 0x00020000) {
      return ttLONG(font_collection + 8);
    }
  }
  return 0;
}
#endif

static struct fnt__buf
fnt__get_subrs(struct fnt__buf cff, struct fnt__buf fontdict) {
  fnt_uint32 subrsoff = 0, private_loc[2] = {0, 0};
  struct fnt__buf pdict;
  fnt__dict_get_ints(&fontdict, 18, 2, private_loc);
  if (!private_loc[1] || !private_loc[0]) return fnt__new_buf(NULL, 0);
  pdict = fnt__buf_range(&cff, private_loc[1], private_loc[0]);
  fnt__dict_get_ints(&pdict, 19, 1, &subrsoff);
  if (!subrsoff) return fnt__new_buf(NULL, 0);
  fnt__buf_seek(&cff, private_loc[1] + subrsoff);
  return fnt__cff_get_index(&cff);
}
static int
fnt_init_internal(struct fnt_info *info, unsigned char *data, int fontstart) {
  fnt_uint32 cmap, t;
  fnt_int32 i, numTables;

  info->data = data;
  info->fontstart = fontstart;
  info->cff = fnt__new_buf(NULL, 0);

  cmap = fnt__find_table(data, fontstart, "cmap");        // required
  info->loca = fnt__find_table(data, fontstart, "loca");  // required
  info->head = fnt__find_table(data, fontstart, "head");  // required
  info->glyf = fnt__find_table(data, fontstart, "glyf");  // required
  info->hhea = fnt__find_table(data, fontstart, "hhea");  // required
  info->hmtx = fnt__find_table(data, fontstart, "hmtx");  // required
  info->kern = fnt__find_table(data, fontstart, "kern");  // not required
  info->gpos = fnt__find_table(data, fontstart, "GPOS");  // not required
  if (!cmap || !info->head || !info->hhea || !info->hmtx) {
    return 0;
  }
  if (info->glyf) {
    // required for truetype
    if (!info->loca) return 0;
  } else {
    // initialization for CFF / Type2 fonts (OTF)
    struct fnt__buf b, topdict, topdictidx;
    fnt_uint32 cstype = 2, charstrings = 0, fdarrayoff = 0, fdselectoff = 0;
    fnt_uint32 cff;

    cff = fnt__find_table(data, fontstart, "CFF ");
    if (!cff) return 0;

    info->fontdicts = fnt__new_buf(NULL, 0);
    info->fdselect = fnt__new_buf(NULL, 0);

    // @TODO this should use size from table (not 512MB)
    info->cff = fnt__new_buf(data + cff, 512 * 1024 * 1024);
    b = info->cff;

    // read the header
    fnt__buf_skip(&b, 2);
    fnt__buf_seek(&b, fnt__buf_get8(&b));  // hdrsize

    // @TODO the name INDEX could list multiple fonts,
    // but we just use the first one.
    fnt__cff_get_index(&b);  // name INDEX
    topdictidx = fnt__cff_get_index(&b);
    topdict = fnt__cff_index_get(topdictidx, 0);
    fnt__cff_get_index(&b);  // string INDEX
    info->gsubrs = fnt__cff_get_index(&b);

    fnt__dict_get_ints(&topdict, 17, 1, &charstrings);
    fnt__dict_get_ints(&topdict, 0x100 | 6, 1, &cstype);
    fnt__dict_get_ints(&topdict, 0x100 | 36, 1, &fdarrayoff);
    fnt__dict_get_ints(&topdict, 0x100 | 37, 1, &fdselectoff);
    info->subrs = fnt__get_subrs(b, topdict);

    // we only support Type 2 charstrings
    if (cstype != 2) return 0;
    if (charstrings == 0) return 0;

    if (fdarrayoff) {
      // looks like a CID font
      if (!fdselectoff) return 0;
      fnt__buf_seek(&b, fdarrayoff);
      info->fontdicts = fnt__cff_get_index(&b);
      info->fdselect = fnt__buf_range(&b, fdselectoff, b.size - fdselectoff);
    }

    fnt__buf_seek(&b, charstrings);
    info->charstrings = fnt__cff_get_index(&b);
  }
  t = fnt__find_table(data, fontstart, "maxp");
  if (t) {
    info->numGlyphs = ttUSHORT(data + t + 4);
  } else {
    info->numGlyphs = 0xffff;
  }
  // find a cmap encoding table we understand *now* to avoid searching
  // later. (todo: could make this installable)
  // the same regardless of glyph.
  numTables = ttUSHORT(data + cmap + 2);
  info->index_map = 0;
  for (i = 0; i < numTables; ++i) {
    fnt_uint32 encoding_record = cmap + 4 + 8 * i;
    // find an encoding we understand:
    switch (ttUSHORT(data + encoding_record)) {
      case FNT_PLATFORM_ID_MICROSOFT:
        switch (ttUSHORT(data + encoding_record + 2)) {
          case FNT_MS_EID_UNICODE_BMP:
          case FNT_MS_EID_UNICODE_FULL:
            // MS/Unicode
            info->index_map = cmap + ttULONG(data + encoding_record + 4);
            break;
        }
        break;
      case FNT_PLATFORM_ID_UNICODE:
        // Mac/iOS has these
        // all the encodingIDs are unicode, so we don't bother to check it
        info->index_map = cmap + ttULONG(data + encoding_record + 4);
        break;
    }
  }
  if (info->index_map == 0) return 0;

  info->indexToLocFormat = ttUSHORT(data + info->head + 50);
  return 1;
}
static int
fnt_fnd_glyph_index(const struct fnt_info *info, int unicode_codepoint) {
  fnt_uint8 *data = info->data;
  fnt_uint32 index_map = info->index_map;

  fnt_uint16 format = ttUSHORT(data + index_map + 0);
  if (format == 0) {  // apple byte encoding
    fnt_int32 bytes = ttUSHORT(data + index_map + 2);
    if (unicode_codepoint < bytes - 6) {
      return ttBYTE(data + index_map + 6 + unicode_codepoint);
    }
    return 0;
  } else if (format == 6) {
    fnt_uint32 first = ttUSHORT(data + index_map + 6);
    fnt_uint32 count = ttUSHORT(data + index_map + 8);
    if ((fnt_uint32)unicode_codepoint >= first &&
        (fnt_uint32)unicode_codepoint < first + count) {
      return ttUSHORT(data + index_map + 10 + (unicode_codepoint - first) * 2);
    }
    return 0;
  } else if (format == 2) {
    FNT_assert(0);  // @TODO: high-byte mapping for japanese/chinese/korean
    return 0;
  } else if (format == 4) {  // standard mapping for windows fonts: binary
                             // search collection of ranges
    fnt_uint16 segcount = ttUSHORT(data + index_map + 6) >> 1;
    fnt_uint16 searchRange = ttUSHORT(data + index_map + 8) >> 1;
    fnt_uint16 entrySelector = ttUSHORT(data + index_map + 10);
    fnt_uint16 rangeShift = ttUSHORT(data + index_map + 12) >> 1;

    // do a binary search of the segments
    fnt_uint32 endCount = index_map + 14;
    fnt_uint32 search = endCount;
    if (unicode_codepoint > 0xffff) {
      return 0;
    }
    // they lie from endCount .. endCount + segCount
    // but searchRange is the nearest power of two, so...
    if (unicode_codepoint >= ttUSHORT(data + search + rangeShift * 2)) {
      search += rangeShift * 2;
    }
    // now decrement to bias correctly to find smallest
    search -= 2;
    while (entrySelector) {
      fnt_uint16 end;
      searchRange >>= 1;
      end = ttUSHORT(data + search + searchRange * 2);
      if (unicode_codepoint > end) search += searchRange * 2;
      --entrySelector;
    }
    search += 2;
    {
      fnt_uint16 offset, start;
      fnt_uint16 item = (fnt_uint16)((search - endCount) >> 1);

      FNT_assert(unicode_codepoint <= ttUSHORT(data + endCount + 2 * item));
      start = ttUSHORT(data + index_map + 14 + segcount * 2 + 2 + 2 * item);
      if (unicode_codepoint < start) {
        return 0;
      }
      offset = ttUSHORT(data + index_map + 14 + segcount * 6 + 2 + 2 * item);
      if (offset == 0)
        return (fnt_uint16)(
            unicode_codepoint +
            ttSHORT(data + index_map + 14 + segcount * 4 + 2 + 2 * item));
      return ttUSHORT(data + offset + (unicode_codepoint - start) * 2 +
                      index_map + 14 + segcount * 6 + 2 + 2 * item);
    }
  } else if (format == 12 || format == 13) {
    fnt_uint32 ngroups = ttULONG(data + index_map + 12);
    fnt_int32 low, high;
    low = 0;
    high = (fnt_int32)ngroups;
    // Binary search the right group.
    while (low < high) {
      // rounds down, so low <= mid < high
      fnt_int32 mid = low + ((high - low) >> 1);
      fnt_uint32 start_char = ttULONG(data + index_map + 16 + mid * 12);
      fnt_uint32 end_char = ttULONG(data + index_map + 16 + mid * 12 + 4);
      if ((fnt_uint32)unicode_codepoint < start_char) {
        high = mid;
      } else if ((fnt_uint32)unicode_codepoint > end_char) {
        low = mid + 1;
      } else {
        fnt_uint32 start_glyph = ttULONG(data + index_map + 16 + mid * 12 + 8);
        if (format == 12) {
          return start_glyph + unicode_codepoint - start_char;
        } else {  // format == 13
          return start_glyph;
        }
      }
    }
    return 0;  // not found
  }
  // @TODO
  FNT_assert(0);
  return 0;
}
#if 0
static int
fnt_get_codepoint_shape(const struct fnt_info *info, int unicode_codepoint,
                        struct fnt_vertex **vertices) {
  return fnt_get_glyph_shape(info, fnt_fnd_glyph_index(info, unicode_codepoint),
                             vertices);
}
#endif

static void
fnt_set_vertex(struct fnt_vertex *v, fnt_uint8 type, fnt_int32 x, fnt_int32 y,
               fnt_int32 cx, fnt_int32 cy) {
  v->type = type;
  v->x = (fnt_int16)x;
  v->y = (fnt_int16)y;
  v->cx = (fnt_int16)cx;
  v->cy = (fnt_int16)cy;
}
static int
fnt__get_glyph_offset(const struct fnt_info *info, int glyph_index) {
  int g1, g2;

  FNT_assert(!info->cff.size);
  if (glyph_index >= info->numGlyphs) {
    return -1;  // glyph index out of range
  }
  if (info->indexToLocFormat >= 2) {
    return -1;  // unknown index->glyph map format
  }
  if (info->indexToLocFormat == 0) {
    g1 = info->glyf + ttUSHORT(info->data + info->loca + glyph_index * 2) * 2;
    g2 = info->glyf +
         ttUSHORT(info->data + info->loca + glyph_index * 2 + 2) * 2;
  } else {
    g1 = info->glyf + ttULONG(info->data + info->loca + glyph_index * 4);
    g2 = info->glyf + ttULONG(info->data + info->loca + glyph_index * 4 + 4);
  }
  return g1 == g2 ? -1 : g1;  // if length is 0, return -1
}
static int
fnt__get_glyph_info_t2(const struct fnt_info *info, int glyph_index, int *x0,
                       int *y0, int *x1, int *y1);

static int
fnt_get_glyph_box(const struct fnt_info *info, int glyph_index, int *x0,
                  int *y0, int *x1, int *y1) {
  if (info->cff.size) {
    fnt__get_glyph_info_t2(info, glyph_index, x0, y0, x1, y1);
  } else {
    int g = fnt__get_glyph_offset(info, glyph_index);
    if (g < 0) return 0;
    if (x0) *x0 = ttSHORT(info->data + g + 2);
    if (y0) *y0 = ttSHORT(info->data + g + 4);
    if (x1) *x1 = ttSHORT(info->data + g + 6);
    if (y1) *y1 = ttSHORT(info->data + g + 8);
  }
  return 1;
}
#if 0
static int
fnt_get_codepoint_box(const struct fnt_info *info, int codepoint, int *x0,
                      int *y0, int *x1, int *y1) {
  return fnt_get_glyph_box(info, fnt_fnd_glyph_index(info, codepoint), x0, y0,
                           x1, y1);
}
static int
fnt_is_glyph_empty(const struct fnt_info *info, int glyph_index) {
  fnt_int16 numberOfContours;
  int g;
  if (info->cff.size)
    return fnt__get_glyph_info_t2(info, glyph_index, NULL, NULL, NULL, NULL) ==
           0;
  g = fnt__get_glyph_offset(info, glyph_index);
  if (g < 0) return 1;
  numberOfContours = ttSHORT(info->data + g);
  return numberOfContours == 0;
}
#endif

static int
fnt__close_shape(struct fnt_vertex *vertices, int num_vertices, int was_off,
                 int start_off, fnt_int32 sx, fnt_int32 sy, fnt_int32 scx,
                 fnt_int32 scy, fnt_int32 cx, fnt_int32 cy) {
  if (start_off) {
    if (was_off)
      fnt_set_vertex(&vertices[num_vertices++], FNT_vcurve, (cx + scx) >> 1,
                     (cy + scy) >> 1, cx, cy);
    fnt_set_vertex(&vertices[num_vertices++], FNT_vcurve, sx, sy, scx, scy);
  } else {
    if (was_off)
      fnt_set_vertex(&vertices[num_vertices++], FNT_vcurve, sx, sy, cx, cy);
    else
      fnt_set_vertex(&vertices[num_vertices++], FNT_vline, sx, sy, 0, 0);
  }
  return num_vertices;
}
static int
fnt__get_glyph_shape_tt(const struct fnt_info *info, int glyph_index,
                        struct fnt_vertex **pvertices) {
  fnt_int16 numberOfContours;
  fnt_uint8 *endPtsOfContours;
  fnt_uint8 *data = info->data;
  struct fnt_vertex *vertices = 0;
  int num_vertices = 0;
  int g = fnt__get_glyph_offset(info, glyph_index);

  *pvertices = NULL;

  if (g < 0) return 0;

  numberOfContours = ttSHORT(data + g);

  if (numberOfContours > 0) {
    fnt_uint8 flags = 0, flagcount;
    fnt_int32 ins, i, j = 0, m, n, next_move, was_off = 0, off, start_off = 0;
    fnt_int32 x, y, cx, cy, sx, sy, scx, scy;
    fnt_uint8 *points;
    endPtsOfContours = (data + g + 10);
    ins = ttUSHORT(data + g + 10 + numberOfContours * 2);
    points = data + g + 10 + numberOfContours * 2 + 2 + ins;

    n = 1 + ttUSHORT(endPtsOfContours + numberOfContours * 2 - 2);
    m = n + 2 * numberOfContours;  // a loose bound on how many vertices we
                                   // might need
    vertices = (struct fnt_vertex *)FNT_malloc(m * sizeof(vertices[0]),
                                               info->userdata);
    if (vertices == 0) return 0;

    next_move = 0;
    flagcount = 0;

    // in first pass, we load uninterpreted data into the allocated array
    // above, shifted to the end of the array so we won't overwrite it when
    // we create our final data starting from the front

    off = m - n;  // starting offset for uninterpreted data, regardless of how m
                  // ends up being calculated
    // first load flags
    for (i = 0; i < n; ++i) {
      if (flagcount == 0) {
        flags = *points++;
        if (flags & 8) flagcount = *points++;
      } else
        --flagcount;
      vertices[off + i].type = flags;
    }
    // now load x coordinates
    x = 0;
    for (i = 0; i < n; ++i) {
      flags = vertices[off + i].type;
      if (flags & 2) {
        fnt_int16 dx = *points++;
        x += (flags & 16) ? dx : -dx;  // ???
      } else {
        if (!(flags & 16)) {
          x = x + (fnt_int16)(points[0] * 256 + points[1]);
          points += 2;
        }
      }
      vertices[off + i].x = (fnt_int16)x;
    }
    // now load y coordinates
    y = 0;
    for (i = 0; i < n; ++i) {
      flags = vertices[off + i].type;
      if (flags & 4) {
        fnt_int16 dy = *points++;
        y += (flags & 32) ? dy : -dy;  // ???
      } else {
        if (!(flags & 32)) {
          y = y + (fnt_int16)(points[0] * 256 + points[1]);
          points += 2;
        }
      }
      vertices[off + i].y = (fnt_int16)y;
    }
    // now convert them to our format
    num_vertices = 0;
    sx = sy = cx = cy = scx = scy = 0;
    for (i = 0; i < n; ++i) {
      flags = vertices[off + i].type;
      x = (fnt_int16)vertices[off + i].x;
      y = (fnt_int16)vertices[off + i].y;

      if (next_move == i) {
        if (i != 0)
          num_vertices = fnt__close_shape(vertices, num_vertices, was_off,
                                          start_off, sx, sy, scx, scy, cx, cy);
        // now start the new one
        start_off = !(flags & 1);
        if (start_off) {
          // if we start off with an off-curve point, then when we need to find
          // a point on the curve where we can start, and we need to save some
          // state for when we wraparound.
          scx = x;
          scy = y;
          if (!(vertices[off + i + 1].type & 1)) {
            // next point is also a curve point, so interpolate an on-point
            // curve
            sx = (x + (fnt_int32)vertices[off + i + 1].x) >> 1;
            sy = (y + (fnt_int32)vertices[off + i + 1].y) >> 1;
          } else {
            // otherwise just use the next point as our start point
            sx = (fnt_int32)vertices[off + i + 1].x;
            sy = (fnt_int32)vertices[off + i + 1].y;
            ++i;  // we're using point i+1 as the starting point, so skip it
          }
        } else {
          sx = x;
          sy = y;
        }
        fnt_set_vertex(&vertices[num_vertices++], FNT_vmove, sx, sy, 0, 0);
        was_off = 0;
        next_move = 1 + ttUSHORT(endPtsOfContours + j * 2);
        ++j;
      } else {
        if (!(flags & 1)) {  // if it's a curve
          if (was_off)       // two off-curve control points in a row means
                             // interpolate an on-curve midpoint
            fnt_set_vertex(&vertices[num_vertices++], FNT_vcurve, (cx + x) >> 1,
                           (cy + y) >> 1, cx, cy);
          cx = x;
          cy = y;
          was_off = 1;
        } else {
          if (was_off)
            fnt_set_vertex(&vertices[num_vertices++], FNT_vcurve, x, y, cx, cy);
          else
            fnt_set_vertex(&vertices[num_vertices++], FNT_vline, x, y, 0, 0);
          was_off = 0;
        }
      }
    }
    num_vertices = fnt__close_shape(vertices, num_vertices, was_off, start_off,
                                    sx, sy, scx, scy, cx, cy);
  } else if (numberOfContours == -1) {
    // Compound shapes.
    int more = 1;
    fnt_uint8 *comp = data + g + 10;
    num_vertices = 0;
    vertices = 0;
    while (more) {
      fnt_uint16 flags, gidx;
      int comp_num_verts = 0, i;
      struct fnt_vertex *comp_verts = 0, *tmp = 0;
      float mtx[6] = {1, 0, 0, 1, 0, 0}, m, n;

      flags = ttSHORT(comp);
      comp += 2;
      gidx = ttSHORT(comp);
      comp += 2;

      if (flags & 2) {    // XY values
        if (flags & 1) {  // shorts
          mtx[4] = ttSHORT(comp);
          comp += 2;
          mtx[5] = ttSHORT(comp);
          comp += 2;
        } else {
          mtx[4] = ttCHAR(comp);
          comp += 1;
          mtx[5] = ttCHAR(comp);
          comp += 1;
        }
      } else {
        // @TODO handle matching point
        FNT_assert(0);
      }
      if (flags & (1 << 3)) {  // WE_HAVE_A_SCALE
        mtx[0] = mtx[3] = ttSHORT(comp) / 16384.0f;
        comp += 2;
        mtx[1] = mtx[2] = 0;
      } else if (flags & (1 << 6)) {  // WE_HAVE_AN_X_AND_YSCALE
        mtx[0] = ttSHORT(comp) / 16384.0f;
        comp += 2;
        mtx[1] = mtx[2] = 0;
        mtx[3] = ttSHORT(comp) / 16384.0f;
        comp += 2;
      } else if (flags & (1 << 7)) {  // WE_HAVE_A_TWO_BY_TWO
        mtx[0] = ttSHORT(comp) / 16384.0f;
        comp += 2;
        mtx[1] = ttSHORT(comp) / 16384.0f;
        comp += 2;
        mtx[2] = ttSHORT(comp) / 16384.0f;
        comp += 2;
        mtx[3] = ttSHORT(comp) / 16384.0f;
        comp += 2;
      }
      // Find transformation scales.
      m = (float)FNT_sqrt(mtx[0] * mtx[0] + mtx[1] * mtx[1]);
      n = (float)FNT_sqrt(mtx[2] * mtx[2] + mtx[3] * mtx[3]);
      // Get indexed glyph.
      comp_num_verts = fnt_get_glyph_shape(info, gidx, &comp_verts);
      if (comp_num_verts > 0) {
        // Transform vertices.
        for (i = 0; i < comp_num_verts; ++i) {
          struct fnt_vertex *v = &comp_verts[i];
          fnt_vertex_type x, y;
          x = v->x;
          y = v->y;
          v->x = (fnt_vertex_type)(m * (mtx[0] * x + mtx[2] * y + mtx[4]));
          v->y = (fnt_vertex_type)(n * (mtx[1] * x + mtx[3] * y + mtx[5]));
          x = v->cx;
          y = v->cy;
          v->cx = (fnt_vertex_type)(m * (mtx[0] * x + mtx[2] * y + mtx[4]));
          v->cy = (fnt_vertex_type)(n * (mtx[1] * x + mtx[3] * y + mtx[5]));
        }
        // Append vertices.
        tmp = (struct fnt_vertex *)FNT_malloc(
            (num_vertices + comp_num_verts) * sizeof(struct fnt_vertex),
            info->userdata);
        if (!tmp) {
          if (vertices) FNT_free(vertices, info->userdata);
          if (comp_verts) FNT_free(comp_verts, info->userdata);
          return 0;
        }
        if (num_vertices > 0)
          FNT_memcpy(tmp, vertices, num_vertices * sizeof(struct fnt_vertex));
        FNT_memcpy(tmp + num_vertices, comp_verts,
                   comp_num_verts * sizeof(struct fnt_vertex));
        if (vertices) FNT_free(vertices, info->userdata);
        vertices = tmp;
        FNT_free(comp_verts, info->userdata);
        num_vertices += comp_num_verts;
      }
      // More components ?
      more = flags & (1 << 5);
    }
  } else if (numberOfContours < 0) {
    // @TODO other compound variations?
    FNT_assert(0);
  } else {
    // numberOfCounters == 0, do nothing
  }
  *pvertices = vertices;
  return num_vertices;
}

struct fnt__csctx {
  int bounds;
  int started;
  float first_x, first_y;
  float x, y;
  fnt_int32 min_x, max_x, min_y, max_y;
  struct fnt_vertex *pvertices;
  int num_vertices;
};

#define FNT__CSCTX_INIT(bounds) \
  { bounds, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0 }

static void
fnt__track_vertex(struct fnt__csctx *c, fnt_int32 x, fnt_int32 y) {
  if (x > c->max_x || !c->started) c->max_x = x;
  if (y > c->max_y || !c->started) c->max_y = y;
  if (x < c->min_x || !c->started) c->min_x = x;
  if (y < c->min_y || !c->started) c->min_y = y;
  c->started = 1;
}
static void
fnt__csctx_v(struct fnt__csctx *c, fnt_uint8 type, fnt_int32 x, fnt_int32 y,
             fnt_int32 cx, fnt_int32 cy, fnt_int32 cx1, fnt_int32 cy1) {
  if (c->bounds) {
    fnt__track_vertex(c, x, y);
    if (type == FNT_vcubic) {
      fnt__track_vertex(c, cx, cy);
      fnt__track_vertex(c, cx1, cy1);
    }
  } else {
    fnt_set_vertex(&c->pvertices[c->num_vertices], type, x, y, cx, cy);
    c->pvertices[c->num_vertices].cx1 = (fnt_int16)cx1;
    c->pvertices[c->num_vertices].cy1 = (fnt_int16)cy1;
  }
  c->num_vertices++;
}
static void
fnt__csctx_close_shape(struct fnt__csctx *ctx) {
  if (ctx->first_x != ctx->x || ctx->first_y != ctx->y)
    fnt__csctx_v(ctx, FNT_vline, (int)ctx->first_x, (int)ctx->first_y, 0, 0, 0,
                 0);
}
static void
fnt__csctx_rmove_to(struct fnt__csctx *ctx, float dx, float dy) {
  fnt__csctx_close_shape(ctx);
  ctx->first_x = ctx->x = ctx->x + dx;
  ctx->first_y = ctx->y = ctx->y + dy;
  fnt__csctx_v(ctx, FNT_vmove, (int)ctx->x, (int)ctx->y, 0, 0, 0, 0);
}
static void
fnt__csctx_rline_to(struct fnt__csctx *ctx, float dx, float dy) {
  ctx->x += dx;
  ctx->y += dy;
  fnt__csctx_v(ctx, FNT_vline, (int)ctx->x, (int)ctx->y, 0, 0, 0, 0);
}
static void
fnt__csctx_rccurve_to(struct fnt__csctx *ctx, float dx1, float dy1, float dx2,
                      float dy2, float dx3, float dy3) {
  float cx1 = ctx->x + dx1;
  float cy1 = ctx->y + dy1;
  float cx2 = cx1 + dx2;
  float cy2 = cy1 + dy2;
  ctx->x = cx2 + dx3;
  ctx->y = cy2 + dy3;
  fnt__csctx_v(ctx, FNT_vcubic, (int)ctx->x, (int)ctx->y, (int)cx1, (int)cy1,
               (int)cx2, (int)cy2);
}
static struct fnt__buf
fnt__get_subr(struct fnt__buf idx, int n) {
  int count = fnt__cff_index_count(&idx);
  int bias = 107;
  if (count >= 33900) {
    bias = 32768;
  } else if (count >= 1240) {
    bias = 1131;
  }
  n += bias;
  if (n < 0 || n >= count) {
    return fnt__new_buf(NULL, 0);
  }
  return fnt__cff_index_get(idx, n);
}
static struct fnt__buf
fnt__cid_get_glyph_subrs(const struct fnt_info *info, int glyph_index) {
  struct fnt__buf fdselect = info->fdselect;
  int nranges, start, end, v, fmt, fdselector = -1, i;

  fnt__buf_seek(&fdselect, 0);
  fmt = fnt__buf_get8(&fdselect);
  if (fmt == 0) {
    // untested
    fnt__buf_skip(&fdselect, glyph_index);
    fdselector = fnt__buf_get8(&fdselect);
  } else if (fmt == 3) {
    nranges = fnt__buf_get16(&fdselect);
    start = fnt__buf_get16(&fdselect);
    for (i = 0; i < nranges; i++) {
      v = fnt__buf_get8(&fdselect);
      end = fnt__buf_get16(&fdselect);
      if (glyph_index >= start && glyph_index < end) {
        fdselector = v;
        break;
      }
      start = end;
    }
  }
  if (fdselector == -1) fnt__new_buf(NULL, 0);
  return fnt__get_subrs(info->cff,
                        fnt__cff_index_get(info->fontdicts, fdselector));
}
static int
fnt__run_charstring(const struct fnt_info *info, int glyph_index,
                    struct fnt__csctx *c) {
  int in_header = 1, maskbits = 0, subr_stack_height = 0, sp = 0, v, i, b0;
  int has_subrs = 0, clear_stack;
  float s[48];
  struct fnt__buf subr_stack[10], subrs = info->subrs, b;
  float f;

#define FNT__CSERR(s) (0)

  // this currently ignores the initial width value, which isn't needed if we
  // have hmtx
  b = fnt__cff_index_get(info->charstrings, glyph_index);
  while (b.cursor < b.size) {
    i = 0;
    clear_stack = 1;
    b0 = fnt__buf_get8(&b);
    switch (b0) {
      // @TODO implement hinting
      case 0x13:                              // hintmask
      case 0x14:                              // cntrmask
        if (in_header) maskbits += (sp / 2);  // implicit "vstem"
        in_header = 0;
        fnt__buf_skip(&b, (maskbits + 7) / 8);
        break;
      case 0x01:  // hstem
      case 0x03:  // vstem
      case 0x12:  // hstemhm
      case 0x17:  // vstemhm
        maskbits += (sp / 2);
        break;
      case 0x15:  // rmoveto
        in_header = 0;
        if (sp < 2) return FNT__CSERR("rmoveto stack");
        fnt__csctx_rmove_to(c, s[sp - 2], s[sp - 1]);
        break;
      case 0x04:  // vmoveto
        in_header = 0;
        if (sp < 1) return FNT__CSERR("vmoveto stack");
        fnt__csctx_rmove_to(c, 0, s[sp - 1]);
        break;
      case 0x16:  // hmoveto
        in_header = 0;
        if (sp < 1) return FNT__CSERR("hmoveto stack");
        fnt__csctx_rmove_to(c, s[sp - 1], 0);
        break;
      case 0x05:  // rlineto
        if (sp < 2) return FNT__CSERR("rlineto stack");
        for (; i + 1 < sp; i += 2) fnt__csctx_rline_to(c, s[i], s[i + 1]);
        break;
        // hlineto/vlineto and vhcurveto/hvcurveto alternate horizontal and
        // vertical starting from a different place.
      case 0x07:  // vlineto
        if (sp < 1) return FNT__CSERR("vlineto stack");
        goto vlineto;
      case 0x06:  // hlineto
        if (sp < 1) return FNT__CSERR("hlineto stack");
        for (;;) {
          if (i >= sp) break;
          fnt__csctx_rline_to(c, s[i], 0);
          i++;
        vlineto:
          if (i >= sp) break;
          fnt__csctx_rline_to(c, 0, s[i]);
          i++;
        }
        break;
      case 0x1F:  // hvcurveto
        if (sp < 4) return FNT__CSERR("hvcurveto stack");
        goto hvcurveto;
      case 0x1E:  // vhcurveto
        if (sp < 4) return FNT__CSERR("vhcurveto stack");
        for (;;) {
          if (i + 3 >= sp) break;
          fnt__csctx_rccurve_to(c, 0, s[i], s[i + 1], s[i + 2], s[i + 3],
                                (sp - i == 5) ? s[i + 4] : 0.0f);
          i += 4;
        hvcurveto:
          if (i + 3 >= sp) break;
          fnt__csctx_rccurve_to(c, s[i], 0, s[i + 1], s[i + 2],
                                (sp - i == 5) ? s[i + 4] : 0.0f, s[i + 3]);
          i += 4;
        }
        break;
      case 0x08:  // rrcurveto
        if (sp < 6) return FNT__CSERR("rcurveline stack");
        for (; i + 5 < sp; i += 6)
          fnt__csctx_rccurve_to(c, s[i], s[i + 1], s[i + 2], s[i + 3], s[i + 4],
                                s[i + 5]);
        break;
      case 0x18:  // rcurveline
        if (sp < 8) return FNT__CSERR("rcurveline stack");
        for (; i + 5 < sp - 2; i += 6)
          fnt__csctx_rccurve_to(c, s[i], s[i + 1], s[i + 2], s[i + 3], s[i + 4],
                                s[i + 5]);
        if (i + 1 >= sp) return FNT__CSERR("rcurveline stack");
        fnt__csctx_rline_to(c, s[i], s[i + 1]);
        break;
      case 0x19:  // rlinecurve
        if (sp < 8) return FNT__CSERR("rlinecurve stack");
        for (; i + 1 < sp - 6; i += 2) fnt__csctx_rline_to(c, s[i], s[i + 1]);
        if (i + 5 >= sp) return FNT__CSERR("rlinecurve stack");
        fnt__csctx_rccurve_to(c, s[i], s[i + 1], s[i + 2], s[i + 3], s[i + 4],
                              s[i + 5]);
        break;
      case 0x1A:  // vvcurveto
      case 0x1B:  // hhcurveto
        if (sp < 4) return FNT__CSERR("(vv|hh)curveto stack");
        f = 0.0;
        if (sp & 1) {
          f = s[i];
          i++;
        }
        for (; i + 3 < sp; i += 4) {
          if (b0 == 0x1B)
            fnt__csctx_rccurve_to(c, s[i], f, s[i + 1], s[i + 2], s[i + 3],
                                  0.0);
          else
            fnt__csctx_rccurve_to(c, f, s[i], s[i + 1], s[i + 2], 0.0,
                                  s[i + 3]);
          f = 0.0;
        }
        break;
      case 0x0A:  // callsubr
        if (!has_subrs) {
          if (info->fdselect.size)
            subrs = fnt__cid_get_glyph_subrs(info, glyph_index);
          has_subrs = 1;
        }
        fall_through
            // fallthrough
            case 0x1D :  // callgsubr
                         if (sp < 1) return FNT__CSERR("call(g|)subr stack");
        v = (int)s[--sp];
        if (subr_stack_height >= 10) return FNT__CSERR("recursion limit");
        subr_stack[subr_stack_height++] = b;
        b = fnt__get_subr(b0 == 0x0A ? subrs : info->gsubrs, v);
        if (b.size == 0) return FNT__CSERR("subr not found");
        b.cursor = 0;
        clear_stack = 0;
        break;
      case 0x0B:  // return
        if (subr_stack_height <= 0) return FNT__CSERR("return outside subr");
        b = subr_stack[--subr_stack_height];
        clear_stack = 0;
        break;
      case 0x0E:  // endchar
        fnt__csctx_close_shape(c);
        return 1;
      case 0x0C: {  // two-byte escape
        float dx1, dx2, dx3, dx4, dx5, dx6, dy1, dy2, dy3, dy4, dy5, dy6;
        float dx, dy;
        int b1 = fnt__buf_get8(&b);
        switch (b1) {
          // @TODO These "flex" implementations ignore the flex-depth and
          // resolution, and always draw beziers.
          case 0x22:  // hflex
            if (sp < 7) return FNT__CSERR("hflex stack");
            dx1 = s[0];
            dx2 = s[1];
            dy2 = s[2];
            dx3 = s[3];
            dx4 = s[4];
            dx5 = s[5];
            dx6 = s[6];
            fnt__csctx_rccurve_to(c, dx1, 0, dx2, dy2, dx3, 0);
            fnt__csctx_rccurve_to(c, dx4, 0, dx5, -dy2, dx6, 0);
            break;

          case 0x23:  // flex
            if (sp < 13) return FNT__CSERR("flex stack");
            dx1 = s[0];
            dy1 = s[1];
            dx2 = s[2];
            dy2 = s[3];
            dx3 = s[4];
            dy3 = s[5];
            dx4 = s[6];
            dy4 = s[7];
            dx5 = s[8];
            dy5 = s[9];
            dx6 = s[10];
            dy6 = s[11];
            // fd is s[12]
            fnt__csctx_rccurve_to(c, dx1, dy1, dx2, dy2, dx3, dy3);
            fnt__csctx_rccurve_to(c, dx4, dy4, dx5, dy5, dx6, dy6);
            break;

          case 0x24:  // hflex1
            if (sp < 9) return FNT__CSERR("hflex1 stack");
            dx1 = s[0];
            dy1 = s[1];
            dx2 = s[2];
            dy2 = s[3];
            dx3 = s[4];
            dx4 = s[5];
            dx5 = s[6];
            dy5 = s[7];
            dx6 = s[8];
            fnt__csctx_rccurve_to(c, dx1, dy1, dx2, dy2, dx3, 0);
            fnt__csctx_rccurve_to(c, dx4, 0, dx5, dy5, dx6, -(dy1 + dy2 + dy5));
            break;

          case 0x25:  // flex1
            if (sp < 11) return FNT__CSERR("flex1 stack");
            dx1 = s[0];
            dy1 = s[1];
            dx2 = s[2];
            dy2 = s[3];
            dx3 = s[4];
            dy3 = s[5];
            dx4 = s[6];
            dy4 = s[7];
            dx5 = s[8];
            dy5 = s[9];
            dx6 = dy6 = s[10];
            dx = dx1 + dx2 + dx3 + dx4 + dx5;
            dy = dy1 + dy2 + dy3 + dy4 + dy5;
            if (FNT_fabs(dx) > FNT_fabs(dy))
              dy6 = -dy;
            else
              dx6 = -dx;
            fnt__csctx_rccurve_to(c, dx1, dy1, dx2, dy2, dx3, dy3);
            fnt__csctx_rccurve_to(c, dx4, dy4, dx5, dy5, dx6, dy6);
            break;

          default:
            return FNT__CSERR("unimplemented");
        }
      } break;
      default:
        if (b0 != 255 && b0 != 28 && (b0 < 32 || b0 > 254))
          return FNT__CSERR("reserved operator");

        // push immediate
        if (b0 == 255) {
          f = (float)(fnt_int32)fnt__buf_get32(&b) / 0x10000;
        } else {
          fnt__buf_skip(&b, -1);
          f = (float)(fnt_int16)fnt__cff_int(&b);
        }
        if (sp >= 48) return FNT__CSERR("push stack overflow");
        s[sp++] = f;
        clear_stack = 0;
        break;
    }
    if (clear_stack) sp = 0;
  }
  return FNT__CSERR("no endchar");

#undef FNT__CSERR
}
static int
fnt__get_glyph_shape_t2(const struct fnt_info *info, int glyph_index,
                        struct fnt_vertex **pvertices) {
  // runs the charstring twice, once to count and once to output (to avoid
  // realloc)
  struct fnt__csctx count_ctx = FNT__CSCTX_INIT(1);
  struct fnt__csctx output_ctx = FNT__CSCTX_INIT(0);
  if (fnt__run_charstring(info, glyph_index, &count_ctx)) {
    *pvertices = (struct fnt_vertex *)FNT_malloc(
        count_ctx.num_vertices * sizeof(struct fnt_vertex), info->userdata);
    output_ctx.pvertices = *pvertices;
    if (fnt__run_charstring(info, glyph_index, &output_ctx)) {
      FNT_assert(output_ctx.num_vertices == count_ctx.num_vertices);
      return output_ctx.num_vertices;
    }
  }
  *pvertices = NULL;
  return 0;
}
static int
fnt__get_glyph_info_t2(const struct fnt_info *info, int glyph_index, int *x0,
                       int *y0, int *x1, int *y1) {
  struct fnt__csctx c = FNT__CSCTX_INIT(1);
  int r = fnt__run_charstring(info, glyph_index, &c);
  if (x0) *x0 = r ? c.min_x : 0;
  if (y0) *y0 = r ? c.min_y : 0;
  if (x1) *x1 = r ? c.max_x : 0;
  if (y1) *y1 = r ? c.max_y : 0;
  return r ? c.num_vertices : 0;
}
static int
fnt_get_glyph_shape(const struct fnt_info *info, int glyph_index,
                    struct fnt_vertex **pvertices) {
  if (!info->cff.size)
    return fnt__get_glyph_shape_tt(info, glyph_index, pvertices);
  else
    return fnt__get_glyph_shape_t2(info, glyph_index, pvertices);
}
static void
fnt_get_glyph_hmetrics(const struct fnt_info *info, int glyph_index,
                       int *advanceWidth, int *leftSideBearing) {
  fnt_uint16 numOfLongHorMetrics = ttUSHORT(info->data + info->hhea + 34);
  if (glyph_index < numOfLongHorMetrics) {
    if (advanceWidth)
      *advanceWidth = ttSHORT(info->data + info->hmtx + 4 * glyph_index);
    if (leftSideBearing)
      *leftSideBearing = ttSHORT(info->data + info->hmtx + 4 * glyph_index + 2);
  } else {
    if (advanceWidth)
      *advanceWidth =
          ttSHORT(info->data + info->hmtx + 4 * (numOfLongHorMetrics - 1));
    if (leftSideBearing)
      *leftSideBearing =
          ttSHORT(info->data + info->hmtx + 4 * numOfLongHorMetrics +
                  2 * (glyph_index - numOfLongHorMetrics));
  }
}
static int
fnt__get_glyph_kern_info_advance(const struct fnt_info *info, int glyph1,
                                 int glyph2) {
  fnt_uint8 *data = info->data + info->kern;
  fnt_uint32 needle, straw;
  int l, r, m;

  // we only look at the first table. it must be 'horizontal' and format 0.
  if (!info->kern) return 0;
  if (ttUSHORT(data + 2) < 1)  // number of tables, need at least 1
    return 0;
  if (ttUSHORT(data + 8) != 1)  // horizontal flag must be set in format
    return 0;

  l = 0;
  r = ttUSHORT(data + 10) - 1;
  needle = glyph1 << 16 | glyph2;
  while (l <= r) {
    m = (l + r) >> 1;
    straw = ttULONG(data + 18 + (m * 6));  // note: unaligned read
    if (needle < straw)
      r = m - 1;
    else if (needle > straw)
      l = m + 1;
    else
      return ttSHORT(data + 22 + (m * 6));
  }
  return 0;
}
static fnt_int32
fnt__get_coverage_index(fnt_uint8 *coverageTable, int glyph) {
  fnt_uint16 coverageFormat = ttUSHORT(coverageTable);
  switch (coverageFormat) {
    case 1: {
      fnt_uint16 glyphCount = ttUSHORT(coverageTable + 2);

      // Binary search.
      fnt_int32 l = 0, r = glyphCount - 1, m;
      int straw, needle = glyph;
      while (l <= r) {
        fnt_uint8 *glyphArray = coverageTable + 4;
        fnt_uint16 glyphID;
        m = (l + r) >> 1;
        glyphID = ttUSHORT(glyphArray + 2 * m);
        straw = glyphID;
        if (needle < straw)
          r = m - 1;
        else if (needle > straw)
          l = m + 1;
        else {
          return m;
        }
      }
    } break;

    case 2: {
      fnt_uint16 rangeCount = ttUSHORT(coverageTable + 2);
      fnt_uint8 *rangeArray = coverageTable + 4;

      // Binary search.
      fnt_int32 l = 0, r = rangeCount - 1, m;
      int strawStart, strawEnd, needle = glyph;
      while (l <= r) {
        fnt_uint8 *rangeRecord;
        m = (l + r) >> 1;
        rangeRecord = rangeArray + 6 * m;
        strawStart = ttUSHORT(rangeRecord);
        strawEnd = ttUSHORT(rangeRecord + 2);
        if (needle < strawStart)
          r = m - 1;
        else if (needle > strawEnd)
          l = m + 1;
        else {
          fnt_uint16 startCoverageIndex = ttUSHORT(rangeRecord + 4);
          return startCoverageIndex + glyph - strawStart;
        }
      }
    } break;

    default: {
      // There are no other cases.
      FNT_assert(0);
    } break;
  }

  return -1;
}
static fnt_int32
fnt__get_glyph_class(fnt_uint8 *classDefTable, int glyph) {
  fnt_uint16 classDefFormat = ttUSHORT(classDefTable);
  switch (classDefFormat) {
    case 1: {
      fnt_uint16 startGlyphID = ttUSHORT(classDefTable + 2);
      fnt_uint16 glyphCount = ttUSHORT(classDefTable + 4);
      fnt_uint8 *classDef1ValueArray = classDefTable + 6;

      if (glyph >= startGlyphID && glyph < startGlyphID + glyphCount)
        return (fnt_int32)ttUSHORT(classDef1ValueArray +
                                   2 * (glyph - startGlyphID));

      classDefTable = classDef1ValueArray + 2 * glyphCount;
    } break;

    case 2: {
      fnt_uint16 classRangeCount = ttUSHORT(classDefTable + 2);
      fnt_uint8 *classRangeRecords = classDefTable + 4;

      // Binary search.
      fnt_int32 l = 0, r = classRangeCount - 1, m;
      int strawStart, strawEnd, needle = glyph;
      while (l <= r) {
        fnt_uint8 *classRangeRecord;
        m = (l + r) >> 1;
        classRangeRecord = classRangeRecords + 6 * m;
        strawStart = ttUSHORT(classRangeRecord);
        strawEnd = ttUSHORT(classRangeRecord + 2);
        if (needle < strawStart)
          r = m - 1;
        else if (needle > strawEnd)
          l = m + 1;
        else
          return (fnt_int32)ttUSHORT(classRangeRecord + 4);
      }

      classDefTable = classRangeRecords + 6 * classRangeCount;
    } break;

    default: {
      // There are no other cases.
      FNT_assert(0);
    } break;
  }
  return -1;
}

// Define to FNT_assert(x) if you want to break on unimplemented formats.
#define FNT_GPOS_TODO_assert(x)

static fnt_int32
fnt__get_glyph_gpos_info_advance(const struct fnt_info *info, int glyph1,
                                 int glyph2) {
  fnt_uint16 lookupListOffset;
  fnt_uint8 *lookupList;
  fnt_uint16 lookupCount;
  fnt_uint8 *data;
  fnt_int32 i;

  if (!info->gpos) return 0;

  data = info->data + info->gpos;

  if (ttUSHORT(data + 0) != 1) return 0;  // Major version 1
  if (ttUSHORT(data + 2) != 0) return 0;  // Minor version 0

  lookupListOffset = ttUSHORT(data + 8);
  lookupList = data + lookupListOffset;
  lookupCount = ttUSHORT(lookupList);

  for (i = 0; i < lookupCount; ++i) {
    fnt_uint16 lookupOffset = ttUSHORT(lookupList + 2 + 2 * i);
    fnt_uint8 *lookupTable = lookupList + lookupOffset;

    fnt_uint16 lookupType = ttUSHORT(lookupTable);
    fnt_uint16 subTableCount = ttUSHORT(lookupTable + 4);
    fnt_uint8 *subTableOffsets = lookupTable + 6;
    switch (lookupType) {
      case 2: {  // Pair Adjustment Positioning Subtable
        fnt_int32 sti;
        for (sti = 0; sti < subTableCount; sti++) {
          fnt_uint16 subtableOffset = ttUSHORT(subTableOffsets + 2 * sti);
          fnt_uint8 *table = lookupTable + subtableOffset;
          fnt_uint16 posFormat = ttUSHORT(table);
          fnt_uint16 coverageOffset = ttUSHORT(table + 2);
          fnt_int32 coverageIndex =
              fnt__get_coverage_index(table + coverageOffset, glyph1);
          if (coverageIndex == -1) continue;

          switch (posFormat) {
            case 1: {
              fnt_int32 l, r, m;
              int straw, needle;
              fnt_uint16 valueFormat1 = ttUSHORT(table + 4);
              fnt_uint16 valueFormat2 = ttUSHORT(table + 6);
              fnt_int32 valueRecordPairSizeInBytes = 2;
              fnt_uint16 pairSetCount = ttUSHORT(table + 8);
              fnt_uint16 pairPosOffset =
                  ttUSHORT(table + 10 + 2 * coverageIndex);
              fnt_uint8 *pairValueTable = table + pairPosOffset;
              fnt_uint16 pairValueCount = ttUSHORT(pairValueTable);
              fnt_uint8 *pairValueArray = pairValueTable + 2;

              // TODO: Support more formats.
              FNT_GPOS_TODO_assert(valueFormat1 == 4);
              if (valueFormat1 != 4) return 0;
              FNT_GPOS_TODO_assert(valueFormat2 == 0);
              if (valueFormat2 != 0) return 0;

              unused(pairSetCount);
              FNT_assert(coverageIndex < pairSetCount);

              needle = glyph2;
              r = pairValueCount - 1;
              l = 0;

              // Binary search.
              while (l <= r) {
                fnt_uint16 secondGlyph;
                fnt_uint8 *pairValue;
                m = (l + r) >> 1;
                pairValue =
                    pairValueArray + (2 + valueRecordPairSizeInBytes) * m;
                secondGlyph = ttUSHORT(pairValue);
                straw = secondGlyph;
                if (needle < straw)
                  r = m - 1;
                else if (needle > straw)
                  l = m + 1;
                else {
                  fnt_int16 xAdvance = ttSHORT(pairValue + 2);
                  return xAdvance;
                }
              }
            } break;

            case 2: {
              fnt_uint16 valueFormat1 = ttUSHORT(table + 4);
              fnt_uint16 valueFormat2 = ttUSHORT(table + 6);

              fnt_uint16 classDef1Offset = ttUSHORT(table + 8);
              fnt_uint16 classDef2Offset = ttUSHORT(table + 10);
              int glyph1class =
                  fnt__get_glyph_class(table + classDef1Offset, glyph1);
              int glyph2class =
                  fnt__get_glyph_class(table + classDef2Offset, glyph2);

              fnt_uint16 class1Count = ttUSHORT(table + 12);
              fnt_uint16 class2Count = ttUSHORT(table + 14);
              FNT_assert(glyph1class < class1Count);
              FNT_assert(glyph2class < class2Count);

              // TODO: Support more formats.
              FNT_GPOS_TODO_assert(valueFormat1 == 4);
              if (valueFormat1 != 4) return 0;
              FNT_GPOS_TODO_assert(valueFormat2 == 0);
              if (valueFormat2 != 0) return 0;

              if (glyph1class >= 0 && glyph1class < class1Count &&
                  glyph2class >= 0 && glyph2class < class2Count) {
                fnt_uint8 *class1Records = table + 16;
                fnt_uint8 *class2Records =
                    class1Records + 2 * (glyph1class * class2Count);
                fnt_int16 xAdvance = ttSHORT(class2Records + 2 * glyph2class);
                return xAdvance;
              }
            } break;

            default: {
              // There are no other cases.
              FNT_assert(0);
              break;
            }
          }
        }
        break;
      }

      default:
        // TODO: Implement other stuff.
        break;
    }
  }

  return 0;
}
static int
fnt_get_glyph_kern_advance(const struct fnt_info *info, int g1, int g2) {
  int xAdvance = 0;
  if (info->gpos) xAdvance += fnt__get_glyph_gpos_info_advance(info, g1, g2);
  if (info->kern) xAdvance += fnt__get_glyph_kern_info_advance(info, g1, g2);
  return xAdvance;
}
static int
fnt_get_codepoint_kern_advance(const struct fnt_info *info, int ch1, int ch2) {
  if (!info->kern && !info->gpos)  // if no kerning table, don't waste time
                                   // looking up both codepoint->glyphs
    return 0;
  return fnt_get_glyph_kern_advance(info, fnt_fnd_glyph_index(info, ch1),
                                    fnt_fnd_glyph_index(info, ch2));
}
static void
fnt_get_codepoint_hmetrics(const struct fnt_info *info, int codepoint,
                           int *advanceWidth, int *leftSideBearing) {
  fnt_get_glyph_hmetrics(info, fnt_fnd_glyph_index(info, codepoint),
                         advanceWidth, leftSideBearing);
}
static void
fnt_get_vmetrics(const struct fnt_info *info, int *ascent, int *descent,
                 int *lineGap) {
  if (ascent) *ascent = ttSHORT(info->data + info->hhea + 4);
  if (descent) *descent = ttSHORT(info->data + info->hhea + 6);
  if (lineGap) *lineGap = ttSHORT(info->data + info->hhea + 8);
}
#if 0
static int
fnt_get_vmetrics_OS2(const struct fnt_info *info, int *typoAscent,
                     int *typoDescent, int *typoLineGap) {
  int tab = fnt__find_table(info->data, info->fontstart, "OS/2");
  if (!tab) return 0;
  if (typoAscent) *typoAscent = ttSHORT(info->data + tab + 68);
  if (typoDescent) *typoDescent = ttSHORT(info->data + tab + 70);
  if (typoLineGap) *typoLineGap = ttSHORT(info->data + tab + 72);
  return 1;
}
static void
fnt_get_bounding_box(const struct fnt_info *info, int *x0, int *y0, int *x1,
                     int *y1) {
  *x0 = ttSHORT(info->data + info->head + 36);
  *y0 = ttSHORT(info->data + info->head + 38);
  *x1 = ttSHORT(info->data + info->head + 40);
  *y1 = ttSHORT(info->data + info->head + 42);
}
#endif

static float
fnt_scale_for_pixel_height(const struct fnt_info *info, float height) {
  int fheight = ttSHORT(info->data + info->hhea + 4) -
                ttSHORT(info->data + info->hhea + 6);
  return (float)height / fheight;
}
static float
fnt_scale_for_mapping_em_to_pixels(const struct fnt_info *info, float pixels) {
  int unitsPerEm = ttUSHORT(info->data + info->head + 18);
  return pixels / unitsPerEm;
}
#if 0
static void
fnt_free_shape(const struct fnt_info *info, struct fnt_vertex *v) {
  FNT_free(v, info->userdata);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// antialiasing software rasterizer
//
static void
fnt_get_glyph_bitmap_box_subpixel(const struct fnt_info *font, int glyph,
                                  float scale_x, float scale_y, float shift_x,
                                  float shift_y, int *ix0, int *iy0, int *ix1,
                                  int *iy1) {
  int x0 = 0, y0 = 0, x1, y1;  // =0 suppresses compiler warning
  if (!fnt_get_glyph_box(font, glyph, &x0, &y0, &x1, &y1)) {
    // e.g. space character
    if (ix0) *ix0 = 0;
    if (iy0) *iy0 = 0;
    if (ix1) *ix1 = 0;
    if (iy1) *iy1 = 0;
  } else {
    // move to integral bboxes (treating pixels as little squares, what pixels
    // get touched)?
    if (ix0) *ix0 = FNT_ifloor(x0 * scale_x + shift_x);
    if (iy0) *iy0 = FNT_ifloor(-y1 * scale_y + shift_y);
    if (ix1) *ix1 = FNT_iceil(x1 * scale_x + shift_x);
    if (iy1) *iy1 = FNT_iceil(-y0 * scale_y + shift_y);
  }
}
static void
fnt_get_glyph_bitmap_box(const struct fnt_info *font, int glyph, float scale_x,
                         float scale_y, int *ix0, int *iy0, int *ix1,
                         int *iy1) {
  fnt_get_glyph_bitmap_box_subpixel(font, glyph, scale_x, scale_y, 0.0f, 0.0f,
                                    ix0, iy0, ix1, iy1);
}
#if 0
static void
fnt_get_codepoint_bitmap_box_subpixel(const struct fnt_info *font,
                                      int codepoint, float scale_x,
                                      float scale_y, float shift_x,
                                      float shift_y, int *ix0, int *iy0,
                                      int *ix1, int *iy1) {
  fnt_get_glyph_bitmap_box_subpixel(font, fnt_fnd_glyph_index(font, codepoint),
                                    scale_x, scale_y, shift_x, shift_y, ix0,
                                    iy0, ix1, iy1);
}
static void
fnt_get_codepoint_bitmap_box(const struct fnt_info *font, int codepoint,
                             float scale_x, float scale_y, int *ix0, int *iy0,
                             int *ix1, int *iy1) {
  fnt_get_codepoint_bitmap_box_subpixel(font, codepoint, scale_x, scale_y, 0.0f,
                                        0.0f, ix0, iy0, ix1, iy1);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  Rasterizer
struct fnt__hheap_chunk {
  struct fnt__hheap_chunk *next;
};
struct fnt__hheap {
  struct fnt__hheap_chunk *head;
  void *first_free;
  int num_remaining_in_head_chunk;
};
struct fnt__edge {
  float x0, y0, x1, y1;
  int invert;
};
struct fnt__active_edge {
  struct fnt__active_edge *next;
#if FNT_RASTERIZER_VERSION == 1
  int x, dx;
  float ey;
  int direction;
#elif FNT_RASTERIZER_VERSION == 2
  float fx, fdx, fdy;
  float direction;
  float sy;
  float ey;
#else
#error "Unrecognized value of FNT_RASTERIZER_VERSION"
#endif
};
static void *
fnt__hheap_alloc(struct fnt__hheap *hh, size_t size, void *userdata) {
  if (hh->first_free) {
    void *p = hh->first_free;
    hh->first_free = *(void **)p;
    return p;
  } else {
    if (hh->num_remaining_in_head_chunk == 0) {
      int count = (size < 32 ? 2000 : size < 128 ? 800 : 100);
      struct fnt__hheap_chunk *c = (struct fnt__hheap_chunk *)FNT_malloc(
          sizeof(struct fnt__hheap_chunk) + size * count, userdata);
      if (c == NULL) return NULL;
      c->next = hh->head;
      hh->head = c;
      hh->num_remaining_in_head_chunk = count;
    }
    --hh->num_remaining_in_head_chunk;
    return (char *)(hh->head) + sizeof(struct fnt__hheap_chunk) +
           size * hh->num_remaining_in_head_chunk;
  }
}
static void
fnt__hheap_free(struct fnt__hheap *hh, void *p) {
  *(void **)p = hh->first_free;
  hh->first_free = p;
}
static void
fnt__hheap_cleanup(struct fnt__hheap *hh, void *userdata) {
  struct fnt__hheap_chunk *c = hh->head;
  while (c) {
    struct fnt__hheap_chunk *n = c->next;
    FNT_free(c, userdata);
    c = n;
  }
}

#if FNT_RASTERIZER_VERSION == 1

#define FNT_FIXSHIFT 10
#define FNT_FIX (1 << FNT_FIXSHIFT)
#define FNT_FIXMASK (FNT_FIX - 1)

static struct fnt__active_edge *
fnt__new_active(struct fnt__hheap *hh, struct fnt__edge *e, int off_x,
                float start_point, void *userdata) {
  struct fnt__active_edge *z =
      (struct fnt__active_edge *)fnt__hheap_alloc(hh, sizeof(*z), userdata);
  float dxdy = (e->x1 - e->x0) / (e->y1 - e->y0);
  FNT_assert(z != NULL);
  if (!z) return z;

  // round dx down to avoid overshooting
  if (dxdy < 0)
    z->dx = -FNT_ifloor(FNT_FIX * -dxdy);
  else
    z->dx = FNT_ifloor(FNT_FIX * dxdy);

  z->x = FNT_ifloor(
      FNT_FIX * e->x0 +
      z->dx * (start_point - e->y0));  // use z->dx so when we offset later it's
                                       // by the same amount
  z->x -= off_x * FNT_FIX;

  z->ey = e->y1;
  z->next = 0;
  z->direction = e->invert ? 1 : -1;
  return z;
}
#elif FNT_RASTERIZER_VERSION == 2
static struct fnt__active_edge *
fnt__new_active(struct fnt__hheap *hh, struct fnt__edge *e, int off_x,
                float start_point, void *userdata) {
  struct fnt__active_edge *z =
      (struct fnt__active_edge *)fnt__hheap_alloc(hh, sizeof(*z), userdata);
  float dxdy = (e->x1 - e->x0) / (e->y1 - e->y0);
  FNT_assert(z != NULL);
  // FNT_assert(e->y0 <= start_point);
  if (!z) return z;
  z->fdx = dxdy;
  z->fdy = dxdy != 0.0f ? (1.0f / dxdy) : 0.0f;
  z->fx = e->x0 + dxdy * (start_point - e->y0);
  z->fx -= off_x;
  z->direction = e->invert ? 1.0f : -1.0f;
  z->sy = e->y0;
  z->ey = e->y1;
  z->next = 0;
  return z;
}
#else
#error "Unrecognized value of FNT_RASTERIZER_VERSION"
#endif

#if FNT_RASTERIZER_VERSION == 1
// note: this routine clips fills that extend off the edges... ideally this
// wouldn't happen, but it could happen if the truetype glyph bounding boxes
// are wrong, or if the user supplies a too-small bitmap
static void
fnt__fill_active_edges(unsigned char *scanline, int len,
                       struct fnt__active_edge *e, int max_weight) {
  // non-zero winding fill
  int x0 = 0, w = 0;
  while (e) {
    if (w == 0) {
      // if we're currently at zero, we need to record the edge start point
      x0 = e->x;
      w += e->direction;
    } else {
      int x1 = e->x;
      w += e->direction;
      // if we went to zero, we need to draw
      if (w == 0) {
        int i = x0 >> FNT_FIXSHIFT;
        int j = x1 >> FNT_FIXSHIFT;

        if (i < len && j >= 0) {
          if (i == j) {
            // x0,x1 are the same pixel, so compute combined coverage
            scanline[i] = scanline[i] +
                          (fnt_uint8)((x1 - x0) * max_weight >> FNT_FIXSHIFT);
          } else {
            if (i >= 0)  // add antialiasing for x0
              scanline[i] =
                  scanline[i] +
                  (fnt_uint8)(((FNT_FIX - (x0 & FNT_FIXMASK)) * max_weight) >>
                              FNT_FIXSHIFT);
            else
              i = -1;  // clip

            if (j < len)  // add antialiasing for x1
              scanline[j] =
                  scanline[j] + (fnt_uint8)(((x1 & FNT_FIXMASK) * max_weight) >>
                                            FNT_FIXSHIFT);
            else
              j = len;  // clip

            for (++i; i < j; ++i)  // fill pixels between x0 and x1
              scanline[i] = scanline[i] + (fnt_uint8)max_weight;
          }
        }
      }
    }
    e = e->next;
  }
}
static void
fnt__rasterize_sorted_edges(struct fnt__bitmap *result, struct fnt__edge *e,
                            int n, int vsubsample, int off_x, int off_y,
                            void *userdata) {
  struct fnt__hheap hh = {0, 0, 0};
  struct fnt__active_edge *active = NULL;
  int y, j = 0;
  int max_weight = (255 / vsubsample);  // weight per vertical scanline
  int s;                                // vertical subsample index
  unsigned char scanline_data[512], *scanline;

  if (result->w > 512)
    scanline = (unsigned char *)FNT_malloc(result->w, userdata);
  else
    scanline = scanline_data;

  y = off_y * vsubsample;
  e[n].y0 = (off_y + result->h) * (float)vsubsample + 1;

  while (j < result->h) {
    FNT_memset(scanline, 0, result->w);
    for (s = 0; s < vsubsample; ++s) {
      // find center of pixel for this scanline
      float scan_y = y + 0.5f;
      struct fnt__active_edge **step = &active;

      // update all active edges;
      // remove all active edges that terminate before the center of this
      // scanline
      while (*step) {
        fnt__active_edge *z = *step;
        if (z->ey <= scan_y) {
          *step = z->next;  // delete from list
          FNT_assert(z->direction);
          z->direction = 0;
          fnt__hheap_free(&hh, z);
        } else {
          z->x += z->dx;            // advance to position for current scanline
          step = &((*step)->next);  // advance through list
        }
      }

      // resort the list if needed
      for (;;) {
        int changed = 0;
        step = &active;
        while (*step && (*step)->next) {
          if ((*step)->x > (*step)->next->x) {
            fnt__active_edge *t = *step;
            fnt__active_edge *q = t->next;

            t->next = q->next;
            q->next = t;
            *step = q;
            changed = 1;
          }
          step = &(*step)->next;
        }
        if (!changed) break;
      }

      // insert all edges that start before the center of this scanline -- omit
      // ones that also end on this scanline
      while (e->y0 <= scan_y) {
        if (e->y1 > scan_y) {
          struct fnt__active_edge *z =
              fnt__new_active(&hh, e, off_x, scan_y, userdata);
          if (z != NULL) {
            // find insertion point
            if (active == NULL)
              active = z;
            else if (z->x < active->x) {
              // insert at front
              z->next = active;
              active = z;
            } else {
              // find thing to insert AFTER
              fnt__active_edge *p = active;
              while (p->next && p->next->x < z->x) p = p->next;
              // at this point, p->next->x is NOT < z->x
              z->next = p->next;
              p->next = z;
            }
          }
        }
        ++e;
      }
      // now process all active edges in XOR fashion
      if (active)
        fnt__fill_active_edges(scanline, result->w, active, max_weight);
      ++y;
    }
    FNT_memcpy(result->pixels + j * result->stride, scanline, result->w);
    ++j;
  }

  fnt__hheap_cleanup(&hh, userdata);
  if (scanline != scanline_data) FNT_free(scanline, userdata);
}
#elif FNT_RASTERIZER_VERSION == 2

// the edge passed in here does not cross the vertical line at x or the vertical
// line at x+1 (i.e. it has already been clipped to those)
static void
fnt__handle_clipped_edge(float *scanline, int x, struct fnt__active_edge *e,
                         float x0, float y0, float x1, float y1) {
  if (y0 == y1) return;
  FNT_assert(y0 < y1);
  FNT_assert(e->sy <= e->ey);
  if (y0 > e->ey) return;
  if (y1 < e->sy) return;
  if (y0 < e->sy) {
    x0 += (x1 - x0) * (e->sy - y0) / (y1 - y0);
    y0 = e->sy;
  }
  if (y1 > e->ey) {
    x1 += (x1 - x0) * (e->ey - y1) / (y1 - y0);
    y1 = e->ey;
  }

  if (x0 == x)
    FNT_assert(x1 <= x + 1);
  else if (x0 == x + 1)
    FNT_assert(x1 >= x);
  else if (x0 <= x)
    FNT_assert(x1 <= x);
  else if (x0 >= x + 1)
    FNT_assert(x1 >= x + 1);
  else
    FNT_assert(x1 >= x && x1 <= x + 1);

  if (x0 <= x && x1 <= x)
    scanline[x] += e->direction * (y1 - y0);
  else if (x0 >= x + 1 && x1 >= x + 1)
    ;
  else {
    FNT_assert(x0 >= x && x0 <= x + 1 && x1 >= x && x1 <= x + 1);
    scanline[x] +=
        e->direction * (y1 - y0) *
        (1 - ((x0 - x) + (x1 - x)) / 2);  // coverage = 1 - average x position
  }
}
static void
fnt__fill_active_edges_new(float *scanline, float *scanline_fill, int len,
                           struct fnt__active_edge *e, float y_top) {
  float y_bottom = y_top + 1;
  while (e) {
    // brute force every pixel
    // compute intersection points with top & bottom
    FNT_assert(e->ey >= y_top);

    if (e->fdx == 0) {
      float x0 = e->fx;
      if (x0 < len) {
        if (x0 >= 0) {
          fnt__handle_clipped_edge(scanline, (int)x0, e, x0, y_top, x0,
                                   y_bottom);
          fnt__handle_clipped_edge(scanline_fill - 1, (int)x0 + 1, e, x0, y_top,
                                   x0, y_bottom);
        } else {
          fnt__handle_clipped_edge(scanline_fill - 1, 0, e, x0, y_top, x0,
                                   y_bottom);
        }
      }
    } else {
      float x0 = e->fx;
      float dx = e->fdx;
      float xb = x0 + dx;
      float x_top, x_bottom;
      float sy0, sy1;
      float dy = e->fdy;
      FNT_assert(e->sy <= y_bottom && e->ey >= y_top);

      // compute endpoints of line segment clipped to this scanline (if the
      // line segment starts on this scanline. x0 is the intersection of the
      // line with y_top, but that may be off the line segment.
      if (e->sy > y_top) {
        x_top = x0 + dx * (e->sy - y_top);
        sy0 = e->sy;
      } else {
        x_top = x0;
        sy0 = y_top;
      }
      if (e->ey < y_bottom) {
        x_bottom = x0 + dx * (e->ey - y_top);
        sy1 = e->ey;
      } else {
        x_bottom = xb;
        sy1 = y_bottom;
      }

      if (x_top >= 0 && x_bottom >= 0 && x_top < len && x_bottom < len) {
        // from here on, we don't have to range check x values

        if ((int)x_top == (int)x_bottom) {
          float height;
          // simple case, only spans one pixel
          int x = (int)x_top;
          height = sy1 - sy0;
          FNT_assert(x >= 0 && x < len);
          scanline[x] +=
              e->direction * (1 - ((x_top - x) + (x_bottom - x)) / 2) * height;
          scanline_fill[x] +=
              e->direction *
              height;  // everything right of this pixel is filled
        } else {
          int x, x1, x2;
          float y_crossing, step, sign, area;
          // covers 2+ pixels
          if (x_top > x_bottom) {
            // flip scanline vertically; signed area is the same
            float t;
            sy0 = y_bottom - (sy0 - y_top);
            sy1 = y_bottom - (sy1 - y_top);
            t = sy0, sy0 = sy1, sy1 = t;
            t = x_bottom, x_bottom = x_top, x_top = t;
            dx = -dx;
            dy = -dy;
            t = x0, x0 = xb, xb = t;
          }

          x1 = (int)x_top;
          x2 = (int)x_bottom;
          // compute intersection with y axis at x1+1
          y_crossing = (x1 + 1 - x0) * dy + y_top;

          sign = e->direction;
          // area of the rectangle covered from y0..y_crossing
          area = sign * (y_crossing - sy0);
          // area of the triangle (x_top,y0), (x+1,y0), (x+1,y_crossing)
          scanline[x1] += area * (1 - ((x_top - x1) + (x1 + 1 - x1)) / 2);

          step = sign * dy;
          for (x = x1 + 1; x < x2; ++x) {
            scanline[x] += area + step / 2;
            area += step;
          }
          y_crossing += dy * (x2 - (x1 + 1));

          FNT_assert(FNT_fabs(area) <= 1.01f);

          scanline[x2] += area + sign *
                                     (1 - ((x2 - x2) + (x_bottom - x2)) / 2) *
                                     (sy1 - y_crossing);

          scanline_fill[x2] += sign * (sy1 - sy0);
        }
      } else {
        // if edge goes outside of box we're drawing, we require
        // clipping logic. since this does not match the intended use
        // of this library, we use a different, very slow brute
        // force implementation
        int x;
        for (x = 0; x < len; ++x) {
          // cases:
          //
          // there can be up to two intersections with the pixel. any
          // intersection with left or right edges can be handled by splitting
          // into two (or three) regions. intersections with top & bottom do not
          // necessitate case-wise logic.
          //
          // the old way of doing this found the intersections with the left &
          // right edges, then used some simple logic to produce up to three
          // segments in sorted order from top-to-bottom. however, this had a
          // problem: if an x edge was epsilon across the x border, then the
          // corresponding y position might not be distinct from the other y
          // segment, and it might ignored as an empty segment. to avoid that,
          // we need to explicitly produce segments based on x positions.

          // rename variables to clearly-defined pairs
          float y0 = y_top;
          float x1 = (float)(x);
          float x2 = (float)(x + 1);
          float x3 = xb;
          float y3 = y_bottom;

          // x = e->x + e->dx * (y-y_top)
          // (y-y_top) = (x - e->x) / e->dx
          // y = (x - e->x) / e->dx + y_top
          float y1 = (x - x0) / dx + y_top;
          float y2 = (x + 1 - x0) / dx + y_top;

          if (x0 < x1 && x3 > x2) {  // three segments descending down-right
            fnt__handle_clipped_edge(scanline, x, e, x0, y0, x1, y1);
            fnt__handle_clipped_edge(scanline, x, e, x1, y1, x2, y2);
            fnt__handle_clipped_edge(scanline, x, e, x2, y2, x3, y3);
          } else if (x3 < x1 &&
                     x0 > x2) {  // three segments descending down-left
            fnt__handle_clipped_edge(scanline, x, e, x0, y0, x2, y2);
            fnt__handle_clipped_edge(scanline, x, e, x2, y2, x1, y1);
            fnt__handle_clipped_edge(scanline, x, e, x1, y1, x3, y3);
          } else if (x0 < x1 && x3 > x1) {  // two segments across x, down-right
            fnt__handle_clipped_edge(scanline, x, e, x0, y0, x1, y1);
            fnt__handle_clipped_edge(scanline, x, e, x1, y1, x3, y3);
          } else if (x3 < x1 && x0 > x1) {  // two segments across x, down-left
            fnt__handle_clipped_edge(scanline, x, e, x0, y0, x1, y1);
            fnt__handle_clipped_edge(scanline, x, e, x1, y1, x3, y3);
          } else if (x0 < x2 &&
                     x3 > x2) {  // two segments across x+1, down-right
            fnt__handle_clipped_edge(scanline, x, e, x0, y0, x2, y2);
            fnt__handle_clipped_edge(scanline, x, e, x2, y2, x3, y3);
          } else if (x3 < x2 &&
                     x0 > x2) {  // two segments across x+1, down-left
            fnt__handle_clipped_edge(scanline, x, e, x0, y0, x2, y2);
            fnt__handle_clipped_edge(scanline, x, e, x2, y2, x3, y3);
          } else {  // one segment
            fnt__handle_clipped_edge(scanline, x, e, x0, y0, x3, y3);
          }
        }
      }
    }
    e = e->next;
  }
}
static void
fnt__rasterize_sorted_edges(struct fnt__bitmap *result, struct fnt__edge *e,
                            int n, int vsubsample, int off_x, int off_y,
                            void *userdata) {
  // directly AA rasterize edges w/o supersampling
  struct fnt__hheap hh = {0, 0, 0};
  struct fnt__active_edge *active = NULL;
  int y, j = 0, i;
  float scanline_data[129], *scanline, *scanline2;

  FNT__NOTUSED(vsubsample);

  if (result->w > 64)
    scanline =
        (float *)FNT_malloc((result->w * 2 + 1) * sizeof(float), userdata);
  else
    scanline = scanline_data;

  scanline2 = scanline + result->w;

  y = off_y;
  e[n].y0 = (float)(off_y + result->h) + 1;

  while (j < result->h) {
    // find center of pixel for this scanline
    float scan_y_top = y + 0.0f;
    float scan_y_bottom = y + 1.0f;
    struct fnt__active_edge **step = &active;

    FNT_memset(scanline, 0, result->w * sizeof(scanline[0]));
    FNT_memset(scanline2, 0, (result->w + 1) * sizeof(scanline[0]));

    // update all active edges;
    // remove all active edges that terminate before the top of this scanline
    while (*step) {
      struct fnt__active_edge *z = *step;
      if (z->ey <= scan_y_top) {
        *step = z->next;  // delete from list
        FNT_assert(z->direction);
        z->direction = 0;
        fnt__hheap_free(&hh, z);
      } else {
        step = &((*step)->next);  // advance through list
      }
    }
    // insert all edges that start before the bottom of this scanline
    while (e->y0 <= scan_y_bottom) {
      if (e->y0 != e->y1) {
        struct fnt__active_edge *z =
            fnt__new_active(&hh, e, off_x, scan_y_top, userdata);
        if (z != NULL) {
          FNT_assert(z->ey >= scan_y_top);
          // insert at front
          z->next = active;
          active = z;
        }
      }
      ++e;
    }

    // now process all active edges
    if (active)
      fnt__fill_active_edges_new(scanline, scanline2 + 1, result->w, active,
                                 scan_y_top);

    {
      float sum = 0;
      for (i = 0; i < result->w; ++i) {
        float k;
        int m;
        sum += scanline2[i];
        k = scanline[i] + sum;
        k = (float)FNT_fabs(k) * 255 + 0.5f;
        m = (int)k;
        if (m > 255) m = 255;
        result->pixels[j * result->stride + i] = (unsigned char)m;
      }
    }
    // advance all the edges
    step = &active;
    while (*step) {
      struct fnt__active_edge *z = *step;
      z->fx += z->fdx;          // advance to position for current scanline
      step = &((*step)->next);  // advance through list
    }

    ++y;
    ++j;
  }

  fnt__hheap_cleanup(&hh, userdata);

  if (scanline != scanline_data) FNT_free(scanline, userdata);
}
#else
#error "Unrecognized value of FNT_RASTERIZER_VERSION"
#endif

#define FNT__COMPARE(a, b) ((a)->y0 < (b)->y0)

static void
fnt__sort_edges_ins_sort(struct fnt__edge *p, int n) {
  int i, j;
  for (i = 1; i < n; ++i) {
    struct fnt__edge t = p[i], *a = &t;
    j = i;
    while (j > 0) {
      struct fnt__edge *b = &p[j - 1];
      int c = FNT__COMPARE(a, b);
      if (!c) break;
      p[j] = p[j - 1];
      --j;
    }
    if (i != j) p[j] = t;
  }
}
static void
fnt__sort_edges_quicksort(struct fnt__edge *p, int n) {
  /* threshhold for transitioning to insertion sort */
  while (n > 12) {
    struct fnt__edge t;
    int c01, c12, c, m, i, j;

    /* compute median of three */
    m = n >> 1;
    c01 = FNT__COMPARE(&p[0], &p[m]);
    c12 = FNT__COMPARE(&p[m], &p[n - 1]);
    /* if 0 >= mid >= end, or 0 < mid < end, then use mid */
    if (c01 != c12) {
      /* otherwise, we'll need to swap something else to middle */
      int z;
      c = FNT__COMPARE(&p[0], &p[n - 1]);
      /* 0>mid && mid<n:  0>n => n; 0<n => 0 */
      /* 0<mid && mid>n:  0>n => 0; 0<n => n */
      z = (c == c12) ? 0 : n - 1;
      t = p[z];
      p[z] = p[m];
      p[m] = t;
    }
    /* now p[m] is the median-of-three */
    /* swap it to the beginning so it won't move around */
    t = p[0];
    p[0] = p[m];
    p[m] = t;

    /* partition loop */
    i = 1;
    j = n - 1;
    for (;;) {
      /* handling of equality is crucial here */
      /* for sentinels & efficiency with duplicates */
      for (;; ++i) {
        if (!FNT__COMPARE(&p[i], &p[0])) break;
      }
      for (;; --j) {
        if (!FNT__COMPARE(&p[0], &p[j])) break;
      }
      /* make sure we haven't crossed */
      if (i >= j) break;
      t = p[i];
      p[i] = p[j];
      p[j] = t;

      ++i;
      --j;
    }
    /* recurse on smaller side, iterate on larger */
    if (j < (n - i)) {
      fnt__sort_edges_quicksort(p, j);
      p = p + i;
      n = n - i;
    } else {
      fnt__sort_edges_quicksort(p + i, n - i);
      n = j;
    }
  }
}
static void
fnt__sort_edges(struct fnt__edge *p, int n) {
  fnt__sort_edges_quicksort(p, n);
  fnt__sort_edges_ins_sort(p, n);
}

struct fnt__point {
  float x, y;
};
static void
fnt__rasterize(struct fnt__bitmap *result, struct fnt__point *pts, int *wcount,
               int windings, float scale_x, float scale_y, float shift_x,
               float shift_y, int off_x, int off_y, int invert,
               void *userdata) {
  float y_scale_inv = invert ? -scale_y : scale_y;
  struct fnt__edge *e;
  int n, i, j, k, m;
#if FNT_RASTERIZER_VERSION == 1
  int vsubsample = result->h < 8 ? 15 : 5;
#elif FNT_RASTERIZER_VERSION == 2
  int vsubsample = 1;
#else
#error "Unrecognized value of FNT_RASTERIZER_VERSION"
#endif
  // vsubsample should divide 255 evenly; otherwise we won't reach full opacity

  // now we have to blow out the windings into explicit edge lists
  n = 0;
  for (i = 0; i < windings; ++i) n += wcount[i];

  e = (struct fnt__edge *)FNT_malloc(
      sizeof(*e) * (n + 1), userdata);  // add an extra one as a sentinel
  if (e == 0) return;
  n = 0;

  m = 0;
  for (i = 0; i < windings; ++i) {
    struct fnt__point *p = pts + m;
    m += wcount[i];
    j = wcount[i] - 1;
    for (k = 0; k < wcount[i]; j = k++) {
      int a = k, b = j;
      // skip the edge if horizontal
      if (p[j].y == p[k].y) continue;
      // add edge from j to k to the list
      e[n].invert = 0;
      if (invert ? p[j].y > p[k].y : p[j].y < p[k].y) {
        e[n].invert = 1;
        a = j, b = k;
      }
      e[n].x0 = p[a].x * scale_x + shift_x;
      e[n].y0 = (p[a].y * y_scale_inv + shift_y) * vsubsample;
      e[n].x1 = p[b].x * scale_x + shift_x;
      e[n].y1 = (p[b].y * y_scale_inv + shift_y) * vsubsample;
      ++n;
    }
  }

  // now sort the edges by their highest point (should snap to integer, and then
  // by x)
  // FNT_sort(e, n, sizeof(e[0]), fnt__edge_compare);
  fnt__sort_edges(e, n);

  // now, traverse the scanlines and find the intersections on each scanline,
  // use xor winding rule
  fnt__rasterize_sorted_edges(result, e, n, vsubsample, off_x, off_y, userdata);

  FNT_free(e, userdata);
}
static void
fnt__add_point(struct fnt__point *points, int n, float x, float y) {
  if (!points) return;  // during first pass, it's unallocated
  points[n].x = x;
  points[n].y = y;
}
static int
fnt__tesselate_curve(struct fnt__point *points, int *num_points, float x0,
                     float y0, float x1, float y1, float x2, float y2,
                     float objspace_flatness_squared, int n) {
  // tesselate until threshhold p is happy... @TODO warped to compensate for
  // non-linear stretching
  // midpoint
  float mx = (x0 + 2 * x1 + x2) / 4;
  float my = (y0 + 2 * y1 + y2) / 4;
  // versus directly drawn line
  float dx = (x0 + x2) / 2 - mx;
  float dy = (y0 + y2) / 2 - my;
  if (n > 16)  // 65536 segments on one curve better be enough!
    return 1;
  if (dx * dx + dy * dy >
      objspace_flatness_squared) {  // half-pixel error allowed... need to be
                                    // smaller if AA
    fnt__tesselate_curve(points, num_points, x0, y0, (x0 + x1) / 2.0f,
                         (y0 + y1) / 2.0f, mx, my, objspace_flatness_squared,
                         n + 1);
    fnt__tesselate_curve(points, num_points, mx, my, (x1 + x2) / 2.0f,
                         (y1 + y2) / 2.0f, x2, y2, objspace_flatness_squared,
                         n + 1);
  } else {
    fnt__add_point(points, *num_points, x2, y2);
    *num_points = *num_points + 1;
  }
  return 1;
}
static void
fnt__tesselate_cubic(struct fnt__point *points, int *num_points, float x0,
                     float y0, float x1, float y1, float x2, float y2, float x3,
                     float y3, float objspace_flatness_squared, int n) {
  // @TODO this "flatness" calculation is just made-up nonsense that seems to
  // work well enough
  float dx0 = x1 - x0;
  float dy0 = y1 - y0;
  float dx1 = x2 - x1;
  float dy1 = y2 - y1;
  float dx2 = x3 - x2;
  float dy2 = y3 - y2;
  float dx = x3 - x0;
  float dy = y3 - y0;
  float longlen = (float)(FNT_sqrt(dx0 * dx0 + dy0 * dy0) +
                          FNT_sqrt(dx1 * dx1 + dy1 * dy1) +
                          FNT_sqrt(dx2 * dx2 + dy2 * dy2));
  float shortlen = (float)FNT_sqrt(dx * dx + dy * dy);
  float flatness_squared = longlen * longlen - shortlen * shortlen;

  if (n > 16)  // 65536 segments on one curve better be enough!
    return;

  if (flatness_squared > objspace_flatness_squared) {
    float x01 = (x0 + x1) / 2;
    float y01 = (y0 + y1) / 2;
    float x12 = (x1 + x2) / 2;
    float y12 = (y1 + y2) / 2;
    float x23 = (x2 + x3) / 2;
    float y23 = (y2 + y3) / 2;

    float xa = (x01 + x12) / 2;
    float ya = (y01 + y12) / 2;
    float xb = (x12 + x23) / 2;
    float yb = (y12 + y23) / 2;

    float mx = (xa + xb) / 2;
    float my = (ya + yb) / 2;

    fnt__tesselate_cubic(points, num_points, x0, y0, x01, y01, xa, ya, mx, my,
                         objspace_flatness_squared, n + 1);
    fnt__tesselate_cubic(points, num_points, mx, my, xb, yb, x23, y23, x3, y3,
                         objspace_flatness_squared, n + 1);
  } else {
    fnt__add_point(points, *num_points, x3, y3);
    *num_points = *num_points + 1;
  }
}
static struct fnt__point *
fnt_flatten_curves(struct fnt_vertex *vertices, int num_verts,
                   float objspace_flatness, int **contour_lengths,
                   int *num_contours, void *userdata) {
  // returns number of contours
  struct fnt__point *points = 0;
  int num_points = 0;

  float objspace_flatness_squared = objspace_flatness * objspace_flatness;
  int i, n = 0, start = 0, pass;

  // count how many "moves" there are to get the contour count
  for (i = 0; i < num_verts; ++i)
    if (vertices[i].type == FNT_vmove) ++n;

  *num_contours = n;
  if (n == 0) return 0;

  *contour_lengths = (int *)FNT_malloc(sizeof(**contour_lengths) * n, userdata);

  if (*contour_lengths == 0) {
    *num_contours = 0;
    return 0;
  }

  // make two passes through the points so we don't need to realloc
  for (pass = 0; pass < 2; ++pass) {
    float x = 0, y = 0;
    if (pass == 1) {
      points = (struct fnt__point *)FNT_malloc(num_points * sizeof(points[0]),
                                               userdata);
      if (points == NULL) goto error;
    }
    num_points = 0;
    n = -1;
    for (i = 0; i < num_verts; ++i) {
      switch (vertices[i].type) {
        case FNT_vmove:
          // start the next contour
          if (n >= 0) (*contour_lengths)[n] = num_points - start;
          ++n;
          start = num_points;

          x = vertices[i].x, y = vertices[i].y;
          fnt__add_point(points, num_points++, x, y);
          break;
        case FNT_vline:
          x = vertices[i].x, y = vertices[i].y;
          fnt__add_point(points, num_points++, x, y);
          break;
        case FNT_vcurve:
          fnt__tesselate_curve(points, &num_points, x, y, vertices[i].cx,
                               vertices[i].cy, vertices[i].x, vertices[i].y,
                               objspace_flatness_squared, 0);
          x = vertices[i].x, y = vertices[i].y;
          break;
        case FNT_vcubic:
          fnt__tesselate_cubic(points, &num_points, x, y, vertices[i].cx,
                               vertices[i].cy, vertices[i].cx1, vertices[i].cy1,
                               vertices[i].x, vertices[i].y,
                               objspace_flatness_squared, 0);
          x = vertices[i].x, y = vertices[i].y;
          break;
      }
    }
    (*contour_lengths)[n] = num_points - start;
  }

  return points;
error:
  FNT_free(points, userdata);
  FNT_free(*contour_lengths, userdata);
  *contour_lengths = 0;
  *num_contours = 0;
  return NULL;
}
static void
fnt_rasterize(struct fnt__bitmap *result, float flatness_in_pixels,
              struct fnt_vertex *vertices, int num_verts, float scale_x,
              float scale_y, float shift_x, float shift_y, int x_off, int y_off,
              int invert, void *userdata) {
  float scale = scale_x > scale_y ? scale_y : scale_x;
  int winding_count = 0;
  int *winding_lengths = NULL;
  struct fnt__point *windings =
      fnt_flatten_curves(vertices, num_verts, flatness_in_pixels / scale,
                         &winding_lengths, &winding_count, userdata);
  if (windings) {
    fnt__rasterize(result, windings, winding_lengths, winding_count, scale_x,
                   scale_y, shift_x, shift_y, x_off, y_off, invert, userdata);
    FNT_free(winding_lengths, userdata);
    FNT_free(windings, userdata);
  }
}
#if 0
static void
fnt_free_bitmap(unsigned char *bitmap, void *userdata) {
  FNT_free(bitmap, userdata);
}
static unsigned char *
fnt_get_glyph_bitmap_subpixel(const struct fnt_info *info, float scale_x,
                              float scale_y, float shift_x, float shift_y,
                              int glyph, int *width, int *height, int *xoff,
                              int *yoff) {
  int ix0, iy0, ix1, iy1;
  struct fnt__bitmap gbm;
  struct fnt_vertex *vertices;
  int num_verts = fnt_get_glyph_shape(info, glyph, &vertices);

  if (scale_x == 0) scale_x = scale_y;
  if (scale_y == 0) {
    if (scale_x == 0) {
      FNT_free(vertices, info->userdata);
      return NULL;
    }
    scale_y = scale_x;
  }

  fnt_get_glyph_bitmap_box_subpixel(info, glyph, scale_x, scale_y, shift_x,
                                    shift_y, &ix0, &iy0, &ix1, &iy1);

  // now we get the size
  gbm.w = (ix1 - ix0);
  gbm.h = (iy1 - iy0);
  gbm.pixels = NULL;  // in case we error

  if (width) *width = gbm.w;
  if (height) *height = gbm.h;
  if (xoff) *xoff = ix0;
  if (yoff) *yoff = iy0;

  if (gbm.w && gbm.h) {
    gbm.pixels = (unsigned char *)FNT_malloc(gbm.w * gbm.h, info->userdata);
    if (gbm.pixels) {
      gbm.stride = gbm.w;
      fnt_rasterize(&gbm, 0.35f, vertices, num_verts, scale_x, scale_y, shift_x,
                    shift_y, ix0, iy0, 1, info->userdata);
    }
  }
  FNT_free(vertices, info->userdata);
  return gbm.pixels;
}
static unsigned char *
fnt_get_glyph_bitmap(const struct fnt_info *info, float scale_x, float scale_y,
                     int glyph, int *width, int *height, int *xoff, int *yoff) {
  return fnt_get_glyph_bitmap_subpixel(info, scale_x, scale_y, 0.0f, 0.0f,
                                       glyph, width, height, xoff, yoff);
}
#endif

static void
fnt_make_glyph_bitmap_subpixel(const struct fnt_info *info,
                               unsigned char *output, int out_w, int out_h,
                               int out_stride, float scale_x, float scale_y,
                               float shift_x, float shift_y, int glyph) {
  int ix0, iy0;
  struct fnt_vertex *vertices;
  int num_verts = fnt_get_glyph_shape(info, glyph, &vertices);
  struct fnt__bitmap gbm;

  fnt_get_glyph_bitmap_box_subpixel(info, glyph, scale_x, scale_y, shift_x,
                                    shift_y, &ix0, &iy0, 0, 0);
  gbm.pixels = output;
  gbm.w = out_w;
  gbm.h = out_h;
  gbm.stride = out_stride;
  if (gbm.w && gbm.h)
    fnt_rasterize(&gbm, 0.35f, vertices, num_verts, scale_x, scale_y, shift_x,
                  shift_y, ix0, iy0, 1, info->userdata);
  FNT_free(vertices, info->userdata);
}
static void
fnt_make_glyph_bitmap(const struct fnt_info *info, unsigned char *output,
                      int out_w, int out_h, int out_stride, float scale_x,
                      float scale_y, int glyph) {
  fnt_make_glyph_bitmap_subpixel(info, output, out_w, out_h, out_stride,
                                 scale_x, scale_y, 0.0f, 0.0f, glyph);
}
#if 0
static unsigned char *
fnt_get_codepoint_bitmap_subpixel(const struct fnt_info *info, float scale_x,
                                  float scale_y, float shift_x, float shift_y,
                                  int codepoint, int *width, int *height,
                                  int *xoff, int *yoff) {
  return fnt_get_glyph_bitmap_subpixel(info, scale_x, scale_y, shift_x, shift_y,
                                       fnt_fnd_glyph_index(info, codepoint),
                                       width, height, xoff, yoff);
}
static void
fnt_make_codepoint_bitmap_subpixel_prefilter(
    const struct fnt_info *info, unsigned char *output, int out_w, int out_h,
    int out_stride, float scale_x, float scale_y, float shift_x, float shift_y,
    int oversample_x, int oversample_y, float *sub_x, float *sub_y,
    int codepoint) {
  fnt_make_glyph_bitmap_subpixel_prefilter(
      info, output, out_w, out_h, out_stride, scale_x, scale_y, shift_x,
      shift_y, oversample_x, oversample_y, sub_x, sub_y,
      fnt_fnd_glyph_index(info, codepoint));
}
static void
fnt_make_codepoint_bitmap_subpixel(const struct fnt_info *info,
                                   unsigned char *output, int out_w, int out_h,
                                   int out_stride, float scale_x, float scale_y,
                                   float shift_x, float shift_y,
                                   int codepoint) {
  fnt_make_glyph_bitmap_subpixel(info, output, out_w, out_h, out_stride,
                                 scale_x, scale_y, shift_x, shift_y,
                                 fnt_fnd_glyph_index(info, codepoint));
}
static unsigned char *
fnt_get_codepoint_bitmap(const struct fnt_info *info, float scale_x,
                         float scale_y, int codepoint, int *width, int *height,
                         int *xoff, int *yoff) {
  return fnt_get_codepoint_bitmap_subpixel(
      info, scale_x, scale_y, 0.0f, 0.0f, codepoint, width, height, xoff, yoff);
}
static void
fnt_make_codepoint_bitmap(const struct fnt_info *info, unsigned char *output,
                          int out_w, int out_h, int out_stride, float scale_x,
                          float scale_y, int codepoint) {
  fnt_make_codepoint_bitmap_subpixel(info, output, out_w, out_h, out_stride,
                                     scale_x, scale_y, 0.0f, 0.0f, codepoint);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// bitmap baking
//
// This is SUPER-CRAPPY packing to keep source code small
static int
fnt_bake_bitmap_internal(
    unsigned char *data,
    int offset,          // font location (use offset=0 for plain .ttf)
    float pixel_height,  // height of font in pixels
    unsigned char *pixels, int pw, int ph,  // bitmap to be filled in
    int first_char, int num_chars,          // characters to bake
    struct fnt_baked_char *chardata) {
  float scale;
  int x, y, bottom_y, i;
  struct fnt_info f;
  f.userdata = NULL;
  if (!fnt_init(&f, data, offset)) return -1;
  FNT_memset(pixels, 0, pw * ph);  // background of 0 around pixels
  x = y = 1;
  bottom_y = 1;

  scale = fnt_scale_for_pixel_height(&f, pixel_height);
  for (i = 0; i < num_chars; ++i) {
    int advance, lsb, x0, y0, x1, y1, gw, gh;
    int g = fnt_fnd_glyph_index(&f, first_char + i);
    fnt_get_glyph_hmetrics(&f, g, &advance, &lsb);
    fnt_get_glyph_bitmap_box(&f, g, scale, scale, &x0, &y0, &x1, &y1);
    gw = x1 - x0;
    gh = y1 - y0;
    if (x + gw + 1 >= pw) y = bottom_y, x = 1;  // advance to next row
    // check if it fits vertically AFTER potentially moving to next row
    if (y + gh + 1 >= ph) return -i;
    FNT_assert(x + gw < pw);
    FNT_assert(y + gh < ph);
    fnt_make_glyph_bitmap(&f, pixels + x + y * pw, gw, gh, pw, scale, scale, g);
    chardata[i].x0 = (fnt_int16)x;
    chardata[i].y0 = (fnt_int16)y;
    chardata[i].x1 = (fnt_int16)(x + gw);
    chardata[i].y1 = (fnt_int16)(y + gh);
    chardata[i].xadvance = scale * advance;
    chardata[i].xoff = (float)x0;
    chardata[i].yoff = (float)y0;
    x = x + gw + 1;
    if (y + gh + 1 > bottom_y) bottom_y = y + gh + 1;
  }
  return bottom_y;
}
#if 0
static void
fnt_get_baked_quad(const struct fnt_baked_char *chardata, int pw, int ph,
                   int char_index, float *xpos, float *ypos,
                   struct fnt_aligned_quad *q, int opengl_fillrule) {
  float d3d_bias = opengl_fillrule ? 0 : -0.5f;
  float ipw = 1.0f / pw, iph = 1.0f / ph;
  const struct fnt_baked_char *b = chardata + char_index;
  int round_x = FNT_ifloor((*xpos + b->xoff) + 0.5f);
  int round_y = FNT_ifloor((*ypos + b->yoff) + 0.5f);

  q->x0 = round_x + d3d_bias;
  q->y0 = round_y + d3d_bias;
  q->x1 = round_x + b->x1 - b->x0 + d3d_bias;
  q->y1 = round_y + b->y1 - b->y0 + d3d_bias;

  q->s0 = b->x0 * ipw;
  q->t0 = b->y0 * iph;
  q->s1 = b->x1 * ipw;
  q->t1 = b->y1 * iph;

  *xpos += b->xadvance;
}
#endif

////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
//                                                                                //
// COMPILER WARNING ?!?!? //
//                                                                                //
//                                                                                //
// if you get a compile warning due to these symbols being defined more than //
// once, move #include "stb_rect_pack.h" before #include "stb_truetype.h" //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////
#if 0
#ifndef STB_RECT_PACK_VERSION // rectangle packing replacement routines if you don't have stb_rect_pack.h

typedef int stbrp_coord;


struct stbrp_context {
  int width, height;
  int x, y, bottom_y;
};
struct stbrp_node {
  unsigned char x;
};
struct stbrp_rect {
  stbrp_coord x, y;
  int id, w, h, was_packed;
};
static void
stbrp_init_target(struct stbrp_context *con, int pw, int ph,
                  struct stbrp_node *nodes, int num_nodes) {
  con->width = pw;
  con->height = ph;
  con->x = 0;
  con->y = 0;
  con->bottom_y = 0;
  FNT__NOTUSED(nodes);
  FNT__NOTUSED(num_nodes);
}
static void
stbrp_pack_rects(struct stbrp_context *con, struct stbrp_rect *rects,
                 int num_rects) {
  int i;
  for (i = 0; i < num_rects; ++i) {
    if (con->x + rects[i].w > con->width) {
      con->x = 0;
      con->y = con->bottom_y;
    }
    if (con->y + rects[i].h > con->height) break;
    rects[i].x = con->x;
    rects[i].y = con->y;
    rects[i].was_packed = 1;
    con->x += rects[i].w;
    if (con->y + rects[i].h > con->bottom_y)
      con->bottom_y = con->y + rects[i].h;
  }
  for (; i < num_rects; ++i) rects[i].was_packed = 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// bitmap baking
//
// This is SUPER-AWESOME (tm Ryan Gordon) packing using stb_rect_pack.h. If
// stb_rect_pack.h isn't available, it uses the BakeFontBitmap strategy.
static int
fnt_pack_begin(struct fnt_pack_ctx *spc, unsigned char *pixels, int pw, int ph,
               int stride_in_bytes, int padding, void *alloc_context) {
  struct stbrp_context *context =
      (struct stbrp_context *)FNT_malloc(sizeof(*context), alloc_context);
  int num_nodes = pw - padding;
  struct stbrp_node *nodes = (struct stbrp_node *)FNT_malloc(
      sizeof(*nodes) * num_nodes, alloc_context);
  if (context == NULL || nodes == NULL) {
    if (context != NULL) FNT_free(context, alloc_context);
    if (nodes != NULL) FNT_free(nodes, alloc_context);
    return 0;
  }

  spc->user_allocator_context = alloc_context;
  spc->width = pw;
  spc->height = ph;
  spc->pixels = pixels;
  spc->pack_info = context;
  spc->nodes = nodes;
  spc->padding = padding;
  spc->stride_in_bytes = stride_in_bytes != 0 ? stride_in_bytes : pw;
  spc->h_oversample = 1;
  spc->v_oversample = 1;

  stbrp_init_target(context, pw - padding, ph - padding, nodes, num_nodes);

  if (pixels) FNT_memset(pixels, 0, pw * ph);  // background of 0 around pixels

  return 1;
}
static void
fnt_pack_end(struct fnt_pack_ctx *spc) {
  FNT_free(spc->nodes, spc->user_allocator_context);
  FNT_free(spc->pack_info, spc->user_allocator_context);
}
static void
fnt_pack_set_oversampling(struct fnt_pack_ctx *spc, unsigned int h_oversample,
                          unsigned int v_oversample) {
  FNT_assert(h_oversample <= FNT_MAX_OVERSAMPLE);
  FNT_assert(v_oversample <= FNT_MAX_OVERSAMPLE);
  if (h_oversample <= FNT_MAX_OVERSAMPLE) spc->h_oversample = h_oversample;
  if (v_oversample <= FNT_MAX_OVERSAMPLE) spc->v_oversample = v_oversample;
}
#define FNT__OVER_MASK (FNT_MAX_OVERSAMPLE - 1)
static void
fnt__h_prefilter(unsigned char *pixels, int w, int h, int stride_in_bytes,
                 unsigned int kernel_width) {
  unsigned char buffer[FNT_MAX_OVERSAMPLE];
  int safe_w = w - kernel_width;
  int j;
  FNT_memset(
      buffer, 0,
      FNT_MAX_OVERSAMPLE);  // suppress bogus warning from VS2013 -analyze
  for (j = 0; j < h; ++j) {
    int i;
    unsigned int total;
    FNT_memset(buffer, 0, kernel_width);

    total = 0;

    // make kernel_width a constant in common cases so compiler can optimize out
    // the divide
    switch (kernel_width) {
      case 2:
        for (i = 0; i <= safe_w; ++i) {
          total += pixels[i] - buffer[i & FNT__OVER_MASK];
          buffer[(i + kernel_width) & FNT__OVER_MASK] = pixels[i];
          pixels[i] = (unsigned char)(total / 2);
        }
        break;
      case 3:
        for (i = 0; i <= safe_w; ++i) {
          total += pixels[i] - buffer[i & FNT__OVER_MASK];
          buffer[(i + kernel_width) & FNT__OVER_MASK] = pixels[i];
          pixels[i] = (unsigned char)(total / 3);
        }
        break;
      case 4:
        for (i = 0; i <= safe_w; ++i) {
          total += pixels[i] - buffer[i & FNT__OVER_MASK];
          buffer[(i + kernel_width) & FNT__OVER_MASK] = pixels[i];
          pixels[i] = (unsigned char)(total / 4);
        }
        break;
      case 5:
        for (i = 0; i <= safe_w; ++i) {
          total += pixels[i] - buffer[i & FNT__OVER_MASK];
          buffer[(i + kernel_width) & FNT__OVER_MASK] = pixels[i];
          pixels[i] = (unsigned char)(total / 5);
        }
        break;
      default:
        for (i = 0; i <= safe_w; ++i) {
          total += pixels[i] - buffer[i & FNT__OVER_MASK];
          buffer[(i + kernel_width) & FNT__OVER_MASK] = pixels[i];
          pixels[i] = (unsigned char)(total / kernel_width);
        }
        break;
    }

    for (; i < w; ++i) {
      FNT_assert(pixels[i] == 0);
      total -= buffer[i & FNT__OVER_MASK];
      pixels[i] = (unsigned char)(total / kernel_width);
    }

    pixels += stride_in_bytes;
  }
}
static void
fnt__v_prefilter(unsigned char *pixels, int w, int h, int stride_in_bytes,
                 unsigned int kernel_width) {
  unsigned char buffer[FNT_MAX_OVERSAMPLE];
  int safe_h = h - kernel_width;
  int j;
  FNT_memset(
      buffer, 0,
      FNT_MAX_OVERSAMPLE);  // suppress bogus warning from VS2013 -analyze
  for (j = 0; j < w; ++j) {
    int i;
    unsigned int total;
    FNT_memset(buffer, 0, kernel_width);

    total = 0;

    // make kernel_width a constant in common cases so compiler can optimize out
    // the divide
    switch (kernel_width) {
      case 2:
        for (i = 0; i <= safe_h; ++i) {
          total += pixels[i * stride_in_bytes] - buffer[i & FNT__OVER_MASK];
          buffer[(i + kernel_width) & FNT__OVER_MASK] =
              pixels[i * stride_in_bytes];
          pixels[i * stride_in_bytes] = (unsigned char)(total / 2);
        }
        break;
      case 3:
        for (i = 0; i <= safe_h; ++i) {
          total += pixels[i * stride_in_bytes] - buffer[i & FNT__OVER_MASK];
          buffer[(i + kernel_width) & FNT__OVER_MASK] =
              pixels[i * stride_in_bytes];
          pixels[i * stride_in_bytes] = (unsigned char)(total / 3);
        }
        break;
      case 4:
        for (i = 0; i <= safe_h; ++i) {
          total += pixels[i * stride_in_bytes] - buffer[i & FNT__OVER_MASK];
          buffer[(i + kernel_width) & FNT__OVER_MASK] =
              pixels[i * stride_in_bytes];
          pixels[i * stride_in_bytes] = (unsigned char)(total / 4);
        }
        break;
      case 5:
        for (i = 0; i <= safe_h; ++i) {
          total += pixels[i * stride_in_bytes] - buffer[i & FNT__OVER_MASK];
          buffer[(i + kernel_width) & FNT__OVER_MASK] =
              pixels[i * stride_in_bytes];
          pixels[i * stride_in_bytes] = (unsigned char)(total / 5);
        }
        break;
      default:
        for (i = 0; i <= safe_h; ++i) {
          total += pixels[i * stride_in_bytes] - buffer[i & FNT__OVER_MASK];
          buffer[(i + kernel_width) & FNT__OVER_MASK] =
              pixels[i * stride_in_bytes];
          pixels[i * stride_in_bytes] = (unsigned char)(total / kernel_width);
        }
        break;
    }

    for (; i < h; ++i) {
      FNT_assert(pixels[i * stride_in_bytes] == 0);
      total -= buffer[i & FNT__OVER_MASK];
      pixels[i * stride_in_bytes] = (unsigned char)(total / kernel_width);
    }

    pixels += 1;
  }
}
static float
fnt__oversample_shift(int oversample) {
  if (!oversample) return 0.0f;

  // The prefilter is a box filter of width "oversample",
  // which shifts phase by (oversample - 1)/2 pixels in
  // oversampled space. We want to shift in the opposite
  // direction to counter this.
  return (float)-(oversample - 1) / (2.0f * (float)oversample);
}

// rects array must be big enough to accommodate all characters in the given
// ranges
static int
fnt_pack_ranges_gather_rects(struct fnt_pack_ctx *spc,
                             const struct fnt_info *info,
                             struct fnt_pack_range *ranges, int num_ranges,
                             struct stbrp_rect *rects) {
  int i, j, k;

  k = 0;
  for (i = 0; i < num_ranges; ++i) {
    float fh = ranges[i].font_size;
    float scale = fh > 0 ? fnt_scale_for_pixel_height(info, fh)
                         : fnt_scale_for_mapping_em_to_pixels(info, -fh);
    ranges[i].h_oversample = (unsigned char)spc->h_oversample;
    ranges[i].v_oversample = (unsigned char)spc->v_oversample;
    for (j = 0; j < ranges[i].num_chars; ++j) {
      int x0, y0, x1, y1;
      int codepoint = ranges[i].array_of_unicode_codepoints == NULL
                          ? ranges[i].first_unicode_codepoint_in_range + j
                          : ranges[i].array_of_unicode_codepoints[j];
      int glyph = fnt_fnd_glyph_index(info, codepoint);
      fnt_get_glyph_bitmap_box_subpixel(info, glyph, scale * spc->h_oversample,
                                        scale * spc->v_oversample, 0, 0, &x0,
                                        &y0, &x1, &y1);
      rects[k].w =
          (stbrp_coord)(x1 - x0 + spc->padding + spc->h_oversample - 1);
      rects[k].h =
          (stbrp_coord)(y1 - y0 + spc->padding + spc->v_oversample - 1);
      ++k;
    }
  }

  return k;
}
static void
fnt_make_glyph_bitmap_subpixel_prefilter(
    const struct fnt_info *info, unsigned char *output, int out_w, int out_h,
    int out_stride, float scale_x, float scale_y, float shift_x, float shift_y,
    int prefilter_x, int prefilter_y, float *sub_x, float *sub_y, int glyph) {
  fnt_make_glyph_bitmap_subpixel(info, output, out_w - (prefilter_x - 1),
                                 out_h - (prefilter_y - 1), out_stride, scale_x,
                                 scale_y, shift_x, shift_y, glyph);
  if (prefilter_x > 1)
    fnt__h_prefilter(output, out_w, out_h, out_stride, prefilter_x);
  if (prefilter_y > 1)
    fnt__v_prefilter(output, out_w, out_h, out_stride, prefilter_y);

  *sub_x = fnt__oversample_shift(prefilter_x);
  *sub_y = fnt__oversample_shift(prefilter_y);
}
static int
fnt_pack_ranges_render_into_rects(struct fnt_pack_ctx *spc,
                                  const struct fnt_info *info,
                                  struct fnt_pack_range *ranges, int num_ranges,
                                  struct stbrp_rect *rects) {
  // rects array must be big enough to accommodate all characters in the given
  // ranges
  int i, j, k, return_value = 1;

  // save current values
  int old_h_over = spc->h_oversample;
  int old_v_over = spc->v_oversample;

  k = 0;
  for (i = 0; i < num_ranges; ++i) {
    float fh = ranges[i].font_size;
    float scale = fh > 0 ? fnt_scale_for_pixel_height(info, fh)
                         : fnt_scale_for_mapping_em_to_pixels(info, -fh);
    float recip_h, recip_v, sub_x, sub_y;
    spc->h_oversample = ranges[i].h_oversample;
    spc->v_oversample = ranges[i].v_oversample;
    recip_h = 1.0f / spc->h_oversample;
    recip_v = 1.0f / spc->v_oversample;
    sub_x = fnt__oversample_shift(spc->h_oversample);
    sub_y = fnt__oversample_shift(spc->v_oversample);
    for (j = 0; j < ranges[i].num_chars; ++j) {
      struct stbrp_rect *r = &rects[k];
      if (r->was_packed) {
        struct fnt_packed_char *bc = &ranges[i].chardata_for_range[j];
        int advance, lsb, x0, y0, x1, y1;
        int codepoint = ranges[i].array_of_unicode_codepoints == NULL
                            ? ranges[i].first_unicode_codepoint_in_range + j
                            : ranges[i].array_of_unicode_codepoints[j];
        int glyph = fnt_fnd_glyph_index(info, codepoint);
        stbrp_coord pad = (stbrp_coord)spc->padding;

        // pad on left and top
        r->x += pad;
        r->y += pad;
        r->w -= pad;
        r->h -= pad;

        fnt_get_glyph_hmetrics(info, glyph, &advance, &lsb);
        fnt_get_glyph_bitmap_box(info, glyph, scale * spc->h_oversample,
                                 scale * spc->v_oversample, &x0, &y0, &x1, &y1);
        fnt_make_glyph_bitmap_subpixel(
            info, spc->pixels + r->x + r->y * spc->stride_in_bytes,
            r->w - spc->h_oversample + 1, r->h - spc->v_oversample + 1,
            spc->stride_in_bytes, scale * spc->h_oversample,
            scale * spc->v_oversample, 0, 0, glyph);

        if (spc->h_oversample > 1)
          fnt__h_prefilter(spc->pixels + r->x + r->y * spc->stride_in_bytes,
                           r->w, r->h, spc->stride_in_bytes, spc->h_oversample);

        if (spc->v_oversample > 1)
          fnt__v_prefilter(spc->pixels + r->x + r->y * spc->stride_in_bytes,
                           r->w, r->h, spc->stride_in_bytes, spc->v_oversample);

        bc->x0 = (fnt_int16)r->x;
        bc->y0 = (fnt_int16)r->y;
        bc->x1 = (fnt_int16)(r->x + r->w);
        bc->y1 = (fnt_int16)(r->y + r->h);
        bc->xadvance = scale * advance;
        bc->xoff = (float)x0 * recip_h + sub_x;
        bc->yoff = (float)y0 * recip_v + sub_y;
        bc->xoff2 = (x0 + r->w) * recip_h + sub_x;
        bc->yoff2 = (y0 + r->h) * recip_v + sub_y;
      } else {
        return_value = 0;  // if any fail, report failure
      }
      ++k;
    }
  }

  // restore original values
  spc->h_oversample = old_h_over;
  spc->v_oversample = old_v_over;
  return return_value;
}
static void
fnt_pack_ranges_pack_rects(struct fnt_pack_ctx *spc, struct stbrp_rect *rects,
                           int num_rects) {
  stbrp_pack_rects((struct stbrp_context *)spc->pack_info, rects, num_rects);
}
static int
fnt_pack_ranges(struct fnt_pack_ctx *spc, const unsigned char *fontdata,
                int font_index, struct fnt_pack_range *ranges, int num_ranges) {
  struct fnt_info info;
  int i, j, n, return_value = 1;
  // stbrp_context *context = (stbrp_context *) spc->pack_info;
  struct stbrp_rect *rects;

  // flag all characters as NOT packed
  for (i = 0; i < num_ranges; ++i)
    for (j = 0; j < ranges[i].num_chars; ++j)
      ranges[i].chardata_for_range[j].x0 = ranges[i].chardata_for_range[j].y0 =
          ranges[i].chardata_for_range[j].x1 =
              ranges[i].chardata_for_range[j].y1 = 0;

  n = 0;
  for (i = 0; i < num_ranges; ++i) n += ranges[i].num_chars;

  rects = (struct stbrp_rect *)FNT_malloc(sizeof(*rects) * n,
                                          spc->user_allocator_context);
  if (rects == NULL) return 0;

  info.userdata = spc->user_allocator_context;
  fnt_init(&info, fontdata,
           fnt_get_font_offset_for_index(fontdata, font_index));

  n = fnt_pack_ranges_gather_rects(spc, &info, ranges, num_ranges, rects);

  fnt_pack_ranges_pack_rects(spc, rects, n);

  return_value =
      fnt_pack_ranges_render_into_rects(spc, &info, ranges, num_ranges, rects);
  FNT_free(rects, spc->user_allocator_context);
  return return_value;
}
static int
fnt_pack_range(struct fnt_pack_ctx *spc, const unsigned char *fontdata,
               int font_index, float font_size,
               int first_unicode_codepoint_in_range, int num_chars_in_range,
               struct fnt_packed_char *chardata_for_range) {
  struct fnt_pack_range range;
  range.first_unicode_codepoint_in_range = first_unicode_codepoint_in_range;
  range.array_of_unicode_codepoints = NULL;
  range.num_chars = num_chars_in_range;
  range.chardata_for_range = chardata_for_range;
  range.font_size = font_size;
  return fnt_pack_ranges(spc, fontdata, font_index, &range, 1);
}
static void
fnt_get_packed_quad(const struct fnt_packed_char *chardata, int pw, int ph,
                    int char_index, float *xpos, float *ypos,
                    struct fnt_aligned_quad *q, int align_to_integer) {
  float ipw = 1.0f / pw, iph = 1.0f / ph;
  const struct fnt_packed_char *b = chardata + char_index;

  if (align_to_integer) {
    int xi = FNT_ifloor((*xpos + b->xoff) + 0.5f);
    int yi = FNT_ifloor((*ypos + b->yoff) + 0.5f);

    float x = (float)xi;
    float y = (float)yi;

    q->x0 = x;
    q->y0 = y;
    q->x1 = x + b->xoff2 - b->xoff;
    q->y1 = y + b->yoff2 - b->yoff;
  } else {
    q->x0 = *xpos + b->xoff;
    q->y0 = *ypos + b->yoff;
    q->x1 = *xpos + b->xoff2;
    q->y1 = *ypos + b->yoff2;
  }

  q->s0 = b->x0 * ipw;
  q->t0 = b->y0 * iph;
  q->s1 = b->x1 * ipw;
  q->t1 = b->y1 * iph;

  *xpos += b->xadvance;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// sdf computation
//
#if 0
#define FNT_min(a, b) ((a) < (b) ? (a) : (b))
#define FNT_max(a, b) ((a) < (b) ? (b) : (a))

static int
fnt__ray_intersect_bezier(float orig[2], float ray[2], float q0[2], float q1[2],
                          float q2[2], float hits[2][2]) {
  float q0perp = q0[1] * ray[0] - q0[0] * ray[1];
  float q1perp = q1[1] * ray[0] - q1[0] * ray[1];
  float q2perp = q2[1] * ray[0] - q2[0] * ray[1];
  float roperp = orig[1] * ray[0] - orig[0] * ray[1];

  float a = q0perp - 2 * q1perp + q2perp;
  float b = q1perp - q0perp;
  float c = q0perp - roperp;

  float s0 = 0., s1 = 0.;
  int num_s = 0;

  if (a != 0.0) {
    float discr = b * b - a * c;
    if (discr > 0.0) {
      float rcpna = -1 / a;
      float d = (float)FNT_sqrt(discr);
      s0 = (b + d) * rcpna;
      s1 = (b - d) * rcpna;
      if (s0 >= 0.0 && s0 <= 1.0) num_s = 1;
      if (d > 0.0 && s1 >= 0.0 && s1 <= 1.0) {
        if (num_s == 0) s0 = s1;
        ++num_s;
      }
    }
  } else {
    // 2*b*s + c = 0
    // s = -c / (2*b)
    s0 = c / (-2 * b);
    if (s0 >= 0.0 && s0 <= 1.0) num_s = 1;
  }

  if (num_s == 0)
    return 0;
  else {
    float rcp_len2 = 1 / (ray[0] * ray[0] + ray[1] * ray[1]);
    float rayn_x = ray[0] * rcp_len2, rayn_y = ray[1] * rcp_len2;

    float q0d = q0[0] * rayn_x + q0[1] * rayn_y;
    float q1d = q1[0] * rayn_x + q1[1] * rayn_y;
    float q2d = q2[0] * rayn_x + q2[1] * rayn_y;
    float rod = orig[0] * rayn_x + orig[1] * rayn_y;

    float q10d = q1d - q0d;
    float q20d = q2d - q0d;
    float q0rd = q0d - rod;

    hits[0][0] = q0rd + s0 * (2.0f - 2.0f * s0) * q10d + s0 * s0 * q20d;
    hits[0][1] = a * s0 + b;

    if (num_s > 1) {
      hits[1][0] = q0rd + s1 * (2.0f - 2.0f * s1) * q10d + s1 * s1 * q20d;
      hits[1][1] = a * s1 + b;
      return 2;
    } else {
      return 1;
    }
  }
}
static int
fnt__equal(float *a, float *b) {
  return (a[0] == b[0] && a[1] == b[1]);
}
static int
fnt__compute_crossings_x(float x, float y, int nverts,
                         struct fnt_vertex *verts) {
  int i;
  float orig[2], ray[2] = {1, 0};
  float y_frac;
  int winding = 0;

  orig[0] = x;
  orig[1] = y;

  // make sure y never passes through a vertex of the shape
  y_frac = (float)FNT_fmod(y, 1.0f);
  if (y_frac < 0.01f)
    y += 0.01f;
  else if (y_frac > 0.99f)
    y -= 0.01f;
  orig[1] = y;

  // test a ray from (-infinity,y) to (x,y)
  for (i = 0; i < nverts; ++i) {
    if (verts[i].type == FNT_vline) {
      int x0 = (int)verts[i - 1].x, y0 = (int)verts[i - 1].y;
      int x1 = (int)verts[i].x, y1 = (int)verts[i].y;
      if (y > FNT_min(y0, y1) && y < FNT_max(y0, y1) && x > FNT_min(x0, x1)) {
        float x_inter = (y - y0) / (y1 - y0) * (x1 - x0) + x0;
        if (x_inter < x) winding += (y0 < y1) ? 1 : -1;
      }
    }
    if (verts[i].type == FNT_vcurve) {
      int x0 = (int)verts[i - 1].x, y0 = (int)verts[i - 1].y;
      int x1 = (int)verts[i].cx, y1 = (int)verts[i].cy;
      int x2 = (int)verts[i].x, y2 = (int)verts[i].y;
      int ax = FNT_min(x0, FNT_min(x1, x2)), ay = FNT_min(y0, FNT_min(y1, y2));
      int by = FNT_max(y0, FNT_max(y1, y2));
      if (y > ay && y < by && x > ax) {
        float q0[2], q1[2], q2[2];
        float hits[2][2];
        q0[0] = (float)x0;
        q0[1] = (float)y0;
        q1[0] = (float)x1;
        q1[1] = (float)y1;
        q2[0] = (float)x2;
        q2[1] = (float)y2;
        if (fnt__equal(q0, q1) || fnt__equal(q1, q2)) {
          x0 = (int)verts[i - 1].x;
          y0 = (int)verts[i - 1].y;
          x1 = (int)verts[i].x;
          y1 = (int)verts[i].y;
          if (y > FNT_min(y0, y1) && y < FNT_max(y0, y1) &&
              x > FNT_min(x0, x1)) {
            float x_inter = (y - y0) / (y1 - y0) * (x1 - x0) + x0;
            if (x_inter < x) winding += (y0 < y1) ? 1 : -1;
          }
        } else {
          int num_hits = fnt__ray_intersect_bezier(orig, ray, q0, q1, q2, hits);
          if (num_hits >= 1)
            if (hits[0][0] < 0) winding += (hits[0][1] < 0 ? -1 : 1);
          if (num_hits >= 2)
            if (hits[1][0] < 0) winding += (hits[1][1] < 0 ? -1 : 1);
        }
      }
    }
  }
  return winding;
}
static float
fnt__cuberoot(float x) {
  if (x < 0)
    return -(float)FNT_pow(-x, 1.0f / 3.0f);
  else
    return (float)FNT_pow(x, 1.0f / 3.0f);
}
static int
fnt__solve_cubic(float a, float b, float c, float *r) {
  // x^3 + c*x^2 + b*x + a = 0
  float s = -a / 3;
  float p = b - a * a / 3;
  float q = a * (2 * a * a - 9 * b) / 27 + c;
  float p3 = p * p * p;
  float d = q * q + 4 * p3 / 27;
  if (d >= 0) {
    float z = (float)FNT_sqrt(d);
    float u = (-q + z) / 2;
    float v = (-q - z) / 2;
    u = fnt__cuberoot(u);
    v = fnt__cuberoot(v);
    r[0] = s + u + v;
    return 1;
  } else {
    float u = (float)FNT_sqrt(-p / 3);
    // p3 must be negative, since d is negative
    float v = (float)FNT_acos(-FNT_sqrt(-27 / p3) * q / 2) /3;
    float m = (float)FNT_cos(v);
    float n = (float)FNT_cos(v - 3.141592f / 2.0f) * 1.732050808f;
    r[0] = s + u * 2 * m;
    r[1] = s - u * (m + n);
    r[2] = s - u * (m - n);

    // FNT_assert( FNT_fabs(((r[0]+a)*r[0]+b)*r[0]+c) < 0.05f);  // these
    // asserts may not be safe at all scales, though they're in bezier t
    // parameter units so maybe? FNT_assert( FNT_fabs(((r[1]+a)*r[1]+b)*r[1]+c)
    // < 0.05f); FNT_assert( FNT_fabs(((r[2]+a)*r[2]+b)*r[2]+c) < 0.05f);
    return 3;
  }
}
static unsigned char *
fnt_get_glyph_sdf(const struct fnt_info *info, float scale, int glyph,
                  int padding, unsigned char onedge_value,
                  float pixel_dist_scale, int *width, int *height, int *xoff,
                  int *yoff) {
  float scale_x = scale, scale_y = scale;
  int ix0, iy0, ix1, iy1;
  int w, h;
  unsigned char *data;

  // if one scale is 0, use same scale for both
  if (scale_x == 0) scale_x = scale_y;
  if (scale_y == 0) {
    if (scale_x == 0) return NULL;  // if both scales are 0, return NULL
    scale_y = scale_x;
  }

  fnt_get_glyph_bitmap_box_subpixel(info, glyph, scale, scale, 0.0f, 0.0f, &ix0,
                                    &iy0, &ix1, &iy1);

  // if empty, return NULL
  if (ix0 == ix1 || iy0 == iy1) return NULL;

  ix0 -= padding;
  iy0 -= padding;
  ix1 += padding;
  iy1 += padding;

  w = (ix1 - ix0);
  h = (iy1 - iy0);

  if (width) *width = w;
  if (height) *height = h;
  if (xoff) *xoff = ix0;
  if (yoff) *yoff = iy0;

  // invert for y-downwards bitmaps
  scale_y = -scale_y;
  {
    int x, y, i, j;
    float *precompute;
    struct fnt_vertex *verts;
    int num_verts = fnt_get_glyph_shape(info, glyph, &verts);
    data = (unsigned char *)FNT_malloc(w * h, info->userdata);
    precompute = (float *)FNT_malloc(num_verts * sizeof(float), info->userdata);

    for (i = 0, j = num_verts - 1; i < num_verts; j = i++) {
      if (verts[i].type == FNT_vline) {
        float x0 = verts[i].x * scale_x, y0 = verts[i].y * scale_y;
        float x1 = verts[j].x * scale_x, y1 = verts[j].y * scale_y;
        float dist =
            (float)FNT_sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
        precompute[i] = (dist == 0) ? 0.0f : 1.0f / dist;
      } else if (verts[i].type == FNT_vcurve) {
        float x2 = verts[j].x * scale_x, y2 = verts[j].y * scale_y;
        float x1 = verts[i].cx * scale_x, y1 = verts[i].cy * scale_y;
        float x0 = verts[i].x * scale_x, y0 = verts[i].y * scale_y;
        float bx = x0 - 2 * x1 + x2, by = y0 - 2 * y1 + y2;
        float len2 = bx * bx + by * by;
        if (len2 != 0.0f)
          precompute[i] = 1.0f / (bx * bx + by * by);
        else
          precompute[i] = 0.0f;
      } else
        precompute[i] = 0.0f;
    }

    for (y = iy0; y < iy1; ++y) {
      for (x = ix0; x < ix1; ++x) {
        float val;
        float min_dist = 999999.0f;
        float sx = (float)x + 0.5f;
        float sy = (float)y + 0.5f;
        float x_gspace = (sx / scale_x);
        float y_gspace = (sy / scale_y);

        int winding = fnt__compute_crossings_x(
            x_gspace, y_gspace, num_verts,
            verts);  // @OPTIMIZE: this could just be a rasterization, but needs
                     // to be line vs. non-tesselated curves so a new path

        for (i = 0; i < num_verts; ++i) {
          float x0 = verts[i].x * scale_x, y0 = verts[i].y * scale_y;

          // check against every point here rather than inside line/curve
          // primitives -- @TODO: wrong if multiple 'moves' in a row produce a
          // garbage point, and given culling, probably more efficient to do
          // within line/curve
          float dist2 = (x0 - sx) * (x0 - sx) + (y0 - sy) * (y0 - sy);
          if (dist2 < min_dist * min_dist) min_dist = (float)FNT_sqrt(dist2);

          if (verts[i].type == FNT_vline) {
            float x1 = verts[i - 1].x * scale_x, y1 = verts[i - 1].y * scale_y;

            // coarse culling against bbox
            // if (sx > FNT_min(x0,x1)-min_dist && sx < FNT_max(x0,x1)+min_dist
            // &&
            //    sy > FNT_min(y0,y1)-min_dist && sy < FNT_max(y0,y1)+min_dist)
            float dist =
                (float)FNT_fabs((x1 - x0) * (y0 - sy) - (y1 - y0) * (x0 - sx)) *
                precompute[i];
            FNT_assert(i != 0);
            if (dist < min_dist) {
              // check position along line
              // x' = x0 + t*(x1-x0), y' = y0 + t*(y1-y0)
              // minimize (x'-sx)*(x'-sx)+(y'-sy)*(y'-sy)
              float dx = x1 - x0, dy = y1 - y0;
              float px = x0 - sx, py = y0 - sy;
              // minimize (px+t*dx)^2 + (py+t*dy)^2 = px*px + 2*px*dx*t +
              // t^2*dx*dx + py*py + 2*py*dy*t + t^2*dy*dy derivative: 2*px*dx +
              // 2*py*dy + (2*dx*dx+2*dy*dy)*t, set to 0 and solve
              float t = -(px * dx + py * dy) / (dx * dx + dy * dy);
              if (t >= 0.0f && t <= 1.0f) min_dist = dist;
            }
          } else if (verts[i].type == FNT_vcurve) {
            float x2 = verts[i - 1].x * scale_x, y2 = verts[i - 1].y * scale_y;
            float x1 = verts[i].cx * scale_x, y1 = verts[i].cy * scale_y;
            float box_x0 = FNT_min(FNT_min(x0, x1), x2);
            float box_y0 = FNT_min(FNT_min(y0, y1), y2);
            float box_x1 = FNT_max(FNT_max(x0, x1), x2);
            float box_y1 = FNT_max(FNT_max(y0, y1), y2);
            // coarse culling against bbox to avoid computing cubic
            // unnecessarily
            if (sx > box_x0 - min_dist && sx < box_x1 + min_dist &&
                sy > box_y0 - min_dist && sy < box_y1 + min_dist) {
              int num = 0;
              float ax = x1 - x0, ay = y1 - y0;
              float bx = x0 - 2 * x1 + x2, by = y0 - 2 * y1 + y2;
              float mx = x0 - sx, my = y0 - sy;
              float res[3], px, py, t, it;
              float a_inv = precompute[i];
              if (a_inv == 0.0) {  // if a_inv is 0, it's 2nd degree so use
                                   // quadratic formula
                float a = 3 * (ax * bx + ay * by);
                float b = 2 * (ax * ax + ay * ay) + (mx * bx + my * by);
                float c = mx * ax + my * ay;
                if (a == 0.0) {  // if a is 0, it's linear
                  if (b != 0.0) {
                    res[num++] = -c / b;
                  }
                } else {
                  float discriminant = b * b - 4 * a * c;
                  if (discriminant < 0)
                    num = 0;
                  else {
                    float root = (float)FNT_sqrt(discriminant);
                    res[0] = (-b - root) / (2 * a);
                    res[1] = (-b + root) / (2 * a);
                    num = 2;  // don't bother distinguishing 1-solution case, as
                              // code below will still work
                  }
                }
              } else {
                float b = 3 * (ax * bx + ay * by) *
                          a_inv;  // could precompute this as it doesn't depend
                                  // on sample point
                float c =
                    (2 * (ax * ax + ay * ay) + (mx * bx + my * by)) * a_inv;
                float d = (mx * ax + my * ay) * a_inv;
                num = fnt__solve_cubic(b, c, d, res);
              }
              if (num >= 1 && res[0] >= 0.0f && res[0] <= 1.0f) {
                t = res[0], it = 1.0f - t;
                px = it * it * x0 + 2 * t * it * x1 + t * t * x2;
                py = it * it * y0 + 2 * t * it * y1 + t * t * y2;
                dist2 = (px - sx) * (px - sx) + (py - sy) * (py - sy);
                if (dist2 < min_dist * min_dist)
                  min_dist = (float)FNT_sqrt(dist2);
              }
              if (num >= 2 && res[1] >= 0.0f && res[1] <= 1.0f) {
                t = res[1], it = 1.0f - t;
                px = it * it * x0 + 2 * t * it * x1 + t * t * x2;
                py = it * it * y0 + 2 * t * it * y1 + t * t * y2;
                dist2 = (px - sx) * (px - sx) + (py - sy) * (py - sy);
                if (dist2 < min_dist * min_dist)
                  min_dist = (float)FNT_sqrt(dist2);
              }
              if (num >= 3 && res[2] >= 0.0f && res[2] <= 1.0f) {
                t = res[2], it = 1.0f - t;
                px = it * it * x0 + 2 * t * it * x1 + t * t * x2;
                py = it * it * y0 + 2 * t * it * y1 + t * t * y2;
                dist2 = (px - sx) * (px - sx) + (py - sy) * (py - sy);
                if (dist2 < min_dist * min_dist)
                  min_dist = (float)FNT_sqrt(dist2);
              }
            }
          }
        }
        if (winding == 0)
          min_dist = -min_dist;  // if outside the shape, value is negative
        val = onedge_value + pixel_dist_scale * min_dist;
        if (val < 0)
          val = 0;
        else if (val > 255)
          val = 255;
        data[(y - iy0) * w + (x - ix0)] = (unsigned char)val;
      }
    }
    FNT_free(precompute, info->userdata);
    FNT_free(verts, info->userdata);
  }
  return data;
}
static unsigned char *
fnt_get_codepoint_sdf(const struct fnt_info *info, float scale, int codepoint,
                      int padding, unsigned char onedge_value,
                      float pixel_dist_scale, int *width, int *height,
                      int *xoff, int *yoff) {
  return fnt_get_glyph_sdf(info, scale, fnt_fnd_glyph_index(info, codepoint),
                           padding, onedge_value, pixel_dist_scale, width,
                           height, xoff, yoff);
}
static void
fnt_free_sdf(unsigned char *bitmap, void *userdata) {
  FNT_free(bitmap, userdata);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// font name matching -- recommended not to use this
//

// check if a utf8 string contains a prefix which is the utf16 string; if so
// return length of matching utf8 string
#if 0
static fnt_int32
fnt__compare_utf8_to_utf16_bigendian_prefix(fnt_uint8 *s1, fnt_int32 len1,
                                            fnt_uint8 *s2, fnt_int32 len2) {
  fnt_int32 i = 0;

  // convert utf16 to utf8 and compare the results while converting
  while (len2) {
    fnt_uint16 ch = s2[0] * 256 + s2[1];
    if (ch < 0x80) {
      if (i >= len1) return -1;
      if (s1[i++] != ch) return -1;
    } else if (ch < 0x800) {
      if (i + 1 >= len1) return -1;
      if (s1[i++] != 0xc0 + (ch >> 6)) return -1;
      if (s1[i++] != 0x80 + (ch & 0x3f)) return -1;
    } else if (ch >= 0xd800 && ch < 0xdc00) {
      fnt_uint32 c;
      fnt_uint16 ch2 = s2[2] * 256 + s2[3];
      if (i + 3 >= len1) return -1;
      c = ((ch - 0xd800) << 10) + (ch2 - 0xdc00) + 0x10000;
      if (s1[i++] != 0xf0 + (c >> 18)) return -1;
      if (s1[i++] != 0x80 + ((c >> 12) & 0x3f)) return -1;
      if (s1[i++] != 0x80 + ((c >> 6) & 0x3f)) return -1;
      if (s1[i++] != 0x80 + ((c)&0x3f)) return -1;
      s2 += 2;  // plus another 2 below
      len2 -= 2;
    } else if (ch >= 0xdc00 && ch < 0xe000) {
      return -1;
    } else {
      if (i + 2 >= len1) return -1;
      if (s1[i++] != 0xe0 + (ch >> 12)) return -1;
      if (s1[i++] != 0x80 + ((ch >> 6) & 0x3f)) return -1;
      if (s1[i++] != 0x80 + ((ch)&0x3f)) return -1;
    }
    s2 += 2;
    len2 -= 2;
  }
  return i;
}
static int
fnt_compare_utf8_to_utf16_bigendian_internal(char *s1, int len1, char *s2,
                                             int len2) {
  return len1 == fnt__compare_utf8_to_utf16_bigendian_prefix(
                     (fnt_uint8 *)s1, len1, (fnt_uint8 *)s2, len2);
}
// returns results in whatever encoding you request... but note that 2-byte
// encodings will be BIG-ENDIAN... use fnt_CompareUTF8toUTF16_bigendian() to
// compare
static const char *
fnt_get_font_name_string(const struct fnt_info *font, int *length,
                         int platformID, int encodingID, int languageID,
                         int nameID) {
  fnt_int32 i, count, stringOffset;
  fnt_uint8 *fc = font->data;
  fnt_uint32 offset = font->fontstart;
  fnt_uint32 nm = fnt__find_table(fc, offset, "name");
  if (!nm) return NULL;

  count = ttUSHORT(fc + nm + 2);
  stringOffset = nm + ttUSHORT(fc + nm + 4);
  for (i = 0; i < count; ++i) {
    fnt_uint32 loc = nm + 6 + 12 * i;
    if (platformID == ttUSHORT(fc + loc + 0) &&
        encodingID == ttUSHORT(fc + loc + 2) &&
        languageID == ttUSHORT(fc + loc + 4) &&
        nameID == ttUSHORT(fc + loc + 6)) {
      *length = ttUSHORT(fc + loc + 8);
      return (const char *)(fc + stringOffset + ttUSHORT(fc + loc + 10));
    }
  }
  return NULL;
}
static int
fnt__matchpair(fnt_uint8 *fc, fnt_uint32 nm, fnt_uint8 *name, fnt_int32 nlen,
               fnt_int32 target_id, fnt_int32 next_id) {
  fnt_int32 i;
  fnt_int32 count = ttUSHORT(fc + nm + 2);
  fnt_int32 stringOffset = nm + ttUSHORT(fc + nm + 4);

  for (i = 0; i < count; ++i) {
    fnt_uint32 loc = nm + 6 + 12 * i;
    fnt_int32 id = ttUSHORT(fc + loc + 6);
    if (id == target_id) {
      // find the encoding
      fnt_int32 platform = ttUSHORT(fc + loc + 0),
                encoding = ttUSHORT(fc + loc + 2),
                language = ttUSHORT(fc + loc + 4);

      // is this a Unicode encoding?
      if (platform == 0 || (platform == 3 && encoding == 1) ||
          (platform == 3 && encoding == 10)) {
        fnt_int32 slen = ttUSHORT(fc + loc + 8);
        fnt_int32 off = ttUSHORT(fc + loc + 10);

        // check if there's a prefix match
        fnt_int32 matchlen = fnt__compare_utf8_to_utf16_bigendian_prefix(
            name, nlen, fc + stringOffset + off, slen);
        if (matchlen >= 0) {
          // check for target_id+1 immediately following, with same encoding &
          // language
          if (i + 1 < count && ttUSHORT(fc + loc + 12 + 6) == next_id &&
              ttUSHORT(fc + loc + 12) == platform &&
              ttUSHORT(fc + loc + 12 + 2) == encoding &&
              ttUSHORT(fc + loc + 12 + 4) == language) {
            slen = ttUSHORT(fc + loc + 12 + 8);
            off = ttUSHORT(fc + loc + 12 + 10);
            if (slen == 0) {
              if (matchlen == nlen) return 1;
            } else if (matchlen < nlen && name[matchlen] == ' ') {
              ++matchlen;
              if (fnt_compare_utf8_to_utf16_bigendian_internal(
                      (char *)(name + matchlen), nlen - matchlen,
                      (char *)(fc + stringOffset + off), slen))
                return 1;
            }
          } else {
            // if nothing immediately following
            if (matchlen == nlen) return 1;
          }
        }
      }

      // @TODO handle other encodings
    }
  }
  return 0;
}
static int
fnt__matches(fnt_uint8 *fc, fnt_uint32 offset, fnt_uint8 *name,
             fnt_int32 flags) {
  fnt_int32 nlen = (fnt_int32)FNT_strlen((char *)name);
  fnt_uint32 nm, hd;
  if (!fnt__isfont(fc + offset)) return 0;

  // check italics/bold/underline flags in macStyle...
  if (flags) {
    hd = fnt__find_table(fc, offset, "head");
    if ((ttUSHORT(fc + hd + 44) & 7) != (flags & 7)) return 0;
  }

  nm = fnt__find_table(fc, offset, "name");
  if (!nm) return 0;

  if (flags) {
    // if we checked the macStyle flags, then just check the family and ignore
    // the subfamily
    if (fnt__matchpair(fc, nm, name, nlen, 16, -1)) return 1;
    if (fnt__matchpair(fc, nm, name, nlen, 1, -1)) return 1;
    if (fnt__matchpair(fc, nm, name, nlen, 3, -1)) return 1;
  } else {
    if (fnt__matchpair(fc, nm, name, nlen, 16, 17)) return 1;
    if (fnt__matchpair(fc, nm, name, nlen, 1, 2)) return 1;
    if (fnt__matchpair(fc, nm, name, nlen, 3, -1)) return 1;
  }

  return 0;
}
static int
fnt_find_matching_font_internal(unsigned char *font_collection, char *name_utf8,
                                fnt_int32 flags) {
  fnt_int32 i;
  for (i = 0;; ++i) {
    fnt_int32 off = fnt_get_font_offset_for_index(font_collection, i);
    if (off < 0) return off;
    if (fnt__matches((fnt_uint8 *)font_collection, off, (fnt_uint8 *)name_utf8,
                     flags))
      return off;
  }
}
#endif

static int
fnt_bake_bitmap(const unsigned char *data, int offset, float pixel_height,
                unsigned char *pixels, int pw, int ph, int first_char,
                int num_chars, struct fnt_baked_char *chardata) {
  return fnt_bake_bitmap_internal((unsigned char *)data, offset, pixel_height,
                                  pixels, pw, ph, first_char, num_chars,
                                  chardata);
}
static int
fnt_init(struct fnt_info *info, const unsigned char *data, int offset) {
  return fnt_init_internal(info, (unsigned char *)data, offset);
}
#if 0
static int
fnt_get_font_offset_for_index(const unsigned char *data, int index) {
  return fnt_get_font_offset_for_index_internal((unsigned char *)data, index);
}
static int
fnt_get_number_of_fonts(const unsigned char *data) {
  return fnt_get_number_of_fonts_internal((unsigned char *)data);
}
static int
fnt_find_matching_font(const unsigned char *fontdata, const char *name,
                       int flags) {
  return fnt_find_matching_font_internal((unsigned char *)fontdata,
                                         (char *)name, flags);
}
static int
fnt_compare_utf8_to_utf16_bigendian(const char *s1, int len1, const char *s2,
                                    int len2) {
  return fnt_compare_utf8_to_utf16_bigendian_internal((char *)s1, len1,
                                                      (char *)s2, len2);
}
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

