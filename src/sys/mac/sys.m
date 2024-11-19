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
#include "../../app.h"

@interface sys__mac_app_delegate : NSObject<NSApplicationDelegate>
@end
@interface sys__mac_window : NSWindow
@end
@interface sys__mac_window_delegate : NSObject<NSWindowDelegate>
@end
@interface sys__mac_view : MTKView
@end
@interface sys__mac_view_delegate : NSViewController<MTKViewDelegate>
@end

struct sys_mac {
  int quit;
  int col_mod;

  struct arena mem;
  struct arena tmp;
  enum sys_cur_style cursor;
  struct str exe_path;
  struct gfx_mtl mtl;

  NSWindow* win;
  NSTrackingArea *track_area;
  sys__mac_app_delegate *app_dlg;
  sys__mac_window_delegate *win_dlg;
  sys__mac_view *view;
  sys__mac_view_delegate *view_dlg;
  unsigned long long tooltip;

  float dpi_scale[2];
  int win_w, win_h;
};
static struct sys_mac _mac;
static struct sys _sys;

static void sys_mac_prep(struct sys *s);

/* ---------------------------------------------------------------------------
 *                                Util
 * ---------------------------------------------------------------------------
 */
#if __has_feature(objc_arc)
#define SYS__OBJ_REL(obj) do{ obj = nil; } while(0)
#else
#define SYS__OBJ_REL(obj) do{ [obj release]; obj = nil; } while(0)
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
}
static void
sys__mac_on_frame(void) {
  _sys.seq++;
  _sys.txt_mod = !!_sys.txt_len;
  _sys.dpi_scale = castf([_mac.win screen].backingScaleFactor);

  const NSRect bounds = [_mac.view bounds];
  _sys.win.w = math_roundi(castf(bounds.size.width) * _sys.dpi_scale);
  _sys.win.h = math_roundi(castf(bounds.size.height) * _sys.dpi_scale);
  _sys.win.x = casti(bounds.origin.x * _sys.dpi_scale);
  _sys.win.y = casti(bounds.origin.y * _sys.dpi_scale);
  if (_sys.drw) {
    _sys.gfx.begin(&_sys, _sys.win.w, _sys.win.h);
  }
  app_run(&_sys);

  if (_sys.cursor != _mac.cursor) {
    switch (_sys.cursor) {
    case SYS_CUR_CNT: default: break;
    case SYS_CUR_ARROW: [[NSCursor arrowCursor] set]; break;
    case SYS_CUR_NO: [[NSCursor operationNotAllowedCursor] set]; break;
    case SYS_CUR_CROSS: [[NSCursor crosshairCursor] set]; break;
    case SYS_CUR_HAND: [[NSCursor pointingHandCursor] set]; break;
    case SYS_CUR_IBEAM: [[NSCursor IBeamCursor] set]; break;
    case SYS_CUR_MOVE: [[NSCursor closedHandCursor] set]; break;
    case SYS_CUR_SIZE_NS: [[NSCursor resizeUpDownCursor] set]; break;
    case SYS_CUR_SIZE_WE: [[NSCursor resizeLeftRightCursor] set]; break;}
    _mac.cursor = _sys.cursor;
  }
  unsigned long long tooltip_id = str_hash(_sys.tooltip.str);
  if (tooltip_id != _mac.tooltip) {
    if (str_len(_sys.tooltip.str)) {
      NSString *str = [[NSString alloc]
        initWithBytes: (const void*)str_beg(_sys.tooltip.str)
        length:(NSUInteger)str_len(_sys.tooltip.str) encoding:NSUTF8StringEncoding];
      [_mac.view setToolTip: str];
    } else {
      [_mac.view setToolTip: nil];
    }
    _mac.tooltip = tooltip_id;
  }
  _sys.tooltip.str = str_nil;
  if(_sys.repaint) {
    [_mac.view setNeedsDisplay:YES];
  }
  if (_sys.drw) {
    _sys.gfx.end(&_sys, (__bridge void*)_mac.view);
  }
  _sys.drw = 0;
  _sys.repaint = 0;
  _sys.focus = 0;
  _sys.keymod = 0;
  _sys.txt_len = 0;
  _sys.btn_mod = 0;
  _sys.txt_mod = 0;
  _sys.key_mod = 0;
  _sys.scrl_mod = 0;
  _sys.mouse_mod = 0;
  _sys.style_mod = 0;
  _sys.mouse.pos_last[0] = _sys.mouse.pos[0];
  _sys.mouse.pos_last[1] = _sys.mouse.pos[1];
  for arr_loopv(i, _sys.keys){
    _sys.keys[i] = 0;
  }
  for (int i = 0; i < SYS_MOUSE_BTN_CNT; ++i) {
    _sys.mouse.btns[i].pressed = 0;
    _sys.mouse.btns[i].released = 0;
    _sys.mouse.btns[i].doubled = 0;
  }
}
static void
sys_mac__resize(void) {
  NSRect bounds = [_mac.view bounds];
  int w = max(1, casti(bounds.size.width));
  int h = max(1, casti(bounds.size.height));
  if (_mac.win_w == w && _mac.win_h == h) {
    return;
  }
  int fw = math_roundi(castf(w) * _sys.dpi_scale);
  int fh = math_roundi(castf(h) * _sys.dpi_scale);

  _mac.win_w = fw;
  _mac.win_h = fh;
  _sys.gfx.resize(&_sys, fw, fh);

  sys__mac_on_frame();
  [_mac.view setNeedsDisplay:YES];
}
static void
sys__mac_on_btn(struct sys_btn *b, int down) {
  assert(b);
  int was_down = b->down;
  b->down = !!down;
  b->pressed = !was_down && down;
  b->released = was_down && !down;
  b->doubled = 0;
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
sys_mac__mouse_pos(const NSEvent *const e) {
  assert(_sys.platform);
  NSPoint pos = e.locationInWindow;
  float new_x = castf(pos.x) * _sys.dpi_scale;
  float new_y = castf(pos.y) * _sys.dpi_scale;

  _sys.mouse.pos[0] = casti(new_x);
  _sys.mouse.pos[1] = _sys.win.h - casti(new_y) - 1;

  _sys.mouse.pos_delta[0] = _sys.mouse.pos[0] - _sys.mouse.pos_last[0];
  _sys.mouse.pos_delta[1] = _sys.mouse.pos[1] - _sys.mouse.pos_last[1];
}

@implementation sys__mac_app_delegate
- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  /* style */
  _sys.has_style = 1;
  _sys.style_mod = 1;
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
  _sys.fnt_pnt_size = castf([NSFont systemFontSize]);

  _sys.op = SYS_SETUP;
  app_run(&_sys);
  _sys.op = SYS_RUN;

  /* create window */
  const NSUInteger style =
    NSWindowStyleMaskTitled |
    NSWindowStyleMaskClosable |
    NSWindowStyleMaskMiniaturizable |
    NSWindowStyleMaskResizable;
  NSRect win_rect = NSMakeRect(0, 0, _sys.win.w, _sys.win.h);
  _mac.win = [[NSWindow alloc] initWithContentRect:win_rect styleMask:style
                              backing:NSBackingStoreBuffered defer:NO];

  _mac.win.releasedWhenClosed = NO;
  _mac.win.title = [NSString stringWithUTF8String:_sys.win.title];
  _mac.win.restorable = YES;
  _mac.win_dlg = [[sys__mac_window_delegate alloc] init];
  _mac.win.delegate = _mac.win_dlg;
  [_mac.win setOpaque: YES];
  _mac.win_w = _sys.win.w;
  _mac.win_h = _sys.win.h;

  /* setup metal view */
  _mac.view = [[sys__mac_view alloc] initWithFrame:win_rect];
  _mac.view_dlg = [[sys__mac_view_delegate alloc] init];
  _mac.view.delegate = _mac.view_dlg;
  _mac.view.autoresizingMask = NSViewWidthSizable|NSViewHeightSizable;
  _mac.view.enableSetNeedsDisplay = YES;
  _mac.view.paused = YES;

  /* init gfx */
  gfx_api(&_sys.gfx, 0);
  _sys.ren = &_mac.mtl;
  _sys.gfx.init(&_sys, (__bridge void*)_mac.view);
  _mac.view.device = _mac.mtl.dev;

  [_mac.view updateTrackingAreas];
  [_mac.win setContentView:_mac.view];
  [_mac.win center];

  NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
  [NSApp activateIgnoringOtherApps:YES];
  [_mac.win makeKeyAndOrderFront:nil];
  [_mac.win makeMainWindow];
  [NSApp finishLaunching];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
  return YES;
}
- (BOOL)application:(NSApplication *)sender openFile:(NSString *)filename {
  return NO;
}
- (void)application:(NSApplication *)sender openFiles:(NSArray<NSString *> *)filenames {

}
- (void)applicationWillTerminate:(NSNotification*)notification {
  _sys.op = SYS_QUIT;
  sys__mac_on_frame();
  _sys.gfx.shutdown(&_sys);
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
- (void)windowDidMiniaturize:(NSNotification*)notification {}
- (void)windowDidDeminiaturize:(NSNotification*)notification {}
- (void)windowDidEnterFullScreen:(NSNotification*)notification {}
- (void)windowDidExitFullScreen:(NSNotification*)notification {}
- (void)windowWillClose:(NSNotification *)notification {
}
@end

@implementation sys__mac_view
- (void)reshape {
  sys_mac__resize();
}
- (void)windowDidExpose {
  sys_mac__resize();
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
    NSTrackingActiveInKeyWindow | NSTrackingEnabledDuringMouseDrag | NSTrackingActiveAlways |
    NSTrackingCursorUpdate | NSTrackingInVisibleRect | NSTrackingAssumeInside | NSTrackingMouseMoved;
  _mac.track_area = [[NSTrackingArea alloc] initWithRect:[self bounds]
                     options:options owner:self userInfo:nil];
  [self addTrackingArea:_mac.track_area];
  [super updateTrackingAreas];
}
- (void)mouseEntered:(NSEvent*)e {

}
- (void)mouseExited:(NSEvent*)e {

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
  sys_mac__mouse_pos(e);
  if (abs(_sys.mouse.pos_delta[0]) > 0 ||
      abs(_sys.mouse.pos_delta[1]) > 0) {
    _sys.btn_mod = 1;
    sys__mac_on_frame();
  }
}
- (void)rightMouseDragged:(NSEvent*)e {
  sys_mac__mouse_pos(e);
  if (abs(_sys.mouse.pos_delta[0]) > 0 ||
      abs(_sys.mouse.pos_delta[1]) > 0) {
    _sys.btn_mod = 1;
    sys__mac_on_frame();
  }
}
- (void)otherMouseDragged:(NSEvent*)e {
  sys_mac__mouse_pos(e);
  if (abs(_sys.mouse.pos_delta[0]) > 0 ||
      abs(_sys.mouse.pos_delta[1]) > 0) {
    _sys.btn_mod = 1;
    sys__mac_on_frame();
  }
}
- (void)scrollWheel:(NSEvent*)e {
  float dx = castf(e.scrollingDeltaX);
  float dy = castf(e.scrollingDeltaY);
  if (e.hasPreciseScrollingDeltas) {
    dx *= 0.1f, dy *= 0.1f;
  }
  if ((fabs(dx) >= 1.0f) || (fabs(dy) >= 1.0f)) {
    _sys.keymod |= sys__mac_mods(e);
    _sys.mouse.scrl[0] = casti(dx);
    _sys.mouse.scrl[1] = casti(dy);
    _sys.scrl_mod = 1;
  }
  sys__mac_on_frame();
}
- (void)keyDown:(NSEvent*)e {
  _sys.keymod |= sys__mac_mods(e);
  sys__mac_on_key(_sys.keys, e.keyCode);
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
        memcpy(_sys.txt + _sys.txt_len, buf, castsz(n));
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

@implementation sys__mac_view_delegate {}
- (void)drawInMTKView:(nonnull MTKView *) view {
  _sys.drw = 1;
  sys__mac_on_frame();
}
- (void) mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
  unused(view);
  unused(size);
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
      [self registerForDraggedTypes:[NSArray arrayWithObjects:NSPasteboardTypeFileURL,NSPasteboardTypeString,nil]];
    #endif
  }
  return self;
}
static BOOL
sys__mac_dnd_files(NSArray *files, enum sys_dnd_state state) {
  assert(files);
  BOOL ret = YES;
  int dnd_file_cnt = casti([files count]);
  for (int at = 0; dnd_file_cnt; ++at) {
    struct str lst[64];
    _sys.dnd.state = SYS_DND_DELIVERY;
    _sys.dnd.response = SYS_DND_REJECT;
    _sys.dnd.file_cnt = min(dnd_file_cnt, cntof(files));
    _sys.dnd.files = lst;

    for loop(i, _sys.dnd.file_cnt) {
      NSUInteger idx = cast(NSUInteger, at);
      NSURL *fileUrl = [NSURL fileURLWithPath:[[files objectAtIndex:idx] stringForType:NSPasteboardTypeFileURL]];
      lst[i] = str0(fileUrl.standardizedURL.path.UTF8String);
    }
    _sys.dnd_mod = 1;
    _sys.dnd.state = state;
    sys__mac_on_frame();
    if (_sys.dnd.response == SYS_DND_REJECT) {
      ret = NO;
      break;
    }
    dnd_file_cnt -= _sys.dnd.file_cnt;
  }
  _sys.dnd.state = SYS_DND_NONE;
  _sys.dnd.response = SYS_DND_REJECT;
  _sys.dnd.files = 0;
  _sys.dnd_mod = 0;
  return ret;
}
static BOOL
sys__mac_dnd_str(NSArray *strs, enum sys_dnd_state state) {
  BOOL ret = NO;
  NSString *str = [strs objectAtIndex:0];
  NSData *utf8Data = [str dataUsingEncoding:NSUTF8StringEncoding];
  int len = cast(int, [utf8Data length]);

  _sys.dnd.str = str(cast(const char*, [utf8Data bytes]), {len});
  _sys.dnd.state = state;
  _sys.dnd_mod = 1;
  sys__mac_on_frame();
  ret = _sys.dnd.response == SYS_DND_ACCEPT ? YES : NO;

  _sys.dnd.state = SYS_DND_NONE;
  _sys.dnd.response = SYS_DND_REJECT;
  _sys.dnd.str = str_nil;
  _sys.dnd_mod = 0;
  return ret;
}
static BOOL
sys__mac_dnd(NSPasteboard *pboard, enum sys_dnd_state state) {
  BOOL ret = NO;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
  if ([pboard.types containsObject:NSPasteboardTypeFileURL]) {
    NSArray *files = pboard.pasteboardItems;
    ret = sys__mac_dnd_files(files, state);
  }
  if ([pboard.types containsObject:NSPasteboardTypeString]) {
    NSArray *strs = pboard.pasteboardItems;
    ret = sys__mac_dnd_str(strs, state);
  }
#endif
  return ret;
}
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
  NSPasteboard *pboard = [sender draggingPasteboard];
  BOOL ret = sys__mac_dnd(pboard, SYS_DND_ENTER);
  if (ret) {
    return NSDragOperationCopy;
  } else {
    return NSDragOperationNone;
  }
