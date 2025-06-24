#define PCK_VERISON 1

struct gui_api;
struct gui_ctx;
struct gui_panel;

enum {
  FILE_LIST_ELM_CNT         = 128,
  FILE_LIST_ELM_BUF_CNT     = (FILE_LIST_ELM_CNT*2),
  FILE_LIST_STR_BUF_SIZ     = (FILE_LIST_ELM_BUF_CNT*MAX_FILE_NAME),
  FILE_LIST_MAX_FILTER      = 32,
  FILE_SPLIT_MAX            = 2,
  FILE_MAX_PAGE_BUF         = 128,
};
struct file_elm {
  struct str name;
  size_t size;
  time_t mtime;
  unsigned file_type : 17;
  unsigned sys_type : 4;
  unsigned isdir : 1;
  unsigned perm : 9;
};
struct file_tbl_col_def {
  struct str title;
  struct gui_split_lay_slot ui;
};
enum file_tbl_hdr_col {
  FILE_TBL_NAME,
  FILE_TBL_TYPE,
  FILE_TBL_SIZE,
  FILE_TBL_PERM,
  FILE_TBL_DATE,
  FILE_TBL_MAX,
};
struct file_tbl_ui {
  int cnt;
  struct gui_tbl_sort sort;
  int state[GUI_TBL_CAP(FILE_TBL_MAX)];
};
struct file_list_txt_buf {
  int cnt, cur;
  char buf[2][FILE_LIST_STR_BUF_SIZ];
};
struct file_list_page {
  int cur, idx;
  int total, cnt;
  struct file_elm elms[FILE_LIST_ELM_BUF_CNT];
  struct file_list_txt_buf txt;
};
struct file_list_view {
  unsigned rev;
  int sel_idx;
  int page_cnt;
  struct file_list_page page;

  int off[2];
  struct file_tbl_ui tbl;

  struct str nav_path;
  char nav_buf[MAX_FILE_PATH];
  struct gui_txt_ed nav_ed;

  struct str fltr;
  char fltr_buf[FILE_LIST_MAX_FILTER];
  struct gui_txt_ed fltr_ed;
};
struct file_view {
  int state;
  char home_path[MAX_FILE_PATH];
  struct str home;
  struct file_list_view lst;
};
struct pck_api {
  int version;
  int (*init)(struct file_view *fs, struct sys *sys, struct gui_ctx *ctx);
  void (*shutdown)(struct file_view*, struct sys *sys);
  struct str (*ui)(char *filepath, int n, struct file_view*, struct gui_ctx *ctx,
                   struct gui_panel *pan, struct gui_panel *parent);
};
static void pck_api(void *export, void *import);

