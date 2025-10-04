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
  [DB_TBL_FLTR_BUF]       = {.title = strv("Filter"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 1200}}},
  [DB_TBL_FLTR_COL]       = {.title = strv("Column"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 1200}}},
  [DB_TBL_FLTR_DEL]       = {.title = strv(""),       .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {10, 200}}},
};
static const struct db_tbl_col_def db_tbl_fltr_col_def[DB_TBL_FLTR_COL_MAX] = {
  [DB_TBL_FLTR_COL_NAME]  = {.title = strv("Name"),  .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {100, 1200}}},
  [DB_TBL_FLTR_COL_TYPE]  = {.title = strv("Type"),  .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {100, 1200}}},
};
static const struct db_tbl_col_def db_tree_col_def[DB_TREE_COL_MAX] = {
  [DB_TREE_COL_NAME]      = {.title = strv("Name"),   .ui = {.type = GUI_LAY_SLOT_FIX, .size = 400, .con = {50, 1000}}},
  [DB_TREE_COL_TYPE]      = {.title = strv("Type"),   .ui = {.type = GUI_LAY_SLOT_FIX, .size = 200, .con = {50, 1000}}},
  [DB_TREE_COL_SQL]       = {.title = strv("Schema"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {50, 1200}}},
};
static const struct db_tbl_col_def db_tbl_disp_col_def[DB_TBL_DISP_COL_MAX] = {
  [DB_TBL_DISP_COL_ACT]   = {.title = strv(""),       .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
  [DB_TBL_DISP_COL_NAME]  = {.title = strv("Name"),   .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 1200}}},
  [DB_TBL_DISP_COL_TYPE]  = {.title = strv("Type"),   .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 1200}}},
  [DB_TBL_DISP_COL_PK]    = {.title = strv("PK"),     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
  [DB_TBL_DISP_COL_FK]    = {.title = strv("FK"),     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
  [DB_TBL_DISP_COL_NN]    = {.title = strv("!0"),     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
  [DB_TBL_DISP_COL_FLTR]  = {.title = strv(""),       .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {60, 60}}},
};
static const struct db_tbl_col_def db_tbl_def[DB_STATE_TBL_COL_CNT] = {
  [DB_STATE_TBL_COL_NAME]  = {.title = strv("Name"),    .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {50, 1200}}},
  [DB_STATE_TBL_COL_COLS]  = {.title = strv("Columns"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {200, 1200}}},
  [DB_STATE_TBL_COL_ROWS]  = {.title = strv("Rows"),    .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {200, 1200}}},
  [DB_STATE_TBL_COL_FLTR]  = {.title = strv("Filters"), .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {200, 1200}}},
  [DB_STATE_TBL_COL_DEL]   = {.title = strv(""),        .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60, .con = {60, 60}}},
};
// clang-format on

/* ---------------------------------------------------------------------------
 *                                Helper
 * ---------------------------------------------------------------------------
 */
struct db_name_lck {
  int err;
  struct str name;
  sqlite3_stmt *stmt;
};
priv int
db__tbl_qry_name(sqlite3_stmt **not_null stmt,
                 struct str *not_null name_res,
                 struct db_state *not_null sdb,
                 long long rowid) {

  requires(name_res);
  requires(stmt);
  requires(sdb);
  *stmt = 0;

  struct str sql = strv("SELECT name FROM sqlite_master WHERE rowid = ?;");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    *stmt = 0;
    return err;
  }
  err = sqlite3_bind_int64(*stmt, 1, rowid);
  if (err != SQLITE_OK) {
    *stmt = 0;
    return err;
  }
  err = sqlite3_step(*stmt);
  if (err != SQLITE_ROW) {
    *stmt = 0;
    return err;
  }
  const char *tbl_name = (const char*)sqlite3_column_text(*stmt, 0);
  int tbl_name_len = sqlite3_column_bytes(*stmt, 0);
  *name_res = strn(tbl_name, tbl_name_len);
  return SQLITE_OK;
}
priv int
db_tbl_name_acq(struct db_name_lck *not_null lck, struct db_state *not_null sdb,
                struct db_tbl_state *not_null tbl) {

  requires(sdb);
  requires(lck);
  requires(tbl);

  lck->stmt = 0;
  lck->err = SQLITE_OK;
  if (tbl->qry_name) {
    lck->err = db__tbl_qry_name(&lck->stmt, &lck->name, sdb, tbl->rowid);
  } else {
    lck->name = tbl->title;
  }
  return lck->err;
}
priv void
db_tbl_name_rel(struct db_name_lck *not_null lck) {
  requires(lck);
  sqlite3_finalize(lck->stmt);
}
priv int
db__tbl_qry_col_name(sqlite3_stmt **not_null stmt,
                     struct str *not_null name_res,
                     struct db_state *not_null sdb,
                     struct str tbl_name, long long col_id) {

  requires(stmt);
  requires(sdb);
  requires(name_res);

  struct str sql = strv("SELECT name FROM pragma_table_info(?) WHERE rowid = ?;");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    *stmt = 0;
    return err;
  }
  err = sqlite3_bind_text(*stmt, 1, db_str(tbl_name), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    *stmt = 0;
    return err;
  }
  err = sqlite3_bind_int64(*stmt, 2, col_id);
  if (err != SQLITE_OK) {
    *stmt = 0;
    return err;
  }
  err = sqlite3_step(*stmt);
  if (err != SQLITE_ROW) {
    *stmt = 0;
    return err;
  }
  const char *col_name = (const char*)sqlite3_column_text(*stmt, 0);
  int col_name_len = sqlite3_column_bytes(*stmt, 0);
  *name_res = strn(col_name, col_name_len);
  return SQLITE_OK;
}
priv int
db_tbl_col_name_acq(struct db_name_lck *not_null lck,
                    struct db_state *not_null sdb,
                    struct db_tbl_view *not_null tbl,
                    struct db_tbl_col *col,
                    struct str tbl_name, long long col_id) {
  requires(sdb);
  requires(lck);
  requires(tbl);

  lck->err = 0;
  if (!col || col->qry_name) {
    lck->err = db__tbl_qry_col_name(&lck->stmt, &lck->name, sdb, tbl_name, col_id);
  } else {
    lck->name = str_buf_get(&tbl->col.buf, col->name);
  }
  return lck->err;
}
priv void
db_tbl_col_name_rel(struct db_name_lck *not_null lck) {
  assert(lck);
  sqlite3_finalize(lck->stmt);
}

/* ---------------------------------------------------------------------------
 *                                Filter
 * ---------------------------------------------------------------------------
 */
priv inline int
db__tbl_fltr_elm_val(const struct db_tbl_fltr_elm *not_null elm) {
  unused(elm);
  assert(elm);

  assert(elm->type >= DB_TBL_FLTR_ELM_TYP_STR);
  assert(elm->type <= DB_TBL_FLTR_ELM_TYP_TM);
  assert(elm->col >= 0);

  assert(str__is_val(&elm->fnd));
  assert(str__is_val(&elm->col_name));
  return 1;
}
priv inline int
db__tbl_fltr_val(const struct db_tbl_fltr_state *not_null fltr) {
  unused(fltr);
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
priv void
db_tbl_fltr_add_str(struct db_state *not_null sdb,
                    struct db_tbl_state *not_null stbl,
                    struct db_tbl_view *not_null vtbl,
                    struct db_tbl_fltr_state *not_null fltr,
                    int idx, long long col, struct str str) {

  requires(db__tbl_fltr_val(fltr));
  requires(sdb);
  requires(stbl);
  requires(vtbl);

  requires(idx >= 0);
  requires(idx < DB_MAX_FLTR_CNT);
  requires(idx < cntof(fltr->elms));
  requires(!fltr->elms[idx].active);

  struct db_tbl_fltr_elm *elm = &fltr->elms[idx];
  elm->col_name = str_inv;
  elm->col = col;

  struct db_name_lck tlck = {0};
  struct db_name_lck clck = {0};
  if (!db_tbl_name_acq(&tlck, sdb, stbl) &&
      !db_tbl_col_name_acq(&clck, sdb, vtbl, 0, tlck.name, col)) {
    elm->col_name = str_sqz(arrv(elm->col_buf), clck.name);
  }
  db_tbl_col_name_rel(&clck);
  db_tbl_name_rel(&tlck);

  if (str_is_val(elm->col_name)) {
    fltr->cnt++;
    elm->fnd = str_sqz(arrv(elm->fnd_buf), str);
    elm->type = DB_TBL_FLTR_ELM_TYP_STR;
    elm->enabled = 1;
    elm->active = 1;
  }
  ensures(db__tbl_fltr_val(fltr));
}
priv void
db_tbl_fltr_view_clr(struct db_tbl_fltr_state *not_null fltr) {
  requires(db__tbl_fltr_val(fltr));
  mset(fltr->elms, 0, szof(fltr->elms));
  fltr->cnt = 0;
  ensures(db__tbl_fltr_val(fltr));
}
priv int
db__tbl_fltr_str_is_act(struct str fltr) {
  return str_len(fltr) > 2;
}
priv void
db_tbl_open_fltr(struct db_tbl_state *not_null tbl, long long col) {
  requires(tbl);
  mset(&tbl->fltr.data_rng, szof(tbl->fltr.data_rng), 0);
  tbl->disp = DB_TBL_VIEW_DSP_FILTER;
  tbl->fltr.state = DB_TBL_FLTR_EDT;
  tbl->fltr.data_rng.total = tbl->row.rng.total;
  tbl->fltr.ini_col = col;
  tbl->fltr.init = 1;
}
priv void
db_tbl_close_fltr(struct db_tbl_state *not_null tbl) {
  requires(tbl);
  tbl->disp = DB_TBL_VIEW_DSP_DATA;
  tbl->fltr.state = DB_TBL_FLTR_LST;
}
priv int
db__tbl_fltr_view_qry_fltr_cnt_stmt(sqlite3_stmt **not_null stmt,
                                    struct db_state *not_null sdb,
                                    struct db_view *not_null vdb,
                                    struct db_tbl_fltr_view *not_null view,
                                    struct str tbl, struct str col) {
  requires(stmt);
  requires(sdb);
  requires(vdb);
  requires(view);

  struct str sql = str_set_fmtsn(arrv(vdb->sql_qry_buf),
    "SELECT COUNT(*) FROM \"%.*s\" WHERE \"%.*s\" LIKE '%%'||?||'%%';",
    strf(tbl), strf(col));
  if (str_is_inv(sql)) {
    return SQLITE_TOOBIG;
  }
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    *stmt = 0;
    return err;
  }
  err = sqlite3_bind_text(*stmt, 1, db_str(view->fnd_str), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    sqlite3_finalize(*stmt);
    *stmt = 0;
    return err;
  }
  return SQLITE_OK;
}
priv int
db__tbl_fltr_view_qry_cnt_stmt(sqlite3_stmt **not_null stmt,
                               struct db_state *not_null sdb,
                               struct db_view *not_null vdb,
                               struct db_tbl_fltr_view *not_null view,
                               struct str tbl, struct str col) {
  requires(stmt);
  requires(sdb);
  requires(vdb);
  requires(view);

  int err = 0;
  if (db__tbl_fltr_str_is_act(view->fnd_str)) {
    err = db__tbl_fltr_view_qry_fltr_cnt_stmt(stmt, sdb, vdb, view, tbl, col);
  } else {
    struct str sql = str_set_fmtsn(arrv(vdb->sql_qry_buf),
      "SELECT COUNT(*) FROM \"%.*s\";", strf(col));
    if (str_is_inv(sql)) {
      return SQLITE_TOOBIG;
    }
    err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  }
  return err;
}
priv int
db__tbl_fltr_view_qry_fltr_stmt(sqlite3_stmt **not_null stmt,
                                struct db_state *not_null sdb,
                                struct db_view *not_null vdb,
                                struct db_tbl_fltr_view *not_null view,
                                struct str tbl, struct str col,
                                int low, int high) {
  requires(stmt);
  requires(sdb);
  requires(vdb);
  requires(view);

  requires(low >= 0);
  requires(high >= 0);
  requires(low <= high);

  struct str sql = str_set_fmtsn(arrv(vdb->sql_qry_buf),
    "SELECT rowid, \"%.*s\" FROM \"%.*s\" WHERE \"%.*s\" LIKE '%%'||?||'%%' LIMIT ?,?;",
    strf(col), strf(tbl), strf(col));
  if (str_is_inv(sql)) {
    return SQLITE_TOOBIG;
  }
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    *stmt = 0;
    return err;
  }
  err = sqlite3_bind_text(*stmt, 1, db_str(view->fnd_str), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_int(*stmt, 2, low);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_int(*stmt, 3, high-low);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  return err;

failed_bind:
  sqlite3_finalize(*stmt);
  *stmt = 0;
  return err;
}
priv int
db__tbl_fltr_view_qry_no_fltr_stmt(sqlite3_stmt **not_null stmt,
                                   struct db_state *not_null sdb,
                                   struct db_view *not_null vdb,
                                   struct db_tbl_fltr_view *not_null view,
                                   struct str tbl, struct str col,
                                   int low, int high) {
  requires(stmt);
  requires(sdb);
  requires(vdb);
  requires(view);

  requires(low >= 0);
  requires(high >= 0);
  requires(low <= high);

  struct str sql = str_set_fmtsn(arrv(vdb->sql_qry_buf),
    "SELECT rowid, \"%.*s\" FROM \"%.*s\" LIMIT ?,?;", strf(col), strf(tbl));
  if (str_is_inv(sql)) {
    return SQLITE_TOOBIG;
  }
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    *stmt = 0;
    return err;
  }
  err = sqlite3_bind_int(*stmt, 1, low);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_int(*stmt, 2, high-low);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  return err;

