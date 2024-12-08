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
// clang-format on
struct db_name_lck {
  struct str name;
  sqlite3_stmt *stmt;
};
static void
db_tbl_name_acq(struct db_name_lck *lck, struct db_state *sdb,
                struct db_tbl_state *tbl) {
  assert(sdb);
  assert(lck);
  assert(tbl);

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
}
static void
db_tbl_name_rel(struct db_name_lck *lck) {
  assert(lck);
  sqlite3_finalize(lck->stmt);
}
static void
db_tbl_col_name_acq(struct db_name_lck *lck, struct db_state *sdb,
                    struct db_tbl_view *tbl, struct db_tbl_col *col,
                    struct str tbl_name, long long col_id) {
  assert(sdb);
  assert(lck);
  assert(tbl);

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
static int
db_tbl_fltr_new(struct db_tbl_fltr_state *fltr) {
  assert(fltr);
  assert(fltr->unused > 0);

  int idx = cpu_bit_ffs32(fltr->unused);
  fltr->unused &= ~(1U << castu(idx));
  assert(idx < cntof(fltr->elms));
  mset(fltr->elms + idx, 0, szof(fltr->elms[0]));
  return idx;
}
static void
db_tbl_fltr_del(struct db_tbl_fltr_state *fltr, int idx) {
  assert(fltr);
  assert(idx >= 0);
  assert(idx < cntof(fltr->elms));
  assert(!(fltr->unused & (1U << castu(idx))));

  struct db_tbl_fltr_elm *elm = &fltr->elms[idx];
  fltr->unused |= (1U << castu(idx));
  mset(elm, 0, szof(*elm));
}
static int
db_tbl_fltr_add(struct db_tbl_fltr_state *fltr, int idx) {
  assert(fltr);
  assert(idx >= 0);
  assert(idx < cntof(fltr->lst));
  assert(fltr->cnt < cntof(fltr->lst));
  assert(!(fltr->unused & (1U << castu(idx))));

  fltr->lst[fltr->cnt] = castb(idx);
  return fltr->cnt++;
}
static void
db_tbl_fltr_rm(struct db_tbl_fltr_state *fltr, int idx) {
  assert(fltr);
  assert(idx >= 0);
  assert(idx < fltr->cnt);
  assert(idx < cntof(fltr->lst));
  assert(fltr->cnt < cntof(fltr->lst));
  assert(!(fltr->unused & (1U << castu(fltr->lst[idx]))));

  arr_rm(fltr->lst, idx, fltr->cnt);
  fltr->cnt--;
}
static int
db_tbl_fltr_add_str(struct db_state *sdb, struct db_tbl_state *stbl,
                    struct db_tbl_view *vtbl, struct db_tbl_fltr_state *fltr,
                    long long col, struct str str) {
  assert(sdb);
  assert(stbl);
  assert(vtbl);
  assert(fltr);

  assert(fltr->unused);
  assert(str_len(str));
  assert(str_len(str) < DB_MAX_FLTR_STR);
  assert(fltr->cnt < cntof(fltr->lst));

  int idx = db_tbl_fltr_new(fltr);
  struct db_tbl_fltr_elm *elm = &fltr->elms[idx];
  elm->type = DB_TBL_FLTR_ELM_TYP_STR;
  elm->fnd = str_sqz(elm->fnd_buf, cntof(elm->fnd_buf), str);
  elm->enabled = 1;
  elm->col = col;

  struct db_name_lck tlck = {0};
  struct db_name_lck clck = {0};
  db_tbl_name_acq(&tlck, sdb, stbl);
  db_tbl_col_name_acq(&clck, sdb, vtbl, 0, tlck.name, col);
  elm->col_name = str_sqz(arrv(elm->col_buf), clck.name);
  db_tbl_col_name_rel(&clck);
  db_tbl_name_rel(&tlck);

  return db_tbl_fltr_add(fltr, idx);
}
static void
db_tbl_fltr_view_rm(struct db_tbl_fltr_state *fltr, int idx) {
  assert(fltr);
  assert(idx >= 0);
  assert(idx < fltr->cnt);
  assert(idx < cntof(fltr->lst));
  assert(!(fltr->unused & (1U << castu(fltr->lst[idx]))));

  int elm = fltr->lst[idx];
  db_tbl_fltr_rm(fltr, idx);
  db_tbl_fltr_del(fltr, elm);
}
static void
db_tbl_fltr_view_clr(struct db_tbl_fltr_state *fltr) {
  assert(fltr);
  for arr_loopn(i, fltr->lst, fltr->cnt) {
    assert(i < cntof(fltr->lst));
    int idx = fltr->lst[i];
    db_tbl_fltr_del(fltr, idx);
  }
  assert(fltr->unused == DB_FLTR_CNT_MSK);
  fltr->unused = DB_FLTR_CNT_MSK;
  fltr->cnt = 0;
}
static void
db_tbl_open_fltr(struct db_tbl_state *tbl, long long col) {
  assert(tbl);
  mset(&tbl->fltr.data_rng, szof(tbl->fltr.data_rng), 0);
  tbl->disp = DB_TBL_VIEW_DSP_FILTER;
  tbl->fltr.state = DB_TBL_FLTR_EDT;
  tbl->fltr.data_rng.total = tbl->row.rng.total;
  tbl->fltr.ini_col = col;
  tbl->fltr.init = 1;
}
static void
db_tbl_close_fltr(struct db_tbl_state *tbl) {
  assert(tbl);
  tbl->disp = DB_TBL_VIEW_DSP_DATA;
  tbl->fltr.state = DB_TBL_FLTR_LST;
}
static int
db_tbl_fltr_view_qry(struct db_state *sdb, struct db_view *vdb,
                     struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                     struct db_tbl_fltr_state *fltr, struct db_tbl_fltr_view *view,
                     int low, int high) {
  assert(sdb);
  assert(vdb);
  assert(stbl);
  assert(vtbl);
  assert(fltr);
  assert(view);

  assert(low >= 0);
  assert(high >= 0);
  assert(low <= high);

  assert(low <= stbl->fltr.data_rng.total);
  assert(high <= stbl->fltr.data_rng.total);
  assert((high - low) <= cntof(view->data));
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
  if (str_len(view->fnd_str)) {
    struct str sql = str_fmtsn(arrv(vdb->sql_qry_buf),
      "SELECT COUNT(*) FROM %.*s WHERE %.*s LIKE '%%'||?||'%%';",
      strf(tlck.name), strf(clck.name));
    if (str_len(sql) < cntof(vdb->sql_qry_buf)-1) {
      err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
      sqlite3_bind_text(stmt, 1, db_str(view->fnd_str), SQLITE_STATIC);
      assert(err == SQLITE_OK);
    }
  } else {
    struct str sql = str_fmtsn(arrv(vdb->sql_qry_buf),
      "SELECT COUNT(*) FROM '%.*s';", strf(tlck.name));
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
      "SELECT rowid, %.*s FROM %.*s WHERE %.*s LIKE '%%'||?||'%%' LIMIT ?,?;",
      strf(clck.name), strf(tlck.name), strf(clck.name));
    err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, db_str(view->fnd_str), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, low);
    sqlite3_bind_int(stmt, 3, high-low);
    assert(err == SQLITE_OK);
  } else {
    struct str sql = str_fmtsn(arrv(vdb->sql_qry_buf),
      "SELECT rowid, %.*s FROM %.*s LIMIT ?,?;", strf(clck.name), strf(tlck.name));
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
  stmt = 0;
  return 0;
}
/* ---------------------------------------------------------------------------
 *                                Table View
 * ---------------------------------------------------------------------------
 */
