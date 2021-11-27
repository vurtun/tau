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

/* os */
#include <dirent.h>
#include <fcntl.h>
#include <locale.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <dlfcn.h>
#import <Cocoa/Cocoa.h>

/* usr */
#include "cpu.h"
#include "../lib/fmt.h"
#include "../lib/fmt.c"
#include "../lib/std.h"
#include "dbg.h"
#include "ren.h"
#include "sys.h"
#include "../lib/std.c"

@interface sys__mac_app_delegate : NSObject<NSApplicationDelegate>
@end
@interface sys__mac_window : NSWindow
@end
@interface sys__mac_window_delegate : NSObject<NSWindowDelegate>
@end
@interface sys__mac_view : NSView
@end

struct sys_mem_blk {
  struct mem_blk blk;
  struct lst_elm hook;
  unsigned long long tags;
};
struct sys_app_sym_table {
  void (*dlEntry)(struct sys *s);
  void (*dlRegister)(struct sys *s);
};
struct sys_dbg_sym_table {
  void (*dlInit)(struct sys *s);
  void (*dlBegin)(struct sys *s);
  void (*dlEnd)(struct sys *s);
};
struct sys_ren_sym_table {
  void (*dlInit)(struct sys *s);
  void (*dlBegin)(struct sys *s);
  void (*dlEnd)(struct sys *s);
  void (*dlShutdown)(struct sys *s);
};
struct sys_mac_module {
  unsigned valid : 1;
  struct str path;
  void *lib;
  ino_t fileId;

  int sym_cnt;
  char **sym_names;
  void **syms;
  void(*dlExport)(void *export, void *import);
};
struct sys_mac {
  int quit;
  struct lst_elm mem_blks;
  struct lck mem_lck;
  int col_mod;

  struct arena mem;
  struct arena tmp;
  enum sys_cur_style cursor;
  struct str exe_path;

  struct str dbg_path;
  struct sys_dbg_sym_table dbg;
  struct sys_mac_module dbg_lib;

  struct str ren_path;
  struct sys_ren_sym_table ren;
  struct sys_mac_module ren_lib;

  struct str app_path;
  struct sys_app_sym_table app;
  struct sys_mac_module app_lib;

  int mod_cnt;
  #define SYS_MAC_MAX_MODS 32
  struct sys_mac_module mods[SYS_MAC_MAX_MODS];

  NSWindow* win;
  NSTrackingArea* track_area;
  sys__mac_app_delegate* app_dlg;
  sys__mac_window_delegate* win_dlg;
  sys__mac_view *view;

  int win_w, win_h;
  unsigned *backbuf;
  CGColorSpaceRef col_space;
  CGContextRef ctx_ref;
  CGImageRef cg_img;
};
static struct sys_mac _mac;
static struct sys _sys;

#if __has_feature(objc_arc)
#define SYS__OBJ_REL(obj) { obj = nil; }
#else
#define SYS__OBJ_REL(obj) { [obj release]; obj = nil; }
#endif

static void
xpanic(const char *fmt, ...) {
  assert(fmt);
  va_list args;
  fflush(stdout);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  exit(EXIT_FAILURE);
}

/* ---------------------------------------------------------------------------
 *
 *                                File
 *
 * ---------------------------------------------------------------------------
 */
static struct str
sys_mac_get_exe_path(struct arena *a) {
  int siz = 0;
  _NSGetExecutablePath(0, &siz);
  char *buf = arena_alloc(a, &_sys, szof(char) * siz + 1);
  _NSGetExecutablePath(buf, &siz);
  return str(buf, siz);
}
static struct str
sys_mac_get_exe_file_path(struct str exe_path, struct str file,
                          struct str suffix, struct arena *a) {
  struct str exe_file = path_file(exe_path);
  struct str path = strp(exe_path.str, exe_file.str);
  return arena_fmt(a, &_sys, "%.*s%.*s%.*s", strf(path), strf(file), strf(suffix));
}
static void
sys__file_perm(char *mod, mode_t perm) {
  mod[0] = (perm & S_IRUSR) ? 'r' : '-';
  mod[1] = (perm & S_IWUSR) ? 'w' : '-';
  mod[2] = (perm & S_IXUSR) ? 'x' : '-';
  mod[3] = (perm & S_IRGRP) ? 'r' : '-';
  mod[4] = (perm & S_IWGRP) ? 'w' : '-';
  mod[5] = (perm & S_IXGRP) ? 'x' : '-';
  mod[6] = (perm & S_IROTH) ? 'r' : '-';
  mod[7] = (perm & S_IWOTH) ? 'w' : '-';
  mod[8] = (perm & S_IXOTH) ? 'x' : '-';
  mod[9] = 0;
}
static int
sys_file_info(struct sys_file_info *info, struct str path, struct arena *tmp) {
  assert(info);
  assert(tmp);

  struct scope scp;
  scope_begin(&scp, tmp);

  struct stat stats;
  char *fullpath = arena_cstr(tmp, &_sys, path);
  int res = stat(fullpath, &stats);
  scope_end(&scp, tmp, &_sys);
  if (res < 0) {
    return 0;
  }
  info->siz = cast(size_t, stats.st_size);
  info->mtime = stats.st_mtime;
  sys__file_perm(info->perm, stats.st_mode);

  if (S_ISDIR(stats.st_mode)) {
    info->type = SYS_FILE_DIR;
  } else if (S_ISLNK(stats.st_mode)) {
    info->type = SYS_FILE_LNK;
  } else if (S_ISSOCK(stats.st_mode)) {
    info->type = SYS_FILE_SOCK;
  } else if (S_ISFIFO(stats.st_mode)) {
    info->type = SYS_FILE_FIFO;
  } else {
    info->type = SYS_FILE_DEF;
  }
  return 1;
}

