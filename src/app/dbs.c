/* ---------------------------------------------------------------------------
 *
 *                                Database
 *
 * ---------------------------------------------------------------------------
 */
#define db_str(s) str_beg(s), str_len(s)
#define db_loopn(i,e,stmt,n)\
  (int i = 0; ((e) = sqlite3_step(stmt)) == SQLITE_ROW && (i) < (n); ++(i))
#define db_loop(i,e,stmt,n) db_loopn(i,e,stmt,cntof(n))

// clang-format off
static const struct db_tbl_col_def db_tbl_fltr_def[DB_TBL_FLTR_MAX] = {
  [DB_TBL_FLTR_STATE]     = {.title = strv(""),       .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {10, 200}}},
  [DB_TBL_FLTR_BUF]       = {.title = strv("Filter"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 800}}},
  [DB_TBL_FLTR_COL]       = {.title = strv("Column"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 800}}},
  [DB_TBL_FLTR_DEL]       = {.title = strv(""),       .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {10, 200}}},
};
static const struct db_tbl_col_def db_tbl_fltr_col_def[DB_TBL_FLTR_COL_MAX] = {
  [DB_TBL_FLTR_COL_NAME]  = {.title = strv("Name"),  .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {100, 400}}},
  [DB_TBL_FLTR_COL_TYPE]  = {.title = strv("Type"),  .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {100, 400}}},
};
static const struct db_tbl_col_def db_tree_col_def[DB_TREE_COL_MAX] = {
  [DB_TREE_COL_NAME]      = {.title = strv("Name"),   .ui = {.type = GUI_LAY_SLOT_FIX, .size = 400, .con = {50, 1000}}},
  [DB_TREE_COL_TYPE]      = {.title = strv("Type"),   .ui = {.type = GUI_LAY_SLOT_FIX, .size = 200, .con = {50, 1000}}},
  [DB_TREE_COL_SQL]       = {.title = strv("Schema"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {50, 1000}}},
};
static const struct db_tbl_col_def db_tbl_disp_col_def[DB_TBL_DISP_COL_MAX] = {
  [DB_TBL_DISP_COL_ACT]   = {.title = strv(""),       .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
  [DB_TBL_DISP_COL_NAME]  = {.title = strv("Name"),   .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 800}}},
  [DB_TBL_DISP_COL_TYPE]  = {.title = strv("Type"),   .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 800}}},
  [DB_TBL_DISP_COL_PK]    = {.title = strv("PK"),     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
  [DB_TBL_DISP_COL_FK]    = {.title = strv("FK"),     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
  [DB_TBL_DISP_COL_NN]    = {.title = strv("!0"),     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
  [DB_TBL_DISP_COL_FLTR]  = {.title = strv(""),       .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
};
static const struct db_tbl_col_def db_tbl_def[DB_STATE_TBL_COL_CNT] = {
  [DB_STATE_TBL_COL_NAME]  = {.title = strv("Name"),    .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {50, 800}}},
  [DB_STATE_TBL_COL_DEL]   = {.title = strv(""),        .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60, .con = {60, 60}}},
  [DB_STATE_TBL_COL_COLS]  = {.title = strv("Columns"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {200, 1000}}},
  [DB_STATE_TBL_COL_ROWS]  = {.title = strv("Rows"),    .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {200, 1000}}},
  [DB_STATE_TBL_COL_FLTR]  = {.title = strv("Filters"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {200, 800}}},
};
// clang-format on

struct db_name_lck {
  struct str name;
  sqlite3_stmt *stmt;
};
static void
db_tbl_name_acq(struct db_name_lck *lck, struct db_state *sdb,
                struct db_tbl_state *tbl) {
  requires(sdb);
  requires(lck);
  requires(tbl);

  lck->stmt = 0;
  if (tbl->qry_name) {
    struct str sql = strv("SELECT name FROM sqlite_master WHERE rowid = ?;");
    sqlite3_prepare_v2(sdb->con, db_str(sql), &lck->stmt, 0);
    sqlite3_bind_int64(lck->stmt, 1, tbl->rowid);
    int ret = sqlite3_step(lck->stmt);
    assert(ret == SQLITE_ROW);

    const char *tbl_name = (const char*)sqlite3_column_text(lck->stmt, 0);
    int tbl_name_len = sqlite3_column_bytes(lck->stmt, 0);
    lck->name = strn(tbl_name, tbl_name_len);
  } else {
    lck->name = tbl->title;
  }
  ensures(str_len(lck->name) > 0);
}
static void
db_tbl_name_rel(struct db_name_lck *lck) {
  requires(lck);
  sqlite3_finalize(lck->stmt);
}
static void
db_tbl_col_name_acq(struct db_name_lck *lck, struct db_state *sdb,
                    struct db_tbl_view *tbl, struct db_tbl_col *col,
                    struct str tbl_name, long long col_id) {
  requires(sdb);
  requires(lck);
  requires(tbl);

  if (!col || col->qry_name) {
    struct str sql = strv("SELECT name FROM pragma_table_info(?) WHERE rowid = ?;");
    sqlite3_prepare_v2(sdb->con, db_str(sql), &lck->stmt, 0);
    sqlite3_bind_text(lck->stmt, 1, db_str(tbl_name), SQLITE_STATIC);
    sqlite3_bind_int64(lck->stmt, 2, col_id);
    int ret = sqlite3_step(lck->stmt);
    assert(ret == SQLITE_ROW);

    const char *col_name = (const char*)sqlite3_column_text(lck->stmt, 0);
    int col_name_len = sqlite3_column_bytes(lck->stmt, 0);
    lck->name = strn(col_name, col_name_len);
  } else {
    lck->name = str_buf_get(&tbl->col.buf, col->name);
  }
  ensures(str_len(lck->name) > 0);
}
static void
db_tbl_col_name_rel(struct db_name_lck *lck) {
  assert(lck);
  sqlite3_finalize(lck->stmt);
}

/* ---------------------------------------------------------------------------
 *                                Filter
 * ---------------------------------------------------------------------------
 */
static inline int
db__tbl_fltr_elm_val(struct db_tbl_fltr_elm *elm) {
  assert(elm);
  assert(elm->type >= DB_TBL_FLTR_ELM_TYP_STR);
  assert(elm->type <= DB_TBL_FLTR_ELM_TYP_TM);
  assert(elm->col >= 0);
  assert(str__is_val(&elm->fnd));
  assert(str__is_val(&elm->col_name));
  return 1;
}
static inline int
db__tbl_fltr_val(struct db_tbl_fltr_state *fltr) {
  assert(fltr);
  assert(fltr->cnt >= 0);
  assert(fltr->cnt < cntof(fltr->elms));
  assert(fltr->cnt < DB_MAX_FLTR_CNT);

  assert(rng__is_val(&fltr->data_rng));
  assert(rng__is_val(&fltr->elm_rng));

  assert(fltr->state >= DB_TBL_FLTR_LST);
  assert(fltr->state <= DB_TBL_FLTR_EDT);
  for arr_loopv(i, fltr->elms) {
    assert(db__tbl_fltr_elm_val(&fltr->elms[i]));
  }
  assert(fltr->data_rng.cnt >= 0);
  assert(fltr->data_rng.lo >= 0);
  assert(fltr->data_rng.hi >= 0);
  return 1;
}
static void
db_tbl_fltr_add_str(struct db_state *sdb, struct db_tbl_state *stbl,
                    struct db_tbl_view *vtbl, struct db_tbl_fltr_state *fltr,
                    int idx, long long col, struct str str) {

  requires(db__tbl_fltr_val(fltr));
  requires(sdb);
  requires(stbl);
  requires(vtbl);

  assert(idx >= 0);
  assert(idx < DB_MAX_FLTR_CNT);
  assert(idx < cntof(fltr->elms));
  assert(!fltr->elms[idx].active);

  struct db_tbl_fltr_elm *elm = &fltr->elms[idx];
  elm->type = DB_TBL_FLTR_ELM_TYP_STR;
  elm->fnd = str_sqz(elm->fnd_buf, cntof(elm->fnd_buf), str);
  elm->enabled = 1;
  elm->active = 1;
  elm->col = col;

  struct db_name_lck tlck = {0};
  struct db_name_lck clck = {0};
  db_tbl_name_acq(&tlck, sdb, stbl);
  db_tbl_col_name_acq(&clck, sdb, vtbl, 0, tlck.name, col);
  elm->col_name = str_sqz(arrv(elm->col_buf), clck.name);

  db_tbl_col_name_rel(&clck);
  db_tbl_name_rel(&tlck);

  fltr->cnt++;
  ensures(db__tbl_fltr_val(fltr));
}
static void
db_tbl_fltr_view_clr(struct db_tbl_fltr_state *fltr) {
  requires(db__tbl_fltr_val(fltr));
  mset(fltr->elms, 0, szof(fltr->elms));
  fltr->cnt = 0;
  ensures(db__tbl_fltr_val(fltr));
}
static void
db_tbl_open_fltr(struct db_tbl_state *tbl, long long col) {
  mset(&tbl->fltr.data_rng, szof(tbl->fltr.data_rng), 0);
  tbl->disp = DB_TBL_VIEW_DSP_FILTER;
  tbl->fltr.state = DB_TBL_FLTR_EDT;
  tbl->fltr.data_rng.total = tbl->row.rng.total;
  tbl->fltr.ini_col = col;
  tbl->fltr.init = 1;
}
static void
db_tbl_close_fltr(struct db_tbl_state *tbl) {
  requires(tbl);
  tbl->disp = DB_TBL_VIEW_DSP_DATA;
  tbl->fltr.state = DB_TBL_FLTR_LST;
}
static int
db_tbl_fltr_view_qry(struct db_state *sdb, struct db_view *vdb,
                     struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                     struct db_tbl_fltr_state *fltr, struct db_tbl_fltr_view *view,
                     int low, int high) {
  requires(sdb);
  requires(vdb);
  requires(stbl);
  requires(vtbl);
  requires(view);
  requires(db__tbl_fltr_val(fltr));

  requires(low >= 0);
  requires(high >= 0);
  requires(low <= high);

  requires(low <= stbl->fltr.data_rng.total);
  requires(high <= stbl->fltr.data_rng.total);
  requires((high - low) <= cntof(view->data));
  if ((high - low) > cntof(view->data)) {
    return -1;
  }
  fltr->data_rng.lo = low;
  fltr->data_rng.hi = high;
  fltr->data_rng.cnt = 0;

  str_buf_clr(&view->buf);
  view->id = stbl->rowid;

  /* query table and column name */
  struct db_name_lck tlck = {0};
  struct db_name_lck clck = {0};
  db_tbl_name_acq(&tlck, sdb, stbl);
  db_tbl_col_name_acq(&clck, sdb, vtbl, 0, tlck.name, fltr->ini_col);

  /* query total filtered element count */
  int err = 0;
  sqlite3_stmt *stmt = 0;
  if (str_len(view->fnd_str) > 2) {
    struct str sql = str_fmtsn(arrv(vdb->sql_qry_buf),
      "SELECT COUNT(*) FROM \"%.*s\" WHERE \"%.*s\" LIKE '%%'||?||'%%';",
      strf(tlck.name), strf(clck.name));
    if (str_len(sql) < cntof(vdb->sql_qry_buf)-1) {
      err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
      sqlite3_bind_text(stmt, 1, db_str(view->fnd_str), SQLITE_STATIC);
      assert(err == SQLITE_OK);
    }
  } else {
    struct str sql = str_fmtsn(arrv(vdb->sql_qry_buf),
      "SELECT COUNT(*) FROM \"%.*s\";", strf(tlck.name));
    if (str_len(sql) < cntof(vdb->sql_qry_buf)-1) {
      err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
      assert(err == SQLITE_OK);
    }
  }
  if (stmt) {
    err = sqlite3_step(stmt);
    assert(err == SQLITE_ROW);
    fltr->data_rng.total = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    stmt = 0;
  } else {
    fltr->data_rng.lo = 0;
    fltr->data_rng.hi = 0;
    fltr->data_rng.cnt = 0;
    fltr->data_rng.total = 0;
    low = high = 0;
  }
  /* setup query table for filered elements */
  if (str_len(view->fnd_str)) {
    struct str sql = str_fmtsn(arrv(vdb->sql_qry_buf),
      "SELECT rowid, \"%.*s\" FROM \"%.*s\" WHERE \"%.*s\" LIKE '%%'||?||'%%' LIMIT ?,?;",
      strf(clck.name), strf(tlck.name), strf(clck.name));
    err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, db_str(view->fnd_str), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, low);
    sqlite3_bind_int(stmt, 3, high-low);
    assert(err == SQLITE_OK);
  } else {
    struct str sql = str_fmtsn(arrv(vdb->sql_qry_buf),
      "SELECT rowid, \"%.*s\" FROM \"%.*s\" LIMIT ?,?;", strf(clck.name), strf(tlck.name));
    err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    sqlite3_bind_int(stmt, 1, low);
    sqlite3_bind_int(stmt, 2, high-low);
    assert(err == SQLITE_OK);
  }
  if (stmt) {
    /* execute query for table filered elements */
    for db_loop(_,err,stmt,view->data) {
      assert(fltr->data_rng.cnt < cntof(view->data));
      long long rowid = sqlite3_column_int64(stmt, 0);
      const char *dat = (const char*)sqlite3_column_text(stmt, 1);
      int len = sqlite3_column_bytes(stmt, 1);

      struct str elm = strn(dat,len);
      view->data[fltr->data_rng.cnt] = str_buf_sqz(&view->buf, elm, DB_MAX_FLTR_ELM_STR);
      view->rowid[fltr->data_rng.cnt] = rowid;
      fltr->data_rng.cnt++;
    }
    assert(err == SQLITE_DONE);
    err = sqlite3_finalize(stmt);
    assert(err == SQLITE_OK);
  } else {
    fltr->data_rng.lo = 0;
    fltr->data_rng.hi = 0;
    fltr->data_rng.cnt = 0;
    fltr->data_rng.total = 0;
  }
  db_tbl_col_name_rel(&clck);
  db_tbl_name_rel(&tlck);

  ensures(view->id == stbl->rowid);
  ensures((stmt && fltr->data_rng.lo == low) || (!stmt && fltr->data_rng.lo == 0));
  ensures((stmt && fltr->data_rng.hi == high) || (!stmt && fltr->data_rng.hi == 0));
  ensures(view->buf.cnt >= 0);
  ensures(view->buf.cnt <= cntof(view->buf.mem));
  ensures(db__tbl_fltr_val(fltr));
  return 0;
}
/* ---------------------------------------------------------------------------
 *                                Table View
 * ---------------------------------------------------------------------------
 */