static int
db_tbl_new(struct db_state *sdb) {
  assert(sdb);
  assert(sdb->unused > 0);

  int idx = cpu_bit_ffs32(sdb->unused);
  assert(idx < cntof(sdb->tbls));
  sdb->unused &= ~(1U << castu(idx));

  mset(sdb->tbls + idx, 0, szof(sdb->tbls[0]));
  sdb->tbls[idx].fltr.unused = DB_FLTR_CNT_MSK;
  return idx;
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
  assert(sdb);
  assert(vdb);
  assert(stbl);
  assert(vtbl);

  assert(low >= 0);
  assert(high >= 0);
  assert(low <= high);

  assert(low < stbl->col.rng.total);
  assert(high <= stbl->col.rng.total);
  assert((high - low) <= cntof(vtbl->col.lst));

  int err = 0;
  stbl->row.rng.lo = 0;
  stbl->row.rng.hi = 0;
  stbl->row.rng.cnt = 0;

  str_buf_clr(&vtbl->col.buf);
  vtbl->col.id = stbl->rowid;
  if (sel) {
    stbl->col.rng.lo = 0;
    stbl->col.rng.hi = stbl->col.sel.cnt;
    stbl->col.rng.cnt = 0;

    stbl->row.cols.lo = 0;
    stbl->row.cols.hi = stbl->col.sel.cnt;
  } else {
    stbl->col.rng.lo = low;
    stbl->col.rng.hi = high;
    stbl->col.rng.cnt = 0;

    stbl->row.cols.lo = low;
    stbl->row.cols.hi = low + stbl->row.cols.cnt;
  }
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
  assert(err == SQLITE_DONE);
  err = sqlite3_finalize(stmt);
  assert(err == SQLITE_OK);
  db_tbl_name_rel(&lck);
}
static void
db_tbl_qry_row_cols(struct db_state *sdb, struct db_view *vdb,
                    struct db_tbl_state *stbl, struct db_tbl_view *vtbl, int low) {
  assert(sdb);
  assert(vdb);
  assert(vtbl);
  assert(stbl);

  assert(low >= 0);
  assert(low + stbl->row.cols.cnt <= stbl->col.rng.total);

  stbl->row.rng.lo = 0;
  stbl->row.rng.hi = 0;
  stbl->row.rng.cnt = 0;

  int end = max(stbl->col.cnt, stbl->col.rng.total) - stbl->col.cnt;
  int lhs = min(end, low);
  int rhs = lhs + stbl->col.cnt;

  if (vtbl->col.id != stbl->rowid ||
      !rng_has_inclv(&stbl->col.rng, lhs) ||
      !rng_has_inclv(&stbl->col.rng, rhs)) {
    db_tbl_qry_cols(sdb, vdb, stbl, vtbl, lhs, rhs, 0);
  }

  int rend = max(stbl->row.cols.cnt, stbl->col.rng.total) - stbl->row.cols.cnt;
  stbl->row.cols.lo = min(low, rend);
  stbl->row.cols.hi = stbl->row.cols.lo + stbl->row.cols.cnt;
}
static struct str
db_tbl_qry_fltr_sql(struct db_state *sdb, struct db_view *vdb,
                    struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                    struct str tbl_name, struct str sql) {
  assert(sdb);
  assert(vdb);
  assert(stbl);
  assert(vtbl);

  /* fill with active filter conditions */
  const char *pre = " WHERE";
  sql = str_add_fmt(vdb->sql_qry_buf, cntof(vdb->sql_qry_buf), sql,
    " FROM '%.*s' ", strf(tbl_name));
  for arr_loopn(i, stbl->fltr.lst, stbl->fltr.cnt) {

    assert(i < cntof(stbl->fltr.lst));
    int idx = stbl->fltr.lst[i];
    assert(idx < cntof(stbl->fltr.elms));

    struct db_tbl_fltr_elm *elm = &stbl->fltr.elms[idx];
    if (!elm->enabled) {
      continue;
    }
    struct db_name_lck clck = {0};
    db_tbl_col_name_acq(&clck, sdb, vtbl, 0, tbl_name, elm->col);
    sql = str_add_fmt(arrv(vdb->sql_qry_buf), sql,
      "%s %.*s LIKE '%%%.*s%%'", pre, strf(clck.name), strf(elm->fnd));
    db_tbl_col_name_rel(&clck);
    pre = " AND";
  }
  return sql;
}
static int
db_tbl_qry_row_cnt(struct db_state *sdb, struct db_view *vdb,
                   struct db_tbl_state *stbl, struct db_tbl_view *vtbl) {
  assert(sdb);
  assert(vdb);
  assert(stbl);
  assert(vtbl);

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
  return cnt;
}
static int
db_tbl_qry_rows(struct db_state *sdb, struct db_view *vdb,
                struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                int low, int high) {
  assert(sdb);
  assert(vdb);
  assert(stbl);
  assert(vtbl);

  assert(low >= 0);
  assert(high >= 0);
  assert(low <= high);

  assert(low <= stbl->row.rng.total);
  assert(high <= stbl->row.rng.total);
  assert((high - low) <= cntof(vtbl->row.lst));
  if ((high - low) > cntof(vtbl->row.rowids)) {
    return -1;
  }
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
  if (str_len(sql) < cntof(vdb->sql_qry_buf)-1) {
    int err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    assert(err == SQLITE_OK);

    int elm_idx = 0;
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
  return 0;
}
static void
db_tbl_rev(struct db_state *sdb, struct db_view *vdb, struct db_tbl_state *stbl,
           struct db_tbl_view *view) {

  assert(sdb);
  assert(vdb);
  assert(stbl);
  assert(view);

  stbl->row.rng.total = db_tbl_qry_row_cnt(sdb, vdb, stbl, view);
  stbl->row.rng.cnt = 0;
  stbl->row.rng.lo = 0;
  stbl->row.rng.hi = 0;
}
static void
db_tbl_setup(struct db_state *sdb, struct db_view *vdb, int idx,
             struct gui_ctx *ctx, struct str name, long long rowid,
             enum db_tbl_type kind) {

  assert(sdb);
  assert(vdb);
  assert(ctx);
  assert(str_is_val(name));

  /* setup table view */
  struct db_tbl_state *tbl = &sdb->tbls[idx];
  mset(&tbl->row.rng, 0, szof(tbl->row.rng));
  mset(&tbl->col.rng, 0, szof(tbl->col.rng));
  tbl->title = str_sqz(tbl->title_buf, cntof(tbl->title_buf), name);
  tbl->qry_name = str_len(name) > str_len(tbl->title);
  tbl->disp = DB_TBL_VIEW_DSP_DATA;
  tbl->state = TBL_VIEW_DISPLAY;
  tbl->fltr.unused = DB_FLTR_CNT_MSK;
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
}
static void
db_tbl_del(struct db_state *sdb, int idx) {
  assert(sdb);
  assert(idx >= 0);
  assert(idx < cntof(sdb->tbls));
  assert(!(sdb->unused & (1U << castu(idx))));

  struct db_tbl_state *tbl = &sdb->tbls[idx];
  sdb->unused |= (1U << castu(idx));
  db_tbl_fltr_view_clr(&tbl->fltr);
  mset(tbl, 0, szof(*tbl));
}
static int
db_tbl_add(struct db_state *sdb, int idx) {
  assert(sdb);
  assert(idx >= 0);
  assert(idx < cntof(sdb->tbls));

  assert(sdb->tab_cnt < cntof(sdb->tabs));
  assert(!(sdb->unused & (1U << castu(idx))));
  assert(sdb->tab_cnt <= UCHAR_MAX);

  sdb->tabs[sdb->tab_cnt] = castb(idx);
  return sdb->tab_cnt++;
}
static void
db_tbl_rm(struct db_state *sdb, int idx) {
  assert(sdb);
  assert(sdb->tab_cnt < cntof(sdb->tabs));

  assert(idx >= 0);
  assert(idx < sdb->tab_cnt);
  assert(idx < cntof(sdb->tabs));
  assert(!(sdb->unused & (1U << castu(sdb->tabs[idx]))));

  arr_rm(sdb->tabs, idx, sdb->tab_cnt);
  sdb->tab_cnt--;
}
static int
db_tbl_fltr_enabled(struct db_tbl_fltr_state *fltr) {
  assert(fltr);
  for arr_loopn(i, fltr->lst, fltr->cnt) {
    assert(i < cntof(fltr->lst));
    int idx = fltr->lst[i];
    assert(idx < cntof(fltr->elms));

    struct db_tbl_fltr_elm *elm = &fltr->elms[idx];
    if (!elm->enabled) {
      return 0;
    }
  }
  return 1;
}
/* ---------------------------------------------------------------------------
 *                                Info
 * ---------------------------------------------------------------------------
 */
static int
db_info_qry_cnt(struct db_state *sdb, enum db_tbl_type tab, struct str fltr) {
  assert(sdb);
  static const char *type[] = {
#define DB_INFO(a,b,c,d) b,
    DB_TBL_MAP(DB_INFO)
#undef DB_INFO
  };
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
  assert(err == SQLITE_OK);
  return cnt;
}
static struct db_info_elm*
db_info_elm_new(struct db_info_state *sinfo, struct db_info_view *vinfo) {
  assert(vinfo);
  assert(sinfo);
  assert(sinfo->elm_cnt < DB_MAX_INFO_ELM_CNT);

  int elm_idx = sinfo->elm_cnt++;
  struct db_info_elm *elm = vinfo->elms + elm_idx;
  return elm;
}
static void
db_info_elm_add(struct db_info_state *sinfo, struct db_info_view *vinfo,
                long long rowid, struct str name, struct str sql) {

  assert(vinfo);
  assert(sinfo);
  assert(sinfo->elm_cnt < DB_MAX_INFO_ELM_CNT);

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
}
static int
db_info_qry_elm(struct db_state *sdb, struct db_info_state *sinfo,
                struct db_info_view *vinfo, int low, int high) {
  int err = 0;
  assert(sdb);
  assert(sinfo);
  assert(vinfo);

  assert(low >= 0);
  assert(low <= high);
  assert(high <= sinfo->tab_cnt[sinfo->sel_tab]);
  assert((high - low) < DB_MAX_INFO_ELM_CNT);
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
  return 0;
}
static void
db_info_sel_elms(struct db_state *sdb, struct db_info_state *sinfo,
                 struct db_info_view *vinfo, const struct gui_lst_sel *sel) {
  assert(sdb);
  assert(sel);
  assert(sinfo);
  assert(vinfo);
  if (sel->mut == GUI_LST_SEL_MOD_REPLACE) {
    tbl_clr(&vinfo->sel);
  }
  if (sel->begin_idx + 1 >= sel->end_idx) {
    /* single-selection */
    if (sinfo->sel_tab == DB_TBL_TYPE_TBL ||
        sinfo->sel_tab == DB_TBL_TYPE_VIEW) {
      assert(sel->idx < cntof(vinfo->elms));

      struct db_info_elm *elm = &vinfo->elms[sel->idx];
      switch (sel->op){
      case GUI_LST_SEL_OP_SET:
        tbl_put(&vinfo->sel, hash_lld(elm->rowid), &elm->rowid); break;
      case GUI_LST_SEL_OP_CLR:
        tbl_del(&vinfo->sel, hash_lld(elm->rowid)); break;
      }
    } else {
      tbl_clr(&vinfo->sel);
    }
  } else if (sel->sel_cnt + vinfo->sel.cnt < cntof(vinfo->sel.keys)) {
    /* multi-selection */
    sqlite3_stmt *stmt = 0;
    struct str sql = strv("SELECT rowid FROM sqlite_master LIMIT ?,?;");
    int err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    sqlite3_bind_int(stmt, 1, sel->begin_idx);
    sqlite3_bind_int(stmt, 2, sel->end_idx - sel->begin_idx);
    assert(err == SQLITE_OK);

    while (sqlite3_step(stmt) == SQLITE_ROW &&
        vinfo->sel.cnt < DB_MAX_INFO_ELM_CNT) {
      long long rowid = sqlite3_column_int64(stmt, 0);
      switch (sel->op){
      case GUI_LST_SEL_OP_SET:
        tbl_put(&vinfo->sel, hash_lld(rowid), &rowid); break;
      case GUI_LST_SEL_OP_CLR:
        tbl_del(&vinfo->sel, hash_lld(rowid)); break;
      }
    }
    sqlite3_finalize(stmt);
  }
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
  assert(sdb);
  assert(ctx);

  sdb->id = str_hash(path);
  sdb->unused = DB_TBL_CNT_MSK;
  int err = sqlite3_open(str_beg(path), &sdb->con);
  if (err != SQLITE_OK) {
    return 0;
  }
  int view = db_tbl_new(sdb);
  db_tbl_add(sdb, view);

  struct gui_split_lay_cfg tbl_cfg = {0};
  tbl_cfg.size = szof(struct db_tbl_col_def);
  tbl_cfg.off = offsetof(struct db_tbl_col_def, ui);
  tbl_cfg.slots = db_tree_col_def;
  tbl_cfg.cnt = DB_TREE_COL_MAX;
  gui.tbl.lay(sdb->info.tbl.state, ctx, &tbl_cfg);
  zero2(sdb->info.tbl.off);

  for arr_loopv(i, sdb->info.tab_cnt) {
    enum db_tbl_type tab_type = cast(enum db_tbl_type, i);
    sdb->info.tab_cnt[i] = db_info_qry_cnt(sdb, tab_type, str_nil);
    sdb->info.tab_act |= castu(!!sdb->info.tab_cnt[i]) << i;
  }
  int sel = cpu_bit_ffs32(sdb->info.tab_act);
  sdb->info.sel_tab = cast(enum db_tbl_type, sel);
  return 1;
}
static void
db_free(struct db_state *sdb) {
  assert(sdb);
  /* cleanup tabs */
  assert(sdb->tab_cnt >= 0);
  assert(sdb->tab_cnt < cntof(sdb->tabs));
  for arr_loopn(i, sdb->tabs, sdb->tab_cnt) {
    assert(i < DB_TBL_CNT);
    int idx = sdb->tabs[i];
    assert(idx < cntof(sdb->tbls));
    db_tbl_del(sdb, idx);
  }
  assert(sdb->unused == DB_TBL_CNT_MSK);
  sdb->unused = DB_TBL_CNT_MSK;
  sdb->tab_cnt = 0;
  if (sdb->con){
    sqlite3_close(sdb->con);
  }
}
static void
db_tab_resort(struct db_state *sdb, int dst_idx, int src_idx) {
  assert(sdb);
  assert(dst_idx >= 0);
  assert(src_idx >= 0);

  assert(dst_idx < sdb->tab_cnt);
  assert(src_idx < sdb->tab_cnt);
  assert(dst_idx < cntof(sdb->tabs));
  assert(src_idx < cntof(sdb->tabs));

  assert(!(sdb->unused & (1U << sdb->tabs[dst_idx])));
  assert(!(sdb->unused & (1U << sdb->tabs[src_idx])));
  iswap(sdb->tabs[dst_idx], sdb->tabs[src_idx]);
}
static int
db_tab_open(struct db_state *sdb, struct db_view *vdb, int view,
            enum db_tbl_type type, struct str tbl_name, long long rowid,
            struct gui_ctx *ctx) {

  assert(sdb);
  assert(vdb);
  assert(ctx);

  assert(sdb->unused > 0);
  assert(view >= 0);
  assert(view < cntof(sdb->tbls));

  assert(sdb->tab_cnt < cntof(sdb->tabs));
  assert(!(sdb->unused & (1llu << view)));
  assert(type == DB_TBL_TYPE_TBL || type == DB_TBL_TYPE_VIEW);

  db_tbl_setup(sdb, vdb, view, ctx, tbl_name, rowid, type);
  sdb->sel_tab = castb(db_tbl_add(sdb, view));
  return sdb->sel_tab;
}
static int
db_tab_open_new(struct db_state *sdb, struct db_view *vdb, enum db_tbl_type type,
                struct str tbl_name, long long rowid, struct gui_ctx *ctx) {
  assert(sdb);
  assert(vdb);
  assert(ctx);

  assert(sdb->unused > 0);
  assert(sdb->tab_cnt < cntof(sdb->tabs));
  assert(type == DB_TBL_TYPE_TBL || type == DB_TBL_TYPE_VIEW);

  int view = db_tbl_new(sdb);
  return db_tab_open(sdb, vdb, view, type, tbl_name, rowid, ctx);
}
static int
db_tab_open_empty(struct db_state *sdb) {
  assert(sdb);
  assert(sdb->unused > 0);
  assert(sdb->tab_cnt < cntof(sdb->tabs));

  int view = db_tbl_new(sdb);
  int tab_idx = db_tbl_add(sdb, view);
  db_tab_resort(sdb, 0, tab_idx);
  return sdb->sel_tab = 0;
}
static void
db_tab_open_tbl_id(struct db_state *sdb, struct db_view *vdb, struct gui_ctx *ctx,
                   int view, long long tbl_id) {
  assert(ctx);
  assert(sdb);
  assert(vdb);

  assert(sdb->unused > 0);
  assert(view < cntof(sdb->tbls));
  assert(sdb->tab_cnt < cntof(sdb->tabs));

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
  if (sdb->tbls[view].state != TBL_VIEW_DISPLAY) {
    db_tbl_setup(sdb, vdb, view, ctx, tbl_name_str, tbl_id, sdb->info.sel_tab);
  } else {
    db_tab_open_new(sdb, vdb, sdb->info.sel_tab, tbl_name_str, tbl_id, ctx);
  }
  sqlite3_finalize(stmt);
}
static void
db_tab_open_tbl_sel(struct db_state *sdb, struct db_view *vdb,
                    struct gui_ctx *ctx, int view) {
  assert(ctx);
  assert(sdb);
  assert(vdb);

  assert(sdb->unused > 0);
  assert(view < cntof(sdb->tbls));
  assert(sdb->tab_cnt < cntof(sdb->tabs));

  for tbl_loop(n, i, &vdb->info.sel) {
    long long rowid = tbl_unref(&vdb->info.sel, n, 0);
    db_tab_open_tbl_id(sdb, vdb, ctx, view, rowid);
  }
}
static void
db_tab_close(struct db_state *sdb, int tab_idx) {
  assert(sdb);
  assert(tab_idx >= 0);
  assert(sdb->tab_cnt > 0);
  assert(sdb->tab_cnt < cntof(sdb->tabs));

  assert(tab_idx < cntof(sdb->tabs));
  assert(tab_idx < sdb->tab_cnt);
  assert(!(sdb->unused & (1llu << sdb->tabs[tab_idx])));

  int tbl = sdb->tabs[tab_idx];
  db_tbl_rm(sdb, tab_idx);
  db_tbl_del(sdb, tbl);
  sdb->sel_tab = clamp(0, sdb->sel_tab, sdb->tab_cnt-1);
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
static struct str
ui_edit_fnd(struct gui_ctx *ctx, struct gui_edit_box *edt,
            struct gui_panel *pan, struct gui_panel *parent,
            struct gui_txt_ed *ted, char *buf, int cap, struct str str) {

  assert(ted);
  assert(buf);
  assert(ctx);

  assert(edt);
  assert(pan);
  assert(parent);

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
ui_tbl_hdr_elm_tog(struct gui_ctx *ctx, struct gui_tbl *tbl,
                   const int *tbl_lay, int *state, int act) {
  assert(ctx);
  assert(tbl);
  assert(state);
  assert(tbl_lay);

  struct gui_btn slot = {0};
  enum res_ico_id ico = act ? RES_ICO_TOGGLE_ON : RES_ICO_TOGGLE_OFF;
  return ui_tbl_hdr_elm_ico(ctx, tbl, tbl_lay, &slot, state, ico);
}
static int
ui_tbl_hdr_elm_lock(struct gui_ctx *ctx, struct gui_tbl *tbl,
                    const int *tbl_lay, int *state, int act) {
  assert(ctx);
  assert(tbl);
  assert(state);
  assert(tbl_lay);

  struct gui_btn slot = {0};
  enum res_ico_id ico = act ? RES_ICO_UNLOCK : RES_ICO_LOCK;
  return ui_tbl_hdr_elm_ico(ctx, tbl, tbl_lay, &slot, state, ico);
}
static int
ui_tbl_lst_elm_col_tog(struct gui_ctx *ctx, struct gui_tbl *tbl,
                       const int *tbl_lay, struct gui_panel *elm, int *is_act) {
  assert(ctx);
  assert(tbl);
  assert(elm);
  assert(is_act);
  assert(tbl_lay);

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
  assert(ctx);
  assert(tbl);
  assert(fltr);
  assert(tbl_cols);

  gui.tbl.hdr.begin(ctx, tbl, tbl_cols, col_cnt, fltr->tbl.state, cntof(fltr->tbl.state));
  {
    /* enable/disable all filters toggle */
    int all_set = db_tbl_fltr_enabled(fltr);
    if (ui_tbl_hdr_elm_tog(ctx, tbl, tbl_cols, fltr->tbl.state, all_set)) {
      for arr_loopn(i, fltr->lst, fltr->cnt) {
        assert(i < cntof(fltr->lst));

        int idx = fltr->lst[i];
        assert(idx < cntof(fltr->elms));
        fltr->elms[idx].enabled = !all_set;
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
  assert(sdb);
  assert(vdb);
  assert(ctx);
  assert(pan);

  assert(stbl);
  assert(vtbl);
  assert(fltr);
  assert(parent);

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
      gui.tbl.lst.cfg(ctx, &cfg, fltr->cnt);
      cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_SINGLE;
      cfg.sel.on = GUI_LST_SEL_ON_HOV;

      int del_idx = -1;
      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for gui_tbl_lst_loopv(i,_,gui,&tbl,fltr->lst) {
        assert(i < cntof(fltr->lst));

        int idx = fltr->lst[i];
        assert(idx < cntof(fltr->elms));
        struct db_tbl_fltr_elm *item = &fltr->elms[idx];

        struct gui_panel elm = {0};
        gui.tbl.lst.elm.begin(ctx, &tbl, &elm, (uintptr_t)item, 0);
        {
          /* columns */
          int is_enabled = item->enabled;
          ui_tbl_lst_elm_col_tog(ctx, &tbl, tbl_cols, &elm, &is_enabled);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, item->fnd, 0);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, item->col_name, 0);
          item->enabled = !!is_enabled;

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
        db_tbl_fltr_view_rm(fltr, del_idx);
        db_tbl_rev(sdb, vdb, stbl, vtbl);
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
  assert(sdb);
  assert(vdb);
  assert(ctx);
  assert(pan);

  assert(stbl);
  assert(vtbl);
  assert(fltr);
  assert(view);
  assert(parent);

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

    confine gui_disable_on_scope(&gui, ctx, !str_len(view->fnd_str)) {
      struct gui_btn add = {0};
      add.box = gui.cut.bot(&lay, ctx->cfg.item, ctx->cfg.gap[1]);
      gui.btn.txt(ctx, &add, pan, strv("Add"), 0);
      if (add.clk) {
        db_tbl_fltr_add_str(sdb, stbl, vtbl, fltr, fltr->ini_col, view->fnd_str);
        db_tbl_close_fltr(stbl);
        db_tbl_rev(sdb, vdb, stbl, vtbl);
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
    for gui_lst_reg_loopv(i,_,gui,&reg,view->data) {
      assert(i >= reg.lst.begin);
      int idx = i - reg.lst.begin;
      assert(idx < cntof(view->rowid));

      struct gui_panel elm = {0};
      unsigned long long id = hash_lld(view->rowid[idx]);
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
  assert(sdb);
  assert(vdb);
  assert(ctx);
  assert(pan);

  assert(stbl);
  assert(vtbl);
  assert(fltr);
  assert(parent);

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
  assert(col);
  assert(ctx);
  assert(slot);

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
  assert(col);
  assert(ctx);
  assert(vtbl);
  assert(stbl);
  assert(slot);

  /* table column header filter icon button */
  struct gui_btn fltr = {.box = slot->pan.box};
  fltr.box.x = gui.bnd.max_ext(slot->pan.box.x.max, ctx->cfg.item);
  int dis = col->blob || (stbl->fltr.cnt >= cntof(stbl->fltr.lst));
  confine gui_disable_on_scope(&gui, ctx, dis) {
    if (gui.btn.ico(ctx, &fltr, &slot->pan, RES_ICO_SEARCH)) {
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
  assert(ctx);
  assert(slot);
  static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
  struct gui_panel pan = {.box = slot->pan.box};
  gui_txt(ctx, &pan, &slot->pan, txt, &align);
}
static void
ui_db_tbl_view_hdr_slot(struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                        struct db_tbl_col *col, struct gui_ctx *ctx,
                        struct gui_btn *slot) {
  assert(col);
  assert(ctx);
  assert(vtbl);
  assert(stbl);

  /* table column header filter icon button */
  struct gui_btn fltr = {.box = slot->pan.box};
  fltr.box.x = gui.bnd.max_ext(slot->pan.box.x.max, ctx->cfg.item);
  int dis = col->blob || (stbl->fltr.cnt >= cntof(stbl->fltr.lst));
  confine gui_disable_on_scope(&gui, ctx, dis) {
    if (gui.btn.ico(ctx, &fltr, &slot->pan, RES_ICO_SEARCH)) {
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
  assert(sdb);
  assert(vdb);
  assert(ctx);
  assert(pan);

  assert(stbl);
  assert(vtbl);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    int back = 0;
    int front = 0;

    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, stbl->row.ui.off, 0);
    {
      if (vtbl->col.id != stbl->rowid) {
        /* reload column data */
        if (stbl->col.state == DB_TBL_COL_STATE_UNLOCKED) {
          stbl->col.total = stbl->col.rng.total;
          stbl->col.rng.lo = stbl->col.rng.hi = stbl->col.rng.cnt = 0;
          stbl->col.rng.total = stbl->col.sel.cnt;
          db_tbl_qry_cols(sdb, vdb, stbl, vtbl, 0, 0, 1);
        } else {
          db_tbl_qry_row_cols(sdb, vdb, stbl, vtbl, stbl->row.cols.lo);
        }
      }
      /* header */
      int tbl_lay[GUI_TBL_COL(DB_MAX_TBL_ROW_COLS)];
      gui.tbl.hdr.begin(ctx, &tbl, tbl_lay, cntof(tbl_lay), stbl->row.ui.state, cntof(stbl->row.ui.state));
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
          int dis = stbl->col.rng.lo == 0 || stbl->col.rng.cnt == stbl->col.rng.total;
          confine gui_disable_on_scope(&gui, ctx, dis) {
            back = gui__scrl_btn(ctx, &prv, &slot.pan, GUI_WEST);
          }
          slot.pan.box.x = gui.bnd.min_max(prv.box.x.max, slot.pan.box.x.max);
        }
        if (i + 1 == stbl->row.cols.hi) {
          struct gui_btn nxt = {.box = slot.pan.box};
          nxt.box.x = gui.bnd.max_ext(slot.pan.box.x.max, ctx->cfg.scrl);

          /* move column window to the right */
          int fits = stbl->col.rng.cnt == stbl->col.rng.total;
          int end = (i >= (stbl->col.rng.total - stbl->row.cols.cnt));
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
        unsigned long long id = hash_lld(rowid);

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
ui_db_tbl_view_dsp_layout(struct db_state *sdb, struct db_view *vdb,
                          struct db_tbl_state *stbl, struct db_tbl_view *vtbl,
                          struct gui_ctx *ctx, struct gui_panel *pan,
                          struct gui_panel *parent) {
  assert(sdb);
  assert(vdb);
  assert(ctx);
  assert(pan);

  assert(stbl);
  assert(vtbl);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, stbl->col.ui.off, 0);
    {
      /* header */
      int tbl_cols[GUI_TBL_COL(DB_TBL_DISP_COL_MAX)];
      gui.tbl.hdr.begin(ctx, &tbl, tbl_cols, cntof(tbl_cols), stbl->col.ui.state, cntof(stbl->col.ui.state));
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
        assert(i < cntof(vtbl->col.lst));
        assert(i < stbl->col.rng.total);

        struct db_tbl_col *col = &vtbl->col.lst[i];
        struct str col_name = str_buf_get(&vtbl->col.buf, col->name);
        struct str col_type = str_buf_get(&vtbl->col.buf, col->type);

        struct gui_panel item = {0};
        unsigned long long key = hash_lld(col->rowid);
        gui.tbl.lst.elm.begin(ctx, &tbl, &item, key, 0);
        {
          int dis = stbl->col.state == DB_TBL_COL_STATE_LOCKED;
          int is_act = (dis || tbl_has(&stbl->col.sel, key));
          confine gui_disable_on_scope(&gui, ctx, dis) {
            /* column selection */
            if (ui_tbl_lst_elm_col_tog(ctx, &tbl, tbl_cols, &item, &is_act)) {
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
  assert(sdb);
  assert(vdb);
  assert(ctx);
  assert(pan);

  assert(stbl);
  assert(vtbl);
  assert(parent);

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
          assert(i < cntof(tabs));
          gui.tab.hdr.slot.begin(ctx, &tab, &hdr, &slot, tabs[i].hash);
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
        ui_db_tbl_view_dsp_layout(sdb, vdb, stbl, vtbl, ctx, &bdy, &tab.pan);
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
          stbl->col.total = stbl->col.rng.total;
          stbl->col.rng.lo = stbl->col.rng.hi = stbl->col.rng.cnt = 0;
          stbl->col.rng.total = stbl->col.sel.cnt;
          db_tbl_qry_cols(sdb, vdb, stbl, vtbl, 0, 0, 1);
        }
      }
      if (tab.sel.idx == DB_TBL_VIEW_DSP_LAYOUT) {
        if (stbl->col.state == DB_TBL_COL_STATE_UNLOCKED) {
          stbl->col.rng.lo = stbl->col.rng.hi = stbl->col.rng.cnt = 0;
          stbl->col.rng.total = stbl->col.total;
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
  assert(sdb);
  assert(vdb);
  assert(ctx);
  assert(pan);

  assert(sinfo);
  assert(vinfo);
  assert(parent);
  assert(view < cntof(sdb->tbls));

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
      gui.tbl.hdr.begin(ctx, &tbl, tbl_cols, cntof(tbl_cols), sinfo->tbl.state, cntof(sinfo->tbl.state));
      for arr_eachv(col, db_tree_col_def) {
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_cols, sinfo->tbl.state, col->title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, sinfo->tab_cnt[sinfo->sel_tab]);
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_MULTI;

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
        int is_sel = tbl_has(&vinfo->sel, hash_lld(elm->rowid));
        gui.tbl.lst.elm.begin(ctx, &tbl, &item, hash_lld(elm->rowid), is_sel);
        {
          gui.tbl.lst.elm.col.txt_ico(ctx, &tbl, tbl_cols, &item, elm_name, types[sinfo->sel_tab].ico);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &item, types[sinfo->sel_tab].type, 0);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &item, elm_sql, 0);
        }
        gui.tbl.lst.elm.end(ctx, &tbl, &item);

        /* input handling */
        struct gui_input pin = {0};
        gui.pan.input(&pin, ctx, &item, GUI_BTN_LEFT);
        if (pin.mouse.btn.left.doubled) {
          open_tbl_idx = elm_idx;
          open_tbl = 1;
        }
      }
      gui.tbl.lst.end(ctx, &tbl);
      if (tbl.lst.sel.mod) {
        db_info_sel_elms(sdb, sinfo, vinfo, &tbl.lst.sel);
      }
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
  assert(sdb);
  assert(vdb);
  assert(ctx);
  assert(pan);

  assert(sinfo);
  assert(vinfo);
  assert(parent);
  assert(view < cntof(sdb->tbls));

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
      for arr_loopn(i, sinfo->tab_cnt, tab.cnt) {
        assert(i < cntof(tabs));
        const struct tab_def *def = &tabs[i];

        /* tab header slots */
        int dis = !(sinfo->tab_act & (1U << i));
        confine gui_disable_on_scope(&gui, ctx, dis) {
          struct gui_panel slot = {0};
          gui.tab.hdr.slot.begin(ctx, &tab, &hdr, &slot, str_hash(def->title));
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
ui_db_main(struct db_state *sdb, struct db_view *vdb, int view,
           struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent) {

  assert(sdb);
  assert(vdb);
  assert(ctx);
  assert(pan);
  assert(parent);

  assert(view >= 0);
  assert(view < cntof(sdb->tbls));
  assert(!(sdb->unused & (1U << castu(view))));

  struct db_tbl_view *vtbl = &vdb->tbl;
  struct db_tbl_state *stbl = &sdb->tbls[view];
  gui.pan.begin(ctx, pan, parent);
  {
    int gap = ctx->cfg.gap[1];
    struct gui_box lay = pan->box;
    switch (stbl->state) {
    case TBL_VIEW_SELECT: {

      struct gui_btn open = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      struct gui_panel overview = {.box = lay};
      ui_db_view_info(sdb, vdb, view, &sdb->info, &vdb->info, ctx, &overview, pan);

      /* open table */
      int dis = !sdb->unused || !vdb->info.sel.cnt;
      confine gui_disable_on_scope(&gui, ctx, dis) {
        if (ui_btn_ico(ctx, &open, pan, strv("Open"), RES_ICO_TH_LIST, 0)) {
          db_tab_open_tbl_sel(sdb, vdb, ctx, view);
          tbl_clr(&vdb->info.sel);
        }
      }
    } break;
    case TBL_VIEW_DISPLAY: {
      /* table view */
      struct gui_panel lst = {.box = pan->box};
      ui_db_tbl_view_dsp(sdb, vdb, stbl, vtbl, ctx, &lst, pan);
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
  assert(ico);
  assert(parent);

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
ui_db_explr_tab_slot(struct db_state *sdb, struct db_tbl_state *stbl,
                     struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                     struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot,
                     struct str title, enum res_ico_id ico) {
  assert(sdb);
  assert(ctx);
  assert(tab);
  assert(hdr);
  assert(ico);

  assert(stbl);
  assert(slot);

  int ret = 0;
  unsigned long long tab_id = hash_lld(stbl->rowid);
  gui.tab.hdr.slot.begin(ctx, tab, hdr, slot, tab_id);
  if (sdb->tab_cnt > 1 && tab->idx == tab->sel.idx) {
    ret = ui_db_explr_tab_slot_close(ctx, slot, &hdr->pan, title, ico);
  } else {
    gui.ico.box(ctx, slot, &hdr->pan, ico, title);
  }
  gui.tab.hdr.slot.end(ctx, tab, hdr, slot, 0);
  return ret;
}
static int
ui_db_explr_tab(struct db_state *sdb, struct db_tbl_state *stbl,
                struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot) {
  assert(sdb);
  assert(ctx);
  assert(tab);
  assert(hdr);

  assert(stbl);
  assert(slot);

  struct str title = strv("Info");
  enum res_ico_id ico = RES_ICO_CUBE;
  if (stbl->state != TBL_VIEW_SELECT) {
    ico = ui_db_tbl_lst_elm_ico(stbl->kind);
    title = stbl->title;
  }
  return ui_db_explr_tab_slot(sdb, stbl, ctx, tab, hdr, slot, title, ico);
}
static int
ui_db_tab_view_lst(struct db_state *sdb, struct gui_ctx *ctx, struct gui_panel *pan,
                   struct gui_panel *parent) {

  assert(sdb);
  assert(ctx);
  assert(pan);
  assert(parent);

  int ret = -1;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_lst_cfg cfg = {0};
    int cnt = min(sdb->tab_cnt, cntof(sdb->tabs));
    gui.lst.cfg(&cfg, cnt, sdb->tbl_lst_off[1]);
    cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
    cfg.sel.on = GUI_LST_SEL_ON_HOV;

    assert(sdb->tab_cnt < cntof(sdb->tabs));
    struct gui_lst_reg reg = {.box = pan->box};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, sdb->tbl_lst_off);
    for gui_lst_reg_loopv(i,_,gui,&reg,sdb->tabs) {

      assert(i < cntof(sdb->tabs));
      int idx = sdb->tabs[i];
      assert(idx < cntof(sdb->tbls));
      struct db_tbl_state *tbl = &sdb->tbls[idx];

      struct str title = strv("Info");
      enum res_ico_id ico = RES_ICO_CUBE;
      if (tbl->state != TBL_VIEW_SELECT) {
        ico = ui_db_tbl_lst_elm_ico(tbl->kind);
        title = tbl->title;
      }
      struct gui_panel elm = {0};
      unsigned long long n = cast(unsigned long long, i);
      unsigned long long id = fnv1au64(n, FNV1A64_HASH_INITIAL);
      gui.lst.reg.elm.txt_ico(ctx, &reg, &elm, id, 0, title, ico);

      struct gui_input pin = {0};
      gui.pan.input(&pin, ctx, &elm, GUI_BTN_LEFT);
      ret = pin.mouse.btn.left.clk ? i : ret;
    }
    gui.lst.reg.end(ctx, &reg, pan, sdb->tbl_lst_off);
  }
  gui.pan.end(ctx, pan, parent);
  return ret;
}
static void
ui_db_explr(struct db_state *sdb, struct db_view *vdb, struct gui_ctx *ctx,
            struct gui_panel *pan, struct gui_panel *parent) {

  assert(sdb);
  assert(vdb);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    if (sdb->id != vdb->id) {
      /* reset revision */
      vdb->id = sdb->id;
      vdb->info.id = 0;
      vdb->tbl.row.id = 0;
      vdb->tbl.col.id = 0;
      vdb->tbl.fltr.id = 0;
    }
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box, .show_btn = 1};
    gui.tab.begin(ctx, &tab, pan, sdb->tab_cnt, sdb->sel_tab);
    {
      /* tab header */
      int del_tab = 0;
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      for arr_loopn(i, sdb->tabs, tab.cnt) {
        /* tab header slots */
        assert(i < cntof(sdb->tabs));
        int idx = sdb->tabs[i];
        assert(idx < cntof(sdb->tbls));

        struct gui_panel slot = {0};
        struct db_tbl_state *tbl = &sdb->tbls[idx];
        if (ui_db_explr_tab(sdb, tbl, ctx, &tab, &hdr, &slot)) {
          del_tab = 1;
        }
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);
      if (tab.sort.mod) {
        db_tab_resort(sdb, tab.sort.dst, tab.sort.src);
      }
      if (del_tab) {
        db_tab_close(sdb, tab.sel.idx);
      }
      confine gui_disable_on_scope(&gui, ctx, sdb->unused == 0) {
        struct gui_btn add = {.box = hdr.pan.box};
        add.box.x = gui.bnd.min_ext(tab.off, ctx->cfg.item);
        if (gui.btn.ico(ctx, &add, &hdr.pan, RES_ICO_FOLDER_ADD)) {
          /* open new table view tab */
          db_tab_open_empty(sdb);
        }
      }
      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      sdb->show_tab_lst = tab.btn.clk ? !sdb->show_tab_lst : sdb->show_tab_lst;
      if (sdb->show_tab_lst) {
        /* overflow tab selection */
        int ret = ui_db_tab_view_lst(sdb, ctx, &bdy, pan);
        if (ret >= 0) {
          db_tab_resort(sdb, 0, ret);
          sdb->show_tab_lst = 0;
          sdb->sel_tab = 0;
        }
      } else {
        assert(sdb->sel_tab < sdb->tab_cnt);
        ui_db_main(sdb, vdb, sdb->tabs[sdb->sel_tab], ctx, &bdy, pan);
      }
    }
    gui.tab.end(ctx, &tab, pan);
    if (tab.sel.mod) {
      sdb->sel_tab = castb(tab.sel.idx);
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

