enum dbg_type {
  DBG_TYPE_UNKNOWN,
  DBG_TYPE_BLK_BEGIN,
  DBG_TYPE_BLK_END,

  DBG_TYPE_ARENA_NAME,
  DBG_TYPE_ARENA_BLK_FREE,
  DBG_TYPE_ARENA_BLK_ALLOC,
  DBG_TYPE_ARENA_ALLOC,
};
#define DBG_GUID__(A, B, C) A "|" #B "|" #C
#define DBG_GUID_(A, B, C) DBG_GUID__(A, B, C)
#define DBG_GUID DBG_GUID_(__FILE__, __LINE__, __COUNTER__)

#define dbg_blk_begin_(sys, guid, name) sys->dbg.rec(sys, DBG_TYPE_BLK_BEGIN, guid, name)
#define dbg_blk_end_(sys, guid, name) sys->dbg.rec(sys, DBG_TYPE_BLK_END, guid, name)
#define dbg_blk_begin(sys, name) dbg_blk_begin_(sys, DBG_GUID, name)
#define dbg_blk_end(sys) dbg_blk_end_(sys, DBG_GUID, "END_BLOCK")

#define dbg_arena_name(sys, a, name) sys->dbg.rec(sys, DBG_TYPE_ARENA_NAME, (const char*)a, name);

struct sys;
struct dbg_api {
  void (*rec)(struct sys *sys, enum dbg_type type, const char *guid, const char *name);
};


static void dbg_init(struct sys *sys);
static void dbg_begin(struct sys *sys);
static void dbg_end(struct sys *sys);