static inline int
db__tbl_state_is_val(const struct db_tbl_state *tbl) {
  assert(tbl);
  assert(tbl->kind >= DB_TBL_TYPE_TBL && tbl->kind <= DB_TBL_TYPE_TRIGGER);
  assert(tbl->state >= TBL_VIEW_SELECT && tbl->state <= TBL_VIEW_DISPLAY);
  assert(tbl->disp >= DB_TBL_VIEW_DSP_DATA && tbl->disp < DB_TBL_VIEW_DSP_CNT);
  assert(str_is_val(tbl->title));

  assert(tbl->col.rng.lo >= 0);
  assert(tbl->col.rng.hi >= 0);
  assert(tbl->col.rng.hi >= tbl->col.rng.lo);

  assert(tbl->col.rng.cnt >= 0);
  assert(tbl->col.rng.cnt <= DB_MAX_TBL_COLS);
  assert(tbl->col.rng.cnt <= SQLITE_MAX_COLUMN);
  assert(tbl->col.rng.cnt <= tbl->col.rng.total);

  assert(tbl->row.cols.lo >= 0);
  assert(tbl->row.cols.hi >= 0);
  assert(tbl->row.cols.lo <= tbl->row.cols.hi);

  assert(tbl->row.rng.lo >= 0);
  assert(tbl->row.rng.hi >= 0);
  assert(tbl->row.rng.hi >= tbl->row.rng.lo);
  assert(tbl->row.rng.cnt >= 0);

  assert(rng__is_val(&tbl->row.rng));
  assert(rng__is_val(&tbl->row.cols));
  assert(rng__is_val(&tbl->row.data_rng));

  assert(rng__is_val(&tbl->col.rng));
  assert(tbl->col.state == DB_TBL_COL_STATE_LOCKED || tbl->col.state == DB_TBL_COL_STATE_UNLOCKED);
  assert(tbl->col.cnt >= 0);
  assert(tbl->col.total >= 0);
  assert(tbl->col.cnt <= tbl->col.rng.total);
  return 1;
}
static inline int
db__state_is_val(const struct db_state *sdb) {
  assert(sdb);
  assert(sdb->sel_tbl >= 0);
  assert(sdb->sel_tbl < DB_TBL_CNT);
  assert(sdb->sel_tbl < cntof(sdb->tbls));

  assert(sdb->info.tab_act >= 0);
  assert(sdb->info.tab_act <= (1 << DB_TBL_TYPE_CNT)-1);
  assert(sdb->info.elm_cnt >= 0);
  assert(rng__is_val(&sdb->info.elm_rng));

  for arr_loopv(idx, sdb->tbls) {
    db__tbl_state_is_val(&sdb->tbls[idx]);
  }
  return 1;
}
static enum res_ico_id
db_tbl_col_ico(struct str type) {
  if (str_eq(type, strv("REAL"))) {
    return RES_ICO_MODIFY;
  } else if(str_eq(type, strv("DOUBLE"))) {
    return RES_ICO_MODIFY;
  } else if(str_eq(type, strv("DOUBLE PRECISION"))) {
    return RES_ICO_MODIFY;
  } else if(str_eq(type, strv("FLOAT"))) {
    return RES_ICO_MODIFY;
  } else if (str_eq(type, strv("DATE"))) {
    return RES_ICO_CALENDAR;
  } else if (str_eq(type, strv("DATETIME"))) {
    return RES_ICO_CALENDAR;
  } else if (str_eq(type, strv("BLOB"))) {
    return RES_ICO_CUBE;
  } else if (str_eq(type, strv("BOOLEAN"))) {
    return RES_ICO_CHECK;
  } else if (str_eq(type, strv("INT"))) {
    return RES_ICO_CALCULATOR;
  } else if (str_eq(type, strv("INTEGER"))) {
    return RES_ICO_CALCULATOR;
  } else if (str_eq(type, strv("TINYINT"))) {
    return RES_ICO_CALCULATOR;
  } else if (str_eq(type, strv("SMALLINT"))) {
    return RES_ICO_CALCULATOR;
  } else if (str_eq(type, strv("MEDIUMINT"))) {
    return RES_ICO_CALCULATOR;
  } else if (str_eq(type, strv("BIGINT"))) {
    return RES_ICO_CALCULATOR;
  } else if (str_eq(type, strv("UNSIGNED BIG INT"))) {
    return RES_ICO_CALCULATOR;
  } else if (str_eq(type, strv("INT2"))) {
    return RES_ICO_CALCULATOR;
  } else if (str_eq(type, strv("INT8"))) {
    return RES_ICO_CALCULATOR;
  } else if (str_eq(type, strv("NUMERIC"))) {
    return RES_ICO_CALCULATOR;
  } else {
    return RES_ICO_FONT;
  }
}
static void
db_tbl_qry_cols(struct db_state *sdb, struct db_view *vdb,
                struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                int low, int high, int sel) {

  requires(db__state_is_val(sdb));
  requires(vdb);
  requires(stbl);
  requires(vtbl);

  requires(sdb->con);
  requires(stbl->col.rng.total <= SQLITE_MAX_COLUMN);
  requires(sel == 0 || sel == 1);

  requires(low >= 0);
  requires(high >= 0);
  requires(low <= high);

  requires(low <= stbl->col.rng.total);
  requires(high <= stbl->col.rng.total);
  requires((high - low) <= cntof(vtbl->col.lst));
  requires((high - low) <= DB_MAX_TBL_COLS);

  int err = 0;
  stbl->row.rng.lo = 0;
  stbl->row.rng.hi = 0;
  stbl->row.rng.cnt = 0;

  str_buf_clr(&vtbl->col.buf);
  vtbl->col.id = stbl->rowid;
  if (sel) {
    stbl->row.cols.total = stbl->col.sel.cnt;
  }
  stbl->col.rng.lo = low;
  stbl->col.rng.cnt = 0;
  stbl->row.cols.lo = low;

  /* query table name */
  struct db_name_lck lck = {0};
  db_tbl_name_acq(&lck, sdb, stbl);

  /* query table columns */
  sqlite3_stmt *stmt = 0;
  if (sel) {
    struct str sql = strv("SELECT rowid, name, type, \"notnull\", pk FROM pragma_table_info(?);");
    sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, db_str(lck.name), SQLITE_STATIC);
  } else {
    struct str sql = strv("SELECT rowid, name, type, \"notnull\", pk FROM pragma_table_info(?) LIMIT ?,?;");
    sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, db_str(lck.name), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, low);
    sqlite3_bind_int(stmt, 3, high-low);
  }
  for db_loopn(_,err,stmt,SQLITE_MAX_COLUMN) {
    long long col_rowid = sqlite3_column_int64(stmt, 0);
    const char *col_name = (const char*)sqlite3_column_text(stmt, 1);
    const char *col_type = (const char*)sqlite3_column_text(stmt, 2);
    const char *col_nn = (const char*)sqlite3_column_text(stmt, 3);
    const char *col_key = (const char*)sqlite3_column_text(stmt, 4);
    if (sel && !tbl_has(&stbl->col.sel, hash_lld(col_rowid))) {
      continue;
    }
    int nam_len = sqlite3_column_bytes(stmt, 1);
    int typ_len = sqlite3_column_bytes(stmt, 2);

    struct str name = strn(col_name, nam_len);
    struct str type = strn(col_type, typ_len);

    int col_idx = stbl->col.rng.cnt++;
    assert(col_idx < cntof(vtbl->col.lst));
    assert(col_idx < stbl->col.rng.total);

    /* add column into table */
    struct db_tbl_col *col = &vtbl->col.lst[col_idx];
    col->rowid = col_rowid;
    col->name = str_buf_sqz(&vtbl->col.buf, name, DB_MAX_TBL_COL_NAME);
    col->type = str_buf_sqz(&vtbl->col.buf, type, DB_MAX_TBL_COL_TYPE);
    col->qry_name = str_buf_len(col->name) < nam_len;
    col->ico = db_tbl_col_ico(type);
    col->pk = !strcmp(col_key, "1");
    col->nn = !strcmp(col_nn, "1");
    col->blob = !str_len(type) || col_type[0] == 0;
    col->blob = col->blob || str_eq(type, strv("BLOB"));

    /* check if foreign key */
    sqlite3_stmt *fstmt = 0;
    struct str sql = strv("SELECT rowid FROM pragma_foreign_key_list(?) WHERE \"from\" = ?");
    sqlite3_prepare_v2(sdb->con, db_str(sql), &fstmt, 0);
    sqlite3_bind_text(fstmt, 1, db_str(lck.name), SQLITE_STATIC);
    sqlite3_bind_text(fstmt, 2, db_str(name), SQLITE_STATIC);
    col->fk = (sqlite3_step(fstmt) == SQLITE_ROW);
    sqlite3_finalize(fstmt);
  }
  stbl->col.rng.hi = stbl->col.rng.lo + stbl->col.rng.cnt;

  assert(err == SQLITE_DONE);
  err = sqlite3_finalize(stmt);
  assert(err == SQLITE_OK);
  db_tbl_name_rel(&lck);

  ensures(stbl->col.rng.lo >= 0);
  ensures(stbl->col.rng.hi >= 0);
  ensures(stbl->col.rng.hi >= stbl->col.rng.lo);

  ensures(stbl->col.rng.cnt >= 0);
  ensures(stbl->col.rng.cnt <= DB_MAX_TBL_COLS);
  ensures(stbl->col.rng.cnt <= SQLITE_MAX_COLUMN);
  ensures(stbl->col.rng.cnt <= stbl->col.rng.total);

  ensures(stbl->row.cols.lo >= 0);
  ensures(stbl->row.cols.hi >= 0);
  ensures(stbl->row.cols.lo <= stbl->row.cols.hi);
  ensures(!sel || (stbl->row.cols.cnt <= stbl->col.sel.cnt));

  ensures(stbl->row.rng.lo >= 0);
  ensures(stbl->row.rng.hi >= 0);
  ensures(stbl->row.rng.hi >= stbl->row.rng.lo);
  ensures(stbl->row.rng.cnt >= 0);

  ensures(vtbl->col.buf.cnt >= 0);
  ensures(vtbl->col.buf.cnt <= cntof(vtbl->col.buf.mem));
  ensures(vtbl->col.id == stbl->rowid);
  ensures(db__state_is_val(sdb));
}
static void
db_tbl_qry_row_cols(struct db_state *sdb, struct db_view *vdb,
                    struct db_tbl_state *stbl, struct db_tbl_view *vtbl, int low) {

  requires(db__state_is_val(sdb));
  requires(vdb);
  requires(vtbl);
  requires(stbl);

  requires(low >= 0);
  requires(low + stbl->row.cols.cnt <= stbl->col.rng.total);
  requires(stbl->row.cols.cnt <= stbl->col.rng.total);
  requires(stbl->row.cols.cnt <= DB_MAX_TBL_ROW_COLS);

  requires(stbl->row.cols.cnt <= stbl->col.rng.total);
  requires(stbl->row.cols.cnt <= DB_MAX_TBL_ROW_COLS);
  requires(stbl->col.rng.cnt <= stbl->col.rng.total);

  stbl->row.rng.lo = 0;
  stbl->row.rng.hi = 0;
  stbl->row.rng.cnt = 0;

  int end = max(stbl->col.cnt, stbl->col.rng.total) - stbl->col.cnt;
  int lhs = min(end, low);
  int rhs = lhs + stbl->col.cnt;

  if (vtbl->col.id != stbl->rowid ||
      !stbl->col.rng.cnt ||
      !rng_has_inclv(&stbl->col.rng, lhs) ||
      !rng_has_inclv(&stbl->col.rng, rhs)) {
    db_tbl_qry_cols(sdb, vdb, stbl, vtbl, lhs, rhs, 0);
  }
  int row_end = max(stbl->row.cols.cnt, stbl->col.rng.total) - stbl->row.cols.cnt;
  stbl->row.cols.lo = min(low, row_end);
  stbl->row.cols.hi = stbl->row.cols.lo + stbl->row.cols.cnt;

  ensures(stbl->row.cols.lo >= 0);
  ensures(stbl->row.cols.hi >= 0);
  ensures(stbl->row.cols.hi >= stbl->row.cols.lo);
  ensures(stbl->row.cols.hi == stbl->row.cols.lo + stbl->row.cols.cnt);

  ensures(stbl->row.cols.lo >= stbl->col.rng.lo);
  ensures(stbl->row.cols.hi <= stbl->col.rng.hi);
  ensures(stbl->row.cols.cnt <= stbl->col.rng.cnt);

  ensures(stbl->row.cols.hi >= stbl->row.cols.lo);
  ensures(stbl->row.cols.hi == stbl->row.cols.lo + stbl->row.cols.cnt);

  ensures(stbl->row.rng.lo == 0);
  ensures(stbl->row.rng.hi == 0);
  ensures(stbl->row.rng.cnt == 0);
  ensures(db__state_is_val(sdb));
}
static struct str
db_tbl_qry_fltr_sql(struct db_state *sdb, struct db_view *vdb,
                    struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                    struct str tbl_name, struct str sql) {

  requires(db__state_is_val(sdb));
  requires(vdb);
  requires(stbl);
  requires(vtbl);

  /* fill with active filter conditions */
  const char *pre = " WHERE";
  sql = str_add_fmt(vdb->sql_qry_buf, cntof(vdb->sql_qry_buf), sql,
    " FROM \"%.*s\" ", strf(tbl_name));
  for arr_loopv(idx, stbl->fltr.elms) {
    struct db_tbl_fltr_elm *elm = &stbl->fltr.elms[idx];
    if (!elm->active || !elm->enabled) {
      continue;
    }
    struct db_name_lck clck = {0};
    db_tbl_col_name_acq(&clck, sdb, vtbl, 0, tbl_name, elm->col);
    sql = str_add_fmt(arrv(vdb->sql_qry_buf), sql,
      "%s \"%.*s\" LIKE '%%%.*s%%'", pre, strf(clck.name), strf(elm->fnd));
    db_tbl_col_name_rel(&clck);
    pre = " AND";
  }
  ensures(db__state_is_val(sdb));
  return sql;
}
static int
db_tbl_qry_row_cnt(struct db_state *sdb, struct db_view *vdb,
                   struct db_tbl_state *stbl, struct db_tbl_view *vtbl) {

  requires(db__state_is_val(sdb));
  requires(vdb);
  requires(stbl);
  requires(vtbl);

  /* query table name */
  struct db_name_lck lck = {0};
  db_tbl_name_acq(&lck, sdb, stbl);
  struct str sql = str_set(arrv(vdb->sql_qry_buf), strv("SELECT COUNT(*)"));
  sql = db_tbl_qry_fltr_sql(sdb, vdb, stbl, vtbl, lck.name, sql);
  db_tbl_name_rel(&lck);

  int cnt = 0;
  if (str_len(sql) < cntof(vdb->sql_qry_buf)-1) {
    sqlite3_stmt *stmt = 0;
    sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);

    int err = sqlite3_step(stmt);
    assert(err == SQLITE_ROW);
    cnt = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
  }
  ensures(cnt >= 0);
  ensures(db__state_is_val(sdb));
  return cnt;
}
static int
db_tbl_qry_rows(struct db_state *sdb, struct db_view *vdb,
                struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                int low, int high) {

  requires(vdb);
  requires(stbl);
  requires(vtbl);
  requires(db__state_is_val(sdb));

  requires(low >= 0);
  requires(high >= 0);
  requires(low <= high);
  requires(low <= stbl->row.rng.total);
  requires(high <= stbl->row.rng.total);

  requires((high - low) <= DB_MAX_TBL_ROWS);
  requires((high - low) <= cntof(vtbl->row.rowids));
  requires((high - low) <= cntof(vtbl->row.lst));
  if ((high - low) > cntof(vtbl->row.rowids)) {
    return -1;
  }
  requires(stbl->row.cols.lo >= 0);
  requires(stbl->row.cols.hi >= 0);
  requires(stbl->row.cols.hi >= stbl->row.cols.lo);
  requires(stbl->row.cols.hi == stbl->row.cols.lo + stbl->row.cols.cnt);

  requires(stbl->row.cols.lo >= stbl->col.rng.lo);
  requires(stbl->row.cols.hi <= stbl->col.rng.hi);
  requires(stbl->row.cols.cnt <= stbl->col.rng.cnt);

  requires(stbl->row.cols.hi >= stbl->row.cols.lo);
  requires(stbl->row.cols.hi == stbl->row.cols.lo + stbl->row.cols.cnt);

  stbl->row.rng.lo = low;
  stbl->row.rng.hi = high;
  stbl->row.rng.cnt = 0;

  str_buf_clr(&vtbl->row.buf);
  vtbl->row.id = stbl->rowid;

  struct db_name_lck tlck = {0};
  db_tbl_name_acq(&tlck, sdb, stbl);

  /* setup sql query string */
  sqlite3_stmt *stmt = 0;
  struct str sql = str_set(arrv(vdb->sql_qry_buf), strv("SELECT rowid, "));
  for looprn(i, stbl->row.cols, DB_MAX_TBL_ROW_COLS) {

    assert(i >= stbl->col.rng.lo);
    int idx = i - stbl->col.rng.lo;
    assert(idx < cntof(vtbl->col.lst));

    struct db_name_lck clck = {0};
    struct db_tbl_col *col = &vtbl->col.lst[idx];

    db_tbl_col_name_acq(&clck, sdb, vtbl, col, tlck.name, col->rowid);
    sql = str_add_fmt(arrv(vdb->sql_qry_buf), sql, "%.*s, ", strf(clck.name));
    db_tbl_col_name_rel(&clck);
  }
  sql.rng = rng_lhs(&sql.rng, sql.rng.cnt-2);
  sql = db_tbl_qry_fltr_sql(sdb, vdb, stbl, vtbl, tlck.name, sql);
  sql = str_add_fmt(arrv(vdb->sql_qry_buf), sql, "LIMIT %d,%d", low, high-low);

  /* query table rows */
  int elm_idx = 0;
  if (str_len(sql) <= cntof(vdb->sql_qry_buf)-1) {
    int err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    assert(err == SQLITE_OK);

    for db_loop(_,err,stmt,vtbl->row.rowids) {
      assert(elm_idx < DB_MAX_TBL_ELM);
      vtbl->row.rowids[stbl->row.rng.cnt++] = sqlite3_column_int64(stmt, 0);
      for loopn(i, stbl->row.cols.cnt, DB_MAX_TBL_ROW_COLS) {

        int idx = i + stbl->row.cols.lo;
        assert(idx < cntof(vtbl->col.lst));
        struct db_tbl_col *col = &vtbl->col.lst[idx];

        if (!col->blob) {
          const char *dat = (const char*)sqlite3_column_text(stmt, i+1);
          int len = sqlite3_column_bytes(stmt, i+1);
          struct str data = strn(dat, len);
          assert(elm_idx < cntof(vtbl->row.lst));
          vtbl->row.lst[elm_idx] = str_buf_sqz(&vtbl->row.buf, data, DB_MAX_TBL_ELM_DATA);
        } else {
          vtbl->row.lst[elm_idx] = str_buf_sqz(&vtbl->row.buf, strv("[blob]"), DB_MAX_TBL_ELM_DATA);
        }
        elm_idx++;
      }
    }
    assert(err == SQLITE_DONE);
    err = sqlite3_finalize(stmt);
    assert(err == SQLITE_OK);
  } else {
    stbl->row.rng.lo = 0;
    stbl->row.rng.hi = 0;
    stbl->row.rng.cnt = 0;
    stbl->row.rng.total = 0;
  }
  db_tbl_name_rel(&tlck);

  ensures(elm_idx >= 0);
  ensures(elm_idx < DB_MAX_TBL_ELM);
  ensures(elm_idx < cntof(vtbl->row.lst));

  ensures(stbl->row.rng.lo >= 0);
  ensures(stbl->row.rng.hi >= 0);
  ensures(stbl->row.rng.lo <= stbl->row.rng.hi);
  ensures(stbl->row.rng.lo + stbl->row.rng.cnt == stbl->row.rng.hi);
  ensures(stbl->row.rng.cnt <= DB_MAX_TBL_ROWS);
  ensures(stbl->row.rng.cnt <= cntof(vtbl->row.rowids));

  ensures(vtbl->row.buf.cnt >= 0);
  ensures(vtbl->row.buf.cnt <= cntof(vtbl->row.buf.mem));
  ensures(vtbl->row.id == stbl->rowid);
  ensures(db__state_is_val(sdb));
  return 0;
}
static void
db_tbl_rev(struct db_state *sdb, struct db_view *vdb, struct db_tbl_state *stbl,
           struct db_tbl_view *view) {

  requires(vdb);
  requires(stbl);
  requires(view);
  requires(db__state_is_val(sdb));

  stbl->row.rng.total = db_tbl_qry_row_cnt(sdb, vdb, stbl, view);
  stbl->row.rng.cnt = 0;
  stbl->row.rng.lo = 0;
  stbl->row.rng.hi = 0;
}
static void
db_tbl_setup(struct db_state *sdb, struct db_view *vdb, int idx,
             struct gui_ctx *ctx, struct str name, long long rowid,
             enum db_tbl_type kind) {

  requires(db__state_is_val(sdb));
  requires(vdb);
  requires(ctx);
  requires(str_is_val(name));

  requires(idx >= 0);
  requires(idx < DB_TBL_CNT);
  requires(idx < cntof(sdb->tbls));

  /* setup table view */
  struct db_tbl_state *tbl = &sdb->tbls[idx];
  mset(&tbl->fltr.elms, 0, szof(tbl->fltr.elms));
  mset(&tbl->row.rng, 0, szof(tbl->row.rng));
  mset(&tbl->col.rng, 0, szof(tbl->col.rng));
  tbl->title = str_sqz(tbl->title_buf, cntof(tbl->title_buf), name);
  tbl->qry_name = str_len(name) > str_len(tbl->title);
  tbl->disp = DB_TBL_VIEW_DSP_DATA;
  tbl->state = TBL_VIEW_DISPLAY;
  tbl->rowid = rowid;
  tbl->kind = kind;

  /* retrive number of columns in table */
  sqlite3_stmt *stmt = 0;
  struct str sql = strv("SELECT COUNT(*) FROM pragma_table_info(?);");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
  sqlite3_bind_text(stmt, 1, db_str(name), SQLITE_STATIC);
  assert(err == SQLITE_OK);
  err = sqlite3_step(stmt);
  assert(err == SQLITE_ROW);

  tbl->col.rng.total = sqlite3_column_int(stmt, 0);
  tbl->col.cnt = min(tbl->col.rng.total, DB_MAX_TBL_COLS);
  tbl->row.cols.cnt = min(tbl->col.rng.total, DB_MAX_TBL_ROW_COLS);
  tbl->row.cols.total = tbl->col.rng.total;
  tbl->row.cols.hi = tbl->row.cols.cnt;
  tbl->row.cols.lo = 0;
  sqlite3_finalize(stmt);
  stmt = 0;

  /* setup table column display table */
  struct gui_split_lay bld = {0};
  gui.splt.lay.begin(&bld, tbl->row.ui.state, tbl->row.cols.cnt, ctx->cfg.sep);
  for loopn(i, tbl->row.cols.cnt, DB_MAX_TBL_ROW_COLS) {
    static const int cons[2] = {100, 1000};
    gui.splt.lay.add(&bld, GUI_LAY_SLOT_DYN, 1, cons);
  }
  gui.splt.lay.end(&bld);

  /* setup table column display columns */
  struct gui_split_lay_cfg dsp_cfg = {0};
  dsp_cfg.size = szof(struct db_tbl_col_def);
  dsp_cfg.off = offsetof(struct db_tbl_col_def, ui);
  dsp_cfg.slots = db_tbl_disp_col_def;
  dsp_cfg.cnt = DB_TBL_DISP_COL_MAX;
  gui.tbl.lay(tbl->col.ui.state, ctx, &dsp_cfg);

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

  /* retrieve total table row count */
  sql = str_fmtsn(arrv(vdb->sql_qry_buf), "SELECT COUNT(*) FROM \"%.*s\";", strf(name));
  if (str_len(sql) < cntof(vdb->sql_qry_buf)-1) {
    err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    assert(err == SQLITE_OK);

    err = sqlite3_step(stmt);
    assert(err == SQLITE_ROW);
    tbl->row.rng.total = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
  } else {
    tbl->row.rng.total = 0;
  }
  ensures(db__state_is_val(sdb));
}
static purist int
db_tbl_fltr_enabled(struct db_tbl_fltr_state *fltr) {
  requires(fltr);
  for arr_loopv(idx, fltr->elms) {
    struct db_tbl_fltr_elm *elm = &fltr->elms[idx];
    if (elm->active && !elm->enabled) {
      return 0;
    }
  }
  return 1;
}
static void
db_tbl_open(struct db_state *sdb, int tbl) {
  requires(db__state_is_val(sdb));
  assert(tbl >= 0 && tbl < DB_TBL_CNT);

  sdb->tbls[tbl].active = 1;
  sdb->frame = DB_FRAME_TBL;
  sdb->sel_tbl = tbl;
  ensures(db__state_is_val(sdb));
}
static void
db_tbl_close(struct db_state *sdb, int tbl) {
  requires(db__state_is_val(sdb));
  assert(tbl >= 0 && tbl < DB_TBL_CNT);

  sdb->tbls[tbl].active = 0;
  sdb->tbls[tbl].state = TBL_VIEW_SELECT;
  sdb->frame = DB_FRAME_LST;
  sdb->sel_tbl = 0;
  ensures(db__state_is_val(sdb));
}

