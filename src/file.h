#define APP_FILE_VERISON 1

struct arena;
struct gui_api;
struct gui_ctx;
struct gui_panel;

struct file_picker_api {
  int version;
  void (*init)(struct sys *sys, struct gui_ctx *ctx, struct arena *mem, struct arena *tmp);
  void (*update)(struct sys *sys);
  void (*shutdown)(struct sys *sys);
  int (*ui)(char **filepath, struct gui_ctx *ctx, struct gui_panel *pan,
            struct gui_panel *parent);
};

