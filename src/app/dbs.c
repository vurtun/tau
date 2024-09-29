/* ---------------------------------------------------------------------------
 *
 *                                Database
 *
 * ---------------------------------------------------------------------------
 */
// clang-format off
static const struct db_tbl_col_def db_tbl_fltr_def[DB_TBL_FLTR_MAX] = {
  [DB_TBL_FLTR_STATE] = {.title = strv(""),                                                                     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {10, 200}}},
  [DB_TBL_FLTR_BUF]   = {.title = strv("Filter"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 800}}},
  [DB_TBL_FLTR_COL]   = {.title = strv("Column"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 800}}},
  [DB_TBL_FLTR_TYP]   = {.title = strv("Type"),   .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {100, 800}}},
  [DB_TBL_FLTR_DEL]   = {.title = strv(""),                                                                     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {10, 200}}},
};
static const struct db_tbl_col_def db_tbl_fltr_col_def[DB_TBL_FLTR_COL_MAX] = {
  [DB_TBL_FLTR_COL_NAME] =  {.title = strv("Name"),  .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {100, 400}}},
  [DB_TBL_FLTR_COL_TYPE] =  {.title = strv("Type"),  .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {100, 400}}},
};
static const struct db_tbl_col_def db_tree_col_def[DB_TREE_COL_MAX] = {
  [DB_TREE_COL_NAME]  = {.title = strv("Name"),   .ui = {.type = GUI_LAY_SLOT_FIX, .size = 400, .con = {50, 1000}}},
  [DB_TREE_COL_TYPE]  = {.title = strv("Type"),   .ui = {.type = GUI_LAY_SLOT_FIX, .size = 200, .con = {50, 1000}}},
  [DB_TREE_COL_SQL]   = {.title = strv("Schema"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {50, 1000}}},
};
// clang-format on

#if 0
static int db_tbl_cmp_asc(const void *a, const void *b);
static int db_tbl_cmp_desc(const void *a, const void *b);
static int db_tbl_cmp_type_asc(const void *a, const void *b);
static int db_tbl_cmp_type_desc(const void *a, const void *b);

static int db_tbl_fltr_cmp_asc(const void *a, const void *b);
static int db_tbl_fltr_cmp_desc(const void *a, const void *b);
static int db_tbl_fltr_cmp_type_asc(const void *a, const void *b);
static int db_tbl_fltr_cmp_type_desc(const void *a, const void *b);
static int db_tbl_fltr_cmp_act_asc(const void *a, const void *b);
static int db_tbl_fltr_cmp_act_desc(const void *a, const void *b);

static int
db_tbl_fltr_cmp_asc(const void *a, const void *b) {
  const struct db_tbl_fltr *fa = (const struct db_tbl_fltr*)a;
  const struct db_tbl_fltr *fb = (const struct db_tbl_fltr*)b;
  return str_cmp(fa->col->name, fb->col->name);
}
static int
db_tbl_fltr_cmp_desc(const void *a, const void *b) {
  const struct db_tbl_fltr *fa = (const struct db_tbl_fltr*)a;
  const struct db_tbl_fltr *fb = (const struct db_tbl_fltr*)b;
  return str_cmp(fb->col->name, fa->col->name);
}
static int
db_tbl_fltr_cmp_type_asc(const void *a, const void *b) {
  const struct db_tbl_fltr *fa = (const struct db_tbl_fltr*)a;
  const struct db_tbl_fltr *fb = (const struct db_tbl_fltr*)b;
  return str_cmp(fa->col->type, fb->col->type);
}
static int
db_tbl_fltr_cmp_type_desc(const void *a, const void *b){
  const struct db_tbl_fltr *fa = (const struct db_tbl_fltr*)a;
  const struct db_tbl_fltr *fb = (const struct db_tbl_fltr*)b;
  return str_cmp(fb->col->type, fa->col->type);
}
static int
db_tbl_fltr_cmp_act_asc(const void *a, const void *b) {
  const struct db_tbl_fltr *fa = (const struct db_tbl_fltr*)a;
  const struct db_tbl_fltr *fb = (const struct db_tbl_fltr*)b;
  if (fa->enabled && !fb->enabled) {
    return -1;
  } else if (!fa->enabled && fb->enabled) {
    return 1;
  }
  return str_cmp(fa->col->name, fb->col->name);
}
static int
db_tbl_fltr_cmp_act_desc(const void *a, const void *b){
  const struct db_tbl_fltr *fa = (const struct db_tbl_fltr*)a;
  const struct db_tbl_fltr *fb = (const struct db_tbl_fltr*)b;
  if (fa->enabled && !fb->enabled) {
    return 1;
  } else if (!fa->enabled && fb->enabled) {
    return -1;
  }
  return str_cmp(fa->col->name, fb->col->name);
}
#endif

/* ---------------------------------------------------------------------------
 *                                Filter
 * ---------------------------------------------------------------------------
 */
static int
db_tbl_view_fltr_view_fltr_new(struct db_tbl_fltr_view *fltr) {
  assert(fltr);
  assert(fltr->unused > 0);

  int idx = cpu_bit_ffs32(fltr->unused);
  fltr->unused &= ~(1llu << idx);
  mset(fltr->elms + idx, 0, szof(fltr->elms[0]));
  return idx;
}
static void
db_tbl_view_fltr_view_fltr_del(struct db_tbl_fltr_view *fltr, int idx) {
  assert(fltr);
  assert(idx >= 0);
  assert(idx < cntof(fltr->elms));
  assert(!(fltr->unused & (1u << idx)));

  struct db_tbl_fltr_elm *elm = &fltr->elms[idx];
  fltr->unused |= (1u << idx);
  mset(elm, 0, szof(*elm));
}
static int
db_tbl_view_fltr_view_fltr_add(struct db_tbl_fltr_view *fltr, int idx) {
  assert(fltr);
  assert(idx >= 0);
  assert(idx < cntof(fltr->lst));
  assert(fltr->fltr_cnt < cntof(fltr->lst));
  assert(!(fltr->unused & (1u << idx)));

  fltr->lst[fltr->fltr_cnt] = castb(idx);
  return fltr->fltr_cnt++;
}
static void
db_tbl_view_fltr_view_fltr_rm(struct db_tbl_fltr_view *fltr, int idx) {
  assert(fltr);
  assert(idx >= 0);
  assert(idx < fltr->fltr_cnt);
  assert(idx < cntof(fltr->lst));
  assert(fltr->fltr_cnt < cntof(fltr->lst));
  assert(!(fltr->unused & (1u << fltr->lst[idx])));

  arr_rm(fltr->lst, idx, fltr->fltr_cnt);
  fltr->fltr_cnt--;
}
static int
db_tbl_view_fltr_view_add_str(struct db_tbl_fltr_view *fltr, int col, struct str str) {
  assert(fltr);
  assert(col > 0);
  assert(fltr->unused);
  assert(str_len(str));
  assert(str_len(str) < DB_MAX_FLTR_STR);
  assert(fltr->fltr_cnt < cntof(fltr->lst));

  int idx = db_tbl_view_fltr_view_fltr_new(fltr);
  struct db_tbl_fltr_elm *elm = &fltr->elms[idx];
  elm->type = DB_TBL_FLTR_ELM_TYP_STR;
  elm->enabled = 1;
  elm->col = col;
  elm->str = str_set(elm->buf, cntof(elm->buf), str);
  return db_tbl_view_fltr_view_fltr_add(fltr, idx);
}
static void
db_tbl_view_fltr_view_rm(struct db_tbl_fltr_view *fltr, int idx) {
  assert(fltr);
  assert(idx >= 0);
  assert(idx < fltr->fltr_cnt);
  assert(idx < cntof(fltr->lst));
  assert(!(fltr->unused & (1u << fltr->lst[idx])));

  int elm = fltr->lst[idx];
  db_tbl_view_fltr_view_fltr_rm(fltr, idx);
  db_tbl_view_fltr_view_fltr_del(fltr, elm);
}
static void
db_tbl_view_fltr_view_clr(struct db_tbl_fltr_view *fltr) {
  assert(fltr);
  unsigned used = ~fltr->unused;
  while (used) {
    int idx = cpu_bit_ffs32(used);
    db_tbl_view_fltr_view_fltr_del(fltr, idx);
    used = ~fltr->unused;
  }
  fltr->fltr_cnt = 0;
}




