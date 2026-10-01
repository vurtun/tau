#define RES_VERSION 1

enum {
  RES_GLYPH_SLOTS     = 256,
  RES_IMG_SLOT_CNT    = 64,
};
enum res_fnt_mode {
  RES_FNT_RUN_FULL,
  RES_FNT_RUN_WORDS,
};
enum res_ico_id {
  RES_ICO_FOLDER,
  RES_ICO_FOLDER_OPEN,
  RES_ICO_COG,
  RES_ICO_FILE,
  RES_ICO_FILE_TXT,
  RES_ICO_CLOSE,
  RES_ICO_EXPAND,
  RES_ICO_COLLAPSE,
  RES_ICO_NO,
  RES_ICO_CHECK,
  RES_ICO_MODIFY,
  RES_ICO_FILE_PDF,
  RES_ICO_FILE_IMG,
  RES_ICO_FILE_ARCHIVE,
  RES_ICO_FILE_AUDIO,
  RES_ICO_FILE_VIDEO,
  RES_ICO_FILE_CODE,
  RES_ICO_MENU,
  RES_ICO_TH_LIST,
  RES_ICO_PLUS,
  RES_ICO_TAG,
  RES_ICO_BOX_LIST,
  RES_ICO_POINT_LIST,
  RES_ICO_NUM_LIST,
  RES_ICO_SORT_LIST,
  RES_ICO_MINUS,
  RES_ICO_IMAGE,
  RES_ICO_HOME,
  RES_ICO_EDIT,
  RES_ICO_DATABASE,
  RES_ICO_SEARCH,
  RES_ICO_TRASH,
  RES_ICO_CUBE = 128,
  RES_ICO_CUBES,
  RES_ICO_SORT_REV,
  RES_ICO_SORT_ALPHA,
  RES_ICO_SORT_ALPHA_REV,
  RES_ICO_SORT_NUM,
  RES_ICO_SORT_NUM_REV,
  RES_ICO_GRAPH,
  RES_ICO_BAR_CHART,
  RES_ICO_AREA_CHART,
  RES_ICO_PIE_CHART,
  RES_ICO_LINE_CHART,
  RES_ICO_PASTE,
  RES_ICO_CUT,
  RES_ICO_CROP,
  RES_ICO_SPLIT,
  RES_ICO_TABLE,
  RES_ICO_EXPR,
  RES_ICO_IMPORT,
  RES_ICO_CALENDAR,
  RES_ICO_LINK,
  RES_ICO_LOCK,
  RES_ICO_UNLOCK,
  RES_ICO_KEY,
  RES_ICO_FONT,
  RES_ICO_BOLT,
  RES_ICO_ADDRESS_BOOK,
  RES_ICO_TOGGLE_OFF,
  RES_ICO_TOGGLE_ON,
  RES_ICO_FOLDER_ADD,
  RES_ICO_CALCULATOR,
  RES_ICO_MAX
};
struct res_glyph {
  int sx, sy;
  int x0, y0;
  int x1, y1;
  int adv;
};
struct res_fnt {
  float size;
  int space_adv;

  int txt_height;
  int ico_height;

  int texid;
  fnt_packedchar glyphs[RES_GLYPH_SLOTS];
};
struct res_txt_bnd {
  int len, width;
  const char *end;
};
#define RES_FNT_MAX_RUN 16

/* Single batch of glyphs for layout/rendering */
struct res_fnt_run {
  int len;
  unsigned short adv[RES_FNT_MAX_RUN];
  unsigned char off[RES_FNT_MAX_RUN];
  unsigned char ext[RES_FNT_MAX_RUN * 2];
  signed char pad[RES_FNT_MAX_RUN * 2];
  unsigned short coord[RES_FNT_MAX_RUN * 2];
};
/* Self-contained iterator holding the current batch */
struct res_fnt_run_it {
  struct str at;
  struct str rest;
  struct res_fnt_run run;
};
struct res {
  enum res_fnt_mode mod;
  float fnt_pnt_size;
  struct sys *sys;
  struct res_fnt fnt;
};

/* API */
#define res_run_loop(run, it, api, rss, txt) \
  (const struct res_fnt_run *(run) = (api)->run.begin((it), (rss), (txt)); \
   (run); (run) = (api)->run.nxt((it), (rss)))
struct res_fnt_api {
  void (*ext)(int *ext, struct res *res, struct str txt);
  void (*fit)(struct res_txt_bnd *bnd, struct res *r, int space, struct str txt);
  void (*glyph)(struct res_glyph *ret, const struct res_fnt *fnt, int x, int y, int in_rune);
};
struct res_run_api {
  const struct res_fnt_run* (*begin)(struct res_fnt_run_it *it, struct res *r, struct str txt);
  const struct res_fnt_run* (*nxt)(struct res_fnt_run_it *it, struct res *r);
  void (*glyph)(struct res_glyph *ret, const struct res_fnt_run *run, int i, int x, int y);
};
struct res_ico_api {
  void (*ext)(int *ret, const struct res *res, enum res_ico_id ico);
};
struct res_api {
  int version;
  struct res_fnt_api fnt;
  struct res_run_api run;
  struct res_ico_api ico;
  void (*full_text_mode)(struct res *rss);
  void (*word_mode)(struct res *rss);
  int (*init)(struct res *res, struct sys *sys);
  void (*shutdown)(struct res *r);
};

static void res_api(void *exp, void *imp);
