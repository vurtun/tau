#define DBS_VERISON 1

struct arena;
struct gui_api;
struct gui_ctx;
struct gui_panel;
struct db_ui_view;

#define DB_MAX_FILTER 64
#define DB_TBL_VIEW_CNT 64
#define DB_SQL_QRY_BUF_SIZ KB(16)

enum db_tbl_lst_elm_type {
  DB_TBL_LST_ELM_UNKNOWN,
  DB_TBL_LST_ELM_TBL,
  DB_TBL_LST_ELM_VIEW,
  DB_TBL_LST_ELM_IDX,
  DB_TBL_LST_ELM_TRIGGER,
};
enum db_tree_col_sel {
  DB_TREE_COL_NAME,
  DB_TREE_COL_TYPE,
  DB_TREE_COL_SQL,
  DB_TREE_COL_MAX,
};
struct db_tree_tbl_state {
  struct gui_tbl_sort sort;
  int state[GUI_TBL_CAP(DB_TREE_COL_MAX)];
  double off[2];
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
  char* fnd_buf;
  struct gui_txt_ed fnd_ed;
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

  struct arena_scope scp;
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
  struct arena_scope scp;
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
  struct arena *tmp_mem;
  struct arena mem;
  struct arena_scope scp;

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
  struct db_tbl_ui_state ui;
};
struct db_view {
  struct arena mem;
  struct arena *tmp_mem;
  sqlite3 *con;
  struct str path;
  struct db_tree_view tree;
  unsigned tree_rev;
  char sql_qry_buf[DB_SQL_QRY_BUF_SIZ];

  /* views */
  double tbl_lst_off[2];
  struct db_tbl_view tbls[DB_TBL_VIEW_CNT];
  unsigned long long unused;
  unsigned char show_tab_lst;
  unsigned char sel_tab;
  unsigned char tab_cnt;
  unsigned char tabs[DB_TBL_VIEW_CNT];
};
struct db_api {
  int version;
  int (*init)(void *mem, int siz);
  struct db_view* (*new)(struct gui_ctx *ctx, struct arena *mem, struct arena *tmp_mem, struct str path);
  void (*update)(struct db_view *db, struct sys *sys);
  void (*del)(struct db_view *db, struct sys *sys);
  void (*ui)(struct db_view *db, struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent);
};

