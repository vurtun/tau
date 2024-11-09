#define _XOPEN_SOURCE

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
#include <ucontext.h>

#import <Cocoa/Cocoa.h>

/* usr */
#include "../cpu.h"
#include "../../lib/fmt.h"
#include "../../lib/fmt.c"
#include "../../lib/std.h"
#include "../gfx.h"
#include "../sys.h"
#include "../../lib/std.c"
#include "../../lib/math.c"
#include "gfx_mtl.c"

@interface sys__mac_window_delegate : NSObject<NSWindowDelegate>
@end
@interface sys__mac_view : MTKView
@property (nonatomic, assign) struct sys *sys;
@end
@interface sys__mac_view_delegate : NSViewController<MTKViewDelegate>
@property (nonatomic, assign) struct sys *sys;
@end

struct sys_mem_blk {
  struct mem_blk blk;
  struct lst_elm hook;
  unsigned long long tags;
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
  struct gfx_mtl mtl;

  NSWindow* win;
  sys__mac_window_delegate *win_dlg;
  sys__mac_view *view;
  sys__mac_view_delegate *view_dlg;
  unsigned long long tooltip;

  char* run_loop_fiber_stack;
  ucontext_t run_loop_fiber;
  ucontext_t main_fiber;

  float dpi_scale[2];
  int win_w, win_h;
};

static void sys_mac_prep(struct sys *s);

/* ---------------------------------------------------------------------------
 *                                Util
 * ---------------------------------------------------------------------------
 */
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
static unsigned
sys__mac_col(NSColor *col) {
  @autoreleasepool {
    NSColor *ref = [col colorUsingColorSpace: [NSColorSpace deviceRGBColorSpace]];
    unsigned char r = castb([ref redComponent] * 255.0);
    unsigned char g = castb([ref greenComponent] * 255.0);
    unsigned char b = castb([ref blueComponent] * 255.0);
    unsigned char a = castb([ref alphaComponent] * 255.0);
    return col_rgba(r,g,b,a);
  }
}
static unsigned long long
sys_mac_timestamp(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  unsigned long long sec = castull(ts.tv_sec) * 1000000llu;
  unsigned long long usec = castull(ts.tv_nsec) / 1000;
  unsigned long long ret = sec + usec;
  return ret;
}

/* ---------------------------------------------------------------------------
 *                                  Random
 * ---------------------------------------------------------------------------
 */
static uintptr_t
sys__rnd_open(void) {
  int fp = open("/dev/urandom", O_RDONLY);
  if (fp == -1) {
    fprintf(stderr, "failed to access system random number\n");
    exit(1);
  }
  return castull(fp);
}
static void
sys__rnd_close(uintptr_t hdl) {
  int fp = casti(hdl);
  close(fp);
}
static unsigned
sys__rnd_gen32(uintptr_t hdl) {
  unsigned val;
  int fp = casti(hdl);
  ssize_t ret = read(fp, cast(char*, &val), sizeof(val));
  if (ret < szof(val)) {
    fprintf(stderr, "failed to generate system random number\n");
    exit(1);
  }
  return val;
}
static unsigned long long
sys__rnd_gen64(uintptr_t hdl) {
  int fp = casti(hdl);
  unsigned long long val = 0;
  ssize_t ret = read(fp, cast(char*, &val), sizeof(val));
  if (ret < szof(val)) {
    fprintf(stderr, "failed to generate system random number\n");
    exit(1);
  }
  return val;
}
static void
sys__rnd_gen128(uintptr_t hdl, void *dst) {
  int fp = casti(hdl);
  ssize_t ret = read(fp, cast(char*, dst), 16);
  if (ret < 16) {
    fprintf(stderr, "failed to generate system random number\n");
    exit(1);
  }
}

/* ---------------------------------------------------------------------------
 *                                File
 * ---------------------------------------------------------------------------
 */
