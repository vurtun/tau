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
#include "lib/rpq.h"
#include "lib/fnt.h"
#include "sys/gfx.h"
#include "sys/sys.h"
#include "gui/res.h"
#include "gui/gui.h"
#include "app/pck.h"

struct gui_app_key {
  int code;
  unsigned mod;
};
struct app_ui_shortcut {
  struct gui_app_key key;
  struct gui_app_key alt;
};
enum app_view_state {
  APP_VIEW_STATE_FILE,
};
struct app_view {
  enum app_view_state state;
  struct lst_elm hook;
  dyn(char) file_path;
};
struct app {
  struct sys sys;
  struct res res;
  struct gui_ctx gui;

  /* views */
  struct file_view *fs;
  int show_tab_lst;
  double tab_lst_off[2];
  dyn(struct app_view*) views;
  struct lst_elm del_lst;
  int sel_tab;
};

static struct sys_api sys;
static struct res_api res;
static struct gui_api gui;
static struct pck_api pck;

#include "cfg.h"
#include "lib/fmt.c"
#include "lib/std.c"
#include "lib/math.c"
#include "lib/rpq.c"
#include "lib/fnt.c"
#include "gui/res.c"
#include "gui/gui.c"
#include "app/pck.c"


/* -----------------------------------------------------------------------------
 *                                  App
 * ---------------------------------------------------------------------------*/
static struct app_view*
app_view_new(struct app *app) {
  assert(app);
  struct app_view *s = 0;
  if (lst_any(&app->del_lst)) {
    s = lst_get(lst_first(&app->del_lst), struct app_view, hook);
    lst_del(app->del_lst.nxt);
  } else {
    s = arena_alloc(app->sys.mem.arena, &app->sys, szof(*s));
  }
  lst_init(&s->hook);
  return s;
}
static void
app_view_del(struct app *app, struct app_view *view) {
  assert(app);
  assert(view);

  dyn_clr(view->file_path);
  lst_init(&view->hook);
  lst_add(&app->del_lst, &view->hook);
}
static void
app_view_setup(struct app *app, struct app_view *view) {
  assert(view);
  assert(app);

  view->state = APP_VIEW_STATE_FILE;
  if (!view->file_path) {
    view->file_path = arena_dyn(app->sys.mem.arena, &app->sys, char, 256);
  }
}
static void
app_view_init(struct app_view *view, struct app *app, struct str path) {
  assert(view);
  assert(app);

  app_view_setup(app, view);
  dyn_asn_str(view->file_path, &app->sys, path);
}
static void
app_swap_view(struct app *app, int dst_idx, int src_idx) {
  assert(app);
  assert(dst_idx < dyn_cnt(app->views));
  assert(src_idx < dyn_cnt(app->views));

  struct app_view *dst = app->views[dst_idx];
  struct app_view *src = app->views[src_idx];

  app->views[dst_idx] = src;
  app->views[src_idx] = dst;
}
static void
app_init(struct app *app) {
  app->sys.win.title = "Tau";
  app->sys.win.x = -1;
  app->sys.win.y = -1;
  app->sys.win.w = 800;
  app->sys.win.h = 600;
  app->sys.gfx.clear_color = col_rgb(0,0,0);
  sys.init(&app->sys);
  {
    struct res_args args = {0};
    args.run_cnt = KB(2);
    args.hash_cnt = KB(4);
    args.sys = &app->sys;
    res.init(&app->res, &args);
  }
  {
    struct gui_args args = {0};
    args.scale = app->sys.dpi_scale;
    if (app->sys.has_style){
      args.scm = GUI_COL_SCHEME_SYS;
    } else {
      args.scm = GUI_COL_SCHEME_DARK;
    }
    args.txt_mem = KB(2);
    args.vtx_mem = KB(64);
    args.idx_mem = KB(64);

    app->gui.sys = &app->sys;
    app->gui.res = &app->res;
    gui.init(&app->gui, app->sys.mem.arena, &args);
  }
  app->fs = pck.init(&app->sys, &app->gui, app->sys.mem.arena, app->sys.mem.tmp);
  app->views = arena_dyn(app->sys.mem.arena, &app->sys, struct app_view*, 16);
  lst_init(&app->del_lst);

  struct app_view *view = app_view_new(app);
  app_view_setup(app, view);
  dyn_add(app->views, &app->sys, view);
}
static void
app_shutdown(struct app *app) {
  fori_dyn(i, app->views) {
    struct app_view *view = app->views[i];
    dyn_free(view->file_path, &app->sys);
    app_view_del(app, view);
  }
  dyn_free(app->views, &app->sys);

  pck.shutdown(app->fs, &app->sys);
  gui.free(&app->gui);
  res.shutdown(&app->res);
  sys.shutdown(&app->sys);
}