#else
  return NSDragOperationCopy;
#endif
}
- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
  NSPoint pos = [_mac.win mouseLocationOutsideOfEventStream];
  float new_x = castf(pos.x) * _mac.dpi_scale[0];
  float new_y = castf(pos.y) * _mac.dpi_scale[1];

  _sys.mouse.pos[0] = casti(new_x);
  _sys.mouse.pos[1] = _sys.win.h - casti(new_y) - 1;

  _sys.mouse.pos_delta[0] = _sys.mouse.pos[0] - _sys.mouse.pos_last[0];
  _sys.mouse.pos_delta[1] = _sys.mouse.pos[1] - _sys.mouse.pos_last[1];

  _sys.mouse_mod = 1;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
  NSPasteboard *pboard = [sender draggingPasteboard];
  BOOL ret = sys__mac_dnd(pboard, SYS_DND_PREVIEW);
  if (ret) {
    return NSDragOperationCopy;
  } else {
    return NSDragOperationNone;
  }
#else
  return NSDragOperationCopy;
#endif
}
- (NSDragOperation)draggingExited:(id<NSDraggingInfo>)sender {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
  NSPasteboard *pboard = [sender draggingPasteboard];
  BOOL ret = sys__mac_dnd(pboard, SYS_DND_LEFT);
  if (ret) {
    return NSDragOperationCopy;
  } else {
    return NSDragOperationNone;
  }
#else
  return NSDragOperationCopy;
#endif
}
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
  NSPasteboard *pboard = [sender draggingPasteboard];
  return sys__mac_dnd(pboard, SYS_DND_DELIVERY);