struct sys_file_path {
  char *ospath;
  char buf[MAX_FILE_PATH];
};
static int
sys__file_path_push(struct sys_file_path *p, struct str path) {
  assert(p);
  if (str_len(path) + 1 < MAX_FILE_PATH) {
    mcpy(p->buf, str_beg(path), str_len(path));
    p->buf[str_len(path)] = 0;
    p->ospath = p->buf;
  } else {
    p->ospath = calloc(1, castsz(str_len(path) + 1));
    if (!p->ospath) {
      return 0;
    }
    mcpy(p->ospath, str_beg(path), str_len(path));
    p->ospath[str_len(path)] = 0;
  }
  return 1;
}
static void
sys__file_path_pop(struct sys_file_path *p) {
  assert(p);
  if (p->ospath != p->buf) {
    free(p->ospath);
  }
  mset(p, 0, szof(*p));
}
static unsigned
sys__file_perm(mode_t perm) {
  unsigned mod = 0;
  mod |= castu(!!(perm & S_IRUSR)) << 0u;
  mod |= castu(!!(perm & S_IWUSR)) << 1u;
  mod |= castu(!!(perm & S_IXUSR)) << 2u;
  mod |= castu(!!(perm & S_IRGRP)) << 3u;
  mod |= castu(!!(perm & S_IWGRP)) << 4u;
  mod |= castu(!!(perm & S_IXGRP)) << 5u;
  mod |= castu(!!(perm & S_IROTH)) << 6u;
  mod |= castu(!!(perm & S_IWOTH)) << 7u;
  mod |= castu(!!(perm & S_IXOTH)) << 8u;
  return mod;
}
static int
sys_file_info(struct sys *s, struct sys_file_info *info, struct str path) {
  assert(s);
  unused(s);
  assert(info);

  struct stat stats;
  struct sys_file_path fp;
  if (!sys__file_path_push(&fp, path)) {
    return 0;
  }
  int res = stat(fp.ospath, &stats);
  sys__file_path_pop(&fp);
  if (res < 0) {
    return 0;
  }
  info->siz = castsz(stats.st_size);
  info->mtime = stats.st_mtime;
  info->perm = sys__file_perm(stats.st_mode);

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
 *                                  Directory
 * ---------------------------------------------------------------------------
 */
static int
sys_dir_exists(struct sys *s, struct str path) {
  assert(s);
  struct sys_file_path fp;
  if (!sys__file_path_push(&fp, path)) {
    return 0;
  }
  struct stat stats;
  int res = stat(fp.ospath, &stats);
  sys__file_path_pop(&fp);
  if (res < 0 || !S_ISDIR(stats.st_mode)) {
    return 0;
  }
  return 1;
}
static void
sys__dir_free(struct sys *s, struct sys_dir_iter *it) {
  assert(s);
  assert(it);
  if (!it->valid) {
    return;
  }
  it->valid = 0;
  it->err = 0;
  closedir(it->handle);
}
static int
sys__dir_excl(struct sys_dir_iter *it) {
  int is_base = !str_cmp(it->name, strv("."));
  int is_prev = !str_cmp(it->name, strv(".."));
  return it->valid && (is_base || is_prev);
}
static void
sys_dir_nxt(struct sys *s, struct sys_dir_iter *it) {
  assert(s);
  assert(it);
  if (!it->valid) {
    return;
  }
  do {
    struct dirent *ent = readdir(it->handle);
    if (!ent) {
      sys__dir_free(s, it);
      return;
    }
    it->name = str0(ent->d_name);
    it->isdir = ent->d_type & DT_DIR;
  } while (sys__dir_excl(it));
}
static void
sys_dir_lst(struct sys *s, struct sys_dir_iter *it, struct str path) {
  assert(s);
  assert(it);
  struct sys_file_path fp;
  if (!sys__file_path_push(&fp, path)) {
    it->valid = 0;
    it->err = 1;
    return;
  }
  mset(it, 0, szof(*it));
  DIR *dir = opendir(fp.ospath);
  sys__file_path_pop(&fp);
  if (!dir) {
    it->valid = 0;
    it->err = 1;
    return;
  }
  it->handle = dir;
  it->valid = 1;
  sys_dir_nxt(s, it);
}

/* ---------------------------------------------------------------------------
 *                                  Log
 * ---------------------------------------------------------------------------
 */
static void
sys__mac_log(const char *fmt, ...) {
  char buf[2*1024];
  va_list args;
  va_start(args, fmt);
  fmtvsn(buf, szof(buf), fmt, args);
  va_end(args);
  fprintf(stdout, "%s", buf);
}
static void
sys__mac_warn(const char *fmt, ...) {
  char buf[2*1024];
  va_list args;
  va_start(args, fmt);
  fmtvsn(buf, szof(buf), fmt, args);
  va_end(args);

  fprintf(stdout, "\033[33m");
  fprintf(stdout, "%s", buf);
  fprintf(stdout, "\033[0m");
}
static void
sys__mac_err(const char *fmt, ...) {
  char buf[2*1024];
  va_list args;
  va_start(args, fmt);
  fmtvsn(buf, szof(buf), fmt, args);
  va_end(args);

  fprintf(stdout, "\033[31m");
  fprintf(stdout, "%s", buf);
  fprintf(stdout, "\033[0m");
}

/* ---------------------------------------------------------------------------
 *                            Delegates
 * ---------------------------------------------------------------------------
 */
static void
sys_mac__resize(struct sys *s) {
  assert(s->platform);
  struct sys_mac *os = cast(struct sys_mac*, s->platform);

  NSRect bounds = [os->view bounds];
  int w = max(1, casti(bounds.size.width));
  int h = max(1, casti(bounds.size.height));
  if (os->win_w == w && os->win_h == h) {
    return;
  }
  int fw = math_roundi(castf(w) * s->dpi_scale);
  int fh = math_roundi(castf(h) * s->dpi_scale);

  os->win_w = fw;
  os->win_h = fh;
  s->gfx.resize(s, fw, fh);
}
@implementation sys__mac_window_delegate {
  struct sys *sys;
}
-(instancetype)initWithSys: (struct sys*)sys_ {
   sys = sys_;
   return self;
}
- (BOOL)windowShouldClose:(id)sender {
  return YES;
}
- (void)windowDidResize:(NSNotification*)notification {
  sys_mac__resize(sys);
}
- (void)windowDidMiniaturize:(NSNotification*)notification {}
- (void)windowDidDeminiaturize:(NSNotification*)notification {}
- (void)windowDidEnterFullScreen:(NSNotification*)notification {}
- (void)windowDidExitFullScreen:(NSNotification*)notification {}
- (void)windowWillClose:(NSNotification *)notification {
  sys->running = 0;
  sys->quit = 1;
}
@end

@implementation sys__mac_view
{
  NSTimer *nstimer;
}
- (BOOL)acceptsFirstResponder { return YES; }
- (void)keyDown:(NSEvent *)theEvent {}
- (void) interruptMainLoop {
  assert(_sys->platform);
  struct sys_mac *os = cast(struct sys_mac*, _sys->platform);
  swapcontext(&os->run_loop_fiber, &os->main_fiber);
}
- (void)viewWillStartLiveResize {
  assert(!nstimer);
  nstimer = [NSTimer timerWithTimeInterval: 0.001
            target: self
            selector: @selector(interruptMainLoop)
            userInfo: nil
            repeats: YES];
  [[NSRunLoop currentRunLoop] addTimer: nstimer forMode: NSRunLoopCommonModes];
  sys_mac_prep(_sys);
}
- (void)viewDidEndLiveResize {
  [nstimer invalidate];
  nstimer = nil;
}
@end

@implementation sys__mac_view_delegate {}
- (void)drawInMTKView:(nonnull MTKView *) view {
  _sys->drw = 1;
  NSEvent* event = [NSEvent otherEventWithType: NSApplicationDefined
    location: NSMakePoint(0,0) modifierFlags: 0 timestamp: 0.0
    windowNumber: 0 context: nil subtype: 0 data1: 0 data2: 0];
  [NSApp postEvent: event atStart: YES];
}
- (void) mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
  unused(view);
  unused(size);
}
@end