/* ---------------------------------------------------------------------------
 *
 *                                  Directory
 *
 * ---------------------------------------------------------------------------
 */
static void
sys__dir_free(struct sys_dir_iter *it, struct arena *a) {
  scope_end(&it->scp_base, a, &_sys);
  if (!it->valid) return;
  it->valid = 0;
  it->err = 0;
  closedir(it->handle);
}
static int
sys__dir_excl(struct sys_dir_iter *it) {
  int is_base = !str_cmp(it->name, str0("."));
  int is_prev = !str_cmp(it->name, str0(".."));
  return it->valid && (is_base || is_prev);
}
static int
sys_dir_exists(struct str path, struct arena *tmp) {
  struct scope scp;
  scope_begin(&scp, tmp);

  struct stat stats;
  char *fullpath = arena_cstr(tmp, &_sys, path);
  int res = stat(fullpath, &stats);
  scope_end(&scp, tmp, &_sys);
  if (res < 0 || !S_ISDIR(stats.st_mode)) {
    return 0;
  }
  return 1;
}
static void
sys_dir_nxt(struct sys_dir_iter *it, struct arena *a) {
  if (!it->valid) {
    return;
  }
  scope_end(&it->scp, a, &_sys);
  scope_begin(&it->scp, a);
  do {
    struct dirent *ent = readdir(it->handle);
    if (!ent) {
      scope_end(&it->scp, a, &_sys);
      sys__dir_free(it, a);
      return;
    }
    it->fullpath = arena_fmt(a, &_sys, "%.*s/%s", strf(it->base), ent->d_name);
    it->name = str0(ent->d_name);
    it->isdir = ent->d_type & DT_DIR;
  } while (sys__dir_excl(it));
}
static void
sys_dir_lst(struct sys_dir_iter *it, struct arena *a, struct str path) {
  memset(it, 0, sizeof(*it));
  scope_begin(&it->scp_base, a);
  it->base = arena_str(a, &_sys, path);

  DIR *dir = opendir(it->base.str);
  if (!dir) {
    it->valid = 0;
    it->err = 1;
  }
  it->handle = dir;
  it->valid = 1;

  scope_begin(&it->scp, a);
  sys_dir_nxt(it, a);
}

/* ---------------------------------------------------------------------------
 *
 *                              Memory
 *
 * ---------------------------------------------------------------------------
 */
