#define SYS_VERSION 1

/* input */
struct sys;
struct sys_btn {
  unsigned down : 1;
  unsigned pressed : 1;
  unsigned released : 1;
  unsigned doubled : 1;
  unsigned long long timestamp;
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
  SYS_MEM_CHK_UFLW  = 0x2,
  SYS_MEM_CHK_OFLW  = 0x4,
  SYS_MEM_CHK_FIT   = 0x8,
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
  struct arena_scope scp;
  struct arena_scope scp_base;
};

/* cursor */
enum sys_cur_style {
  SYS_CUR_ARROW,
  SYS_CUR_DEFAULT = SYS_CUR_ARROW,
  SYS_CUR_NO,
  SYS_CUR_CROSS,
  SYS_CUR_HAND,
  SYS_CUR_IBEAM,
  SYS_CUR_MOVE,
  SYS_CUR_SIZE_NS,
  SYS_CUR_SIZE_WE,
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

/* window */
struct sys_win {
  int x, y, w, h;
  const char *title;
  unsigned resized:1;
};

/* drag & drop */
enum sys_dnd_response {
  SYS_DND_REJECT,
  SYS_DND_ACCEPT,
};
enum sys_dnd_state {
  SYS_DND_NONE,
  SYS_DND_ENTER,
  SYS_DND_PREVIEW,
  SYS_DND_DELIVERY,
  SYS_DND_LEFT
};
enum sys_dnd_data_type {
  SYS_DND_FILE,
  SYS_DND_STR,
};
struct sys_dnd {
  enum sys_dnd_state state;
  enum sys_dnd_data_type type;
  enum sys_dnd_response response;
  struct str *files;
  int file_cnt;
  struct str str;
};

/* tooltip */
struct sys_tooltip {
  char buf[256];
  struct str str;
};

/* api */
struct sys_mem_api {
  long page_siz;
  long phy_siz;

  struct arena *arena;
  struct arena *tmp;

  struct mem_blk*(*alloc)(struct sys *s, struct mem_blk* opt_old, int siz, unsigned flags, unsigned long long tag);
  void (*free)(struct sys *s, struct mem_blk *blk);
  void (*free_tag)(struct sys *s, unsigned long long tag);
  void (*info)(struct sys *s, struct sys_mem_stats *stats);
};
enum sys_file_type {
  SYS_FILE_DEF,
  SYS_FILE_DIR,
  SYS_FILE_SOCK,
  SYS_FILE_LNK,
  SYS_FILE_FIFO,
};
struct sys_file_info {
  size_t siz;
  time_t mtime;
  char perm[10];
  enum sys_file_type type;
};
struct sys_file_api {
  int (*info)(struct sys*, struct sys_file_info *info, struct str path, struct arena *tmp);
};
struct sys_dir_api {
  #define sys_dir_lst_each(s, i, a, p)\
    ((s)->dir.lst((s), (i), (a), (p)); (i)->valid; (s)->dir.nxt((s), (i), (a)))
  void (*lst)(struct sys *s, struct sys_dir_iter *it, struct arena *a, struct str path);
  void (*nxt)(struct sys *s, struct sys_dir_iter *it, struct arena *a);
  int (*exists)(struct sys *s, struct str path, struct arena *tmp);
};
struct sys_clip_api {
  void (*set)(struct str s, struct arena *a);
  struct str (*get)(struct arena *a);
};
struct sys_time_api {
  unsigned long long (*timestamp)(void);
};
struct sys_con_api {
  void(*log)(const char *fmt, ...);
  void(*warn)(const char *fmt, ...);
  void(*err)(const char *fmt, ...);
};
struct sys_rnd_api {
  uintptr_t gen;
  uintptr_t(*open)(void);
  void(*close)(uintptr_t hdl);
  unsigned (*gen32)(uintptr_t hdl);
  unsigned long long(*gen64)(uintptr_t hdl);
  void(*gen128)(uintptr_t hdl, void *dst);
};

/* platform */
#define SYS_MAX_INPUT 256
struct sys {
  int quit;
  int running;
  unsigned seq;
  int version;

  struct cpu_info cpu;
  enum sys_cur_style cursor;
  struct sys_win win;
  struct sys_dnd dnd;
  struct sys_tooltip tooltip;

  /* style */
  unsigned has_style:1;
  unsigned style_mod:1;
  unsigned col[SYS_COL_CNT];
  float fnt_pnt_size;
  float dpi_scale;

  /* modules */
  void *platform;
  void *ren;

  /* api */
  struct sys_mem_api mem;
  struct sys_dir_api dir;
  struct sys_file_api file;
  struct sys_clip_api clipboard;
  struct sys_time_api time;
  struct sys_con_api con;
  struct sys_rnd_api rnd;
  struct gfx_api gfx;

  /* poll */
  unsigned key_mod:1;
  unsigned dnd_mod:1;
  unsigned btn_mod:1;
  unsigned txt_mod:1;
  unsigned mouse_mod:1;
  unsigned mouse_grap:1;
  unsigned scrl_mod:1;
  unsigned resized:1;
  unsigned drw:1;
  unsigned repaint:1;

  /* input */
  unsigned keymod;
  unsigned focus;
  struct sys_mouse mouse;
  unsigned long keys[bits_to_long(SYS_KEY_CNT)];
  int txt_len;
  char txt[SYS_MAX_INPUT];
};
struct sys_api {
  int version;
  int(*init)(struct sys *s);
  int(*pull)(struct sys *s);
  void(*push)(struct sys *s);
  void(*shutdown)(struct sys *s);
};
extern void sys_api(void *export, void *import);

