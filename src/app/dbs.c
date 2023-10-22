/* ---------------------------------------------------------------------------
 *                                Database
 * ---------------------------------------------------------------------------
*/
#define MAX_FILTER 64
struct db_tbl_col_def {
  struct str title;
  struct gui_split_lay_slot ui;
  sort_f sort[2];
};
enum db_tbl_hdr_col {
  DB_TBL_NAME,
  DB_TBL_TYPE,
  DB_TBL_MAX
};
struct db_tbl_state {
  int cnt;
  struct gui_tbl_sort sort;
  int state[GUI_TBL_CAP(DB_TBL_MAX)];
};
enum db_tbl_lst_elm_type {
  DB_TBL_LST_ELM_UNKNOWN,
  DB_TBL_LST_ELM_TBL,
  DB_TBL_LST_ELM_VIEW,
  DB_TBL_LST_ELM_IDX,
  DB_TBL_LST_ELM_TRIGGER,
};
struct db_tbl_col {
  struct str name;
  struct str type;
  struct str dflt;

  unsigned not_null:1;
  unsigned pk:1;
  unsigned fk:1;
  unsigned blob:1;
};
struct db_tbl_ui_state {
  struct gui_tbl_sort sort;
  int *state;
  double off[2];
};
enum db_tbl_view_state {
  TBL_VIEW_SELECT,
  TBL_VIEW_DISPLAY,
  TBL_VIEW_FILTER_LIST,
  TBL_VIEW_FILTER_COL_SEL,
  TBL_VIEW_FILTER,
  TBL_VIEW_BLOB_VIEW,
};
enum db_tbl_fltr_col {
  DB_TBL_FLTR_STATE,
  DB_TBL_FLTR_BUF,
  DB_TBL_FLTR_COL,
  DB_TBL_FLTR_TYP,
  DB_TBL_FLTR_DEL,
  DB_TBL_FLTR_MAX
};
enum db_tbl_fltr_col_sel {
  DB_TBL_FLTR_COL_NAME,
  DB_TBL_FLTR_COL_TYPE,
  DB_TBL_FLTR_COL_MAX,
};
struct db_tbl_fltr_tm {
  time_t min;
  time_t max;

  struct tm from;
  struct tm to;

  time_t from_val;
  time_t to_val;
};
struct db_tbl_fltr {
  struct db_tbl_col *col;
  struct db_tbl_fltr_tm tm;
  struct str str;

  unsigned enabled: 1;
  unsigned is_date: 1;
  unsigned date_init: 1;
  unsigned hide_date: 1;
};
struct db_tbl_fltr_state {
  int cnt;
  struct gui_tbl_sort sort;
  int state[GUI_TBL_CAP(DB_TBL_FLTR_MAX)];
  double off[2];
};
struct db_tbl_fltr_view {
  dyn(struct db_tbl_fltr) lst;
  struct db_tbl_fltr_state tbl;
  struct db_tbl_fltr_state tbl_col;
  struct db_tbl_fltr ini;
  dyn(char) buf;
  unsigned rev;

  struct mem_scp scp;
  struct str *data;

  int sel_col;
  int row_cnt;
  int row_begin;
  int row_end;
  double off[2];
};
struct db_tbl_blob_img_view {
  int act;
  int id;
  int w, h;
  double off[2];
};
struct db_tbl_blob_view {
  unsigned disabled : 1;
  struct mem_scp scp;
  unsigned char *mem;
  int siz;
  struct db_tbl_blob_img_view img;
  int sel_tab;
  double off[2];
};
struct db_tbl_view {
  enum db_tbl_lst_elm_type kind;
  enum db_tbl_view_state state;

  int init;
  struct lst_elm hook;
  struct arena *tmp_mem;
  struct arena mem;
  struct mem_scp scp;

  unsigned rev;
  struct str name;
  struct db_tbl_col *cols;
  struct str *data;
  struct db_tbl_blob_view blob;
  struct db_tbl_fltr_view fltr;

  /* ui */
  int total;
  int row_cnt;
  int row_begin;
  int row_end;
  struct db_tbl_ui_state tbl;
};
enum db_tree_col_sel {
  DB_TREE_COL_NAME,
  DB_TREE_COL_TYPE,
  DB_TREE_COL_SQL,
  DB_TREE_COL_MAX,
};
struct db_tree_node {
  struct db_tree_node *parent;
  struct lst_elm hook;
  struct lst_elm sub;

  enum db_tbl_lst_elm_type kind;
  unsigned long long id;
  int depth;
  unsigned is_pk:1;
  unsigned is_fk:1;

  struct str name;
  struct str type;
  struct str sql;
};
struct db_tree_tbl_state {
  struct gui_tbl_sort sort;
  int state[GUI_TBL_CAP(DB_TREE_COL_MAX)];
  double off[2];
};
struct db_tree_view {
  unsigned rev;
  struct arena mem;
  dyn(struct db_tree_node*) lst;
  unsigned long long *exp;
  tbl(struct db_tree_node*) sel;

  /* tree */
  struct db_tree_node root;
  struct db_tree_node *tbls;
  struct db_tree_node *views;
  struct db_tree_node *idxs;
  struct db_tree_node *seqs;
  struct db_tree_node *trigs;

  /* ui */
  struct db_tree_tbl_state tbl;
  dyn(char) fnd_buf;
  struct gui_txt_ed fnd_ed;
};
struct db_graph_node {
  struct str name;
  unsigned long long id;
  struct gui_box box;
  dyn(struct db_tbl_col) cols;
  int pk_idx;
};
struct db_graph {
  double off[2];
  dyn(struct db_graph_node) nodes;
  struct gui_box box;
};
enum db_view_state {
  DB_VIEW_TREE,
  DB_VIEW_GRAPH,
  DB_VIEW_CNT
};
struct db_ui_view {
  struct arena mem;
  struct arena *tmp_mem;
  sqlite3 *con;
  struct str path;

  enum db_view_state state;
  struct db_tree_view tree;
  struct db_graph graph;
  unsigned tree_rev;

  /* views */
  int show_tab_lst;
  double tbl_lst_off[2];
  dyn(struct db_tbl_view*) tbls;
  struct lst_elm del_lst;
  int sel_tbl;
};

/* ---------------------------------------------------------------------------
 *
 *                                Database
 *
 * ---------------------------------------------------------------------------
 */
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