#if 0
struct db_fltr_cfg {
  struct str tbl;
  struct str col;
  struct str match;
  int off, lim;
};
static void
db_tbl_view_fltr_init(struct db_tbl_fltr_view *view, struct db_tbl_fltr *fltr,
                      struct sys *_sys, struct arena *mem, sqlite3 *con,
                      struct db_fltr_cfg *cfg) {
  assert(con);
  assert(mem);
  assert(view);
  assert(fltr);

  view->row_cnt = 0;
  view->row_begin = cfg->off;
  view->row_end = cfg->off + cfg->lim;

  if (view->data) {
    arena_scope_pop(&view->scp, mem, _sys);
    fltr->is_date = 0;
    fltr->date_init = 0;
  }
  int has_match = str_len(cfg->match) > 0;

  /* retrieve total table row count */
  struct str sql;
  sqlite3_stmt *stmt;
  struct arena_scope scp;
  confine arena_scope(mem, &scp, _sys) {
    if (has_match) {
      sql = arena_fmt(mem, _sys, "SELECT COUNT(%.*s) FROM %.*s WHERE %.*s LIKE '%%%.*s%%';", strf(cfg->col), strf(cfg->tbl), strf(cfg->col), strf(cfg->match));
      sqlite3_prepare_v2(con, str_beg(sql), -1, &stmt, 0);
      sqlite3_step(stmt);
      view->row_cnt = sqlite3_column_int(stmt, 0);
      sqlite3_finalize(stmt);
    } else {
      sql = arena_fmt(mem, _sys, "SELECT COUNT(%.*s) FROM %.*s;", strf(cfg->col), strf(cfg->tbl));
      sqlite3_prepare_v2(con, str_beg(sql), -1, &stmt, 0);
      sqlite3_step(stmt);
      view->row_cnt = sqlite3_column_int(stmt, 0);
      sqlite3_finalize(stmt);
    }
  }
  stmt = 0;

  /* retrieve data window */
  int i = 0;
  arena_scope_push(&view->scp, mem);
  view->data = arena_arr(mem, _sys, struct str, cfg->lim);
  if (has_match) {
    sql = arena_fmt(mem, _sys, "SELECT %.*s FROM %.*s WHERE %.*s LIKE '%%%.*s%%'" "LIMIT %d, %d;", strf(cfg->col), strf(cfg->tbl), strf(cfg->col), strf(cfg->match), cfg->off, cfg->lim);
  } else {
    sql = arena_fmt(mem, _sys, "SELECT %.*s FROM %.*s LIMIT %d, %d;", strf(cfg->col), strf(cfg->tbl), cfg->off, cfg->lim);
  }
  sqlite3_prepare_v2(con, str_beg(sql), -1, &stmt, 0);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *dat = (const char*)sqlite3_column_text(stmt, 0);
    if (dat){
      int len = sqlite3_column_bytes(stmt, 0);
      view->data[i++] = arena_str(mem, _sys, strn(dat,len));
    }
  }
  sqlite3_finalize(stmt);
}
static int
db_tbl_view_fltr_time_range_init(struct db_tbl_fltr *fltr, struct sys *_sys,
                                 struct arena *mem, sqlite3 *con,
                                 struct str tbl, struct str col) {
  assert(con);
  assert(mem);
  assert(fltr);

  struct arena_scope scp;
  confine arena_scope(mem, &scp, _sys) {
    /* retrieve time range */
    sqlite3_stmt *stmt = 0;
    struct str sql = arena_fmt(mem, _sys, "SELECT MIN(strftime('%%s',%.*s)),""MAX(strftime('%%s',%.*s)) FROM %.*s;", strf(col), strf(col), strf(tbl));
    sqlite3_prepare_v2(con, str_beg(sql), -1, &stmt, 0);
    sqlite3_step(stmt);

    const char *min = (const char*)sqlite3_column_text(stmt, 0);
    const char *max = (const char*)sqlite3_column_text(stmt, 1);
    if (!min || !max) {
      fltr->hide_date = 1;
      fltr->is_date = 0;
      arena_scope_pop(&scp, mem, _sys);
      return 0;
    }
    fltr->tm.min = sqlite3_column_int64(stmt, 0);
    fltr->tm.max = sqlite3_column_int64(stmt, 1);
    sqlite3_finalize(stmt);

    fltr->tm.from = *localtime(&fltr->tm.min);
    fltr->tm.to = *localtime(&fltr->tm.max);
  }
  fltr->is_date = 1;
  return 1;
}
struct db_fltr_time_cfg {
  struct str tbl;
  struct str col;
  int off, lim;
};
static void
db_tbl_view_fltr_tm_init(struct db_tbl_fltr_view *view, struct db_tbl_fltr *fltr,
                         struct sys *_sys, struct arena *mem, sqlite3 *con,
                         struct db_fltr_time_cfg *cfg) {
  assert(con);
  assert(mem);
  assert(view);
  assert(fltr);

  view->row_cnt = 0;
  view->row_begin = cfg->off;
  view->row_end = cfg->off + cfg->lim;
  if (view->data) {
    arena_scope_pop(&view->scp, mem, _sys);
    fltr->is_date = 1;
  }
  struct arena_scope scp;
  confine arena_scope(mem, &scp, _sys) {
    /* retrieve total count */
    sqlite3_stmt *stmt = 0;
    fltr->tm.from_val = mktime(&fltr->tm.from);
    fltr->tm.to_val = mktime(&fltr->tm.to);
    struct str sql = arena_fmt(mem, _sys, "SELECT COUNT(%.*s) FROM %.*s WHERE" "strftime('%%s',%.*s) BETWEEN '%lld'" "AND '%lld';", strf(cfg->col), strf(cfg->tbl), strf(cfg->tbl), strf(cfg->col), fltr->tm.from_val, fltr->tm.to_val);
    sqlite3_prepare_v2(con, str_beg(sql), -1, &stmt, 0);
    sqlite3_step(stmt);
    view->row_cnt = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
  }
  /* retrieve data window */
  int i = 0;
  arena_scope_push(&view->scp, mem);
  view->data = arena_arr(mem, _sys, struct str, cfg->lim);
  struct str sql = arena_fmt(mem, _sys, "SELECT %.*s FROM %.*s WHERE""strftime('%%s',%.*s) BETWEEN '%lld' AND '%lld' LIMIT %d, %d;", strf(cfg->col), strf(cfg->tbl), strf(cfg->col), fltr->tm.from_val, fltr->tm.to_val, cfg->off, cfg->lim);

  sqlite3_stmt *stmt = 0;
  sqlite3_prepare_v2(con, str_beg(sql), -1, &stmt, 0);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *dat = (const char*)sqlite3_column_text(stmt, 0);
    if (dat) {
      int len = sqlite3_column_bytes(stmt, 0);
      view->data[i++] = arena_str(mem, _sys, strn(dat, len));
    }
  }
  sqlite3_finalize(stmt);
}
static void
db_tbl_view_fltr_clr(struct db_tbl_view *view, struct db_tbl_fltr_view *fltr,
                     struct sys *_sys) {
  assert(view);
  assert(fltr);

  dyn_clr(fltr->buf);
  if (fltr->data) {
    arena_scope_pop(&fltr->scp, &view->mem, _sys);
    fltr->data = 0;
  }
  fltr->row_cnt = 0;
  fltr->row_begin = 0;
  fltr->row_end = 0;
  fltr->off[0] = fltr->off[1] = 0.0;
  memset(&fltr->ini, 0, sizeof(fltr->ini));
}
#endif

/* ---------------------------------------------------------------------------
 *                                Table View
 * ---------------------------------------------------------------------------
 */
static int
db_tbl_view_new(struct db_view *db) {
  assert(db);
  assert(db->unused > 0);

  int idx = cpu_bit_ffs64(db->unused);
  db->unused &= ~(1llu << idx);
  mset(db->tbls + idx, 0, szof(db->tbls[0]));
  return idx;
}
static struct db_tbl_col*
db__tbl_view_qry_cols(struct db_view *db, struct db_tbl_view *view, struct sys *_sys,
                      struct str id, struct db_tbl_col *cols, struct arena *mem) {
  assert(db);
  assert(mem);
  assert(cols);

  /* query table column schema */
  sqlite3_stmt *stmt = 0;
  struct str sql = str_fmtsn(db->sql_qry_buf, cntof(db->sql_qry_buf), "PRAGMA table_info(%.*s);", strf(id));
  sqlite3_prepare_v2(db->con, str_beg(sql), -1, &stmt, 0);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *col_name = (const char*)sqlite3_column_text(stmt, 1);
    const char *col_type = (const char*)sqlite3_column_text(stmt, 2);
    const char *col_notnull = (const char*)sqlite3_column_text(stmt, 3);
    const char *col_dflt = (const char*)sqlite3_column_text(stmt, 4);
    const char *col_key = (const char*)sqlite3_column_text(stmt, 5);

    int nam_len = sqlite3_column_bytes(stmt, 1);
    int typ_len = sqlite3_column_bytes(stmt, 2);

    struct db_tbl_col col = {0};
    col.name = arena_str(mem, _sys, strn(col_name, nam_len));
    col.type = arena_str(mem, _sys, strn(col_type, typ_len));
    col.dflt = col_dflt ? arena_str(mem, _sys, str0(col_dflt)) : str_nil;
    col.not_null = !strcmp(col_notnull, "1");
    col.pk = !strcmp(col_key, "1");
    col.blob = !str_len(col.type) || col_type[0] == 0;
    col.blob = col.blob || !str_cmp(col.type, strv("BLOB"));
    if (view && col.pk && col.blob) {
      view->blob.disabled = 1;
    }
    dyn_add(cols, _sys, col);
  }
  sqlite3_finalize(stmt);
  stmt = 0;

  /* query table column foreign keys */
  sql = str_fmtsn(db->sql_qry_buf, cntof(db->sql_qry_buf), "PRAGMA foreign_key_list(%.*s);", strf(id));
  sqlite3_prepare_v2(db->con, str_beg(sql), -1, &stmt, 0);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *ccol_from = (const char*)sqlite3_column_text(stmt, 3);
    struct str col_from = str0(ccol_from);
    struct db_tbl_col *col = 0;
    for dyn_each(col, cols) {
      if (!str_cmp(col->name, col_from)) {
        col->fk = 1;
        break;
      }
    }
  }
  sqlite3_finalize(stmt);
  return cols;
}
static int
db_tbl_view_setup(struct db_view *db, int idx, struct sys *_sys,
                  struct gui_ctx *ctx, struct arena *tmp_mem,
                  struct str id, enum db_tbl_type kind) {
  assert(db);
  assert(con);
  assert(ctx);
  assert(mem);
  assert(tmp_mem);
  assert(idx >= 0);
  assert(idx < cntof(db->tbls));
  assert(!(db->unused & (1llu << idx)));

  struct db_tbl_view *tbl = &db->tbls[idx];
  tbl->kind = kind;
  tbl->tmp_mem = tmp_mem;
  tbl->cols = arena_dyn(&tbl->mem, _sys, struct db_tbl_col, 128);
  tbl->name = arena_str(&tbl->mem, _sys, id);
  tbl->fltr.buf = arena_dyn(&tbl->mem, _sys, char, DB_MAX_FILTER);
  tbl->fltr.unused = ~0u;

  struct arena_scope scp = {0};
  arena_scope_push(&scp, tmp_mem);
  tbl->cols = db__tbl_view_qry_cols(db, tbl, _sys, id, tbl->cols, &tbl->mem);

  /* generate table state */
  struct gui_split_lay bld = {0};
  int word_cnt = GUI_TBL_CAP(dyn_cnt(tbl->cols));
  tbl->ui.state = arena_arr(&tbl->mem, _sys, int, word_cnt);
  gui.splt.lay.begin(&bld, tbl->ui.state, dyn_cnt(tbl->cols), ctx->cfg.sep);
  for dyn_loop(i, tbl->cols) {
    assert(i < dyn_cnt(tbl->cols));
    static const int cons[2] = {100, 600};
    gui.splt.lay.add(&bld, GUI_LAY_SLOT_DYN, 1, cons);
  }
  gui.splt.lay.end(&bld);

  /* retrieve total table row count */
  sqlite3_stmt *stmt = 0;
  struct str sql = arena_fmt(tmp_mem, _sys, "SELECT COUNT(*) FROM %.*s;", strf(id));
  sqlite3_prepare_v2(db->con, str_beg(sql), -1, &stmt, 0);
  sqlite3_step(stmt);
  tbl->total = sqlite3_column_int(stmt, 0);
  tbl->row_cnt = tbl->total;
  sqlite3_finalize(stmt);
  arena_scope_pop(&scp, tmp_mem, _sys);
  tbl->rev = (unsigned)-1;

  /* setup filter list table */
  struct gui_split_lay_cfg tbl_cfg = {0};
  tbl_cfg.size = szof(struct db_tbl_col_def);
  tbl_cfg.off = offsetof(struct db_tbl_col_def, ui);
  tbl_cfg.slots = db_tbl_fltr_def;
  tbl_cfg.cnt = DB_TBL_FLTR_MAX;
  gui.tbl.lay(tbl->fltr.tbl.state, ctx, &tbl_cfg);

  /* setup filter column table */
  struct gui_split_lay_cfg col_cfg = {0};
  col_cfg.size = szof(struct db_tbl_col_def);
  col_cfg.off = offsetof(struct db_tbl_col_def, ui);
  col_cfg.slots = db_tbl_fltr_col_def;
  col_cfg.cnt = DB_TBL_FLTR_COL_MAX;
  gui.tbl.lay(tbl->fltr.tbl_col.state, ctx, &col_cfg);

  tbl->state = TBL_VIEW_DISPLAY;
  return 0;
}
static void
db_tbl_view_del(struct db_view *db, struct sys *_sys, int idx) {
  assert(db);
  assert(_sys_);
  assert(idx >= 0);
  assert(idx < cntof(db->tbls));
  assert(!(db->unused & (1llu << idx)));

  struct db_tbl_view *tbl = &db->tbls[idx];
  db->unused |= (1llu << idx);
  if (tbl->data) {
    arena_scope_pop(&tbl->scp, &tbl->mem, _sys);
  }
  db_tbl_view_fltr_view_clr(&tbl->fltr);
  dyn_free(tbl->cols, _sys);
  arena_free(&tbl->mem, _sys);
  mset(tbl, 0, szof(*tbl));
}
static int
db_tbl_view_add(struct db_view *db, int idx) {
  assert(db);
  assert(idx >= 0);
  assert(idx < cntof(db->tbls));
  assert(db->tab_cnt < cntof(db->tabs));
  assert(!(db->unused & (1llu << idx)));

  db->tabs[db->tab_cnt] = castb(idx);
  return db->tab_cnt++;
}
static void
db_tbl_view_rm(struct db_view *db, int idx) {
  assert(db);
  assert(idx >= 0);
  assert(idx < db->tab_cnt);
  assert(idx < cntof(db->tabs));
  assert(db->tab_cnt < cntof(db->tabs));
  assert(!(db->unused & (1llu << db->tabs[idx])));

  arr_rm(db->tabs, idx, db->tab_cnt);
  db->tab_cnt--;
}
static int
db_tbl_view_fltrs_enabled(struct db_tbl_fltr_view *fltr) {
  assert(fltr);
  for loop(i, fltr->fltr_cnt) {
    struct db_tbl_fltr_elm *elm = &fltr->elms[fltr->lst[i]];
    if (!elm->enabled) {
      return 0;
    }
  }
  return 1;
}
static char*
db_tbl_view_sql(struct db_tbl_view *view, struct sys *_sys, struct arena *mem,
                const struct str sel, int off, int lim) {
  assert(mem);
  assert(view);

  const char *pre = " WHERE";
  char *sql = arena_dyn(mem, _sys, char, KB(16));
  dyn_fmt(sql, _sys, "SELECT %.*s FROM %.*s", strf(sel), strf(view->name));

  for loop(i, view->fltr.fltr_cnt) {
    int idx = view->fltr.lst[i];
    struct db_tbl_fltr_elm *fltr = &view->fltr.elms[idx];
    if (!fltr->enabled) {
      continue;
    }
    struct db_tbl_col *col = &view->cols[fltr->col];
#if 0
    if (fltr->is_date) {
      dyn_fmt(sql, _sys, "%s strftime('%%s',%.*s) BETWEEN '%lld' AND '%lld'", pre,
              strf(col->name), fltr->tm.from_val, fltr->tm.to_val);
    } else {
    }
#endif
    dyn_fmt(sql, _sys, "%s %.*s LIKE '%%%.*s%%'", pre, strf(col->name), strf(fltr->str));
    pre = " AND";
  }
  if (lim > 0) {
    dyn_fmt(sql, _sys, " LIMIT %d, %d", off, lim);
  }
  dyn_add(sql, _sys, ';');
  dyn_add(sql, _sys, '\0');
  return sql;
}
static int
db_tbl_view_row_cnt(struct db_tbl_view *view, struct sys *_sys, sqlite3 *con) {
  assert(con);
  assert(_sys);
  assert(view);

  int ret = 0;
  struct arena_scope scp;
  confine arena_scope(view->tmp_mem, &scp, _sys) {
    sqlite3_stmt *stmt = 0;
    char *sql = db_tbl_view_sql(view, _sys, view->tmp_mem, strv("COUNT(*)"),0,0);
    sqlite3_prepare_v2(con, sql, -1, &stmt, 0);
    sqlite3_step(stmt);
    ret = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    dyn_free(sql, _sys);
  }
  return ret;
}
static void
db_tbl_view_fill(struct db_view *db, struct db_tbl_view *view,
                 struct sys *_sys, int off, int lim) {
  assert(db);
  assert(view);
  if (view->data) {
    arena_scope_pop(&view->scp, &view->mem, _sys);
  }
  struct arena_scope scp;
  confine arena_scope(view->tmp_mem, &scp, _sys) {
    char *sql_buf = 0;
    struct str sql = str_nil;
    sqlite3_stmt *stmt = 0;
    if (view->fltr.fltr_cnt) {
      /* select filtered content */
      view->row_cnt = db_tbl_view_row_cnt(view, _sys, db->con);
      sql_buf = db_tbl_view_sql(view, _sys, view->tmp_mem, str0("*"), off, lim);
      sql = dyn_str(sql_buf);
    } else {
      /* select everything */
      view->row_cnt = view->total;
      sql = arena_fmt(view->tmp_mem, _sys, "SELECT * FROM %.*s LIMIT %d, %d;", strf(view->name), off, lim);
    }
    view->row_begin = clamp(0, off, view->row_cnt);
    view->row_end = min(off + lim, view->row_cnt);
    view->row_begin = max(0, view->row_end - lim);
    arena_scope_push(&view->scp, &view->mem);

    int num = dyn_cnt(view->cols) * lim;
    view->data = arena_arr(&view->mem, _sys, struct str, num);
    sqlite3_prepare_v2(db->con, str_beg(sql), -1, &stmt, 0);

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      for dyn_loop(col, view->cols) {
        if (!view->cols[col].blob) {
          const char *dat = (const char*)sqlite3_column_text(stmt, col);
          if (dat) {
            assert(i < num);
            int len = sqlite3_column_bytes(stmt, col);
            view->data[i] = arena_str(&view->mem, _sys, strn(dat, len));
          }
        }
        i++;
      }
    }
    sqlite3_finalize(stmt);
    if (view->fltr.fltr_cnt) {
      dyn_free(sql_buf, _sys);
    }
  }
}
static void
db_tbl_view_clr(struct db_tbl_view *view) {
  assert(view);
  view->row_cnt = 0;
  view->row_begin = 0;
  view->row_end = 0;
}
static int
db_tbl_view_fnd_pk(const struct db_tbl_view *view) {
  assert(view);
  struct db_tbl_col *col = 0;
  for dyn_each(col, view->cols) {
    if (!col->pk) {
      continue;
    }
    return cast(int, col - view->cols);
  }
  return dyn_cnt(view->cols);
}
static void
db_tbl_view_blob_view(struct db_tbl_view *view, struct db_tbl_blob_view *blob,
                      struct sys *_sys, sqlite3 *con, struct str tbl,
                      struct str pk_col, struct str pk_key, struct str blob_col) {
  assert(con);
  assert(blob);
  assert(view);

  /* load blob memory from database */
  struct arena_scope scp;
  confine arena_scope(view->tmp_mem, &scp, _sys) {
    sqlite3_stmt *stmt = 0;
    struct str sql = arena_fmt(view->tmp_mem, _sys, "SELECT %.*s FROM %.*s WHERE %.*s = %.*s;", strf(blob_col), strf(tbl), strf(pk_col), strf(pk_key));
    sqlite3_prepare_v2(con, str_beg(sql), -1, &stmt, 0);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const void *mem = sqlite3_column_blob(stmt, 0);
      int siz = sqlite3_column_bytes(stmt, 0);
      if (mem && siz) {
        view->state = TBL_VIEW_BLOB_VIEW;
        arena_scope_push(&blob->scp, &view->mem);
        blob->mem = arena_cpy(&view->mem, _sys, mem, siz);
        blob->siz = siz;
      } else {
        fprintf(stderr, "[SQL] blob cell doesn't have any data\n");
      }
    }
    sqlite3_finalize(stmt);
  }
  blob->sel_tab = 0;

  memset(&blob->img, 0, sizeof(blob->img));
  if (blob->mem) {
    /* try loading blob memory as image */
    int chan_cnt = 0;
    unsigned char* img = 0;
    img = img_load_from_memory(blob->mem, blob->siz, &blob->img.w, &blob->img.h, &chan_cnt, 4);
    if (img) {
      blob->img.id = _sys->gfx.tex.load(_sys, GFX_PIX_FMT_R8G8B8A8, img, blob->img.w, blob->img.h);
      blob->sel_tab = 1;
      blob->img.act = 1;
      free(img);
    }
  }
}