/* ---------------------------------------------------------------------------
 *                                Info
 * ---------------------------------------------------------------------------
 */
static int
db_info_qry_cnt(struct db_state *sdb, enum db_tbl_type tab, struct str fltr) {
  static const char *type[] = {
#define DB_INFO(a,b,c,d) b,
    DB_TBL_MAP(DB_INFO)
#undef DB_INFO
  };
  requires(db__state_is_val(sdb));
  requires(str__is_val(&fltr));
  requires(tab <= cntof(type));
  requires(tab >= 0);

  int err = 0;
  sqlite3_stmt *stmt = 0;
  if (str_len(fltr)) {
    struct str sql = strv("SELECT COUNT(*) FROM sqlite_master WHERE type = ? AND name LIKE '%'||?||'%';");
    err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    assert(err == SQLITE_OK);
    sqlite3_bind_text(stmt, 1, type[tab], -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, db_str(fltr), SQLITE_STATIC);
  } else {
    struct str sql = strv("SELECT COUNT(*) FROM sqlite_master WHERE type = ?;");
    err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, type[tab], -1, SQLITE_STATIC);
  }
  assert(err == SQLITE_OK);
  err = sqlite3_step(stmt);
  assert(err == SQLITE_ROW);
  int cnt = sqlite3_column_int(stmt, 0);
  err = sqlite3_finalize(stmt);

  ensures(cnt >= 0);
  ensures(err == SQLITE_OK);
  ensures(db__state_is_val(sdb));
  return cnt;
}
static struct db_info_elm*
db_info_elm_new(struct db_info_state *sinfo, struct db_info_view *vinfo) {
  requires(vinfo != 0);
  requires(sinfo != 0);
  requires(sinfo->elm_cnt < DB_MAX_INFO_ELM_CNT);

  int old_cnt = sinfo->elm_cnt;
  int elm_idx = sinfo->elm_cnt++;
  struct db_info_elm *elm = vinfo->elms + elm_idx;

  ensures(elm != 0);
  ensures(sinfo->elm_cnt == old_cnt + 1);
  ensures(elm == vinfo->elms + old_cnt);
  return elm;
}
static void
db_info_elm_add(struct db_info_state *sinfo, struct db_info_view *vinfo,
                long long rowid, struct str name, struct str sql) {

  requires(vinfo);
  requires(sinfo);
  requires(sinfo->elm_cnt < DB_MAX_INFO_ELM_CNT);

  int old_cnt = sinfo->elm_cnt;
  struct db_info_elm *elm = db_info_elm_new(sinfo, vinfo);
  elm->name = str_buf_sqz(&vinfo->buf, name, DB_MAX_TBL_NAME);
  elm->sql = str_buf_sqz(&vinfo->buf, sql, DB_MAX_TBL_SQL);
  elm->rowid = rowid;

  struct str elm_sql = str_buf_get(&vinfo->buf, elm->sql);
  for str_loop(i, elm_sql) {
    char *byte = (char*)str_ptr(elm_sql, i);
    if (*byte == '\n' || *byte == '\r') {
      *byte = ' ';
    }
  }
  ensures(elm->rowid == rowid);
  ensures(sinfo->elm_cnt == old_cnt + 1);
  ensures(str_buf_len(elm->name) <= DB_MAX_TBL_NAME);
  ensures(str_buf_len(elm->sql) <= DB_MAX_TBL_SQL);
}
static int
db_info_qry_elm(struct db_state *sdb, struct db_info_state *sinfo,
                struct db_info_view *vinfo, int low, int high) {
  int err = 0;
  requires(sdb != 0);
  requires(sinfo != 0);
  requires(vinfo != 0);

  requires(low >= 0);
  requires(low <= high);
  requires(high <= sinfo->tab_cnt[sinfo->sel_tab]);
  requires(high - low < DB_MAX_INFO_ELM_CNT);
  if ((high - low) > DB_MAX_INFO_ELM_CNT) {
    return -1;
  }
  static const char *type[] = {
#define DB_INFO(a,b,c,d) b,
    DB_TBL_MAP(DB_INFO)
#undef DB_INFO
  };
  sqlite3_stmt *stmt = 0;
  if (str_len(vinfo->fnd_str)) {
    struct str sql = strv("SELECT rowid, name, sql FROM sqlite_master WHERE type = ? AND name LIKE '%'||?||'%' LIMIT ?,?;");
    err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, type[sinfo->sel_tab], -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, db_str(vinfo->fnd_str), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, low);
    sqlite3_bind_int(stmt, 4, high-low);
    assert(err == SQLITE_OK);
  } else {
    struct str sql = strv("SELECT rowid, name, sql FROM sqlite_master WHERE type = ? LIMIT ?,?;");
    err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, type[sinfo->sel_tab], -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, low);
    sqlite3_bind_int(stmt, 3, high-low);
    assert(err == SQLITE_OK);
  }
  str_buf_clr(&vinfo->buf);
  sinfo->elm_rng = rng(low, high, sinfo->tab_cnt[sinfo->sel_tab]);
  sinfo->elm_cnt = 0;
  vinfo->id = sdb->id;

  for db_loopn(_,err,stmt,DB_MAX_INFO_ELM_CNT) {
    long long rowid = sqlite3_column_int64(stmt, 0);
    const char *tbl_name = (const char*)sqlite3_column_text(stmt, 1);
    const char *tbl_sql = (const char*)sqlite3_column_text(stmt, 2);
    int tbl_name_len = sqlite3_column_bytes(stmt, 1);
    int tbl_sql_len = sqlite3_column_bytes(stmt, 2);

    struct str str_name = strn(tbl_name, tbl_name_len);
    struct str str_sql = strn(tbl_sql, tbl_sql_len);
    db_info_elm_add(sinfo, vinfo, rowid, str_name, str_sql);
  }
  assert(err == SQLITE_DONE);
  sqlite3_finalize(stmt);

  ensures(err == SQLITE_DONE);
  ensures(sinfo->elm_cnt <= DB_MAX_INFO_ELM_CNT);
  ensures(sinfo->elm_rng.lo == low);
  ensures(sinfo->elm_rng.hi == high);
  ensures(sinfo->elm_rng.cnt == high - low);
  ensures(vinfo->buf.cnt <= cntof(vinfo->buf.mem));
  ensures(vinfo->id == sdb->id);
  return 0;
}

