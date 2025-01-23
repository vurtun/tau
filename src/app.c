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
#include "gui/res.h"
#include "gui/gui.h"
#include "app/cfg.h"
#include "app/pck.h"
#include "app/dbs.h"
#include "app.h"

enum {
  APP_VIEW_CNT      = 8,
  APP_VIEW_PATH_BUF = (MAX_FILE_PATH * APP_VIEW_CNT),
};
#define APP_VIEW_CNT_MSK ((1U << castu(APP_VIEW_CNT))-1)

enum app_view_state {
  APP_VIEW_STATE_FILE,
  APP_VIEW_STATE_DB,
};
struct app_view {
  enum app_view_state state;
  enum app_view_state last_state;
  struct db_state db;
  struct str file_path;
  int path_buf_off;
};
struct app {
  struct res res;
  struct gui_ctx gui;
  struct file_view fs;
  struct db_view db;

  /* views */
  int tab_lst_off[2];
  unsigned unused;
  unsigned char show_tab_lst;
  unsigned char sel_tab;
  unsigned char tab_cnt;
  unsigned char tabs[APP_VIEW_CNT];
  struct app_view views[APP_VIEW_CNT];

  char path_buf[APP_VIEW_PATH_BUF];
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
#include "gui/res.c"
#include "gui/gui.c"
#include "app/pck.c"
#include "app/dbs.c"

/* -----------------------------------------------------------------------------
 *                                  App
 * ---------------------------------------------------------------------------*/
static int
app_view_new(struct app *app) {
  requires(app);
  requires(app->tab_cnt < APP_VIEW_CNT);
  requires(app->unused > 0);
  requires(app->unused <= APP_VIEW_CNT_MSK);

  int idx = cpu_bit_ffs32(app->unused);
  assert(idx >= 0);
  assert(idx < 32);
  assert(idx < APP_VIEW_CNT);
  assert(idx < cntof(app->views));
  assert((app->unused & (1u << castu(idx))) != 0U);

  unsigned old_unused = app->unused;
  app->unused &= ~(1U << castu(idx));
  mset(app->views + idx, 0, szof(app->views[0]));
  assert((app->unused & (1 << idx)) == 0);

  ensures(app->unused < old_unused);
  ensures((app->unused & (1 << idx)) == 0U);
  return idx;
}
static void
app_view_del(struct app *app, int idx) {
  requires(app);
  requires(idx >= 0);
  requires(idx < APP_VIEW_CNT);
  requires(idx < cntof(app->views));

  requires(app->unused <= APP_VIEW_CNT_MSK);
  requires(!(app->unused & (1U << castu(idx))));

  unsigned old_unused = app->unused;
  struct app_view *view = &app->views[idx];
  app->unused |= (1U << castu(idx));
  view->file_path = str_nil;

  ensures(app->unused == (old_unused|(1u << castu(idx))));
  ensures(app->unused > old_unused);
}
static void
app_view_setup(struct app *app, int idx) {
  requires(app);
  requires(idx >= 0);
  requires(idx < APP_VIEW_CNT);
  requires(idx < cntof(app->views));
  requires(!(app->unused & (1U << castu(idx))));
  requires(app->unused <= APP_VIEW_CNT_MSK);

  struct app_view *view = &app->views[idx];
  view->state = APP_VIEW_STATE_FILE;
  view->last_state = view->state;
  view->path_buf_off = idx * MAX_FILE_PATH;
}
static int
app_view_init(struct app *app, int idx, struct str path) {
  requires(app);
  requires(idx >= 0);
  requires(idx < APP_VIEW_CNT);
  requires(idx < cntof(app->views));
  requires(!(app->unused & (1U << castu(idx))));

  struct app_view *view = &app->views[idx];
  app_view_setup(app, idx);
  view->file_path = str_set(app->path_buf + view->path_buf_off, MAX_FILE_PATH, path);
  if (str_is_inv(view->file_path)) {
    return -1;
  }
  view->state = APP_VIEW_STATE_DB;
  return 0;
}
static int
app_tab_add(struct app *app, int idx) {
  requires(app);
  requires(app->tab_cnt >= 0);
  requires(app->tab_cnt <= APP_VIEW_CNT);
  requires(app->tab_cnt <= cntof(app->tabs));

  requires(idx >= 0);
  requires(idx <= 0xff);
  requires(idx < APP_VIEW_CNT);
  requires(idx < cntof(app->views));

  requires(!(app->unused & (1U << castu(idx))));
  requires(app->unused <= APP_VIEW_CNT_MSK);

  int old_tab_cnt = app->tab_cnt;
  int ret = app->tab_cnt++;
  app->tabs[ret] = castb(idx);

  ensures(app->tab_cnt > old_tab_cnt);
  ensures(app->tab_cnt <= APP_VIEW_CNT);
  ensures(app->tab_cnt <= cntof(app->tabs));

  ensures(ret >= 0);
  ensures(ret < APP_VIEW_CNT);
  ensures(ret < cntof(app->tabs));
  return ret;
}
static void
app_tab_rm(struct app *app, int tab_idx) {
  requires(app);
  requires(app->tab_cnt > 0);
  requires(app->tab_cnt <= APP_VIEW_CNT);
  requires(app->tab_cnt <= app->tab_cnt);

  requires(tab_idx >= 0);
  requires(tab_idx < app->tab_cnt);
  requires(tab_idx < APP_VIEW_CNT);
  requires(tab_idx < cntof(app->tabs));

  requires(app->sel_tab < app->tab_cnt);
  requires(app->sel_tab < APP_VIEW_CNT);
  requires(app->sel_tab < cntof(app->tabs));

  requires(!(app->unused & (1 << app->tabs[tab_idx])));
  requires(app->unused <= APP_VIEW_CNT_MSK);

  arr_rm(app->tabs, tab_idx, app->tab_cnt);
  int old_tab_cnt = app->tab_cnt--;
  app->sel_tab = castb(clamp(0, app->sel_tab, app->tab_cnt-1));

  ensures(app->sel_tab >= 0);
  ensures(app->sel_tab < app->tab_cnt);
  ensures(app->sel_tab < APP_VIEW_CNT);
  ensures(app->sel_tab < cntof(app->tabs));

  ensures(app->tab_cnt >= 0);
  ensures(app->tab_cnt < old_tab_cnt);
}
static void
app_tab_close(struct app *app, int tab_idx) {
  requires(app);
  requires(app->tab_cnt > 0);
  requires(app->tab_cnt <= APP_VIEW_CNT);
  requires(app->tab_cnt <= cntof(app->tabs));

  requires(tab_idx >= 0);
  requires(tab_idx < app->tab_cnt);
  requires(tab_idx < APP_VIEW_CNT);
  requires(tab_idx < cntof(app->tabs));

  requires(!(app->unused & (1U << castu(app->tabs[tab_idx]))));
  requires(app->unused <= APP_VIEW_CNT_MSK);

  unsigned old_unused = app->unused;
  app_view_del(app, app->tabs[tab_idx]);
  app_tab_rm(app, tab_idx);

  ensures(app->tab_cnt >= 0);
  ensures(app->sel_tab >= 0);
  ensures(app->sel_tab < app->tab_cnt);
  ensures(app->sel_tab < APP_VIEW_CNT);
  ensures(app->sel_tab < cntof(app->tabs));
  ensures(app->unused >= old_unused);
}
static void
app_tab_open_files(struct app *app, const struct str *files, int file_cnt) {
  requires(app);
  requires(files);

  requires(app->tab_cnt <= APP_VIEW_CNT);
  requires(app->tab_cnt <= cntof(app->tabs));

  requires(file_cnt >= 0);
  requires(file_cnt <= APP_VIEW_CNT);
  requires(file_cnt <= APP_VIEW_CNT - app->tab_cnt);

  requires(app->tab_cnt >= 0);
  requires(app->tab_cnt <= cntof(app->tabs));
  requires(file_cnt <= cntof(app->tabs) - app->tab_cnt);

  unsigned oldest_unused = app->unused;
  int num = cntof(app->tabs) - app->tab_cnt;
  assert(num >= 0);

  int add_cnt = min(file_cnt, num);
  assert(add_cnt <= file_cnt);
  assert(add_cnt <= num);
  assert(add_cnt >= 0);
  assert(add_cnt < APP_VIEW_CNT);

  for arr_loopn(i, app->tabs, file_cnt) {
    /* open each database in new tab */
    loop_invariant(i >= 0);
    loop_invariant(i < add_cnt);
    loop_invariant(i < file_cnt);
    loop_invariant(i < APP_VIEW_CNT);
    loop_invariant(i < cntof(app->tabs));

    assert(app->unused >= 0);
    assert(app->unused <= APP_VIEW_CNT_MSK);

    unsigned old_unused = app->unused;
    int view = app_view_new(app);
    assert(app->unused < old_unused);

    if (app_view_init(app, view, files[i]) < 0) {
      app_view_del(app, view);
      assert(app->unused == old_unused);
      continue;
    }
    unsigned old_tab_cnt = app->tab_cnt;
    int tab_idx = app_tab_add(app, view);
    app->sel_tab = castb(tab_idx);

    if (!dbs.setup(&app->views[view].db, &app->gui, files[i])) {
      app_tab_close(app, app->sel_tab);
      assert(app->tab_cnt == old_tab_cnt);
    }
  }
  ensures(app->tab_cnt >= 0);
  ensures(app->tab_cnt <= APP_VIEW_CNT);
  ensures(app->unused <= oldest_unused);

  ensures(app->sel_tab >= 0);
  ensures(app->sel_tab < app->tab_cnt);
  ensures(app->sel_tab < APP_VIEW_CNT);
  ensures(app->sel_tab < cntof(app->tabs));
}
static int
app_tab_open(struct app *app) {
  requires(app);
  requires(app->unused > 0);
  requires(app->unused <= APP_VIEW_CNT_MSK);

  requires(app->tab_cnt >= 0);
  requires(app->tab_cnt <= cntof(app->tabs));
  requires(app->tab_cnt <= APP_VIEW_CNT);

  unsigned old_unused = app->unused;
  int view = app_view_new(app);
  int tab_idx = app_tab_add(app, view);
  app->sel_tab = castb(tab_idx);

  ensures(app->sel_tab >= 0);
  ensures(app->sel_tab < app->tab_cnt);
  ensures(app->sel_tab < APP_VIEW_CNT);
  ensures(app->sel_tab < cntof(app->tabs));

  ensures(app->unused < old_unused);
  return app->sel_tab;
}
static void
app_tab_swap(struct app *app, int dst_idx, int src_idx) {
  requires(app);
  requires(dst_idx >= 0);
  requires(src_idx >= 0);

  requires(dst_idx < app->tab_cnt);
  requires(src_idx < app->tab_cnt);

  requires(dst_idx < cntof(app->tabs));
  requires(src_idx < cntof(app->tabs));

  requires(app->tabs[dst_idx] < cntof(app->views));
  requires(app->tabs[src_idx] < cntof(app->views));

  requires(!(app->unused & (1U << castu(app->tabs[dst_idx]))));
  requires(!(app->unused & (1U << castu(app->tabs[src_idx]))));

  int old_dst = app->tabs[dst_idx];
  int old_src = app->tabs[src_idx];

  app->tabs[src_idx] = castb(old_dst);
  app->tabs[dst_idx] = castb(old_src);

  ensures(app->tabs[dst_idx] == old_src);
  ensures(app->tabs[src_idx] == old_dst);
}
static void
app_init(struct app *app, struct sys *sys) {
  requires(sys);
  requires(app);
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
  if (pck.init(&app->fs, sys, &app->gui) < 0) {

  }
  dbs.init(app->db_mem, szof(app->db_mem));

  app->tab_cnt = 0;
  app->unused = APP_VIEW_CNT_MSK;
  app->tabs[app->tab_cnt++] = castb(app_view_new(app));
  app_view_setup(app, app->tabs[0]);

#ifdef DEBUG_MODE
  ut_str();
#endif
  ensures(app->tab_cnt == 1);
  ensures(app->tabs[0] == 0);
  ensures(app->unused == APP_VIEW_CNT_MSK - 1u);
}
static void
app_shutdown(struct app *app, struct sys *sys) {
  requires(sys);
  requires(app);

  requires(app->tab_cnt >= 0);
  requires(app->tab_cnt < cntof(app->tabs));
  requires(app->unused <= APP_VIEW_CNT_MSK);
  requires(app->unused >= 0);

  for arr_loopn(i, app->tabs, app->tab_cnt) {
    loop_invariant(i >= 0);
    loop_invariant(i < app->tab_cnt);
    loop_invariant(i < cntof(app->tabs));
    loop_invariant(i < APP_VIEW_CNT);

    int idx = app->tabs[i];
    assert(idx < APP_VIEW_CNT);
    assert(idx < cntof(app->views));
    assert(!(app->unused & (1U << castu(idx))));

    struct app_view *view = &app->views[idx];
    if (view->db.con) {
      dbs.del(&view->db);
    }
    app_view_del(app, idx);
  }
  assert(app->unused == APP_VIEW_CNT_MSK);
  app->unused = APP_VIEW_CNT_MSK;
  app->tab_cnt = 0;

  pck.shutdown(&app->fs,sys);
  res.shutdown(&app->res);
  ensures(app->tab_cnt == 0);
}

/* -----------------------------------------------------------------------------
 *
 *                                  GUI
 *
 * -----------------------------------------------------------------------------
 */
static void
ui_app_file_view(struct app *app, struct app_view *view, struct gui_ctx *ctx,
                struct gui_panel *pan, struct gui_panel *parent) {