/* ---------------------------------------------------------------------------
 *                                Info
 * ---------------------------------------------------------------------------
 */
static int
db_info_qry_cnt(struct db_view *db, enum db_tbl_type tab) {
  assert(con);
  static const char *type[] = {
#define DB_INFO(a,b,c) b,
    DB_TBL_MAP(DB_INFO)
#undef DB_INFO
  };
  sqlite3_stmt *stmt = 0;
  struct str sql = str_fmtsn(db->sql_qry_buf, cntof(db->sql_qry_buf), "SELECT COUNT(*) FROM sqlite_master WHERE type = '%s'", type[tab]);
  sqlite3_prepare_v2(db->con, str_beg(sql), -1, &stmt, 0);
  sqlite3_step(stmt);
  int cnt = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return cnt;
}
static struct db_info_elm*
db_info_elm_new(struct db_info_view *info) {
  assert(info);
  assert(info->elm_cnt < cntof(info->elms));

  int elm_idx = info->elm_cnt++;
  struct db_info_elm *elm = info->elms + elm_idx;
  return elm;
}
static void
db_info_elm_add(struct db_info_view *info, long long rowid, struct str name,
                struct str sql) {
  assert(info);
  assert(info->elm_cnt < cntof(info->elms));

  int nam_cap = cntof(info->buf.name) - info->buf.name_cnt;
  int sql_cap = cntof(info->buf.sql) - info->buf.sql_cnt;

  char *nam_dst = info->buf.name + info->buf.name_cnt;
  char *sql_dst = info->buf.sql + info->buf.sql_cnt;

  struct db_info_elm *elm = db_info_elm_new(info);
  elm->name = str_set(nam_dst, nam_cap, name);
  elm->sql = str_set(sql_dst, sql_cap, sql);
  elm->rowid = rowid;

  info->buf.name_cnt += str_is_val(elm->name) * str_len(elm->name);
  info->buf.sql_cnt += str_is_val(elm->sql) * str_len(elm->sql);
}
static void
db_info_qry_elm(struct db_view *db, struct db_info_view *info, int lo, int hi) {
  assert(db);
  assert(info);
  assert(lo >= 0);
  assert(lo <= hi);
  assert(hi < info->tab_cnt[info->sel_tab]);

  static const char *type[] = {
#define DB_INFO(a,b,c) b,
    DB_TBL_MAP(DB_INFO)
#undef DB_INFO
  };
  sqlite3_stmt *stmt = 0;
  struct str sql = str_fmtsn(db->sql_qry_buf, cntof(db->sql_qry_buf), "SELECT rowid, name, sql FROM sqlite_master WHERE type = '%s' LIMIT %d, %d;", type[info->sel_tab], lo, hi - lo);
  int rc = sqlite3_prepare_v2(db->con, str_beg(sql), -1, &stmt, 0);
  assert(rc == SQLITE_OK);
  int col_cnt = sqlite3_column_count(stmt);
  assert(col_cnt == 2);

  info->elm_cnt = 0;
  info->buf.sql_cnt = 0;
  info->buf.name_cnt = 0;
  info->elm_rng = rng(lo, hi, info->tab_cnt[info->sel_tab]);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    long long rowid = sqlite3_column_int64(stmt, 0);
    const char *tbl_name = (const char*)sqlite3_column_text(stmt, 1);
    const char *tbl_sql = (const char*)sqlite3_column_text(stmt, 2);

    int tbl_name_len = sqlite3_column_bytes(stmt, 1);
    int tbl_sql_len = sqlite3_column_bytes(stmt, 2);

    struct str str_name = strn(tbl_name, tbl_name_len);
    struct str str_sql = strn(tbl_sql, tbl_sql_len);
    db_info_elm_add(info, rowid, str_name, str_sql);
  }
  sqlite3_finalize(stmt);
}
static void
db_info_sel_elms(struct db_view *db, struct db_info_view *info,
                 const struct gui_lst_sel *sel) {
  assert(db);
  assert(sel);
  assert(inf);
  if (sel->mut == GUI_LST_SEL_MOD_REPLACE) {
    tbl_clr(&info->sel);
  }
  if (sel->begin_idx + 1 == sel->end_idx) {
    /* single-selection */
    if (info->sel_tab == DB_TBL_TYPE_TBL || info->sel_tab == DB_TBL_TYPE_VIEW) {
      struct db_info_elm *elm = &info->elms[sel->idx];
      switch (sel->op){
      case GUI_LST_SEL_OP_SET:
        tbl_put(&info->sel, hash_lld(elm->rowid), &elm->rowid); break;
      case GUI_LST_SEL_OP_CLR:
        tbl_del(&info->sel, hash_lld(elm->rowid)); break;
      }
    } else {
      tbl_clr(&info->sel);
    }
    return;
  }
  /* multi-selection */
  sqlite3_stmt *stmt = 0;
  struct str sql = str_fmtsn(db->sql_qry_buf, cntof(db->sql_qry_buf), "SELECT rowid FROM sqlite_master LIMIT %d, %d;", sel->begin_idx, sel->end_idx - sel->begin_idx);
  int rc = sqlite3_prepare_v2(db->con, str_beg(sql), -1, &stmt, 0);
  assert(rc == SQLITE_OK);
  while (sqlite3_step(stmt) == SQLITE_ROW && info->sel.cnt < cntof(info->sel.keys)) {
    long long rowid = sqlite3_column_int64(stmt, 0);
    switch (sel->op){
    case GUI_LST_SEL_OP_SET:
      tbl_put(&info->sel, hash_lld(rowid), &rowid); break;
    case GUI_LST_SEL_OP_CLR:
      tbl_del(&info->sel, hash_lld(rowid)); break;
    }
  }
  sqlite3_finalize(stmt);
}