/* ---------------------------------------------------------------------------
 *                              Database
 * ---------------------------------------------------------------------------
 */
static int
db_init(void *mem, int siz) {
  int err = sqlite3_config(SQLITE_CONFIG_HEAP, mem, siz, 64);
  return err == SQLITE_OK;
}
static int
db_setup(struct db_state *sdb, struct gui_ctx *ctx, struct str path) {
  requires(sdb);
  requires(ctx);

  sdb->id = str_hash(path);
  int err = sqlite3_open(str_beg(path), &sdb->con);
  if (err != SQLITE_OK) {
    return 0;
  }
  struct gui_split_lay_cfg tbl_cfg = {0};
  tbl_cfg.size = szof(struct db_tbl_col_def);
  tbl_cfg.off = offsetof(struct db_tbl_col_def, ui);
  tbl_cfg.slots = db_tree_col_def;
  tbl_cfg.cnt = DB_TREE_COL_MAX;
  gui.tbl.lay(sdb->info.tbl.state, ctx, &tbl_cfg);
  zero2(sdb->info.tbl.off);

  struct gui_split_lay_cfg tab_cfg = {0};
  tab_cfg.size = szof(struct db_tbl_col_def);
  tab_cfg.off = offsetof(struct db_tbl_col_def, ui);
  tab_cfg.cnt = DB_STATE_TBL_COL_CNT;
  tab_cfg.slots = db_tbl_def;
  gui.tbl.lay(sdb->ui.state, ctx, &tab_cfg);

  for arr_loopv(i, sdb->info.tab_cnt) {
    enum db_tbl_type tab_type = cast(enum db_tbl_type, i);
    sdb->info.tab_cnt[i] = db_info_qry_cnt(sdb, tab_type, str_nil);
    sdb->info.tab_act |= castu(!!sdb->info.tab_cnt[i]) << i;
  }
  int sel = cpu_bit_ffs32(sdb->info.tab_act);
  sdb->info.sel_tab = cast(enum db_tbl_type, sel);

  sdb->tbls[0].active = 1;
  sdb->frame = DB_FRAME_TBL;

  ensures(sdb->info.sel_tab >= 0);
  ensures(sdb->info.sel_tab < cntof(sdb->info.tab_cnt));
  ensures(db__state_is_val(sdb));
  return 1;
}
static void
db_free(struct db_state *sdb) {
  requires(db__state_is_val(sdb));
  mset(sdb->tbls, 0, sizeof(sdb->tbls));
  sdb->sel_tbl = 0;
  if (sdb->con){
    sqlite3_close(sdb->con);
  }
  ensures(db__state_is_val(sdb));
}
static void
db_tab_open_tbl_id(struct db_state *sdb, struct db_view *vdb, struct gui_ctx *ctx,
                   int view, long long tbl_id) {

  requires(db__state_is_val(sdb));
  requires(ctx);
  requires(sdb);
  requires(vdb);

  requires(view >= 0);
  requires(view < cntof(sdb->tbls));

  sqlite3_stmt *stmt = 0;
  struct str sql = strv("SELECT name FROM sqlite_master WHERE rowid = ?;");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
  sqlite3_bind_int64(stmt, 1, tbl_id);
  assert(err == SQLITE_OK);
  if (sqlite3_step(stmt) != SQLITE_ROW) {
    return;
  }
  const char *tbl_name = (const char*)sqlite3_column_text(stmt, 0);
  int tbl_name_len = sqlite3_column_bytes(stmt, 0);
  struct str tbl_name_str = strn(tbl_name, tbl_name_len);
  db_tbl_setup(sdb, vdb, view, ctx, tbl_name_str, tbl_id, sdb->info.sel_tab);
  sqlite3_finalize(stmt);
  ensures(db__state_is_val(sdb));
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

  requires(ctx);
  requires(btn);
  requires(icon);
  requires(parent);

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

  requires(ctx);
  requires(btn);
  requires(icon);
  requires(parent);

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
static struct str
ui_edit_fnd(struct gui_ctx *ctx, struct gui_edit_box *edt,
            struct gui_panel *pan, struct gui_panel *parent,
            struct gui_txt_ed *ted, char *buf, int cap, struct str str) {

  requires(ted);
  requires(buf);
  requires(ctx);

  requires(edt);
  requires(pan);
  requires(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    static const int pad[2] = {3,3};
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
    ted->buf = buf;
    ted->cap = cap;
    ted->str = str;

    edt->pan.focusable = 1;
    edt->pan.box.x = gui.bnd.min_max(ico.box.x.max, pan->box.x.max);
    edt->pan.box.x = gui.bnd.shrink(&edt->pan.box.x, pad[0]);
    edt->pan.box.y = gui.bnd.shrink(&pan->box.y, pad[1]);
    str = gui.edt.fld(ctx, edt, &edt->pan, pan, ted);
  }
  gui.pan.end(ctx, pan, parent);
  return str;
}
static int
ui_tbl_hdr_elm_ico(struct gui_ctx *ctx, struct gui_tbl *tbl,
                   const int *tbl_lay, struct gui_btn *slot,
                   int *state, enum res_ico_id ico) {
  requires(ctx);
  requires(tbl);
  requires(ico);

  requires(slot);
  requires(state);
  requires(tbl_lay);

  gui.tbl.hdr.slot.begin(ctx, tbl, slot);
  {
    struct gui_panel tog = {.box = slot->box};
    gui.ico.img(ctx, &tog, &slot->pan, ico);
  }
  gui.tbl.hdr.slot.end(ctx, tbl, tbl_lay, slot, state);
  return slot->clk;
}
static int
ui_tbl_hdr_elm_tog(struct gui_ctx *ctx, struct gui_tbl *tbl,
                   const int *tbl_lay, int *state, int act) {
  requires(ctx);
  requires(tbl);
  requires(state);
  requires(tbl_lay);

  struct gui_btn slot = {0};
  enum res_ico_id ico = act ? RES_ICO_TOGGLE_ON : RES_ICO_TOGGLE_OFF;
  return ui_tbl_hdr_elm_ico(ctx, tbl, tbl_lay, &slot, state, ico);
}
static int
ui_tbl_hdr_elm_lock(struct gui_ctx *ctx, struct gui_tbl *tbl,
                    const int *tbl_lay, int *state, int act) {
  requires(ctx);
  requires(tbl);
  requires(state);
  requires(tbl_lay);

  struct gui_btn slot = {0};
  enum res_ico_id ico = act ? RES_ICO_UNLOCK : RES_ICO_LOCK;
  return ui_tbl_hdr_elm_ico(ctx, tbl, tbl_lay, &slot, state, ico);
}
static int
ui_tbl_lst_elm_col_tog(struct gui_ctx *ctx, struct gui_tbl *tbl,
                       const int *tbl_lay, struct gui_panel *elm, int *is_act) {
  requires(ctx);
  requires(tbl);
  requires(elm);
  requires(is_act);
  requires(tbl_lay);

  struct gui_icon icon = {0};
  enum res_ico_id ico = *is_act ? RES_ICO_TOGGLE_ON : RES_ICO_TOGGLE_OFF;
  gui.tbl.lst.elm.col.ico(ctx, tbl, tbl_lay, elm, &icon, ico);
  if (icon.clk) {
    *is_act = !*is_act;
    return 1;
  }
  return 0;
}
/* ---------------------------------------------------------------------------
 *                                  Filter
 * ---------------------------------------------------------------------------
 */
static void
ui_db_tbl_fltr_lst_tbl_hdr(struct db_tbl_fltr_state *fltr, struct gui_tbl *tbl,
                           int *tbl_cols, int col_cnt, struct gui_ctx *ctx) {
  requires(ctx);
  requires(tbl);
  requires(fltr);
  requires(tbl_cols);

  gui.tbl.hdr.begin(ctx, tbl, tbl_cols, col_cnt, arrv(fltr->tbl.state));
  {
    /* enable/disable all filters toggle */
    int all_set = db_tbl_fltr_enabled(fltr);
    if (ui_tbl_hdr_elm_tog(ctx, tbl, tbl_cols, fltr->tbl.state, all_set)) {
      for arr_loopv(idx, fltr->elms) {
        if (fltr->elms[idx].active) {
          fltr->elms[idx].enabled = !all_set;
        }
      }
    }
    for (int i = 1; i + 1 < cntof(db_tbl_fltr_def); ++i) {
      assert(i < cntof(db_tbl_fltr_def));
      const struct db_tbl_col_def *col = &db_tbl_fltr_def[i];
      gui.tbl.hdr.slot.txt(ctx, tbl, tbl_cols, fltr->tbl.state, col->title);
    }
    /* delete all filters icon */
    struct gui_btn slot = {0};
    ui_tbl_hdr_elm_ico(ctx, tbl, tbl_cols, &slot, fltr->tbl.state, RES_ICO_TRASH);
    if (slot.clk) {
      db_tbl_fltr_view_clr(fltr);
    }
  }
  gui.tbl.hdr.end(ctx, tbl);
}
static void
ui_db_tbl_fltr_lst_view(struct db_state *sdb, struct db_view *vdb,
                        struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                        struct db_tbl_fltr_state *fltr, struct gui_ctx *ctx,
                        struct gui_panel *pan, struct gui_panel *parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(stbl);
  requires(vtbl);
  requires(fltr);
  requires(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, fltr->tbl.off, 0);
    {
      /* header */
      int tbl_cols[GUI_TBL_COL(DB_TBL_FLTR_MAX)];
      ui_db_tbl_fltr_lst_tbl_hdr(fltr, &tbl, tbl_cols, cntof(tbl_cols), ctx);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, DB_MAX_FLTR_CNT);
      cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_SINGLE;
      cfg.sel.on = GUI_LST_SEL_ON_HOV;

      int del_idx = -1;
      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for gui_tbl_lst_loopv(idx, _, gui, &tbl, fltr->elms) {
        struct db_tbl_fltr_elm *item = &fltr->elms[idx];

        struct gui_panel elm = {0};
        gui.tbl.lst.elm.begin(ctx, &tbl, &elm, gui_id_ptr(item), 0);
        {
          /* columns */
          if (item->active) {
            int is_enabled = item->enabled;
            ui_tbl_lst_elm_col_tog(ctx, &tbl, tbl_cols, &elm, &is_enabled);
            gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, item->fnd, 0);
            gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, item->col_name, 0);
            item->enabled = !!is_enabled;
          } else {
            struct gui_box col = {0};
            gui_tbl_lst_elm_col(&col, ctx, &tbl, tbl_cols);
            gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, strv("[Empty Slot]"), 0);
            gui_tbl_lst_elm_col(&col, ctx, &tbl, tbl_cols);
          }
          /* remove icon */
          struct gui_icon del = {0};
          gui.tbl.lst.elm.col.slot(&del.box, ctx, &tbl, tbl_cols);
          gui.ico.clk(ctx, &del, &elm, RES_ICO_TRASH);
          if (del.clk){
            del_idx = idx;
          }
        }
        gui.tbl.lst.elm.end(ctx, &tbl, &elm);
      }
      gui.tbl.lst.end(ctx, &tbl);
      if (del_idx >= 0) {
        mset(&fltr->elms[del_idx], 0, szof(fltr->elms[0]));
        db_tbl_rev(sdb, vdb, stbl, vtbl);
        fltr->cnt--;
      }
    }
    gui.tbl.end(ctx, &tbl, pan, fltr->tbl.off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_tbl_fltr_view(struct db_state *sdb, struct db_view *vdb,
                    struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                    struct db_tbl_fltr_state *fltr, struct db_tbl_fltr_view *view,
                    struct gui_ctx *ctx, struct gui_panel *pan,
                    struct gui_panel *parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(stbl);
  requires(vtbl);
  requires(fltr);
  requires(view);
  requires(parent);

  struct gui_box lay = pan->box;
  gui.pan.begin(ctx, pan, parent);
  {
    /* filter edit field */
    struct gui_edit_box edt = {.flags = GUI_EDIT_SEL_ON_ACT};
    edt.box = gui.cut.top(&lay, ctx->cfg.item, ctx->cfg.gap[1]);
    view->fnd_str = gui.edt.box(ctx, &edt, pan, view->fnd_buf, cntof(view->fnd_buf), view->fnd_str);
    if (edt.mod){
      fltr->data_rng = rng_nil;
      fltr->off[1] = 0;
    }
    struct gui_btn back = {0};
    back.box = gui.cut.bot(&lay, ctx->cfg.item, ctx->cfg.gap[1]);
    gui.btn.txt(ctx, &back, pan, strv("Back"), 0);
    if (back.clk) {
      db_tbl_close_fltr(stbl);
    }
    int dis = !str_len(view->fnd_str) || fltr->cnt >= DB_MAX_FLTR_CNT;
    confine gui_disable_on_scope(&gui, ctx, dis) {
      struct gui_btn add = {0};
      add.box = gui.cut.bot(&lay, ctx->cfg.item, ctx->cfg.gap[1]);
      gui.btn.txt(ctx, &add, pan, strv("Add"), 0);
      if (add.clk) {
        for arr_loopv(idx, fltr->elms) {
          struct db_tbl_fltr_elm *elm = &fltr->elms[idx];
          if (!elm->active) {
            assert(str_len(view->fnd_str) > 0);
            db_tbl_fltr_add_str(sdb, stbl, vtbl, fltr, idx, fltr->ini_col, view->fnd_str);
            db_tbl_close_fltr(stbl);
            db_tbl_rev(sdb, vdb, stbl, vtbl);
            break;
          }
        }
      }
    }
    /* search list */
    struct gui_lst_cfg cfg = {0};
    gui.lst.cfg(&cfg, fltr->data_rng.total, fltr->off[1]);
    cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
    cfg.sel.src = GUI_LST_SEL_SRC_EXT;
    cfg.sel.mode = GUI_LST_SEL_SINGLE;
    cfg.sel.on = GUI_LST_SEL_ON_HOV;

    struct gui_lst_reg reg = {.box = lay};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, fltr->off);
    if (view->id != stbl->rowid || edt.mod || fltr->init ||
        reg.lst.begin != fltr->data_rng.lo ||
        reg.lst.end != fltr->data_rng.hi) {

      int ret = db_tbl_fltr_view_qry(sdb, vdb, stbl, vtbl, fltr, view, reg.lst.begin, reg.lst.end);
      if (ret < 0) {
        fltr->data_rng.total = 0;
        reg.lst.begin = 0;
        reg.lst.end = 0;
      }
      fltr->init = 0;
    }
    for gui_lst_reg_loopv(i, _, gui, &reg, view->data) {
      assert(i >= reg.lst.begin);
      int idx = i - reg.lst.begin;
      assert(idx < cntof(view->rowid));

      struct gui_panel elm = {0};
      struct gui_id id = gui_id64(hash_lld(view->rowid[idx]));
      gui.lst.reg.elm.begin(ctx, &reg, &elm, id, 0);
      {
        struct gui_panel lbl = {.box = elm.box};
        unsigned hdl = view->data[idx];
        struct str dat = str_buf_get(&view->buf, hdl);
        gui.txt.lbl(ctx, &lbl, &elm, dat, 0);
      }
      gui.lst.reg.elm.end(ctx, &reg, &elm);
    }
    gui.lst.reg.end(ctx, &reg, pan, fltr->off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_tbl_view_dsp_fltr(struct db_state *sdb, struct db_view *vdb,
                        struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                        struct db_tbl_fltr_state *fltr, struct db_tbl_fltr_view *view,
                        struct gui_ctx *ctx, struct gui_panel *pan,
                        struct gui_panel *parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(stbl);
  requires(vtbl);
  requires(fltr);
  requires(parent);

  switch (fltr->state) {
  case DB_TBL_FLTR_LST: {
    ui_db_tbl_fltr_lst_view(sdb, vdb, stbl, vtbl, fltr, ctx, pan, parent);
  } break;
  case DB_TBL_FLTR_EDT: {
    ui_db_tbl_fltr_view(sdb, vdb, stbl, vtbl, fltr, view, ctx, pan, parent);
  } break;}
}

/* ---------------------------------------------------------------------------
 *                                  Table
 * ---------------------------------------------------------------------------
 */
static enum res_ico_id
ui_db_tbl_lst_elm_ico(enum db_tbl_type type) {
  switch (type) {
  case DB_TBL_TYPE_CNT:
    assert(0);
  case DB_TBL_TYPE_TBL:
    return RES_ICO_TABLE;
  case DB_TBL_TYPE_VIEW:
    return RES_ICO_IMAGE;
  case DB_TBL_TYPE_IDX:
    return RES_ICO_TAG;
  case DB_TBL_TYPE_TRIGGER:
    return RES_ICO_BOLT;
  }
  return RES_ICO_DATABASE;
}
static void
ui_db_tbl_view_hdr_key_slot(struct db_tbl_view *view, struct db_tbl_col *col,
                            struct gui_ctx *ctx, struct gui_btn *slot) {
  requires(col);
  requires(ctx);
  requires(slot);

  struct gui_cfg_stk stk[1] = {0};
  unsigned fk_col = ctx->cfg.col[GUI_COL_TXT_DISABLED];
  confine gui_cfg_pushu_scope(&gui, stk, &ctx->cfg.col[GUI_COL_ICO], fk_col) {
    struct gui_btn hdr = {.box = slot->pan.box};
    struct str col_name = str_buf_get(&view->col.buf, col->name);
    ui_btn_ico_txt(ctx, &hdr, &slot->pan, col_name, RES_ICO_KEY);
  }
}
static void
ui_db_tbl_view_hdr_lnk_slot(struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                            struct db_tbl_col *col, struct gui_ctx *ctx,
                            struct gui_btn *slot) {
  requires(col);
  requires(ctx);
  requires(vtbl);
  requires(stbl);
  requires(slot);

  /* table column header filter icon button */
  struct gui_btn fltr = {.box = slot->pan.box};
  fltr.box.x = gui.bnd.max_ext(slot->pan.box.x.max, ctx->cfg.item);
  int dis = col->blob || (stbl->fltr.cnt >= cntof(stbl->fltr.elms));
  confine gui_disable_on_scope(&gui, ctx, dis) {
    if (gui.btn.ico(ctx, &fltr, &slot->pan, RES_ICO_SEARCH)) {
      assert(!col->blob && (stbl->fltr.cnt < cntof(stbl->fltr.elms)));
      db_tbl_open_fltr(stbl, col->rowid);
    }
  }
  /* header label with foreign key icon */
  struct gui_cfg_stk stk[1] = {0};
  unsigned fk_col = ctx->cfg.col[GUI_COL_TXT_DISABLED];
  confine gui_cfg_pushu_scope(&gui, stk, &ctx->cfg.col[GUI_COL_ICO], fk_col) {
    struct str col_name = str_buf_get(&vtbl->col.buf, col->name);
    struct gui_btn hdr = {.box = slot->pan.box};
    hdr.box.x = gui.bnd.min_max(slot->pan.box.x.min, fltr.box.x.min);
    ui_btn_ico_txt(ctx, &hdr, &slot->pan, col_name, RES_ICO_LINK);
  }
}
static void
ui_db_tbl_view_hdr_blob_slot(struct gui_ctx *ctx, struct str txt,
                             struct gui_btn *slot) {
  requires(ctx);
  requires(slot);

  static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
  struct gui_panel pan = {.box = slot->pan.box};
  gui_txt(ctx, &pan, &slot->pan, txt, &align);
}
static void
ui_db_tbl_view_hdr_slot(struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                        struct db_tbl_col *col, struct gui_ctx *ctx,
                        struct gui_btn *slot) {
  requires(col);
  requires(ctx);
  requires(vtbl);
  requires(stbl);

  /* table column header filter icon button */
  struct gui_btn fltr = {.box = slot->pan.box};
  fltr.box.x = gui.bnd.max_ext(slot->pan.box.x.max, ctx->cfg.item);
  int dis = col->blob || (stbl->fltr.cnt >= cntof(stbl->fltr.elms));
  confine gui_disable_on_scope(&gui, ctx, dis) {
    if (gui.btn.ico(ctx, &fltr, &slot->pan, RES_ICO_SEARCH)) {
      assert(!col->blob && (stbl->fltr.cnt < cntof(stbl->fltr.elms)));
      db_tbl_open_fltr(stbl, col->rowid);
    }
  }
  static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
  struct str col_name = str_buf_get(&vtbl->col.buf, col->name);
  struct gui_btn hdr = {.box = slot->pan.box};
  hdr.box.x = gui.bnd.min_max(slot->pan.box.x.min, fltr.box.x.min);
  gui.btn.txt(ctx, &hdr, &slot->pan, col_name, &align);
}
static void
ui_db_tbl_view_dsp_data(struct db_state *sdb, struct db_view *vdb,
                        struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                        struct gui_ctx *ctx, struct gui_panel *pan,
                        struct gui_panel *parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(stbl);
  requires(vtbl);
  requires(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    if (vtbl->col.id != stbl->rowid) {
      /* reload column data */
      db_tbl_qry_row_cols(sdb, vdb, stbl, vtbl, stbl->row.cols.lo);
    }
    int back = 0;
    int front = 0;
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, stbl->row.ui.off, 0);
    {
      /* header */
      int tbl_lay[GUI_TBL_COL(DB_MAX_TBL_ROW_COLS)];
      gui.tbl.hdr.begin(ctx, &tbl, arrv(tbl_lay), arrv(stbl->row.ui.state));
      for looprn(i, stbl->row.cols, DB_MAX_TBL_ROW_COLS) {

        assert(i >= stbl->col.rng.lo);
        int idx = i - stbl->col.rng.lo;
        assert(idx < cntof(vtbl->col.lst));
        struct db_tbl_col* col = &vtbl->col.lst[idx];

        struct gui_btn slot = {0};
        gui.tbl.hdr.slot.begin(ctx, &tbl, &slot);
        if (i == stbl->row.cols.lo) {
          struct gui_btn prv = {.box = slot.pan.box};
          prv.box.x = gui.bnd.min_ext(slot.pan.box.x.min, ctx->cfg.scrl);

          /* move column window to the left */
          int fits = stbl->row.cols.cnt >= stbl->col.rng.total;
          int begin = stbl->row.cols.lo == 0;
          confine gui_disable_on_scope(&gui, ctx, fits || begin) {
            back = gui__scrl_btn(ctx, &prv, &slot.pan, GUI_WEST);
          }
          slot.pan.box.x = gui.bnd.min_max(prv.box.x.max, slot.pan.box.x.max);
        }
        if (i + 1 == stbl->row.cols.hi) {
          struct gui_btn nxt = {.box = slot.pan.box};
          nxt.box.x = gui.bnd.max_ext(slot.pan.box.x.max, ctx->cfg.scrl);

          /* move column window to the right */
          int fits = stbl->row.cols.cnt >= stbl->col.rng.total;
          int end = (stbl->row.cols.lo + stbl->row.cols.cnt >= stbl->col.rng.total);
          confine gui_disable_on_scope(&gui, ctx, fits || end) {
            front = gui__scrl_btn(ctx, &nxt, &slot.pan, GUI_EAST);
          }
          slot.pan.box.x = gui.bnd.min_max(slot.pan.box.x.min, nxt.box.x.min);
        }
        if (col->pk) {
          ui_db_tbl_view_hdr_key_slot(vtbl, col, ctx, &slot);
        } else if (col->fk) {
          ui_db_tbl_view_hdr_lnk_slot(stbl, vtbl, col, ctx, &slot);
        } else if (col->blob) {
          struct str col_name = str_buf_get(&vtbl->col.buf, col->name);
          ui_db_tbl_view_hdr_blob_slot(ctx, col_name, &slot);
        } else {
          ui_db_tbl_view_hdr_slot(stbl, vtbl, col, ctx, &slot);
        }
        gui.tbl.hdr.slot.end(ctx, &tbl, tbl_lay, &slot, stbl->row.ui.state);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, stbl->row.rng.total);
      cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_SINGLE;
      cfg.sel.on = GUI_LST_SEL_ON_HOV;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      if (vtbl->row.id != stbl->rowid ||
          tbl.lst.begin != stbl->row.rng.lo ||
          tbl.lst.end != stbl->row.rng.hi) {

        int err = db_tbl_qry_rows(sdb, vdb, stbl, vtbl, tbl.lst.begin, tbl.lst.end);
        if (err < 0) {
          stbl->row.rng.total = 0;
          tbl.lst.begin = 0;
          tbl.lst.end = 0;
        }
      }
      int elm_idx = 0;
      for gui_tbl_lst_loopv(i,_,gui,&tbl,vtbl->row.rowids) {
        assert(i >= tbl.lst.begin);
        int idx = i - tbl.lst.begin;
        assert(idx < cntof(vtbl->row.rowids));

        struct gui_panel item = {0};
        long long rowid = vtbl->row.rowids[idx];
        struct gui_id id = gui_id64(hash_lld(rowid));

        gui.tbl.lst.elm.begin(ctx, &tbl, &item, id, 0);
        for loopn(j, stbl->row.cols.cnt, DB_MAX_TBL_ROW_COLS) {
          assert(elm_idx < cntof(vtbl->row.lst));

          unsigned elm = vtbl->row.lst[elm_idx++];
          struct str dat = str_buf_get(&vtbl->row.buf, elm);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_lay, &item, dat, 0);
        }
        gui.tbl.lst.elm.end(ctx, &tbl, &item);
      }
      gui.tbl.lst.end(ctx, &tbl);
    }
    gui.tbl.end(ctx, &tbl, pan, stbl->row.ui.off);

    if (back) {
      db_tbl_qry_row_cols(sdb, vdb, stbl, vtbl, stbl->row.cols.lo-1);
    } else if (front) {
      db_tbl_qry_row_cols(sdb, vdb, stbl, vtbl, stbl->row.cols.lo+1);
    }
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_tbl_view_dsp_lay(struct db_state *sdb, struct db_view *vdb,
                          struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                          struct gui_ctx *ctx, struct gui_panel *pan,
                          struct gui_panel *parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(stbl);
  requires(vtbl);
  requires(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, stbl->col.ui.off, 0);
    {
      /* header */
      int tbl_cols[GUI_TBL_COL(DB_TBL_DISP_COL_MAX)];
      gui.tbl.hdr.begin(ctx, &tbl, arrv(tbl_cols), arrv(stbl->col.ui.state));
      if (ui_tbl_hdr_elm_lock(ctx, &tbl, tbl_cols, stbl->col.ui.state, !!stbl->col.state)) {

        switch (stbl->col.state) {
        case DB_TBL_COL_STATE_LOCKED: {
          stbl->col.state = DB_TBL_COL_STATE_UNLOCKED;
          tbl_clr(&stbl->col.sel);
        } break;
        default: assert(0); break;
        case DB_TBL_COL_STATE_UNLOCKED: {
          stbl->col.state = DB_TBL_COL_STATE_LOCKED;
        } break;}
      }
      for (int i = 1; i < cntof(db_tbl_disp_col_def); ++i) {
        const struct db_tbl_col_def *itr = db_tbl_disp_col_def + i;
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_cols, stbl->col.ui.state, itr->title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, stbl->col.rng.total);
      cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_SINGLE;
      cfg.sel.on = GUI_LST_SEL_ON_HOV;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      if (vtbl->col.id != stbl->rowid ||
          tbl.lst.begin != stbl->col.rng.lo ||
          tbl.lst.end != stbl->col.rng.hi) {
        db_tbl_qry_cols(sdb, vdb, stbl, vtbl, tbl.lst.begin, tbl.lst.end, 0);
      }
      for gui_tbl_lst_loopv(i,_,gui,&tbl,vtbl->col.lst) {
        assert(i < stbl->col.rng.total);
        struct db_tbl_col *col = &vtbl->col.lst[i];
        struct str col_name = str_buf_get(&vtbl->col.buf, col->name);
        struct str col_type = str_buf_get(&vtbl->col.buf, col->type);

        struct gui_panel item = {0};
        unsigned long long key = hash_lld(col->rowid);
        gui.tbl.lst.elm.begin(ctx, &tbl, &item, gui_id64(key), 0);
        {
          int dis = stbl->col.state == DB_TBL_COL_STATE_LOCKED;
          confine gui_disable_on_scope(&gui, ctx, dis) {
            int is_act = (dis || tbl_has(&stbl->col.sel, key));
            /* column selection */
            if (ui_tbl_lst_elm_col_tog(ctx, &tbl, tbl_cols, &item, &is_act)) {
              assert(stbl->col.state != DB_TBL_COL_STATE_LOCKED);
              if (is_act) {
                tbl_put(&stbl->col.sel, key, &col->rowid);
              } else{
                tbl_del(&stbl->col.sel, key);
              }
            }
          }
          gui.tbl.lst.elm.col.txt_ico(ctx, &tbl, tbl_cols, &item, col_name, col->ico);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &item, col_type, 0);
          if (col->pk) {
            struct gui_icon icon;
            gui.tbl.lst.elm.col.ico(ctx, &tbl, tbl_cols, &item, &icon, RES_ICO_KEY);
          } else {
            gui_tbl_lst_elm_col(&item.box, ctx, &tbl, tbl_cols);
          }
          if (col->fk) {
            struct gui_icon icon;
            gui.tbl.lst.elm.col.ico(ctx, &tbl, tbl_cols, &item, &icon, RES_ICO_LINK);
          } else {
            gui_tbl_lst_elm_col(&item.box, ctx, &tbl, tbl_cols);
          }
          if (col->nn) {
            struct gui_icon icon;
            gui.tbl.lst.elm.col.ico(ctx, &tbl, tbl_cols, &item, &icon, RES_ICO_CHECK);
          } else {
            gui_tbl_lst_elm_col(&item.box, ctx, &tbl, tbl_cols);
          }
          if (!col->pk && !col->blob && !col->fk) {
            struct gui_icon icon;
            gui.tbl.lst.elm.col.ico(ctx, &tbl, tbl_cols, &item, &icon, RES_ICO_SEARCH);
            if (icon.clk) {
              db_tbl_open_fltr(stbl, col->rowid);
            }
          } else {
            gui_tbl_lst_elm_col(&item.box, ctx, &tbl, tbl_cols);
          }
        }
        gui.tbl.lst.elm.end(ctx, &tbl, &item);
      }
      gui.tbl.lst.end(ctx, &tbl);
    }
    gui.tbl.end(ctx, &tbl, pan, stbl->col.ui.off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_tbl_view_dsp(struct db_state *sdb, struct db_view *vdb,
                   struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                   struct gui_ctx *ctx, struct gui_panel *pan,
                   struct gui_panel *parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(stbl);
  requires(vtbl);
  requires(parent);

  static const struct {
    struct str name;
    unsigned long long hash;
    enum res_ico_id ico;
  } tabs[DB_TBL_VIEW_DSP_CNT] = {
    [DB_TBL_VIEW_DSP_DATA]    = {.name = strv("Data"),    .hash = 791601637ull, .ico = RES_ICO_TH_LIST},
    [DB_TBL_VIEW_DSP_FILTER]  = {.name = strv("Filters"), .hash = 404898220ull, .ico = RES_ICO_MODIFY},
    [DB_TBL_VIEW_DSP_LAYOUT]  = {.name = strv("Layout"),  .hash = 268480831ull, .ico = RES_ICO_COG},
  };
  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box, .hdr_pos = GUI_TAB_HDR_BOT};
    gui.tab.begin(ctx, &tab, pan, DB_TBL_VIEW_DSP_CNT, casti(stbl->disp));
    {
      /* tab header */
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      {
        for arr_loopv(i, tabs) {
          struct gui_panel slot = {0};
          gui.tab.hdr.slot.begin(ctx, &tab, &hdr, &slot, gui_id64(tabs[i].hash));
          gui.ico.box(ctx, &slot, &hdr.pan, tabs[i].ico, tabs[i].name);
          gui.tab.hdr.slot.end(ctx, &tab, &hdr, &slot, 0);
        }
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);

      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      switch (stbl->disp) {
      case DB_TBL_VIEW_DSP_CNT: assert(0); break;
      case DB_TBL_VIEW_DSP_DATA: {
        ui_db_tbl_view_dsp_data(sdb, vdb, stbl, vtbl, ctx, &bdy, &tab.pan);
      } break;
      case DB_TBL_VIEW_DSP_FILTER: {
        ui_db_tbl_view_dsp_fltr(sdb, vdb, stbl, vtbl, &stbl->fltr, &vtbl->fltr, ctx, &bdy, &tab.pan);
      } break;
      case DB_TBL_VIEW_DSP_LAYOUT: {
        ui_db_tbl_view_dsp_lay(sdb, vdb, stbl, vtbl, ctx, &bdy, &tab.pan);
      } break;}
    }
    gui.tab.end(ctx, &tab, pan);
    if (tab.sel.mod) {
      /* tab selection change */
      if (stbl->disp == DB_TBL_VIEW_DSP_LAYOUT) {
        if (stbl->col.state == DB_TBL_COL_STATE_UNLOCKED) {
          if (stbl->col.sel.cnt == 0) {
            stbl->col.state = DB_TBL_COL_STATE_LOCKED;
          }
        }
        if (stbl->col.state == DB_TBL_COL_STATE_UNLOCKED) {
          stbl->col.rng.total = stbl->col.sel.cnt;
          stbl->col.rng.cnt = stbl->col.rng.hi = stbl->col.rng.lo = 0;

          stbl->row.cols.lo = 0;
          stbl->row.cols.cnt = min(stbl->col.cnt, DB_MAX_TBL_ROW_COLS);
          stbl->row.cols.total = stbl->col.sel.cnt;
          stbl->row.cols.hi = stbl->row.cols.cnt;

          stbl->col.cnt = min(stbl->col.sel.cnt, DB_MAX_TBL_COLS);
          stbl->col.total = stbl->col.rng.total;
          db_tbl_qry_row_cols(sdb, vdb, stbl, vtbl, 0);
        }
      }
      if (tab.sel.idx == DB_TBL_VIEW_DSP_LAYOUT) {
        if (stbl->col.state == DB_TBL_COL_STATE_UNLOCKED) {
          stbl->col.rng.lo = stbl->col.rng.hi = stbl->col.rng.cnt = 0;
          stbl->col.rng.total = stbl->col.total;

          stbl->col.cnt = min(stbl->col.rng.total, DB_MAX_TBL_COLS);
          stbl->row.cols.cnt = min(stbl->col.total, DB_MAX_TBL_ROW_COLS);
          stbl->row.cols.total = stbl->col.total;
        }
      }
      stbl->disp = cast(enum db_tbl_view_dsp_state, tab.sel.idx);
    }
  }
  gui.pan.end(ctx, pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  Info
 * ---------------------------------------------------------------------------
 */
static void
ui_db_view_info_tbl(struct db_state *sdb, struct db_view *vdb, int view,
                    struct db_info_state *sinfo, struct db_info_view *vinfo,
                    struct gui_ctx *ctx, struct gui_panel *pan,
                    struct gui_panel *parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(sinfo);
  requires(vinfo);
  requires(parent);
  requires(view < cntof(sdb->tbls));

  int open_tbl = 0;
  int open_tbl_idx = -1;
  gui.pan.begin(ctx, pan, parent);
  {
    const struct {struct str type; enum res_ico_id ico;} types[] = {
      #define DB_INFO(a,b,c,d) {.type = strv(b), .ico = (d)},
        DB_TBL_MAP(DB_INFO)
      #undef DB_INFO
    };
    int gap = ctx->cfg.gap[1];
    struct gui_box lay = pan->box;
    struct gui_panel fltr = {.box = gui.cut.top(&lay, ctx->cfg.item, gap)};
    struct gui_edit_box edt = {.box = fltr.box};
    vinfo->fnd_str = ui_edit_fnd(ctx, &edt, &fltr, pan, &vinfo->fnd_ed,
        arrv(vinfo->fnd_buf), vinfo->fnd_str);

    if (edt.mod) {
      assert(sinfo->sel_tab < cntof(sinfo->tab_cnt));
      sinfo->tab_cnt[sinfo->sel_tab] = db_info_qry_cnt(sdb, sinfo->sel_tab, vinfo->fnd_str);
    }
    struct gui_tbl tbl = {.box = lay};
    gui.tbl.begin(ctx, &tbl, pan, sinfo->tbl.off, 0);
    {
      /* header */
      const struct db_tbl_col_def *col = 0;
      int tbl_cols[GUI_TBL_COL(DB_TREE_COL_MAX)];
      gui.tbl.hdr.begin(ctx, &tbl, arrv(tbl_cols), arrv(sinfo->tbl.state));
      for arr_eachv(col, db_tree_col_def) {
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_cols, sinfo->tbl.state, col->title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, sinfo->tab_cnt[sinfo->sel_tab]);
      cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_SINGLE;
      cfg.sel.on = GUI_LST_SEL_ON_HOV;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      if (sdb->id != vdb->info.id || edt.mod ||
          tbl.lst.begin != sinfo->elm_rng.lo ||
          tbl.lst.end != sinfo->elm_rng.hi) {

        int ret = db_info_qry_elm(sdb, sinfo, vinfo, tbl.lst.begin, tbl.lst.end);
        if (ret < 0) {
          sinfo->tab_cnt[sinfo->sel_tab] = 0;
          tbl.lst.begin = 0;
          tbl.lst.end = 0;
        }
      }
      for gui_tbl_lst_loopv(i,_,gui,&tbl,vinfo->elms) {
        assert(i >= tbl.lst.begin);
        int elm_idx = i - tbl.lst.begin;
        assert(elm_idx < cntof(vinfo->elms));
        assert(elm_idx >= 0);

        struct db_info_elm *elm = &vinfo->elms[elm_idx];
        struct str elm_name = str_buf_get(&vinfo->buf, elm->name);
        struct str elm_sql = str_buf_get(&vinfo->buf, elm->sql);

        struct gui_panel item = {0};
        unsigned long long hash = hash_lld(elm->rowid);
        int is_sel = tbl_has(&vinfo->sel, hash);
        gui.tbl.lst.elm.begin(ctx, &tbl, &item, gui_id64(hash), is_sel);
        {
          gui.tbl.lst.elm.col.txt_ico(ctx, &tbl, tbl_cols, &item, elm_name, types[sinfo->sel_tab].ico);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &item, types[sinfo->sel_tab].type, 0);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &item, elm_sql, 0);
        }
        gui.tbl.lst.elm.end(ctx, &tbl, &item);

        /* input handling */
        struct gui_input pin = {0};
        gui.pan.input(&pin, ctx, &item, GUI_BTN_LEFT);
        if (pin.mouse.btn.left.clk) {
          open_tbl_idx = elm_idx;
          open_tbl = 1;
        }
      }
      gui.tbl.lst.end(ctx, &tbl);
    }
    gui.tbl.end(ctx, &tbl, pan, sinfo->tbl.off);
  }
  gui.pan.end(ctx, pan, parent);

  if (open_tbl) {
    assert(open_tbl_idx >= 0);
    assert(open_tbl_idx < cntof(vinfo->elms));

    struct db_info_elm *elm = &vinfo->elms[open_tbl_idx];
    db_tab_open_tbl_id(sdb, vdb, ctx, view, elm->rowid);
    tbl_clr(&vinfo->sel);
  }
}
static void
ui_db_view_info(struct db_state *sdb, struct db_view *vdb, int view,
                struct db_info_state *sinfo, struct db_info_view *vinfo,
                struct gui_ctx *ctx, struct gui_panel *pan,
                struct gui_panel *parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(sinfo);
  requires(vinfo);
  requires(parent);
  requires(view < cntof(sdb->tbls));

  static const struct tab_def {
    struct str title;
    enum res_ico_id ico;
  } tabs[] = {
    #define DB_INFO(a,b,c,d) {.title = strv(c), .ico = (d)},
      DB_TBL_MAP(DB_INFO)
    #undef DB_INFO
  };
  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box, .hdr_pos = GUI_TAB_HDR_BOT};
    gui.tab.begin(ctx, &tab, pan, cntof(sinfo->tab_cnt), casti(sinfo->sel_tab));
    {
      /* tab header */
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      for arr_loopv(i, tabs) {
        const struct tab_def *def = &tabs[i];
        /* tab header slots */
        int dis = !(sinfo->tab_act & (1U << i));
        confine gui_disable_on_scope(&gui, ctx, dis) {
          struct gui_id iid = gui_id64(str_hash(def->title));

          struct gui_panel slot = {0};
          gui.tab.hdr.slot.begin(ctx, &tab, &hdr, &slot, iid);
          gui.ico.box(ctx, &slot, &hdr.pan, def->ico, def->title);
          gui.tab.hdr.slot.end(ctx, &tab, &hdr, &slot, 0);
        }
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);
      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      ui_db_view_info_tbl(sdb, vdb, view, sinfo, vinfo, ctx, &bdy, &tab.pan);
    }
    gui.tab.end(ctx, &tab, pan);
    if (tab.sel.mod) {
      if (str_len(vinfo->fnd_str)) {
        vinfo->fnd_str = str_nil;
        assert(sinfo->sel_tab < cntof(sinfo->tab_cnt));
        sinfo->tab_cnt[sinfo->sel_tab] = db_info_qry_cnt(sdb, sinfo->sel_tab, str_nil);
      }
      sinfo->sel_tab = castb(tab.sel.idx);
      tbl_clr(&vinfo->sel);
    }
  }
  gui.pan.end(ctx, pan, parent);
}
/* ---------------------------------------------------------------------------
 *                                Database
 * ---------------------------------------------------------------------------
 */