  requires(app);
  requires(view);
  requires(ctx);
  requires(pan);
  requires(parent);
  requires(ctx->sys);

  requires(app->tab_cnt > 0);
  requires(app->tab_cnt < cntof(app->tabs));
  requires(app->unused >= 0);
  requires(app->unused < APP_VIEW_CNT_MSK);

  int old_tab_cnt = app->tab_cnt;
  unsigned old_unused = app->unused;

  view->file_path = pck.ui(app->path_buf + view->path_buf_off, MAX_FILE_PATH,
    &app->fs, ctx, pan, parent);
  if (str_is_val(view->file_path)) {
    struct sys *sys = ctx->sys;
    dbs.setup(&view->db, &app->gui, view->file_path);
    view->state = APP_VIEW_STATE_DB;

    int new_view = app_view_new(app);
    app_view_setup(app, new_view);
    app_tab_add(app, new_view);
    sys->repaint = 1;
  }
  ensures(app->unused <= old_unused);
  ensures(app->tab_cnt >= old_tab_cnt);
}
static void
ui_app_view(struct app *app, struct app_view *view, struct gui_ctx *ctx,
            struct gui_panel *pan, struct gui_panel *parent) {

  requires(app);
  requires(view);
  requires(ctx);
  requires(pan);
  requires(parent);