/* ---------------------------------------------------------------------------
 *                            Input
 * ---------------------------------------------------------------------------
 */
enum sys_mac_constants {
#if defined(__MAC_10_12) && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_12
  SYS_EVT_FLAGS_CHANGED = NSEventTypeFlagsChanged,
  SYS_EVT_KEY_DOWN = NSEventTypeKeyDown,
  SYS_EVT_KEY_UP = NSEventTypeKeyUp,
  SYS_EVT_LEFT_MOUSE_DOWN = NSEventTypeLeftMouseDown,
  SYS_EVT_LEFT_MOUSE_DRAG = NSEventTypeLeftMouseDragged,
  SYS_EVT_LEFT_MOUSE_UP = NSEventTypeLeftMouseUp,
  SYS_EVT_MOUSE_MOVED = NSEventTypeMouseMoved,
  SYS_EVT_RIGHT_MOUSE_DOWN = NSEventTypeRightMouseDown,
  SYS_EVT_RIGHT_MOUSE_DRAG = NSEventTypeRightMouseDragged,
  SYS_EVT_RIGHT_MOUSE_UP = NSEventTypeRightMouseUp,
  SYS_EVT_SCROLL = NSEventTypeScrollWheel,
#else
  SYS_EVT_FLAGS_CHANGED = NSFlagsChanged,
  SYS_EVT_KEY_DOWN = NSKeyDown,
  SYS_EVT_KEY_UP = NSKeyUp,
  SYS_EVT_LEFT_MOUSE_DOWN = NSLeftMouseDown,
  SYS_EVT_LEFT_MOUSE_DRAG = NSLeftMouseDragged,
  SYS_EVT_LEFT_MOUSE_UP = NSLeftMouseUp,
  SYS_EVT_MOUSE_MOVED = NSMouseMoved,
  SYS_EVT_RIGHT_MOUSE_DOWN = NSRightMouseDown,
  SYS_EVT_RIGHT_MOUSE_DRAG = NSRightMouseDragged,
  SYS_EVT_RIGHT_MOUSE_UP = NSRightMouseUp,
  SYS_EVT_SCROLL = NSScrollWheel,
#endif
};
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
sys__mac_mods(const NSEvent *const ev) {
  unsigned res = 0u;
  const NSEventModifierFlags flg = ev.modifierFlags;
  if (flg & NSEventModifierFlagControl) {
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
sys_mac__mouse_pos(struct sys *s, const NSEvent *const e) {
  assert(s->platform);
  NSPoint pos = e.locationInWindow;
  float new_x = castf(pos.x) * s->dpi_scale;
  float new_y = castf(pos.y) * s->dpi_scale;

  s->mouse.pos[0] = casti(new_x);
  s->mouse.pos[1] = s->win.h - casti(new_y) - 1;

  s->mouse.pos_delta[0] = s->mouse.pos[0] - s->mouse.pos_last[0];
  s->mouse.pos_delta[1] = s->mouse.pos[1] - s->mouse.pos_last[1];
}
static void
sys__mac_on_key(unsigned long *keys, int scan) {
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
  case 0x4E: bit_set(keys, '-'); break;}
}
static int
sys_mac_evt(struct sys *s, NSEvent const *const e) {
  int hdl = 1;
  switch ([e type]) {
  default: hdl = 0; break;
  case SYS_EVT_KEY_UP: break;
  case SYS_EVT_KEY_DOWN: {
    s->keymod |= sys__mac_mods(e);
    sys__mac_on_key(s->keys, e.keyCode);
    s->key_mod = 1;

    const NSString* chars = e.characters;
    const NSUInteger len = chars.length;
    if (len > 0) {
      for (NSUInteger i = 0; i < len; i++) {
        const unichar cp = [chars characterAtIndex:i];
        if ((cp & 0xFF00) == 0xF700) {
          continue;
        }
        char buf[UTF_SIZ+1];
        int n = utf_enc(buf, cntof(buf), cp);
        if (s->txt_len + n < cntof(s->txt)) {
          mcpy(s->txt + s->txt_len, buf, n);
          s->txt_len += n;
        }
      }
      s->txt_mod = 1;
    }
  } break;
  case SYS_EVT_LEFT_MOUSE_DOWN: {
    sys__mac_on_btn(&s->mouse.btn.left, 1);
    if (e.clickCount == 2) {
      s->mouse.btn.left.doubled = 1;
    }
    s->btn_mod = 1;
    s->mouse_mod = 1;
    s->keymod |= sys__mac_mods(e);
  } break;
  case SYS_EVT_LEFT_MOUSE_DRAG: {
    sys_mac__mouse_pos(s,e);
    if (abs(s->mouse.pos_delta[0]) > 0 ||
        abs(s->mouse.pos_delta[1]) > 0) {
      s->btn_mod = 1;
    }
  } break;
  case SYS_EVT_LEFT_MOUSE_UP: {
    sys__mac_on_btn(&s->mouse.btn.left, 0);
    s->keymod |= sys__mac_mods(e);
    s->mouse_mod = 1;
    s->btn_mod = 1;
  } break;
  case SYS_EVT_MOUSE_MOVED: {
    sys_mac__mouse_pos(s,e);
    s->keymod |= sys__mac_mods(e);
    s->mouse_mod = 1;
  } break;
  case SYS_EVT_RIGHT_MOUSE_DOWN: {
    sys__mac_on_btn(&s->mouse.btn.right, 0);
    s->keymod |= sys__mac_mods(e);
    s->mouse_mod = 1;
    s->btn_mod = 1;
  } break;
  case SYS_EVT_RIGHT_MOUSE_DRAG: {
    sys_mac__mouse_pos(s,e);
    if (abs(s->mouse.pos_delta[0]) > 0 ||
        abs(s->mouse.pos_delta[1]) > 0) {
      s->btn_mod = 1;
    }
  } break;
  case SYS_EVT_RIGHT_MOUSE_UP: {
    sys__mac_on_btn(&s->mouse.btn.right, 0);
    s->keymod |= sys__mac_mods(e);
    s->mouse_mod = 1;
    s->btn_mod = 1;
  } break;
  case SYS_EVT_SCROLL: {
    float dx = castf(e.scrollingDeltaX);
    float dy = castf(e.scrollingDeltaY);
    if ((fabs(dx) >= 1.0f) || (fabs(dy) >= 1.0f)) {
      s->keymod |= sys__mac_mods(e);
      s->mouse.scrl[0] = casti(dx);
      s->mouse.scrl[1] = casti(dy);
      s->scrl_mod = 1;
    }
  } break;}
  return hdl;
}
static void
sys_mac_evt_loop(struct sys *s) {
  assert(s->platform);
  int evt_cnt = 0;
  int evt_cap = 128;
  @autoreleasepool {
    NSEvent *e = nil;
    NSEvent *evt_buf[evt_cap];
    NSEvent **evts = evt_buf;
    while ((e = [NSApp nextEventMatchingMask:NSEventMaskAny
      untilDate:NSDate.now inMode:NSDefaultRunLoopMode dequeue:YES])) {
      if (evt_cnt == evt_cap) {
        evt_cap *= 2;
        int was_on_stack = (evts == evt_buf);
        evts = realloc(was_on_stack ? 0: evts, castsz(evt_cap) * sizeof(*evts));
        if (was_on_stack) {
          mcpy(evts, evt_buf, szof(evt_buf));
        }
      }
      evts[evt_cnt++] = e;
    }
    for (int i = 0; i < evt_cnt; ++i) {
      e = evts[i];
      sys_mac_evt(s, e);
      [NSApp sendEvent:e];
      [NSApp updateWindows];
    }
    if (evts != evt_buf) {
        free(evts);
        evts = 0;
    }
  }
}

/* ---------------------------------------------------------------------------
 *                            Entry Points
 * ---------------------------------------------------------------------------
 */
static void
sys_mac_run_loop_fiber(unsigned lo, unsigned hi) {
  struct sys *s = recast(struct sys*, castull(hi) << 32llu | lo);
  assert(s->platform);
  struct sys_mac *os = cast(struct sys_mac*, s->platform);
  for (;;) {
    sys_mac_evt_loop(s);
    swapcontext(&os->run_loop_fiber, &os->main_fiber);
  }
}
static int
sys_mac_init(struct sys *s) {
  struct sys_mac *os = calloc(sizeof(struct sys_mac), 1);
  s->platform = os;
  s->running = 1;
  cpu_info(&s->cpu);

  /* memory */
  s->mem.page_siz = sysconf(_SC_PAGE_SIZE);
  s->mem.phy_siz = sysconf(_SC_PHYS_PAGES) * s->mem.page_siz;

  /* api */
  s->dir.lst = sys_dir_lst;
  s->dir.nxt = sys_dir_nxt;
  s->dir.exists = sys_dir_exists;
  s->time.timestamp = sys_mac_timestamp;
  s->file.info = sys_file_info;
  s->con.log = sys__mac_log;
  s->con.warn = sys__mac_warn;
  s->con.err = sys__mac_err;
  s->rnd.open = sys__rnd_open;
  s->rnd.close = sys__rnd_close;
  s->rnd.gen32 = sys__rnd_gen32;
  s->rnd.gen64 = sys__rnd_gen64;
  s->rnd.gen128 = sys__rnd_gen128;

  /* constants */
  os->dpi_scale[0] = 1.0f;
  os->dpi_scale[1] = 1.0f;

  /* application: act as a bundled app, even if not bundled */
  [NSApplication sharedApplication];
  [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];

  /* style */
  s->has_style = 1;
  s->style_mod = 1;
  s->col[SYS_COL_HOV] = sys__mac_col([NSColor controlBackgroundColor]);
  s->col[SYS_COL_WIN] = sys__mac_col([NSColor windowBackgroundColor]);
  s->col[SYS_COL_BG] = sys__mac_col([NSColor controlBackgroundColor]);
  s->col[SYS_COL_CTRL] = sys__mac_col([NSColor controlColor]);
  s->col[SYS_COL_SEL] = sys__mac_col([NSColor selectedControlColor]);
  s->col[SYS_COL_TXT] = sys__mac_col([NSColor controlTextColor]);
  s->col[SYS_COL_TXT_SEL] = sys__mac_col([NSColor selectedControlTextColor]);
  s->col[SYS_COL_TXT_DISABLED] = sys__mac_col([NSColor tertiaryLabelColor]);
  s->col[SYS_COL_ICO] = sys__mac_col([NSColor controlTextColor]);
  s->col[SYS_COL_LIGHT] = sys__mac_col([NSColor unemphasizedSelectedContentBackgroundColor]);
  s->col[SYS_COL_SHADOW] = sys__mac_col([NSColor underPageBackgroundColor]);
  s->fnt_pnt_size = castf([NSFont systemFontSize]);

  /* create window */
  const NSUInteger style =
    NSWindowStyleMaskTitled |
    NSWindowStyleMaskClosable |
    NSWindowStyleMaskMiniaturizable |
    NSWindowStyleMaskResizable;
  NSRect win_rect = NSMakeRect(0, 0, s->win.w, s->win.h);
  os->win = [[NSWindow alloc] initWithContentRect:win_rect styleMask:style
                              backing:NSBackingStoreBuffered defer:NO];
  os->win.releasedWhenClosed = NO;
  os->win.title = [NSString stringWithUTF8String:s->win.title];
  os->win.restorable = YES;
  os->win_dlg = [[sys__mac_window_delegate alloc] initWithSys:s];
  os->win.delegate = os->win_dlg;
  [os->win setOpaque: YES];
  os->win_w = s->win.w;
  os->win_h = s->win.h;

  /* setup metal view */
  os->view = [[sys__mac_view alloc] initWithFrame:win_rect];
  os->view_dlg = [[sys__mac_view_delegate alloc] init];
  os->view.delegate = os->view_dlg;
  os->view.enableSetNeedsDisplay = YES;
  os->view.autoresizingMask = NSViewWidthSizable|NSViewHeightSizable;
  os->view.paused = YES;
  os->view_dlg.sys = s;
  os->view.sys = s;

  /* init gfx */
  gfx_api(&s->gfx, 0);
  s->ren = &os->mtl;
  s->gfx.init(s, (__bridge void*)os->view);
  os->view.device = os->mtl.dev;

  [os->win setContentView:os->view];
  [os->win center];

  NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
  [os->win makeKeyAndOrderFront:nil];
  [os->win makeMainWindow];
  [NSApp activateIgnoringOtherApps:YES];
  [NSApp finishLaunching];

  s->dpi_scale = castf([os->win screen].backingScaleFactor);

  /* init fibers */
  getcontext(&os->run_loop_fiber);
  os->run_loop_fiber.uc_stack.ss_size = 512*1024;
  os->run_loop_fiber_stack = calloc(1, os->run_loop_fiber.uc_stack.ss_size);
  os->run_loop_fiber.uc_stack.ss_sp = os->run_loop_fiber_stack;
  os->run_loop_fiber.uc_link = 0;
  unsigned s_lo = (cast(uintptr_t,s) & 0xffffffffu);
  unsigned s_hi = (cast(uintptr_t,s) >> 32u) & 0xffffffffu;
  makecontext(&os->run_loop_fiber, sys_mac_run_loop_fiber, 2, s_lo, s_hi);
  if (getcontext(&os->main_fiber) != 0) {
    xpanic("failed to get fiber context!\n");
  }
  return 0;
}
static void
sys_mac_prep(struct sys *s) {
  s->focus = 0;
  s->keymod = 0;
  s->txt_len = 0;
  s->btn_mod = 0;
  s->txt_mod = 0;
  s->key_mod = 0;
  s->scrl_mod = 0;
  s->mouse_mod = 0;
  s->style_mod = 0;
  s->mouse.pos_last[0] = s->mouse.pos[0];
  s->mouse.pos_last[1] = s->mouse.pos[1];
  for arr_loopv(i, s->keys){
    s->keys[i] = 0;
  }
  for (int i = 0; i < SYS_MOUSE_BTN_CNT; ++i) {
    s->mouse.btns[i].pressed = 0;
    s->mouse.btns[i].released = 0;
    s->mouse.btns[i].doubled = 0;
  }
}
static int
sys_mac_pull(struct sys *s) {
  assert(s);
  assert(s->platform);
  struct sys_mac *os = cast(struct sys_mac*, s->platform);

  s->seq++;
  sys_mac_prep(s);
  [NSApp nextEventMatchingMask:NSEventMaskAny
         untilDate:NSDate.distantFuture
         inMode:NSDefaultRunLoopMode
         dequeue:NO];

  swapcontext(&os->main_fiber, &os->run_loop_fiber);
  s->txt_mod = !!s->txt_len;
  s->dpi_scale = castf([os->win screen].backingScaleFactor);

  const NSRect bounds = [os->view bounds];
  s->win.w = math_roundi(castf(bounds.size.width) * s->dpi_scale);
  s->win.h = math_roundi(castf(bounds.size.height) * s->dpi_scale);
  s->win.x = casti(bounds.origin.x * s->dpi_scale);
  s->win.y = casti(bounds.origin.y * s->dpi_scale);
  if (s->drw) {
    s->gfx.begin(s, s->win.w, s->win.h);
  }
  return 0;
}
static void
sys_mac_push(struct sys *s) {
  assert(s);
  assert(s->platform);
  struct sys_mac *os = cast(struct sys_mac*, s->platform);
  if (s->cursor != os->cursor) {
    switch (s->cursor) {
    case SYS_CUR_CNT: default: break;
    case SYS_CUR_ARROW: [[NSCursor arrowCursor] set]; break;
    case SYS_CUR_NO: [[NSCursor operationNotAllowedCursor] set]; break;
    case SYS_CUR_CROSS: [[NSCursor crosshairCursor] set]; break;
    case SYS_CUR_HAND: [[NSCursor pointingHandCursor] set]; break;
    case SYS_CUR_IBEAM: [[NSCursor IBeamCursor] set]; break;
    case SYS_CUR_MOVE: [[NSCursor closedHandCursor] set]; break;
    case SYS_CUR_SIZE_NS: [[NSCursor resizeUpDownCursor] set]; break;
    case SYS_CUR_SIZE_WE: [[NSCursor resizeLeftRightCursor] set]; break;}
    os->cursor = s->cursor;
  }
  unsigned long long tooltip_id = str_hash(s->tooltip.str);
  if (tooltip_id != os->tooltip) {
    if (str_len(s->tooltip.str)) {
      NSString *str = [[NSString alloc]
        initWithBytes: (const void*)str_beg(s->tooltip.str)
        length:(NSUInteger)str_len(s->tooltip.str) encoding:NSUTF8StringEncoding];
      [os->view setToolTip: str];
    } else {
      [os->view setToolTip: nil];
    }
    os->tooltip = tooltip_id;
  }
  s->tooltip.str = str_nil;
  if(s->repaint) {
    [os->view setNeedsDisplay:YES];
  }
  if (s->drw) {
    s->gfx.end(s, (__bridge void*)os->view);
  }
  s->repaint = 0;
  s->drw = 0;
}
static void
sys_mac_shutdown(struct sys *s) {
  assert(s);
  assert(s->platform);
  struct sys_mac *os = cast(struct sys_mac*, s->platform);

  free(os->run_loop_fiber_stack);
  os->run_loop_fiber_stack = 0;

  SYS__OBJ_REL(os->win_dlg);
  SYS__OBJ_REL(os->view);
  SYS__OBJ_REL(os->win);

  s->gfx.shutdown(s);
  free(s->platform);
  s->platform = 0;
  s->ren = 0;
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
static const struct sys_api sys_mac_api = {
  .version = SYS_VERSION,
  .init = sys_mac_init,
  .shutdown = sys_mac_shutdown,
  .pull = sys_mac_pull,
  .push = sys_mac_push,
};
extern void
sys_api(void *export, void *import) {
  unused(import);
  struct sys_api *api = (struct sys_api*)export;
  *api = sys_mac_api;
}

