/* ---------------------------------------------------------------------------
 *
 *                                Database
 *
 * ---------------------------------------------------------------------------
 */
#define db_str(s) str_beg(s), str_len(s)

// clang-format off
static const struct db_tbl_col_def db_tbl_fltr_def[DB_TBL_FLTR_MAX] = {
  [DB_TBL_FLTR_STATE] = {.title = strv(""),                                                                     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {10, 200}}},
  [DB_TBL_FLTR_BUF]   = {.title = strv("Filter"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 800}}},
  [DB_TBL_FLTR_COL]   = {.title = strv("Column"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 800}}},
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
static const struct db_tbl_col_def db_tbl_disp_col_def[DB_TBL_DISP_COL_MAX] = {
  [DB_TBL_DISP_COL_ACT]       = {.title = strv(""),     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
  [DB_TBL_DISP_COL_NAME]      = {.title = strv("Name"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 800}}},
  [DB_TBL_DISP_COL_TYPE]      = {.title = strv("Type"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 800}}},
  [DB_TBL_DISP_COL_PK]        = {.title = strv("PK"),   .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
  [DB_TBL_DISP_COL_FK]        = {.title = strv("FK"),   .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
  [DB_TBL_DISP_COL_NN]        = {.title = strv("!0"),   .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
  [DB_TBL_DISP_COL_FLTR]      = {.title = strv(""),     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
};
// clang-format on
struct db_name_lck {
  struct str name;
  sqlite3_stmt *stmt;
};
static void
db_tbl_name_acq(struct db_name_lck *lck, struct db_view *db,
                struct db_tbl_view *tbl) {
  assert(db);
  assert(lck);
  assert(tbl);

  lck->stmt = 0;
  if (tbl->qry_name) {
    struct str sql = strv("SELECT name FROM sqlite_master WHERE rowid = ?;");
    sqlite3_prepare_v2(db->con, db_str(sql), &lck->stmt, 0);
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
db_tbl_col_name_acq(struct db_name_lck *lck, struct db_view *db,
                    struct db_tbl_view *tbl, struct db_tbl_col *col,
                    struct str tbl_name, long long col_id) {
  assert(db);
  assert(lck);
  assert(tbl);

  if (!col || col->qry_name) {
    struct str sql = strv("SELECT name FROM pragma_table_info(?) WHERE rowid = ?;");
    sqlite3_prepare_v2(db->con, db_str(sql), &lck->stmt, 0);
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
  assert(fltr->cnt < cntof(fltr->lst));
  assert(!(fltr->unused & (1u << idx)));

  fltr->lst[fltr->cnt] = castb(idx);
  return fltr->cnt++;
}
static void
db_tbl_view_fltr_view_fltr_rm(struct db_tbl_fltr_view *fltr, int idx) {
  assert(fltr);
  assert(idx >= 0);
  assert(idx < fltr->cnt);
  assert(idx < cntof(fltr->lst));
  assert(fltr->cnt < cntof(fltr->lst));
  assert(!(fltr->unused & (1u << fltr->lst[idx])));

  arr_rm(fltr->lst, idx, fltr->cnt);
  fltr->cnt--;
}
static int
db_tbl_view_fltr_view_add_str(struct db_view *db, struct db_tbl_view *tbl,
                              struct db_tbl_fltr_view *fltr, long long col,
                              struct str str) {
  assert(fltr);
  assert(fltr->unused);
  assert(str_len(str));
  assert(str_len(str) < DB_MAX_FLTR_STR);
  assert(fltr->cnt < cntof(fltr->lst));

  int idx = db_tbl_view_fltr_view_fltr_new(fltr);
  struct db_tbl_fltr_elm *elm = &fltr->elms[idx];
  elm->type = DB_TBL_FLTR_ELM_TYP_STR;
  elm->fnd = str_sqz(elm->fnd_buf, cntof(elm->fnd_buf), str);
  elm->enabled = 1;
  elm->col = col;

  struct db_name_lck tlck = {0}, clck = {0};
  db_tbl_name_acq(&tlck, db, tbl);
  db_tbl_col_name_acq(&clck, db, tbl, 0, tlck.name, col);
  elm->col_name = str_sqz(elm->col_buf, cntof(elm->col_buf), clck.name);
  db_tbl_col_name_rel(&clck);
  db_tbl_name_rel(&tlck);

  return db_tbl_view_fltr_view_fltr_add(fltr, idx);
}
static void
db_tbl_view_fltr_view_rm(struct db_tbl_fltr_view *fltr, int idx) {
  assert(fltr);
  assert(idx >= 0);
  assert(idx < fltr->cnt);
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
  fltr->cnt = 0;
}
static void
db_tbl_view_fltr_open(struct db_tbl_view *tbl, long long col) {
  assert(tbl);
  mset(&tbl->fltr.data_rng, szof(tbl->fltr.data_rng), 0);
  tbl->disp = DB_TBL_VIEW_DSP_FILTER;
  tbl->fltr.state = DB_TBL_FLTR_EDT;
  tbl->fltr.data_rng.total = tbl->row.rng.total;
  tbl->fltr.fnd_str = str_nil;
  tbl->fltr.ini_col = col;
  tbl->fltr.init = 1;
}
static void
db_tbl_view_fltr_close(struct db_tbl_view *tbl) {
  tbl->disp = DB_TBL_VIEW_DSP_DATA;
  tbl->fltr.state = DB_TBL_FLTR_LST;
  tbl->fltr.fnd_str = str_nil;
}
static void
db_tbl_view_fltr_view_qry(struct db_view *db, struct db_tbl_view *tbl,
                          struct db_tbl_fltr_view *fltr, int lo, int hi) {
  assert(db);
  assert(tbl);
  assert(fltr);
  assert(lo >= 0);
  assert(hi >= 0);
  assert(lo <= hi);
  assert(lo <= tbl->fltr.data_rng.total);
  assert(hi <= tbl->fltr.data_rng.total);
  assert((hi - lo) <= cntof(tbl->fltr.data));

  fltr->data_rng.lo = lo;
  fltr->data_rng.hi = hi;
  fltr->data_rng.cnt = 0;
  str_buf_clr(&fltr->buf);

  /* query table and column name */
  struct db_name_lck tlck = {0}, clck = {0};
  db_tbl_name_acq(&tlck, db, tbl);
  db_tbl_col_name_acq(&clck, db, tbl, 0, tlck.name, fltr->ini_col);

  /* query total filtered element count */
  sqlite3_stmt *stmt = 0;
  if (str_len(fltr->fnd_str)) {
    struct str sql = str_fmtsn(db->sql_qry_buf, cntof(db->sql_qry_buf),
      "SELECT COUNT(*) FROM %.*s WHERE %.*s LIKE '%%'||?||'%%';",
      strf(tlck.name), strf(clck.name));
    int rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, db_str(fltr->fnd_str), SQLITE_STATIC);
    assert(rc == SQLITE_OK);
  } else {
    struct str sql = str_fmtsn(db->sql_qry_buf, cntof(db->sql_qry_buf),
      "SELECT COUNT(*) FROM '%.*s';", strf(tlck.name));
    int rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
    assert(rc == SQLITE_OK);
  }
  int rc = sqlite3_step(stmt);
  assert(rc == SQLITE_ROW);
  fltr->data_rng.total = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  stmt = 0;

  /* query table filered elements */
  if (str_len(fltr->fnd_str)) {
    struct str sql = str_fmtsn(db->sql_qry_buf, cntof(db->sql_qry_buf),
      "SELECT rowid, %.*s FROM %.*s WHERE %.*s LIKE '%%'||?||'%%' LIMIT ?,?;",
      strf(clck.name), strf(tlck.name), strf(clck.name));
    rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, db_str(fltr->fnd_str), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, lo);
    sqlite3_bind_int(stmt, 3, hi-lo);
    assert(rc == SQLITE_OK);
  } else {
    struct str sql = str_fmtsn(db->sql_qry_buf, cntof(db->sql_qry_buf),
      "SELECT rowid, %.*s FROM %.*s LIMIT ?,?;", strf(clck.name), strf(tlck.name));
    rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
    sqlite3_bind_int(stmt, 1, lo);
    sqlite3_bind_int(stmt, 2, hi-lo);
    assert(rc == SQLITE_OK);
  }
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    long long rowid = sqlite3_column_int64(stmt, 0);
    const char *dat = (const char*)sqlite3_column_text(stmt, 1);
    int len = sqlite3_column_bytes(stmt, 1);
    struct str elm = strn(dat,len);

    fltr->rowid[fltr->data_rng.cnt] = rowid;
    fltr->data[fltr->data_rng.cnt] = str_buf_sqz(&fltr->buf, elm, DB_MAX_FLTR_ELM_STR);
    fltr->data_rng.cnt++;
  }
  assert(rc == SQLITE_DONE);
  db_tbl_col_name_rel(&clck);
  db_tbl_name_rel(&tlck);
  rc = sqlite3_finalize(stmt);
  assert(rc == SQLITE_OK);
  stmt = 0;
}
/* ---------------------------------------------------------------------------
 *                                Table View
 * ---------------------------------------------------------------------------
 */
static int
db_tbl_view_new(struct db_view *db) {
  assert(db);
  assert(db->unused > 0);

  int idx = cpu_bit_ffs32(db->unused);
  db->unused &= ~(1u << idx);
  mset(db->tbls + idx, 0, szof(db->tbls[0]));
  return idx;
}
static enum res_ico_id
db__tbl_view_col_ico(struct str type) {
  if (str_eq(type, strv("REAL")) ||
      str_eq(type, strv("DOUBLE")) ||
      str_eq(type, strv("DOUBLE PRECISION")) ||
      str_eq(type, strv("FLOAT"))) {
    return RES_ICO_MODIFY;
  } else if (str_eq(type, strv("DATE"))) {
    return RES_ICO_CALENDAR;
  } else if (str_eq(type, strv("DATETIME"))) {
    return RES_ICO_CALENDAR;
  } else if (str_eq(type, strv("BLOB"))) {
    return RES_ICO_CUBE;
  } else if (str_eq(type, strv("BOOLEAN"))) {
    return RES_ICO_CHECK;
  } else if (str_eq(type, strv("INT")) ||
      str_eq(type, strv("INTEGER")) ||
      str_eq(type, strv("TINYINT")) ||
      str_eq(type, strv("SMALLINT")) ||
      str_eq(type, strv("MEDIUMINT")) ||
      str_eq(type, strv("BIGINT")) ||
      str_eq(type, strv("UNSIGNED BIG INT")) ||
      str_eq(type, strv("INT2")) ||
      str_eq(type, strv("INT8")) ||
      str_eq(type, strv("NUMERIC"))) {
    return RES_ICO_CALCULATOR;
  } else {
    return RES_ICO_FONT;
  }
}
static void
db_tbl_view__qry_cols(struct db_view *db, struct db_tbl_view *tbl,
                      int lo, int hi, int sel) {
  assert(db);
  assert(tbl);
  assert(lo >= 0);
  assert(hi >= 0);
  assert(lo <= hi);
  assert(lo < tbl->col.rng.total);
  assert(hi <= tbl->col.rng.total);
  assert((hi - lo) <= cntof(tbl->col.lst));

  int rc = 0;
  tbl->row.rng.lo = 0;
  tbl->row.rng.hi = 0;
  tbl->row.rng.cnt = 0;

  str_buf_clr(&tbl->col.buf);
  if (sel) {
    tbl->col.rng.lo = 0;
    tbl->col.rng.hi = tbl->col.sel.cnt;
    tbl->col.rng.cnt = 0;

    tbl->row.cols.lo = 0;
    tbl->row.cols.hi = tbl->col.sel.cnt;
  } else {
    tbl->col.rng.lo = lo;
    tbl->col.rng.hi = hi;
    tbl->col.rng.cnt = 0;

    tbl->row.cols.lo = lo;
    tbl->row.cols.hi = lo + tbl->row.cols.cnt;
  }
  /* query table name */
  struct db_name_lck lck = {0};
  db_tbl_name_acq(&lck, db, tbl);

  /* query table columns */
  sqlite3_stmt *stmt = 0;
  if (sel) {
    struct str sql = strv("SELECT rowid, name, type, \"notnull\", pk FROM pragma_table_info(?);");
    sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, db_str(lck.name), SQLITE_STATIC);
  } else {
    struct str sql = strv("SELECT rowid, name, type, \"notnull\", pk FROM pragma_table_info(?) LIMIT ?,?;");
    sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, db_str(lck.name), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, lo);
    sqlite3_bind_int(stmt, 3, hi-lo);
  }
  struct tbl(int, DB_MAX_TBL_COLS) col_tbl = {0};
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    long long col_rowid = sqlite3_column_int64(stmt, 0);
    const char *col_name = (const char*)sqlite3_column_text(stmt, 1);
    const char *col_type = (const char*)sqlite3_column_text(stmt, 2);
    const char *col_nn = (const char*)sqlite3_column_text(stmt, 3);
    const char *col_key = (const char*)sqlite3_column_text(stmt, 4);
    if (sel && !tbl_has(&tbl->col.sel, hash_lld(col_rowid))) {
      continue;
    }
    int nam_len = sqlite3_column_bytes(stmt, 1);
    int typ_len = sqlite3_column_bytes(stmt, 2);

    struct str name = strn(col_name, nam_len);
    struct str type = strn(col_type, typ_len);

    /* add column into table */
    int col_idx = tbl->col.rng.cnt++;
    struct db_tbl_col *col = &tbl->col.lst[col_idx];
    col->rowid = col_rowid;
    col->name = str_buf_sqz(&tbl->col.buf, name, DB_MAX_TBL_COL_NAME);
    col->type = str_buf_sqz(&tbl->col.buf, type, DB_MAX_TBL_COL_TYPE);
    col->qry_name = str_buf_len(col->name) < nam_len;
    col->ico = db__tbl_view_col_ico(type);
    col->pk = !strcmp(col_key, "1");
    col->nn = !strcmp(col_nn, "1");
    col->blob = !str_len(type) || col_type[0] == 0;
    col->blob = col->blob || str_eq(type, strv("BLOB"));
    if (col->pk && col->blob) {
      tbl->blob.disabled = 1;
    }
    unsigned long long key = str_hash(name);
    tbl_put(&col_tbl, key, &col_idx);
  }
  assert(rc == SQLITE_DONE);
  rc = sqlite3_finalize(stmt);
  assert(rc == SQLITE_OK);
  stmt = 0;

  /* query table column foreign keys */
  struct str sql = strv("SELECT * FROM pragma_foreign_key_list(?);");
  sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
  sqlite3_bind_text(stmt, 1, db_str(lck.name), SQLITE_STATIC);
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    const char *from = (const char*)sqlite3_column_text(stmt, 3);
    int from_len = sqlite3_column_bytes(stmt, 3);
    struct str from_str = strn(from, from_len);

    unsigned long long hash = str_hash(from_str);
    int it = tbl_fnd(&col_tbl, hash);
    if (tbl_val(&col_tbl, it)) {
      int idx = tbl_unref(&col_tbl, it, 0);
      struct db_tbl_col *col = &tbl->col.lst[idx];
      struct str col_name = str_buf_get(&tbl->col.buf, col->name);
      int min_len = min(str_len(col_name), from_len);
      if (!memcmp(str_beg(col_name), from, min_len)) {
        col->fk = 1;
      }
    }
  }
  assert(rc == SQLITE_DONE);
  rc = sqlite3_finalize(stmt);
  assert(rc == SQLITE_OK);
  db_tbl_name_rel(&lck);
}
static void
db_tbl_view__qry_row_cols(struct db_view *db, struct db_tbl_view *tbl, int lo) {
  assert(db);
  assert(tbl);
  assert(lo >= 0);
  assert(lo + tbl->row.cols.cnt <= tbl->col.rng.total);

  tbl->row.rng.lo = 0;
  tbl->row.rng.hi = 0;
  tbl->row.rng.cnt = 0;

  int hi = lo + tbl->row.cols.cnt;
  if (!rng_has_inclv(&tbl->col.rng, lo)) {
    int clo = max(0, lo - (tbl->col.cnt >> 1u));
    db_tbl_view__qry_cols(db, tbl, clo, clo + tbl->col.cnt, 0);
  } else if (!rng_has_inclv(&tbl->col.rng, hi)) {
    int clo = min(tbl->col.rng.total - tbl->col.cnt, hi + (tbl->col.cnt >> 1u));
    db_tbl_view__qry_cols(db, tbl, clo, clo + tbl->col.cnt, 0);
  }
  tbl->row.cols.lo = lo;
  tbl->row.cols.hi = hi;
}
static struct str
db_tbl_view__qry_fltr_sql(struct db_view *db, struct db_tbl_view *tbl,
                          struct str tbl_name, struct str sql) {
  assert(db);
  assert(tbl);

  /* fill with active filter conditins */
  const char *pre = " WHERE";
  sql = str_add_fmt(db->sql_qry_buf, cntof(db->sql_qry_buf), sql,
    " FROM '%.*s' ", strf(tbl_name));
  for loop(i, tbl->fltr.cnt) {
    int idx = tbl->fltr.lst[i];
    struct db_tbl_fltr_elm *elm = &tbl->fltr.elms[idx];
    if (!elm->enabled) {
      continue;
    }
    struct db_name_lck clck = {0};
    db_tbl_col_name_acq(&clck, db, tbl, 0, tbl_name, elm->col);
    sql = str_add_fmt(db->sql_qry_buf, cntof(db->sql_qry_buf), sql,
      "%s %.*s LIKE '%%%.*s%%'", pre, strf(clck.name), strf(elm->fnd));
    db_tbl_col_name_rel(&clck);
    pre = " AND";
  }
  return sql;
}
static int
db_tbl_view_qry_row_cnt(struct db_view *db, struct db_tbl_view *tbl) {
  assert(db);
  assert(tbl);

  /* query table name */
  struct db_name_lck lck = {0};
  db_tbl_name_acq(&lck, db, tbl);
  struct str sql = str_set(db->sql_qry_buf, cntof(db->sql_qry_buf), strv("SELECT COUNT(*)"));
  sql = db_tbl_view__qry_fltr_sql(db, tbl, lck.name, sql);
  db_tbl_name_rel(&lck);

  sqlite3_stmt *stmt = 0;
  sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
  int rc = sqlite3_step(stmt);
  assert(rc == SQLITE_ROW);
  int cnt = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return cnt;
}
static void
db_tbl_view__qry_rows(struct db_view *db, struct db_tbl_view *tbl, int lo, int hi) {
  assert(db);
  assert(tbl);
  assert(lo >= 0);
  assert(hi >= 0);
  assert(lo <= hi);
  assert(lo <= tbl->row.rng.total);
  assert(hi <= tbl->row.rng.total);
  assert((hi - lo) <= cntof(tbl->row.lst));

  tbl->row.rng.lo = lo;
  tbl->row.rng.hi = hi;
  tbl->row.rng.cnt = 0;
  str_buf_clr(&tbl->row.buf);

  struct db_name_lck tlck = {0};
  db_tbl_name_acq(&tlck, db, tbl);

  /* setup sql query string */
  sqlite3_stmt *stmt = 0;
  struct str sql = str_set(db->sql_qry_buf, cntof(db->sql_qry_buf), strv("SELECT rowid, "));
  for loopr(i, tbl->row.cols) {
    struct db_name_lck clck = {0};
    struct db_tbl_col *col = &tbl->col.lst[i - tbl->col.rng.lo];
    db_tbl_col_name_acq(&clck, db, tbl, col, tlck.name, col->rowid);
    sql = str_add_fmt(db->sql_qry_buf, cntof(db->sql_qry_buf), sql, "%.*s, ", strf(clck.name));
    db_tbl_col_name_rel(&clck);
  }
  sql.rng = rng_lhs(&sql.rng, sql.rng.cnt-2);
  sql = db_tbl_view__qry_fltr_sql(db, tbl, tlck.name, sql);
  sql = str_add_fmt(db->sql_qry_buf, cntof(db->sql_qry_buf), sql, "LIMIT %d,%d", lo, hi);
  db->sql_qry_buf[min(sql.rng.cnt, cntof(db->sql_qry_buf)-1)] = 0;

  /* query table rows */
  int elm_idx = 0;
  int rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
  assert(rc == SQLITE_OK);
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    assert(elm_idx < cntof(tbl->row.rowids));
    tbl->row.rowids[tbl->row.rng.cnt++] = sqlite3_column_int64(stmt, 0);
    for loop(i, tbl->row.cols.cnt) {
      const char *dat = (const char*)sqlite3_column_text(stmt, i+1);
      int len = sqlite3_column_bytes(stmt, i+1);
      struct str data = strn(dat, len);

      assert(elm_idx < cntof(tbl->row.lst));
      tbl->row.lst[elm_idx] = str_buf_sqz(&tbl->row.buf, data, DB_MAX_TBL_ELM_DATA);
      elm_idx++;
    }
  }
  assert(rc == SQLITE_DONE);
  rc = sqlite3_finalize(stmt);
  assert(rc == SQLITE_OK);
  db_tbl_name_rel(&tlck);
  stmt = 0;
}
static void
db_tbl_view_rev(struct db_view *db, struct db_tbl_view *tbl) {
  tbl->row.rng.total = db_tbl_view_qry_row_cnt(db, tbl);
  tbl->row.rng.lo = 0;
  tbl->row.rng.hi = 0;
  tbl->row.rng.cnt = 0;
}
static void
db_tbl_view_setup(struct db_view *db, int idx, struct gui_ctx *ctx,
                  struct str id, long long rowid, enum db_tbl_type kind) {
  assert(db);
  assert(ctx);
  assert(str_is_val(id));

  /* setup table view */
  struct db_tbl_view *tbl = &db->tbls[idx];
  mset(&tbl->row.rng, 0, szof(tbl->row.rng));
  mset(&tbl->col.rng, 0, szof(tbl->col.rng));
  tbl->title = str_sqz(tbl->title_buf, cntof(tbl->title_buf), id);
  tbl->qry_name = str_len(id) > str_len(tbl->title);
  tbl->state = TBL_VIEW_DISPLAY;
  tbl->disp = DB_TBL_VIEW_DSP_DATA;
  tbl->fltr.unused = ~0u;
  tbl->rowid = rowid;
  tbl->kind = kind;

  /* retrive number of columns in table */
  sqlite3_stmt *stmt = 0;
  struct str sql = strv("SELECT COUNT(*) FROM pragma_table_info(?);");
  int rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
  sqlite3_bind_text(stmt, 1, db_str(id), SQLITE_STATIC);
  assert(rc == SQLITE_OK);
  rc = sqlite3_step(stmt);
  assert(rc == SQLITE_ROW);
  tbl->col.rng.total = sqlite3_column_int(stmt, 0);
  tbl->col.cnt = min(tbl->col.rng.total, cntof(tbl->col.lst));
  tbl->row.cols.cnt = min(tbl->col.rng.total, DB_MAX_TBL_ROW_COLS);
  sqlite3_finalize(stmt);
  db_tbl_view__qry_cols(db, tbl, 0, tbl->col.cnt, 0);
  stmt = 0;

  /* setup table column display table */
  struct gui_split_lay bld = {0};
  gui.splt.lay.begin(&bld, tbl->row.ui.state, tbl->col.cnt, ctx->cfg.sep);
  for loop(i, tbl->col.cnt) {
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
  sql = str_fmtsn(db->sql_qry_buf, cntof(db->sql_qry_buf),
    "SELECT COUNT(*) FROM \"%.*s\";", strf(id));
  rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
  assert(rc == SQLITE_OK);
  rc = sqlite3_step(stmt);
  assert(rc == SQLITE_ROW);
  tbl->row.rng.total = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
}
static void
db_tbl_view_del(struct db_view *db, int idx) {
  assert(db);
  assert(idx >= 0);
  assert(idx < cntof(db->tbls));
  assert(!(db->unused & (1u << idx)));

  struct db_tbl_view *tbl = &db->tbls[idx];
  db->unused |= (1u << idx);
  db_tbl_view_fltr_view_clr(&tbl->fltr);
  mset(tbl, 0, szof(*tbl));
}
static int
db_tbl_view_add(struct db_view *db, int idx) {
  assert(db);
  assert(idx >= 0);
  assert(idx < cntof(db->tbls));
  assert(db->tab_cnt < cntof(db->tabs));
  assert(!(db->unused & (1u << idx)));

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
  assert(!(db->unused & (1u << db->tabs[idx])));

  arr_rm(db->tabs, idx, db->tab_cnt);
  db->tab_cnt--;
}
static int
db_tbl_view_fltrs_enabled(struct db_tbl_fltr_view *fltr) {
  assert(fltr);
  for loop(i, fltr->cnt) {
    struct db_tbl_fltr_elm *elm = &fltr->elms[fltr->lst[i]];
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
db_info_qry_cnt(struct db_view *db, enum db_tbl_type tab, struct str fltr) {
  assert(db);
  static const char *type[] = {
#define DB_INFO(a,b,c,d) b,
    DB_TBL_MAP(DB_INFO)
#undef DB_INFO
  };
  int rc = 0;
  sqlite3_stmt *stmt = 0;
  if (str_len(fltr)) {
    struct str sql = strv("SELECT COUNT(*) FROM sqlite_master WHERE type = ? AND name LIKE '%'||?||'%';");
    rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
    assert(rc == SQLITE_OK);
    sqlite3_bind_text(stmt, 1, type[tab], -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, db_str(fltr), SQLITE_STATIC);
  } else {
    struct str sql = strv("SELECT COUNT(*) FROM sqlite_master WHERE type = ?;");
    rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, type[tab], -1, SQLITE_STATIC);
  }
  assert(rc == SQLITE_OK);
  rc = sqlite3_step(stmt);
  assert(rc == SQLITE_ROW);
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

  struct db_info_elm *elm = db_info_elm_new(info);
  elm->name = str_buf_sqz(&info->buf, name, DB_MAX_TBL_NAME);
  elm->sql = str_buf_sqz(&info->buf, sql, DB_MAX_TBL_SQL);
  elm->rowid = rowid;

  struct str elm_sql = str_buf_get(&info->buf, elm->sql);
  for str_loop(i, elm_sql) {
    char *at = (char*)str_ptr(elm_sql, i);
    if (*at == '\n' || *at == '\r') {
      *at = ' ';
    }
  }
}
static void
db_info_qry_elm(struct db_view *db, struct db_info_view *info, int lo, int hi) {
  assert(db);
  assert(info);
  assert(lo >= 0);
  assert(lo <= hi);
  assert(hi <= info->tab_cnt[info->sel_tab]);

  static const char *type[] = {
#define DB_INFO(a,b,c,d) b,
    DB_TBL_MAP(DB_INFO)
#undef DB_INFO
  };
  sqlite3_stmt *stmt = 0;
  if (str_len(info->fnd_str)) {
    struct str sql = strv("SELECT rowid, name, sql FROM sqlite_master WHERE type = '%'||?||'%' AND name LIKE '%'||?||'%' LIMIT ?,?;");
    int rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, type[info->sel_tab], -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, db_str(info->fnd_str), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, lo);
    sqlite3_bind_int(stmt, 4, hi-lo);
    assert(rc == SQLITE_OK);
  } else {
    struct str sql = strv("SELECT rowid, name, sql FROM sqlite_master WHERE type = ? LIMIT ?,?;");
    int rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
    sqlite3_bind_text(stmt, 1, type[info->sel_tab], -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, lo);
    sqlite3_bind_int(stmt, 3, hi-lo);
    assert(rc == SQLITE_OK);
  }
  str_buf_clr(&info->buf);
  info->elm_rng = rng(lo, hi, info->tab_cnt[info->sel_tab]);
  info->elm_cnt = 0;

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
  assert(info);
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
  } else if (sel->sel_cnt + info->sel.cnt < cntof(info->sel.keys)) {
    /* multi-selection */
    sqlite3_stmt *stmt = 0;
    struct str sql = strv("SELECT rowid FROM sqlite_master LIMIT ?,?;");
    int rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
    sqlite3_bind_int(stmt, 1, sel->begin_idx);
    sqlite3_bind_int(stmt, 2, sel->end_idx - sel->begin_idx);
    assert(rc == SQLITE_OK);
    while (sqlite3_step(stmt) == SQLITE_ROW &&
        info->sel.cnt < cntof(info->sel.keys)) {
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
db_setup(struct gui_ctx *ctx, struct arena *mem, struct str path) {
  assert(ctx);
  assert(mem);

  struct arena_scope scp = {0};
  arena_scope_push(&scp, mem);
  struct db_view *db = arena_obj(mem, ctx->sys, struct db_view);
  db->path = str_set(db->path_buf, cntof(db->path_buf), path);
  db->unused = ~0u;

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
    db->info.tab_cnt[i] = db_info_qry_cnt(db, i, str_nil);
    db->info.tab_act |= castu(!!db->info.tab_cnt[i]) << i;
  }
  db->info.sel_tab = cpu_bit_ffs32(db->info.tab_act);
  return db;
}
static void
db_free(struct db_view *db) {
  assert(db);
  /* cleanup tabs */
  unsigned used = ~db->unused;
  while (used) {
    int idx = cpu_bit_ffs32(used);
    db_tbl_view_del(db, idx);
    used = ~db->unused;
  }
  db->tab_cnt = 0;
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
            struct str tbl_name, long long rowid, struct gui_ctx *ctx) {
  assert(db);
  assert(ctx);
  assert(db->unused > 0);
  assert(view >= 0);
  assert(view < cntof(db->tbls));
  assert(db->tab_cnt < cntof(db->tabs));
  assert(!(db->unused & (1llu << view)));
  assert(type == DB_TBL_TYPE_TBL || type == DB_TBL_TYPE_VIEW);

  db_tbl_view_setup(db, view, ctx, tbl_name, rowid, type);
  db->sel_tab = castb(db_tbl_view_add(db, view));
  return db->sel_tab;
}
static int
db_tab_open_new(struct db_view *db, enum db_tbl_type type,
                struct str tbl_name, long long rowid, struct gui_ctx *ctx) {
  assert(db);
  assert(ctx);
  assert(db->unused > 0);
  assert(db->tab_cnt < cntof(db->tabs));
  assert(type == DB_TBL_TYPE_TBL || type == DB_TBL_TYPE_VIEW);

  int view = db_tbl_view_new(db);
  return db_tab_open(db, view, type, tbl_name, rowid, ctx);
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

  sqlite3_stmt *stmt = 0;
  struct str sql = strv("SELECT name FROM sqlite_master WHERE rowid = ?;");
  int rc = sqlite3_prepare_v2(db->con, db_str(sql), &stmt, 0);
  sqlite3_bind_int64(stmt, 1, tbl_id);
  assert(rc == SQLITE_OK);
  if (sqlite3_step(stmt) != SQLITE_ROW) {
    return;
  }
  const char *tbl_name = (const char*)sqlite3_column_text(stmt, 0);
  int tbl_name_len = sqlite3_column_bytes(stmt, 0);
  struct str tbl_name_str = strn(tbl_name, tbl_name_len);
  if (db->tbls[view].state != TBL_VIEW_DISPLAY) {
    db_tbl_view_setup(db, view, ctx, tbl_name_str, tbl_id, db->info.sel_tab);
  } else {
    db_tab_open_new(db, db->info.sel_tab, tbl_name_str, tbl_id, ctx);
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
db_tab_close(struct db_view *db, int tab_idx) {
  assert(db);
  assert(tab_idx >= 0);
  assert(db->tab_cnt > 0);
  assert(tab_idx < cntof(db->tabs));
  assert(tab_idx < db->tab_cnt);
  assert(db->tab_cnt < cntof(db->tabs));
  assert(!(db->unused & (1llu << db->tabs[tab_idx])));

  db_tbl_view_del(db, db->tabs[tab_idx]);
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
static struct str
ui_edit_fnd(struct gui_ctx *ctx, struct gui_edit_box *edt,
            struct gui_panel *pan, struct gui_panel *parent,
            struct gui_txt_ed *ed, char *buf, int cap, struct str s) {
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
    ed->str = s;

    edt->pan.focusable = 1;
    edt->pan.box.x = gui.bnd.min_max(ico.box.x.max, pan->box.x.max);
    edt->pan.box.x = gui.bnd.shrink(&edt->pan.box.x, pad[0]);
    edt->pan.box.y = gui.bnd.shrink(&pan->box.y, pad[1]);
    s = gui.edt.fld(ctx, edt, &edt->pan, pan, ed);
  }
  gui.pan.end(ctx, pan, parent);
  return s;
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
ui_tbl_lst_elm_col_tog(struct gui_ctx *ctx, struct gui_tbl *tbl, const int *tbl_lay,
                       struct gui_panel *elm, int *is_act) {
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
    if (ui_tbl_hdr_elm_tog(ctx, tbl, tbl_cols, fltr->tbl.state, all_on)) {
      for loop(i, fltr->cnt) {
        fltr->elms[fltr->lst[i]].enabled = !all_on;
      }
    }
    for (int i = 1; i + 1 < cntof(db_tbl_fltr_def); ++i) {
      const struct db_tbl_col_def *col = &db_tbl_fltr_def[i];
      gui.tbl.hdr.slot.txt(ctx, tbl, tbl_cols, fltr->tbl.state, col->title);
    }
    /* delete all filters icon */
    struct gui_btn slot = {0};
    ui_tbl_hdr_elm_ico(ctx, tbl, tbl_cols, &slot, fltr->tbl.state, RES_ICO_TRASH);
    if (slot.clk) {
      db_tbl_view_fltr_view_clr(fltr);
    }
  }
  gui.tbl.hdr.end(ctx, tbl);
}
static void
ui_db_tbl_fltr_lst_view(struct db_view *db, struct db_tbl_view *view,
                        struct db_tbl_fltr_view *fltr, struct gui_ctx *ctx,
                        struct gui_panel *pan, struct gui_panel *parent) {
  assert(db);
  assert(ctx);
  assert(pan);
  assert(fltr);
  assert(view);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, fltr->tbl.off, 0);
    {
      /* header */
      int tbl_cols[GUI_TBL_COL(DB_TBL_FLTR_MAX)];
      ui_db_tbl_fltr_lst_tbl_hdr(fltr, &tbl, tbl_cols, ctx);

      /* list */
      int del_idx = -1;
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, fltr->cnt);
      cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_SINGLE;
      cfg.sel.on = GUI_LST_SEL_ON_HOV;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for gui_tbl_lst_loop(i,gui,&tbl) {
        int idx = fltr->lst[i];
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
        db_tbl_view_fltr_view_rm(fltr, del_idx);
        db_tbl_view_rev(db, view);
      }
    }
    gui.tbl.end(ctx, &tbl, pan, fltr->tbl.off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_tbl_fltr_view(struct db_view *db, struct db_tbl_view *view,
                    struct db_tbl_fltr_view *fltr, struct gui_ctx *ctx,
                    struct gui_panel *pan, struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  struct gui_box lay = pan->box;
  gui.pan.begin(ctx, pan, parent);
  {
    /* filter edit field */
    struct gui_edit_box edt = {.flags = GUI_EDIT_SEL_ON_ACT};
    edt.box = gui.cut.top(&lay, ctx->cfg.item, ctx->cfg.gap[1]);
    fltr->fnd_str = gui.edt.box(ctx, &edt, pan, fltr->fnd_buf, cntof(fltr->fnd_buf), fltr->fnd_str);
    if (edt.mod){
      view->fltr.data_rng = rng_nil;
      fltr->off[1] = 0;
    }
    struct gui_btn back = {0};
    back.box = gui.cut.bot(&lay, ctx->cfg.item, ctx->cfg.gap[1]);
    gui.btn.txt(ctx, &back, pan, strv("Back"), 0);
    if (back.clk) {
      db_tbl_view_fltr_close(view);
    }
    confine gui_disable_on_scope(&gui, ctx, !str_len(fltr->fnd_str)) {
      struct gui_btn add = {0};
      add.box = gui.cut.bot(&lay, ctx->cfg.item, ctx->cfg.gap[1]);
      gui.btn.txt(ctx, &add, pan, strv("Add"), 0);
      if (add.clk) {
        db_tbl_view_fltr_view_add_str(db, view, fltr, fltr->ini_col, fltr->fnd_str);
        db_tbl_view_fltr_close(view);
        db_tbl_view_rev(db, view);
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
    if (reg.lst.begin != fltr->data_rng.lo || fltr->init ||
        reg.lst.end != fltr->data_rng.hi || edt.mod) {
      db_tbl_view_fltr_view_qry(db, view, fltr, reg.lst.begin, reg.lst.end);
      fltr->init = 0;
    }
    for gui_lst_reg_loop(i,gui,&reg) {
      struct gui_panel elm = {0};
      unsigned long long id = hash_lld(fltr->rowid[i-reg.lst.begin]);
      gui.lst.reg.elm.begin(ctx, &reg, &elm, id, 0);
      {
        struct gui_panel lbl = {.box = elm.box};
        unsigned hdl = fltr->data[i - reg.lst.begin];
        struct str dat = str_buf_get(&fltr->buf, hdl);
        gui.txt.lbl(ctx, &lbl, &elm, dat, 0);
      }
      gui.lst.reg.elm.end(ctx, &reg, &elm);
    }
    gui.lst.reg.end(ctx, &reg, pan, fltr->off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_tbl_view_dsp_fltr(struct db_view *db, struct db_tbl_view *view,
                        struct db_tbl_fltr_view *fltr, struct gui_ctx *ctx,
                        struct gui_panel *pan, struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  switch (fltr->state) {
  case DB_TBL_FLTR_LST: {
    ui_db_tbl_fltr_lst_view(db, view, fltr, ctx, pan, parent);
  } break;
  case DB_TBL_FLTR_EDT: {
    ui_db_tbl_fltr_view(db, view, fltr, ctx, pan, parent);
  } break;
  }
}

/* ---------------------------------------------------------------------------
 *                                  Table
 * ---------------------------------------------------------------------------
 */
static enum res_ico_id
ui_db_tbl_lst_elm_ico(enum db_tbl_type type) {
  switch (type) {
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
ui_db_tbl_view_hdr_lnk_slot(struct db_tbl_view *view, struct db_tbl_col *col,
                            struct gui_ctx *ctx, struct gui_btn *slot) {
  assert(col);
  assert(ctx);
  assert(view);

  /* table column header filter icon button */
  struct gui_btn fltr = {.box = slot->pan.box};
  fltr.box.x = gui.bnd.max_ext(slot->pan.box.x.max, ctx->cfg.item);
  int dis = col->blob || (view->fltr.cnt >= cntof(view->fltr.lst));
  confine gui_disable_on_scope(&gui, ctx, dis) {
    if (gui.btn.ico(ctx, &fltr, &slot->pan, RES_ICO_SEARCH)) {
      db_tbl_view_fltr_open(view, col->rowid);
    }
  }
  /* header label with foreign key icon */
  struct gui_cfg_stk stk[1] = {0};
  unsigned fk_col = ctx->cfg.col[GUI_COL_TXT_DISABLED];
  confine gui_cfg_pushu_scope(&gui, stk, &ctx->cfg.col[GUI_COL_ICO], fk_col) {
    struct str col_name = str_buf_get(&view->col.buf, col->name);
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
  struct gui_panel pan = {.box = slot->box};
  gui_txt(ctx, &pan, &slot->pan, txt, &align);
}
static void
ui_db_tbl_view_hdr_slot(struct db_tbl_view *view, struct db_tbl_col *col,
                        struct gui_ctx *ctx, struct gui_btn *slot) {
  assert(col);
  assert(ctx);
  assert(view);

  /* table column header filter icon button */
  struct gui_btn fltr = {.box = slot->pan.box};
  fltr.box.x = gui.bnd.max_ext(slot->pan.box.x.max, ctx->cfg.item);
  int dis = col->blob || (view->fltr.cnt >= cntof(view->fltr.lst));
  confine gui_disable_on_scope(&gui, ctx, dis) {
    if (gui.btn.ico(ctx, &fltr, &slot->pan, RES_ICO_SEARCH)) {
      db_tbl_view_fltr_open(view, col->rowid);
    }
  }
  static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
  struct str col_name = str_buf_get(&view->col.buf, col->name);
  struct gui_btn hdr = {.box = slot->pan.box};
  hdr.box.x = gui.bnd.min_max(slot->pan.box.x.min, fltr.box.x.min);
  gui.btn.txt(ctx, &hdr, &slot->pan, col_name, &align);
}
static void
ui_db_tbl_view_dsp_data(struct db_view *db, struct db_tbl_view *view,
                        struct gui_ctx *ctx, struct gui_panel *pan,
                        struct gui_panel *parent) {
  assert(db);
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, view->row.ui.off, 0);
    {
      /* header */
      int tbl_lay[GUI_TBL_COL(DB_MAX_TBL_COLS)];
      gui.tbl.hdr.begin(ctx, &tbl, tbl_lay, view->row.ui.state);
      for loopr(i, view->row.cols) {
        struct db_tbl_col* col = &view->col.lst[i - view->col.rng.lo];
        struct gui_btn slot = {0};
        gui.tbl.hdr.slot.begin(ctx, &tbl, &slot);
        if (i == view->row.cols.lo) {
          struct gui_btn prv = {.box = slot.pan.box};
          prv.box.x = gui.bnd.min_ext(slot.pan.box.x.min, ctx->cfg.scrl);
          /* move column window to the left */
          int dis = view->col.rng.lo == 0 || view->col.rng.cnt == view->col.rng.total;
          confine gui_disable_on_scope(&gui, ctx, dis) {
            if (gui__scrl_btn(ctx, &prv, &slot.pan, GUI_WEST)) {
              db_tbl_view__qry_row_cols(db, view, view->row.cols.lo-1);
            }
          }
          slot.pan.box.x = gui.bnd.min_max(prv.box.x.max, slot.pan.box.x.max);
        }
        if (i + 1 == view->row.cols.hi) {
          struct gui_btn nxt = {.box = slot.pan.box};
          nxt.box.x = gui.bnd.max_ext(slot.pan.box.x.max, ctx->cfg.scrl);
          /* move column window to the right */
          int fits = view->col.rng.cnt == view->col.rng.total;
          int end = (i == (view->col.rng.total - view->row.cols.cnt));
          confine gui_disable_on_scope(&gui, ctx, fits || end) {
            if (gui__scrl_btn(ctx, &nxt, &slot.pan, GUI_EAST)) {
              db_tbl_view__qry_row_cols(db, view, view->row.cols.lo+1);
            }
          }
          slot.pan.box.x = gui.bnd.min_max(slot.pan.box.x.min, nxt.box.x.min);
        }
        if (col->pk) {
          ui_db_tbl_view_hdr_key_slot(view, col, ctx, &slot);
        } else if (col->fk) {
          ui_db_tbl_view_hdr_lnk_slot(view, col, ctx, &slot);
        } else if (col->blob) {
          struct str col_name = str_buf_get(&view->col.buf, col->name);
          ui_db_tbl_view_hdr_blob_slot(ctx, col_name, &slot);
        } else {
          ui_db_tbl_view_hdr_slot(view, col, ctx, &slot);
        }
        gui.tbl.hdr.slot.end(ctx, &tbl, tbl_lay, &slot, view->row.ui.state);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* list */
      int idx = 0;
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, view->row.rng.total);
      cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_SINGLE;
      cfg.sel.on = GUI_LST_SEL_ON_HOV;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      if (tbl.lst.begin != view->row.rng.lo ||
          tbl.lst.end != view->row.rng.hi) {
        db_tbl_view__qry_rows(db, view, tbl.lst.begin, tbl.lst.end);
      }
      for gui_tbl_lst_loop(i,gui,&tbl) {
        struct gui_panel item = {0};
        unsigned long long id = hash_lld(view->row.rowids[i-tbl.lst.begin]);
        gui.tbl.lst.elm.begin(ctx, &tbl, &item, id, 0);
        for loop(j, view->row.cols.cnt) {
          unsigned elm = view->row.lst[idx++];
          struct str dat = str_buf_get(&view->row.buf, elm);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_lay, &item, dat, 0);
        }
        gui.tbl.lst.end(ctx, &tbl);
      }
    }
    gui.tbl.end(ctx, &tbl, pan, view->row.ui.off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_tbl_view_dsp_layout(struct db_view *db, struct db_tbl_view *view,
                          struct gui_ctx *ctx, struct gui_panel *pan,
                          struct gui_panel *parent) {
  unused(db);
  assert(db);
  assert(view);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, view->col.ui.off, 0);
    {
      /* header */
      int tbl_cols[GUI_TBL_COL(DB_TBL_DISP_COL_MAX)];
      gui.tbl.hdr.begin(ctx, &tbl, tbl_cols, view->col.ui.state);
      if (ui_tbl_hdr_elm_lock(ctx, &tbl, tbl_cols, view->col.ui.state, !!view->col.state)) {
        switch (view->col.state) {
        default: assert(0); break;
        case DB_TBL_COL_STATE_LOCKED: {
          view->col.state = DB_TBL_COL_STATE_UNLOCKED;
          tbl_clr(&view->col.sel);
        } break;
        case DB_TBL_COL_STATE_UNLOCKED: {
          view->col.state = DB_TBL_COL_STATE_LOCKED;
        } break;}
      }
      for (int i = 1; i < cntof(db_tbl_disp_col_def); ++i) {
        const struct db_tbl_col_def *itr = db_tbl_disp_col_def + i;
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_cols, view->col.ui.state, itr->title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, view->col.rng.total);
      cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_SINGLE;
      cfg.sel.on = GUI_LST_SEL_ON_HOV;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      if (tbl.lst.begin != view->col.rng.lo ||
          tbl.lst.end != view->col.rng.hi) {
        db_tbl_view__qry_cols(db, view, tbl.lst.begin, tbl.lst.end, 0);
      }
      for gui_tbl_lst_loop(i,gui,&tbl) {
        struct gui_panel item = {0};
        struct db_tbl_col *col = &view->col.lst[i];
        struct str col_name = str_buf_get(&view->col.buf, col->name);
        struct str col_type = str_buf_get(&view->col.buf, col->type);

        unsigned long long k = hash_lld(col->rowid);
        gui.tbl.lst.elm.begin(ctx, &tbl, &item, k, 0);
        {
          int dis = view->col.state == DB_TBL_COL_STATE_LOCKED;
          int is_act = (dis || tbl_has(&view->col.sel, k));
          confine gui_disable_on_scope(&gui, ctx, dis) {
            if (ui_tbl_lst_elm_col_tog(ctx, &tbl, tbl_cols, &item, &is_act)) {
              if (is_act) {
                tbl_put(&view->col.sel, k, &col->rowid);
              } else{
                tbl_del(&view->col.sel, k);
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
              db_tbl_view_fltr_open(view, col->rowid);
            }
          } else {
            gui_tbl_lst_elm_col(&item.box, ctx, &tbl, tbl_cols);
          }
        }
        gui.tbl.lst.elm.end(ctx, &tbl, &item);
      }
      gui.tbl.lst.end(ctx, &tbl);
    }
    gui.tbl.end(ctx, &tbl, pan, view->col.ui.off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_tbl_view_dsp(struct db_view *db, struct db_tbl_view *tbl,
                   struct gui_ctx *ctx, struct gui_panel *pan,
                   struct gui_panel *parent) {
  assert(db);
  assert(tbl);
  assert(ctx);
  assert(pan);
  assert(parent);

  static const struct {
    struct str name;
    unsigned long long hash;
    enum res_ico_id ico;
  } tabs[DB_TBL_VIEW_DSP_CNT] = {
    [DB_TBL_VIEW_DSP_DATA]    = {.name = strv("Data"),    .hash = STR_HASH4("Data"),    .ico = RES_ICO_TH_LIST},
    [DB_TBL_VIEW_DSP_FILTER]  = {.name = strv("Filters"), .hash = STR_HASH4("Filters"), .ico = RES_ICO_MODIFY},
    [DB_TBL_VIEW_DSP_LAYOUT]  = {.name = strv("Layout"),  .hash = STR_HASH8("Layout"),  .ico = RES_ICO_COG},
  };
  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box, .hdr_pos = GUI_TAB_HDR_BOT};
    gui.tab.begin(ctx, &tab, pan, DB_TBL_VIEW_DSP_CNT, tbl->disp);
    {
      /* tab header */
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      {
        for arr_loopv(i, tabs) {
          struct gui_panel slot = {0};
          gui.tab.hdr.slot.begin(ctx, &tab, &hdr, &slot, tabs[i].hash);
          gui.ico.box(ctx, &slot, &hdr.pan, tabs[i].ico, tabs[i].name);
          gui.tab.hdr.slot.end(ctx, &tab, &hdr, &slot, 0);
        }
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);
      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      switch(tbl->disp) {
      case DB_TBL_VIEW_DSP_DATA: {
        ui_db_tbl_view_dsp_data(db, tbl, ctx, &bdy, &tab.pan);
      } break;
      case DB_TBL_VIEW_DSP_FILTER: {
        ui_db_tbl_view_dsp_fltr(db, tbl, &tbl->fltr, ctx, &bdy, &tab.pan);
      } break;
      case DB_TBL_VIEW_DSP_LAYOUT: {
        ui_db_tbl_view_dsp_layout(db, tbl, ctx, &bdy, &tab.pan);
      } break;}
    }
    gui.tab.end(ctx, &tab, pan);
    if (tab.sel.mod) {
      if (tbl->disp == DB_TBL_VIEW_DSP_LAYOUT) {
        if (tbl->col.state == DB_TBL_COL_STATE_UNLOCKED) {
          if (tbl->col.sel.cnt == 0) {
            tbl->col.state = DB_TBL_COL_STATE_LOCKED;
          }
        }
        if (tbl->col.state == DB_TBL_COL_STATE_UNLOCKED) {
          tbl->col.total = tbl->col.rng.total;
          tbl->col.rng.lo = tbl->col.rng.hi = tbl->col.rng.cnt = 0;
          tbl->col.rng.total = tbl->col.sel.cnt;
          db_tbl_view__qry_cols(db, tbl, 0, 0, 1);
        }
      }
      if (tab.sel.idx == DB_TBL_VIEW_DSP_LAYOUT) {
        if (tbl->col.state == DB_TBL_COL_STATE_UNLOCKED) {
          tbl->col.rng.lo = tbl->col.rng.hi = tbl->col.rng.cnt = 0;
          tbl->col.rng.total = tbl->col.total;
        }
      }
      tbl->disp = tab.sel.idx;
    }
  }
  gui.pan.end(ctx, pan, parent);
}

/* ---------------------------------------------------------------------------
 *                                  Info
 * ---------------------------------------------------------------------------
 */
static void
ui_db_view_info_tbl(struct db_view *db, struct db_info_view *info,
                    struct gui_ctx *ctx, struct gui_panel *pan,
                    struct gui_panel *parent) {
  assert(db);
  assert(ctx);
  assert(pan);
  assert(info);
  assert(parent);
  const struct {struct str type; enum res_ico_id ico;} types[] = {
    #define DB_INFO(a,b,c,d) {.type = strv(b), .ico = d},
      DB_TBL_MAP(DB_INFO)
    #undef DB_INFO
  };
  gui.pan.begin(ctx, pan, parent);
  {
    int gap = ctx->cfg.gap[1];
    struct gui_box lay = pan->box;
    struct gui_panel fltr = {.box = gui.cut.top(&lay, ctx->cfg.item, gap)};
    struct gui_edit_box edt = {.box = fltr.box};
    info->fnd_str = ui_edit_fnd(ctx, &edt, &fltr, pan, &info->fnd_ed,
        info->fnd_buf, cntof(info->fnd_buf), info->fnd_str);
    if (edt.mod) {
      info->tab_cnt[info->sel_tab] = db_info_qry_cnt(db, info->sel_tab, info->fnd_str);
    }
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
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_MULTI;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      if (tbl.lst.begin != info->elm_rng.lo ||
          tbl.lst.end != info->elm_rng.hi || edt.mod) {
        db_info_qry_elm(db, &db->info, tbl.lst.begin, tbl.lst.end);
      }
      for gui_tbl_lst_loop(i,gui,&tbl) {
        struct gui_panel item = {0};
        struct db_info_elm *elm = &info->elms[i-tbl.lst.begin];
        struct str elm_name = str_buf_get(&info->buf, elm->name);
        struct str elm_sql = str_buf_get(&info->buf, elm->sql);

        int is_sel = tbl_has(&info->sel, hash_lld(elm->rowid));
        gui.tbl.lst.elm.begin(ctx, &tbl, &item, hash_lld(elm->rowid), is_sel);
        {
          gui.tbl.lst.elm.col.txt_ico(ctx, &tbl, tbl_cols, &item, elm_name, types[info->sel_tab].ico);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &item, types[info->sel_tab].type, 0);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &item, elm_sql, 0);
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
  static const struct {struct str title;enum res_ico_id ico;} tabs[] = {
    #define DB_INFO(a,b,c,d) {.title = strv(c), .ico = d},
      DB_TBL_MAP(DB_INFO)
    #undef DB_INFO
  };
  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box, .hdr_pos = GUI_TAB_HDR_BOT};
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
          gui.tab.hdr.slot.begin(ctx, &tab, &hdr, &slot, str_hash(tabs[i].title));
          gui.ico.box(ctx, &slot, &hdr.pan, tabs[i].ico, tabs[i].title);
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
      if (str_len(db->info.fnd_str)) {
        db->info.fnd_str = str_nil;
        info->tab_cnt[info->sel_tab] = db_info_qry_cnt(db, db->info.sel_tab, str_nil);
      }
      db->info.sel_tab = castb(tab.sel.idx);
      tbl_clr(&db->info.sel);
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
      /* table view */
      struct gui_panel lst = {.box = pan->box};
      ui_db_tbl_view_dsp(db, tbl, ctx, &lst, pan);
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
ui_db_explr_tab(struct db_view *db, struct db_tbl_view *tbl,
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
    title = tbl->title;
  }
  return ui_db_explr_tab_slot(db, tbl, ctx, tab, hdr, slot, title, ico);
}
static int
ui_db_tab_view_lst(struct db_view *db, struct gui_ctx *ctx,
                   struct gui_panel *pan, struct gui_panel *parent) {

  assert(db);
  assert(ctx);
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
        title = tbl->title;
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
        db_tab_close(db, tab.sel.idx);
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
        assert(db->sel_tab < db->tab_cnt);
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