failed_bind:
  sqlite3_finalize(*stmt);
  *stmt = 0;
  return err;
}
priv int
db__tbl_fltr_view_qry_stmt(sqlite3_stmt **not_null stmt,
                           struct db_state *not_null sdb,
                           struct db_view *not_null vdb,
                           struct db_tbl_fltr_view *not_null view,
                           struct str tbl, struct str col,
                           int low, int high) {
  requires(stmt);
  requires(sdb);
  requires(vdb);
  requires(view);

  requires(low >= 0);
  requires(high >= 0);
  requires(low <= high);

  int err = 0;
  if (db__tbl_fltr_str_is_act(view->fnd_str)) {
    err = db__tbl_fltr_view_qry_fltr_stmt(stmt, sdb, vdb, view, tbl, col, low, high);
  } else {
    err = db__tbl_fltr_view_qry_no_fltr_stmt(stmt, sdb, vdb, view, tbl, col, low, high);
  }
  return err;
}
priv int
db_tbl_fltr_view_qry(struct db_state *not_null sdb,
                     struct db_view *not_null vdb,
                     struct db_tbl_state *not_null stbl,
                     struct db_tbl_view *not_null vtbl,
                     struct db_tbl_fltr_state *not_null fltr,
                     struct db_tbl_fltr_view *not_null view,
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
  int err = 0;
  sqlite3_stmt *stmt = 0;
  struct db_name_lck tlck = {0};
  struct db_name_lck clck = {0};
  if (!db_tbl_name_acq(&tlck, sdb, stbl) &&
      !db_tbl_col_name_acq(&clck, sdb, vtbl, 0, tlck.name, fltr->ini_col)) {
    /* query total filtered element count */
    err = db__tbl_fltr_view_qry_cnt_stmt(&stmt, sdb, vdb, view, tlck.name, clck.name);
    if (err == SQLITE_OK) {
      err = sqlite3_step(stmt);
      assert(err == SQLITE_ROW);
      fltr->data_rng.total = sqlite3_column_int(stmt, 0);
      sqlite3_finalize(stmt);
    } else {
      goto failed;
    }
    stmt = 0;
    err = db__tbl_fltr_view_qry_stmt(&stmt, sdb, vdb, view, tlck.name, clck.name, low, high);
    if (err == SQLITE_OK) {
      /* add filtered elements */
      for db_loop(_, err, stmt, view->data) {
        assert(fltr->data_rng.cnt < cntof(view->data));
        long long rowid = sqlite3_column_int64(stmt, 0);

        const char *dat = (const char*)sqlite3_column_text(stmt, 1);
        int len = sqlite3_column_bytes(stmt, 1);
        struct str elm = strn(dat,len);

        view->rowid[fltr->data_rng.cnt] = rowid;
        view->data[fltr->data_rng.cnt] = str_buf_sqz(&view->buf, elm, DB_MAX_FLTR_ELM_STR);
        fltr->data_rng.cnt++;
      }
      err = sqlite3_finalize(stmt);
    }
  } else {
    err = tlck.err|clck.err;
  }
  db_tbl_col_name_rel(&clck);
  db_tbl_name_rel(&tlck);

  if (err != SQLITE_OK) {
failed:
    fltr->data_rng.lo = 0;
    fltr->data_rng.hi = 0;
    fltr->data_rng.cnt = 0;
    fltr->data_rng.total = 0;
  }
  ensures(view->id == stbl->rowid);
  ensures((stmt && fltr->data_rng.lo == low) || (!stmt && fltr->data_rng.lo == 0));
  ensures((stmt && fltr->data_rng.hi == high) || (!stmt && fltr->data_rng.hi == 0));
  ensures(view->buf.cnt <= cntof(view->buf.mem));
  ensures(view->buf.cnt >= 0);
  ensures(db__tbl_fltr_val(fltr));
  return 0;
}
/* ---------------------------------------------------------------------------
 *                                Table View
 * ---------------------------------------------------------------------------
 */
