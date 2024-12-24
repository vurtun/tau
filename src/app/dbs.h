#define DBS_VERISON 1

struct gui_ctx;
struct gui_panel;

enum {
  DB_MAX_FILTER               = 32,
  DB_TBL_CNT                  = 16,
  DB_MAX_TBL_NAME             = 64,
  DB_MAX_TBL_TITLE            = 32,
  DB_MAX_TBL_SQL              = 64,
  DB_MAX_TBL_COLS             = 64,
  DB_MAX_TBL_ROW_COLS         = 8,
  DB_MAX_TBL_ROWS             = 128,
  DB_MAX_TBL_ELM              = (DB_MAX_TBL_ROWS*DB_MAX_TBL_ROW_COLS),
  DB_MAX_TBL_COL_NAME         = 64,
  DB_MAX_TBL_COL_TYPE         = 64,
  DB_MAX_TBL_ELM_DATA         = 64,
  DB_MAX_INFO_ELM_CNT         = 128,
  DB_MAX_FLTR_STR             = 32,
  DB_MAX_FLTR_CNT             = 8,
  DB_MAX_FLTR_ELM             = 128,
  DB_MAX_FLTR_ELM_STR         = 64,

  DB_INFO_NAME_STR_BUF_SIZ    = (DB_MAX_INFO_ELM_CNT * DB_MAX_TBL_NAME),
  DB_INFO_SQL_STR_BUF_SIZ     = (DB_MAX_INFO_ELM_CNT * DB_MAX_TBL_SQL),
  DB_INFO_STR_BUF_SIZ         = (DB_INFO_NAME_STR_BUF_SIZ + DB_INFO_SQL_STR_BUF_SIZ),
  DB_TBL_COL_NAME_STR_BUF_SIZ = (DB_MAX_TBL_COLS * DB_MAX_TBL_COL_NAME),
  DB_TBL_COL_TYPE_STR_BUF_SIZ = (DB_MAX_TBL_COLS * DB_MAX_TBL_COL_TYPE),
  DB_TBL_COL_STR_BUF_SIZ      = (DB_TBL_COL_NAME_STR_BUF_SIZ + DB_TBL_COL_TYPE_STR_BUF_SIZ),
  DB_TBL_ELM_STR_BUF_SIZ      = (DB_MAX_TBL_ELM*DB_MAX_TBL_ELM_DATA),
  DB_TBL_FLTR_STR_BUF_SIZ     = (DB_MAX_FLTR_ELM * DB_MAX_FLTR_ELM_STR),
  DB_SQL_QRY_BUF_SIZ          = KB(16),
  DB_SQL_QRY_NAME_BUF_SIZ     = 128,
};
#define DB_FLTR_CNT_MSK   ((1u << castu(DB_MAX_FLTR_CNT))-1)
#define DB_TBL_CNT_MSK    ((1u << castu(DB_TBL_CNT))-1)

/* ----------------------------------------------------------------------------
 *                                State
 *-----------------------------------------------------------------------------
 */
/* filter */
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
enum db_tbl_fltr_ui_view {
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
  long long col;
  char fnd_buf[DB_MAX_FLTR_STR];
  struct str fnd;
  char col_buf[DB_MAX_TBL_COL_NAME];
  struct str col_name;
};
struct db_tbl_fltr_ui {
  int cnt;
  int state[GUI_TBL_CAP(DB_TBL_FLTR_MAX)];
  int off[2];
};
struct db_tbl_fltr_state {
  enum db_tbl_fltr_ui_view state;
  unsigned unused;
  struct db_tbl_fltr_elm elms[DB_MAX_FLTR_CNT];
  unsigned char lst[DB_MAX_FLTR_CNT];
  unsigned char cnt;
  long long ini_col;
  struct rng data_rng;

  /* ui */
  struct db_tbl_fltr_ui tbl;
  struct db_tbl_fltr_ui tbl_col;
  int off[2];
  unsigned rev;
  struct rng elm_rng;
  int sel_col;
  int init;
};

