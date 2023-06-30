#define DBS_VERISON 1

struct arena;
struct gui_api;
struct gui_ctx;
struct gui_panel;
struct db_ui_view;

struct db_api {
  int version;
  int (*init)(struct mem_blk *blk);
  struct db_ui_view* (*new)(struct gui_ctx *ctx, struct arena *mem, struct arena *tmp_mem, struct str path);
  void (*update)(struct db_ui_view *db, struct sys *sys);
  void (*del)(struct db_ui_view *db, struct sys *sys);
  void (*ui)(struct db_ui_view *db, struct gui_ctx *ctx, struct gui_panel *pan, struct gui_panel *parent);
};