/* ---------------------------------------------------------------------------
 *                              Database
 * ---------------------------------------------------------------------------
 */
static int
db_init(void *mem, int siz) {
  int rc = sqlite3_config(SQLITE_CONFIG_HEAP, mem, siz, 64);
  return rc == 0;
}
static struct db_view*
db_setup(struct gui_ctx *ctx, struct arena *mem, struct arena *tmp_mem,
         struct str path) {
  assert(db);
  assert(ctx);
  assert(mem);
  assert(tmp_mem);

  struct arena_scope scp = {0};
  arena_scope_push(&scp, mem);
  struct db_view *db = arena_obj(mem, ctx->sys, struct db_view);
  db->path = str_set(db->path_buf, cntof(db->path_buf), path);
  db->tmp_mem = tmp_mem;
  db->unused = ~0llu;

  int rc = sqlite3_open(str_beg(db->path), &db->con);
  if (rc != SQLITE_OK) {
    arena_scope_pop(&scp, mem, ctx->sys);
    return 0;
  }
  int view = db_tbl_view_new(db);
  db_tbl_view_add(db, view);

  struct gui_split_lay_cfg tbl_cfg = {0};
  tbl_cfg.size = szof(struct db_tbl_col_def);
  tbl_cfg.off = offsetof(struct db_tbl_col_def, ui);
  tbl_cfg.slots = db_tree_col_def;
  tbl_cfg.cnt = DB_TREE_COL_MAX;
  gui.tbl.lay(db->info.tbl.state, ctx, &tbl_cfg);
  zero2(db->info.tbl.off);

  for arr_loopv(i, db->info.tab_cnt) {
    db->info.tab_cnt[i] = db_info_qry_cnt(db, i);
    db->info.tab_act |= castu(!!db->info.tab_cnt[i]) << i;
  }
  db->info.sel_tab = cpu_bit_ffs32(db->info.tab_act);
  return db;
}
static void
db_free(struct db_view *db, struct sys *_sys) {
  assert(db);
  assert(_sys);
  /* cleanup tabs */
  unsigned long long used = ~db->unused;
  while (used) {
    int idx = cpu_bit_ffs64(used);
    db_tbl_view_del(db, _sys, idx);
    used = ~db->unused;
  }
  db->tab_cnt = 0;
  arena_free(&db->mem, _sys);
  if (db->con){
    sqlite3_close(db->con);
  }
}
static void
db_tab_resort(struct db_view *db, int dst_idx, int src_idx) {
  assert(db);
  assert(dst_idx >= 0);
  assert(src_idx >= 0);
  assert(dst_idx < db->tab_cnt);
  assert(src_idx < db->tab_cnt);
  assert(dst_idx < cntof(db->tabs));
  assert(src_idx < cntof(db->tabs));
  assert(!(db->unused & (1u << db->tabs[dst_idx])));
  assert(!(db->unused & (1u << db->tabs[src_idx])));
  iswap(db->tabs[dst_idx], db->tabs[src_idx]);
}
static int
db_tab_open(struct db_view *db, int view, enum db_tbl_type type,
            struct str tbl_name, struct sys *_sys, struct gui_ctx *ctx) {
  assert(n);
  assert(db);
  assert(ctx);
  assert(_sys);
  assert(db->unused > 0);
  assert(view >= 0);
  assert(view < cntof(db->tbls));
  assert(db->tab_cnt < cntof(db->tabs));
  assert(!(db->unused & (1llu << view)));
  assert(type == DB_TBL_LST_ELM_TBL || type == DB_TBL_LST_ELM_VIEW);

  db_tbl_view_setup(db, view, _sys, ctx, db->tmp_mem, tbl_name, type);
  db->sel_tab = castb(db_tbl_view_add(db, view));
  return db->sel_tab;
}
static int
db_tab_open_new(struct db_view *db, enum db_tbl_type type,
                struct str tbl_name, struct sys *_sys, struct gui_ctx *ctx) {
  assert(n);
  assert(db);
  assert(ctx);
  assert(_sys);
  assert(db->unused > 0);
  assert(db->tab_cnt < cntof(db->tabs));
  assert(type == DB_TBL_LST_ELM_TBL || type == DB_TBL_LST_ELM_VIEW);

  int view = db_tbl_view_new(db);
  return db_tab_open(db, view, type, tbl_name, _sys, ctx);
}
static int
db_tab_open_empty(struct db_view *db) {
  assert(db);
  assert(db->unused > 0);
  assert(db->tab_cnt < cntof(db->tabs));

  int view = db_tbl_view_new(db);
  int tab_idx = db_tbl_view_add(db, view);
  db_tab_resort(db, 0, tab_idx);
  return db->sel_tab = 0;
}
static void
db_tab_open_tbl_id(struct db_view *db, struct gui_ctx *ctx,
                   int view, long long tbl_id) {
  assert(db);
  assert(db->unused > 0);
  assert(db->tab_cnt < cntof(db->tabs));

  sqlite3_stmt *stmt;
  struct str sql = str_fmtsn(db->sql_qry_buf, cntof(db->sql_qry_buf), "SELECT name FROM sqlite_master WHERE rowid = %lld;", tbl_id);
  sqlite3_prepare_v2(db->con, str_beg(sql), -1, &stmt, 0);
  if (sqlite3_step(stmt) != SQLITE_ROW) {
    return;
  }
  const char *tbl_name = (const char*)sqlite3_column_text(stmt, 0);
  int tbl_name_len = sqlite3_column_bytes(stmt, 0);
  struct str tbl_name_str = strn(tbl_name, tbl_name_len);

  if (db->tbls[view].state != TBL_VIEW_DISPLAY) {
    db_tbl_view_setup(db, view, ctx->sys, ctx, db->tmp_mem, tbl_name_str, db->info.sel_tab);
  } else {
    db_tab_open_new(db, db->info.sel_tab, tbl_name_str, ctx->sys, ctx);
  }
  sqlite3_finalize(stmt);
}
static void
db_tab_open_tbl_sel(struct db_view *db, struct gui_ctx *ctx, int view) {
  assert(db);
  assert(db->unused > 0);
  assert(db->tab_cnt < cntof(db->tabs));
  for tbl_loop(n, i, &db->info.sel) {
    long long rowid = tbl_unref(&db->info.sel, n, 0);
    db_tab_open_tbl_id(db, ctx, view, rowid);
  }
}
static void
db_tab_close(struct db_view *db, struct sys *_sys, int tab_idx) {
  assert(db);
  assert(_sys);
  assert(idx >= 0);
  assert(db->tab_cnt > 0);
  assert(idx < cntof(db->tabs));
  assert(idx < db->tab_cnt);
  assert(db->tab_cnt < cntof(db->tabs));
  assert(!(db->unused & (1llu << db->tabs[tab_idx])));

  db_tbl_view_del(db, _sys, db->tabs[tab_idx]);
  db_tbl_view_rm(db, tab_idx);
  db->sel_tab = clamp(0, db->sel_tab, db->tab_cnt-1);
}

/* ---------------------------------------------------------------------------
 *
 *                                  GUI
 *
 * ---------------------------------------------------------------------------
 */
static int
ui_btn_ico(struct gui_ctx *ctx, struct gui_btn *btn, struct gui_panel *parent,
            struct str txt, enum res_ico_id icon, int uline) {
  assert(ctx);
  assert(btn);
  assert(icon);
  assert(parent);

  static const struct gui_align align = {GUI_HALIGN_MID, GUI_VALIGN_MID};
  gui.btn.begin(ctx, btn, parent);
  {
    struct gui_panel lbl = {.box = btn->pan.box};
    lbl.box.x = gui.bnd.shrink(&btn->pan.box.x, ctx->cfg.pad[0]);
    gui.txt.uln(ctx, &lbl, &btn->pan, txt, &align, uline, 1);

    struct gui_panel ico = {0};
    ico.box.x = gui.bnd.max_ext(lbl.box.x.min - ctx->cfg.gap[0], ctx->cfg.ico);
    ico.box.y = gui.bnd.mid_ext(btn->box.y.mid, ctx->cfg.ico);
    gui.ico.img(ctx, &ico, &btn->pan, icon);
  }
  gui.btn.end(ctx, btn, parent);
  return btn->clk;
}
static int
ui_btn_ico_txt(struct gui_ctx *ctx, struct gui_btn *btn, struct gui_panel *parent,
               struct str txt, enum res_ico_id icon) {
  assert(ctx);
  assert(btn);
  assert(icon);
  assert(parent);

  gui.btn.begin(ctx, btn, parent);
  {
    /* icon */
    struct gui_panel ico = {.box = btn->pan.box};
    ico.box.x = gui.bnd.max_ext(btn->pan.box.x.max, ctx->cfg.item);
    gui.ico.img(ctx, &ico, &btn->pan, icon);

    /* label */
    struct gui_panel lbl = {.box = btn->pan.box};
    lbl.box.x = gui.bnd.min_max(btn->pan.box.x.min + ctx->cfg.pad[0], ico.box.x.min);
    gui.txt.lbl(ctx, &lbl, &btn->pan, txt, 0);
  }
  gui.btn.end(ctx, btn, parent);
  return btn->clk;
}
static void
ui_edit_fnd(struct gui_ctx *ctx, struct gui_edit_box *edt,
               struct gui_panel *pan, struct gui_panel *parent,
               struct gui_txt_ed *ed, char *buf, int cap) {
  assert(ed);
  assert(buf);
  assert(ctx);
  assert(edt);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    static const int pad[2] = {3, 3};
    if (ctx->pass == GUI_RENDER &&
        pan->state != GUI_HIDDEN) {
      gui.edt.drw(ctx, pan);
    }
    /* icon */
    struct gui_panel ico = {0};
    ico.box.y = gui.bnd.shrink(&pan->box.y, pad[1]);
    ico.box.x = gui.bnd.min_ext(pan->box.x.min, ctx->cfg.item);
    gui.ico.img(ctx, &ico, pan, RES_ICO_SEARCH);

    /* edit */
    ed->buf = buf;
    ed->cap = cap;
    ed->str = str_nil;

    edt->pan.focusable = 1;
    edt->pan.box.x = gui.bnd.min_max(ico.box.x.max, pan->box.x.max);
    edt->pan.box.x = gui.bnd.shrink(&edt->pan.box.x, pad[0]);
    edt->pan.box.y = gui.bnd.shrink(&pan->box.y, pad[1]);
    gui.edt.fld(ctx, edt, &edt->pan, pan, ed);
  }
  gui.pan.end(ctx, pan, parent);
}
/* ---------------------------------------------------------------------------
 *                                  FILTER
 * ---------------------------------------------------------------------------
 */
