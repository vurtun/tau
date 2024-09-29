#define DBS_VERISON 1

struct arena;
struct gui_api;
struct gui_ctx;
struct gui_panel;

#define DB_MAX_FILTER   64
#define DB_TBL_VIEW_CNT 64
#define DB_MAX_TBL_NAME 256
#define DB_MAX_TBL_SQL  1024
#define DB_MAX_ELM_CNT  256
#define DB_MAX_FLTR_STR 32
#define DB_MAX_FLTR_CNT 32

#define DB_INFO_NAME_STR_BUF_SIZ (DB_MAX_ELM_CNT * DB_MAX_TBL_NAME)
#define DB_INFO_SQL_STR_BUF_SIZ (DB_MAX_ELM_CNT * DB_MAX_TBL_SQL)
#define DB_SQL_QRY_BUF_SIZ KB(16)

enum db_tree_col_sel {
  DB_TREE_COL_NAME,
  DB_TREE_COL_TYPE,
  DB_TREE_COL_SQL,
  DB_TREE_COL_MAX,
};
struct db_tree_tbl_state {
  int state[GUI_TBL_CAP(DB_TREE_COL_MAX)];
  double off[2];
};
#define DB_TBL_MAP(TYPE)\
  TYPE(TBL,     "table",    RES_ICO_TABLE)\
  TYPE(VIEW,    "view",     RES_ICO_IMAGE)\
  TYPE(IDX,     "index",    RES_ICO_TAG)\
  TYPE(TRIGGER, "trigger",  RES_ICO_BOLT)

enum db_tbl_type {
#define DB_TBL_TYPE(a,b,c) DB_TBL_TYPE_##a,
  DB_TBL_MAP(DB_TBL_TYPE)
#undef DB_TBL_TYPE
  DB_TBL_TYPE_CNT
};
struct db_info_buf {
  int name_cnt;
  int sql_cnt;
  char name[DB_INFO_NAME_STR_BUF_SIZ];
  char sql[DB_INFO_SQL_STR_BUF_SIZ];
};
struct db_info_elm {
  long long rowid;
  struct str name;
  struct str sql;
};
struct db_info_view {
  enum db_tbl_type sel_tab;
  int tab_cnt[DB_TBL_TYPE_CNT];
  unsigned tab_act;

  /* ui */
  char *fnd_buf;
  struct gui_txt_ed fnd_ed;
  struct db_tree_tbl_state tbl;
  struct tbl(long long, DB_MAX_ELM_CNT) sel;

  /* elms */
  int elm_cnt;
  struct rng elm_rng;
  struct db_info_elm elms[DB_MAX_ELM_CNT];
  struct db_info_buf buf;
};
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
struct db_tbl_ui_state {
  struct gui_tbl_sort sort;
  int *state;
  double off[2];
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

#if 0
struct db_tbl_fltr_tm {
  time_t min;
  time_t max;

  struct tm from;
  struct tm to;

  time_t from_val;
  time_t to_val;
};
#endif

enum db_tbl_fltr_elm_typ {
  DB_TBL_FLTR_ELM_TYP_STR,
};
struct db_tbl_fltr_elm {
  unsigned enabled: 1;
  unsigned type: 31;
  int col;
  char buf[DB_MAX_FLTR_STR];
  struct str str;
};
struct db_tbl_fltr_state {
  int cnt;
  struct gui_tbl_sort sort;
  int state[GUI_TBL_CAP(DB_TBL_FLTR_MAX)];
  double off[2];
};
struct db_tbl_fltr_view {
  unsigned unused;
  struct db_tbl_fltr_elm elms[DB_MAX_FLTR_CNT];
  unsigned char lst[DB_MAX_FLTR_CNT];
  unsigned char fltr_cnt;

  /* ui */
  struct db_tbl_fltr_state tbl;
  struct db_tbl_fltr_state tbl_col;
  double off[2];

  dyn(char) buf;
  struct arena_scope scp;
  struct str *data;

  int sel_col;
  unsigned rev;
  struct rng elm_rng;
};
struct db_tbl_blob_view {
  unsigned disabled : 1;
  struct arena_scope scp;
  unsigned char *mem;
  int siz;
  int sel_tab;
  struct db_tbl_blob_img_view {
    int act, id;
    int w, h;
    double off[2];
  } img;
  double off[2];
};
enum db_tbl_view_state {
  TBL_VIEW_SELECT,
  TBL_VIEW_DISPLAY,
  TBL_VIEW_FILTER,
  TBL_VIEW_FILTER_LIST,
  TBL_VIEW_BLOB_VIEW,
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
struct db_tbl_view {
  enum db_tbl_type kind;
  enum db_tbl_view_state state;

  int init;
  struct arena *tmp_mem;
  struct arena mem;
  struct arena_scope scp;

  unsigned rev;
  struct str name;
  struct db_tbl_fltr_view fltr;
  struct db_tbl_blob_view blob;
  struct db_tbl_col *cols;
  struct str *data;

  /* ui */
  int total;
  int row_cnt;
  int row_begin;
  int row_end;
  struct db_tbl_ui_state ui;
};
struct db_view {
  struct arena mem;
  struct arena *tmp_mem;
  sqlite3 *con;
  struct str path;
  char path_buf[MAX_FILE_PATH];
  struct db_info_view info;
  char sql_qry_buf[DB_SQL_QRY_BUF_SIZ];

  /* tbls */
  unsigned long long unused;
  struct db_tbl_view tbls[DB_TBL_VIEW_CNT];
  unsigned char tabs[DB_TBL_VIEW_CNT];
  unsigned char show_tab_lst;
  unsigned char tab_cnt;
  unsigned char sel_tab;
  double tbl_lst_off[2];
};
struct db_api {
  int version;
  int (*init)(void *mem, int siz);
  struct db_view* (*new)(struct gui_ctx *ctx, struct arena *mem, struct arena *tmp_mem, struct str path);
  void (*del)(struct db_view *db, struct sys *sys);
  void (*ui)(struct db_view *db, struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent);
};

