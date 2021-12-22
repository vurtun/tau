#ifdef DEBUG_MODE
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

#include "cpu.h"
#include "../lib/fmt.h"
#include "../lib/std.h"
#include "dbg.h"
#include "ren.h"
#include "sys.h"
#include "../lib/fmt.c"
#include "../lib/std.c"
#endif
#include "../gui.h"

struct dbg_evt {
  enum dbg_type type;
  unsigned long long clk;
  const char *guid;
  const char *name;
};
struct dbg_tbl {
  int evt_buf_idx;
  volatile unsigned long long idx;
  struct dbg_evt evt[2][128*1024];
};
struct dbg_elm {
  struct dbg_elm *parent;
  struct lst_elm hook;
  struct lst_elm sub;

  const char *guid;
  unsigned long long clk;
  unsigned long long dur;
  const char *name;
};
struct dbg_tbl_col_def {
  struct str title;
  struct gui_split_lay_slot ui;
};
enum dbg_tbl_hdr_col {
  DBG_TBL_TIME,
  DBG_TBL_RATIO,
  DBG_TBL_NAME,
  DBG_TBL_MAX,
};
struct dbg {
  struct arena mem;
  struct dbg_tbl tbl;
  struct dbg_elm *tree;
  struct dbg_elm **lst;
  unsigned collate : 1;

  int tbl_init;
  int tbl_state[GUI_TBL_CAP(DBG_TBL_MAX)];
  double off[2];
};
extern void dlInit(struct sys *sys);
extern void dlBegin(struct sys *sys);
extern void dlEnd(struct sys *sys);

// clang-format off
static const struct dbg_tbl_col_def dbg_profile_tbl_def[DBG_TBL_MAX] = {
  [DBG_TBL_TIME]  =  {.title = strv("Time(usec)"),  .ui = {.type = GUI_LAY_SLOT_FIX, .size = 240, .con = {100, 400}}},
  [DBG_TBL_RATIO] =  {.title = strv("Ratio"),       .ui = {.type = GUI_LAY_SLOT_FIX, .size =  80, .con = {100, 400}}},
  [DBG_TBL_NAME]  =  {.title = strv("Name"),        .ui = {.type = GUI_LAY_SLOT_DYN, .size =  1,  .con = {100, 400}}},
};
// clang-format on

