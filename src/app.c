/* std */
#include <assert.h>
#include <stddef.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* sys */
#include "sys/cpu.h"
#include "lib/fmt.h"
#include "lib/std.h"
#include "sys/dbg.h"
#include "sys/ren.h"
#include "sys/sys.h"

/* app */
#include "res.h"
#include "gui.h"
#include "pck.h"
#include "dbs.h"

/* -----------------------------------------------------------------------------
 *                                  App
 * ---------------------------------------------------------------------------*/
struct app;
struct gui_app_key {
  int code;
  unsigned mod;
};
struct app_ui_shortcut {
  struct gui_app_key key;
  struct gui_app_key alt;
};
enum app_key_id {
  APP_KEY_NONE,
  APP_KEY_CNT
};
enum app_op_id {
  APP_OP_QUIT,
  APP_OP_CNT
};
union app_param {
  int i;
  float f;
  const char *s;
  void *p;
  enum app_key_id k;
};
typedef void(*app_op_f)(struct app*, const union app_param*);
struct app_op {
  struct gui_app_key key;
  struct gui_app_key alt;
  app_op_f handler;
  union app_param arg;
};
enum app_state {
  APP_STATE_FILE,
  APP_STATE_DB,
};
struct app {
  struct res res;
  struct gui_ctx gui;

  int quit;
  enum app_state state;
  unsigned long ops[bits_to_long(APP_KEY_CNT)];

  dyn(char) file_path;
  struct file_view *fs;
  struct db_ui_view *db;
};
static void app_op_quit(struct app* app, const union app_param *arg);

#include "cfg.h"
#include "lib/fmt.c"
#include "lib/std.c"

static struct res_api res;
static struct gui_api gui;
static struct file_picker_api file;
static struct db_api db;

/* =============================================================================
 *
 *                                  App
 *
 * =============================================================================
 */
extern void dlEntry(struct sys *s);
extern void dlRegister(struct sys *sys);

static int
app_on_mod(unsigned mod, unsigned keymod) {
  if (mod == 0) {
    return 1;
  } else if (mod == (unsigned)-1) {
    return keymod == 0;
  } else {
    return keymod == mod;
  }
}
static void
app_op_quit(struct app *app, const union app_param *arg) {
  unused(arg);
  app->quit = 1;
}
static void
app_init(struct app *app, struct sys *sys) {
  assert(app);
  assert(sys);

  app->res.sys = sys;
  app->gui.sys = sys;
  app->gui.res = &app->res;

  res.init(&app->res);
  gui.init(&app->gui, sys->mem.arena, CFG_COLOR_SCHEME);

  app->fs = file.init(sys, &app->gui, sys->mem.arena, sys->mem.tmp);
  app->file_path = arena_dyn(sys->mem.arena, sys, char, 256);
}
static void
app_shutdown(struct app *app, struct sys *sys) {
  assert(app);
  assert(sys);

  file.shutdown(app->fs, sys);
  db.shutdown(app->db, sys);
}
static void
app_ui_main(struct app *app, struct gui_ctx *ctx, struct gui_panel *pan,
            struct gui_panel *parent) {
  assert(app);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_panel bdy = {.box = pan->box};
    switch (app->state) {
    case APP_STATE_FILE: {
      struct sys *sys = ctx->sys;
      if (file.ui(&app->file_path, app->fs, ctx, &bdy, pan)) {
        app->db = db.init(&app->gui, sys->mem.arena, sys->mem.tmp, dyn_str(app->file_path));
        app->state = APP_STATE_DB;
      }
    } break;
    case APP_STATE_DB:
      db.ui(app->db, ctx, &bdy, pan);
      break;
    }
  }
  gui.pan.end(ctx, pan, parent);
}
extern void
dlRegister(struct sys *sys) {
  assert(sys);
  sys->plugin.add(&res, 0, strv("res"));
  if (res.version != RES_VERSION) {

  }
  sys->plugin.add(&gui, &res, strv("gui"));
  if (gui.version != GUI_VERSION) {

  }
  sys->plugin.add(&file, &gui, strv("pck"));
  if (file.version != PCK_VERISON) {

  }
  sys->plugin.add(&db, &gui, strv("dbs"));
  if (db.version != DBS_VERISON) {

  }
}
extern void
dlEntry(struct sys *sys) {
  struct app *app = sys->app;
  if (!sys->app) {
    sys->app = arena_obj(sys->mem.arena, sys, struct app);
    app = sys->app;
    app_init(app, sys);
  }
  if (sys->quit) {
    app_shutdown(app, sys);
    return;
  }
#ifdef SYS_LINUX
  gui.color_scheme(&app->gui, GUI_COL_SCHEME_DARK);
#else
  if (sys->col_mod) {
    gui.color_scheme(&app->gui, CFG_COLOR_SCHEME);
  }
#endif
  memset(app->ops, 0, sizeof(app->ops));
  const struct app_op *op = 0;
  for_arrv(op, app_ops) {
    /* handle app shortcuts */
    if ((bit_tst(sys->keys, op->key.code) && sys->keymod == op->key.mod) ||
        (bit_tst(sys->keys, op->alt.code) && sys->keymod == op->alt.mod))
      op->handler(app, &op->arg);
  }
  fori_arrv(i, app_ui_key_tbl) {
    /* map system keys to ui shortcuts */
    const struct app_ui_shortcut *s = app_ui_key_tbl + i;
    struct gui_ctx *ctx = &app->gui;
    int keymod = app_on_mod(s->key.mod, sys->keymod);
    if (bit_tst(sys->keys, s->key.code) && keymod) {
      bit_set(ctx->keys, i);
    } else if (bit_tst(sys->keys, s->alt.code) && keymod) {
      bit_set(ctx->keys, i);
    }
  }
  /* update */
  switch (app->state) {
  case APP_STATE_FILE:
    file.update(app->fs, sys);
    break;
  case APP_STATE_DB:
    db.update(app->db, sys);
    break;
  }
  /* gui */
  dbg_blk_begin(sys, "app:gui");
  while (gui.begin(&app->gui)) {
    dbg_blk_begin(sys, "app:gui:pass");
    struct gui_panel pan = {.box = app->gui.box};
    app_ui_main(app, &app->gui, &pan, &app->gui.root);
    gui.end(&app->gui);
    dbg_blk_end(sys);
  }
  dbg_blk_end(sys);
}