/* table */
enum db_tree_col_sel {
  DB_TREE_COL_NAME,
  DB_TREE_COL_TYPE,
  DB_TREE_COL_SQL,
  DB_TREE_COL_MAX,
};
struct db_tree_tbl_state {
  int state[GUI_TBL_CAP(DB_TREE_COL_MAX)];
  int off[2];
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
struct db_tbl_col_def {
  struct str title;
  struct gui_split_lay_slot ui;
};
enum db_tbl_hdr_col {
  DB_TBL_NAME,
  DB_TBL_TYPE,
  DB_TBL_MAX
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
enum db_tbl_view_state {
  TBL_VIEW_SELECT,
  TBL_VIEW_DISPLAY,
};
struct db_tbl_ui_state {
  int state[GUI_TBL_CAP(DB_MAX_TBL_ROW_COLS)];
  int off[2];
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
  int off[2];
};
struct db_tbl_col_lst_state {
  enum db_tbl_col_state state;
  int cnt, total;
  struct rng rng;
  struct db_tbl_ui_col_state ui;
  struct tbl(long long, DB_MAX_TBL_COLS) sel;
};
struct db_tbl_row_lst_state {
  struct rng rng;
  struct rng cols;
  struct rng data_rng;
  struct db_tbl_ui_state ui;
};
struct db_tbl_state {
  enum db_tbl_type kind;
  enum db_tbl_view_state state;
  enum db_tbl_view_dsp_state disp;

  long long rowid;
  char title_buf[DB_MAX_TBL_NAME];
  struct str title;
  unsigned qry_name:1;

  struct db_tbl_col_lst_state col;
  struct db_tbl_row_lst_state row;
  struct db_tbl_fltr_state fltr;
};

/* database */
struct db_info_state {
  enum db_tbl_type sel_tab;
  int tab_cnt[DB_TBL_TYPE_CNT];
  unsigned tab_act;

  int elm_cnt;
  struct rng elm_rng;
  struct db_tree_tbl_state tbl;
};
struct db_state {
  sqlite3 *con;
  unsigned long long id;

  struct db_tbl_state tbls[DB_TBL_CNT];
  struct db_info_state info;
  int tbl_lst_off[2];

  unsigned unused;
  unsigned char tabs[DB_TBL_CNT];
  unsigned char tab_cnt;
  unsigned char sel_tab;
  unsigned char show_tab_lst;
};

/* ----------------------------------------------------------------------------
 *                                View
 *-----------------------------------------------------------------------------
 */
/* filter */
struct db_tbl_fltr_view {
  long long id;
  struct str fnd_str;
  char fnd_buf[DB_MAX_FILTER];
  long long rowid[DB_MAX_FLTR_ELM];
  unsigned data[DB_MAX_FLTR_ELM];
  struct str_buf(DB_TBL_FLTR_STR_BUF_SIZ) buf;
};
/* tables */
struct db_tbl_col {
  long long rowid;
  unsigned name;
  unsigned type;
  unsigned ico:27;
  unsigned pk:1;
  unsigned fk:1;
  unsigned nn:1;
  unsigned blob:1;
  unsigned qry_name:1;
};
struct db_tbl_col_lst_view {
  long long id;
  struct db_tbl_col lst[DB_MAX_TBL_COLS];
  struct str_buf(DB_TBL_COL_STR_BUF_SIZ) buf;
};
struct db_tbl_row_lst_view {
  long long id;
  long long rowids[DB_MAX_TBL_ROWS];
  unsigned lst[DB_MAX_TBL_ELM];
  struct str_buf(DB_TBL_ELM_STR_BUF_SIZ) buf;
};
struct db_tbl_view {
  struct db_tbl_col_lst_view col;
  struct db_tbl_row_lst_view row;
  struct db_tbl_fltr_view fltr;
};

/* database */
struct db_info_elm {
  long long rowid;
  unsigned name;
  unsigned sql;
};
struct db_info_view {
  /* elms */
  unsigned long long id;
  struct db_info_elm elms[DB_MAX_INFO_ELM_CNT];
  struct str_buf(DB_INFO_STR_BUF_SIZ) buf;
  /* ui */
  struct str fnd_str;
  char fnd_buf[DB_MAX_FLTR_STR];
  struct gui_txt_ed fnd_ed;
  struct tbl(long long, DB_MAX_INFO_ELM_CNT) sel;
};
struct db_view {
  unsigned long long id;
  char sql_qry_buf[DB_SQL_QRY_BUF_SIZ];
  struct db_tbl_view tbl;
  struct db_info_view info;
};

/* ----------------------------------------------------------------------------
 *                                API
 *-----------------------------------------------------------------------------
 */
struct db_api {
  int version;
  int (*init)(void *mem, int siz);
  int (*setup)(struct db_state *state, struct gui_ctx *ctx, struct str path);
  void (*del)(struct db_state *db);
  void (*ui)(struct db_state *db, struct db_view *view, struct gui_ctx *ctx,
             struct gui_panel *pan, struct gui_panel *parent);
};

