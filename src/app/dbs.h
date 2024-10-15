#define DBS_VERISON 1

struct arena;
struct gui_api;
struct gui_ctx;
struct gui_panel;

#define DB_MAX_FILTER         64
#define DB_TBL_VIEW_CNT       16
#define DB_MAX_TBL_NAME       128
#define DB_MAX_TBL_SQL        256
#define DB_MAX_TBL_COLS       128
#define DB_MAX_TBL_ROW_COLS   8
#define DB_MAX_TBL_ROWS       128
#define DB_MAX_TBL_ELM        (DB_MAX_TBL_ROWS*DB_MAX_TBL_ROW_COLS)
#define DB_MAX_TBL_COL_NAME   128
#define DB_MAX_TBL_COL_TYPE   128
#define DB_MAX_TBL_ELM_DATA   64
#define DB_MAX_INFO_ELM_CNT   128
#define DB_MAX_FLTR_STR       64
#define DB_MAX_FLTR_CNT       32
#define DB_MAX_FLTR_ELM       128
#define DB_MAX_FLTR_ELM_STR   64

#define DB_INFO_NAME_STR_BUF_SIZ    (DB_MAX_INFO_ELM_CNT * DB_MAX_TBL_NAME)
#define DB_INFO_SQL_STR_BUF_SIZ     (DB_MAX_INFO_ELM_CNT * DB_MAX_TBL_SQL)
#define DB_INFO_STR_BUF_SIZ         (DB_INFO_NAME_STR_BUF_SIZ + DB_INFO_SQL_STR_BUF_SIZ)
#define DB_TBL_COL_NAME_STR_BUF_SIZ (DB_MAX_TBL_COLS * DB_MAX_TBL_COL_NAME)
#define DB_TBL_COL_TYPE_STR_BUF_SIZ (DB_MAX_TBL_COLS * DB_MAX_TBL_COL_TYPE)
#define DB_TBL_COL_STR_BUF_SIZ      (DB_TBL_COL_NAME_STR_BUF_SIZ + DB_TBL_COL_TYPE_STR_BUF_SIZ)
#define DB_TBL_ELM_STR_BUF_SIZ      (DB_MAX_TBL_ELM*DB_MAX_TBL_ELM_DATA)
#define DB_TBL_FLTR_STR_BUF_SIZ     (DB_MAX_FLTR_ELM * DB_MAX_FLTR_ELM_STR)
#define DB_SQL_QRY_BUF_SIZ          KB(16)

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
#define DB_TBL_MAP(TYPE)                                \
  TYPE(TBL,       "table",    "Tables",   RES_ICO_TABLE)\
  TYPE(VIEW,      "view",     "Views",    RES_ICO_IMAGE)\
  TYPE(IDX,       "index",    "Indexs",   RES_ICO_TAG)  \
  TYPE(TRIGGER,   "trigger",  "Triggers", RES_ICO_BOLT)

enum db_tbl_type {
#define DB_TBL_TYPE(a,b,c,d) DB_TBL_TYPE_##a,
  DB_TBL_MAP(DB_TBL_TYPE)
#undef DB_TBL_TYPE
  DB_TBL_TYPE_CNT
};
struct db_info_elm {
  long long rowid;
  unsigned name;
  unsigned sql;
};
struct db_info_view {
  enum db_tbl_type sel_tab;
  int tab_cnt[DB_TBL_TYPE_CNT];
  unsigned tab_act;

  /* elms */
  int elm_cnt;
  struct rng elm_rng;
  struct db_info_elm elms[DB_MAX_INFO_ELM_CNT];
  struct str_buf(DB_INFO_STR_BUF_SIZ) buf;

  /* ui */
  struct str fnd_str;
  char fnd_buf[DB_MAX_FLTR_STR];
  struct gui_txt_ed fnd_ed;
  struct db_tree_tbl_state tbl;
  struct tbl(long long, DB_MAX_INFO_ELM_CNT) sel;
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
  int state[GUI_TBL_CAP(DB_TBL_MAX)];
};
enum db_tbl_fltr_col {
  DB_TBL_FLTR_STATE,
  DB_TBL_FLTR_BUF,
  DB_TBL_FLTR_COL,
  DB_TBL_FLTR_DEL,
  DB_TBL_FLTR_MAX
};
enum db_tbl_fltr_col_sel {
  DB_TBL_FLTR_COL_NAME,
  DB_TBL_FLTR_COL_TYPE,
  DB_TBL_FLTR_COL_MAX,
};
enum db_tbl_fltr_state {
  DB_TBL_FLTR_LST,
  DB_TBL_FLTR_EDT
};
enum db_tbl_fltr_elm_typ {
  DB_TBL_FLTR_ELM_TYP_STR,
  DB_TBL_FLTR_ELM_TYP_SEL,
  DB_TBL_FLTR_ELM_TYP_TM
};
struct db_tbl_fltr_elm {
  unsigned enabled: 1;
  unsigned type: 31;
  char col_buf[DB_MAX_TBL_COL_NAME];
  struct str col;
  char fnd_buf[DB_MAX_FLTR_STR];
  struct str fnd;
};
struct db_tbl_fltr_ui {
  int cnt;
  int state[GUI_TBL_CAP(DB_TBL_FLTR_MAX)];
  double off[2];
};
struct db_tbl_fltr_view {
  enum db_tbl_fltr_state state;