/* -----------------------------------------------------------------------------
 *                                  GUI
 * -----------------------------------------------------------------------------
 */
static void
ui_app_view(struct app *app, struct app_view *view, struct gui_ctx *ctx,
            struct gui_panel *pan, struct gui_panel *parent) {
  assert(app);
  assert(ctx);
  assert(pan);
  assert(parent);
  unused(view);

  if (pck.ui(&view->file_path, app->fs, ctx, pan, parent)) {

  }
  gui.pan.end(ctx, pan, parent);
}
static int
ui_app_tab_view_lst(struct app *app, struct gui_ctx *ctx, struct gui_panel *pan,
                     struct gui_panel *parent) {
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
    for_gui_lst_reg(i,gui,&reg) {
      struct gui_panel elm = {0};
      struct app_view *view = app->views[i];
      unsigned long long tab_id = hash_ptr(view);
      gui.lst.reg.elm.txt(ctx, &reg, &elm, tab_id, 0, strv("Open"), 0);

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
ui_app_main(struct app *app, struct gui_ctx *ctx, struct gui_panel *pan,
            struct gui_panel *parent) {
  unused(app);
  assert(app);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box, .show_btn = 1};
    gui.tab.begin(ctx, &tab, pan, dyn_cnt(app->views), app->sel_tab);
    {
      /* tab header */
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      for_cnt(i, tab.cnt) {
        gui.tab.hdr.slot.txt(ctx, &tab, &hdr, strv("Open"));
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);
      if (tab.sort.mod) {
        app_swap_view(app, tab.sort.dst, tab.sort.src);
      }
      struct gui_btn add = {.box = hdr.pan.box};
      add.box.x = gui.bnd.min_ext(tab.off, ctx->cfg.item);
      if (gui.btn.txt(ctx, &add, &hdr.pan, strv("+"), 0)) {
        /* open new file view tab */
        struct app_view *view = app_view_new(app);
        app->sel_tab = dyn_cnt(app->views);
        dyn_add(app->views, ctx->sys, view);
      }
      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      app->show_tab_lst = tab.btn.clk ? !app->show_tab_lst : app->show_tab_lst;
      if (app->show_tab_lst) {
        /* tab selection */
        int ret = ui_app_tab_view_lst(app, ctx, &bdy, pan);
        if (ret >= 0) {
          app_swap_view(app, 0, ret);
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
}

/* -----------------------------------------------------------------------------
 *                                  Main
 * -----------------------------------------------------------------------------
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
usage(const char *app) {
  printf("\n"
      "usage: %s [options] file0 file1\n"
      "\n"
      "   arguments:\n"
      "\n"
      "   file,         Input sqlite database\n"
      "\n"
      "   options:\n"
      "   -h            help message\n"
      "\n",
      app
  );
  exit(1);
}
extern int
main(int argc, char **argv) {
  const char *exe = 0;
  /* api */
  sys_api(&sys, 0);
  res_api(&res, 0);
  gui_api(&gui, 0);
  pck_api(&pck, 0);

  /* init */
  struct app app = {0};
  cmd_arg_begin(exe, argc, argv){
  case 'h': default: usage(exe); break;
  } cmd_arg_end;
  app_init(&app);

  /* run */
  while (app.sys.running) {
    sys.pull(&app.sys);
    pck.update(app.fs, &app.sys);
    /* gui */
    fori_arrv(i, app_ui_key_tbl) {
      /* map system keys to ui shortcuts */
      const struct app_ui_shortcut *s = app_ui_key_tbl + i;
      struct gui_ctx *ctx = &app.gui;
      int keymod = app_on_mod(s->key.mod, app.sys.keymod);
      if (bit_tst(app.sys.keys, s->key.code) && keymod) {
        bit_set(ctx->keys, i);
      } else if (bit_tst(app.sys.keys, s->alt.code) && keymod) {
        bit_set(ctx->keys, i);
      }
    }
    while (gui.begin(&app.gui)) {
      struct gui_panel pan = {.box = app.gui.box};
      ui_app_main(&app, &app.gui, &pan, &app.gui.root);
      gui.end(&app.gui);
    }
    sys.push(&app.sys);
  }
  app_shutdown(&app);
  return 0;
}

