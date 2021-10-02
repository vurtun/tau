#define SYS_VERSION 1

/* input */
struct sys_btn {
  unsigned down : 1;
  unsigned pressed : 1;
  unsigned released : 1;
  unsigned doubled : 1;
  long timestamp;
};
enum sys_keymod {
  SYS_KEYMOD_KEY_CTRL,
  SYS_KEYMOD_KEY_SHIFT,
  SYS_KEYMOD_KEY_ALT,
};
enum sys_keymods {
  SYS_KEYMOD_CTRL = (1u << SYS_KEYMOD_KEY_CTRL),
  SYS_KEYMOD_SHIFT = (1u << SYS_KEYMOD_KEY_SHIFT),
  SYS_KEYMOD_ALT = (1u << SYS_KEYMOD_KEY_ALT),
};
enum sys_keys {
  SYS_KEY_END = 128,
  SYS_KEY_CAPS,
  SYS_KEY_RETURN,
  SYS_KEY_SPACE,
  SYS_KEY_BACKSPACE,
  SYS_KEY_ESCAPE,
  SYS_KEY_TAB,
  SYS_KEY_LEFT,
  SYS_KEY_RIGHT,
  SYS_KEY_UP,
  SYS_KEY_DOWN,
  SYS_KEY_DEL,
  SYS_KEY_PGUP,
  SYS_KEY_PGDN,
  SYS_KEY_HOME,
  SYS_KEY_PLUS,
  SYS_KEY_MINUS,
  SYS_KEY_F1,
  SYS_KEY_F2,
  SYS_KEY_F3,
  SYS_KEY_F4,
  SYS_KEY_F5,
  SYS_KEY_F6,
  SYS_KEY_F7,
  SYS_KEY_F8,
  SYS_KEY_F9,
  SYS_KEY_F10,
  SYS_KEY_F11,
  SYS_KEY_F12,
  SYS_KEY_CNT
};
enum sys_mouse_id {
  SYS_MOUSE_LEFT,
  SYS_MOUSE_RIGHT,
  SYS_MOUSE_MIDDLE,
  SYS_MOUSE_BTN_CNT
};
struct sys_mouse_btns {
  struct sys_btn left;
  struct sys_btn right;
  struct sys_btn middle;
};
struct sys_mouse {
  int has_grab;
  int pos[2];
  int pos_last[2];
  int pos_delta[2];
  int scrl[2];
  union {
    struct sys_btn btns[SYS_MOUSE_BTN_CNT];
    struct sys_mouse_btns btn;
  };
};

/* memory */
enum sys_mem_blk_flags {
  SYS_MEM_GROWABLE  = 0x1,
};
struct sys_mem_stats {
  int blk_cnt;
  int total;
  int used;
};

/* directory */
struct sys_dir_iter {
  char valid, err;
  struct str fullpath;
  struct str base;
  struct str name;

  char isdir;
  void *handle;
  struct scope scp;
  struct scope scp_base;
};

/* cursor */
enum sys_cur_style {
  SYS_CUR_ARROW,
  SYS_CUR_DEFAULT = SYS_CUR_ARROW,
  SYS_CUR_NO,
  SYS_CUR_CROSS,
  SYS_CUR_HAND,
  SYS_CUR_HELP,
  SYS_CUR_IBEAM,
  SYS_CUR_MOVE,
  SYS_CUR_SIZE_NS,
  SYS_CUR_SIZE_WE,
  SYS_CUR_UP_ARROW,
  SYS_CUR_DOWN_ARROW,
  SYS_CUR_LEFT_ARROW,
  SYS_CUR_RIGHT_ARROW,
  SYS_CUR_CNT,
};

/* color */
enum sys_color {
  SYS_COL_WIN,
  SYS_COL_BG,
  SYS_COL_CTRL,
  SYS_COL_SEL,
  SYS_COL_HOV,
  SYS_COL_TXT,
  SYS_COL_TXT_SEL,
  SYS_COL_TXT_DISABLED,
  SYS_COL_ICO,
  SYS_COL_LIGHT,
  SYS_COL_SHADOW,
  SYS_COL_CNT
};

/* ren */
#define sys_rect(x, y, w, h) (struct sys_rect) { x, y, w, h }
struct sys_rect {
  int x,y,w,h;
};
struct sys_ren_target {
  int resized;
  int w, h;
  unsigned *pixels;
  dyn(struct sys_rect) dirty_rects;
};

/* window */
struct sys_win {
  int w, h;
};

/* api */
struct sys_mem_api {
  long page_siz;
  long phy_siz;

  struct arena *arena;
  struct arena *tmp;

  struct mem_blk*(*alloc)(struct mem_blk* opt_old, int siz, unsigned flags, unsigned long long tag);
  void (*free)(struct mem_blk *blk);
  void (*free_tag)(unsigned long long tag);
  void (*info)(struct sys_mem_stats *stats);
};
struct sys_dir_api {
  void (*lst)(struct sys_dir_iter *it, struct arena *a, struct str path);
  void (*nxt)(struct sys_dir_iter *it, struct arena *a);
  int (*exists)(struct str path, struct arena *tmp);
};
struct sys_clip_api {
  void (*set)(struct str s, struct arena *a);
  struct str (*get)(struct arena *a);
};
struct sys_mod_api {
  int (*add)(void *exp, void *imp, struct str name);
};

/* platform */
struct sys {
  int quit;
  int running;
  unsigned seq;
  int version;

  /* args */
  int argc;
  char **argv;

  float dpi_scale;
  struct cpu_info cpu;
  enum sys_cur_style cursor;
  struct sys_win win;

  /* colors */
  unsigned col_mod:1;
  unsigned col[SYS_COL_CNT];

  /* modules */
  void *platform;
  void *renderer;
  void *app;

  /* api */
  struct sys_mem_api mem;
  struct sys_dir_api dir;
  struct sys_clip_api clipboard;
  struct ren_api ren;
  struct sys_mod_api plugin;

  /* input */
  unsigned key_mod:1;
  unsigned btn_mod:1;
  unsigned txt_mod:1;
  unsigned mouse_mod:1;
  unsigned mouse_grap:1;
  unsigned scrl_mod:1;

  int txt_len;
  #define SYS_MAX_INPUT 1024
  char txt[SYS_MAX_INPUT];
  unsigned keymod;
  unsigned focus;
  struct sys_mouse mouse;
  unsigned long keys[bits_to_long(SYS_KEY_CNT)];

  /* render */
  struct sys_ren_target ren_target;
};

