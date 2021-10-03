enum dbg_type {
  dbg_type_unknown,
  dbg_type_name,
  dbg_type_frameMarker,
  dbg_type_blk_begin,
  dbg_type_blk_end,

  dbg_type_arena_name,
  dbg_type_arena_blk_free,
  dbg_type_arena_blk_alloc,
  dbg_type_arena_alloc,
};
#define DBG_ID__(A, B, C) A "|" #B "|" #C
#define DBG_ID_(A, B, C) DBG_ID__(A, B, C)
#define DBG_ID(Name) DBG_ID_(__FILE__, __LINE__, __COUNTER__)

#define DBG_BLK_BEGIN_(sys, guid, name) sys->dbg.rec(sys, dbg_type_blk_begin, guid, name)
#define DBG_BLK_END_(sys, guid, name) sys->dbg.rec(sys, dbg_type_blk_end, guid, name);
#define DBG_BLK_BEGIN(sys, name) DBG_BLK_BEGIN_(sys, DBG_ID(Name), name)
#define DBG_BLK_END(sys) DBG_BLK_END_(sys, DBG_ID("END_BLOCK"), "END_BLOCK")

struct sys;
struct dbg_api {
  void (*rec)(struct sys *sys, enum dbg_type type, const char *guid, const char *name);
};