priv inline int
db__tbl_state_is_val(const struct db_tbl_state *not_null tbl) {
  unused(tbl);
  assert(tbl);

  assert(tbl->kind >= DB_TBL_TYPE_TBL && tbl->kind <= DB_TBL_TYPE_TRIGGER);
  assert(tbl->state >= TBL_VIEW_SELECT && tbl->state <= TBL_VIEW_DISPLAY);
  assert(tbl->disp >= DB_TBL_VIEW_DSP_DATA && tbl->disp < DB_TBL_VIEW_DSP_CNT);
  assert(tbl->data >= DB_TBL_VIEW_DSP_DATA_LIST && tbl->data < DB_TBL_VIEW_DSP_DATA_CNT);
  assert(tbl->blb.sel_tab >= DB_TBL_BLB_HEX && tbl->blb.sel_tab < DB_TBL_BLB_CNT);
  assert(str_is_val(tbl->title));

  assert(rng__is_val(&tbl->row.rng));
  assert(rng__is_val(&tbl->row.cols));
  assert(rng__is_val(&tbl->row.data_rng));
  assert(rng__is_val(&tbl->col.rng));
  assert(rng__is_val(&tbl->blb.rng));

  assert(tbl->col.rng.lo >= 0);
  assert(tbl->col.rng.hi >= 0);
  assert(tbl->col.rng.hi >= tbl->col.rng.lo);

  assert(tbl->col.rng.cnt >= 0);
  assert(tbl->col.rng.cnt <= DB_MAX_TBL_COLS);
  assert(tbl->col.rng.cnt <= SQLITE_MAX_COLUMN);
  assert(tbl->col.rng.cnt <= tbl->col.rng.total);

  assert(tbl->col.sel.cnt <= DB_MAX_TBL_COLS);
  assert(tbl->col.state == DB_TBL_COL_STATE_LOCKED ||
    tbl->col.state == DB_TBL_COL_STATE_UNLOCKED);

  assert(tbl->row.cols.lo >= 0);
  assert(tbl->row.cols.hi >= 0);
  assert(tbl->row.cols.lo <= tbl->row.cols.hi);

  assert(tbl->row.rng.lo >= 0);
  assert(tbl->row.rng.hi >= 0);
  assert(tbl->row.rng.hi >= tbl->row.rng.lo);
  assert(tbl->row.rng.cnt >= 0);

  assert(tbl->col.state == DB_TBL_COL_STATE_LOCKED ||
    tbl->col.state == DB_TBL_COL_STATE_UNLOCKED);
  assert(tbl->col.cnt >= 0);
  assert(tbl->col.total >= 0);
  assert(tbl->col.cnt <= tbl->col.total);
  assert(tbl->col.cnt <= DB_MAX_TBL_COLS);
  assert(tbl->col.total <= SQLITE_MAX_COLUMN);

  assert(tbl->row.cols.lo >= 0);
  assert(tbl->row.cols.hi >= 0);
  assert(tbl->row.cols.lo <= tbl->row.cols.hi);
  assert(tbl->row.cols.lo >= tbl->col.rng.lo);
  assert(tbl->row.cols.lo <= tbl->col.rng.hi);
  assert(tbl->row.cols.hi >= tbl->col.rng.lo);
  assert(tbl->row.cols.hi <= tbl->col.rng.hi);
  assert(tbl->row.cols.cnt <= DB_MAX_TBL_ROW_COLS);

  assert(tbl->col.rng.lo >= 0);
  assert(tbl->col.rng.hi >= 0);
  assert(tbl->col.rng.hi >= tbl->col.rng.lo);
  assert(tbl->col.rng.cnt >= 0);
  assert(tbl->col.rng.cnt <= DB_MAX_TBL_COLS);
  assert(tbl->col.rng.cnt <= SQLITE_MAX_COLUMN);
  assert(tbl->col.rng.total <= SQLITE_MAX_COLUMN);

  assert(tbl->blb.rng.lo >= 0);
  assert(tbl->blb.rng.hi >= 0);
  assert(tbl->blb.rng.hi >= tbl->blb.rng.lo);
  assert(tbl->blb.rng.cnt >= 0);
  assert(tbl->blb.rng.cnt <= DB_MAX_BLB_ROW_CNT);

  assert(tbl->str.rng.lo >= 0);
  assert(tbl->str.rng.hi >= 0);
  assert(tbl->str.rng.hi >= tbl->str.rng.lo);
  assert(tbl->str.rng.cnt >= 0);
  assert(tbl->str.rng.cnt <= DB_MAX_STR_ROW_CNT);

  assert(db__tbl_fltr_val(&tbl->fltr));
  return 1;
}
priv inline int
db__state_is_val(const struct db_state *not_null sdb) {
  assert(sdb);
  assert(sdb->frame == DB_FRAME_LST || sdb->frame == DB_FRAME_TBL);
  assert(sdb->sel_tab >= 0);
  assert(sdb->sel_tab < DB_TBL_CNT);
  assert(sdb->sel_tab <= sdb->tab_cnt);

  assert(sdb->info.tab_act >= 0);
  assert(sdb->info.tab_act <= (1 << DB_TBL_TYPE_CNT)-1);
  assert(sdb->info.elm_cnt >= 0);
  assert(sdb->info.elm_cnt < DB_MAX_INFO_ELM_CNT);
  assert(sdb->info.sel_tab >= 0 && sdb->info.sel_tab < DB_TBL_TYPE_CNT);
  assert(rng__is_val(&sdb->info.elm_rng));
  for arr_loopv(idx, sdb->tbls) {
    db__tbl_state_is_val(&sdb->tbls[idx]);
  }
  for loopn(i, sdb->tab_cnt, DB_TBL_CNT) {
    int idx = sdb->tabs[i];
    assert(idx >= 0);
    assert(idx < DB_TBL_CNT);
    assert(idx < cntof(sdb->tabs));
  }
  if (sdb->tab_cnt) {
    int idx = sdb->tabs[sdb->sel_tab];
    assert(idx >= 0);
    assert(idx < DB_TBL_CNT);
    assert(idx < cntof(sdb->tabs));
  }
  return 1;
}
priv enum res_ico_id
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
priv int
db__tbl_qry_col_cnt_fltr_stmt(sqlite3_stmt **not_null stmt,
                              struct db_state *not_null sdb,
                              struct db_tbl_state *not_null stbl,
                              struct str tbl, struct str fltr) {
  requires(stmt);
  requires(stbl);
  requires(sdb);

  struct str sql = strv("SELECT COUNT(*) FROM pragma_table_info(?) WHERE name LIKE '%'||?||'%';");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    return err;
  }
  err = sqlite3_bind_text(*stmt, 1, db_str(tbl), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_text(*stmt, 2, db_str(fltr), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  return err;

failed_bind:
  sqlite3_finalize(*stmt);
  *stmt = 0;
  return err;
}
priv int
db__tbl_qry_col_cnt_no_fltr_stmt(sqlite3_stmt **not_null stmt,
                                 struct db_state *not_null sdb,
                                 struct db_tbl_state *not_null stbl,
                                 struct str tbl) {
  requires(stmt);
  requires(stbl);
  requires(sdb);

  struct str sql = strv("SELECT COUNT(*) FROM pragma_table_info(?);");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    return err;
  }
  err = sqlite3_bind_text(*stmt, 1, db_str(tbl), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    sqlite3_finalize(*stmt);
    *stmt = 0;
  }
  return err;
}
priv int
db_tbl_qry_col_cnt(struct db_state *not_null sdb,
                   struct db_tbl_state *not_null stbl,
                   struct str fltr) {

  requires(db__state_is_val(sdb));
  requires(stbl);
  requires(sdb);

  int cnt = 0;
  sqlite3_stmt *stmt = 0;
  struct db_name_lck lck = {0};
  if (!db_tbl_name_acq(&lck, sdb, stbl)) {
    int err = 0;
    if (str_len(fltr)) {
      err = db__tbl_qry_col_cnt_fltr_stmt(&stmt, sdb, stbl, lck.name, fltr);
    } else {
      err = db__tbl_qry_col_cnt_no_fltr_stmt(&stmt, sdb, stbl, lck.name);
    }
    if (err == SQLITE_OK) {
      err = sqlite3_step(stmt);
      if (err == SQLITE_ROW) {
        cnt = sqlite3_column_int(stmt, 0);
      }
      err = sqlite3_finalize(stmt);
      if (err != SQLITE_OK) {
        cnt = 0;
      }
    }
  }
  db_tbl_name_rel(&lck);

  ensures(cnt >= 0);
  ensures(db__state_is_val(sdb));
  return cnt;
}
priv int
db__tbl_qry_cols_sel_stmt(sqlite3_stmt **not_null stmt,
                         struct db_state *not_null sdb,
                         struct str tbl_name) {
  requires(sdb);
  requires(stmt);

  struct str sql = strv("SELECT rowid, name, type, \"notnull\", pk FROM pragma_table_info(?);");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    return err;
  }
  err = sqlite3_bind_text(*stmt, 1, db_str(tbl_name), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    sqlite3_finalize(*stmt);
    *stmt = 0;
  }
  return err;
}
priv int
db__tbl_qry_cols_fltr_sql(sqlite3_stmt **not_null stmt,
                          struct db_state *not_null sdb,
                          struct str tbl_name, struct str fltr,
                          int low, int high) {
  requires(sdb);
  requires(stmt);

  struct str sql = strv("SELECT rowid, name, type, \"notnull\", pk FROM pragma_table_info(?) WHERE name LIKE '%'||?||'%' LIMIT ?,?;;");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    return err;
  }
  err = sqlite3_bind_text(*stmt, 1, db_str(tbl_name), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_text(*stmt, 2, db_str(fltr), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_int(*stmt, 3, low);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_int(*stmt, 4, high-low);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  return err;

failed_bind:
  sqlite3_finalize(*stmt);
  *stmt = 0;
  return err;
}
priv int
db__tbl_qry_cols_no_fltr_sql(sqlite3_stmt **not_null stmt,
                           struct db_state *not_null sdb,
                           struct str tbl_name,
                           int low, int high) {
  requires(sdb);
  requires(stmt);

  struct str sql = strv("SELECT rowid, name, type, \"notnull\", pk FROM pragma_table_info(?) LIMIT ?,?;");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    return err;
  }
  err = sqlite3_bind_text(*stmt, 1, db_str(tbl_name), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_int(*stmt, 2, low);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_int(*stmt, 3, high-low);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  return err;

failed_bind:
  sqlite3_finalize(*stmt);
  *stmt = 0;
  return err;
}
priv int
db__tbl_qry_cols_stmt(sqlite3_stmt **not_null stmt,
                      struct db_state *not_null sdb,
                      struct str tbl_name, struct str fltr,
                      int low, int high) {
  requires(sdb);
  requires(stmt);

  int err = 0;
  if (str_len(fltr)) {
    err = db__tbl_qry_cols_fltr_sql(stmt, sdb, tbl_name, fltr, low, high);
  } else {
    err = db__tbl_qry_cols_no_fltr_sql(stmt, sdb, tbl_name, low, high);
  }
  return err;
}
priv int
db__tbl_qry_col_fk(struct db_state *not_null sdb,
                   struct str tbl_name, struct str col_name) {
  requires(sdb);
  sqlite3_stmt *stmt = 0;
  struct str sql = strv("SELECT rowid FROM pragma_foreign_key_list(?) WHERE \"from\" = ?");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
  if (err != SQLITE_OK) {
    return err;
  }
  err = sqlite3_bind_text(stmt, 1, db_str(tbl_name), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_text(stmt, 2, db_str(col_name), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_step(stmt);
  err = (err == SQLITE_ROW) ? SQLITE_OK : err;

failed_bind:
  sqlite3_finalize(stmt);
  return err;
}
priv int
db_tbl_qry_cols(struct db_state *not_null sdb,
                struct db_tbl_state *not_null stbl,
                struct db_tbl_view *not_null vtbl,
                int low, int high,
                struct str fltr, int sel) {

  requires(db__state_is_val(sdb));
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
  int err = 0;
  struct db_name_lck lck = {0};
  if (!db_tbl_name_acq(&lck, sdb, stbl)) {
    sqlite3_stmt *stmt = 0;
    if (sel) {
      err = db__tbl_qry_cols_sel_stmt(&stmt, sdb, lck.name);
    } else {
      err = db__tbl_qry_cols_stmt(&stmt, sdb, lck.name, fltr, low, high);
    }
    if (err == SQLITE_OK) {
      for db_loopn(_, err, stmt, SQLITE_MAX_COLUMN) {
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
        col->txt = strstr(col_type, "CHAR") != 0;
        col->txt |= strstr(col_type, "CLOB") != 0;
        col->txt |= strstr(col_type, "TEXT") != 0;
        col->fk = !!db__tbl_qry_col_fk(sdb, lck.name, name);
      }
      stbl->col.rng.hi = stbl->col.rng.lo + stbl->col.rng.cnt;
      assert(err == SQLITE_DONE);
      err = sqlite3_finalize(stmt);
    }
  } else {
    err = lck.err;
  }
  db_tbl_name_rel(&lck);
  if (err != SQLITE_OK) {
    stbl->col.rng = rng_nil;
  }
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
  return err;
}
priv int
db_tbl_qry_row_cols(struct db_state *not_null sdb,
                    struct db_tbl_state *not_null stbl,
                    struct db_tbl_view *not_null vtbl, int low) {

  requires(db__state_is_val(sdb));
  requires(stbl);
  requires(vtbl);

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

  int err = 0;
  if (vtbl->col.id != stbl->rowid || !stbl->col.rng.cnt ||
      !rng_has_inclv(&stbl->col.rng, lhs) ||
      !rng_has_inclv(&stbl->col.rng, rhs)) {
    err = db_tbl_qry_cols(sdb, stbl, vtbl, lhs, rhs, str_nil, 0);
  }
  if (err == SQLITE_OK) {
    int row_end = max(stbl->row.cols.cnt, stbl->col.rng.total) - stbl->row.cols.cnt;
    stbl->row.cols.lo = min(low, row_end);
    stbl->row.cols.hi = stbl->row.cols.lo + stbl->row.cols.cnt;
  } else {
    stbl->row.cols = rng_nil;
  }
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
  return err;
}
priv struct str
db_tbl_qry_fltr_stmt(struct db_state *not_null sdb,
                    struct db_view *not_null vdb,
                    struct db_tbl_state *not_null stbl,
                    struct db_tbl_view *not_null vtbl,
                    struct str tbl_name, struct str sql) {

  requires(db__state_is_val(sdb));
  requires(vdb);
  requires(stbl);
  requires(vtbl);

  /* fill with active filter conditions */
  const char *pre = " WHERE";
  sql = str_add_fmt_or_inv(arrv(vdb->sql_qry_buf), sql,
    " FROM \"%.*s\" ", strf(tbl_name));
  for arr_loopv(idx, stbl->fltr.elms) {
    struct db_tbl_fltr_elm *elm = &stbl->fltr.elms[idx];
    if (!elm->active || !elm->enabled) {
      continue;
    }
    struct db_name_lck clck = {0};
    if (!db_tbl_col_name_acq(&clck, sdb, vtbl, 0, tbl_name, elm->col)) {
      sql = str_add_fmt_or_inv(arrv(vdb->sql_qry_buf), sql,
        "%s \"%.*s\" LIKE '%%%.*s%%'", pre, strf(clck.name), strf(elm->fnd));
    } else {
      sql = str_inv;
      break;
    }
    db_tbl_col_name_rel(&clck);
    pre = " AND";
  }
  ensures(db__state_is_val(sdb));
  return sql;
}
priv int
db_tbl_qry_row_cnt(struct db_state *not_null sdb,
                   struct db_view *not_null vdb,
                   struct db_tbl_state *not_null stbl,
                   struct db_tbl_view *not_null vtbl) {

  requires(db__state_is_val(sdb));
  requires(vdb);
  requires(stbl);
  requires(vtbl);

  struct str sql = str_inv;
  struct db_name_lck lck = {0};
  if (!db_tbl_name_acq(&lck, sdb, stbl)) {
    sql = str_set(arrv(vdb->sql_qry_buf), strv("SELECT COUNT(*)"));
    sql = db_tbl_qry_fltr_stmt(sdb, vdb, stbl, vtbl, lck.name, sql);
  }
  db_tbl_name_rel(&lck);

  int cnt = 0;
  if (str_is_val(sql)) {
    sqlite3_stmt *stmt = 0;
    int err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    if (err == SQLITE_OK) {
      err = sqlite3_step(stmt);
      if (err == SQLITE_ROW) {
        cnt = sqlite3_column_int(stmt, 0);
      }
      sqlite3_finalize(stmt);
    }
  }
  ensures(cnt >= 0);
  ensures(db__state_is_val(sdb));
  return cnt;
}
priv int
db_tbl_qry_rows(struct db_state *not_null sdb,
                struct db_view *not_null vdb,
                struct db_tbl_state *not_null stbl,
                struct db_tbl_view *not_null vtbl,
                int low, int high) {
  int ret = 0;
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

  int elm_idx = 0;
  struct db_name_lck tlck = {0};
  if (!db_tbl_name_acq(&tlck, sdb, stbl)) {
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
      if (col->txt) {
        sql = str_add_fmt_or_inv(arrv(vdb->sql_qry_buf), sql, "SUBSTR(CAST(\"%.*s\" AS BLOB), 1, 1024), ", strf(clck.name));
      } else {
        sql = str_add_fmt_or_inv(arrv(vdb->sql_qry_buf), sql, "%.*s, ", strf(clck.name));
      }
      db_tbl_col_name_rel(&clck);
    }
    sql.rng = rng_lhs(&sql.rng, sql.rng.cnt-2);
    sql = db_tbl_qry_fltr_stmt(sdb, vdb, stbl, vtbl, tlck.name, sql);
    sql = str_add_fmt_or_inv(arrv(vdb->sql_qry_buf), sql, "LIMIT %d,%d", low, high-low);

    /* query table rows */
    if (str_is_val(sql)) {
      int err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
      if (err != SQLITE_OK) {
        goto failed;
      }
      for db_loop(_, err, stmt, vtbl->row.rowids) {
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
            vtbl->row.lst[elm_idx] = str_buf_sqz(&vtbl->row.buf, strv("[Open]"), DB_MAX_TBL_ELM_DATA);
          }
          elm_idx++;
        }
      }
      assert(err == SQLITE_DONE);
      err = sqlite3_finalize(stmt);
      assert(err == SQLITE_OK);
    } else {
  failed:
      stbl->row.rng = rng_nil;
      ret = -1;
    }
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
  return ret;
}
priv long long
db__tbl_qry_cell_len(struct db_state *not_null sdb,
                     struct db_view *not_null vdb,
                     struct db_tbl_state *not_null stbl,
                     struct db_tbl_view *not_null vtbl,
                     long long colid, long long rowid) {

  requires(db__state_is_val(sdb));
  requires(vdb);
  requires(stbl);
  requires(vtbl);

  long long siz = 0;
  struct db_name_lck tlck = {0};
  struct db_name_lck clck = {0};
  if (db_tbl_name_acq(&tlck, sdb, stbl) ||
      db_tbl_col_name_acq(&clck, sdb, vtbl, 0, tlck.name, colid)) {
    db_tbl_col_name_rel(&clck);
    db_tbl_name_rel(&tlck);
    return 0;
  }
  struct str sql = str_set_fmtsn(arrv(vdb->sql_qry_buf),
    "SELECT LENGTH(\"%.*s\") FROM \"%.*s\" WHERE ROWID = ?",
    strf(clck.name), strf(tlck.name));

  db_tbl_col_name_rel(&clck);
  db_tbl_name_rel(&tlck);
  if (str_is_inv(sql)) {
    return 0;
  }
  sqlite3_stmt *stmt = 0;
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
  if (err == SQLITE_OK) {
    err = sqlite3_bind_int64(stmt, 1, rowid);
    if (err == SQLITE_OK) {
      err = sqlite3_step(stmt);
      if (err == SQLITE_ROW) {
        siz = sqlite3_column_int64(stmt, 0);
      }
    }
    sqlite3_finalize(stmt);
  }
  return casti(siz);
}
priv int
db__tbl_qry_str_stmt(sqlite3_stmt **not_null stmt,
                     struct db_state *not_null sdb,
                     struct db_view *not_null vdb,
                     struct db_tbl_state *not_null stbl,
                     struct db_tbl_view *not_null vtbl,
                     long long colid, long long rowid,
                     int off, int len) {

  requires(db__state_is_val(sdb));
  requires(stmt);
  requires(vdb);
  requires(stbl);
  requires(vtbl);

  int err = 0;
  sqlite3_stmt *tbl_stmt = 0;
  sqlite3_stmt *col_stmt = 0;
  struct str tbl_name, col_name;
  if (!db__tbl_qry_name(&tbl_stmt, &tbl_name, sdb, stbl->rowid) &&
      !db__tbl_qry_col_name(&col_stmt, &col_name, sdb, tbl_name, colid)) {

    struct str sql = str_set_fmtsn(arrv(vdb->sql_qry_buf),
      "SELECT SUBSTR(\"%.*s\",%d,%d) FROM \"%.*s\" WHERE ROWID = ?",
      strf(col_name), off, len, strf(tbl_name));
    if (str_is_val(sql)) {
      err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
      if (err != SQLITE_OK) {
        return err;
      }
      err = sqlite3_bind_int64(*stmt, 1, rowid);
      if (err != SQLITE_OK) {
        sqlite3_finalize(tbl_stmt);
        sqlite3_finalize(col_stmt);
        sqlite3_finalize(*stmt);
        *stmt = 0;
        return err;
      }
    }
  }
  sqlite3_finalize(tbl_stmt);
  sqlite3_finalize(col_stmt);
  ensures(db__state_is_val(sdb));
  return err;
}
priv int
db_tbl_qry_str_row_cnt(struct db_state *not_null sdb,
                       struct db_view *not_null vdb,
                       struct db_tbl_state *not_null stbl,
                       struct db_tbl_view *not_null vtbl) {

  requires(db__state_is_val(sdb));
  requires(vdb);
  requires(stbl);
  requires(vtbl);

  long long siz = db__tbl_qry_cell_len(sdb, vdb, stbl, vtbl,
    stbl->str.colid, stbl->str.rowid);
  int len = casti(min(INT_MAX, siz));
  int cnt = div_ceil(len, DB_SQL_IO_BUF_SIZ);

  int off = 0;
  int ret = siz > 0;
  for loopn(i, cnt, INT_MAX / DB_SQL_IO_BUF_SIZ) {
    sqlite3_stmt *stmt = 0;
    int err = db__tbl_qry_str_stmt(&stmt, sdb, vdb, stbl, vtbl,
      stbl->str.colid, stbl->str.rowid, off, DB_SQL_IO_BUF_SIZ);
    if (err != SQLITE_OK || sqlite3_step(stmt) != SQLITE_ROW) {
      ret = 0;
      break;
    }
    const char *buf = (const char*)sqlite3_column_text(stmt, 0);
    int buf_len = sqlite3_column_bytes(stmt, 0);
    ret += cpu_str_cnt_ln(buf, buf_len);
    off += DB_SQL_IO_BUF_SIZ;
    sqlite3_finalize(stmt);
  }
  ensures(db__state_is_val(sdb));
  return ret;
}
priv int
db_tbl_qry_str(struct db_state *not_null sdb,
               struct db_view *not_null vdb,
               struct db_tbl_state *not_null stbl,
               struct db_tbl_view *not_null vtbl,
               long long colid, long long rowid,
               int low, int high) {

  requires(db__state_is_val(sdb));
  requires(vdb);
  requires(stbl);
  requires(vtbl);

  requires(low >= 0);
  requires(high >= 0);
  requires(low <= high);
  requires(low <= stbl->str.rng.total);
  requires(high <= stbl->str.rng.total);
  requires((high - low) < DB_MAX_STR_ROW_CNT);

  long long siz = db__tbl_qry_cell_len(sdb, vdb, stbl, vtbl,
    stbl->str.colid, stbl->str.rowid);
  int len = casti(min(INT_MAX, siz));
  int cnt = div_ceil(len, DB_SQL_IO_BUF_SIZ);
  int space = high - low;

  stbl->str.rng.cnt = 0;
  stbl->str.rng.lo = low;
  stbl->str.rng.hi = high;

  vdb->str.len = 0;
  vdb->str.rowid = stbl->str.rowid;
  vdb->str.colid = stbl->str.colid;

  int off = 0;
  int row_idx = 0;
  for (int i = 0; i < cnt &&
      i < INT_MAX / DB_SQL_IO_BUF_SIZ &&
      vdb->str.len < DB_SQL_STR_BUF_SIZ &&
      stbl->str.rng.cnt < space &&
      row_idx < high; ++i) {

    sqlite3_stmt *stmt = 0;
    int err = db__tbl_qry_str_stmt(&stmt, sdb, vdb, stbl, vtbl, colid,
      rowid, off, DB_SQL_IO_BUF_SIZ);
    if (err != SQLITE_OK) {
      return -1;
    }
    err = sqlite3_step(stmt);
    if (err != SQLITE_ROW) {
      return -1;
    }
    const char *buf = (const char*)sqlite3_column_text(stmt, 0);
    int buf_len = sqlite3_column_bytes(stmt, 0);
    for (int s = 0; s < buf_len && s < DB_SQL_IO_BUF_SIZ &&
        vdb->str.len < DB_SQL_STR_BUF_SIZ &&
        stbl->str.rng.cnt < DB_MAX_STR_ROW_CNT; ++s) {

      row_idx += (buf[s] == '\n');
      vdb->str.buf[vdb->str.len] = buf[s];
      vdb->str.len += (row_idx >= low && buf[s] != '\n');
      vdb->str.rows[stbl->str.rng.cnt] = vdb->str.len;
      stbl->str.rng.cnt += (row_idx >= low && buf[s] == '\n');
    }
    off += DB_SQL_IO_BUF_SIZ;
  }
  vdb->str.rows[stbl->str.rng.cnt] = (siz && stbl->str.rng.cnt < space) ? vdb->str.len : 0;
  stbl->str.rng.cnt += (siz && stbl->str.rng.cnt < space);

  ensures(vdb->str.colid == colid);
  ensures(vdb->str.rowid == rowid);
  ensures(vdb->str.len <= DB_SQL_STR_BUF_SIZ);
  ensures(vdb->str.len >= 0);

  ensures(stbl->str.rng.cnt == (high - low));
  ensures(stbl->str.rng.lo == low);
  ensures(stbl->str.rng.hi == high);
  ensures(stbl->str.rng.lo <= stbl->str.rng.hi);
  ensures(stbl->str.rng.lo + stbl->str.rng.cnt == stbl->str.rng.hi);

  ensures(db__state_is_val(sdb));
  return 0;
}
priv int
db_tbl_qry_blb_row_cnt(struct db_state *not_null sdb,
                       struct db_view *not_null vdb,
                       struct db_tbl_state *not_null stbl,
                       struct db_tbl_view *not_null vtbl) {

  requires(db__state_is_val(sdb));
  requires(vdb);
  requires(stbl);
  requires(vtbl);

  long long siz = db__tbl_qry_cell_len(sdb, vdb, stbl, vtbl,
    stbl->blb.colid, stbl->blb.rowid);
  int cnt = casti(min(INT_MAX, siz));
  ensures(db__state_is_val(sdb));
  return div_ceil(cnt, DB_MAX_BLB_HEX_COL_CNT);
}
priv int
db_tbl_qry_blb(struct db_state *not_null sdb,
               struct db_view *not_null vdb,
               struct db_tbl_state *not_null stbl,
               struct db_tbl_view *not_null vtbl,
               long long colid, long long rowid,
               int low, int high) {

  requires(vdb);
  requires(stbl);
  requires(vtbl);

  requires(low >= 0);
  requires(high >= 0);
  requires(low <= high);
  requires(low <= stbl->blb.rng.total);
  requires(high <= stbl->blb.rng.total);
  requires(high - low < DB_MAX_BLB_ROW_CNT);

  /* query table and column name */
  sqlite3_stmt *tbl_stmt = 0;
  sqlite3_stmt *col_stmt = 0;
  struct str tbl_name, col_name;
  if (db__tbl_qry_name(&tbl_stmt, &tbl_name, sdb, stbl->rowid) ||
      db__tbl_qry_col_name(&col_stmt, &col_name, sdb, tbl_name, colid)) {

    sqlite3_finalize(tbl_stmt);
    sqlite3_finalize(col_stmt);
    return -1;
  }
  /* open blob */
  sqlite3_blob *blb = 0;
  int rc = sqlite3_blob_open(sdb->con, "main", tbl_name.ptr, col_name.ptr, rowid, 1, &blb);
  sqlite3_finalize(tbl_stmt);
  sqlite3_finalize(col_stmt);
  if (rc != SQLITE_OK) {
    return -1;
  }
  /* query blob size */
  int digit_cnt = 0;
  sqlite3_int64 blb_siz = sqlite3_blob_bytes(blb);
  for (long long n = blb_siz - 1; n > 0; n >>= 4) {
    digit_cnt++;
  }
  /* setup blob */
  stbl->blb.rng.total = casti(div_ceil(blb_siz, DB_MAX_BLB_HEX_COL_CNT));
  stbl->blb.rng.lo = low;
  stbl->blb.rng.hi = high;
  stbl->blb.rng.cnt = high - low;

  vtbl->row.id = stbl->rowid;
  vdb->blb.rowid = stbl->blb.rowid;
  vdb->blb.colid = stbl->blb.colid;

  /* calculate memory block to request */
  int off = stbl->blb.rng.lo * DB_MAX_BLB_HEX_COL_CNT;
  int siz = min(stbl->blb.rng.cnt * DB_MAX_BLB_HEX_COL_CNT, casti(blb_siz) - off);
  assert(siz < cntof(vdb->sql_qry_buf));
  assert(siz >= 0);
  if (!siz) {
    return -1;
  }
  /* read memory */
  rc = sqlite3_blob_read(blb, vdb->sql_qry_buf, siz, off);
  sqlite3_blob_close(blb);
  if (rc != SQLITE_OK) {
    return -1;
  }
  /* fill hex string buffer */
  int addr = 0;
  struct str buf = str_nil;
  for loop(i, stbl->blb.rng.cnt) {
    /* generate hex value representation */
    int ln_begin = buf.rng.cnt;
    buf = str_add_fmt(arrv(vdb->blb.buf), buf, "%0*X: ", digit_cnt, off + addr);
    for (int c = 0; c < DB_MAX_BLB_HEX_COL_CNT && addr + c < siz; ++c) {
      unsigned char b = castb(vdb->sql_qry_buf[addr + c]);
      buf = str_add_fmt(arrv(vdb->blb.buf), buf, "%.2X ", b);
    }
    /* pad to align up to ascii represenation */
    if (off + addr + DB_MAX_BLB_HEX_COL_CNT > blb_siz) {
      int cnt = addr + DB_MAX_BLB_HEX_COL_CNT - siz;
      for loop(j,cnt) {
        buf = str_add(arrv(vdb->blb.buf), buf, strv("   "));
      }
    }
    buf = str_add(arrv(vdb->blb.buf), buf, strv("   "));
    for (int c = 0; c < DB_MAX_BLB_HEX_COL_CNT && addr + c < siz; ++c) {
      /* generate ascii representation */
      unsigned char byte = castb(vdb->sql_qry_buf[addr + c]);
      char sym = is_printable(byte) ? castc(byte) : '.';
      buf = str_add(arrv(vdb->blb.buf), buf, strc(sym));
    }
    vdb->blb.rows[i] = strn(buf.ptr + ln_begin, buf.rng.cnt - ln_begin);
    addr += DB_MAX_BLB_HEX_COL_CNT;
  }
  ensures(stbl->blb.rng.lo >= 0);
  ensures(stbl->blb.rng.hi >= 0);
  ensures(stbl->blb.rng.lo <= stbl->blb.rng.hi);
  ensures(stbl->blb.rng.lo + stbl->blb.rng.cnt == stbl->blb.rng.hi);
  ensures(db__state_is_val(sdb));
  return 0;
}
priv void
db_tbl_rev(struct db_state *not_null sdb,
           struct db_view *not_null vdb,
           struct db_tbl_state *not_null stbl,
           struct db_tbl_view *not_null view) {

  requires(vdb);
  requires(stbl);
  requires(view);
  requires(db__state_is_val(sdb));

  stbl->row.rng.total = db_tbl_qry_row_cnt(sdb, vdb, stbl, view);
  stbl->row.rng.cnt = 0;
  stbl->row.rng.lo = 0;
  stbl->row.rng.hi = 0;
}
priv void
db_tbl_setup(struct db_state *not_null sdb,
             struct db_view *not_null vdb, int idx,
             struct gui_ctx *not_null ctx,
             struct str name, long long rowid, enum db_tbl_type kind) {

  requires(db__state_is_val(sdb));
  requires(vdb);
  requires(ctx);
  requires(str_is_val(name));

  requires(idx >= 0);
  requires(idx < DB_TBL_CNT);
  requires(idx < cntof(sdb->tbls));

  /* setup table view */
  sdb->tbl_act |= (1u << idx);

  struct db_tbl_state *tbl = &sdb->tbls[idx];
  mset(&tbl->fltr.elms, 0, szof(tbl->fltr.elms));
  mset(&tbl->row.rng, 0, szof(tbl->row.rng));
  mset(&tbl->col.rng, 0, szof(tbl->col.rng));
  mset(&tbl->row.cols, 0, szof(tbl->row.cols));

  tbl->title = str_sqz(arrv(tbl->title_buf), name);
  tbl->qry_name = str_len(name) > str_len(tbl->title);
  tbl->disp = DB_TBL_VIEW_DSP_DATA;
  tbl->state = TBL_VIEW_DISPLAY;
  tbl->rowid = rowid;
  tbl->kind = kind;

  int col_cnt = db_tbl_qry_col_cnt(sdb, tbl, str_nil);
  tbl->col.total = col_cnt;
  tbl->col.cnt = min(col_cnt, DB_MAX_TBL_COLS);
  tbl->col.rng.total = col_cnt;
  tbl->col.rng.cnt = tbl->col.cnt;
  tbl->col.rng.hi = tbl->col.rng.cnt;
  tbl->col.rng.lo = 0;

  tbl->row.cols.cnt = min(tbl->col.rng.cnt, DB_MAX_TBL_ROW_COLS);
  tbl->row.cols.total = tbl->col.rng.total;
  tbl->row.cols.hi = tbl->row.cols.cnt;
  tbl->row.cols.lo = 0;

  /* setup table column display table */
  struct gui_split_lay bld = {0};
  gui.splt.lay.begin(&bld, tbl->row.ui.state, tbl->row.cols.cnt, ctx->cfg.sep);
  for loopn(i, tbl->row.cols.cnt, DB_MAX_TBL_ROW_COLS) {
    static const int cons[2] = {100, 2000};
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
  sqlite3_stmt *stmt = 0;
  struct str sql = str_set_fmtsn(arrv(vdb->sql_qry_buf), "SELECT COUNT(*) FROM \"%.*s\";", strf(name));
  tbl->row.rng.total = 0;
  if (str_is_val(sql)) {
    int err = sqlite3_prepare_v2(sdb->con, db_str(sql), &stmt, 0);
    if (err == SQLITE_OK) {
      err = sqlite3_step(stmt);
      if (err == SQLITE_ROW) {
        tbl->row.rng.total = sqlite3_column_int(stmt, 0);
      }
      sqlite3_finalize(stmt);
    }
  }
  ensures(db__state_is_val(sdb));
}
priv purist int
db_tbl_fltr_enabled(struct db_tbl_fltr_state *not_null fltr) {
  requires(fltr);
  for arr_loopv(idx, fltr->elms) {
    struct db_tbl_fltr_elm *elm = &fltr->elms[idx];
    if (elm->active && !elm->enabled) {
      return 0;
    }
  }
  return 1;
}
priv void
db_tbl_close(struct db_state *not_null sdb, int tbl) {
  requires(db__state_is_val(sdb));
  requires(tbl >= 0);
  requires(tbl < DB_TBL_CNT);
  requires(sdb->tab_cnt > 0);

  sdb->tbl_act &= ~(1u << tbl);
  sdb->tbls[tbl].state = TBL_VIEW_SELECT;
  ensures(db__state_is_val(sdb));
}
/* ---------------------------------------------------------------------------
 *                                Info
 * ---------------------------------------------------------------------------
 */
priv int
db__info_qry_cnt_fltr_stmt(sqlite3_stmt **not_null stmt,
                           struct db_state *not_null sdb,
                           const char *type, struct str fltr) {
  requires(stmt);
  requires(sdb);
  requires(type);

  struct str sql = strv("SELECT COUNT(*) FROM sqlite_master WHERE type = ? AND name LIKE '%'||?||'%';");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    return err;
  }
  err = sqlite3_bind_text(*stmt, 1, type, -1, SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_text(*stmt, 2, db_str(fltr), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  return err;

failed_bind:
  sqlite3_finalize(*stmt);
  *stmt = 0;
  return err;
}
priv int
db__info_qry_cnt_no_fltr_stmt(sqlite3_stmt **not_null stmt,
                              struct db_state *not_null sdb,
                              const char *type) {
  requires(stmt);
  requires(sdb);
  requires(type);

  struct str sql = strv("SELECT COUNT(*) FROM sqlite_master WHERE type = ?;");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    return err;
  }
  err = sqlite3_bind_text(*stmt, 1, type, -1, SQLITE_STATIC);
  if (err != SQLITE_OK) {
    sqlite3_finalize(*stmt);
    *stmt = 0;
  }
  return err;
}
priv int
db__info_qry_cnt_stmt(sqlite3_stmt **not_null stmt,
                      struct db_state *not_null sdb,
                      enum db_tbl_type tab, struct str fltr) {

  static const char *type[] = {
#define DB_INFO(a,b,c,d) b,
    DB_TBL_MAP(DB_INFO)
#undef DB_INFO
  };
  requires(tab >= 0);
  requires(tab <= cntof(type));

  int err = 0;
  if (str_len(fltr)) {
    err = db__info_qry_cnt_fltr_stmt(stmt, sdb, type[tab], fltr);
  } else {
    err = db__info_qry_cnt_no_fltr_stmt(stmt, sdb, type[tab]);
  }
  return err;
}
priv int
db_info_qry_cnt(struct db_state *not_null sdb, enum db_tbl_type tab, struct str fltr) {
  requires(db__state_is_val(sdb));
  requires(str__is_val(&fltr));

  int cnt = 0;
  sqlite3_stmt *stmt = 0;
  int err = db__info_qry_cnt_stmt(&stmt, sdb, tab, fltr);
  if (err != SQLITE_OK) {
    return 0;
  }
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    cnt = sqlite3_column_int(stmt, 0);
    err = sqlite3_finalize(stmt);
  }
  ensures(cnt >= 0);
  ensures(err == SQLITE_OK);
  ensures(db__state_is_val(sdb));
  return cnt;
}
priv int
db__info_qry_elm_fltr_stmt(sqlite3_stmt **not_null stmt,
                           struct db_state *not_null sdb,
                           struct db_info_state *not_null sinfo,
                           struct str type, struct str fltr,
                           int low, int high) {
  requires(sdb);
  requires(stmt);
  requires(sinfo);

  struct str sql = strv("SELECT rowid, name, sql FROM sqlite_master WHERE type = ? AND name LIKE '%'||?||'%' LIMIT ?,?;");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    return err;
  }
  err = sqlite3_bind_text(*stmt, 1, db_str(type), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_text(*stmt, 2, db_str(fltr), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_int(*stmt, 3, low);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_int(*stmt, 4, high-low);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  return err;

failed_bind:
  sqlite3_finalize(*stmt);
  *stmt = 0;
  return err;
}
priv int
db__info_qry_elm_no_fltr_stmt(sqlite3_stmt **not_null stmt,
                              struct db_state *not_null sdb,
                              struct db_info_state *not_null sinfo,
                              struct str type, int low, int high) {
  requires(sdb);
  requires(stmt);
  requires(sinfo);

  struct str sql = strv("SELECT rowid, name, sql FROM sqlite_master WHERE type = ? LIMIT ?,?;");
  int err = sqlite3_prepare_v2(sdb->con, db_str(sql), stmt, 0);
  if (err != SQLITE_OK) {
    return err;
  }
  err = sqlite3_bind_text(*stmt, 1, db_str(type), SQLITE_STATIC);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_int(*stmt, 2, low);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  err = sqlite3_bind_int(*stmt, 3, high-low);
  if (err != SQLITE_OK) {
    goto failed_bind;
  }
  return err;

failed_bind:
  sqlite3_finalize(*stmt);
  *stmt = 0;
  return err;
}
priv int
db__info_qry_elm_stmt(sqlite3_stmt **not_null stmt,
                      struct db_state *not_null sdb,
                      struct db_info_state *not_null sinfo,
                      struct str fltr, int low, int high) {
  requires(sdb);
  requires(stmt);
  requires(sinfo);
  static const char *type[] = {
#define DB_INFO(a,b,c,d) b,
    DB_TBL_MAP(DB_INFO)
#undef DB_INFO
  };
  int err = 0;
  struct str typ = str0(type[sinfo->sel_tab]);
  if (str_len(fltr)) {
    err = db__info_qry_elm_fltr_stmt(stmt, sdb, sinfo, typ, fltr, low, high);
  } else {
    err = db__info_qry_elm_no_fltr_stmt(stmt, sdb, sinfo, typ, low, high);
  }
  return err;
}
priv void
db_info_elm_add(struct db_info_state *not_null sinfo,
                struct db_info_view *not_null vinfo,
                long long rowid, struct str name, struct str sql) {

  requires(vinfo);
  requires(sinfo);
  requires(sinfo->elm_cnt < DB_MAX_INFO_ELM_CNT);

  int old_cnt = sinfo->elm_cnt;
  int elm_idx = sinfo->elm_cnt++;
  struct db_info_elm *elm = vinfo->elms + elm_idx;

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
  unused(old_cnt);
  ensures(elm->rowid == rowid);
  ensures(sinfo->elm_cnt == old_cnt + 1);
  ensures(elm == vinfo->elms + old_cnt);
  ensures(str_buf_len(elm->name) <= DB_MAX_TBL_NAME);
  ensures(str_buf_len(elm->sql) <= DB_MAX_TBL_SQL);
}
priv int
db_info_qry_elm(struct db_state *not_null sdb,
                struct db_info_state *not_null sinfo,
                struct db_info_view *not_null vinfo,
                int low, int high) {
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
  str_buf_clr(&vinfo->buf);
  sinfo->elm_rng = rng(low, high, sinfo->tab_cnt[sinfo->sel_tab]);
  sinfo->elm_cnt = 0;
  vinfo->id = sdb->id;

  sqlite3_stmt *stmt = 0;
  err = db__info_qry_elm_stmt(&stmt, sdb, sinfo, vinfo->fnd_str, low, high);
  if (err == SQLITE_OK) {
    for db_loopn(_, err, stmt, DB_MAX_INFO_ELM_CNT) {
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
  } else {
    sinfo->elm_cnt = 0;
    sinfo->elm_rng = rng_nil;
  }
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
pub int
db_init(void *not_null mem, int siz) {
  int err = sqlite3_config(SQLITE_CONFIG_HEAP, mem, siz, 64);
  return err == SQLITE_OK;
}
pub int
db_setup(struct db_state *not_null sdb,
         struct gui_ctx *not_null ctx,
         struct str path) {

  requires(sdb);
  requires(ctx);

  sdb->id = str_hash(path);
  int err = sqlite3_open(str_beg(path), &sdb->con);
  if (err != SQLITE_OK) {
    return 0;
  }
  sqlite3_db_config(sdb->con, SQLITE_DBCONFIG_TRUSTED_SCHEMA, 0, 0);

  /* query sqlite limits */
  sdb->limits.row_len = sqlite3_limit(sdb->con, SQLITE_LIMIT_LENGTH, -1);
  sdb->limits.sql_len = sqlite3_limit(sdb->con, SQLITE_LIMIT_SQL_LENGTH, -1);
  sdb->limits.col_cnt = sqlite3_limit(sdb->con, SQLITE_LIMIT_COLUMN, -1);

  /* setup ui table definitions */
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

  /* setup first tab */
  for arr_loopv(i, sdb->info.tab_cnt) {
    enum db_tbl_type tab_type = cast(enum db_tbl_type, i);
    sdb->info.tab_cnt[i] = db_info_qry_cnt(sdb, tab_type, str_nil);
    sdb->info.tab_act |= castu(!!sdb->info.tab_cnt[i]) << i;
  }
  if (!sdb->info.tab_act) {
    return 0;
  }
  int sel = cpu_bit_ffs32(sdb->info.tab_act);
  sdb->info.sel_tab = cast(enum db_tbl_type, sel);

  sdb->frame = DB_FRAME_TBL;
  sdb->tabs[sdb->tab_cnt] = 0;
  sdb->tab_cnt++;

  ensures(sdb->info.sel_tab >= 0);
  ensures(sdb->info.sel_tab < cntof(sdb->info.tab_cnt));
  ensures(db__state_is_val(sdb));
  return 1;
}
pub void
db_free(struct db_state *not_null sdb) {
  requires(db__state_is_val(sdb));
  mset(sdb->tbls, 0, sizeof(sdb->tbls));
  sdb->sel_tab = 0;
  sdb->tab_cnt = 0;
  if (sdb->con){
    sqlite3_close(sdb->con);
  }
  ensures(db__state_is_val(sdb));
}
priv void
db_tab_open_tbl_id(struct db_state *not_null sdb,
                   struct db_view *not_null vdb,
                   struct gui_ctx *not_null ctx,
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
  if (err != SQLITE_OK) {
    return;
  }
  err = sqlite3_bind_int64(stmt, 1, tbl_id);
  if (err != SQLITE_OK) {
    return;
  }
  err = sqlite3_step(stmt);
  if (err != SQLITE_ROW) {
    return;
  }
  const char *tbl_name = (const char*)sqlite3_column_text(stmt, 0);
  int tbl_name_len = sqlite3_column_bytes(stmt, 0);
  struct str tbl_name_str = strn(tbl_name, tbl_name_len);
  db_tbl_setup(sdb, vdb, view, ctx, tbl_name_str, tbl_id, sdb->info.sel_tab);

  sqlite3_finalize(stmt);
  ensures(db__state_is_val(sdb));
}
priv int
db_tab_open(struct db_state *not_null sdb, int tbl_idx) {
  requires(db__state_is_val(sdb));
  requires(tbl_idx >= 0);
  requires(tbl_idx < DB_TBL_CNT);
  requires(sdb->tab_cnt >= 0);
  requires(sdb->tab_cnt < DB_TBL_CNT);
  requires(!(sdb->tbl_act & (1u << tbl_idx)));

  int tab_idx = sdb->tab_cnt;
  sdb->tabs[tab_idx] = castc(tbl_idx);
  sdb->sel_tab = tbl_idx;
  sdb->tab_cnt++;

  ensures(db__state_is_val(sdb));
  return sdb->tab_cnt;
}
priv int
db_tab_open_new(struct db_state *not_null sdb) {
  assert(~sdb->tbl_act);
  assert(sdb->tab_cnt < DB_TBL_CNT);

  requires(db__state_is_val(sdb));
  int tbl_idx = cpu_bit_ffs64(~sdb->tbl_act);
  int tab_idx = db_tab_open(sdb, tbl_idx);

  ensures(tbl_idx >= 0);
  ensures(tbl_idx < DB_TBL_CNT);
  ensures(tab_idx >= 0);
  ensures(tab_idx < DB_TBL_CNT);
  ensures(db__state_is_val(sdb));
  return tab_idx;
}
priv void
db_tab_close(struct db_state *not_null sdb, int tab_idx) {
  requires(db__state_is_val(sdb));
  requires(sdb->tab_cnt > 0);
  requires(tab_idx >= 0);
  requires(tab_idx < DB_TBL_CNT);
  requires(tab_idx < sdb->tab_cnt);

  db_tbl_close(sdb, sdb->tabs[tab_idx]);
  sdb->sel_tab = min(sdb->sel_tab, max(0, sdb->tab_cnt-1));
  arr_rm(sdb->tabs, tab_idx, cntof(sdb->tabs));
  sdb->tab_cnt--;

  ensures(sdb->tab_cnt >= 0);
  ensures(db__state_is_val(sdb));
}

/* ---------------------------------------------------------------------------
 *
 *                                  GUI
 *
 * ---------------------------------------------------------------------------
 */
priv int
ui_btn_ico(struct gui_ctx *not_null ctx,
           struct gui_btn *not_null btn,
           struct gui_panel *not_null parent,
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
priv int
ui_btn_ico_txt(struct gui_ctx *not_null ctx,
               struct gui_btn *not_null btn,
               struct gui_panel *not_null parent,
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
priv int
ui_tab(struct gui_ctx *not_null ctx,
       struct gui_btn *not_null btn,
       struct gui_panel *not_null parent,
       struct str txt) {

  requires(ctx);
  requires(btn);
  requires(parent);

  static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
  gui.btn.begin(ctx, btn, parent);
  {
    int gapx = ctx->cfg.gap[0];
    struct gui_box lay = btn->pan.box;
    struct gui_panel ico = {.box = gui.cut.lhs(&lay, ctx->cfg.item, gapx)};
    gui.ico.img(ctx, &ico, &btn->pan, RES_ICO_TABLE);

    struct gui_panel lbl = {.box = lay};
    gui.txt.lbl(ctx, &lbl, &btn->pan, txt, &align);
  }
  gui.btn.end(ctx, btn, parent);
  return btn->clk;
}
priv struct str
ui_edit_fnd(struct gui_ctx *not_null ctx,
            struct gui_edit_box *not_null edt,
            struct gui_panel *not_null pan,
            struct gui_panel *not_null parent,
            struct gui_txt_ed *not_null ted,
            char *not_null buf, int cap, struct str str) {

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
priv int
ui_tbl_hdr_elm_ico(struct gui_ctx *not_null ctx,
                   struct gui_tbl *not_null tbl,
                   const int *not_null tbl_lay,
                   struct gui_btn *not_null slot,
                   int *not_null state,
                   enum res_ico_id ico) {
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
priv int
ui_tbl_hdr_elm_tog(struct gui_ctx *not_null ctx,
                   struct gui_tbl *not_null tbl,
                   const int *not_null tbl_lay,
                   int *not_null state, int act) {
  requires(ctx);
  requires(tbl);
  requires(state);
  requires(tbl_lay);

  struct gui_btn slot = {0};
  enum res_ico_id ico = act ? RES_ICO_TOGGLE_ON : RES_ICO_TOGGLE_OFF;
  return ui_tbl_hdr_elm_ico(ctx, tbl, tbl_lay, &slot, state, ico);
}
priv int
ui_tbl_hdr_elm_lock(struct gui_ctx *not_null ctx,
                    struct gui_tbl *not_null tbl,
                    const int *not_null tbl_lay,
                    int *not_null state, int act) {
  requires(ctx);
  requires(tbl);
  requires(state);
  requires(tbl_lay);

  struct gui_btn slot = {0};
  enum res_ico_id ico = act ? RES_ICO_UNLOCK : RES_ICO_LOCK;
  return ui_tbl_hdr_elm_ico(ctx, tbl, tbl_lay, &slot, state, ico);
}
priv int
ui_tbl_lst_elm_col_tog(struct gui_ctx *not_null ctx,
                       struct gui_tbl *not_null tbl,
                       const int *not_null tbl_lay,
                       struct gui_panel *not_null elm,
                       int *not_null is_act) {
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
priv void
ui_db_tbl_fltr_lst_tbl_hdr(struct db_tbl_fltr_state *not_null fltr,
                           struct gui_tbl *not_null tbl,
                           int *not_null tbl_cols, int col_cnt,
                           struct gui_ctx *not_null ctx) {
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
priv void
ui_db_tbl_fltr_lst_view(struct db_state *not_null sdb,
                        struct db_view *not_null vdb,
                        struct db_tbl_state *not_null stbl,
                        struct db_tbl_view *not_null vtbl,
                        struct db_tbl_fltr_state *not_null fltr,
                        struct gui_ctx *not_null ctx,
                        struct gui_panel *not_null pan,
                        struct gui_panel *not_null parent) {
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
            gui.tbl.lst.elm.col.slot(&col, ctx, &tbl, tbl_cols);
            gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, strv("[Empty Slot]"), 0);
            gui.tbl.lst.elm.col.slot(&col, ctx, &tbl, tbl_cols);
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
priv void
ui_db_tbl_fltr_view(struct db_state *not_null sdb,
                    struct db_view *not_null vdb,
                    struct db_tbl_state *not_null stbl,
                    struct db_tbl_view *not_null vtbl,
                    struct db_tbl_fltr_state *not_null fltr,
                    struct db_tbl_fltr_view *not_null view,
                    struct gui_ctx *not_null ctx,
                    struct gui_panel *not_null pan,
                    struct gui_panel *not_null parent) {
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
        fltr->data_rng = rng_nil;
        reg.lst.begin = 0;
        reg.lst.end = 0;
      }
      fltr->init = 0;

      assert(view->id == stbl->rowid);
      assert(view->buf.cnt >= 0);
      assert(view->buf.cnt <= cntof(view->buf.mem));

      assert(reg.lst.begin == fltr->data_rng.lo);
      assert(reg.lst.end == fltr->data_rng.hi);
      assert(db__tbl_fltr_val(fltr));
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
        struct str dat = str_buf_get(&view->buf, view->data[idx]);
        gui.txt.lbl(ctx, &lbl, &elm, dat, 0);
      }
      gui.lst.reg.elm.end(ctx, &reg, &elm);
    }
    gui.lst.reg.end(ctx, &reg, pan, fltr->off);
  }
  gui.pan.end(ctx, pan, parent);
}
priv void
ui_db_tbl_view_dsp_fltr(struct db_state *not_null sdb,
                        struct db_view *not_null vdb,
                        struct db_tbl_state *not_null stbl,
                        struct db_tbl_view *not_null vtbl,
                        struct db_tbl_fltr_state *not_null fltr,
                        struct db_tbl_fltr_view *not_null view,
                        struct gui_ctx *not_null ctx,
                        struct gui_panel *not_null pan,
                        struct gui_panel *not_null parent) {
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
priv enum res_ico_id
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
priv void
ui_db_tbl_view_hdr_key_slot(struct db_tbl_view *not_null view,
                            struct db_tbl_col *not_null col,
                            struct gui_ctx *not_null ctx,
                            struct gui_btn *not_null slot) {
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
priv void
ui_db_tbl_view_hdr_lnk_slot(struct db_tbl_state *not_null stbl,
                            struct db_tbl_view *not_null vtbl,
                            struct db_tbl_col *not_null col,
                            struct gui_ctx *not_null ctx,
                            struct gui_btn *not_null slot) {
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
priv void
ui_db_tbl_view_hdr_blob_slot(struct gui_ctx *not_null ctx, struct str txt,
                             struct gui_btn *not_null slot) {
  requires(ctx);
  requires(slot);

  static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
  struct gui_panel pan = {.box = slot->pan.box};
  gui.txt.lbl(ctx, &pan, &slot->pan, txt, &align);
}
priv void
ui_db_tbl_view_hdr_slot(struct db_tbl_state *not_null stbl,
                        struct db_tbl_view *not_null vtbl,
                        struct db_tbl_col *not_null col,
                        struct gui_ctx *not_null ctx,
                        struct gui_btn *not_null slot) {
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
priv void
ui_db_tbl_view_dsp_data_lst(struct db_state *not_null sdb,
                            struct db_view *not_null vdb,
                            struct db_tbl_state *not_null stbl,
                            struct db_tbl_view *not_null vtbl,
                            struct gui_ctx *not_null ctx,
                            struct gui_panel *not_null pan,
                            struct gui_panel *not_null parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(stbl);
  requires(vtbl);
  requires(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    /* reload column data */
    if (vtbl->col.id != stbl->rowid) {
      db_tbl_qry_row_cols(sdb, stbl, vtbl, stbl->row.cols.lo);
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
          stbl->row.rng = rng_nil;
          tbl.lst.begin = 0;
          tbl.lst.end = 0;
        }
        assert(vtbl->row.id == stbl->rowid);
        assert(tbl.lst.begin == stbl->row.rng.lo);
        assert(tbl.lst.end == stbl->row.rng.hi);
        assert(db__state_is_val(sdb));
      }
      int elm_idx = 0;
      for gui_tbl_lst_loopv(i, _, gui, &tbl, vtbl->row.rowids) {
        assert(i >= tbl.lst.begin);
        int idx = i - tbl.lst.begin;
        assert(idx < cntof(vtbl->row.rowids));

        struct gui_panel item = {0};
        long long rowid = vtbl->row.rowids[idx];
        struct gui_id id = gui_id64(hash_lld(rowid));

        gui.tbl.lst.elm.begin(ctx, &tbl, &item, id, 0);
        for looprn(j, stbl->row.cols, DB_MAX_TBL_ROW_COLS) {
          assert(j >= stbl->col.rng.lo);

          int col_idx = j - stbl->col.rng.lo;
          assert(col_idx < cntof(vtbl->col.lst));
          struct db_tbl_col *col = &vtbl->col.lst[col_idx];

          assert(elm_idx < cntof(vtbl->row.lst));
          unsigned long long elm = vtbl->row.lst[elm_idx++];
          struct str dat = str_buf_get(&vtbl->row.buf, elm);

          if (col->blob || (col->txt && str_len(dat) >= DB_MAX_FLTR_ELM_STR)) {
            struct gui_panel cel = {0};
            gui.tbl.lst.elm.col.lnk(ctx, &tbl, tbl_lay, &item, &cel, dat, 0);
            gui_panel_cur_hov(ctx, &cel, SYS_CUR_HAND);

            struct gui_input pin = {0};
            gui.pan.input(&pin, ctx, &cel, GUI_BTN_LEFT);
            if (pin.mouse.btn.left.clk) {
              if (col->blob) {
                stbl->data = DB_TBL_VIEW_DSP_DATA_BLOB;
                stbl->blb.colid = col->rowid;
                stbl->blb.rowid = vtbl->row.rowids[idx];
                stbl->blb.rng.total = db_tbl_qry_blb_row_cnt(sdb, vdb, stbl, vtbl);
              } else {
                stbl->data = DB_TBL_VIEW_DSP_DATA_STR;
                stbl->str.colid = col->rowid;
                stbl->str.rowid = vtbl->row.rowids[idx];
                stbl->str.rng.total = db_tbl_qry_str_row_cnt(sdb, vdb, stbl, vtbl);
              }
            }
          } else {
            gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_lay, &item, dat, 0);
          }
        }
        gui.tbl.lst.elm.end(ctx, &tbl, &item);
      }
      gui.tbl.lst.end(ctx, &tbl);
    }
    gui.tbl.end(ctx, &tbl, pan, stbl->row.ui.off);

    if (back) {
      db_tbl_qry_row_cols(sdb, stbl, vtbl, stbl->row.cols.lo-1);
    } else if (front) {
      db_tbl_qry_row_cols(sdb, stbl, vtbl, stbl->row.cols.lo+1);
    }
  }
  gui.pan.end(ctx, pan, parent);
}
priv void
ui_db_tbl_view_dsp_data_blb_hex(struct db_state *not_null sdb,
                                struct db_view *not_null vdb,
                                struct db_tbl_state *not_null stbl,
                                struct db_tbl_view *not_null vtbl,
                                struct gui_ctx *not_null ctx,
                                struct gui_panel *not_null pan,
                                struct gui_panel *not_null parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(stbl);
  requires(vtbl);
  requires(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_lst_cfg cfg = {0};
    gui.lst.cfg(&cfg, stbl->blb.rng.total, stbl->blb.off[1]);
    cfg.sel.src = GUI_LST_SEL_SRC_EXT;
    cfg.sel.on = GUI_LST_SEL_ON_HOV;
    cfg.ctl.show_cursor = 1;

    struct gui_lst_reg reg = {.box = pan->box};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, stbl->blb.off);
    if (vtbl->row.id != stbl->rowid ||
        vdb->blb.rowid != stbl->blb.rowid ||
        vdb->blb.colid != stbl->blb.colid ||
        reg.lst.begin != stbl->blb.rng.lo ||
        reg.lst.end != stbl->blb.rng.hi) {

      int ret = db_tbl_qry_blb(sdb, vdb, stbl, vtbl, stbl->blb.colid,
        stbl->blb.rowid, reg.lst.begin, reg.lst.end);
      if (ret < 0) {
        stbl->blb.rng = rng_nil;
        reg.lst.begin = 0;
        reg.lst.end = 0;
      }
      assert(vtbl->row.id == stbl->rowid);
      assert(vdb->blb.rowid == stbl->blb.rowid);
      assert(vdb->blb.colid == stbl->blb.colid);
      assert(reg.lst.begin == stbl->blb.rng.lo);
      assert(reg.lst.end == stbl->blb.rng.hi);
    }
    res.full_text_mode(ctx->res);
    for gui_lst_reg_loop(i,gui,&reg) {
      assert(i >= reg.lst.begin);
      /* display each line of hex/ascii data representation */
      struct gui_panel elm = {0};
      struct str ln = vdb->blb.rows[i - reg.lst.begin];
      unsigned long long elm_id = hash_ptr(ln.ptr);
      gui.lst.reg.elm.txt(ctx, &reg, &elm, gui_id64(elm_id), 0, ln, 0);
    }
    gui.lst.reg.end(ctx, &reg, pan, stbl->blb.off);
    res.word_mode(ctx->res);
  }
  gui.pan.end(ctx, pan, parent);
}
priv void
ui_db_tbl_view_dsp_data_blb(struct db_state *not_null sdb,
                            struct db_view *not_null vdb,
                            struct db_tbl_state *not_null stbl,
                            struct db_tbl_view *not_null vtbl,
                            struct gui_ctx *not_null ctx,
                            struct gui_panel *not_null pan,
                            struct gui_panel *not_null parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(stbl);
  requires(vtbl);
  requires(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_box lay = pan->box;
    struct gui_box btn = gui.cut.bot(&lay, ctx->cfg.item, ctx->cfg.gap[1]);

    /* tab control */
    int tab_cnt = 1;
    struct gui_tab_ctl tab = {.box = lay};
    gui.tab.begin(ctx, &tab, pan, tab_cnt, stbl->blb.sel_tab);
    {
      /* tab header */
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      gui.tab.hdr.item.txt(ctx, &tab, &hdr, gui_id64(15030268859237645412ull), strv("Hex"));
      gui.tab.hdr.end(ctx, &tab, &hdr);
      if (tab.sel.mod) {
        stbl->blb.sel_tab = tab.sel.idx;
      }
      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      switch (stbl->blb.sel_tab) {
      case DB_TBL_BLB_HEX:
        ui_db_tbl_view_dsp_data_blb_hex(sdb, vdb, stbl, vtbl, ctx, &bdy, pan);
        break;
      }
    }
    gui.tab.end(ctx, &tab, pan);

    /* back button */
    struct gui_btn back = {.box = btn};
    gui.btn.txt(ctx, &back, pan, strv("Back"), 0);
    if (back.clk) {
      stbl->data = DB_TBL_VIEW_DSP_DATA_LIST;
      stbl->blb.rng = rng_nil;
      vdb->blb.colid = 0;
      vdb->blb.rowid = 0;
    }
  }
  gui.pan.end(ctx, pan, parent);
}
priv void
ui_db_tbl_view_dsp_data_str(struct db_state *not_null sdb,
                            struct db_view *not_null vdb,
                            struct db_tbl_state *not_null stbl,
                            struct db_tbl_view *not_null vtbl,
                            struct gui_ctx *not_null ctx,
                            struct gui_panel *not_null pan,
                            struct gui_panel *not_null parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(stbl);
  requires(vtbl);
  requires(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_box lay = pan->box;
    struct gui_box btn = gui.cut.bot(&lay, ctx->cfg.item, ctx->cfg.gap[1]);

    struct gui_lst_cfg cfg = {0};
    gui.lst.cfg(&cfg, stbl->str.rng.total, stbl->str.off[1]);
    cfg.sel.src = GUI_LST_SEL_SRC_EXT;
    cfg.sel.on = GUI_LST_SEL_ON_HOV;
    cfg.ctl.show_cursor = 1;

    struct gui_lst_reg reg = {.box = lay};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, stbl->blb.off);
    if (vtbl->row.id != stbl->rowid ||
        vdb->str.rowid != stbl->str.rowid ||
        vdb->str.colid != stbl->str.colid ||
        reg.lst.begin != stbl->str.rng.lo ||
        reg.lst.end != stbl->str.rng.hi) {

      /* reload stale view data */
      int ret = db_tbl_qry_str(sdb, vdb, stbl, vtbl, stbl->str.colid,
        stbl->str.rowid, reg.lst.begin, reg.lst.end);
      if (ret < 0) {
        stbl->str.rng = rng_nil;
        reg.lst.begin = 0;
        reg.lst.end = 0;
      }
      assert(vdb->str.rowid == stbl->str.rowid);
      assert(vdb->str.colid == stbl->str.colid);
      assert(reg.lst.begin == stbl->str.rng.lo);
      assert(reg.lst.end == stbl->str.rng.hi);
    }
    res.full_text_mode(ctx->res);
    for gui_lst_reg_loop(i,gui,&reg) {
      assert(i >= reg.lst.begin);
      int idx = i - reg.lst.begin;

      int beg = (idx == 0) ? 0 : vdb->str.rows[idx-1];
      int end = vdb->str.rows[idx];
      int cnt = end - beg;
      struct str ln = strn(vdb->str.buf + beg, cnt);

      assert(beg >= 0);
      assert(cnt >= 0);
      assert(beg <= vdb->str.len);
      assert(beg + cnt <= vdb->str.len);

      struct gui_panel elm = {0};
      unsigned long long elm_id = hash_ptr(ln.ptr);
      gui.lst.reg.elm.txt(ctx, &reg, &elm, gui_id64(elm_id), 0, ln, 0);
    }
    gui.lst.reg.end(ctx, &reg, pan, stbl->blb.off);
    res.word_mode(ctx->res);

    /* back button */
    struct gui_btn back = {.box = btn};
    gui.btn.txt(ctx, &back, pan, strv("Back"), 0);
    if (back.clk) {
      stbl->data = DB_TBL_VIEW_DSP_DATA_LIST;
      stbl->str.rng = rng_nil;
      vdb->str.colid = 0;
      vdb->str.rowid = 0;
    }
  }
  gui.pan.end(ctx, pan, parent);
}
priv void
ui_db_tbl_view_dsp_data(struct db_state *not_null sdb,
                        struct db_view *not_null vdb,
                        struct db_tbl_state *not_null stbl,
                        struct db_tbl_view *not_null vtbl,
                        struct gui_ctx *not_null ctx,
                        struct gui_panel *not_null pan,
                        struct gui_panel *not_null parent) {
  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);

  requires(stbl);
  requires(vtbl);
  requires(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_panel bdy = {.box = pan->box};
    switch (stbl->data) {
    case DB_TBL_VIEW_DSP_DATA_LIST: {
      ui_db_tbl_view_dsp_data_lst(sdb, vdb, stbl, vtbl, ctx, &bdy, pan);
    } break;
    case DB_TBL_VIEW_DSP_DATA_BLOB: {
      ui_db_tbl_view_dsp_data_blb(sdb, vdb, stbl, vtbl, ctx, &bdy, pan);
    } break;
    case DB_TBL_VIEW_DSP_DATA_STR: {
      ui_db_tbl_view_dsp_data_str(sdb, vdb, stbl, vtbl, ctx, &bdy, pan);
    } break;}
  }
  gui.pan.end(ctx, pan, parent);
}
priv void
ui_db_tbl_view_dsp_lay(struct db_state *not_null sdb,
                       struct db_view *not_null vdb,
                       struct db_tbl_state *not_null stbl,
                       struct db_tbl_view *not_null vtbl,
                       struct gui_ctx *not_null ctx,
                       struct gui_panel *not_null pan,
                       struct gui_panel *not_null parent) {
  requires(sdb);
  requires(ctx);
  requires(pan);

  requires(stbl);
  requires(vtbl);
  requires(parent);

  struct gui_box lay = pan->box;
  gui.pan.begin(ctx, pan, parent);
  {
    /* filter */
    struct gui_edit_box edt = {0};
    edt.box = gui.cut.top(&lay, ctx->cfg.item, ctx->cfg.gap[1]);
    vdb->info.fnd_str = gui.edt.box(ctx, &edt, pan, vdb->info.fnd_buf, cntof(vdb->info.fnd_buf), vdb->info.fnd_str);
    if (edt.mod) {
      stbl->col.ui.off[1] = 0;
      stbl->col.rng.total = db_tbl_qry_col_cnt(sdb, stbl, vdb->info.fnd_str);
      stbl->col.rng.cnt = stbl->col.rng.lo = stbl->col.rng.hi = 0;
      stbl->row.cols.cnt = stbl->row.cols.lo = stbl->row.cols.hi = 0;
    }
    struct gui_tbl tbl = {.box = lay};
    gui.tbl.begin(ctx, &tbl, pan, stbl->col.ui.off, 0);
    {
      /* header */
      int tbl_cols[GUI_TBL_COL(DB_TBL_DISP_COL_MAX)];
      gui.tbl.hdr.begin(ctx, &tbl, arrv(tbl_cols), arrv(stbl->col.ui.state));
      if (ui_tbl_hdr_elm_lock(ctx, &tbl, tbl_cols, stbl->col.ui.state, !!stbl->col.state)) {
        switch (stbl->col.state) {
        default: assert(0); break;
        case DB_TBL_COL_STATE_LOCKED: {
          stbl->col.state = DB_TBL_COL_STATE_UNLOCKED;
          tbl_clr(&stbl->col.sel);
        } break;
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
      if (vtbl->col.id != stbl->rowid || edt.mod ||
          tbl.lst.begin != stbl->col.rng.lo ||
          tbl.lst.end != stbl->col.rng.hi) {
        db_tbl_qry_cols(sdb, stbl, vtbl, tbl.lst.begin, tbl.lst.end, vdb->info.fnd_str, 0);
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
                tbl_put(&stbl->col.sel, key, col->rowid);
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
            gui.tbl.lst.elm.col.slot(&item.box, ctx, &tbl, tbl_cols);
          }
          if (col->fk) {
            struct gui_icon icon;
            gui.tbl.lst.elm.col.ico(ctx, &tbl, tbl_cols, &item, &icon, RES_ICO_LINK);
          } else {
            gui.tbl.lst.elm.col.slot(&item.box, ctx, &tbl, tbl_cols);
          }
          if (col->nn) {
            struct gui_icon icon;
            gui.tbl.lst.elm.col.ico(ctx, &tbl, tbl_cols, &item, &icon, RES_ICO_CHECK);
          } else {
            gui.tbl.lst.elm.col.slot(&item.box, ctx, &tbl, tbl_cols);
          }
          if (!col->pk && !col->blob && !col->fk) {
            struct gui_icon icon;
            gui.tbl.lst.elm.col.ico(ctx, &tbl, tbl_cols, &item, &icon, RES_ICO_SEARCH);
            if (icon.clk) {
              db_tbl_open_fltr(stbl, col->rowid);
            }
          } else {
            gui.tbl.lst.elm.col.slot(&item.box, ctx, &tbl, tbl_cols);
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
priv void
ui_db_tbl_view_dsp(struct db_state *not_null sdb,
                   struct db_view *not_null vdb,
                   struct db_tbl_state *not_null stbl,
                   struct db_tbl_view *not_null vtbl,
                   struct gui_ctx *not_null ctx,
                   struct gui_panel *not_null pan,
                   struct gui_panel *not_null parent) {
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
        vdb->info.fnd_str = str_nil;
        if (stbl->col.state == DB_TBL_COL_STATE_UNLOCKED) {
          if (stbl->col.sel.cnt == 0) {
            stbl->col.state = DB_TBL_COL_STATE_LOCKED;
          }
        }
        if (stbl->col.state == DB_TBL_COL_STATE_UNLOCKED) {
          stbl->col.cnt = min(stbl->col.sel.cnt, DB_MAX_TBL_COLS);
          stbl->col.total = stbl->col.rng.total;

          stbl->col.rng.lo = 0;
          stbl->col.rng.cnt = stbl->col.sel.cnt;
          stbl->col.rng.hi = stbl->col.rng.cnt;
          stbl->col.rng.total = stbl->col.sel.cnt;

          stbl->row.cols.lo = 0;
          stbl->row.cols.cnt = min(stbl->col.rng.cnt, DB_MAX_TBL_ROW_COLS);
          stbl->row.cols.total = stbl->col.sel.cnt;
          stbl->row.cols.hi = stbl->row.cols.cnt;
          db_tbl_qry_cols(sdb, stbl, vtbl, 0, stbl->col.cnt, str_nil, 1);
        } else {
          stbl->col.rng.lo = 0;
          stbl->col.rng.cnt = min(stbl->col.cnt, DB_MAX_TBL_COLS);
          stbl->col.rng.hi = stbl->col.rng.cnt;
          stbl->col.rng.total = stbl->col.total;

          stbl->row.cols.lo = 0;
          stbl->row.cols.cnt = min(stbl->col.cnt, DB_MAX_TBL_ROW_COLS);
          stbl->row.cols.hi = stbl->row.cols.cnt;
          stbl->row.cols.total = stbl->col.total;
          db_tbl_qry_cols(sdb, stbl, vtbl, 0, stbl->col.cnt, str_nil, 0);
        }
      }
      if (tab.sel.idx == DB_TBL_VIEW_DSP_LAYOUT) {
        vdb->info.fnd_str = str_nil;
        if (stbl->col.state == DB_TBL_COL_STATE_UNLOCKED) {
          stbl->col.rng.total = stbl->col.total;
          stbl->col.rng.cnt = min(stbl->col.rng.total, DB_MAX_TBL_COLS);
          stbl->col.rng.lo = 0;
          stbl->col.rng.hi = stbl->col.rng.cnt;

          stbl->row.cols.lo = 0;
          stbl->row.cols.cnt = min(stbl->col.total, DB_MAX_TBL_ROW_COLS);
          stbl->row.cols.total = stbl->col.total;
          stbl->row.cols.hi = stbl->row.cols.cnt;

          stbl->col.cnt = stbl->col.rng.cnt;
          db_tbl_qry_cols(sdb, stbl, vtbl, 0, stbl->col.cnt, str_nil, 0);
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
priv void
ui_db_view_info_tbl(struct db_state *not_null sdb,
                    struct db_view *not_null vdb, int view,
                    struct db_info_state *not_null sinfo,
                    struct db_info_view *not_null vinfo,
                    struct gui_ctx *not_null ctx,
                    struct gui_panel *not_null pan,
                    struct gui_panel *not_null parent) {
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
      int tab_cnt = db_info_qry_cnt(sdb, sinfo->sel_tab, vinfo->fnd_str);
      sinfo->tab_cnt[sinfo->sel_tab] = tab_cnt;
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
      for gui_tbl_lst_loopv(i, _, gui, &tbl, vinfo->elms) {
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
priv void
ui_db_view_info(struct db_state *not_null sdb,
                struct db_view *not_null vdb, int view,
                struct db_info_state *not_null sinfo,
                struct db_info_view *not_null vinfo,
                struct gui_ctx *not_null ctx,
                struct gui_panel *not_null pan,
                struct gui_panel *not_null parent) {

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
          struct gui_panel slot = {0};
          struct gui_id iid = gui_id64(str_hash(def->title));
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
priv int
ui_db_tab_view_lst(struct db_state *not_null sdb,
                   struct gui_ctx *not_null ctx,
                   struct gui_panel *not_null pan,
                   struct gui_panel *not_null parent) {

  requires(ctx);
  requires(pan);
  requires(parent);
  requires(db__state_is_val(sdb));

  int ret = -1;
  int do_del = 0;
  int del_idx = -1;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_box lay = pan->box;
    struct gui_edit_box edt = {.flags = GUI_EDIT_SEL_ON_ACT};
    edt.box = gui.cut.top(&lay, ctx->cfg.item, ctx->cfg.gap[1]);
    sdb->fnd_str = gui.edt.box(ctx, &edt, pan, arrv(sdb->fnd_buf), sdb->fnd_str);

    struct gui_tbl tbl = {.box = lay};
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

      /* filter */
      unsigned long fltr = 0;
      if (str_len(sdb->fnd_str)) {
        for arr_loopn(tab_idx, sdb->tabs, sdb->tab_cnt) {
          int tbl_idx = sdb->tabs[tab_idx];
          assert(tbl_idx < cntof(sdb->tabs));
          assert(tbl_idx < DB_TBL_CNT);
          assert(tbl_idx >= 0);
          struct db_tbl_state *stbl = &sdb->tbls[tbl_idx];
          if (!(sdb->tbl_act & (1u << tbl_idx)) ||
              !str_has(stbl->title, sdb->fnd_str)) {
            fltr |= 1UL << tab_idx;
          }
        }
      }
      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, sdb->tab_cnt);
      cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_SINGLE;
      cfg.sel.on = GUI_LST_SEL_ON_HOV;
      cfg.fltr.on = GUI_LST_FLTR_ON_ONE;
      cfg.fltr.bitset = &fltr;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for gui_tbl_lst_loopn(idx, _, gui, &tbl, DB_TBL_CNT) {
        int tbl_idx = sdb->tabs[idx];
        assert(tbl_idx >= 0);
        assert(tbl_idx < DB_TBL_CNT);

        struct db_tbl_state *stbl = &sdb->tbls[idx];
        unsigned tbl_is_act = !!(sdb->tbl_act & (1u << idx));

        enum res_ico_id ico;
        struct str title = strv("New Tab");
        if (tbl_is_act) {
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
          if (tbl_is_act) {
            gui.tbl.lst.elm.col.txtf(ctx, &tbl, tbl_lay, pan, 0, "%d", stbl->col.rng.total);
            gui.tbl.lst.elm.col.txtf(ctx, &tbl, tbl_lay, pan, 0, "%d", stbl->row.rng.total);
            gui.tbl.lst.elm.col.txtf(ctx, &tbl, tbl_lay, pan, 0, "%d", stbl->fltr.cnt);
            gui.tbl.lst.elm.col.slot(&del.box, ctx, &tbl, tbl_lay);

            gui.ico.clk(ctx, &del, &elm, RES_ICO_TRASH);
            if (del.clk){
              del_idx = idx;
              do_del = 1;
            }
          } else {
            struct gui_box item = {0};
            gui.tbl.lst.elm.col.slot(&item, ctx, &tbl, tbl_lay);
            gui.tbl.lst.elm.col.slot(&item, ctx, &tbl, tbl_lay);
            gui.tbl.lst.elm.col.slot(&item, ctx, &tbl, tbl_lay);
            gui.tbl.lst.elm.col.slot(&item, ctx, &tbl, tbl_lay);
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
    db_tab_close(sdb, del_idx);
  }
  ensures(db__state_is_val(sdb));
  return ret;
}
priv void
ui_db_main(struct db_state *not_null sdb,
           struct db_view *not_null vdb, int view,
           struct gui_ctx *not_null ctx,
           struct gui_panel *not_null pan,
           struct gui_panel *not_null parent) {

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
priv void
ui_db_explr(struct db_state *not_null sdb,
            struct db_view *not_null vdb,
            struct gui_ctx *not_null ctx,
            struct gui_panel *not_null pan,
            struct gui_panel *not_null parent) {

  requires(sdb);
  requires(vdb);
  requires(ctx);
  requires(pan);
  requires(parent);

  if (sdb->id != vdb->id) {
    vdb->id = sdb->id;
    vdb->info.id = 0;
    vdb->tbl.row.id = 0;
    vdb->tbl.col.id = 0;
    vdb->tbl.fltr.id = 0;
  }
  gui.pan.begin(ctx, pan, parent);
  {
    int swap = 0;
    int swap_src = 0;
    int swap_dst = 0;

    int gapy = ctx->cfg.gap[1];
    struct gui_box lay = pan->box;
    int row = ctx->cfg.item + ctx->cfg.scrl;
    struct gui_box hdr = gui.cut.top(&lay, row, gapy);
    {
      int gapx = ctx->cfg.gap[0];
      struct gui_btn add = {.box = gui.cut.rhs(&hdr, row, gapx)};
      struct gui_btn tab = {.box = gui.cut.lhs(&hdr, row, gapx)};
      if (gui.btn.ico(ctx, &tab, pan, RES_ICO_TH_LIST)) {
        sdb->frame = DB_FRAME_LST;
      }
      gui.tooltip(ctx, &tab.pan, strv("View Table List"));

      /* search list */
      struct gui_lst_cfg cfg = {0};
      gui.lst.cfg(&cfg, sdb->tab_cnt, sdb->tab_off[1]);
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_SINGLE;
      cfg.sel.on = GUI_LST_SEL_ON_NEVER;
      cfg.sel.hov = GUI_LST_SEL_HOV_NO;
      cfg.lay.orient = GUI_HORIZONTAL;
      cfg.lay.item[0] = 2 * GUI_CFG_TAB;

      struct gui_lst_reg reg = {.box = hdr};
      gui.lst.reg.begin(ctx, &reg, pan, &cfg, sdb->tab_off);
      reg.reg.force_hscrl = 1;
      reg.reg.no_vscrl = 1;

      for gui_lst_reg_loop(i, gui, &reg) {
        int tbl_idx = sdb->tabs[i];
        assert(tbl_idx >= 0);
        assert(tbl_idx < cntof(sdb->tbls));
        assert(tbl_idx < DB_TBL_CNT);
        struct db_tbl_state *tbl = &sdb->tbls[tbl_idx];

        struct gui_panel elm = {0};
        struct gui_id id = gui_id64(hash_ptr(tbl));
        gui.lst.reg.elm.begin(ctx, &reg, &elm, id, 0);
        {
          int ret = 0;
          struct gui_btn btn = {.box = elm.box, .unfocusable = 1};
          if (sdb->sel_tab == i) {
            static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
            gui.btn.begin(ctx, &btn, parent);
            {
              struct gui_box hlay = btn.pan.box;
              struct gui_panel ico = {.box = gui.cut.lhs(&hlay, ctx->cfg.item, gapx)};
              gui.ico.img(ctx, &ico, &btn.pan, RES_ICO_TABLE);

              /* icon */
              struct gui_icon del = {.box = gui.cut.rhs(&hlay, ctx->cfg.item, gapx)};
              gui.ico.clk(ctx, &del, &btn.pan, RES_ICO_CLOSE);
              if (del.clk){
                db_tab_close(sdb, i);
              }
              struct gui_panel lbl = {.box = hlay};
              gui.txt.lbl(ctx, &lbl, &btn.pan, tbl->title, &align);
            }
            gui.btn.end(ctx, &btn, &elm);
            if (btn.in.mouse.btn.left.dragged) {
              int min_x = elm.box.x.min - (elm.box.x.ext >> 1);
              int max_x = elm.box.x.max + (elm.box.x.ext >> 1);
              if (i > 0 && btn.in.mouse.pos[0] < min_x) {
                swap = 1; swap_src = i; swap_dst = i - 1;
              } else if (i + 1 < sdb->tab_cnt && btn.in.mouse.pos[0] > max_x) {
                swap = 1; swap_src = i; swap_dst = i + 1;
              }
            }
            ret = btn.clk;
          } else {
            ret = ui_tab(ctx, &btn, &elm, tbl->title);
          }
          if (ret) {
            sdb->sel_tab = i;
          }
        }
        gui.lst.reg.elm.end(ctx, &reg, &elm);
      }
      /* shortcut handling */
      struct sys *_sys = ctx->sys;
      if ((_sys->keymod & SYS_KEYMOD_ALT) &&
          (_sys->keymod & SYS_KEYMOD_SHIFT) &&
          bit_tst_clr(_sys->keys, SYS_KEY_TAB)) {
        sdb->sel_tab = (sdb->sel_tab + sdb->sel_tab + 1) % sdb->sel_tab;
      } else if ((_sys->keymod & SYS_KEYMOD_ALT) &&
          bit_tst_clr(ctx->sys->keys, SYS_KEY_TAB)) {
        sdb->sel_tab = (sdb->sel_tab + 1) % sdb->sel_tab;
      } else if((_sys->keymod & SYS_KEYMOD_ALT) &&
          bit_tst_clr(ctx->sys->keys, 'w')) {
        db_tab_close(sdb, sdb->sel_tab);
      } else if((_sys->keymod & SYS_KEYMOD_ALT) &&
          bit_tst_clr(ctx->sys->keys, 't')) {
        sdb->frame = DB_FRAME_LST;
      }
      if (_sys->keymod == SYS_KEYMOD_ALT) {
        for loop(i, 9) {
          if (bit_tst_clr(ctx->sys->keys, '1' + i)) {
            sdb->sel_tab = min(i, max(0, sdb->tab_cnt));
          }
        }
      }
      gui.lst.reg.end(ctx, &reg, pan, sdb->tab_off);

      confine gui_disable_on_scope(&gui, ctx, !(~sdb->tbl_act)) {
        if (gui.btn.ico(ctx, &add, pan, RES_ICO_PLUS)) {
          db_tab_open_new(sdb);
        }
        gui.tooltip(ctx, &tab.pan, strv("Open a new Tab"));
      }
    }
    if (swap) {
      iswap(sdb->tabs[swap_src], sdb->tabs[swap_dst]);
      sdb->sel_tab = swap_dst;
    }
    struct gui_panel bdy = {.box = lay};
    switch (sdb->frame) {
    case DB_FRAME_CNT:
      assert(0); break;
    case DB_FRAME_LST: {
      int ret = ui_db_tab_view_lst(sdb, ctx, &bdy, pan);
      if (ret >= 0) {
        sdb->frame = DB_FRAME_TBL;
        sdb->sel_tab = ret;
      }
    } break;
    case DB_FRAME_TBL: {
      ui_db_main(sdb, vdb, sdb->tabs[sdb->sel_tab], ctx, &bdy, pan);
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

