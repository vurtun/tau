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
enum app_state {
  APP_STATE_FILE,
  APP_STATE_DB,
  APP_STATE_PROFILER,
};
struct app_view {
  enum app_state state;
  enum app_state last_state;
  struct lst_elm hook;

  dyn(char) file_path;
  struct db_ui_view *db;
};
struct app {
  struct res res;
  struct gui_ctx gui;
  struct sys *sys;

  int quit;
  unsigned long ops[bits_to_long(APP_KEY_CNT)];
  struct file_view *fs;

  /* views */
  int show_tab_lst;
  double tab_lst_off[2];
  dyn(struct app_view*) views;
  struct lst_elm del_lst;
  int sel_tab;
};
static void app_op_quit(struct app* app, const union app_param *arg);
static void app_op_profiler(struct app* app, const union app_param *arg);

#include "cfg.h"
#include "lib/fmt.c"
#include "lib/std.c"

static struct res_api res;
static struct gui_api gui;
static struct file_picker_api file;
static struct db_api db;

#ifdef RELEASE_MODE
#include "lib/fnt.c"
#include "lib/img.h"
#include "lib/img.c"
#include "lib/sql.h"
#include "lib/sql.c"
#include "res.c"
#include "gui.c"
#include "pck.c"
#include "dbs.c"
#endif

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
  assert(app);
  unused(arg);
  app->quit = 1;
}
static void
app_op_profiler(struct app* app, const union app_param *arg) {
  assert(app);
  unused(arg);

  struct sys *sys = app->sys;
  sys->dbg.disable(sys);

  struct app_view *view = app->views[app->sel_tab];
  view->last_state = view->state;
  view->state = APP_STATE_PROFILER;
}
static struct app_view*
app_view_new(struct app *app, struct sys *sys) {
  assert(app);
  assert(sys);

  struct app_view *s = 0;
  if (lst_any(&app->del_lst)) {
    s = lst_get(lst_first(&app->del_lst), struct app_view, hook);
    lst_del(app->del_lst.nxt);
  } else {
    s = arena_alloc(sys->mem.arena, sys, szof(*s));
  }
  lst_init(&s->hook);
  return s;
}
static void
app_view_del(struct app *app, struct app_view *view, struct sys* sys) {
  assert(app);
  assert(sys);
  assert(view);

  dyn_clr(view->file_path);
  lst_init(&view->hook);
  lst_add(&app->del_lst, &view->hook);
}
static void
app_view_setup(struct app_view *view, struct sys *sys) {
  assert(sys);
  assert(view);

  view->state = APP_STATE_FILE;
  view->last_state = view->state;
  if (!view->file_path) {
    view->file_path = arena_dyn(sys->mem.arena, sys, char, 256);
  }
}
static void
app_view_init(struct app_view *view, struct sys *sys, struct str path) {
  app_view_setup(view, sys);
  dyn_asn_str(view->file_path, sys, path);
  view->state = APP_STATE_DB;
}
static void
app_open_files(struct app *app, struct sys *sys, const struct str *files, int cnt) {
  int i = 0;
  assert(app);
  assert(sys);
  assert(files);
  if (cnt && app->views[app->sel_tab]->state == APP_STATE_FILE) {
    /* open first database in place */
    struct app_view *view = app->views[app->sel_tab];
    for (; i < cnt && !view->db; ++i) {
      view->db = db.init(&app->gui, sys->mem.arena, sys->mem.tmp, files[i]);
      if (view->db) {
        app_view_init(view, sys, files[i]);
        i++;
        break;
      }
    }
  }
  for (; i < cnt; ++i) {
    /* open each database in new tab */
    struct app_view *view = app_view_new(app, sys);
    view->db = db.init(&app->gui, sys->mem.arena, sys->mem.tmp, files[i]);
    if (!view->db) {
      app_view_del(app, view, sys);
      continue;
    }
    app_view_init(view, sys, files[i]);
    dyn_put(app->views, sys, 0, &view, 1);
    app->sel_tab = 0;
  }
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
  gui.init(&app->gui, sys->mem.arena, CFG_COLOR_SCHEME);

  app->fs = file.init(sys, &app->gui, sys->mem.arena, sys->mem.tmp);
  app->views = arena_dyn(sys->mem.arena, sys, struct app_view*, 16);
  lst_init(&app->del_lst);

  struct app_view *view = app_view_new(app, sys);
  app_view_setup(view, sys);
  dyn_add(app->views, sys, view);
  sys->dbg.enable(sys);
}
static void
app_shutdown(struct app *app, struct sys *sys) {
  assert(app);
  assert(sys);
  file.shutdown(app->fs, sys);

  fori_dyn(i, app->views) {
    struct app_view *view = app->views[i];
    db.shutdown(view->db, sys);
    dyn_free(view->file_path, sys);
    app_view_del(app, view, sys);
  }
  dyn_free(app->views, sys);
}
static int
ui_app_view_tab_slot_close(struct gui_ctx *ctx, struct gui_panel *pan,
                           struct gui_panel *parent, struct str title,
                           const char *ico) {
  assert(ctx);
  assert(pan);
  assert(parent);
  assert(ico);

  int ret = 0;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_box lay = pan->box;
    struct gui_icon close = {.box = gui.cut.rhs(&lay, ctx->cfg.item, 0)};
    gui.ico.clk(ctx, &close, pan, ICO_TIMES);
    ret = close.clk;

    struct gui_panel lbl = {.box = lay};
    gui.ico.box(ctx, &lbl, pan, ico, title);
  }
  gui.pan.end(ctx, pan, parent);
  return ret;
}
static int
ui_app_view_tab_slot(struct app *app, struct app_view *view,
                     struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                     struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot,
                     struct str title, const char *ico) {
  assert(app);
  assert(view);
  assert(ctx);
  assert(tab);
  assert(hdr);
  assert(slot);
  assert(ico);

  int ret = 0;
  unsigned long long tab_id = hash_ptr(view);
  gui.tab.hdr.slot.begin(ctx, tab, hdr, slot, tab_id);
  if (dyn_cnt(app->views) > 1 && tab->idx == tab->sel.idx) {
    ret = ui_app_view_tab_slot_close(ctx, slot, &hdr->pan, title, ico);
  } else {
    gui.ico.box(ctx, slot, &hdr->pan, ico, title);
  }
  gui.tab.hdr.slot.end(ctx, tab, hdr, slot, 0);
  return ret;
}
static int
ui_app_view_tab(struct app *app, struct app_view *view,
                struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot) {
  assert(app);
  assert(ctx);
  assert(tab);
  assert(hdr);
  assert(view);
  assert(slot);

  struct str title = strv("Open");
  const char *ico = ICO_FOLDER_OPEN;
  if (view->state != APP_STATE_FILE) {
    ico = ICO_DATABASE;
    title = path_file(dyn_str(view->file_path));
  }
  return ui_app_view_tab_slot(app, view, ctx, tab, hdr, slot, title, ico);
}
static void
ui_app_view(struct app *app, struct app_view *view, struct gui_ctx *ctx,
            struct gui_panel *pan, struct gui_panel *parent) {
  assert(app);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_panel bdy = {.box = pan->box};
    switch (view->state) {
    case APP_STATE_FILE: {
      struct sys *sys = ctx->sys;
      if (file.ui(&view->file_path, app->fs, ctx, &bdy, pan)) {
        struct str file_path = dyn_str(view->file_path);
        view->db = db.init(&app->gui, sys->mem.arena, sys->mem.tmp, file_path);
        view->state = APP_STATE_DB;
      }
    } break;
    case APP_STATE_DB:
      db.ui(view->db, ctx, &bdy, pan);
      break;
    case APP_STATE_PROFILER: {
      struct sys *sys = app->sys;
      if (sys->dbg.ui(&gui, sys, ctx, &bdy, pan)) {
        view->state = view->last_state;
        sys->dbg.enable(sys);
      }
    } break;}
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_app_dnd_files(struct app *app, struct gui_ctx *ctx, struct gui_panel *pan) {
  assert(app);
  assert(ctx);
  assert(pan);

  if (gui.dnd.dst.begin(ctx, pan)) {
    struct gui_dnd_paq *paq = gui.dnd.dst.get(ctx, STR_HASH16("[sys:files]"));
    if (paq) { /* file drag & drop */
      const struct str *file_urls = paq->data;
      switch (paq->state) {
      case GUI_DND_DELIVERY: {
        int file_cnt = paq->size;
        app_open_files(app, ctx->sys, file_urls, file_cnt);
        paq->response = GUI_DND_ACCEPT;
      } break;
      case GUI_DND_LEFT: break;
      case GUI_DND_ENTER:
      case GUI_DND_PREVIEW: {
        paq->response = GUI_DND_ACCEPT;
      } break;}
    }
    gui.dnd.dst.end(ctx);
  }
}
static int
ui_app_tab_view_lst(struct app *app, struct gui_ctx *ctx,
                    struct gui_panel *pan, struct gui_panel *parent) {

  assert(app);
  assert(ctx);
  assert(pan);
  assert(parent);

  int ret = -1;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_lst_cfg cfg = {0};
    gui.lst.cfg(&cfg, dyn_cnt(app->views), app->tab_lst_off[1]);
    cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
    cfg.sel.on = GUI_LST_SEL_ON_HOV;

    struct gui_lst_reg reg = {.box = pan->box};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, app->tab_lst_off);
    for_gui_reg_lst(i,gui,&reg) {
      struct app_view *view = app->views[i];
      struct str title = strv("Open");
      const char *ico = ICO_FOLDER_OPEN;
      if (view->state != APP_STATE_FILE) {
        ico = ICO_DATABASE;
        title = path_file(dyn_str(view->file_path));
      }
      struct gui_panel elm = {0};
      unsigned long long n = cast(unsigned long long, i);
      unsigned long long id = fnv1au64(n, FNV1A64_HASH_INITIAL);
      gui.lst.reg.elm.txt(ctx, &reg, &elm, id, 0, title, ico, 0);

      struct gui_input in = {0};
      gui.pan.input(&in, ctx, &elm, GUI_BTN_LEFT);
      ret = in.mouse.btn.left.clk ? i : ret;
    }
    gui.lst.reg.end(ctx, &reg, pan, app->tab_lst_off);
  }
  gui.pan.end(ctx, pan, parent);
  return ret;
}
static void
ui_app_swap_view(struct app *app, int dst_idx, int src_idx) {
  assert(app);
  assert(dst_idx < dyn_cnt(app->views));
  assert(src_idx < dyn_cnt(app->views));

  struct app_view *dst = app->views[dst_idx];
  struct app_view *src = app->views[src_idx];

  app->views[dst_idx] = src;
  app->views[src_idx] = dst;
}
static void
ui_app_main(struct app *app, struct gui_ctx *ctx, struct gui_panel *pan,
            struct gui_panel *parent) {
  assert(app);
  assert(ctx);
  assert(pan);
  assert(parent);

  dbg_blk_begin(ctx->sys, "app:gui:db:explr");
  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box, .show_btn = 1};
    gui.tab.begin(ctx, &tab, pan, dyn_cnt(app->views), app->sel_tab);
    {
      /* tab header */
      int del_tab = 0;
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      for_cnt(i, tab.cnt) {
        /* tab header slots */
        struct gui_panel slot = {0};
        struct app_view *view = app->views[i];
        if (ui_app_view_tab(app, view, ctx, &tab, &hdr, &slot)) {
          del_tab = 1;
        }
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);
      if (tab.sort.mod) {
        ui_app_swap_view(app, tab.sort.dst, tab.sort.src);
      }
      if (del_tab) {
        assert(dyn_any(app->views));
        assert(app->sel_tab < dyn_cnt(app->views));
        /* close database view tab */
        struct app_view *view = app->views[app->sel_tab];
        dyn_rm(app->views, app->sel_tab);
        app_view_del(app, view, ctx->sys);
        app->sel_tab = clamp(0, tab.sel.idx, dyn_cnt(app->views)-1);
      }
      struct gui_btn add = {.box = hdr.pan.box};
      add.box.x = gui.bnd.min_ext(tab.off, ctx->cfg.item);
      if (gui.btn.ico(ctx, &add, &hdr.pan, ICO_FOLDER_PLUS)) {
        /* new open file view tab */
        struct app_view *view = app_view_new(app, ctx->sys);
        dyn_put(app->views, ctx->sys, 0, &view, 1);
        app->sel_tab = 0;
      }
      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      app->show_tab_lst = tab.btn.clk ? !app->show_tab_lst : app->show_tab_lst;
      if (app->show_tab_lst) {
        /* tab selection */
        int ret = ui_app_tab_view_lst(app, ctx, &bdy, pan);
        if (ret >= 0) {
          ui_app_swap_view(app, 0, ret);
          app->show_tab_lst = 0;
          app->sel_tab = 0;
        }
      } else {
        ui_app_view(app, app->views[app->sel_tab], ctx, &bdy, pan);
      }
    }
    gui.tab.end(ctx, &tab, pan);
    if (tab.sel.mod) {
      app->sel_tab = tab.sel.idx;
    }
  }
  gui.pan.end(ctx, pan, parent);
  ui_app_dnd_files(app, ctx, pan);
  dbg_blk_end(ctx->sys);
}
extern void
app_on_api(struct sys *sys) {
  assert(sys);
#ifdef RELEASE_MODE
  unused(sys);
  res_get_api(&res, 0);
  gui_get_api(&gui, &res);
  pck_get_api(&file, &gui);
  db_get_api(&db, &gui);
#else
  /* load plugins */
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
#ifdef SYS_LINUX
  gui.color_scheme(&app->gui, GUI_COL_SCHEME_DARK);
#else
  if (sys->style_mod) {
    gui.color_scheme(&app->gui, CFG_COLOR_SCHEME);
  }
#endif
  memset(app->ops, 0, sizeof(app->ops));
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
  /* update */
  fori_dyn(i, app->views) {
    struct app_view *view = app->views[i];
    switch (view->state) {
    case APP_STATE_FILE: break;
    case APP_STATE_PROFILER: break;
    case APP_STATE_DB:
      db.update(view->db, sys);
      break;
    }
  }
  file.update(app->fs, sys);

  /* gui */
  dbg_blk_begin(sys, "app:gui");
  while (gui.begin(&app->gui)) {
    dbg_blk_begin(sys, "app:gui:pass");
    struct gui_panel pan = {.box = app->gui.box};
    ui_app_main(app, &app->gui, &pan, &app->gui.root);
    gui.end(&app->gui);
    dbg_blk_end(sys);
  }
  dbg_blk_end(sys);
}
#ifdef DEBUG_MODE
extern void
dlRegister(struct sys *sys) {
  app_on_api(sys);
}
extern void
dlEntry(struct sys *sys) {
  app_run(sys);
}
#endif