static struct mem_blk*
sys_mac_mem_alloc(struct mem_blk* opt_old, int siz, unsigned flags,
                  unsigned long long tags) {
  compiler_assert(szof(struct sys_mem_blk) == 64);
  int total = siz + szof(struct sys_mem_blk);
  int base_off = szof(struct sys_mem_blk);

  struct sys_mem_blk *blk = 0;
  size_t mapsiz = cast(size_t, total);
  if (flags & SYS_MEM_GROWABLE) {
    if (opt_old){
      assert(opt_old->flags == flags);
      struct sys_mem_blk *sys_blk;
      sys_blk = containerof(opt_old, struct sys_mem_blk, blk);

      lck_acq(&_mac.mem_lck);
      lst_del(&sys_blk->hook);
      lck_rel(&_mac.mem_lck);
    }
    blk = realloc(opt_old, (size_t)total);
    blk->blk.base = (unsigned char*)blk + base_off;
  } else {
    blk = mmap(0, mapsiz, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    blk->blk.base = (unsigned char*)blk + base_off;
  }
  lst_init(&blk->hook);

  blk->blk.size = siz;
  blk->blk.flags = flags;
  blk->tags = tags;

  lck_acq(&_mac.mem_lck);
  lst_add(&_mac.mem_blks, &blk->hook);
  lck_rel(&_mac.mem_lck);
  return &blk->blk;
}
static void
sys_mac_mem_free(struct mem_blk *mem_blk) {
  struct sys_mem_blk *blk;
  blk = containerof(mem_blk, struct sys_mem_blk, blk);
  int total = blk->blk.size + szof(struct sys_mem_blk);

  lck_acq(&_mac.mem_lck);
  lst_del(&blk->hook);
  lck_rel(&_mac.mem_lck);

  if (mem_blk->flags & SYS_MEM_GROWABLE) {
    free(blk);
  } else {
    size_t unmapsiz = cast(size_t, total);
    munmap(blk, unmapsiz);
  }
}
static void
sys_mac_mem_stats(struct sys_mem_stats *stats) {
  memset(stats, 0, sizeof(*stats));
  lck_acq(&_mac.mem_lck);
  {
    struct lst_elm *elm = 0;
    for_lst(elm, &_mac.mem_blks) {
      struct sys_mem_blk *blk;
      blk = lst_get(elm, struct sys_mem_blk, hook);
      stats->total += blk->blk.size;
      stats->used += blk->blk.used;
    }
  }
  lck_rel(&_mac.mem_lck);
}
static void
sys_mac_mem_free_tag(unsigned long long tag) {
  lck_acq(&_mac.mem_lck);
  struct lst_elm *elm = 0;
  for_lst(elm, &_mac.mem_blks) {
    struct sys_mem_blk *blk = 0;
    blk = lst_get(elm, struct sys_mem_blk, hook);
    if (blk->tags == tag) {
      sys_mac_mem_free(&blk->blk);
    }
  }
  lck_rel(&_mac.mem_lck);
}

/* ---------------------------------------------------------------------------
 *
 *                            Dynamic Library
 *
 * ---------------------------------------------------------------------------
 */
static void *
sys_mac_lib_sym(void *lib, const char *s) {
  void *sym = dlsym(lib, s);
  if (!sym) {
    fprintf(stderr, "failed to load dynamic library symbol: %s\n", dlerror());
  }
  return sym;
}
static void *
sys_mac_lib_open(const char *path) {
  void *lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!lib) {
    fprintf(stderr, "failed to open dynamic library: %s\n", dlerror());
  }
  return lib;
}
static void
sys_mac_lib_close(void *lib) {
  if (lib != 0) {
    dlclose(lib);
  }
}
static ino_t
sys_mac_file_id(struct str path) {
  struct stat attr = {0};
  if (stat(path.str, &attr)) {
    attr.st_ino = 0;
  }
  return attr.st_ino;
}
static void
sys_mac_mod_close(struct sys_mac_module *mod) {
  if (mod->lib) {
    sys_mac_lib_close(mod->lib);
    mod->lib = 0;
  }
  mod->fileId = 0;
  mod->valid = 0;
}
static void
sys_mac_mod_open(struct sys_mac_module *mod, struct arena *mem) {
  ino_t fileID = sys_mac_file_id(mod->path);
  if (mod->fileId == fileID) {
    if(!mod->valid) {
      sys_mac_mod_close(mod);
    }
    return;
  }
  struct scope scp = {0};
  scope_begin(&scp, mem);
  {
    mod->fileId = fileID;
    mod->valid = 1;
    mod->lib = sys_mac_lib_open(mod->path.str);
    if (mod->lib) {
      for (int i = 0; i < mod->sym_cnt; ++i) {
        void *sym = sys_mac_lib_sym(mod->lib, mod->sym_names[i]);
        if (sym) {
          mod->syms[i] = sym;
        } else {
          mod->valid = 0;
        }
      }
    }
    if(!mod->valid) {
      sys_mac_mod_close(mod);
    }
  }
  scope_end(&scp, mem, &_sys);
}
static int
sys_mac_mod_chk(struct sys_mac_module *mod) {
  ino_t libId = sys_mac_file_id(mod->path);
  return libId != mod->fileId;
}
static void
sys_mac_mod_reload(struct sys_mac_module *mod, struct arena *a) {
  sys_mac_mod_close(mod);
  for(int i = 0; !mod->valid && i < 100; ++i) {
    sys_mac_mod_open(mod, a);
    usleep(100000);
  }
}

/* ---------------------------------------------------------------------------
 *
 *                                Clipboard
 *
 * ---------------------------------------------------------------------------
 */
static void
sys_mac_clipboard_set(struct str s, struct arena *a) {
  struct scope scp = {0};
  scope_begin(&scp, a);
  {
    struct str tmp = arena_str(a, &_sys, s);
    @autoreleasepool {
      NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
      [pasteboard declareTypes:@[NSPasteboardTypeString] owner:nil];
      [pasteboard setString:@(tmp.str) forType:NSPasteboardTypeString];
    }
  }
  scope_end(&scp, a, &_sys);
}
static struct str
sys_mac_clipboard_get(struct arena *a) {
  struct str data = str_nil;
  @autoreleasepool {
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    if (![[pasteboard types] containsObject:NSPasteboardTypeString]) {
      return str_nil;
    }
    NSString* str = [pasteboard stringForType:NSPasteboardTypeString];
    if (!str) {
      return str_nil;
    }
    struct str cs = str0([str UTF8String]);
    data = arena_str(a, &_sys, cs);
  }
  return data;
}

/* ---------------------------------------------------------------------------
 *
 *                                APP
 *
 * ---------------------------------------------------------------------------
 */
