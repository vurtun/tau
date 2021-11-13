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
struct dbg {
  struct dbg_tbl tbl;
};
extern void dlInit(struct sys *sys);
extern void dlBegin(struct sys *sys);
extern void dlEnd(struct sys *sys);

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
static const struct dbg_api dbg_api = {
  .rec = dbg_rec,
};
extern void
dlInit(struct sys *sys) {
  assert(sys);
  sys->debug = arena_obj(sys->mem.arena, sys, struct dbg);
  sys->dbg = dbg_api;
}
extern void
dlBegin(struct sys *sys) {
  assert(sys);
  dbg_blk_begin(sys, "FrameBegin");
}
extern void
dlEnd(struct sys *sys) {
  assert(sys);
  dbg_blk_end(sys);

  struct dbg *dbg = cast(struct dbg*, sys->debug);
  dbg->tbl.evt_buf_idx = !dbg->tbl.evt_buf_idx;
  unsigned long long new_idx = cast(unsigned long long, dbg->tbl.evt_buf_idx) << 32llu;
  atom_xchg(&dbg->tbl.idx, new_idx);
#if 0
  unsigned long long evt_idx = atom_xchg(&dbg->tbl.idx, new_idx);
  unsigned evt_buf_idx = evt_idx >> 32u;
  assert(evt_buf_idx <= 1);
  unsigned evt_cnt = evt_idx & 0xFFFFFFFF;
#endif
}