  requires(app->tab_cnt > 0);
  requires(app->tab_cnt < cntof(app->tabs));
  requires(app->unused >= 0);
  requires(app->unused < APP_VIEW_CNT_MSK);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_panel bdy = {.box = pan->box};
    switch (view->state) {
    case APP_VIEW_STATE_FILE: {
      ui_app_file_view(app, view, ctx, pan, parent);
    } break;
    case APP_VIEW_STATE_DB: {
      dbs.ui(&view->db, &app->db, ctx, &bdy, pan);
    } break;}
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_app_dnd_files(struct app *app, struct gui_ctx *ctx, struct gui_panel *pan) {
  requires(app);
  requires(ctx);
  requires(pan);

  requires(app->tab_cnt > 0);
  requires(app->unused >= 0);

  if (gui.dnd.dst.begin(ctx, pan)) {
    struct gui_dnd_paq *paq = gui.dnd.dst.get(ctx, GUI_DND_SYS_FILES);
    if (paq) { /* file drag & drop */
      const struct str *file_urls = paq->data;
      switch (paq->state) {
      case GUI_DND_DELIVERY: {

        int file_cnt = paq->size;
        int rest = cpu_bit_cnt(app->unused & APP_VIEW_CNT_MSK);
        int cnt = min(file_cnt, rest);

        app_tab_open_files(app, file_urls, cnt);
        paq->response = GUI_DND_ACCEPT;
      } break;
      case GUI_DND_LEFT: break;
      case GUI_DND_ENTER:
      case GUI_DND_PREVIEW: {
        if (app->unused & APP_VIEW_CNT_MSK) {
          paq->response = GUI_DND_ACCEPT;
        }
      } break;}
    }
    gui.dnd.dst.end(ctx);
  }
}
static int
ui_app_tab_view_lst(struct app *app, struct gui_ctx *ctx, struct gui_panel *pan,
                    struct gui_panel *parent) {
  requires(app);
  requires(ctx);
  requires(pan);
  requires(parent);

