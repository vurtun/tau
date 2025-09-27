#define _GNU_SOURCE

/* std */
#include <assert.h>
#include <stddef.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* app */
#include "sys/cpu.h"
#include "lib/fmt.h"
#include "lib/std.h"
#include "lib/rpq.h"
#include "lib/fnt.h"
#include "lib/sql.h"
#include "sys/gfx.h"
#include "sys/sys.h"

#include "res.h"
#include "gui.h"
#include "cfg.h"
#include "pck.h"
#include "dbs.h"
#include "app.h"

enum app_view_state {
  APP_VIEW_STATE_FILE,
  APP_VIEW_STATE_DB,
  APP_VIEW_STATE_CNT,
};
struct app_view {
  unsigned short state;
  unsigned short last_state;
  struct str file_path;
  struct db_state db;
};
struct app {
  struct res res;
  struct gui_ctx gui;
  struct file_view fs;
  struct db_view db;

  /* views */
  struct app_view view;
  char path_buf[MAX_FILE_PATH];
  char db_mem[CFG_DB_MAX_MEMORY];
  char gui_mem[CFG_GUI_MAX_MEMORY];
};
static struct res_api res;
static struct gui_api gui;
static struct pck_api pck;
static struct db_api dbs;
static struct app g_app;

#include "lib/fmt.c"
#include "lib/std.c"
#include "lib/math.c"
#include "lib/rpq.c"
#include "lib/fnt.c"
#include "lib/sql.c"

#include "res.c"
#include "gui.c"
#include "pck.c"
#include "dbs.c"

/* -----------------------------------------------------------------------------
 *                                  App
 * ---------------------------------------------------------------------------*/
static inline int
app_is_val(struct app *not_null app) {
  unused(app);
  assert(app);
  assert(app->view.state >= APP_VIEW_STATE_FILE ||
    app->view.state < APP_VIEW_STATE_CNT || app->view.state == 0);
  assert(app->view.last_state >= APP_VIEW_STATE_FILE ||
    app->view.last_state < APP_VIEW_STATE_CNT ||
    app->view.last_state == 0);
  assert(str_is_val(app->view.file_path));
  return 1;
}
static void
app_view_init(struct app_view *not_null view) {
  requires(view);
  view->state = APP_VIEW_STATE_FILE;
  view->last_state = view->state;
  view->db.con = 0;
}
static int
app_view_setup(struct app *not_null app,
               struct app_view *not_null view,
               struct str path) {

  requires(app_is_val(app));
  view->file_path = str_set(app->path_buf, MAX_FILE_PATH, path);
  if (str_is_inv(view->file_path)) {
    view->state = APP_VIEW_STATE_FILE;
    view->file_path = str_nil;
    assert(app_is_val(app));
    return -1;
  }
  view->state = APP_VIEW_STATE_DB;
  if (!dbs.setup(&app->view.db, &app->gui, path)) {
    view->state = APP_VIEW_STATE_FILE;
    view->file_path = str_nil;
    assert(app_is_val(app));
    return -1;
  }
  ensures(app_is_val(app));
  return 0;
}
static int
app_open_file(struct app *not_null app, struct str file) {
  requires(app_is_val(app));
  if (app->view.db.con) {
    dbs.del(&app->view.db);
  }
  app_view_init(&app->view);
  int ret = app_view_setup(app, &app->view, file);
  ensures(app_is_val(app));
  return ret;
}
static void
app_init(struct app *not_null app,
         struct sys *not_null sys) {
  requires(sys);
  requires(app_is_val(app));
  res.init(&app->res, sys);
  {
    struct gui_args args = {0};
    args.scale = castu(sys->dpi_scale * castf(1 << 16u));
    if (sys->has_style){
      args.scm = GUI_COL_SCHEME_SYS;
    } else {
      args.scm = GUI_COL_SCHEME_DARK;
    }
    app->gui.sys = sys;
    app->gui.res = &app->res;
    app->gui.vtx_buf = app->gui_mem;
    app->gui.idx_buf = app->gui_mem + CFG_GUI_VTX_MEMORY;
    app->gui.vtx_buf_siz = CFG_GUI_VTX_MEMORY;
    app->gui.idx_buf_siz = CFG_GUI_IDX_MEMORY;
    gui.init(&app->gui, &args);
  }
  app_view_init(&app->view);

  int ret = pck.init(&app->fs, sys, &app->gui);
  if (ret < 0) {
    sys->con.err("[app] failed to initialize file picker!\n");
    exit(1);
  }
  dbs.init(app->db_mem, szof(app->db_mem));
#ifdef DEBUG_MODE
  ut_bit();
  ut_str();
  ut_utf();
  ut_str2();
  ut_tbl();
#endif
  ensures(app_is_val(app));
}
static void
app_shutdown(struct app *not_null app,
             struct sys *not_null sys) {
  requires(sys);
  requires(app_is_val(app));
  if (app->view.db.con) {
    dbs.del(&app->view.db);
  }
  pck.shutdown(&app->fs, sys);
  res.shutdown(&app->res);
}

/* -----------------------------------------------------------------------------
 *
 *                                  GUI
 *
 * -----------------------------------------------------------------------------
 */
