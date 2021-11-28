#define PCK_VERISON 1

struct arena;
struct gui_api;
struct gui_ctx;
struct gui_panel;
struct file_view;

struct file_picker_api {
  int version;
  struct file_view* (*init)(struct sys *sys, struct gui_ctx *ctx, struct arena *mem, struct arena *tmp);
  void (*update)(struct file_view*, struct sys *sys);
  void (*shutdown)(struct file_view*, struct sys *sys);
  int (*ui)(char **filepath, struct file_view*, struct gui_ctx *ctx,
            struct gui_panel *pan, struct gui_panel *parent);
};
static void pck_get_api(void *export, void *import);