static void
sys__mac_free(void) {
  SYS__OBJ_REL(_mac.track_area);
  SYS__OBJ_REL(_mac.app_dlg);
  SYS__OBJ_REL(_mac.win_dlg);
  SYS__OBJ_REL(_mac.win);
  SYS__OBJ_REL(_mac.view);
  if (_mac.ctx_ref) {
    CGContextRelease(_mac.ctx_ref);
    _mac.ctx_ref = 0;
  }
  if (_mac.col_space) {
    CGColorSpaceRelease(_mac.col_space);
    _mac.col_space = 0;
  }
  if (_mac.cg_img) {
    CGImageRelease(_mac.cg_img);
  }
}
static void
sys__mac_on_frame(void) {
  if (_mac.dbg.dlBegin) {
    _mac.dbg.dlBegin(&_sys);
  }
  _sys.txt_mod = !!_sys.txt_len;
  _sys.win.w = _mac.win_w;
  _sys.win.h = _mac.win_h;
  if (_mac.ren.dlBegin) {
    _mac.ren.dlBegin(&_sys);
  }
  if (_mac.app.dlEntry)  {
    _mac.app.dlEntry(&_sys);
  }
  if (_mac.ren.dlEnd) {
    dyn_clr(_sys.ren_target.dirty_rects);
    _sys.ren_target.w = _mac.win_w;
    _sys.ren_target.h = _mac.win_h;
    _sys.ren_target.pixels = _mac.backbuf;
    _mac.ren.dlEnd(&_sys);

    _sys.ren_target.resized = 0;
    if (dyn_any(_sys.ren_target.dirty_rects)) {
      [_mac.view setNeedsDisplay:YES];
    }
  }
  _sys.seq++;
  _sys.col_mod = 0;
  _sys.keymod = 0;
  _sys.txt_len = 0;
  _sys.focus = 0;
  _sys.btn_mod = 0;
  _sys.txt_mod = 0;
  _sys.key_mod = 0;
  _sys.scrl_mod = 0;
  _sys.mouse_mod = 0;
  _sys.mouse.pos_last[0] = _sys.mouse.pos[0];
  _sys.mouse.pos_last[1] = _sys.mouse.pos[1];
  for (int i = 0; i < cntof(_sys.keys); ++i){
    _sys.keys[i] = 0;
  }
  for (int i = 0; i < SYS_MOUSE_BTN_CNT; ++i) {
    _sys.mouse.btns[i].pressed = 0;
    _sys.mouse.btns[i].released = 0;
    _sys.mouse.btns[i].doubled = 0;
  }
  if (_mac.dbg.dlEnd) {
    _mac.dbg.dlEnd(&_sys);
  }
}
static void
sys_mac__resize(void) {
  NSRect bounds = [_mac.view bounds];
  int w = max(1, cast(int, bounds.size.width));
  int h = max(1, cast(int, bounds.size.height));
  if (_mac.win_w == w && _mac.win_h == h) {
    return;
  }
  if (_mac.ctx_ref) {
    CGContextRelease(_mac.ctx_ref);
    _mac.ctx_ref = 0;
    munmap(_mac.backbuf, cast(size_t, _mac.win_w * _mac.win_h * 4));
    _mac.backbuf = 0;
  }
  size_t bpc = 8;
  size_t num_comp = CGColorSpaceGetNumberOfComponents(_mac.col_space) + 1u;
  size_t bpp = (bpc * num_comp) / 8;
  size_t bpr = bpp * cast(size_t, w);

  _mac.win_w = w;
  _mac.win_h = h;
  _mac.backbuf = mmap(0, cast(size_t, w*h*4), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  _mac.ctx_ref = CGBitmapContextCreate(_mac.backbuf, cast(size_t, _mac.win_w),
      cast(size_t, _mac.win_h), bpc, bpr, _mac.col_space, kCGImageAlphaNoneSkipLast);
  _sys.ren_target.resized = 1;
  sys__mac_on_frame();
  [_mac.view setNeedsDisplay:YES];
}
static unsigned
sys__mac_col(NSColor *col) {
  @autoreleasepool {
    NSColor *ref = [col colorUsingColorSpace: [NSColorSpace deviceRGBColorSpace]];
    unsigned char r = cast(unsigned char, [ref redComponent] * 255.0);
    unsigned char g = cast(unsigned char, [ref greenComponent] * 255.0);
    unsigned char b = cast(unsigned char, [ref blueComponent] * 255.0);
    unsigned char a = cast(unsigned char, [ref alphaComponent] * 255.0);
    return col_rgba(r,g,b,a);
  }
}
static void
sys__mac_load_col(void) {
  _sys.col_mod = 1;
  _sys.col[SYS_COL_HOV] = sys__mac_col([NSColor controlBackgroundColor]);
  _sys.col[SYS_COL_WIN] = sys__mac_col([NSColor windowBackgroundColor]);
  _sys.col[SYS_COL_BG] = sys__mac_col([NSColor controlBackgroundColor]);
  _sys.col[SYS_COL_CTRL] = sys__mac_col([NSColor controlColor]);
  _sys.col[SYS_COL_SEL] = sys__mac_col([NSColor selectedControlColor]);
  _sys.col[SYS_COL_TXT] = sys__mac_col([NSColor controlTextColor]);
  _sys.col[SYS_COL_TXT_SEL] = sys__mac_col([NSColor selectedControlTextColor]);
  _sys.col[SYS_COL_TXT_DISABLED] = sys__mac_col([NSColor tertiaryLabelColor]);
  _sys.col[SYS_COL_ICO] = sys__mac_col([NSColor controlTextColor]);
  _sys.col[SYS_COL_LIGHT] = sys__mac_col([NSColor unemphasizedSelectedContentBackgroundColor]);
  _sys.col[SYS_COL_SHADOW] = sys__mac_col([NSColor underPageBackgroundColor]);
}
@implementation sys__mac_app_delegate
- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  _mac.col_space = CGColorSpaceCreateDeviceRGB();
  sys__mac_load_col();

  /* create window */
  const NSUInteger style =
    NSWindowStyleMaskTitled |
    NSWindowStyleMaskClosable |
    NSWindowStyleMaskMiniaturizable |
    NSWindowStyleMaskResizable;

  NSRect window_rect = NSMakeRect(0, 0, 800, 600);
  if (_mac.ren.dlInit) {
    _mac.ren.dlInit(&_sys);
  }
  _mac.win = [[sys__mac_window alloc]
    initWithContentRect:window_rect
    styleMask:style
    backing:NSBackingStoreBuffered
    defer:NO];

  _mac.win.releasedWhenClosed = NO;
  _mac.win.title = [NSString stringWithUTF8String:"Lepton"];
  _mac.win.acceptsMouseMovedEvents = YES;
  _mac.win.restorable = YES;
  _mac.win_dlg = [[sys__mac_window_delegate alloc] init];
  _mac.win.delegate = _mac.win_dlg;
  _mac.view = [[sys__mac_view alloc] initWithFrame:window_rect];
  [_mac.view updateTrackingAreas];
  [_mac.win setContentView:_mac.view];
  [_mac.win center];

  NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
  [NSApp activateIgnoringOtherApps:YES];
  [_mac.win makeKeyAndOrderFront:nil];
  sys_mac__resize();
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
  return YES;
}
- (void)applicationWillTerminate:(NSNotification*)notification {
  if (_mac.ren.dlShutdown) {
    _mac.ren.dlShutdown(&_sys);
  }
  sys_mac_mod_close(&_mac.app_lib);
  sys_mac_mod_close(&_mac.ren_lib);
  sys__mac_free();
}
@end