  requires(app->tab_cnt > 0);
  requires(app->tab_cnt <= APP_VIEW_CNT);
  requires(app->tab_cnt <= cntof(app->tabs));
  requires(app->unused >= 0);

  int ret = -1;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_lst_cfg cfg = {0};
    gui.lst.cfg(&cfg, app->tab_cnt, app->tab_lst_off[1]);
    cfg.ctl.focus = GUI_LST_FOCUS_ON_HOV;
    cfg.sel.on = GUI_LST_SEL_ON_HOV;

    struct gui_lst_reg reg = {.box = pan->box};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, app->tab_lst_off);
    for gui_tbl_lst_loopv(i,_,gui,&reg,app->tabs){

      assert(i < cntof(app->tabs));
      int idx = app->tabs[i];
      assert(idx < cntof(app->views));

      struct gui_panel elm = {0};
      struct app_view *view = &app->views[idx];
      unsigned long long tab_id = hash_ptr(view);
      gui.lst.reg.elm.txt(ctx, &reg, &elm, gui_id64(tab_id), 0, strv("Open"), 0);

      struct gui_input pin = {0};
      gui.pan.input(&pin, ctx, &elm, GUI_BTN_LEFT);
      ret = pin.mouse.btn.left.clk ? i : ret;
    }
    gui.lst.reg.end(ctx, &reg, pan, app->tab_lst_off);
  }
  gui.pan.end(ctx, pan, parent);
  return ret;
}
static int
ui_app_view_tab_slot_close(struct gui_ctx *ctx, struct gui_panel *pan,
                           struct gui_panel *parent, struct str title,
                           enum res_ico_id ico) {
  requires(ctx);
  requires(pan);
  requires(ico);
  requires(parent);

  int ret = 0;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_box lay = pan->box;
    struct gui_icon close = {.box = gui.cut.rhs(&lay, ctx->cfg.item, 0)};
    gui.ico.clk(ctx, &close, pan, RES_ICO_CLOSE);
    ret = close.clk;

    struct gui_panel lbl = {.box = lay};
    gui.ico.box(ctx, &lbl, pan, ico, title);
  }
  gui.pan.end(ctx, pan, parent);
  ensures(ret == 0 || ret == 1);
  return ret;
}
static int
ui_app_view_tab_slot(struct app *app, struct app_view *view,
                     struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                     struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot,
                     struct str title, enum res_ico_id ico) {
  requires(app);
  requires(ctx);
  requires(tab);
  requires(hdr);
  requires(ico);
  requires(slot);
  requires(view);

  int ret = 0;
  gui.tab.hdr.slot.begin(ctx, tab, hdr, slot, gui_id_ptr(view));
  if (app->tab_cnt > 1 && tab->idx == tab->sel.idx) {
    ret = ui_app_view_tab_slot_close(ctx, slot, &hdr->pan, title, ico);
  } else {
    gui.ico.box(ctx, slot, &hdr->pan, ico, title);
  }
  gui.tab.hdr.slot.end(ctx, tab, hdr, slot, 0);
  ensures(ret == 0 || ret == 1);
  return ret;
}
static int
ui_app_view_tab(struct app *app, struct app_view *view,
                struct gui_ctx *ctx, struct gui_tab_ctl *tab,
                struct gui_tab_ctl_hdr *hdr, struct gui_panel *slot) {
  requires(app);
  requires(ctx);
  requires(tab);
  requires(hdr);
  requires(slot);
  requires(view);

  enum res_ico_id ico = RES_ICO_FOLDER_OPEN;
  struct str title = strv("Open");
  if (view->state != APP_VIEW_STATE_FILE) {
    title = path_file(view->file_path);
    ico = RES_ICO_DATABASE;
  }
  return ui_app_view_tab_slot(app, view, ctx, tab, hdr, slot, title, ico);
}
static void
ui_app_main(struct app *app, struct gui_ctx *ctx, struct gui_panel *pan,
            struct gui_panel *parent) {

  requires(app);
  requires(ctx);
  requires(pan);
  requires(parent);

  requires(app->tab_cnt > 0);
  requires(app->tab_cnt <= cntof(app->tabs));
  requires(app->sel_tab < cntof(app->tabs));
  requires(app->sel_tab < app->tab_cnt);
  requires(app->unused >= 0);
  requires(app->unused < APP_VIEW_CNT_MSK);

  gui.pan.begin(ctx, pan, parent);
  {
    /* tab control */
    struct gui_tab_ctl tab = {.box = pan->box, .show_btn = 1};
    gui.tab.begin(ctx, &tab, pan, app->tab_cnt, app->sel_tab);
    {
      /* tab header */
      unsigned del_tab = 0;
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      assert(tab.cnt <= cntof(app->tabs));
      for arr_loopn(i, app->tabs, tab.cnt) {

        assert(i < cntof(app->tabs));
        int idx = app->tabs[i];
        assert(idx < cntof(app->views));

        /* tab header slots */
        struct gui_panel slot = {0};
        struct app_view *view = &app->views[idx];
        int ret = ui_app_view_tab(app, view, ctx, &tab, &hdr, &slot);
        del_tab |= castu(ret);
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);
      if (del_tab) {
        app_tab_close(app, app->sel_tab);
      } else if (tab.sort.mod) {
        app_tab_swap(app, tab.sort.dst, tab.sort.src);
      }
      if (app->unused) {
        /* add/open new file view tab */
        struct gui_btn add = {.box = hdr.pan.box};
        add.box.x = gui.bnd.min_ext(tab.off, ctx->cfg.item);
        if (gui.btn.ico(ctx, &add, &hdr.pan, RES_ICO_FOLDER_ADD)) {
          app_tab_open(app);
        }
      }
      /* tab body */
      struct gui_panel bdy = {.box = tab.bdy};
      app->show_tab_lst = tab.btn.clk ? !app->show_tab_lst : app->show_tab_lst;
      if (app->show_tab_lst) {
        /* tab selection */
        int ret = ui_app_tab_view_lst(app, ctx, &bdy, pan);
        if (ret >= 0) {
          app_tab_swap(app, 0, ret);
          app->show_tab_lst = 0;
          app->sel_tab = 0;
        }
      } else {
        assert(app->sel_tab < cntof(app->tabs));
        assert(app->sel_tab < app->tab_cnt);

        int sel_view = app->tabs[app->sel_tab];
        assert(!(app->unused & (1U << sel_view)));
        ui_app_view(app, app->views + sel_view, ctx, &bdy, pan);
      }
    }
    gui.tab.end(ctx, &tab, pan);
    if (tab.sel.mod) {
      app->sel_tab = castb(tab.sel.idx);
      assert(app->sel_tab < cntof(app->tabs));
      assert(app->sel_tab < APP_VIEW_CNT);
    }
  }
  gui.pan.end(ctx, pan, parent);
  ui_app_dnd_files(app, ctx, pan);

  ensures(app->show_tab_lst == 0 || app->show_tab_lst == 1);
  ensures(app->sel_tab < cntof(app->tabs));
  ensures(app->sel_tab < APP_VIEW_CNT);
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
  assert(sys);
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