static int
ui_db_tab_view_lst(struct db_state *sdb, struct gui_ctx *ctx,
                   struct gui_panel *pan, struct gui_panel *parent) {

  requires(ctx);
  requires(pan);
  requires(parent);
  requires(db__state_is_val(sdb));

  int ret = -1;
  int do_del = 0;
  int del_idx = -1;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, sdb->ui.off, 0);
    {
      /* header */
      int tbl_lay[GUI_TBL_COL(DB_STATE_TBL_COL_CNT)];
      gui.tbl.hdr.begin(ctx, &tbl, arrv(tbl_lay), arrv(sdb->ui.state));
      for arr_loopv(i, db_tbl_def) {
        struct str title = db_tbl_def[i].title;
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_lay, sdb->ui.state, title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, DB_TBL_CNT);
      cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_SINGLE;
      cfg.sel.on = GUI_LST_SEL_ON_HOV;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for gui_tbl_lst_loopn(idx, _, gui, &tbl, DB_TBL_CNT) {
        struct db_tbl_state *stbl = &sdb->tbls[idx];

        enum res_ico_id ico;
        struct str title = strv("Open...");
        if (stbl->active) {
          ico = RES_ICO_TABLE;
          title = stbl->title;
        } else {
          ico = RES_ICO_FOLDER_OPEN;
        }
        struct gui_panel elm = {0};
        gui.tbl.lst.elm.begin(ctx, &tbl, &elm, gui_id_ptr(stbl), 0);
        {
          struct gui_icon del = {0};
          gui.tbl.lst.elm.col.txt_ico(ctx, &tbl, tbl_lay, pan, title, ico);
          if (stbl->active) {
            gui.tbl.lst.elm.col.slot(&del.box, ctx, &tbl, tbl_lay);
            gui.ico.clk(ctx, &del, &elm, RES_ICO_NO);
            if (del.clk){
              del_idx = idx;
              do_del = 1;
            }
            gui.tbl.lst.elm.col.txtf(ctx, &tbl, tbl_lay, pan, 0, "%d", stbl->col.rng.total);
            gui.tbl.lst.elm.col.txtf(ctx, &tbl, tbl_lay, pan, 0, "%d", stbl->row.rng.total);
            gui.tbl.lst.elm.col.txtf(ctx, &tbl, tbl_lay, pan, 0, "%d", stbl->fltr.cnt);
          } else {
            struct gui_box item = {0};
            gui_tbl_lst_elm_col(&item, ctx, &tbl, tbl_lay);
            gui_tbl_lst_elm_col(&item, ctx, &tbl, tbl_lay);
            gui_tbl_lst_elm_col(&item, ctx, &tbl, tbl_lay);
            gui_tbl_lst_elm_col(&item, ctx, &tbl, tbl_lay);
          }
        }
        gui.tbl.lst.elm.end(ctx, &tbl, &elm);

        /* input handling */
        struct gui_input pin = {0};
        gui.pan.input(&pin, ctx, &elm, GUI_BTN_LEFT);
        if (pin.mouse.btn.left.clk) {
          ret = do_del ? -1: idx;
        }
      }
      gui.tbl.lst.end(ctx, &tbl);
    }
    gui.tbl.end(ctx, &tbl, pan, sdb->ui.off);
  }
  gui.pan.end(ctx, pan, parent);
  if (do_del) {
    db_tbl_close(sdb, del_idx);
  }
  ensures(db__state_is_val(sdb));
  return ret;
}
static void
ui_db_main(struct db_state *sdb, struct db_view *vdb, int view,
           struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent) {

  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);
  requires(parent);

  requires(view >= 0);
  requires(view < cntof(sdb->tbls));

  struct db_tbl_view *vtbl = &vdb->tbl;
  struct db_tbl_state *stbl = &sdb->tbls[view];
  gui.pan.begin(ctx, pan, parent);
  {
    switch (stbl->state) {
    case TBL_VIEW_SELECT: {
      struct gui_panel overview = {.box = pan->box};
      ui_db_view_info(sdb, vdb, view, &sdb->info, &vdb->info, ctx, &overview, pan);
    } break;
    case TBL_VIEW_DISPLAY: {
      struct gui_panel lst = {.box = pan->box};
      ui_db_tbl_view_dsp(sdb, vdb, stbl, vtbl, ctx, &lst, pan);
    } break;
    }
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_explr(struct db_state *sdb, struct db_view *vdb, struct gui_ctx *ctx,
            struct gui_panel *pan, struct gui_panel *parent, struct str file) {

  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);
  requires(parent);

  if (sdb->id != vdb->id) {
    vdb->info.id = 0;
    vdb->id = sdb->id;

    vdb->tbl.row.id = 0;
    vdb->tbl.col.id = 0;
    vdb->tbl.fltr.id = 0;
  }
  gui.pan.begin(ctx, pan, parent);
  {
    int gapy = ctx->cfg.gap[1];
    struct gui_box lay = pan->box;
    struct gui_box hdr = gui.cut.top(&lay, ctx->cfg.item, gapy);
    {
      int gapx = ctx->cfg.gap[0];
      struct gui_btn tab = {.box = gui.cut.lhs(&hdr, ctx->cfg.item, gapx)};
      if (gui.btn.ico(ctx, &tab, pan, RES_ICO_TH_LIST)) {
        sdb->frame = DB_FRAME_LST;
      }
      struct gui_panel lbl = {.box = hdr};
      gui.lbl.txt(ctx, &lbl, pan, &(struct gui_box_cut){&hdr, GUI_BOX_CUT_LHS, 0}, file);
      if (sdb->frame == DB_FRAME_TBL && sdb->tbls[sdb->sel_tbl].state == TBL_VIEW_DISPLAY) {

        struct db_tbl_state *tbl = &sdb->tbls[sdb->sel_tbl];
        gui.lbl.txt(ctx, &lbl, pan, &(struct gui_box_cut){&hdr, GUI_BOX_CUT_LHS, 0}, strv("/"));
        gui.lbl.txt(ctx, &lbl, pan, &(struct gui_box_cut){&hdr, GUI_BOX_CUT_LHS, 0}, tbl->title);
      }
    }
    struct gui_panel bdy = {.box = lay};
    switch (sdb->frame) {
    case DB_FRAME_CNT: assert(0); break;
    case DB_FRAME_LST: {
      int ret = ui_db_tab_view_lst(sdb, ctx, &bdy, pan);
      if (ret >= 0) {
        db_tbl_open(sdb, ret);
      }
    } break;
    case DB_FRAME_TBL: {
      ui_db_main(sdb, vdb, sdb->sel_tbl, ctx, &bdy, pan);
    } break;}
  }
  gui.pan.end(ctx, pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
static const struct db_api db__api = {
  .init = db_init,
  .setup = db_setup,
  .del = db_free,
  .ui = ui_db_explr,
};
static void
db_api(void *export, void *import) {
  unused(import);
  struct db_api *exp = cast(struct db_api*, export);
  *exp = db__api;
}