#else
  return NO;
#endif
}
@end

extern int
main(int argc, char *argv[]) {
  _sys.platform = &_mac;
  _sys.argc = argc;
  _sys.argv = argv;
  cpu_info(&_sys.cpu);

  /* memory */
  _sys.mem.page_siz = sysconf(_SC_PAGE_SIZE);
  _sys.mem.phy_siz = sysconf(_SC_PHYS_PAGES) * _sys.mem.page_siz;

  /* api */
  _sys.dir.lst = sys_dir_lst;
  _sys.dir.nxt = sys_dir_nxt;
  _sys.dir.exists = sys_dir_exists;
  _sys.file.info = sys_file_info;
  _sys.time.timestamp = sys_mac_timestamp;

  _sys.con.log = sys__mac_log;
  _sys.con.warn = sys__mac_warn;
  _sys.con.err = sys__mac_err;

  _sys.rnd.open = sys__rnd_open;
  _sys.rnd.close = sys__rnd_close;
  _sys.rnd.gen32 = sys__rnd_gen32;
  _sys.rnd.gen64 = sys__rnd_gen64;
  _sys.rnd.gen128 = sys__rnd_gen128;

  /* constants */
  _mac.dpi_scale[0] = 1.0f;
  _mac.dpi_scale[1] = 1.0f;

  /* application: act as a bundled app, even if not bundled */
  [NSApplication sharedApplication];
  _mac.app_dlg = [[sys__mac_app_delegate alloc] init];
  NSApp.delegate = _mac.app_dlg;
  [NSApp run];
  return 0;
}