#if 0
static void
ui_db_tbl_fltr_str_view(struct db_view *db, struct db_tbl_view *view,
                        struct gui_ctx *ctx, struct gui_panel *pan,
                        struct gui_panel *parent) {
  assert(db);
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  struct gui_box lay = pan->box;
  struct db_tbl_fltr_view *fltr = &view->fltr;
  gui.pan.begin(ctx, pan, parent);
  {
    /* search expression */
    struct gui_edit_box edt = {.flags = GUI_EDIT_SEL_ON_ACT};
    edt.box = gui.cut.top(&lay, ctx->cfg.item, ctx->cfg.gap[1]);
    gui.edt.box(ctx, &edt, pan, fltr->buf, DB_MAX_FILTER, str_nil);
    if (edt.mod){
      fltr->row_cnt = 0;
      fltr->off[1] = 0;
    }
    /* search list */
    struct gui_lst_cfg cfg = {0};
    gui.lst.cfg(&cfg, fltr->row_cnt, fltr->off[1]);

    struct gui_lst_reg reg = {.box = lay};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, fltr->off);

    int mod = 0;
    mod |= reg.lst.begin != fltr->row_begin;
    mod |= reg.lst.end != fltr->row_end;
    mod |= edt.mod || !fltr->data;

    if (mod) {
      struct db_fltr_cfg param = {0};
      param.tbl = view->name;
      param.col = fltr->ini.col->name;
      param.match = str0(fltr->buf);
      param.off = reg.lst.begin;
      param.lim = reg.lst.cnt;
      db_tbl_view_fltr_init(fltr, &fltr->ini, ctx->sys, &view->mem, db->con, &param);
    }
    int idx = 0;
    for gui_lst_reg_loop(i,gui,&reg) {
      unsigned long long n = cast(unsigned long long, i);
      unsigned long long id = fnv1au64(n, FNV1A64_HASH_INITIAL);

      struct gui_panel elm = {0};
      gui.lst.reg.elm.begin(ctx, &reg, &elm, id, 0);
      if (str_len(fltr->data[idx])) {
        struct gui_panel lbl = {.box = elm.box};
        gui.txt.lbl(ctx, &lbl, &elm, fltr->data[idx], 0);
      }
      gui.lst.reg.elm.end(ctx, &reg, &elm);
      idx++;
    }
    gui.lst.reg.end(ctx, &reg, pan, fltr->off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_tbl_fltr_time_view(struct db_view *db, struct db_tbl_view *view,
                         struct gui_ctx *ctx, struct gui_panel *pan,
                         struct gui_panel *parent) {
  assert(db);
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  struct db_tbl_fltr_view *fltr = &view->fltr;
  gui.pan.begin(ctx, pan, parent);
  {
    int mod = 0;
    struct gui_box lay = pan->box;
    struct gui_box hdr = gui.cut.top(&lay, ctx->cfg.item, ctx->cfg.gap[1]);
    struct gui_box rhs = gui.box.div_x(&hdr, 0, 2, 1);
    {
      /* range from: */
      int gap = ctx->cfg.gap[0];
      struct gui_panel from = {0};
      gui.lbl.txt(ctx, &from, pan, &gui_box_cut(&hdr, GUI_BOX_CUT_LHS, gap), strv("From:"));

      struct gui_edit_box tm_min = {0};
      tm_min.box = gui.cut.lhs(&hdr, 5 * ctx->cfg.item, gap);
      // mod |= gui_time_ctl(ctx, &tm_min, pan, &fltr->ini.tm.from, db->tmp_mem);

      struct gui_edit_box ctl_min = {0};
      ctl_min.box = gui.cut.lhs(&hdr, 5 * ctx->cfg.item, gap);
      // mod |= gui_date_ctl(ctx, &ctl_min, pan, &fltr->ini.tm.from, db->tmp_mem);
    }
    {
      /* range to: */
      int gap = ctx->cfg.gap[0];
      struct gui_panel to = {0};
      gui.lbl.txt(ctx, &to, pan, &gui_box_cut(&rhs, GUI_BOX_CUT_LHS, gap), strv("To:"));

      struct gui_edit_box tm_max = {0};
      tm_max.box = gui.cut.lhs(&rhs, 5 * ctx->cfg.item, gap);
      // mod |= gui_time_ctl(ctx, &tm_max, pan, &fltr->ini.tm.to, db->tmp_mem);

      struct gui_edit_box ctl_max = {0};
      ctl_max.box = gui.cut.lhs(&rhs, 5 * ctx->cfg.item, gap);
      // mod |= gui_date_ctl(ctx, &ctl_max, pan, &fltr->ini.tm.to, db->tmp_mem);
    }
    /* search list */
    struct gui_lst_cfg cfg = {0};
    gui.lst.cfg(&cfg, fltr->row_cnt, fltr->off[1]);

    struct gui_lst_reg reg = {.box = lay};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, fltr->off);

    mod |= reg.lst.begin != fltr->row_begin;
    mod |= reg.lst.end != fltr->row_end;
    mod |= !fltr->data || !fltr->ini.date_init;
    if (mod) {
      struct db_fltr_time_cfg param = {0};
      param.tbl = view->name;
      param.col = fltr->ini.col->name;
      param.off = reg.lst.begin;
      param.lim = reg.lst.cnt;
      db_tbl_view_fltr_tm_init(fltr, &fltr->ini, ctx->sys, &view->mem, db->con, &param);
    }
    int idx = 0;
    for gui_lst_reg_loop(i,gui,&reg) {
      struct gui_panel elm = {0};
      unsigned long long n = cast(unsigned long long, i);
      unsigned long long id = fnv1au64(n, FNV1A64_HASH_INITIAL);
      gui.lst.reg.elm.begin(ctx, &reg, &elm, id, 0);
      if (str_len(fltr->data[idx])) {
        struct gui_panel lbl = {.box = elm.box};
        gui.txt.lbl(ctx, &lbl, &elm, fltr->data[idx], 0);
      }
      gui.lst.reg.elm.end(ctx, &reg, &elm);
      idx++;
    }
    gui.lst.reg.end(ctx, &reg, pan, fltr->off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_fltr_view_sel(struct db_view *d, struct db_tbl_view *view,
                    struct db_tbl_fltr_view *fltr, struct gui_ctx *ctx,
                    struct gui_panel *pan, struct gui_panel *parent) {
  assert(db);
  assert(ctx);
  assert(pan);
  assert(view);
  assert(fltr);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box};
    gui.tab.begin(ctx, &tab, pan, 2, fltr->ini.is_date);
    {
      /* tab header */
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      gui.tab.hdr.item.txt(ctx, &tab, &hdr, strv("Text"));
      gui.tab.hdr.item.txt(ctx, &tab, &hdr, strv("Date & Time"));
      gui.tab.hdr.end(ctx, &tab, &hdr);

      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      if (fltr->ini.is_date) {
        ui_db_tbl_fltr_time_view(d, view, ctx, &bdy, &tab.pan);
      } else {
        ui_db_tbl_fltr_str_view(d, view, ctx, &bdy, &tab.pan);
      }
    }
    gui.tab.end(ctx, &tab, pan);
    if (tab.sel.mod) {
      fltr->ini.is_date = !!tab.sel.idx;
    }
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_fltr_view(struct db_view *d, struct db_tbl_view *view,
                struct gui_ctx *ctx, struct gui_panel *pan,
                struct gui_panel *parent) {
  assert(db);
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  struct db_tbl_fltr_view *fltr = &view->fltr;
  gui.pan.begin(ctx, pan, parent);
  {
    if (!fltr->ini.date_init) {
      /* check if column is a date */
      sqlite3 *con = d->con;
      struct str tbl = view->name;
      struct str col = fltr->ini.col->name;
      db_tbl_view_fltr_time_range_init(&fltr->ini, ctx->sys, view->tmp_mem, con, tbl, col);
      fltr->ini.date_init = 1;
    }
    struct gui_panel bdy = {.box = pan->box};
    if (fltr->ini.hide_date) {
      ui_db_tbl_fltr_str_view(d, view, ctx, &bdy, pan);
    } else {
      ui_db_fltr_view_sel(d, view, fltr, ctx, &bdy, pan);
    }
  }
  gui.pan.end(ctx, pan, parent);
}
#endif

static int
ui_db_tbl_fltr_lst_ico_slot(struct gui_ctx *ctx, struct gui_tbl *tbl,
                            const int *tbl_lay, struct gui_btn *slot,
                            int *state, enum res_ico_id ico) {
  assert(ctx);
  assert(tbl);
  assert(ico);
  assert(slot);
  assert(state);
  assert(tbl_lay);

  gui.tbl.hdr.slot.begin(ctx, tbl, slot);
  {
    struct gui_panel tog = {.box = slot->box};
    gui.ico.img(ctx, &tog, &slot->pan, ico);
  }
  gui.tbl.hdr.slot.end(ctx, tbl, tbl_lay, slot, state);
  return slot->clk;
}
static int
ui_db_tbl_fltr_lst_tog_slot(struct gui_ctx *ctx, struct gui_tbl *tbl,
                            const int *tbl_lay, int *state, int act) {
  assert(ctx);
  assert(tbl);
  assert(state);
  assert(tbl_lay);

  struct gui_btn slot = {0};
  enum res_ico_id ico = act ? RES_ICO_TOGGLE_ON : RES_ICO_TOGGLE_OFF;
  return ui_db_tbl_fltr_lst_ico_slot(ctx, tbl, tbl_lay, &slot, state, ico);
}
static void
ui_db_tbl_fltr_lst_tbl_hdr(struct db_tbl_fltr_view *fltr, struct gui_tbl *tbl,
                           int *tbl_cols, struct gui_ctx *ctx) {
  assert(ctx);
  assert(tbl);
  assert(fltr);
  assert(tbl_cols);

  gui.tbl.hdr.begin(ctx, tbl, tbl_cols, fltr->tbl.state);
  {
    /* enable/disable all filters toggle */
    int all_on = db_tbl_view_fltrs_enabled(fltr);
    if (ui_db_tbl_fltr_lst_tog_slot(ctx, tbl, tbl_cols, fltr->tbl.state, all_on)) {
      for loop(i, fltr->fltr_cnt) {
        fltr->elms[fltr->lst[i]].enabled = !all_on;
      }
    }
    for (int i = 1; i + 1 < cntof(db_tbl_fltr_def); ++i) {
      const struct db_tbl_col_def *col = &db_tbl_fltr_def[i];
      gui.tbl.hdr.slot.txt(ctx, tbl, tbl_cols, fltr->tbl.state, col->title);
    }
    /* delete all filters icon */
    struct gui_btn slot = {0};
    ui_db_tbl_fltr_lst_ico_slot(ctx, tbl, tbl_cols, &slot, fltr->tbl.state, RES_ICO_TRASH);
    if (slot.clk) {
      db_tbl_view_fltr_view_clr(fltr);
    }
  }
  gui.tbl.hdr.end(ctx, tbl);
}
static void
ui_db_tbl_fltr_lst_tog(struct db_tbl_fltr_elm *item, struct gui_ctx *ctx,
                       struct gui_tbl *tbl, const int *tbl_lay,
                       struct gui_panel *elm) {
  assert(ctx);
  assert(tbl);
  assert(elm);
  assert(item);
  assert(tbl_lay);

  struct gui_icon tog = {0};
  int enabled = item->enabled;
  gui.tbl.lst.elm.col.slot(&tog.box, ctx, tbl, tbl_lay);
  {
    gui.ico.clk(ctx, &tog, elm, enabled ? RES_ICO_TOGGLE_ON : RES_ICO_TOGGLE_OFF);
    if (tog.clk) {
      item->enabled = !item->enabled;
    }
  }
}
static void
ui_db_tbl_fltr_lst_view(struct db_tbl_view *view, struct db_tbl_fltr_view *fltr,
                        struct gui_ctx *ctx, struct gui_panel *pan,
                        struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, fltr->tbl.off, &fltr->tbl.sort);
    {
      /* header */
      int tbl_cols[GUI_TBL_COL(DB_TBL_FLTR_MAX)];
      ui_db_tbl_fltr_lst_tbl_hdr(fltr, &tbl, tbl_cols, ctx);

#if 0
      /* sorting */
      if (tbl.resort && dyn_cnt(fltr->lst)) {
        sort_f fn = db_tbl_fltr_def[tbl.sort.col].sort[tbl.sort.order];
        if (fn) {
          dyn_sort(fltr->lst, db_tbl_fltr_def[tbl.sort.col].sort[tbl.sort.order]);
          fltr->tbl.sort = tbl.sort;
        }
      }
#endif

      /* list */
      int del_idx = -1;
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, fltr->fltr_cnt);
      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for gui_tbl_lst_loop(i,gui,&tbl) {
        int idx = fltr->lst[i];
        struct gui_panel elm = {0};
        struct db_tbl_fltr_elm *item = &fltr->elms[idx];
        struct db_tbl_col *col = &view->cols[item->col];

        gui.tbl.lst.elm.begin(ctx, &tbl, &elm, (uintptr_t)item, 0);
        {
          /* columns */
          ui_db_tbl_fltr_lst_tog(item, ctx, &tbl, tbl_cols, &elm);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, item->str, 0);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, col->name, 0);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, col->type, 0);

          /* remove icon */
          struct gui_icon del = {0};
          gui.tbl.lst.elm.col.slot(&del.box, ctx, &tbl, tbl_cols);
          gui.ico.clk(ctx, &del, &elm, RES_ICO_TRASH);
          if (del.clk){
            del_idx = i;
          }
        }
        gui.tbl.lst.elm.end(ctx, &tbl, &elm);
      }
      gui.tbl.lst.end(ctx, &tbl);
      if (del_idx >= 0) {
        db_tbl_view_fltr_view_rm(fltr, del_idx);
      }
    }
    gui.tbl.end(ctx, &tbl, pan, fltr->tbl.off);
  }
  gui.pan.end(ctx, pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  BLOB
 * ---------------------------------------------------------------------------
 */
static dyn(char)
ui_db_hex_view_gen_str(dyn(char) ln, struct sys *_sys, const unsigned char *mem,
                       int siz, int digit_cnt, int col_cnt, int addr) {
  dyn_fmt(ln, _sys, "%0*X: ", digit_cnt, addr);
  for (int col = 0; col < col_cnt && addr + col < siz; ++col) {
    /* generate ascii representation */
    unsigned char byte = mem[addr + col];
    char sym = (byte < 32 || byte >= 127) ? '.' : (char)byte;
    dyn_add(ln, _sys, sym);
  }
  if (addr + col_cnt > siz) {
    /* align up ascii represenation */
    int cnt = addr + col_cnt - siz;
    for loop(i,cnt) {
      dyn_add(ln, _sys, '.');
    }
  }
  dyn_add(ln, _sys, ' ');
  for (int col = 0; col < col_cnt && addr + col < siz; ++col) {
    /* generate hex value representation */
    unsigned char b = mem[addr + col];
    dyn_fmt(ln, _sys, "%.2X ", b);
  }
  return ln;
}
static void
ui_db_hex_view(struct db_tbl_view *view, struct db_tbl_blob_view *blob,
               struct gui_ctx *ctx, struct gui_panel *pan,
               struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(blob);
  assert(view);
  assert(parent);

  int col_cnt = 24;
  int digit_cnt = 0;
  int line_cnt = (blob->siz + col_cnt - 1) / col_cnt;
  for (int n = blob->siz - 1; n > 0; n >>= 4) {
    digit_cnt++;
  }
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_lst_cfg cfg = {0};
    gui.lst.cfg(&cfg, line_cnt, blob->off[1]);
    cfg.sel.on = GUI_LST_SEL_ON_HOV;
    cfg.sel.src = GUI_LST_SEL_SRC_EXT;
    cfg.ctl.show_cursor = 0;

    struct gui_lst_reg reg = {.box = pan->box};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, blob->off);
    for gui_lst_reg_loop(i,gui,&reg) {
      int addr = i * col_cnt;

      struct arena_scope scp;
      confine arena_scope(view->tmp_mem, &scp, ctx->sys) {
        /* generate and display each line of hex/ascii data representation */
        struct gui_panel elm = {0};
        char *ln = arena_dyn(view->tmp_mem, ctx->sys, char, KB(8));
        ln = ui_db_hex_view_gen_str(ln, ctx->sys, blob->mem, blob->siz, digit_cnt, col_cnt, addr);
        unsigned long long elm_id = hash_ptr(blob->mem + addr);
        gui.lst.reg.elm.txt(ctx, &reg, &elm, elm_id, 0, dyn_str(ln), 0);
        dyn_free(ln, ctx->sys);
      }
    }
    gui.lst.reg.end(ctx, &reg, pan, blob->off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_img_view(struct db_tbl_blob_img_view *img,
               struct gui_ctx *ctx, struct gui_panel *pan,
               struct gui_panel *parent) {
  assert(img);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_reg reg = {.box = pan->box};
    gui.reg.begin(ctx, &reg, pan, img->off);
    {
      struct gui_panel ico = {.box = reg.pan.box};
      ico.box.x = gui.bnd.min_ext(ico.box.x.min, img->w);
      ico.box.y = gui.bnd.min_ext(ico.box.y.min, img->h);
      if (ctx->pass == GUI_RENDER) {
        gui.drw.blit(ctx, img->id, ico.box.x.min, ico.box.y.min);
      }
    }
    gui.reg.end(ctx, &reg, pan, img->off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_blob_view(struct db_tbl_view *view, struct db_tbl_blob_view *blob,
                struct gui_ctx *ctx, struct gui_panel *pan,
                struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(blob);
  assert(view);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    int tab_cnt = 1 + blob->img.act != 0;
    struct gui_tab_ctl tab = {.box = pan->box};
    gui.tab.begin(ctx, &tab, pan, tab_cnt, blob->sel_tab);
    {
      /* tab header */
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      {
        gui.tab.hdr.item.txt(ctx, &tab, &hdr, strv("Hex"));
        if (blob->img.act) {
          gui.tab.hdr.item.txt(ctx, &tab, &hdr, strv("Image"));
        }
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);
      if (tab.sel.mod) {
        blob->sel_tab = tab.sel.idx;
      }
      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      if (blob->sel_tab == 0) {
        ui_db_hex_view(view, blob, ctx, &bdy, &tab.pan);
      } else {
        ui_db_img_view(&blob->img, ctx, &bdy, &tab.pan);
      }
    }
    gui.tab.end(ctx, &tab, pan);
  }
  gui.pan.end(ctx, pan, parent);
}
/* ---------------------------------------------------------------------------
 *                                  TABLE
 * ---------------------------------------------------------------------------
 */
static enum res_ico_id
ui_db_tbl_lst_elm_ico(enum db_tbl_type type) {
  switch (type) {
  case DB_TBL_TYPE_TBL: return RES_ICO_TABLE;
  case DB_TBL_TYPE_VIEW: return RES_ICO_IMAGE;
  case DB_TBL_TYPE_IDX: return RES_ICO_TAG;
  case DB_TBL_TYPE_TRIGGER: return RES_ICO_BOLT;
  }
  return RES_ICO_DATABASE;
}
static void
ui_db_tbl_view_hdr_key_slot(struct db_tbl_view *view, struct db_tbl_col *col,
                            struct gui_ctx *ctx, struct gui_tbl *tbl,
                            const int *lay) {
  assert(col);
  assert(ctx);
  assert(tbl);
  assert(lay);
  assert(view);

  struct gui_btn slot = {0};
  gui.tbl.hdr.slot.begin(ctx, tbl, &slot);
  {
    struct gui_cfg_stk stk[1] = {0};
    unsigned fk_col = ctx->cfg.col[GUI_COL_TXT_DISABLED];
    confine gui_cfg_pushu_scope(&gui,stk,&ctx->cfg.col[GUI_COL_ICO], fk_col) {
      /* icon */
      struct gui_panel ico = {.box = slot.box};
      ico.box.x = gui.bnd.max_ext(slot.box.x.max, ctx->cfg.item);
      gui.ico.img(ctx, &ico, &slot.pan, RES_ICO_KEY);
      /* label */
      struct gui_panel lbl = {.box = slot.box};
      lbl.box.x = gui.bnd.min_max(slot.box.x.min, ico.box.x.min);
      gui.txt.lbl(ctx, &lbl, &slot.pan, col->name, 0);
    }
  }
  gui.tbl.hdr.slot.end(ctx, tbl, lay, &slot, view->ui.state);
}
static void
ui_db_tbl_view_hdr_lnk_slot(struct db_tbl_view *view,
                            struct db_tbl_col *col, struct gui_ctx *ctx,
                            struct gui_tbl *tbl, const int *tbl_lay) {
  assert(col);
  assert(ctx);
  assert(tbl);
  assert(view);
  assert(tlb_lay);

  struct gui_btn slot = {0};
  gui.tbl.hdr.slot.begin(ctx, tbl, &slot);
  {
    /* table column header filter icon button */
    struct gui_btn fltr = {.box = slot.pan.box};
    fltr.box.x = gui.bnd.max_ext(slot.pan.box.x.max, ctx->cfg.item);
    confine gui_disable_on_scope(&gui, ctx, col->blob) {
      if (gui.btn.ico(ctx, &fltr, &slot.pan, RES_ICO_SEARCH)) {
#if 0
        view->state = TBL_VIEW_FILTER;
        view->fltr.ini.col = col;
#endif
      }
    }
    /* header label with foreign key icon */
    struct gui_cfg_stk stk[1] = {0};
    unsigned fk_col = ctx->cfg.col[GUI_COL_TXT_DISABLED];
    confine gui_cfg_pushu_scope(&gui,stk,&ctx->cfg.col[GUI_COL_ICO], fk_col) {
      struct gui_btn hdr = {.box = slot.pan.box};
      hdr.box.x = gui.bnd.min_max(slot.pan.box.x.min, fltr.box.x.min);
      ui_btn_ico_txt(ctx, &hdr, &slot.pan, col->name, RES_ICO_LINK);
    }
  }
  gui.tbl.hdr.slot.end(ctx, tbl, tbl_lay, &slot, view->ui.state);
}
static void
ui_db_tbl_view_hdr_slot(struct db_tbl_view *view, struct db_tbl_col *col,
                        struct gui_ctx *ctx, struct gui_tbl *tbl,
                        const int *tbl_lay) {
  assert(col);
  assert(ctx);
  assert(tbl);
  assert(view);
  assert(tbl_lay);

  struct gui_btn slot = {0};
  gui.tbl.hdr.slot.begin(ctx, tbl, &slot);
  {
    /* table column header filter icon button */
    struct gui_btn fltr = {.box = slot.pan.box};
    fltr.box.x = gui.bnd.max_ext(slot.pan.box.x.max, ctx->cfg.item);
    confine gui_disable_on_scope(&gui, ctx, col->blob) {
      if (gui.btn.ico(ctx, &fltr, &slot.pan, RES_ICO_SEARCH)) {
#if 0
        view->state = TBL_VIEW_FILTER;
        view->fltr.ini.col = col;
#endif
      }
    }
    static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
    struct gui_btn hdr = {.box = slot.pan.box};
    hdr.box.x = gui.bnd.min_max(slot.pan.box.x.min, fltr.box.x.min);
    gui.btn.txt(ctx, &hdr, &slot.pan, col->name, &align);
  }
  gui.tbl.hdr.slot.end(ctx, tbl, tbl_lay, &slot, view->ui.state);
}
static void
ui_db_tbl_view_lst_elm_blob(struct db_view *sql, struct db_tbl_view *view,
                            struct gui_ctx *ctx, struct gui_panel *col,
                            struct db_tbl_col *meta, const struct str *data) {
  assert(sql);
  assert(ctx);
  assert(col);
  assert(meta);
  assert(view);
  assert(data);

  if (col->is_hot && !view->blob.disabled) {
    struct gui_box lay = col->box;
    struct gui_box ico = gui.cut.lhs(&lay, ctx->cfg.item, ctx->cfg.gap[0]);
    /* show icon to open hex view on column hover */
    struct gui_icon btn = {.box = ico};
    gui.ico.clk(ctx, &btn, col, RES_ICO_IMPORT);
    if (btn.clk) {
      int pki = db_tbl_view_fnd_pk(view);
      if (pki < dyn_cnt(view->cols)) {
        struct db_tbl_col *pk = &view->cols[pki];
        db_tbl_view_blob_view(view, &view->blob, ctx->sys, sql->con,
          view->name, pk->name, data[pki], meta->name);
      }
    }
    confine gui_disable_on_scope(&gui, ctx, 1) {
      struct gui_panel lbl = {.box = lay};
      gui.txt.lbl(ctx, &lbl, col, strv("blob"), 0);
    }
  } else {
    /* show dummy text for blob data */
    confine gui_disable_on_scope(&gui, ctx, 1) {
      struct gui_panel lbl = {.box = col->box};
      gui.txt.lbl(ctx, &lbl, col, strv("blob"), 0);
    }
  }
}
static void
ui_db_tbl_view_lst_elm(struct db_view *db, struct db_tbl_view *view,
                       struct gui_ctx *ctx, struct gui_tbl *tbl,
                       struct gui_panel *elm, const int *tbl_cols,
                       const struct str *data, unsigned long long id){
  assert(db);
  assert(ctx);
  assert(tbl);
  assert(elm);
  assert(data);
  assert(view);
  assert(tbl_cols);

  gui.tbl.lst.elm.begin(ctx, tbl, elm, id, 0);
  for dyn_loop(i, view->cols) {
    assert(i < cntof(view->cols));
    struct db_tbl_col *meta = &view->cols[i];
    if (str_len(data[i])) {
      gui.tbl.lst.elm.col.txt(ctx, tbl, tbl_cols, elm, data[i], 0);
      continue;
    }
    struct gui_panel col = {0};
    gui.tbl.lst.elm.col.slot(&col.box, ctx, tbl, tbl_cols);
    gui.pan.begin(ctx, &col, elm);
    if (meta->blob) {
      ui_db_tbl_view_lst_elm_blob(db, view, ctx, &col, meta, data);
    } else {
      confine gui_disable_on_scope(&gui, ctx, 1) {
        struct gui_panel lbl = {.box = col.box};
        gui.txt.lbl(ctx, &lbl, &col, strv("null"), 0);
      }
    }
    gui.pan.end(ctx, &col, elm);
  }
  gui.tbl.lst.elm.end(ctx, tbl, elm);
}
static void
ui_db_tbl_view_lst_hdr(struct db_tbl_view *view, struct gui_ctx *ctx,
                       struct gui_tbl *tbl, int *tbl_lay, int *state) {
  assert(ctx);
  assert(tbl);
  assert(view);
  assert(state);
  assert(tbl_lay);

  struct db_tbl_col *col = 0;
  gui.tbl.hdr.begin(ctx, tbl, tbl_lay, view->ui.state);
  for dyn_each(col, view->cols) {
    if (col->pk) {
      ui_db_tbl_view_hdr_key_slot(view, col, ctx, tbl, tbl_lay);
    } else if (col->fk) {
      ui_db_tbl_view_hdr_lnk_slot(view, col, ctx, tbl, tbl_lay);
    } else if (col->blob) {
      gui.tbl.hdr.slot.txt(ctx, tbl, tbl_lay, state, col->name);
    } else {
      ui_db_tbl_view_hdr_slot(view, col, ctx, tbl, tbl_lay);
    }
  }
  gui.tbl.hdr.end(ctx, tbl);
}
static void
ui_db_tbl_view_lst(struct db_view *db, struct db_tbl_view *view,
                   struct gui_ctx *ctx, struct arena *tmp_mem,
                   struct gui_panel *pan, struct gui_panel *parent) {
  assert(db);
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);
  assert(tmp_mem);

  struct arena_scope scp;
  confine arena_scope(tmp_mem, &scp, ctx->sys)
  {
    gui.pan.begin(ctx, pan, parent);
    {
      struct gui_tbl tbl = {.box = pan->box};
      gui.tbl.begin(ctx, &tbl, pan, view->ui.off, &view->ui.sort);
      {
        /* header */
        int word_cnt = GUI_TBL_COL(dyn_cnt(view->cols));
        int *tbl_lay = arena_arr(tmp_mem, ctx->sys, int, word_cnt);
        ui_db_tbl_view_lst_hdr(view, ctx, &tbl, tbl_lay, view->ui.state);

        /* list */
        struct gui_tbl_lst_cfg cfg = {0};
        gui.tbl.lst.cfg(ctx, &cfg, view->row_cnt);
        gui.tbl.lst.begin(ctx, &tbl, &cfg);

        int mod = 0;
        mod |= tbl.lst.begin != view->row_begin;
        mod |= tbl.lst.end != view->row_end;
        mod |= view->rev != view->fltr.rev;
        if (mod) db_tbl_view_fill(db, view, ctx->sys, tbl.lst.begin, tbl.lst.cnt);

        int idx = 0;
        for gui_tbl_lst_loop(i,gui,&tbl) {
          unsigned long long n = cast(unsigned long long, i);
          unsigned long long id = fnv1au64(n, FNV1A64_HASH_INITIAL);
          struct str *row_data = view->data + idx;

          struct gui_panel elm = {0};
          ui_db_tbl_view_lst_elm(db, view, ctx, &tbl, &elm, tbl_lay, row_data, id);
          idx += dyn_cnt(view->cols);
        }
        gui.tbl.lst.end(ctx, &tbl);
      }
      gui.tbl.end(ctx, &tbl, pan, view->ui.off);
    }
    gui.pan.end(ctx, pan, parent);
  }
}

/* ---------------------------------------------------------------------------
 *                                  INFO
 * ---------------------------------------------------------------------------
 */
static void
ui_db_view_info_tbl(struct db_view *db, struct db_info_view *info,
                    struct gui_ctx *ctx, struct gui_panel *pan,
                    struct gui_panel *parent) {
  assert(t);
  assert(db);
  assert(ctx);
  assert(pan);
  assert(parent);

  static const struct str type[] = {
#define DB_INFO(a,b,c) strv(b),
    DB_TBL_MAP(DB_INFO)
#undef DB_INFO
  };
  enum res_ico_id ico[] = {
  #define DB_INFO(a,b,c) c,
    DB_TBL_MAP(DB_INFO)
  #undef DB_INFO
  };
  gui.pan.begin(ctx, pan, parent);
  {
    int gap = ctx->cfg.gap[1];
    struct gui_box lay = pan->box;
    struct gui_panel fltr = {.box = gui.cut.top(&lay, ctx->cfg.item, gap)};
    struct gui_edit_box edt = {.box = fltr.box};
#if 0
    ui_edit_fnd(ctx, &edt, &fltr, pan, &d->tree.fnd_ed, d->tree.fnd_buf, DB_MAX_FILTER);
#endif

    struct gui_tbl tbl = {.box = lay};
    gui.tbl.begin(ctx, &tbl, pan, info->tbl.off, 0);
    {
      /* header */
      const struct db_tbl_col_def *col = 0;
      int tbl_cols[GUI_TBL_COL(DB_TREE_COL_MAX)];
      gui.tbl.hdr.begin(ctx, &tbl, tbl_cols, info->tbl.state);
      for arr_eachv(col, db_tree_col_def) {
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_cols, info->tbl.state, col->title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, info->tab_cnt[info->sel_tab]);
      cfg.sel.mode = GUI_LST_SEL_MULTI;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      if (tbl.lst.begin != info->elm_rng.lo ||
          tbl.lst.end != info->elm_rng.hi) {
        db_info_qry_elm(db, &db->info, tbl.lst.begin, tbl.lst.end);
      }
      for gui_tbl_lst_loop(i,gui,&tbl) {
        struct gui_panel item = {0};
        struct db_info_elm *elm = &info->elms[i-tbl.lst.begin];
        int is_sel = tbl_has(&info->sel, hash_lld(elm->rowid));
        gui.tbl.lst.elm.begin(ctx, &tbl, &item, str_hash(elm->name), is_sel);
        {
          gui.tbl.lst.elm.col.txt_ico(ctx, &tbl, tbl_cols, &item, elm->name, ico[info->sel_tab]);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &item, type[info->sel_tab], 0);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &item, elm->sql, 0);
        }
        gui.tbl.lst.elm.end(ctx, &tbl, &item);
      }
      gui.tbl.lst.end(ctx, &tbl);
      if (tbl.lst.sel.mod) {
        db_info_sel_elms(db, info, &tbl.lst.sel);
      }
    }
    gui.tbl.end(ctx, &tbl, pan, info->tbl.off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_view_info(struct db_view *db, struct db_info_view *info,
                struct gui_ctx *ctx, struct gui_panel *pan,
                struct gui_panel *parent) {
  assert(db);
  assert(info);
  assert(ctx);
  assert(pan);
  assert(parent);

  static const struct str title[] = {
  #define DB_INFO(a,b,c) strv(b),
    DB_TBL_MAP(DB_INFO)
  #undef DB_INFO
  };
  enum res_ico_id ico[] = {
  #define DB_INFO(a,b,c) c,
    DB_TBL_MAP(DB_INFO)
  #undef DB_INFO
  };
  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box};
    gui.tab.begin(ctx, &tab, pan, cntof(db->info.tab_cnt), casti(db->info.sel_tab));
    {
      /* tab header */
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      for loop(i,tab.cnt) {
        /* tab header slots */
        int dis = !(db->info.tab_act & (1u << i));
        confine gui_disable_on_scope(&gui, ctx, dis) {
          struct gui_panel slot = {0};
          gui.tab.hdr.slot.begin(ctx, &tab, &hdr, &slot, str_hash(title[i]));
          gui.ico.box(ctx, &slot, &hdr.pan, ico[i], title[i]);
          gui.tab.hdr.slot.end(ctx, &tab, &hdr, &slot, 0);
        }
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);
      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      ui_db_view_info_tbl(db, info, ctx, &bdy, &tab.pan);
    }
    gui.tab.end(ctx, &tab, pan);
    if (tab.sel.mod) {
      db->info.sel_tab = castb(tab.sel.idx);
      tbl_clr(&db->info.sel);
    }
  }
  gui.pan.end(ctx, pan, parent);
}
/* ---------------------------------------------------------------------------
 *                                DATABASE
 * ---------------------------------------------------------------------------
 */
static void
ui_db_view_tab(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                   struct gui_tab_ctl_hdr *hdr, struct str title,
                   enum res_ico_id ico) {
  assert(ctx);
  assert(tab);
  assert(hdr);
  assert(title.ptr);

  struct gui_panel slot = {0};
  gui.tab.hdr.slot.begin(ctx, tab, hdr, &slot, str_hash(title));
  gui.ico.box(ctx, &slot, &hdr->pan, ico, title);
  gui.tab.hdr.slot.end(ctx, tab, hdr, &slot, 0);
}
static void
ui_db_main(struct db_view *db, int view, struct gui_ctx *ctx,
           struct gui_panel *pan, struct gui_panel *parent) {
  assert(db);
  assert(ctx);
  assert(pan);
  assert(parent);
  assert(view >= 0);
  assert(view < cntof(db->tbls));
  assert(!(db->unused & (1llu << view)));

  struct db_tbl_view *tbl = &db->tbls[view];
  gui.pan.begin(ctx, pan, parent);
  {
    int gap = ctx->cfg.gap[1];
    struct gui_box lay = pan->box;
    switch (tbl->state) {
    case TBL_VIEW_SELECT: {
      struct gui_btn open = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      struct gui_panel overview = {.box = lay};
      ui_db_view_info(db, &db->info, ctx, &overview, pan);

      /* open table */
      int dis = !db->unused || !db->info.sel.cnt;
      confine gui_disable_on_scope(&gui, ctx, dis) {
        if (ui_btn_ico(ctx, &open, pan, strv("Open"), RES_ICO_TH_LIST, 0)) {
          db_tab_open_tbl_sel(db, ctx, view);
          tbl_clr(&db->info.sel);
        }
      }
    } break;

    case TBL_VIEW_DISPLAY: {
      /* layout clear button */
      struct gui_btn clr = {0};
      clr.box.x = gui.bnd.max_ext(pan->box.x.max, ctx->cfg.item);
      clr.box.y = gui.bnd.max_ext(pan->box.y.max, ctx->cfg.item);
      {
        confine gui_disable_on_scope(&gui, ctx, !tbl->fltr.fltr_cnt) {
          if (gui.btn.ico(ctx, &clr, pan, RES_ICO_TRASH)) {
            db_tbl_view_fltr_view_clr(&tbl->fltr);
          }
        }
      }
      /* jump to filter view button */
      struct gui_btn fltr = {.box = clr.box};
      fltr.box.x = gui.bnd.min_max(pan->box.x.min, clr.box.x.min);
      if (ui_btn_ico(ctx, &fltr, pan, strv("Filters"), RES_ICO_MENU, 0)) {
        tbl->state = TBL_VIEW_FILTER_LIST;
      }
      /* table view */
      struct gui_panel lst = {.box = pan->box};
      lst.box.y = gui.bnd.min_max(pan->box.y.min, fltr.box.y.min - ctx->cfg.pad[1]);
      ui_db_tbl_view_lst(db, tbl, ctx, db->tmp_mem, &lst, pan);
    } break;

    case TBL_VIEW_FILTER_LIST: {
      /* back button */
      struct gui_btn back = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      if (ui_btn_ico(ctx, &back, pan, strv("Back"), RES_ICO_TH_LIST, 0)) {
        tbl->state = TBL_VIEW_DISPLAY;
      }
      /* filter list view */
      struct gui_panel lst = {.box = lay};
      ui_db_tbl_fltr_lst_view(tbl, &tbl->fltr, ctx, &lst, pan);
    } break;

    case TBL_VIEW_FILTER: {
#if 0
      struct gui_btn back = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      struct gui_btn apply = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      struct gui_panel fltr = {.box = lay};
      ui_db_fltr_view(db, tbl, ctx, &fltr, pan);

      /* back button */
      if (ui_btn_ico(ctx, &back, pan, strv("Back"), RES_ICO_MENU, 0)) {
        tbl->state = TBL_VIEW_FILTER_LIST;
        db_tbl_view_fltr_clr(tbl, &tbl->fltr, ctx->sys);
        db_tbl_view_clr(tbl);
      }
      /* apply button */
      int dis = !tbl->fltr.ini.is_date && dyn_empty(tbl->fltr.buf);
      confine gui_disable_on_scope(&gui, ctx, dis) {
        if (ui_btn_ico(ctx, &apply, pan, strv("Apply"), RES_ICO_CHECK, 0)) {
          tbl->fltr.ini.enabled = 1;
          tbl->fltr.ini.str = arena_str(&tbl->mem, ctx->sys, dyn_str(tbl->fltr.buf));
          dyn_add(tbl->fltr.lst, ctx->sys, tbl->fltr.ini);

          db_tbl_view_fltr_clr(tbl, &tbl->fltr, ctx->sys);
          db_tbl_view_clr(tbl);

          tbl->state = TBL_VIEW_DISPLAY;
          tbl->fltr.rev++;
        }
      }
#endif
    } break;

    case TBL_VIEW_BLOB_VIEW: {
      /* back button */
      struct gui_btn back = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      if (ui_btn_ico(ctx, &back, pan, strv("Back"), RES_ICO_TH_LIST, 0)) {
        if (tbl->blob.img.act) {
          struct sys *s = ctx->sys;
          s->gfx.tex.del(s, tbl->blob.img.id);
          mset(&tbl->blob.img, 0, szof(tbl->blob.img));
        }
        arena_scope_pop(&tbl->blob.scp, &tbl->mem, ctx->sys);
        tbl->state = TBL_VIEW_DISPLAY;
      }
      struct gui_panel hex = {.box = lay};
      ui_db_blob_view(tbl, &tbl->blob, ctx, &hex, pan);
    } break;
    }
  }
  gui.pan.end(ctx, pan, parent);
}
static int
ui_db_explr_tab_slot_close(struct gui_ctx *ctx, struct gui_panel *pan,
                             struct gui_panel *parent, struct str title,
                             enum res_ico_id ico) {
  assert(ctx);
  assert(pan);
  assert(parent);
  assert(ico);

  int ret = 0;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_box lay = pan->box;
    struct gui_icon close = {.box = gui.cut.rhs(&lay, ctx->cfg.item, 0)};
    gui.ico.clk(ctx, &close, pan, RES_ICO_CLOSE);
    ret = close.clk;

    struct gui_panel lbl = {.box = lay};
    gui.ico.box(ctx, &lbl, pan, ico, title);
  }
  gui.pan.end(ctx, pan, parent);
  return ret;
}
static int
ui_db_explr_tab_slot(struct db_view *db, struct db_tbl_view *tbl,
                     struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                     struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot,
                     struct str title, enum res_ico_id ico) {
  assert(db);
  assert(tbl);
  assert(ctx);
  assert(tab);
  assert(hdr);
  assert(slot);
  assert(ico);

  int ret = 0;
  unsigned long long tab_id = hash_ptr(tbl);
  gui.tab.hdr.slot.begin(ctx, tab, hdr, slot, tab_id);
  if (db->tab_cnt > 1 && tab->idx == tab->sel.idx) {
    ret = ui_db_explr_tab_slot_close(ctx, slot, &hdr->pan, title, ico);
  } else {
    gui.ico.box(ctx, slot, &hdr->pan, ico, title);
  }
  gui.tab.hdr.slot.end(ctx, tab, hdr, slot, 0);
  return ret;
}
static int
ui_db_explr_tab(struct db_view *d, struct db_tbl_view *tbl,
                struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot) {
  assert(db);
  assert(tbl);
  assert(ctx);
  assert(tab);
  assert(hdr);
  assert(slot);

  struct str title = strv("Info");
  enum res_ico_id ico = RES_ICO_CUBE;
  if (tbl->state != TBL_VIEW_SELECT) {
    ico = ui_db_tbl_lst_elm_ico(tbl->kind);
    title = tbl->name;
  }
  return ui_db_explr_tab_slot(d, tbl, ctx, tab, hdr, slot, title, ico);
}
static int
ui_db_tab_view_lst(struct db_view *db, struct gui_ctx *ctx,
                   struct gui_panel *pan, struct gui_panel *parent) {

  assert(app);
  assert(ctx);
  assert(tab);
  assert(pan);
  assert(parent);

  int ret = -1;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_lst_cfg cfg = {0};
    gui.lst.cfg(&cfg, db->tab_cnt, db->tbl_lst_off[1]);
    cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
    cfg.sel.on = GUI_LST_SEL_ON_HOV;

    struct gui_lst_reg reg = {.box = pan->box};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, db->tbl_lst_off);
    for gui_lst_reg_loop(i,gui,&reg) {
      struct db_tbl_view *tbl = &db->tbls[db->tabs[i]];
      struct str title = strv("Info");
      enum res_ico_id ico = RES_ICO_CUBE;
      if (tbl->state != TBL_VIEW_SELECT) {
        ico = ui_db_tbl_lst_elm_ico(tbl->kind);
        title = tbl->name;
      }
      struct gui_panel elm = {0};
      unsigned long long n = cast(unsigned long long, i);
      unsigned long long id = fnv1au64(n, FNV1A64_HASH_INITIAL);
      gui.lst.reg.elm.txt_ico(ctx, &reg, &elm, id, 0, title, ico);

      struct gui_input in = {0};
      gui.pan.input(&in, ctx, &elm, GUI_BTN_LEFT);
      ret = in.mouse.btn.left.clk ? i : ret;
    }
    gui.lst.reg.end(ctx, &reg, pan, db->tbl_lst_off);
  }
  gui.pan.end(ctx, pan, parent);
  return ret;
}
static void
ui_db_explr(struct db_view *db, struct gui_ctx *ctx,
            struct gui_panel *pan, struct gui_panel *parent) {
  assert(db);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box, .show_btn = 1};
    gui.tab.begin(ctx, &tab, pan, db->tab_cnt, db->sel_tab);
    {
      /* tab header */
      int del_tab = 0;
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      for loop(i, tab.cnt) {
        /* tab header slots */
        struct gui_panel slot = {0};
        struct db_tbl_view *tbl = &db->tbls[db->tabs[i]];
        if (ui_db_explr_tab(db, tbl, ctx, &tab, &hdr, &slot)) {
          del_tab = 1;
        }
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);
      if (tab.sort.mod) {
        db_tab_resort(db, tab.sort.dst, tab.sort.src);
      }
      if (del_tab) {
        /* close table view tab */
        db_tab_close(db, ctx->sys, del_tab);
      }
      confine gui_disable_on_scope(&gui, ctx, db->unused == 0) {
        struct gui_btn add = {.box = hdr.pan.box};
        add.box.x = gui.bnd.min_ext(tab.off, ctx->cfg.item);
        if (gui.btn.ico(ctx, &add, &hdr.pan, RES_ICO_FOLDER_ADD)) {
          /* open new table view tab */
          db_tab_open_empty(db);
        }
      }
      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      db->show_tab_lst = tab.btn.clk ? !db->show_tab_lst : db->show_tab_lst;
      if (db->show_tab_lst) {
        /* overflow tab selection */
        int ret = ui_db_tab_view_lst(db, ctx, &bdy, pan);
        if (ret >= 0) {
          db_tab_resort(db, 0, ret);
          db->show_tab_lst = 0;
          db->sel_tab = 0;
        }
      } else {
        assert(db->sel_tab < cntof(db->tab_cnt));
        ui_db_main(db, db->tabs[db->sel_tab], ctx, &bdy, pan);
      }
    }
    gui.tab.end(ctx, &tab, pan);
    if (tab.sel.mod) {
      db->sel_tab = castb(tab.sel.idx);
    }
  }
  gui.pan.end(ctx, pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
static const struct db_api db__api = {
  .init = db_init,
  .new = db_setup,
  .del = db_free,
  .ui = ui_db_explr,
};
static void
db_api(void *export, void *import) {
  unused(import);
  struct db_api *exp = cast(struct db_api*, export);
  *exp = db__api;
}