  unsigned unused;
  struct db_tbl_fltr_elm elms[DB_MAX_FLTR_CNT];
  unsigned char lst[DB_MAX_FLTR_CNT];
  unsigned char fltr_cnt;
  char ini_col_buf[DB_MAX_TBL_COL_NAME];
  struct str ini_col;

  struct str fnd_str;
  char fnd_buf[DB_MAX_FILTER];
  long long rowid[DB_MAX_FLTR_ELM];
  unsigned data[DB_MAX_FLTR_ELM];
  struct str_buf(DB_TBL_FLTR_STR_BUF_SIZ) buf;
  struct rng data_rng;

  /* ui */
  struct db_tbl_fltr_ui tbl;
  struct db_tbl_fltr_ui tbl_col;
  double off[2];
  unsigned rev;
  struct rng elm_rng;
  int sel_col;
  int init;
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
};
struct db_tbl_ui_state {
  int state[GUI_TBL_CAP(DB_MAX_TBL_COLS)];
  double off[2];
};
enum db_tbl_ui_disp_state_col {
  DB_TBL_DISP_COL_ACT,
  DB_TBL_DISP_COL_NAME,
  DB_TBL_DISP_COL_TYPE,
  DB_TBL_DISP_COL_PK,
  DB_TBL_DISP_COL_FK,
  DB_TBL_DISP_COL_NN,
  DB_TBL_DISP_COL_FLTR,
  DB_TBL_DISP_COL_MAX,
};
struct db_tbl_ui_col_state {
  int state[GUI_TBL_CAP(DB_TBL_DISP_COL_MAX)];
  double off[2];
};
struct db_tbl_col {
  long long rowid;
  unsigned name;
  unsigned type;
  unsigned ico:28;
  unsigned pk:1;
  unsigned fk:1;
  unsigned nn:1;
  unsigned blob:1;
};
enum db_tbl_view_dsp_state {
  DB_TBL_VIEW_DSP_DATA,
  DB_TBL_VIEW_DSP_FILTER,
  DB_TBL_VIEW_DSP_LAYOUT,
  DB_TBL_VIEW_DSP_CNT
};
enum db_tbl_col_state {
  DB_TBL_COL_STATE_LOCKED,
  DB_TBL_COL_STATE_UNLOCKED,
};
struct db_tbl_col_lst {
  enum db_tbl_col_state state;
  int cnt, total;
  struct rng rng;
  struct db_tbl_col lst[DB_MAX_TBL_COLS];
  struct str_buf(DB_TBL_COL_STR_BUF_SIZ) buf;
  struct tbl(long long, DB_MAX_TBL_COLS) sel;
  struct db_tbl_ui_col_state ui;
};
struct db_tbl_row_lst {
  struct rng rng;
  struct rng cols;
  long long rowids[DB_MAX_TBL_ROWS];
  unsigned lst[DB_MAX_TBL_ELM];
  struct str_buf(DB_TBL_ELM_STR_BUF_SIZ) buf;
  struct db_tbl_ui_state ui;
};
struct db_tbl_view {
  enum db_tbl_type kind;
  enum db_tbl_view_state state;
  enum db_tbl_view_dsp_state disp;

  unsigned rev;
  char name_buf[DB_MAX_TBL_NAME];
  struct str name;
  long long rowid;

  struct db_tbl_col_lst col;
  struct db_tbl_row_lst row;
  struct db_tbl_fltr_view fltr;
  struct db_tbl_blob_view blob;
};
struct db_view {
  sqlite3 *con;
  struct str path;
  char path_buf[MAX_FILE_PATH];
  struct db_info_view info;
  char sql_qry_buf[DB_SQL_QRY_BUF_SIZ];

  /* tbls */
  unsigned unused;
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
  struct db_view* (*new)(struct gui_ctx *ctx, struct arena *mem, struct str path);
  void (*del)(struct db_view *db);
  void (*ui)(struct db_view *db, struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent);
};

