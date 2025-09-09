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

enum {
  APP_MAX_VAR       = 256,
  APP_MAX_VAR_FLTR  = 32,
};
enum app_view_state {
  APP_VIEW_STATE_FILE,
  APP_VIEW_STATE_DB,
  APP_VIEW_STATE_VARS,
  APP_VIEW_STATE_CNT,
};
struct app_view {
  unsigned short state;
  unsigned short last_state;
  struct str file_path;
  struct db_state db;
};
enum var_type {
  VAR_BOOL,
  VAR_INT,
  VAR_UINT,
  VAR_FLT,
};
union var_val {
  int b;
  int i;
  unsigned u;
  float f;
};
struct var {
  unsigned type:30;
  unsigned paq:1;
  unsigned mod:1;
  struct str name;
  union var_val val;
  union var_val min;
  union var_val max;
};
enum app_tbl_var_col_def {
  APP_TBL_VAR_NAME,
  APP_TBL_VAR_VAL,
  APP_TBL_VAR_CNT,
};
struct app {
  struct res res;
  struct gui_ctx gui;
  struct file_view fs;
  struct db_view db;

  /* vars */
  struct tbl(struct var*, APP_MAX_VAR) vars;
  struct str fnd_str;
  char fnd_buf[APP_MAX_VAR_FLTR];
  int var_tbl_state[GUI_TBL_CAP(APP_TBL_VAR_CNT)];
  int var_tbl_off[2];

  /* views */
  struct app_view view;
  char path_buf[MAX_FILE_PATH];
  char db_mem[CFG_DB_MAX_MEMORY];
  char gui_mem[CFG_GUI_MAX_MEMORY];
};
static struct var app_col_style = {.type = VAR_INT,   .name = strv("app/color_style"),  .val = {.i = CFG_COLOR_SCHEME}};
static struct var app_col_text  = {.type = VAR_UINT,  .name = strv("app/text_color"),   .val = {.u = 0xFFE0E0E0}};

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