static void
dbg_rec(struct sys *sys, enum dbg_type type, const char *guid, const char *name) {
  struct dbg *dbg = cast(struct dbg*, sys->debug);
  unsigned long long id = atom_add(&dbg->tbl.idx, 1u);
  unsigned long long idx = id & 0xFFFFFFFFu;
  assert(idx < cntof(dbg->tbl.evt[0]));

  struct dbg_evt *evt = dbg->tbl.evt[id >> 32u] + idx;
  evt->clk = sys->time.timestamp();
  evt->type = type;
  evt->guid = guid;
  evt->name = name;
}
static void
dbg_begin(struct sys *sys) {
  assert(sys);
  dbg_blk_begin(sys, "frame");
}
static void
dbg_elm_setup(struct dbg_elm *elm, struct dbg_elm *p, const struct dbg_evt *evt) {
  elm->parent = p;
  lst_init(&elm->hook);
  lst_init(&elm->sub);
  if (elm->parent) {
    lst_add_tail(&p->sub, &elm->hook);
  }
  elm->guid = evt->guid;
  elm->name = evt->name;
  elm->clk = evt->clk;
  elm->dur = 0;
}
static struct dbg_elm *
dbg_elm_new(struct arena *mem, struct sys *sys, struct dbg_elm *p,
            const struct dbg_evt *evt) {
  struct dbg_elm *elm = arena_obj(mem, sys, struct dbg_elm);
  dbg_elm_setup(elm, p, evt);
  return elm;
}
static int
dbg_cmp_elms_dur(const void *a, const void *b) {
  const struct dbg_elm **ea = (const struct dbg_elm **)a;
  const struct dbg_elm **eb = (const struct dbg_elm **)b;
  return cast(int, (*eb)->dur) - cast(int, (*ea)->dur);
}
static void
dbg_end(struct sys *sys) {
  assert(sys);
  dbg_blk_end(sys);

  struct dbg *dbg = cast(struct dbg*, sys->debug);
  dbg->tbl.evt_buf_idx = !dbg->tbl.evt_buf_idx;
  unsigned long long new_idx = cast(unsigned long long, dbg->tbl.evt_buf_idx) << 32llu;
  unsigned long long evt_idx = atom_xchg(&dbg->tbl.idx, new_idx);
  if (!dbg->collate) {
    return;
  }
  unsigned evt_buf_idx = evt_idx >> 32u;
  assert(evt_buf_idx <= 1);
  unsigned evt_cnt = evt_idx & 0xFFFFFFFF;

  dyn_free(dbg->lst, sys);
  arena_reset(&dbg->mem, sys);
  dbg->tree = 0;

  struct dbg_elm *p = 0;
  struct dbg_evt *evt = 0;
  for_arr(evt, dbg->tbl.evt[evt_buf_idx], evt_cnt) {
    switch (evt->type) {
    case DBG_TYPE_BLK_BEGIN: {
      struct dbg_elm *elm = dbg_elm_new(&dbg->mem, sys, p, evt);
      if (!dbg->tree) {
        dbg->tree = elm;
      }
      dyn_add(dbg->lst, sys, elm);
      p = elm;
    } break;
    case DBG_TYPE_BLK_END: {
      assert(p);
      struct dbg_elm *elm = p;
      elm->dur = evt->clk - elm->clk;
      p = elm->parent;
    } break;}
  }
  dyn_sort(dbg->lst, dbg_cmp_elms_dur);
}
static int
ui_dbg_btn_txt(struct gui_api *gui, struct gui_ctx *ctx, struct gui_btn *btn,
               struct gui_panel *parent, struct str txt, int uline) {
  assert(ctx);
  assert(btn);
  assert(parent);

  static const struct gui_align align = {GUI_HALIGN_MID, GUI_VALIGN_MID};
  gui->btn.begin(ctx, btn, parent);
  {
    struct gui_panel lbl = {.box = btn->pan.box};
    lbl.box.x = gui->bnd.shrink(&btn->pan.box.x, ctx->cfg.pad[0]);
    gui->txt.uln(ctx, &lbl, &btn->pan, txt, &align, uline, 1);
  }
  gui->btn.end(ctx, btn, parent);
  return btn->clk;
}
static int
dbg_ui(struct gui_api *gui, struct sys *sys, struct gui_ctx *ctx,
       struct gui_panel *pan, struct gui_panel *parent) {

  assert(gui);
  assert(sys);
  assert(ctx);
  assert(pan);
  assert(parent);

  int res = 0;
  struct dbg *dbg = cast(struct dbg*, sys->debug);
  if (!dbg->tbl_init) {
    /* setup table state */
    struct gui_split_lay_cfg tbl_cfg = {.cnt = DBG_TBL_MAX};
    tbl_cfg.size = sizeof(struct dbg_tbl_col_def);
    tbl_cfg.off = offsetof(struct dbg_tbl_col_def, ui);
    tbl_cfg.slots = dbg_profile_tbl_def;
    gui->tbl.lay(dbg->tbl_state, ctx, &tbl_cfg);
  }
  gui->pan.begin(ctx, pan, parent);
  {
    struct gui_box lay = pan->box;
    struct gui_btn back = {.box = gui->cut.bot(&lay, ctx->cfg.item, ctx->cfg.gap[0])};
    if (ui_dbg_btn_txt(gui, ctx, &back, pan, strv("Back"), 0)) {
      res = 1;
    }
    struct gui_tbl tbl = {.box = lay};
    gui->tbl.begin(ctx, &tbl, pan, dbg->off, 0);
    {
      /* header */
      const struct dbg_tbl_col_def *col = 0;
      int tbl_lay[GUI_TBL_COL(DBG_TBL_MAX)];
      gui->tbl.hdr.begin(ctx, &tbl, tbl_lay, dbg->tbl_state);
      for_arrv(col, dbg_profile_tbl_def) {
        gui->tbl.hdr.slot.txt(ctx, &tbl, tbl_lay, dbg->tbl_state, col->title);
      }
      gui->tbl.hdr.end(ctx, &tbl);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui->tbl.lst.cfg(ctx, &cfg, dyn_cnt(dbg->lst));
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;

      gui->tbl.lst.begin(ctx, &tbl, &cfg);
      for_gui_tbl_lst(i,*gui,&tbl) {
        struct gui_panel elm = {0};
        struct dbg_elm *e = dbg->lst[i];
        unsigned long long elm_id = str_hash(str0(dbg->lst[i]->guid));
        gui->tbl.lst.elm.begin(ctx, &tbl, &elm, elm_id, 0);
        {
          const float ratio = cast(float, e->dur) / cast(float, dbg->tree->dur);
          gui->tbl.lst.elm.col.txtf(ctx, &tbl, tbl_lay, &elm, 0, "%llu", e->dur);
          gui->tbl.lst.elm.col.txtf(ctx, &tbl, tbl_lay, &elm, 0, "%.2f", ratio);
          gui->tbl.lst.elm.col.txt(ctx, &tbl, tbl_lay, &elm, str0(e->name), 0, 0);
        }
        gui->tbl.lst.elm.end(ctx, &tbl, &elm);
      }
      gui->tbl.lst.end(ctx, &tbl);
    }
    gui->tbl.end(ctx, &tbl, pan, dbg->off);
  }
  gui->pan.end(ctx, pan, parent);
  return res;
}
static void
dbg_enable(struct sys *sys) {
  assert(sys);
  struct dbg *dbg = cast(struct dbg*, sys->debug);
  dbg->collate = 1;
}
static void
dbg_disable(struct sys *sys) {
  assert(sys);
  struct dbg *dbg = cast(struct dbg*, sys->debug);
  dbg->collate = 0;
}
static void
dbg_init(struct sys *sys) {
  static const struct dbg_api dbg_api = {
    .rec = dbg_rec,
    .ui = dbg_ui,
    .enable = dbg_enable,
    .disable = dbg_disable,
  };
  assert(sys);
  sys->debug = arena_obj(sys->mem.arena, sys, struct dbg);
  sys->dbg = dbg_api;
}
#ifdef DEBUG_MODE
extern void
dlInit(struct sys *sys) {
  dbg_init(sys);
}
extern void
dlBegin(struct sys *sys) {
  dbg_begin(sys);
}
extern void
dlEnd(struct sys *sys) {
  dbg_end(sys);
}
#endif