// clang-format off
static const struct db_tbl_col_def db_tbl_fltr_def[DB_TBL_FLTR_MAX] = {
  [DB_TBL_FLTR_STATE] = {.title = strv(""),                                                                     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 60,  .con = {10, 200}}},
  [DB_TBL_FLTR_BUF]   = {.title = strv("Filter"), .sort = {db_tbl_fltr_cmp_asc,     db_tbl_fltr_cmp_desc},      .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 800}}},
  [DB_TBL_FLTR_COL]   = {.title = strv("Column"), .sort = {db_tbl_fltr_cmp_asc,     db_tbl_fltr_cmp_desc},      .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {200, 800}}},
  [DB_TBL_FLTR_TYP]   = {.title = strv("Type"),   .sort = {db_tbl_fltr_cmp_type_asc,db_tbl_fltr_cmp_type_desc}, .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1,   .con = {100, 800}}},
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
static void
db_tree_node_lnk(struct db_tree_node *n, struct db_tree_node *s) {
  assert(s);
  assert(n);

  struct lst_elm *elm = &n->sub;
  lst_del(&s->hook);
  lst_init(&s->hook);
  lst__add(&s->hook, elm->prv, elm);
}
struct db_tree_node_arg {
  struct str name;
  struct str type;
  struct str sql;
  unsigned long long id;
  enum db_tbl_lst_elm_type kind;
  unsigned is_pk:1;
  unsigned is_fk:1;
};
static void
db_tree_node_setup(struct sys *_sys, struct arena *mem, struct db_tree_node *s,
                   struct db_tree_node *n, const struct db_tree_node_arg *arg) {
  assert(s);
  assert(n);
  assert(mem);

  s->id = arg->id;
  s->parent = n;
  s->depth = n->depth + 1;
  s->is_pk = arg->is_pk;
  s->is_fk = arg->is_fk;
  s->kind = arg->kind;
  s->name = arena_str(mem, _sys, arg->name);
  s->type = arena_str(mem, _sys, arg->type);
  s->sql = arena_str(mem, _sys, arg->sql);
  for str_loopi(i, &s->sql) {
    if (!is_printable(s->sql.str[i])) {
      *recast(char*, s->sql.str + i) = ' ';
    }
  }
  lst_init(&s->hook);
  lst_init(&s->sub);
  db_tree_node_lnk(n, s);
}
static struct db_tree_node*
db_tree_node_new(struct sys *_sys, struct arena *mem, struct db_tree_node *n,
                 const struct db_tree_node_arg *arg) {
  assert(n);
  assert(mem);

  struct db_tree_node *s = arena_alloc(mem, _sys, szof(*s));
  db_tree_node_setup(_sys, mem, s, n, arg);
  return s;
}
static struct db_tree_node*
db_tree_root_node_new(struct db_tree_view *t, struct sys *_sys, struct arena *mem,
                      struct str name) {
  assert(t);
  assert(sys);
  assert(mem);

  struct db_tree_node_arg arg = {0};
  arg.name = name;
  arg.type = strv("folder");
  arg.sql = str_nil;
  arg.id = str_hash(name);
  arg.kind = DB_TBL_LST_ELM_UNKNOWN;
  return db_tree_node_new(_sys, mem, &t->root, &arg);
}
static enum res_ico_id
db_tree_node_icon(const struct db_tree_view *t, const struct db_tree_node *n) {
  assert(n);
  if (n->is_pk) {
    return RES_ICO_KEY;
  } else if (n->is_fk) {
    return RES_ICO_LINK;
  } else if (str_eq(n->type, strv("folder"))) {
    if (set_fnd(t->exp, n->id)) {
      return RES_ICO_FOLDER_OPEN;
    } else {
      return RES_ICO_FOLDER;
    }
  } else if (str_eq(n->type, strv("table"))) {
    return RES_ICO_TABLE;
  } else if (str_eq(n->type, strv("view"))) {
    return RES_ICO_IMAGE;
  } else if (str_eq(n->type, strv("trigger"))) {
    return RES_ICO_BOLT;
  } else if (str_eq(n->type, strv("index"))) {
    return RES_ICO_TAG;
  } else {
    if (str_eq(n->type, strv("REAL")) ||
        str_eq(n->type, strv("DOUBLE")) ||
        str_eq(n->type, strv("DOUBLE PRECISION")) ||
        str_eq(n->type, strv("FLOAT"))) {
      return RES_ICO_MODIFY;
    } else if (str_eq(n->type, strv("DATE"))) {
      return RES_ICO_CALENDAR;
    } else if (str_eq(n->type, strv("DATETIME"))) {
      return RES_ICO_CALENDAR;
    } else if (str_eq(n->type, strv("BLOB"))) {
      return RES_ICO_CUBE;
    } else if (str_eq(n->type, strv("BOOLEAN"))) {
      return RES_ICO_CHECK;
    } else if (str_eq(n->type, strv("INT")) ||
        str_eq(n->type, strv("INTEGER")) ||
        str_eq(n->type, strv("TINYINT")) ||
        str_eq(n->type, strv("SMALLINT")) ||
        str_eq(n->type, strv("MEDIUMINT")) ||
        str_eq(n->type, strv("BIGINT")) ||
        str_eq(n->type, strv("UNSIGNED BIG INT")) ||
        str_eq(n->type, strv("INT2")) ||
        str_eq(n->type, strv("INT8")) ||
        str_eq(n->type, strv("NUMERIC"))) {
      return RES_ICO_CALCULATOR;
    } else {
      return RES_ICO_FONT;
    }
  }
}
static void
db_tree_begin(struct db_tree_view *t, struct sys *_sys, struct arena *mem) {
  assert(t);
  assert(_sys);
  assert(mem);

  t->rev = (unsigned)-1;
  t->lst = arena_dyn(mem, _sys, struct db_tree_node*, 128);
  t->exp = arena_set(mem, _sys, 1024);
  t->fnd_buf = arena_dyn(mem, _sys, char, MAX_FILTER);
  t->sel = arena_tbl(mem, _sys, struct db_tree_node*, 256);
  gui.edt.buf.init(&t->fnd_ed);

  /* setup root node */
  t->root.depth = 0;
  t->root.parent = 0;
  t->root.id = str_hash(strv("root"));

  lst_init(&t->root.hook);
  lst_init(&t->root.sub);
  set_put(t->exp, _sys, t->root.id);

  /* setup base nodes */
  t->tbls = db_tree_root_node_new(t, _sys, mem, strv("Tables"));
  t->views = db_tree_root_node_new(t, _sys, mem, strv("Views"));
  t->idxs = db_tree_root_node_new(t, _sys, mem, strv("Indexes"));
  t->seqs = db_tree_root_node_new(t, _sys, mem, strv("Sequences"));
  t->trigs = db_tree_root_node_new(t, _sys, mem, strv("Triggers"));
}
static struct db_tbl_col*
db_tbl_qry_cols(sqlite3 *con, struct db_tbl_view *view, struct sys *_sys,
                struct str id, struct db_tbl_col *cols, struct arena *mem,
                struct arena *tmp_mem) {
  assert(con);
  assert(mem);
  assert(cols);
  assert(view);
  assert(tmp_mem);

  /* query table column schema */
  sqlite3_stmt *stmt = 0;
  struct str sql = arena_fmt(tmp_mem, _sys, "PRAGMA table_info(%.*s);", strf(id));
  sqlite3_prepare_v2(con, sql.str, -1, &stmt, 0);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *col_name = (const char*)sqlite3_column_text(stmt, 1);
    const char *col_type = (const char*)sqlite3_column_text(stmt, 2);
    const char *col_notnull = (const char*)sqlite3_column_text(stmt, 3);
    const char *col_dflt = (const char*)sqlite3_column_text(stmt, 4);
    const char *col_key = (const char*)sqlite3_column_text(stmt, 5);

    int nam_len = sqlite3_column_bytes(stmt, 1);
    int typ_len = sqlite3_column_bytes(stmt, 2);

    struct db_tbl_col col = {0};
    col.name = arena_str(mem, _sys, str(col_name, nam_len));
    col.type = arena_str(mem, _sys, str(col_type, typ_len));
    col.dflt = col_dflt ? arena_str(mem, _sys, str0(col_dflt)) : str_nil;
    col.not_null = !strcmp(col_notnull, "1");
    col.pk = !strcmp(col_key, "1");
    col.blob = !col.type.len || col_type[0] == 0;
    col.blob = col.blob || !str_cmp(col.type, strv("BLOB"));
    if (view && col.pk && col.blob) {
      view->blob.disabled = 1;
    }
    dyn_add(cols, _sys, col);
  }
  sqlite3_finalize(stmt);
  stmt = 0;

  /* query table column foreign keys */
  sql = arena_fmt(tmp_mem, _sys, "PRAGMA foreign_key_list(%.*s);", strf(id));
  sqlite3_prepare_v2(con, sql.str, -1, &stmt, 0);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *ccol_from = (const char*)sqlite3_column_text(stmt, 3);
    struct str col_from = str0(ccol_from);
    struct db_tbl_col *col = 0;
    for dyn_loop(col, cols) {
      if (!str_cmp(col->name, col_from)) {
        col->fk = 1;
        break;
      }
    }
  }
  sqlite3_finalize(stmt);
  return cols;
}
static void
db_tree_qry_tbl(struct db_ui_view *d, struct db_tree_node *n,
                struct sys *_sys, struct arena *mem) {
  assert(n);
  assert(db);
  assert(mem);

  /* query table columns */
  scp_mem(d->tmp_mem, _sys) {
    struct db_tbl_col *cols = arena_dyn(d->tmp_mem, _sys, struct db_tbl_col, 128);
    cols = db_tbl_qry_cols(d->con, 0, _sys, n->name, cols, d->tmp_mem, d->tmp_mem);
    /* create node for each table column */
    struct db_tbl_col *col = 0;
    for dyn_loop(col, cols) {
      struct str sql = str_nil;
      if (col->not_null) {
        sql = arena_fmt(d->tmp_mem, _sys, "\"%.*s\" %.*s NOT NULL",
                        strf(col->name), strf(col->type));
      } else {
        sql = arena_fmt(d->tmp_mem, _sys, "\"%.*s\" %.*s",
                        strf(col->name), strf(col->type));
      }
      struct db_tree_node_arg arg = {0};
      arg.sql = sql;
      arg.name = col->name;
      arg.type = col->type;
      arg.is_pk = col->pk;
      arg.is_fk = col->fk;
      arg.id = str__hash(col->name, n->id);
      db_tree_node_new(_sys, mem, n, &arg);
    }
    dyn_free(cols, _sys);
  }
}
static void
db_tree_end(struct db_ui_view *d, struct db_tree_view *t, struct arena *mem,
            struct gui_ctx *ctx) {
  assert(t);
  assert(db);
  assert(mem);
  assert(ctx);

  /* setup table ui state */
  struct gui_split_lay_cfg tbl_cfg = {0};
  tbl_cfg.size = szof(struct db_tbl_col_def);
  tbl_cfg.off = offsetof(struct db_tbl_col_def, ui);
  tbl_cfg.slots = db_tree_col_def;
  tbl_cfg.cnt = DB_TREE_COL_MAX;
  gui.tbl.lay(t->tbl.state, ctx, &tbl_cfg);
  zero2(t->tbl.off);

  /* setup table tree nodes */
  struct lst_elm *elm = 0;
  for lst_loop(elm, &t->tbls->sub) {
    struct db_tree_node *n = 0;
    n = lst_get(elm, struct db_tree_node, hook);
    db_tree_qry_tbl(d, n, ctx->sys, mem);
  }
  /* setup views tree nodes */
  for lst_loop(elm, &t->views->sub) {
    struct db_tree_node *n = 0;
    n = lst_get(elm, struct db_tree_node, hook);
    db_tree_qry_tbl(d, n, ctx->sys, mem);
  }
}
static void
db_graph_begin(struct db_graph *g, struct sys *_sys, struct arena *mem) {
  assert(g);
  assert(mem);

  g->off[0] = g->off[1] = 0;
  g->nodes = arena_dyn(mem, _sys, struct db_graph_node, 256);
  g->box = gui.box.box(0,0,0,0);
}
static void
db_graph_end(struct db_ui_view *d, struct db_graph *g, struct gui_ctx *ctx,
             struct arena *mem, struct arena *tmp) {
  assert(g);
  assert(db);
  assert(ctx);
  assert(mem);
  assert(tmp);

  int suf_w = gui.txt.width(ctx, strv("NOT NULL")) + ctx->cfg.gap[0];
  int base_w = suf_w + ctx->cfg.pan_pad[0] * 2;

  struct db_graph_node *n = 0;
  for dyn_loop(n, g->nodes) {
    /* query node table column layout */
    n->cols = arena_dyn(mem, ctx->sys, struct db_tbl_col, 64);
    n->cols = db_tbl_qry_cols(d->con, 0, ctx->sys, n->name, n->cols, mem, tmp);

    /* calculate node size  */
    int min_h = (dyn_cnt(n->cols) + 1) * ctx->cfg.item + 4;
    int hdr_w = gui.txt.width(ctx, n->name) + ctx->cfg.item + ctx->cfg.gap[0];
    int min_w = hdr_w + base_w;

    const struct db_tbl_col *col = 0;
    for dyn_loop(col, n->cols) {
      int w = gui.txt.width(ctx, col->name) + ctx->cfg.item + ctx->cfg.gap[0];
      min_w = max(min_w, w + base_w);
      if (col->pk) {
        n->pk_idx = cast(int, col - n->cols);
      }
    }
    n->box = gui.box.box(0,0,min_w,min_h);
  }
  /* graph auto node layouting */
  int at_x = 0;
  for dyn_loop(n, g->nodes) {
    n->box.x = gui.bnd.min_ext(at_x, n->box.x.ext);
    at_x += n->box.x.ext + ctx->cfg.grid;
  }
}
static struct db_tree_node**
db_tree_serial(struct db_tree_view *tree, struct db_tree_node *n,
               struct db_tree_node **lst, struct sys *_sys) {
  assert(n);
  assert(lst);
  assert(tree);
  if (n->parent) {
    dyn_add(lst, _sys, n);
  }
  if (set_fnd(tree->exp, n->id)) {
    struct lst_elm *it = 0;
    for lst_loop(it, &n->sub) {
      struct db_tree_node *s;
      s = lst_get(it, struct db_tree_node, hook);
      lst = db_tree_serial(tree, s, lst, _sys);
    }
  }
  return lst;
}
static void
db_tree_update(struct db_tree_view *t, struct sys *_sys) {
  assert(t);
  dyn_clr(t->lst);
  t->lst = db_tree_serial(t, &t->root, t->lst, _sys);
}
static struct db_tbl_view*
db_tbl_view_new(struct db_ui_view *d, struct sys *_sys) {
  assert(db);
  struct db_tbl_view *s;
  if (lst_any(&d->del_lst)) {
    s = lst_get(d->del_lst.nxt, struct db_tbl_view, hook);
    lst_del(d->del_lst.nxt);
  } else {
    s = arena_alloc(&d->mem, _sys, szof(*s));
  }
  lst_init(&s->hook);
  return s;
}
static void
db_tbl_view_del(struct db_ui_view *d, struct db_tbl_view *view, struct sys* _sys) {
  assert(db);
  assert(view);
  if (view->data) {
    mem_scp_end(&view->scp, &view->mem, _sys);
  }
  dyn_free(view->cols, _sys);
  arena_free(&view->mem, _sys);

  view->cols = 0;
  view->data = 0;
  view->tmp_mem = 0;
  view->name = str_nil;
  view->state = TBL_VIEW_SELECT;
  mset(&view->scp, 0, sizeof(view->scp));

  view->row_cnt = 0;
  view->row_end = 0;
  view->row_begin = 0;

  memset(&view->tbl, 0, sizeof(view->tbl));
  memset(&view->blob, 0, sizeof(view->blob));

  lst_init(&view->hook);
  lst_add(&d->del_lst, &view->hook);
}
static char*
db_tbl_sql(struct db_tbl_view *view, struct sys *_sys, struct arena *mem,
               const struct str sel, int off, int lim) {
  assert(mem);
  assert(view);

  const char *pre = " WHERE";
  char *sql = arena_dyn(mem, _sys, char, KB(16));
  dyn_fmt(sql, _sys, "SELECT %.*s FROM %.*s", strf(sel), strf(view->name));

  struct db_tbl_fltr *fltr = 0;
  for dyn_loop(fltr, view->fltr.lst) {
    struct db_tbl_col *col = fltr->col;
    if (!fltr->enabled) continue;
    if (fltr->is_date) {
      dyn_fmt(sql, _sys, "%s strftime('%%s',%.*s) BETWEEN '%lld' AND '%lld'", pre,
              strf(col->name), fltr->tm.from_val, fltr->tm.to_val);
    } else {
      dyn_fmt(sql, _sys, "%s %.*s LIKE '%%%.*s%%'", pre, strf(col->name), strf(fltr->str));
    }
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
db_tbl_row_cnt(struct db_tbl_view *view, struct sys *_sys, sqlite3 *con) {
  assert(con);
  assert(_sys);
  assert(view);

  int ret = 0;
  scp_mem(view->tmp_mem, _sys) {
    sqlite3_stmt *stmt = 0;
    char *sql = db_tbl_sql(view, _sys, view->tmp_mem, strv("COUNT(*)"),0,0);
    sqlite3_prepare_v2(con, sql, -1, &stmt, 0);
    sqlite3_step(stmt);
    ret = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    dyn_free(sql, _sys);
  }
  return ret;
}
static void
db_tbl_view_setup(struct db_tbl_view *view, struct sys *_sys, sqlite3 *con,
                  int off, int lim) {
  assert(con);
  assert(view);
  if (view->data) {
    mem_scp_end(&view->scp, &view->mem, _sys);
  }
  scp_mem(view->tmp_mem, _sys) {
    char *sql_buf = 0;
    struct str sql = str_nil;
    sqlite3_stmt *stmt = 0;
    if (dyn_cnt(view->fltr.lst)) {
      /* select filtered content */
      view->row_cnt = db_tbl_row_cnt(view, _sys, con);
      sql_buf = db_tbl_sql(view, _sys, view->tmp_mem, str0("*"), off, lim);
      sql = dyn_str(sql_buf);
    } else {
      /* select everything */
      view->row_cnt = view->total;
      const char *fmt = "SELECT * FROM %.*s LIMIT %d, %d;";
      sql = arena_fmt(view->tmp_mem, _sys, fmt, strf(view->name), off, lim);
    }
    view->row_begin = clamp(0, off, view->row_cnt);
    view->row_end = min(off + lim, view->row_cnt);
    view->row_begin = max(0, view->row_end - lim);
    mem_scp_begin(&view->scp, &view->mem);

    int num = dyn_cnt(view->cols) * lim;
    view->data = arena_arr(&view->mem, _sys, struct str, num);
    sqlite3_prepare_v2(con, sql.str, -1, &stmt, 0);

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      for dyn_loopi(col, view->cols) {
        assert(col < dyn_cnt(view->cols));
        if (!view->cols[col].blob) {
          const char *dat = (const char*)sqlite3_column_text(stmt, col);
          if (dat) {
            assert(i < num);
            int len = sqlite3_column_bytes(stmt, col);
            view->data[i] = arena_str(&view->mem, _sys, str(dat, len));
          }
        }
        i++;
      }
    }
    sqlite3_finalize(stmt);
    if (dyn_cnt(view->fltr.lst)) {
      dyn_free(sql_buf, _sys);
    }
  }
}
static int
db_tbl_setup(struct db_tbl_view *view, struct sys *_sys, sqlite3 *con,
             struct gui_ctx *ctx, struct arena *tmp_mem,
             struct str id, enum db_tbl_lst_elm_type kind) {
  assert(con);
  assert(ctx);
  assert(mem);
  assert(view);
  assert(tmp_mem);

  view->kind = kind;
  view->tmp_mem = tmp_mem;
  view->cols = arena_dyn(&view->mem, _sys, struct db_tbl_col, 128);
  view->name = arena_str(&view->mem, _sys, id);
  view->fltr.lst = arena_dyn(&view->mem, _sys, struct db_tbl_fltr, 64);
  view->fltr.buf = arena_dyn(&view->mem, _sys, char, MAX_FILTER);

  struct mem_scp scp = {0};
  mem_scp_begin(&scp, tmp_mem);
  view->cols = db_tbl_qry_cols(con, view, _sys, id, view->cols, &view->mem, tmp_mem);

  /* generate table state */
  struct gui_split_lay bld = {0};
  int word_cnt = GUI_TBL_CAP(dyn_cnt(view->cols));
  view->tbl.state = arena_arr(&view->mem, _sys, int, word_cnt);
  gui.splt.lay.begin(&bld, view->tbl.state, dyn_cnt(view->cols), ctx->cfg.sep);
  for dyn_loopi(i, view->cols) {
    assert(i < dyn_cnt(view->cols));
    static const int cons[2] = {100, 600};
    gui.splt.lay.add(&bld, GUI_LAY_SLOT_DYN, 1, cons);
  }
  gui.splt.lay.end(&bld);

  /* retrieve total table row count */
  sqlite3_stmt *stmt = 0;
  struct str sql = arena_fmt(tmp_mem, _sys, "SELECT COUNT(*) FROM %.*s;", strf(id));
  sqlite3_prepare_v2(con, sql.str, -1, &stmt, 0);
  sqlite3_step(stmt);
  view->total = sqlite3_column_int(stmt, 0);
  view->row_cnt = view->total;
  sqlite3_finalize(stmt);
  mem_scp_end(&scp, tmp_mem, _sys);
  view->rev = (unsigned)-1;

  /* setup filter list table */
  struct gui_split_lay_cfg tbl_cfg = {0};
  tbl_cfg.size = szof(struct db_tbl_col_def);
  tbl_cfg.off = offsetof(struct db_tbl_col_def, ui);
  tbl_cfg.slots = db_tbl_fltr_def;
  tbl_cfg.cnt = DB_TBL_FLTR_MAX;
  gui.tbl.lay(view->fltr.tbl.state, ctx, &tbl_cfg);

  /* setup filter column table */
  struct gui_split_lay_cfg col_cfg = {0};
  col_cfg.size = szof(struct db_tbl_col_def);
  col_cfg.off = offsetof(struct db_tbl_col_def, ui);
  col_cfg.slots = db_tbl_fltr_col_def;
  col_cfg.cnt = DB_TBL_FLTR_COL_MAX;
  gui.tbl.lay(view->fltr.tbl_col.state, ctx, &col_cfg);
  return 0;
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
  for dyn_loop(col, view->cols) {
    if (!col->pk) continue;
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
  scp_mem(view->tmp_mem, _sys) {
    sqlite3_stmt *stmt = 0;
    struct str sql = arena_fmt(view->tmp_mem, _sys,
      "SELECT %.*s FROM %.*s WHERE %.*s = %.*s;", strf(blob_col), strf(tbl),
      strf(pk_col), strf(pk_key));
    sqlite3_prepare_v2(con, sql.str, -1, &stmt, 0);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const void *mem = sqlite3_column_blob(stmt, 0);
      int siz = sqlite3_column_bytes(stmt, 0);
      if (mem && siz) {
        view->state = TBL_VIEW_BLOB_VIEW;
        mem_scp_begin(&blob->scp, &view->mem);
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
struct db_fltr_arg {
  struct str tbl;
  struct str col;
  struct str match;
  int off, lim;
};
static void
db_tbl_view_fltr_init(struct db_tbl_fltr_view *view, struct db_tbl_fltr *fltr,
                      struct sys *_sys, struct arena *mem, sqlite3 *con,
                      struct db_fltr_arg *arg) {
  assert(con);
  assert(mem);
  assert(view);
  assert(fltr);

  view->row_cnt = 0;
  view->row_begin = arg->off;
  view->row_end = arg->off + arg->lim;

  if (view->data) {
    mem_scp_end(&view->scp, mem, _sys);
    fltr->is_date = 0;
    fltr->date_init = 0;
  }
  int has_match = arg->match.len > 0;

  /* retrieve total table row count */
  struct str sql;
  sqlite3_stmt *stmt;
  scp_mem(mem, _sys) {
    if (has_match) {
      sql = arena_fmt(mem, _sys, "SELECT COUNT(%.*s) FROM %.*s WHERE %.*s LIKE '%%%.*s%%';",
                      strf(arg->col), strf(arg->tbl), strf(arg->col), strf(arg->match));
      sqlite3_prepare_v2(con, sql.str, -1, &stmt, 0);
      sqlite3_step(stmt);
      view->row_cnt = sqlite3_column_int(stmt, 0);
      sqlite3_finalize(stmt);
    } else {
      sql = arena_fmt(mem, _sys, "SELECT COUNT(%.*s) FROM %.*s;",
                      strf(arg->col), strf(arg->tbl));
      sqlite3_prepare_v2(con, sql.str, -1, &stmt, 0);
      sqlite3_step(stmt);
      view->row_cnt = sqlite3_column_int(stmt, 0);
      sqlite3_finalize(stmt);
    }
  }
  stmt = 0;

  /* retrieve data window */
  int i = 0;
  mem_scp_begin(&view->scp, mem);
  view->data = arena_arr(mem, _sys, struct str, arg->lim);
  if (has_match) {
    sql = arena_fmt(mem, _sys, "SELECT %.*s FROM %.*s WHERE %.*s LIKE '%%%.*s%%'"
                    "LIMIT %d, %d;", strf(arg->col), strf(arg->tbl), strf(arg->col),
                    strf(arg->match), arg->off, arg->lim);
  } else {
    sql = arena_fmt(mem, _sys, "SELECT %.*s FROM %.*s LIMIT %d, %d;",
                    strf(arg->col), strf(arg->tbl), arg->off, arg->lim);
  }
  sqlite3_prepare_v2(con, sql.str, -1, &stmt, 0);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *dat = (const char*)sqlite3_column_text(stmt, 0);
    if (dat){
      int len = sqlite3_column_bytes(stmt, 0);
      view->data[i++] = arena_str(mem, _sys, str(dat, len));
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

  struct mem_scp scp;
  scp__mem(mem, &scp, _sys) {
    /* retrieve time range */
    sqlite3_stmt *stmt = 0;
    struct str sql = arena_fmt(mem, _sys, "SELECT MIN(strftime('%%s',%.*s)),"
      "MAX(strftime('%%s',%.*s)) FROM %.*s;", strf(col), strf(col), strf(tbl));
    sqlite3_prepare_v2(con, sql.str, -1, &stmt, 0);
    sqlite3_step(stmt);

    const char *min = (const char*)sqlite3_column_text(stmt, 0);
    const char *max = (const char*)sqlite3_column_text(stmt, 1);
    if (!min || !max) {
      fltr->hide_date = 1;
      fltr->is_date = 0;
      mem_scp_end(&scp, mem, _sys);
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
struct db_fltr_time_arg {
  struct str tbl;
  struct str col;
  int off, lim;
};
static void
db_tbl_view_fltr_tm_init(struct db_tbl_fltr_view *view, struct db_tbl_fltr *fltr,
                         struct sys *_sys, struct arena *mem, sqlite3 *con,
                         struct db_fltr_time_arg *p) {
  assert(con);
  assert(mem);
  assert(view);
  assert(fltr);

  view->row_cnt = 0;
  view->row_begin = p->off;
  view->row_end = p->off + p->lim;
  if (view->data) {
    mem_scp_end(&view->scp, mem, _sys);
    fltr->is_date = 1;
  }
  scp_mem(mem, _sys) {
    /* retrieve total count */
    sqlite3_stmt *stmt = 0;
    fltr->tm.from_val = mktime(&fltr->tm.from);
    fltr->tm.to_val = mktime(&fltr->tm.to);
    struct str sql = arena_fmt(mem, _sys, "SELECT COUNT(%.*s) FROM %.*s WHERE"
      "strftime('%%s',%.*s) BETWEEN '%lld'" "AND '%lld';",
      strf(p->col), strf(p->tbl), strf(p->tbl), strf(p->col),
      fltr->tm.from_val, fltr->tm.to_val);
    sqlite3_prepare_v2(con, sql.str, -1, &stmt, 0);
    sqlite3_step(stmt);
    view->row_cnt = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
  }

  /* retrieve data window */
  int i = 0;
  mem_scp_begin(&view->scp, mem);
  view->data = arena_arr(mem, _sys, struct str, p->lim);
  struct str sql = arena_fmt(mem, _sys, "SELECT %.*s FROM %.*s WHERE"
    "strftime('%%s',%.*s) BETWEEN '%lld' AND '%lld' LIMIT %d, %d;",
    strf(p->col), strf(p->tbl), strf(p->col), fltr->tm.from_val,
    fltr->tm.to_val, p->off, p->lim);

  sqlite3_stmt *stmt = 0;
  sqlite3_prepare_v2(con, sql.str, -1, &stmt, 0);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *dat = (const char*)sqlite3_column_text(stmt, 0);
    if (dat) {
      int len = sqlite3_column_bytes(stmt, 0);
      view->data[i++] = arena_str(mem, _sys, str(dat, len));
    }
  }
  sqlite3_finalize(stmt);
}
static void
db_tbl_fltr_clr(struct db_tbl_view *view, struct db_tbl_fltr_view *fltr,
                struct sys *_sys) {
  assert(view);
  assert(fltr);

  dyn_clr(fltr->buf);
  if (fltr->data) {
    mem_scp_end(&fltr->scp, &view->mem, _sys);
    fltr->data = 0;
  }
  fltr->row_cnt = 0;
  fltr->row_begin = 0;
  fltr->row_end = 0;
  fltr->off[0] = fltr->off[1] = 0.0;
  memset(&fltr->ini, 0, sizeof(fltr->ini));
}
static int*
bitmsk_idx(struct sys *_sys, struct arena *mem, unsigned long *bitset,
            int cnt, int total) {
  assert(mem);
  assert(bitset);

  int *ret = arena_arr(mem, _sys, int, cnt);
  for loop(i, cnt) {
    int idx = bit_ffs(bitset, total, 0);
    ret[i] = idx;
    bit_clr(bitset, idx);
  }
  return ret;
}
static void
db_tab_open(struct db_ui_view *ui, struct db_tbl_view *v, struct sys *_sys,
            struct gui_ctx *ctx, struct db_tree_node **elms, int cnt) {
  assert(v);
  assert(ui);
  assert(ctx);
  assert(elms);

  int i = 0;
  if (cnt && v) {
    for (; i < cnt; ++i) {
      const struct db_tree_node *n = elms[i];
      if (n->kind != DB_TBL_LST_ELM_TBL &&
          n->kind != DB_TBL_LST_ELM_VIEW) {
        continue;
      }
      /* open first table in place */
      db_tbl_setup(v, _sys, ui->con, ctx, ui->tmp_mem, n->name, n->kind);
      v->state = TBL_VIEW_DISPLAY;
      i++;
      break;
    }
  }
  for (; i < cnt; ++i) {
    /* open each following table in new tab */
    const struct db_tree_node *e = elms[i];
    if (e->kind != DB_TBL_LST_ELM_TBL &&
        e->kind != DB_TBL_LST_ELM_VIEW) {
      continue;
    }
    struct db_tbl_view *n = db_tbl_view_new(ui, _sys);
    db_tbl_setup(n, _sys, ui->con, ctx, ui->tmp_mem, e->name, e->kind);
    n->state = TBL_VIEW_DISPLAY;
    dyn_add(ui->tbls, _sys, n);
  }
}
static int
db_tbl_fltrs_enabled(struct db_tbl_fltr_view *fltr) {
  assert(fltr);
  struct db_tbl_fltr *elm = 0;
  for dyn_loop(elm, fltr->lst) {
    if (!elm->enabled) {
      return 0;
    }
  }
  return 1;
}
static int
db_init(struct mem_blk *blk) {
  int rc = sqlite3_config(SQLITE_CONFIG_HEAP, blk->base, blk->size, 64);
  return rc == 0;
}
static struct db_ui_view*
db_setup(struct gui_ctx *ctx, struct arena *mem, struct arena *tmp_mem,
         struct str path) {
  assert(db);
  assert(ctx);
  assert(mem);
  assert(tmp_mem);

  struct mem_scp scp = {0};
  mem_scp_begin(&scp, mem);
  struct db_ui_view *d = arena_obj(mem, ctx->sys, struct db_ui_view);
  d->path = arena_str(mem, ctx->sys, path);

  int rc = sqlite3_open(d->path.str, &d->con);
  if (rc != SQLITE_OK) {
    mem_scp_end(&scp, mem, ctx->sys);
    return 0;
  }
  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(d->con, "SELECT type, name, sql FROM sqlite_master;", -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    goto fail;
  }
  int col_cnt = sqlite3_column_count(stmt);
  assert(col_cnt == 3);
  if (col_cnt != 3) goto fail;

  d->tree_rev = 0;
  d->tmp_mem = tmp_mem;
  lst_init(&d->del_lst);
  d->tbls = arena_dyn(&d->mem, ctx->sys, struct db_tbl_view*, 32);

  db_tree_begin(&d->tree, ctx->sys, &d->mem);
  db_graph_begin(&d->graph, ctx->sys, &d->mem);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *tbl_type = (const char*)sqlite3_column_text(stmt, 0);
    const char *tbl_name = (const char*)sqlite3_column_text(stmt, 1);
    const char *tbl_sql = (const char*)sqlite3_column_text(stmt, 2);

    int tbl_type_len = sqlite3_column_bytes(stmt, 0);
    int tbl_name_len = sqlite3_column_bytes(stmt, 1);
    int tbl_sql_len = sqlite3_column_bytes(stmt, 2);

    struct str name = str(tbl_name, tbl_name_len);
    struct str type = str(tbl_type, tbl_type_len);
    struct str sql = str(tbl_sql, tbl_sql_len);

    /* add tree view node */
    struct db_tree_node *node = 0;
    enum db_tbl_lst_elm_type kind = DB_TBL_LST_ELM_UNKNOWN;
    if (!strcmp(tbl_type, "table")) {
      node = d->tree.tbls;
      kind = DB_TBL_LST_ELM_TBL;
    } else if (!strcmp(tbl_type, "index")) {
      node = d->tree.idxs;
      kind = DB_TBL_LST_ELM_IDX;
    } else if (!strcmp(tbl_type, "view")) {
      node = d->tree.views;
      kind = DB_TBL_LST_ELM_VIEW;
    } else if (!strcmp(tbl_type, "trigger")) {
      node = d->tree.trigs;
      kind = DB_TBL_LST_ELM_TRIGGER;
    }
    if (node) {
      struct db_tree_node_arg arg = {0};
      arg.name = name;
      arg.type = type;
      arg.sql = sql;
      arg.kind = kind;
      arg.id = fnv1a64(name.str, name.len, node->id);
      db_tree_node_new(ctx->sys, &d->mem, node, &arg);
    }
    /* add graph view node */
    if (!strcmp(tbl_type, "table")) {
      struct db_graph_node gn = {0};
      gn.name = arena_str(&d->mem, ctx->sys, name);
      gn.id = str_hash(name);
      dyn_add(d->graph.nodes, ctx->sys, gn);
    }
  }
  sqlite3_finalize(stmt);
  db_graph_end(d, &d->graph, ctx, mem, d->tmp_mem);
  db_tree_end(d, &d->tree, mem, ctx);

  struct db_tbl_view *view = db_tbl_view_new(d, ctx->sys);
  d->sel_tbl = dyn_cnt(d->tbls);
  dyn_add(d->tbls, ctx->sys, view);
  return d;

fail:
  mem_scp_end(&scp, mem, ctx->sys);
  sqlite3_close(d->con);
  d->con = 0;
  return 0;
}
static void
db_update(struct db_ui_view *d, struct sys *_sys) {
  assert(d);
  assert(_sys);

  if (d->tree_rev != d->tree.rev) {
    db_tree_update(&d->tree, _sys);
    d->tree.rev = d->tree_rev;
  }
}
static void
db_tbl_view_free(struct db_tbl_view *tbl, struct sys *_sys) {
  assert(tbl);
  assert(_sys);

  dyn_free(tbl->fltr.lst, _sys);
  dyn_free(tbl->cols, _sys);
  arena_free(&tbl->mem, _sys);
}
static void
db_free(struct db_ui_view *d, struct sys *_sys) {
  assert(d);
  assert(_sys);

  /* cleanup view list */
  struct db_tbl_view **tbl = 0;
  for dyn_loop(tbl, d->tbls) {
    db_tbl_view_free(*tbl, _sys);
  }
  /* cleanup view free list */
  struct lst_elm *elm = 0;
  for lst_loop(elm, &d->del_lst) {
    struct db_tbl_view *it = 0;
    it = lst_get(elm, struct db_tbl_view, hook);
    db_tbl_view_free(it, _sys);
  }
  /* cleanup view tree */
  dyn_free(d->tree.lst, _sys);
  dyn_free(d->tree.fnd_buf, _sys);
  set_free(d->tree.exp, _sys);
  tbl_free(d->tree.sel, _sys);

  /* cleanup view graph */
  struct db_graph_node *n = 0;
  for dyn_loop(n, d->graph.nodes) {
    dyn_free(n->cols, _sys);
  }
  dyn_free(d->graph.nodes, _sys);

  dyn_free(d->tbls, _sys);
  arena_free(&d->mem, _sys);
  if (d->con){
    sqlite3_close(d->con);
  }
}

/* ---------------------------------------------------------------------------
 *                                  GUI
 * ---------------------------------------------------------------------------
 */
static int
ui_btn_ico(struct gui_ctx *ctx, struct gui_btn *btn, struct gui_panel *parent,
            struct str txt, enum res_ico_id icon, int uline) {
  assert(ctx);
  assert(btn);
  assert(parent);
  assert(icon);

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
               struct gui_txt_ed *ed, char **buf) {
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
    edt->pan.focusable = 1;
    edt->pan.box.x = gui.bnd.min_max(ico.box.x.max, pan->box.x.max);
    edt->pan.box.x = gui.bnd.shrink(&edt->pan.box.x, pad[0]);
    edt->pan.box.y = gui.bnd.shrink(&pan->box.y, pad[1]);
    gui.edt.fld(ctx, edt, &edt->pan, pan, ed, buf);
  }
  gui.pan.end(ctx, pan, parent);
}
static enum res_ico_id
ui_db_tbl_lst_elm_ico(enum db_tbl_lst_elm_type type) {
  switch (type) {
  case DB_TBL_LST_ELM_TBL: return RES_ICO_TABLE;
  case DB_TBL_LST_ELM_VIEW: return RES_ICO_IMAGE;
  case DB_TBL_LST_ELM_IDX: return RES_ICO_TAG;
  case DB_TBL_LST_ELM_TRIGGER: return RES_ICO_BOLT;
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
    scp_gui_cfg_pushu(&gui,stk,&ctx->cfg.col[GUI_COL_ICO], fk_col) {
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
  gui.tbl.hdr.slot.end(ctx, tbl, lay, &slot, view->tbl.state);
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
    scp_gui_disable_on(&gui, ctx, col->blob) {
      if (gui.btn.ico(ctx, &fltr, &slot.pan, RES_ICO_SEARCH)) {
        view->state = TBL_VIEW_FILTER;
        view->fltr.ini.col = col;
      }
    }
    /* header label with foreign key icon */
    struct gui_cfg_stk stk[1] = {0};
    unsigned fk_col = ctx->cfg.col[GUI_COL_TXT_DISABLED];
    scp_gui_cfg_pushu(&gui,stk,&ctx->cfg.col[GUI_COL_ICO], fk_col) {
      struct gui_btn hdr = {.box = slot.pan.box};
      hdr.box.x = gui.bnd.min_max(slot.pan.box.x.min, fltr.box.x.min);
      ui_btn_ico_txt(ctx, &hdr, &slot.pan, col->name, RES_ICO_LINK);
    }
  }
  gui.tbl.hdr.slot.end(ctx, tbl, tbl_lay, &slot, view->tbl.state);
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
    scp_gui_disable_on(&gui, ctx, col->blob) {
      if (gui.btn.ico(ctx, &fltr, &slot.pan, RES_ICO_SEARCH)) {
        view->state = TBL_VIEW_FILTER;
        view->fltr.ini.col = col;
      }
    }
    struct gui_btn hdr = {.box = slot.pan.box};
    static const struct gui_align align = {GUI_HALIGN_LEFT, GUI_VALIGN_MID};
    hdr.box.x = gui.bnd.min_max(slot.pan.box.x.min, fltr.box.x.min);
    gui.btn.txt(ctx, &hdr, &slot.pan, col->name, &align);
  }
  gui.tbl.hdr.slot.end(ctx, tbl, tbl_lay, &slot, view->tbl.state);
}
static void
ui_db_tbl_view_lst_elm_blob(struct db_ui_view *sql, struct db_tbl_view *view,
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
    scp_gui_disable_on(&gui, ctx, 1) {
      struct gui_panel lbl = {.box = lay};
      gui.txt.lbl(ctx, &lbl, col, strv("blob"), 0);
    }
  } else {
    /* show dummy text for blob data */
    scp_gui_disable_on(&gui, ctx, 1) {
      struct gui_panel lbl = {.box = col->box};
      gui.txt.lbl(ctx, &lbl, col, strv("blob"), 0);
    }
  }
}
static void
ui_db_tbl_view_lst_elm(struct db_ui_view *sql, struct db_tbl_view *view,
                       struct gui_ctx *ctx, struct gui_tbl *tbl,
                       struct gui_panel *elm, const int *tbl_cols,
                       const struct str *data, unsigned long long id){
  assert(sql);
  assert(ctx);
  assert(tbl);
  assert(elm);
  assert(data);
  assert(view);
  assert(tbl_cols);

  gui.tbl.lst.elm.begin(ctx, tbl, elm, id, 0);
  for dyn_loopi(i, view->cols) {
    assert(i < cntof(view->cols));
    struct db_tbl_col *meta = &view->cols[i];
    if (data[i].len) {
      gui.tbl.lst.elm.col.txt(ctx, tbl, tbl_cols, elm, data[i], 0);
      continue;
    }
    struct gui_panel col = {0};
    gui.tbl.lst.elm.col.slot(&col.box, ctx, tbl, tbl_cols);
    gui.pan.begin(ctx, &col, elm);
    if (meta->blob) {
      ui_db_tbl_view_lst_elm_blob(sql, view, ctx, &col, meta, data);
    } else {
      scp_gui_disable_on(&gui, ctx, 1) {
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
  gui.tbl.hdr.begin(ctx, tbl, tbl_lay, view->tbl.state);
  for dyn_loop(col, view->cols) {
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
ui_db_tbl_view_lst(struct db_ui_view *sql, struct db_tbl_view *view,
                   struct gui_ctx *ctx, struct arena *tmp_mem,
                   struct gui_panel *pan, struct gui_panel *parent) {
  assert(sql);
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);
  assert(tmp_mem);

  scp_mem(tmp_mem, ctx->sys)
  {
    gui.pan.begin(ctx, pan, parent);
    {
      struct gui_tbl tbl = {.box = pan->box};
      gui.tbl.begin(ctx, &tbl, pan, view->tbl.off, &view->tbl.sort);
      {
        /* header */
        int word_cnt = GUI_TBL_COL(dyn_cnt(view->cols));
        int *tbl_lay = arena_arr(tmp_mem, ctx->sys, int, word_cnt);
        ui_db_tbl_view_lst_hdr(view, ctx, &tbl, tbl_lay, view->tbl.state);

        /* list */
        struct gui_tbl_lst_cfg cfg = {0};
        gui.tbl.lst.cfg(ctx, &cfg, view->row_cnt);
        gui.tbl.lst.begin(ctx, &tbl, &cfg);

        int mod = 0;
        mod |= tbl.lst.begin != view->row_begin;
        mod |= tbl.lst.end != view->row_end;
        mod |= view->rev != view->fltr.rev;
        if (mod) db_tbl_view_setup(view, ctx->sys, sql->con, tbl.lst.begin, tbl.lst.cnt);

        int idx = 0;
        for gui_tbl_lst_loop(i,gui,&tbl) {
          unsigned long long n = cast(unsigned long long, i);
          unsigned long long id = fnv1au64(n, FNV1A64_HASH_INITIAL);
          struct str *row_data = view->data + idx;

          struct gui_panel elm = {0};
          ui_db_tbl_view_lst_elm(sql, view, ctx, &tbl, &elm, tbl_lay, row_data, id);
          idx += dyn_cnt(view->cols);
        }
        gui.tbl.lst.end(ctx, &tbl);
      }
      gui.tbl.end(ctx, &tbl, pan, view->tbl.off);
    }
    gui.pan.end(ctx, pan, parent);
  }
}
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
ui_db_tbl_fltr_tbl_hdr(struct db_tbl_fltr_view *fltr, struct gui_tbl *tbl,
                       int *tbl_cols, struct gui_ctx *ctx) {
  assert(ctx);
  assert(tbl);
  assert(fltr);
  assert(tbl_cols);

  gui.tbl.hdr.begin(ctx, tbl, tbl_cols, fltr->tbl.state);
  {
    /* enable/disable all filters toggle */
    int all_on = db_tbl_fltrs_enabled(fltr);
    if (ui_db_tbl_fltr_lst_tog_slot(ctx, tbl, tbl_cols, fltr->tbl.state, all_on)) {
      for dyn_loopi(i, fltr->lst) {
        assert(i < dyn_cnt(fltr-lst));
        fltr->lst[i].enabled = !all_on;
      }
    }
    for (int i = 1; i + 1 < cntof(db_tbl_fltr_def); ++i) {
      const struct db_tbl_col_def *col = 0;
      col = &db_tbl_fltr_def[i];
      gui.tbl.hdr.slot.txt(ctx, tbl, tbl_cols, fltr->tbl.state, col->title);
    }
    /* delete all filters icon */
    struct gui_btn slot = {0};
    ui_db_tbl_fltr_lst_ico_slot(ctx, tbl, tbl_cols, &slot, fltr->tbl.state, RES_ICO_TRASH);
    if (slot.clk) {
      dyn_clr(fltr->lst);
    }
  }
  gui.tbl.hdr.end(ctx, tbl);
}
static void
ui_db_tbl_fltr_tog(struct db_tbl_fltr *item, struct gui_ctx *ctx,
                   struct gui_tbl *tbl, const int *tbl_lay, struct gui_panel *elm) {
  assert(ctx);
  assert(tbl);
  assert(elm);
  assert(item);
  assert(tbl_lay);

  int enabled = item->enabled;
  struct gui_icon tog = {0};
  gui.tbl.lst.elm.col.slot(&tog.box, ctx, tbl, tbl_lay);
  {
    gui.ico.clk(ctx, &tog, elm, enabled ? RES_ICO_TOGGLE_ON : RES_ICO_TOGGLE_OFF);
    if (tog.clk) {
      item->enabled = !item->enabled;
    }
  }
}
static void
ui_db_tbl_fltr_lst_view(struct db_tbl_view *view, struct gui_ctx *ctx,
                        struct gui_panel *pan, struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  struct db_tbl_fltr_view *fltr = &view->fltr;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, fltr->tbl.off, &fltr->tbl.sort);
    {
      /* header */
      int tbl_cols[GUI_TBL_COL(DB_TBL_FLTR_MAX)];
      ui_db_tbl_fltr_tbl_hdr(fltr, &tbl, tbl_cols, ctx);

      /* sorting */
      if (tbl.resort && dyn_cnt(fltr->lst)) {
        sort_f fn = db_tbl_fltr_def[tbl.sort.col].sort[tbl.sort.order];
        if (fn) {
          dyn_sort(fltr->lst, db_tbl_fltr_def[tbl.sort.col].sort[tbl.sort.order]);
          fltr->tbl.sort = tbl.sort;
        }
      }
      /* list */
      int del_idx = -1;
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, dyn_cnt(fltr->lst));
      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for gui_tbl_lst_loop(i,gui,&tbl) {
        struct gui_panel elm = {0};
        struct db_tbl_fltr *item = fltr->lst + i;
        gui.tbl.lst.elm.begin(ctx, &tbl, &elm, (uintptr_t)item, 0);
        {
          /* columns */
          ui_db_tbl_fltr_tog(item, ctx, &tbl, tbl_cols, &elm);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, item->str, 0);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, item->col->name, 0);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, item->col->type, 0);

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
        dyn_rm(fltr->lst, del_idx);
      }
    }
    gui.tbl.end(ctx, &tbl, pan, fltr->tbl.off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_tbl_fltr_col_view(struct db_tbl_view *view, struct gui_ctx *ctx,
                        struct gui_panel *pan, struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  struct db_tbl_fltr_view *fltr = &view->fltr;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, fltr->tbl_col.off, &fltr->tbl_col.sort);
    {
      /* header */
      const struct db_tbl_col_def *col = 0;
      int tbl_cols[GUI_TBL_COL(DB_TBL_FLTR_COL_MAX)];
      gui.tbl.hdr.begin(ctx, &tbl, tbl_cols, fltr->tbl_col.state);
      for arr_loopv(col, db_tbl_fltr_col_def) {
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_cols, fltr->tbl_col.state, col->title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, dyn_cnt(view->cols));
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for gui_tbl_lst_loop(i,gui,&tbl) {
        int sel = fltr->sel_col == i;

        struct gui_panel elm = {0};
        const struct db_tbl_col *item = view->cols + i;
        gui.tbl.lst.elm.begin(ctx, &tbl, &elm, (uintptr_t)item, sel);
        {
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, item->name, 0);
          gui.tbl.lst.elm.col.txt(ctx, &tbl, tbl_cols, &elm, item->type, 0);
        }
        gui.tbl.lst.elm.end(ctx, &tbl, &elm);
      }
      gui.tbl.lst.end(ctx, &tbl);
      if (tbl.lst.sel.mod) {
        fltr->sel_col = tbl.lst.sel.idx;
      }
    }
    gui.tbl.end(ctx, &tbl, pan, fltr->tbl_col.off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_tbl_fltr_str_view(struct db_ui_view *d, struct db_tbl_view *view,
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
    gui.edt.box(ctx, &edt, pan, &fltr->buf);
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
      struct db_fltr_arg arg = {0};
      arg.tbl = view->name;
      arg.col = fltr->ini.col->name;
      arg.match = str0(fltr->buf);
      arg.off = reg.lst.begin;
      arg.lim = reg.lst.cnt;
      db_tbl_view_fltr_init(fltr, &fltr->ini, ctx->sys, &view->mem, d->con, &arg);
    }
    int idx = 0;
    for gui_lst_reg_loop(i,gui,&reg) {
      unsigned long long n = cast(unsigned long long, i);
      unsigned long long id = fnv1au64(n, FNV1A64_HASH_INITIAL);

      struct gui_panel elm = {0};
      gui.lst.reg.elm.begin(ctx, &reg, &elm, id, 0);
      if (fltr->data[idx].len) {
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
ui_db_tbl_fltr_time_view(struct db_ui_view *d, struct db_tbl_view *view,
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
      struct db_fltr_time_arg arg = {0};
      arg.tbl = view->name;
      arg.col = fltr->ini.col->name;
      arg.off = reg.lst.begin;
      arg.lim = reg.lst.cnt;
      db_tbl_view_fltr_tm_init(fltr, &fltr->ini, ctx->sys, &view->mem, d->con, &arg);
    }
    int idx = 0;
    for gui_lst_reg_loop(i,gui,&reg) {
      struct gui_panel elm = {0};
      unsigned long long n = cast(unsigned long long, i);
      unsigned long long id = fnv1au64(n, FNV1A64_HASH_INITIAL);
      gui.lst.reg.elm.begin(ctx, &reg, &elm, id, 0);
      if (fltr->data[idx].len) {
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
ui_db_fltr_view_sel(struct db_ui_view *d, struct db_tbl_view *view,
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
      {
        gui.tab.hdr.slot.txt(ctx, &tab, &hdr, strv("Text"));
        gui.tab.hdr.slot.txt(ctx, &tab, &hdr, strv("Date & Time"));
      }
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
ui_db_fltr_view(struct db_ui_view *d, struct db_tbl_view *view,
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
static dyn(char)
ui_db_hex_view_gen_str(dyn(char) ln, struct sys *_sys, const unsigned char *mem,
                       int siz, int digit_cnt, int col_cnt, int addr) {
  dyn_fmt(ln, _sys, "%0*X: ", digit_cnt, addr);
  for (int col = 0; col < col_cnt && addr + col < siz; ++col) {
    /* generate hex value representation */
    unsigned char b = mem[addr + col];
    dyn_fmt(ln, _sys, "%.2X ", b);
  }
  if (addr + col_cnt > siz) {
    /* align up to ascii represenation */
    int cnt = addr + col_cnt - siz;
    for loop(i,cnt) {
      dyn_add(ln, _sys, ' ');
      dyn_add(ln, _sys, ' ');
      dyn_add(ln, _sys, ' ');
    }
  }
  dyn_add(ln, _sys, ' ');
  for (int col = 0; col < col_cnt && addr + col < siz; ++col) {
    /* generate ascii representation */
    unsigned char byte = mem[addr + col];
    char sym = (byte < 32 || byte >= 127) ? '.' : (char)byte;
    dyn_add(ln, _sys, sym);
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
      scp_mem(view->tmp_mem, ctx->sys) {
        /* generate and display each line of hex/ascii data representation */
        struct gui_panel elm = {0};
        char *ln = arena_dyn(view->tmp_mem, ctx->sys, char, KB(4));
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
        gui.tab.hdr.slot.txt(ctx, &tab, &hdr, strv("Hex"));
        if (blob->img.act) {
          gui.tab.hdr.slot.txt(ctx, &tab, &hdr, strv("Image"));
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
static void
ui_db_view_tree_node(struct gui_ctx *ctx, struct gui_tree_node *node,
                     struct gui_panel *parent,
                     const struct db_tree_view *t,
                     const struct db_tree_node *n){
  assert(ctx);
  assert(node);
  assert(parent);

  gui.tree.begin(ctx, node, parent, n->depth - 1);
  {
    struct gui_panel lbl = {.box = node->box};
    enum res_ico_id ico = db_tree_node_icon(t,n);
    gui.ico.box(ctx, &lbl, &node->pan, ico, n->name);
  }
  gui.tree.end(ctx, node, parent);
}
static int
ui_db_view_tree_elm_node_col(struct gui_ctx *ctx, struct db_tree_view *t,
                             const struct db_tree_node *n,
                             struct gui_panel *pan, struct gui_panel *parent) {
  assert(t);
  assert(n);
  assert(ctx);
  assert(pan);
  assert(parent);

  int ret = 0;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tree_node node = {0};
    node.type = lst_any(&n->sub) ? GUI_TREE_NODE : GUI_TREE_LEAF;
    node.open = set_fnd(t->exp, n->id);
    node.box = pan->box;

    ui_db_view_tree_node(ctx, &node, pan, t, n);
    if (node.changed) {
      if (node.open) {
        set_put(t->exp, ctx->sys, n->id);
      } else {
        set_del(t->exp, n->id);
      }
      ret = 1;
    }
  }
  gui.pan.end(ctx, pan, parent);
  return ret;
}
static int
ui_db_view_tree_elm(struct gui_ctx *ctx, struct db_tree_view *t,
                    const struct db_tree_node *n, struct gui_tbl *tbl,
                    const int *lay, struct gui_panel *elm, int is_sel) {
  assert(t);
  assert(n);
  assert(tbl);
  assert(ctx);
  assert(lay);
  assert(elm);

  int ret = 0;
  gui.tbl.lst.elm.begin(ctx, tbl, elm, n->id, is_sel);
  {
    struct gui_panel node = {0};
    gui.tbl.lst.elm.col.slot(&node.box, ctx, tbl, lay);
    ret = ui_db_view_tree_elm_node_col(ctx, t, n, &node, elm);
    gui.tbl.lst.elm.col.txt(ctx, tbl, lay, elm, n->type, 0);
    gui.tbl.lst.elm.col.txt(ctx, tbl, lay, elm, n->sql, 0);
  }
  gui.tbl.lst.elm.end(ctx, tbl, elm);
  return ret;
}
static void
ui_db_view_tree_sel(struct db_tree_view *t, struct gui_tbl *tbl,
                    struct gui_ctx *ctx) {
  assert(t);
  assert(tbl);
  assert(ctx);
  if (tbl->lst.sel.mut == GUI_LST_SEL_MOD_REPLACE) {
    tbl_clr(t->sel);
  }
  for (int i = tbl->lst.sel.begin_idx; i < tbl->lst.sel.end_idx; ++i) {
    assert(i < dyn_cnt(t->lst));
    struct db_tree_node *n = t->lst[i];
    switch (tbl->lst.sel.op) {
    case GUI_LST_SEL_OP_SET:
      tbl_put(t->sel, ctx->sys, n->id, &n);
      break;
    case GUI_LST_SEL_OP_CLR:
      tbl_del(t->sel, n->id);
      break;
    }
  }
}
static void
ui_db_view_tree(struct db_ui_view *d, struct db_tree_view *t,
                struct gui_ctx *ctx, struct gui_panel *pan,
                struct gui_panel *parent) {
  assert(t);
  assert(db);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    int gap = ctx->cfg.gap[1];
    struct gui_box lay = pan->box;
    struct gui_panel fltr = {.box = gui.cut.top(&lay, ctx->cfg.item, gap)};
    struct gui_edit_box edt = {.box = fltr.box};
    ui_edit_fnd(ctx, &edt, &fltr, pan, &d->tree.fnd_ed, &d->tree.fnd_buf);

    struct gui_tbl tbl = {.box = lay};
    gui.tbl.begin(ctx, &tbl, pan, t->tbl.off, &t->tbl.sort);
    {
      /* header */
      const struct db_tbl_col_def *col = 0;
      int tbl_cols[GUI_TBL_COL(DB_TREE_COL_MAX)];
      gui.tbl.hdr.begin(ctx, &tbl, tbl_cols, t->tbl.state);
      for arr_loopv(col, db_tree_col_def) {
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_cols, t->tbl.state, col->title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, dyn_cnt(t->lst));
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_MULTI;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for gui_tbl_lst_loop(i,gui,&tbl) {
        struct gui_panel elm = {0};
        const struct db_tree_node *n = t->lst[i];
        int is_sel = tbl_has(t->sel, n->id);
        if (ui_db_view_tree_elm(ctx, t, n, &tbl, tbl_cols, &elm, is_sel)) {
          d->tree_rev++;
        }
      }
      gui.tbl.lst.end(ctx, &tbl);
      if (tbl.lst.sel.mod) {
        ui_db_view_tree_sel(t, &tbl, ctx);
      }
    }
    gui.tbl.end(ctx, &tbl, pan, t->tbl.off);
  }
  gui.pan.end(ctx, pan, parent);
}
static enum res_ico_id
ui_db_view_graph_node_attr_ico(const struct db_tbl_col *col) {
  assert(col);
  if (col->pk) {
    return RES_ICO_KEY;
  } else if (col->fk) {
    return RES_ICO_LINK;
  } else if (str_eq(col->type, strv("REAL")) ||
      str_eq(col->type, strv("DOUBLE")) ||
      str_eq(col->type, strv("DOUBLE PRECISION")) ||
      str_eq(col->type, strv("FLOAT"))) {
    return RES_ICO_MODIFY;
  } else if (str_eq(col->type, strv("DATE"))) {
    return RES_ICO_CALENDAR;
  } else if (str_eq(col->type, strv("DATETIME"))) {
    return RES_ICO_CALENDAR;
  } else if (str_eq(col->type, strv("BLOB"))) {
    return RES_ICO_CUBE;
  } else if (str_eq(col->type, strv("BOOLEAN"))) {
    return RES_ICO_CHECK;
  } else if (str_eq(col->type, strv("INT")) ||
      str_eq(col->type, strv("INTEGER")) ||
      str_eq(col->type, strv("TINYINT")) ||
      str_eq(col->type, strv("SMALLINT")) ||
      str_eq(col->type, strv("MEDIUMINT")) ||
      str_eq(col->type, strv("BIGINT")) ||
      str_eq(col->type, strv("UNSIGNED BIG INT")) ||
      str_eq(col->type, strv("INT2")) ||
      str_eq(col->type, strv("INT8")) ||
      str_eq(col->type, strv("NUMERIC"))) {
    return RES_ICO_MODIFY;
  } else {
    return RES_ICO_FONT;
  }
}
static void
ui_db_view_graph_node(struct gui_ctx *ctx, struct db_graph_node *n,
                      struct gui_panel *parent) {
  assert(n);
  assert(ctx);
  assert(parent);

  struct gui_graph_node node = {.box = n->box};
  gui.graph_node.begin(ctx, &node, parent);
  {
    /* header */
    struct gui_graph_node_hdr hdr = {0};
    gui.graph_node.item(&hdr.box, ctx, &node, 0);
    gui.graph_node.hdr.begin(ctx, &node, &hdr);
    {
      struct gui_panel lbl = {.box = hdr.content};
      gui.ico.box(ctx, &lbl, &hdr.pan, RES_ICO_TABLE, n->name);
    }
    gui.graph_node.hdr.end(ctx, &node, &hdr);
    if (hdr.moved) {
      n->box = gui.box.posv(&n->box, hdr.pos);
    }
    /* body */
    const struct db_tbl_col *col = 0;
    for dyn_loop(col,n->cols) {
      struct gui_panel lbl = {0};
      enum res_ico_id ico = ui_db_view_graph_node_attr_ico(col);
      gui.graph_node.item(&lbl.box, ctx, &node, 0);
      gui.ico.box(ctx, &lbl, &node.pan, ico, col->name);
    }
  }
  gui.graph_node.end(ctx, &node, parent);
}
static void
ui_db_view_graph(struct db_graph *g, struct gui_ctx *ctx,
                 struct gui_panel *pan, struct gui_panel *parent) {
  assert(g);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_grid grid = {.box = pan->box};
    gui.grid.begin(ctx, &grid, pan, g->off);
    struct db_graph_node *n = 0;
    for dyn_loop(n, g->nodes) {
      ui_db_view_graph_node(ctx, n, &grid.pan);
    }
    gui.grid.end(ctx, &grid, pan, g->off);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_view_tab(struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                   struct gui_tab_ctl_hdr *hdr, struct str title,
                   enum res_ico_id ico) {
  assert(ctx);
  assert(tab);
  assert(hdr);
  assert(ico);

  struct gui_panel slot = {0};
  gui.tab.hdr.slot.begin(ctx, tab, hdr, &slot, str_hash(title));
  gui.ico.box(ctx, &slot, &hdr->pan, ico, title);
  gui.tab.hdr.slot.end(ctx, tab, hdr, &slot, 0);
}
static void
ui_db_open_sel(struct db_ui_view *ui, struct db_tbl_view *view,
               struct gui_ctx *ctx) {
  scp_mem(ui->tmp_mem, ctx->sys) {
    struct db_tree_node **lst = 0;
    int cnt = tbl_cnt(ui->tree.sel);
    lst = arena_arr(ui->tmp_mem, ctx->sys, struct db_tree_node*, cnt);
    for tbl_loop(slot, idx, ui->tree.sel) {
      lst[idx] = ui->tree.sel[slot];
    }
    db_tab_open(ui, view, ctx->sys, ctx, lst, cnt);

    struct db_tbl_view *new_view = db_tbl_view_new(ui, ctx->sys);
    dyn_add(ui->tbls, ctx->sys, new_view);
  }
}
static void
ui_db_view(struct db_ui_view *d, struct gui_ctx *ctx, struct gui_panel *pan,
           struct gui_panel *parent) {
  assert(db);
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box};
    gui.tab.begin(ctx, &tab, pan, DB_VIEW_CNT, casti(d->state));
    {
      /* tab header */
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      {
        ui_db_view_tab(ctx, &tab, &hdr, strv("Tree"), RES_ICO_BOX_LIST);
        ui_db_view_tab(ctx, &tab, &hdr, strv("Graph"), RES_ICO_GRAPH);
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);
      if (tab.sel.mod) {
        d->state = cast(enum db_view_state, tab.sel.idx);
      }
      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      switch (d->state) {
      case DB_VIEW_TREE:
        ui_db_view_tree(d, &d->tree, ctx, &bdy, pan);
        break;
      case DB_VIEW_GRAPH:
        ui_db_view_graph(&d->graph, ctx, &bdy, pan);
        break;
      }
    }
    gui.tab.end(ctx, &tab, pan);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_db_main(struct db_ui_view *ui, struct db_tbl_view *view, struct gui_ctx *ctx,
           struct gui_panel *pan, struct gui_panel *parent) {
  assert(ui);
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    int gap = ctx->cfg.gap[1];
    struct gui_box lay = pan->box;
    switch (view->state) {
    case TBL_VIEW_SELECT: {
      struct gui_btn open = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      struct gui_panel overview = {.box = lay};
      ui_db_view(ui, ctx, &overview, pan);

      /* open tables */
      int dis = tbl_empty(&ui->tree.sel);
      scp_gui_disable_on(&gui, ctx, dis) {
        if (ui_btn_ico(ctx, &open, pan, strv("Open"), RES_ICO_TH_LIST, 0)) {
          ui_db_open_sel(ui, view, ctx);
          tbl_clr(ui->tree.sel);
        }
      }
    } break;

    case TBL_VIEW_DISPLAY: {
      /* layout clear button */
      struct gui_btn clr = {0};
      clr.box.x = gui.bnd.max_ext(pan->box.x.max, ctx->cfg.item);
      clr.box.y = gui.bnd.max_ext(pan->box.y.max, ctx->cfg.item);
      {
        int dis = !dyn_cnt(view->fltr.lst);
        scp_gui_disable_on(&gui, ctx, dis) {
          if (gui.btn.ico(ctx, &clr, pan, RES_ICO_TRASH)) {
            dyn_clr(view->fltr.lst);
          }
        }
      }
      /* jump to filter view button */
      struct gui_btn fltr = {.box = clr.box};
      fltr.box.x = gui.bnd.min_max(pan->box.x.min, clr.box.x.min);
      if (ui_btn_ico(ctx, &fltr, pan, strv("Filters"), RES_ICO_MENU, 0)) {
        view->state = TBL_VIEW_FILTER_LIST;
      }
      /* table view */
      struct gui_panel lst = {.box = pan->box};
      lst.box.y = gui.bnd.min_max(pan->box.y.min, fltr.box.y.min - ctx->cfg.pad[1]);
      ui_db_tbl_view_lst(ui, view, ctx, ui->tmp_mem, &lst, pan);
    } break;

    case TBL_VIEW_FILTER_LIST: {
      /* back button */
      struct gui_btn back = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      if (ui_btn_ico(ctx, &back, pan, strv("Back"), RES_ICO_TH_LIST, 0)) {
        view->state = TBL_VIEW_DISPLAY;
      }
      /* add button */
      struct gui_btn add = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      if (ui_btn_ico(ctx, &add, pan, strv("Add"), RES_ICO_PLUS,0)) {
        view->state = TBL_VIEW_FILTER_COL_SEL;
        memset(&view->fltr.ini, 0, sizeof(view->fltr.ini));
        view->fltr.sel_col = -1;
      }
      /* filter list view */
      struct gui_panel lst = {.box = lay};
      ui_db_tbl_fltr_lst_view(view, ctx, &lst, pan);
    } break;

    case TBL_VIEW_FILTER_COL_SEL: {
      /* back button */
      struct gui_btn back = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      if (ui_btn_ico(ctx, &back, pan, strv("Back"), RES_ICO_MENU,0)) {
        view->state = TBL_VIEW_FILTER_LIST;
      }
      /* select button */
      int dis = view->fltr.sel_col < 0;
      dis = !dis ? view->cols[view->fltr.sel_col].blob : dis;
      scp_gui_disable_on(&gui, ctx, dis) {
        struct gui_btn sel = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
        if (ui_btn_ico(ctx, &sel, pan, strv("Select"), RES_ICO_CHECK,0)) {
          view->fltr.ini.col = &view->cols[view->fltr.sel_col];
          view->state = TBL_VIEW_FILTER;
        }
      }
      /* filter column select view */
      struct gui_panel lst = {.box = lay};
      ui_db_tbl_fltr_col_view(view, ctx, &lst, pan);
    } break;

    case TBL_VIEW_FILTER: {
      struct gui_btn back = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      struct gui_btn apply = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      struct gui_panel fltr = {.box = lay};
      ui_db_fltr_view(ui, view, ctx, &fltr, pan);

      /* back button */
      if (ui_btn_ico(ctx, &back, pan, strv("Back"), RES_ICO_MENU, 0)) {
        view->state = TBL_VIEW_FILTER_LIST;
        db_tbl_fltr_clr(view, &view->fltr, ctx->sys);
        db_tbl_view_clr(view);
      }
      /* apply button */
      int dis = !view->fltr.ini.is_date && dyn_empty(view->fltr.buf);
      scp_gui_disable_on(&gui, ctx, dis) {
        if (ui_btn_ico(ctx, &apply, pan, strv("Apply"), RES_ICO_CHECK, 0)) {
          view->fltr.ini.enabled = 1;
          view->fltr.ini.str = arena_str(&view->mem, ctx->sys, dyn_str(view->fltr.buf));
          dyn_add(view->fltr.lst, ctx->sys, view->fltr.ini);

          db_tbl_fltr_clr(view, &view->fltr, ctx->sys);
          db_tbl_view_clr(view);

          view->state = TBL_VIEW_DISPLAY;
          view->fltr.rev++;
        }
      }
    } break;

    case TBL_VIEW_BLOB_VIEW: {
      /* back button */
      struct gui_btn back = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
      if (ui_btn_ico(ctx, &back, pan, strv("Back"), RES_ICO_TH_LIST, 0)) {
        if (view->blob.img.act) {
          struct sys *s = ctx->sys;
          s->gfx.tex.del(s, view->blob.img.id);
          mset(&view->blob.img, 0, szof(view->blob.img));
        }
        mem_scp_end(&view->blob.scp, &view->mem, ctx->sys);
        view->state = TBL_VIEW_DISPLAY;
      }
      struct gui_panel hex = {.box = lay};
      ui_db_blob_view(view, &view->blob, ctx, &hex, pan);
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
ui_db_explr_tab_slot(struct db_ui_view *d, struct db_tbl_view *tbl,
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
  if (dyn_cnt(d->tbls) > 1 && tab->idx == tab->sel.idx) {
    ret = ui_db_explr_tab_slot_close(ctx, slot, &hdr->pan, title, ico);
  } else {
    gui.ico.box(ctx, slot, &hdr->pan, ico, title);
  }
  gui.tab.hdr.slot.end(ctx, tab, hdr, slot, 0);
  return ret;
}
static int
ui_db_explr_tab(struct db_ui_view *d, struct db_tbl_view *tbl,
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
ui_db_tab_view_lst(struct db_ui_view *dbv, struct gui_ctx *ctx,
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
    gui.lst.cfg(&cfg, dyn_cnt(dbv->tbls), dbv->tbl_lst_off[1]);
    cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
    cfg.sel.on = GUI_LST_SEL_ON_HOV;

    struct gui_lst_reg reg = {.box = pan->box};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, dbv->tbl_lst_off);
    for gui_lst_reg_loop(i,gui,&reg) {
      struct db_tbl_view *tbl = dbv->tbls[i];
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
    gui.lst.reg.end(ctx, &reg, pan, dbv->tbl_lst_off);
  }
  gui.pan.end(ctx, pan, parent);
  return ret;
}
static void
ui_db_resort_tbls(struct db_ui_view *d, int dst_idx, int src_idx) {
  assert(dst_idx < dyn_cnt(d->tbls));
  assert(src_idx < dyn_cnt(d->tbls));
  assert(db);

  struct db_tbl_view *dst = d->tbls[dst_idx];
  struct db_tbl_view *src = d->tbls[src_idx];

  d->tbls[dst_idx] = src;
  d->tbls[src_idx] = dst;
}
static void
ui_db_explr(struct db_ui_view *d, struct gui_ctx *ctx,
            struct gui_panel *pan, struct gui_panel *parent) {
  assert(db);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box, .show_btn = 1};
    gui.tab.begin(ctx, &tab, pan, dyn_cnt(d->tbls), d->sel_tbl);
    {
      /* tab header */
      int del_tab = 0;
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      for loop(i, tab.cnt) {
        /* tab header slots */
        struct gui_panel slot = {0};
        struct db_tbl_view *tbl = d->tbls[i];
        if (ui_db_explr_tab(d, tbl, ctx, &tab, &hdr, &slot)) {
          del_tab = 1;
        }
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);
      if (tab.sort.mod) {
        ui_db_resort_tbls(d, tab.sort.dst, tab.sort.src);
      }
      if (del_tab) {
        /* close table view tab */
        assert(dyn_cnt(d->tbls));
        assert(d->sel_tbl < dyn_cnt(d->tbls));
        struct db_tbl_view *view = d->tbls[d->sel_tbl];
        dyn_rm(d->tbls, d->sel_tbl);
        db_tbl_view_del(d, view, ctx->sys);
        d->sel_tbl = clamp(0, tab.sel.idx, dyn_cnt(d->tbls)-1);
      }
      struct gui_btn add = {.box = hdr.pan.box};
      add.box.x = gui.bnd.min_ext(tab.off, ctx->cfg.item);
      if (gui.btn.ico(ctx, &add, &hdr.pan, RES_ICO_FOLDER_ADD)) {
        /* open new table view tab */
        struct db_tbl_view *view = db_tbl_view_new(d, ctx->sys);
        dyn_put(d->tbls, ctx->sys, 0, &view, 1);
        d->sel_tbl = 0;
      }
      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      d->show_tab_lst = tab.btn.clk ? !d->show_tab_lst : d->show_tab_lst;
      if (d->show_tab_lst) {
        /* overflow tab selection */
        int ret = ui_db_tab_view_lst(d, ctx, &bdy, pan);
        if (ret >= 0) {
          ui_db_resort_tbls(d, 0, ret);
          d->show_tab_lst = 0;
          d->sel_tbl = 0;
        }
      } else {
        ui_db_main(d, d->tbls[d->sel_tbl], ctx, &bdy, pan);
      }
    }
    gui.tab.end(ctx, &tab, pan);
    if (tab.sel.mod) {
      d->sel_tbl = tab.sel.idx;
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
  .update = db_update,
  .del = db_free,
  .ui = ui_db_explr,
};
static void
db_api(void *export, void *import) {
  unused(import);
  struct db_api *exp = cast(struct db_api*, export);
  *exp = db__api;
}