struct app_tbl_col_def {
  struct str title;
  struct gui_split_lay_slot ui;
};
static const struct app_tbl_col_def app_tbl_var_def[DB_TBL_FLTR_COL_MAX] = {
  [APP_TBL_VAR_NAME]  = {.title = strv("Name"),   .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {100, 1000}}},
  [APP_TBL_VAR_VAL]   = {.title = strv("Value"),  .ui = {.type = GUI_LAY_SLOT_DYN, .size = 1, .con = {100, 1000}}},
};
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
app_var_reg(struct app *not_null app, struct var *not_null var) {
  requires(app_is_val(app));
  requires(var);
  requires(app->vars.cnt < APP_MAX_VAR);

  unsigned long long key = str_hash(var->name);
  tbl_put(&app->vars, key, &var);
}
static struct var*
app_var_fnd(struct app *not_null app, struct str name) {
  requires(app_is_val(app));
  requires(str__is_val(&name));

  unsigned long long key = str_hash(name);
  int idx = tbl_fnd(&app->vars, key);
  struct var **var = tbl_get(&app->vars, idx);
  return var ? *var : 0;
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

  app_var_reg(app, &app_col_style);
  app_var_reg(app, &app_col_text);

  struct gui_split_lay_cfg tab_cfg = {0};
  tab_cfg.size = szof(struct app_tbl_col_def);
  tab_cfg.off = offsetof(struct app_tbl_col_def, ui);
  tab_cfg.cnt = APP_TBL_VAR_CNT;
  tab_cfg.slots = app_tbl_var_def;
  gui.tbl.lay(app->var_tbl_state, &app->gui, &tab_cfg);

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
static int
app_sort_var(const void *lhs, const void *rhs) {
  const struct var * const *lv = (const struct var* const*)lhs;
  const struct var * const *rv = (const struct var* const*)rhs;
  return str_cmp((*lv)->name, (*rv)->name);
}
static void
ui_app_var_lst(struct app *not_null app,
               struct gui_ctx *not_null ctx,
               struct gui_panel *not_null pan,
               struct gui_panel *not_null parent) {

  requires(ctx);
  requires(pan);
  requires(parent);
  requires(app_is_val(app));

  gui.pan.begin(ctx, pan, parent);
  {
    /* search */
    int gap = ctx->cfg.gap[1];
    struct gui_box lay = pan->box;
    struct gui_panel fltr = {.box = gui.cut.top(&lay, ctx->cfg.item, gap)};
    struct gui_btn back = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
    struct gui_edit_box edt = {.box = fltr.box};
    app->fnd_str = gui.edt.box(ctx, &edt, pan, arrv(app->fnd_buf), app->fnd_str);

    /* setup list */
    int cnt = 0;
    struct var *lst[APP_MAX_VAR];
    for tbl_loop(n,i, &app->vars) {
      struct var *var = tbl_unref(&app->vars, n, 0);
      if (!str_len(app->fnd_str) ||
          str_has(var->name, app->fnd_str)) {
        lst[cnt++] = var;
      }
    }
    qsort(lst, castsz(cnt), sizeof(lst[0]), app_sort_var);

    /* table */
    struct gui_tbl tbl = {.box = lay};
    gui.tbl.begin(ctx, &tbl, pan, app->var_tbl_off, 0);
    {
      /* header */
      const struct app_tbl_col_def *col = 0;
      int tbl_cols[GUI_TBL_COL(APP_TBL_VAR_CNT)];
      gui.tbl.hdr.begin(ctx, &tbl, arrv(tbl_cols), arrv(app->var_tbl_state));
      for arr_eachv(col, app_tbl_var_def) {
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_cols, app->var_tbl_state, col->title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, cnt);
      cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.sel.mode = GUI_LST_SEL_SINGLE;
      cfg.sel.on = GUI_LST_SEL_ON_HOV;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for gui_tbl_lst_loopn(i, _, gui, &tbl, APP_MAX_VAR) {
        struct var *var = lst[i];
        unsigned long long hash = str_hash(var->name);

        struct gui_panel item = {0};
        gui.tbl.lst.elm.begin(ctx, &tbl, &item, gui_id64(hash), 0);
        {
          gui.tbl.lst.elm.col.txt_ico(ctx, &tbl, tbl_cols, &item, var->name, RES_ICO_COG);
          switch (var->type) {
          case VAR_BOOL: {
            struct gui_panel tog = {0};
            gui.tbl.lst.elm.col.slot(&tog.box, ctx, &tbl, tbl_cols);
            gui.tog.ico(ctx, &tog, &item, &var->val.b);
          } break;
          case VAR_INT: {
            struct gui_spin spin = {0};
            gui.tbl.lst.elm.col.slot(&spin.box, ctx, &tbl, tbl_cols);
            gui.spin.i(ctx, &spin, &item, &var->val.i);
          } break;
          case VAR_UINT: {
            struct gui_spin spin = {0};
            gui.tbl.lst.elm.col.slot(&spin.box, ctx, &tbl, tbl_cols);
            gui.spin.u(ctx, &spin, &item, &var->val.u);
          } break;
          case VAR_FLT: {
            struct gui_spin spin = {0};
            gui.tbl.lst.elm.col.slot(&spin.box, ctx, &tbl, tbl_cols);
            gui.spin.f(ctx, &spin, &item, &var->val.f);
          } break;}
        }
        gui.tbl.lst.elm.end(ctx, &tbl, &item);
      }
      gui.tbl.lst.end(ctx, &tbl);
    }
    gui.tbl.end(ctx, &tbl, pan, app->var_tbl_off);

    gui.btn.txt(ctx, &back, pan, strv("Back"), 0);
    if (back.clk) {
      app->view.state = app->view.last_state;
    }
  }
  gui.pan.end(ctx, pan, parent);
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
    } break;
    case APP_VIEW_STATE_VARS: {
      ui_app_var_lst(app, ctx, pan, parent);
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
    if (bit_tst_clr(sys->keys, SYS_KEY_F1)) {
      app->view.last_state = app->view.state;
      app->view.state = APP_VIEW_STATE_VARS;
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

