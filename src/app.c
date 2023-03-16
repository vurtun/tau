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
#include "sys/ren.h"
#include "sys/sys.h"

/* app */
#include "res.h"
#include "gui.h"
#include "app.h"

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
  APP_OP_PROFILE,
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
struct app {
  struct res res;
  struct gui_ctx gui;
  struct sys *sys;
  int quit;
  unsigned long ops[bits_to_long(APP_KEY_CNT)];
};
static void app_op_quit(struct app* app, const union app_param *arg);

#include "cfg.h"
#include "lib/fmt.c"
#include "lib/std.c"
#include "lib/math.c"

static struct res_api res;
static struct gui_api gui;

#include "lib/fnt.c"
#include "res.c"
#include "gui.c"

/* =============================================================================
 *
 *                                  App
 *
 * =============================================================================
 */
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
  assert(app);
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

  struct res_args args;
  args.run_cnt = 2 * 1024;
  args.hash_cnt = 4 * 1024;

  res.init(&app->res, &args);
  enum gui_col_scheme style;
  if (sys->has_style){
    style = GUI_COL_SCHEME_SYS;
  } else {
    style = GUI_COL_SCHEME_DARK;
  }
  gui.init(&app->gui, sys->mem.arena, style);
}
static void
app_shutdown(struct app *app, struct sys *sys) {
  unused(app);
  unused(sys);
}
static void
ui_app_main(struct app *app, struct gui_ctx *ctx, struct gui_panel *pan,
            struct gui_panel *parent) {
  assert(app);
  assert(ctx);
  assert(pan);
  assert(parent);
  unused(app);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_btn btn = {0};
    btn.box.x = gui.bnd.min_ext(50, 100);
    btn.box.y = gui.bnd.min_ext(50, ctx->cfg.item);
    if (gui.btn.txt(ctx, &btn, parent, strv("Button"), 0)) {
      printf("test\n");
    }
  }
  gui.pan.end(ctx, pan, parent);
}
extern void
app_on_api(struct sys *sys) {
  assert(sys);
  unused(sys);
  res_get_api(&res, 0);
  gui_get_api(&gui, &res);

#ifndef RELEASE_MODE
  /* run unit tests */
  ut_str(sys);
  ut_set(sys);
  ut_tbl(sys);
#endif
}
extern void
app_run(struct sys *sys) {
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
  if (sys->has_style && sys->style_mod) {
    gui.color_scheme(&app->gui, GUI_COL_SCHEME_SYS);
  }
  mset(app->ops, 0, sizeof(app->ops));
  app->sys = sys;

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
  /* gui */
  while (gui.begin(&app->gui)) {
    struct gui_panel pan = {.box = app->gui.box};
    ui_app_main(app, &app->gui, &pan, &app->gui.root);
    gui.end(&app->gui);
  }
}