static void
ui_app_file_view(struct app *not_null app,
                 struct app_view *not_null view,
                 struct gui_ctx *not_null ctx,
                 struct gui_panel *not_null pan,
                 struct gui_panel *not_null parent) {

  requires(app_is_val(app));
  requires(ctx);
  requires(pan);
  requires(parent);
  requires(ctx->sys);

  view->file_path = pck.ui(app->path_buf, MAX_FILE_PATH, &app->fs, ctx, pan, parent);
  if (str_is_val(view->file_path)) {
    struct sys *sys = ctx->sys;
    if (app_view_setup(app, &app->view, view->file_path) >= 0) {
      view->state = APP_VIEW_STATE_DB;
    }
    sys->repaint = 1;
  } else {
    view->file_path = str_nil;
  }
  ensures(app_is_val(app));
}
static void
ui_app_dnd_file(struct app *not_null app,
                struct gui_ctx *not_null ctx,
                struct gui_panel *not_null pan) {
  requires(app_is_val(app));
  requires(ctx);
  requires(pan);

  if (gui.dnd.dst.begin(ctx, pan)) {
    struct gui_dnd_paq *paq = gui.dnd.dst.get(ctx, GUI_DND_SYS_FILES);
    if (paq) {
      /* file drag & drop */
      const struct str *file_urls = paq->data;
      switch (paq->state) {
      case GUI_DND_LEFT: break;
      case GUI_DND_ENTER:
      case GUI_DND_PREVIEW: {
        paq->response = GUI_DND_ACCEPT;
      } break;
      case GUI_DND_DELIVERY: {
        for loop(i, paq->size) {
          int ret = app_open_file(app, file_urls[i]);
          if (ret == 0) {
            break;
          }
        }
        paq->response = GUI_DND_ACCEPT;
      } break;}
    }
    gui.dnd.dst.end(ctx);
  }
  ensures(app_is_val(app));
}
static void
ui_app_main(struct app *not_null app,
            struct gui_ctx *not_null ctx,
            struct gui_panel *not_null pan,
            struct gui_panel *not_null parent) {

  requires(ctx);
  requires(pan);
  requires(parent);
  requires(app_is_val(app));

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_panel bdy = {.box = pan->box};
    gui.pan.begin(ctx, &bdy, pan);
    switch (app->view.state) {
    case APP_VIEW_STATE_FILE: {
      ui_app_file_view(app, &app->view, ctx, &bdy, pan);
    } break;
    case APP_VIEW_STATE_DB: {
      dbs.ui(&app->view.db, &app->db, ctx, &bdy, pan);
    } break;}
    gui.pan.end(ctx, &bdy, pan);
  }
  gui.pan.end(ctx, pan, parent);
  ui_app_dnd_file(app, ctx, pan);
  ensures(app_is_val(app));
}

/* -----------------------------------------------------------------------------
 *                                  Main
 * -----------------------------------------------------------------------------
 */
static int
app_on_mod(unsigned mod, unsigned keymod) {
  if (mod == 0) {
    return 1;
  }
  if (mod == (unsigned)-1) {
    return keymod == 0;
  }
  return keymod == mod;
}
extern void
app_run(struct sys *sys) {
  ensures(sys);
  switch (sys->op) {
  case SYS_SETUP: {
    /* setup OS window */
    sys->win.title = "Tau";
    sys->win.x = -1;
    sys->win.y = -1;
    sys->win.w = CFG_WIN_WIDTH;
    sys->win.h = CFG_WIN_HEIGHT;
    sys->win.min_w = CFG_WIN_MIN_WIDTH;
    sys->win.min_h = CFG_WIN_MIN_HEIGHT;
    sys->win.max_w = CFG_WIN_MAX_WIDTH;
    sys->win.max_h = CFG_WIN_MAX_HEIGHT;
    sys->gfx.clear_color = col_black;
  } break;

  case SYS_INIT: {
    /* init */
    res_api(&res, 0);
    gui_api(&gui, 0);
    pck_api(&pck, 0);
    db_api(&dbs, 0);

    struct app *app = &g_app;
    app_init(app, sys);
    sys->app = app;
    if (sys->argc > 1) {
      app_open_file(app, sys->argv[1]);
    }
  } break;

  case SYS_RUN: {
    /* user interface */
    assert(sys->app);
    struct app *app = cast(struct app*, sys->app);
    if (sys->style_mod) {
      gui.color_scheme(&app->gui, CFG_COLOR_SCHEME);
    }
    for arr_loopv(i, app_ui_key_tbl) {
      /* map system keys to ui shortcuts */
      struct gui_ctx *ctx = &app->gui;
      const struct app_ui_shortcut *sck = app_ui_key_tbl + i;
      int keymod = app_on_mod(sck->key.mod, sys->keymod);
      if ((bit_tst(sys->keys, sck->key.code) && keymod) ||
          (bit_tst(sys->keys, sck->alt.code) && keymod)) {
        bit_set(ctx->keys, i);
      }
    }
    /* run app ui */
    for gui_loop(_, &gui, &app->gui) {
      struct gui_panel pan = {.box = app->gui.box};
      ui_app_main(app, &app->gui, &pan, &app->gui.root);
    }
  } break;

  case SYS_QUIT: {
    /* shutdown */
    app_shutdown(sys->app, sys);
    sys->app = 0;
  } break;
  }
}