@implementation sys__mac_window_delegate
- (BOOL)windowShouldClose:(id)sender {
  return YES;
}
- (void)windowDidResize:(NSNotification*)notification {
  sys_mac__resize();
}
- (void)windowDidMiniaturize:(NSNotification*)notification {

}
- (void)windowDidDeminiaturize:(NSNotification*)notification {

}
- (void)windowDidEnterFullScreen:(NSNotification*)notification {

}
- (void)windowDidExitFullScreen:(NSNotification*)notification {

}
@end

@implementation sys__mac_window
- (instancetype)initWithContentRect:(NSRect)contentRect
                          styleMask:(NSWindowStyleMask)style
                            backing:(NSBackingStoreType)backingStoreType
                              defer:(BOOL)flag {
  self = [super initWithContentRect:contentRect styleMask:style backing:backingStoreType defer:flag];
  if (self) {
    #if __MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
      [self registerForDraggedTypes:[NSArray arrayWithObject:NSPasteboardTypeFileURL]];
    #endif
  }
  return self;
}
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
  return NSDragOperationCopy;
}
- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
  return NSDragOperationCopy;
}
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
  return NO;
}
@end

static void
sys__mac_on_btn(struct sys_btn *b, int down) {
  assert(b);
  int was_down = b->down;
  b->down = !!down;
  b->pressed = !was_down && down;
  b->released = was_down && !down;
  b->doubled = 0;
}
static unsigned
sys__mac_mods(NSEvent* ev) {
  unsigned res = 0u;
  const NSEventModifierFlags flg = ev.modifierFlags;
  if (flg & NSEventModifierFlagCommand) {
    res |= SYS_KEYMOD_CTRL;
  }
  if (flg & NSEventModifierFlagShift) {
    res |= SYS_KEYMOD_SHIFT;
  }
  if (flg & NSEventModifierFlagOption) {
    res |= SYS_KEYMOD_ALT;
  }
  return res;
}
static void
sys__macos_on_key(unsigned long *keys, int scan) {
  switch (scan) {
  default: break;
  case 0x1D: bit_set(keys, '0'); break;
  case 0x12: bit_set(keys, '1'); break;
  case 0x13: bit_set(keys, '2'); break;
  case 0x14: bit_set(keys, '3'); break;
  case 0x15: bit_set(keys, '4'); break;
  case 0x17: bit_set(keys, '5'); break;
  case 0x16: bit_set(keys, '6'); break;
  case 0x1A: bit_set(keys, '7'); break;
  case 0x1C: bit_set(keys, '8'); break;
  case 0x19: bit_set(keys, '9'); break;
  case 0x00: bit_set(keys, 'a'); bit_set(keys, 'A'); break;
  case 0x0B: bit_set(keys, 'b'); bit_set(keys, 'B'); break;
  case 0x08: bit_set(keys, 'c'); bit_set(keys, 'C'); break;
  case 0x02: bit_set(keys, 'd'); bit_set(keys, 'D'); break;
  case 0x0E: bit_set(keys, 'e'); bit_set(keys, 'E'); break;
  case 0x03: bit_set(keys, 'f'); bit_set(keys, 'F'); break;
  case 0x05: bit_set(keys, 'g'); bit_set(keys, 'G'); break;
  case 0x04: bit_set(keys, 'h'); bit_set(keys, 'H'); break;
  case 0x22: bit_set(keys, 'i'); bit_set(keys, 'I'); break;
  case 0x26: bit_set(keys, 'j'); bit_set(keys, 'J'); break;
  case 0x28: bit_set(keys, 'k'); bit_set(keys, 'K'); break;
  case 0x25: bit_set(keys, 'l'); bit_set(keys, 'L'); break;
  case 0x2E: bit_set(keys, 'm'); bit_set(keys, 'M'); break;
  case 0x2D: bit_set(keys, 'n'); bit_set(keys, 'N'); break;
  case 0x1F: bit_set(keys, 'o'); bit_set(keys, 'O'); break;
  case 0x23: bit_set(keys, 'p'); bit_set(keys, 'P'); break;
  case 0x0C: bit_set(keys, 'q'); bit_set(keys, 'Q'); break;
  case 0x0F: bit_set(keys, 'r'); bit_set(keys, 'R'); break;
  case 0x01: bit_set(keys, 's'); bit_set(keys, 'S'); break;
  case 0x11: bit_set(keys, 't'); bit_set(keys, 'T'); break;
  case 0x20: bit_set(keys, 'u'); bit_set(keys, 'U'); break;
  case 0x09: bit_set(keys, 'v'); bit_set(keys, 'V'); break;
  case 0x0D: bit_set(keys, 'w'); bit_set(keys, 'W'); break;
  case 0x07: bit_set(keys, 'x'); bit_set(keys, 'X'); break;
  case 0x10: bit_set(keys, 'y'); bit_set(keys, 'Y'); break;
  case 0x06: bit_set(keys, 'z'); bit_set(keys, 'Z'); break;
  case 0x27: bit_set(keys, '"'); break;
  case 0x2A: bit_set(keys, '/'); break;
  case 0x2B: bit_set(keys, ','); break;
  case 0x18: bit_set(keys, '='); break;
  case 0x32: bit_set(keys, '`'); break;
  case 0x21: bit_set(keys, '('); break;
  case 0x1B: bit_set(keys, '-'); break;
  case 0x2F: bit_set(keys, '.'); break;
  case 0x1E: bit_set(keys, ']'); break;
  case 0x29: bit_set(keys, ';'); break;
  case 0x2C: bit_set(keys, '/'); break;
  case 0x33: bit_set(keys, SYS_KEY_BACKSPACE); break;
  case 0x39: bit_set(keys, SYS_KEY_CAPS); break;
  case 0x75: bit_set(keys, SYS_KEY_DEL); break;
  case 0x7D: bit_set(keys, SYS_KEY_DOWN); break;
  case 0x77: bit_set(keys, SYS_KEY_END); break;
  case 0x24: bit_set(keys, SYS_KEY_RETURN); break;
  case 0x35: bit_set(keys, SYS_KEY_ESCAPE); break;
  case 0x7A: bit_set(keys, SYS_KEY_F1); break;
  case 0x78: bit_set(keys, SYS_KEY_F2); break;
  case 0x63: bit_set(keys, SYS_KEY_F3); break;
  case 0x76: bit_set(keys, SYS_KEY_F4); break;
  case 0x60: bit_set(keys, SYS_KEY_F5); break;
  case 0x61: bit_set(keys, SYS_KEY_F6); break;
  case 0x62: bit_set(keys, SYS_KEY_F7); break;
  case 0x64: bit_set(keys, SYS_KEY_F8); break;
  case 0x65: bit_set(keys, SYS_KEY_F9); break;
  case 0x6D: bit_set(keys, SYS_KEY_F10); break;
  case 0x67: bit_set(keys, SYS_KEY_F11); break;
  case 0x6F: bit_set(keys, SYS_KEY_F12); break;
  case 0x73: bit_set(keys, SYS_KEY_HOME); break;
  case 0x7B: bit_set(keys, SYS_KEY_LEFT); break;
  case 0x79: bit_set(keys, SYS_KEY_PGDN); break;
  case 0x74: bit_set(keys, SYS_KEY_PGUP); break;
  case 0x7C: bit_set(keys, SYS_KEY_RIGHT); break;
  case 0x31: bit_set(keys, ' '); break;
  case 0x30: bit_set(keys, SYS_KEY_TAB); break;
  case 0x7E: bit_set(keys, SYS_KEY_UP); break;
  case 0x52: bit_set(keys, '0'); break;
  case 0x53: bit_set(keys, '1'); break;
  case 0x54: bit_set(keys, '2'); break;
  case 0x55: bit_set(keys, '3'); break;
  case 0x56: bit_set(keys, '4'); break;
  case 0x57: bit_set(keys, '5'); break;
  case 0x58: bit_set(keys, '6'); break;
  case 0x59: bit_set(keys, '7'); break;
  case 0x5B: bit_set(keys, '8'); break;
  case 0x5C: bit_set(keys, '9'); break;
  case 0x45: bit_set(keys, '+'); break;
  case 0x41: bit_set(keys, '.'); break;
  case 0x4B: bit_set(keys, '/'); break;
  case 0x4C: bit_set(keys, SYS_KEY_RETURN); break;
  case 0x51: bit_set(keys, '='); break;
  case 0x43: bit_set(keys, '*'); break;
  case 0x4E: bit_set(keys, '-'); break;
  }
}
@implementation sys__mac_view
- (void)reshape {
  sys_mac__resize();
}
- (void)windowDidExpose {
  sys_mac__resize();
}
- (void)drawRect:(NSRect)rect {
  if (_mac.col_mod) {
    sys__mac_load_col();
    _sys.ren_target.resized = 1;
    sys__mac_on_frame();
    _mac.col_mod = 0;
  }
  if (dyn_any(_sys.ren_target.dirty_rects)) {
    CGImageRelease(_mac.cg_img);
    _mac.cg_img = CGBitmapContextCreateImage(_mac.ctx_ref);
  }
  if (_mac.cg_img) {
    CGContextRef gctx = [[NSGraphicsContext currentContext] CGContext];
    CGContextDrawImage(gctx, CGRectMake(0, 0, _mac.win_w, _mac.win_h), _mac.cg_img);
  }
  dyn_clr(_sys.ren_target.dirty_rects);
}
- (void)viewDidChangeEffectiveAppearance {
  _mac.col_mod = 1;
  [_mac.view setNeedsDisplay:YES];
}
- (BOOL)isOpaque {return YES;}
- (BOOL)canBecomeKeyView {return YES;}
- (BOOL)acceptsFirstResponder {return YES;}
- (void)updateTrackingAreas {
    if (_mac.track_area != nil) {
      [self removeTrackingArea:_mac.track_area];
      SYS__OBJ_REL(_mac.track_area);
    }
    const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
      NSTrackingActiveInKeyWindow | NSTrackingEnabledDuringMouseDrag |
      NSTrackingCursorUpdate | NSTrackingInVisibleRect | NSTrackingAssumeInside;
    _mac.track_area = [[NSTrackingArea alloc] initWithRect:[self bounds]
                       options:options owner:self userInfo:nil];
    [self addTrackingArea:_mac.track_area];
    [super updateTrackingAreas];
}
- (void)mouseEntered:(NSEvent*)e {

}
- (void)mouseExited:(NSEvent*)e {

}
static void
sys_mac__mouse_pos(NSEvent *e) {
  NSPoint pos = e.locationInWindow;
  float new_x = cast(float, pos.x) * _sys.dpi_scale[0];
  float new_y = cast(float, pos.y) * _sys.dpi_scale[1];

  _sys.mouse.pos[0] = cast(int, new_x);
  _sys.mouse.pos[1] = _sys.win.h - cast(int, new_y) - 1;

  _sys.mouse.pos_delta[0] = _sys.mouse.pos[0] - _sys.mouse.pos_last[0];
  _sys.mouse.pos_delta[1] = _sys.mouse.pos[1] - _sys.mouse.pos_last[1];
}
- (void)mouseDown:(NSEvent*)e {
  sys__mac_on_btn(&_sys.mouse.btn.left, 1);
  if (e.clickCount == 2) {
    _sys.mouse.btn.left.doubled = 1;
  }
  _sys.btn_mod = 1;
  _sys.mouse_mod = 1;
  _sys.keymod |= sys__mac_mods(e);
  sys__mac_on_frame();
}
- (void)mouseUp:(NSEvent*)e {
  sys__mac_on_btn(&_sys.mouse.btn.left, 0);

  _sys.btn_mod = 1;
  _sys.mouse_mod = 1;
  _sys.keymod |= sys__mac_mods(e);
  sys__mac_on_frame();
}
- (void)rightMouseDown:(NSEvent*)e {
  sys__mac_on_btn(&_sys.mouse.btn.right, 1);

  _sys.btn_mod = 1;
  _sys.mouse_mod = 1;
  _sys.keymod |= sys__mac_mods(e);
  sys__mac_on_frame();
}
- (void)rightMouseUp:(NSEvent*)e {
  sys__mac_on_btn(&_sys.mouse.btn.right, 0);

  _sys.btn_mod = 1;
  _sys.mouse_mod = 1;
  _sys.keymod |= sys__mac_mods(e);
  sys__mac_on_frame();
}
- (void)otherMouseDown:(NSEvent*)e {
  if (2 == e.buttonNumber) {
    sys__mac_on_btn(&_sys.mouse.btn.middle, 1);
    sys__mac_on_frame();
  }
}
- (void)otherMouseUp:(NSEvent*)e {
  if (2 == e.buttonNumber) {
    sys__mac_on_btn(&_sys.mouse.btn.middle, 0);
    sys__mac_on_frame();
  }
}
- (void)mouseMoved:(NSEvent*)e {
  sys_mac__mouse_pos(e);
  _sys.mouse_mod = 1;
  _sys.keymod |= sys__mac_mods(e);
  if (abs(_sys.mouse.pos_delta[0]) > 0 ||
      abs(_sys.mouse.pos_delta[1]) > 0) {
    sys__mac_on_frame();
  }
}
- (void)mouseDragged:(NSEvent*)e {
}
- (void)rightMouseDragged:(NSEvent*)e {
}
- (void)otherMouseDragged:(NSEvent*)e {
}
- (void)scrollWheel:(NSEvent*)e {
  float dx = cast(float, e.scrollingDeltaX);
  float dy = cast(float, e.scrollingDeltaY);
  if (e.hasPreciseScrollingDeltas) {
    dx *= 0.1f, dy *= 0.1f;
  }
  if ((fabs(dx) >= 1.0f) || (fabs(dy) >= 1.0f)) {
    _sys.keymod |= sys__mac_mods(e);
    _sys.mouse.scrl[0] = cast(int, dx);
    _sys.mouse.scrl[1] = cast(int, dy);
    _sys.scrl_mod = 1;
  }
  sys__mac_on_frame();
}
- (void)keyDown:(NSEvent*)e {
  _sys.keymod |= sys__mac_mods(e);
  sys__macos_on_key(_sys.keys, e.keyCode);
  _sys.key_mod = 1;

  const NSString* chars = e.characters;
  const NSUInteger len = chars.length;
  if (len > 0) {
    for (NSUInteger i = 0; i < len; i++) {
      const unichar codepoint = [chars characterAtIndex:i];
      if ((codepoint & 0xFF00) == 0xF700) {
        continue;
      }
      char buf[UTF_SIZ+1];
      int n = utf_enc(buf, cntof(buf), codepoint);
      if (_sys.txt_len + n < cntof(_sys.txt)) {
        memcpy(_sys.txt + _sys.txt_len, buf, cast(size_t, n));
        _sys.txt_len += n;
      }
    }
    _sys.txt_mod = 1;
  }
  sys__mac_on_frame();
}
- (void)flagsChanged:(NSEvent*)event {

}
- (void)cursorUpdate:(NSEvent*)event {

}
@end

static unsigned long long
sys_mac_timestamp(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  unsigned long long sec = cast(unsigned long long, ts.tv_sec) * 1000000llu;
  unsigned long long nsec = cast(unsigned long long, ts.tv_nsec);
  unsigned long long ret = sec + nsec;
  return ret;
}
static int
sys_mac_plugin_add(void *exp, void *imp, struct str name) {
  assert(_mac.mod_cnt < SYS_MAC_MAX_MODS);
  int at = _mac.mod_cnt++;
  struct sys_mac_module *mod = _mac.mods + at;

  static char *sys_module_fn_sym[] = {"dlExport"};
  mod->path = sys_mac_get_exe_file_path(_mac.exe_path, name, strv(".so"), &_mac.mem);
  mod->sym_cnt = cntof(sys_module_fn_sym);
  mod->sym_names = sys_module_fn_sym;
  mod->syms = (void**)&mod->dlExport;
  sys_mac_mod_open(mod, &_mac.tmp);
  if (mod->dlExport) {
    mod->dlExport(exp, imp);
  }
  return mod->valid;
}
extern int
main(int argc, char* argv[]) {
  _sys.app = 0;
  _sys.platform = &_mac;
  _sys.argc = argc;
  _sys.argv = argv;
  cpu_info(&_sys.cpu);

  /* memory */
  _sys.mem.alloc = sys_mac_mem_alloc;
  _sys.mem.free = sys_mac_mem_free;
  _sys.mem.info = sys_mac_mem_stats;
  _sys.mem.free_tag = sys_mac_mem_free_tag;
  _sys.mem.arena = &_mac.mem;
  _sys.mem.tmp = &_mac.tmp;
  _sys.mem.page_siz = sysconf(_SC_PAGE_SIZE);
  _sys.mem.phy_siz = sysconf(_SC_PHYS_PAGES) * _sys.mem.page_siz;
  lst_init(&_mac.mem_blks);

  /* directory */
  _sys.dir.lst = sys_dir_lst;
  _sys.dir.nxt = sys_dir_nxt;
  _sys.dir.exists = sys_dir_exists;
  /* clipboard */
  _sys.clipboard.set = sys_mac_clipboard_set;
  _sys.clipboard.get = sys_mac_clipboard_get;
  /* plugin */
  _sys.plugin.add = sys_mac_plugin_add;
  /* time */
  _sys.time.timestamp = sys_mac_timestamp;
  /* file */
  _sys.file.info = sys_file_info;

  /* constants */
  _sys.dpi_scale[0] = 1.0f;
  _sys.dpi_scale[1] = 1.0f;
  _mac.exe_path = sys_mac_get_exe_path(&_mac.mem);
  _mac.ren_path = sys_mac_get_exe_file_path(_mac.exe_path, strv("ren"), strv(".so"), &_mac.mem);
  _mac.app_path = sys_mac_get_exe_file_path(_mac.exe_path, strv("app"), strv(".so"), &_mac.mem);
  _mac.dbg_path = sys_mac_get_exe_file_path(_mac.exe_path, strv("dbg"), strv(".so"), &_mac.mem);

  /* open dbg dynamic library */
  static char *sys_dbg_module_fn_sym[] = {"dlInit","dlBegin","dlEnd"};
  _mac.dbg_lib.path = _mac.dbg_path;
  _mac.dbg_lib.sym_cnt = cntof(sys_dbg_module_fn_sym);
  _mac.dbg_lib.sym_names = sys_dbg_module_fn_sym;
  _mac.dbg_lib.syms = (void**)&_mac.dbg;
  sys_mac_mod_open(&_mac.dbg_lib, &_mac.tmp);
  if (_mac.dbg.dlInit) {
    _mac.dbg.dlInit(&_sys);
  }
  /* open ren dynamic library */
  static char *sys_ren_module_fn_sym[] = {"dlInit","dlBegin","dlEnd","dlShutdown"};
  _mac.ren_lib.path = _mac.ren_path;
  _mac.ren_lib.sym_cnt = cntof(sys_ren_module_fn_sym);
  _mac.ren_lib.sym_names = sys_ren_module_fn_sym;
  _mac.ren_lib.syms = (void**)&_mac.ren;
  sys_mac_mod_open(&_mac.ren_lib, &_mac.tmp);

  /* open app dynamic library */
  static char *sys_app_module_fn_sym[] = {"dlEntry", "dlRegister"};
  _mac.app_lib.path = _mac.app_path;
  _mac.app_lib.sym_cnt = cntof(sys_app_module_fn_sym);
  _mac.app_lib.sym_names = sys_app_module_fn_sym;
  _mac.app_lib.syms = (void**)&_mac.app;
  sys_mac_mod_open(&_mac.app_lib, &_mac.tmp);
  if (_mac.app.dlRegister)  {
    _mac.app.dlRegister(&_sys);
  }
  /* start application */
  [NSApplication sharedApplication];
  _mac.app_dlg = [[sys__mac_app_delegate alloc] init];
  NSApp.delegate = _mac.app_dlg;
  [NSApp run];
  return 0;
}

