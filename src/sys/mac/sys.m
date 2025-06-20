#define _XOPEN_SOURCE

/* std */
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
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

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
  id keyup_monitor;

  float scrl[2];
  float dpi_scale[2];
  int win_w, win_h;
};
static struct sys_mac g_mac;
static struct sys g_sys;

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
sys__file_path_push(struct sys_file_path *sfp, struct str path) {
  assert(sfp);
  if (str_len(path) + 1 < MAX_FILE_PATH) {
    mcpy(sfp->buf, str_beg(path), str_len(path));
    sfp->buf[str_len(path)] = 0;
    sfp->ospath = sfp->buf;
  } else {
    sfp->ospath = calloc(1, castsz(str_len(path) + 1));
    if (!sfp->ospath) {
      return 0;
    }
    mcpy(sfp->ospath, str_beg(path), str_len(path));
    sfp->ospath[str_len(path)] = 0;
  }
  return 1;
}
static void
sys__file_path_pop(struct sys_file_path *sfp) {
  assert(sfp);
  if (sfp->ospath != sfp->buf) {
    free(sfp->ospath);
  }
  mset(sfp, 0, szof(*sfp));
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
sys_file_info(struct sys *sys, struct sys_file_info *info, struct str path) {
  assert(sys);
  unused(sys);
  assert(info);

  struct stat stats;
  struct sys_file_path sfp;
  if (!sys__file_path_push(&sfp, path)) {
    return 0;
  }
  int res = stat(sfp.ospath, &stats);
  sys__file_path_pop(&sfp);
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
sys_dir_exists(struct sys *sys, struct str path) {
  assert(sys);
  struct sys_file_path sfp;
  if (!sys__file_path_push(&sfp, path)) {
    return 0;
  }
  struct stat stats;
  int res = stat(sfp.ospath, &stats);
  sys__file_path_pop(&sfp);
  if (res < 0 || !S_ISDIR(stats.st_mode)) {
    return 0;
  }
  return 1;
}
static void
sys__dir_free(struct sys *sys, struct sys_dir_iter *itr) {
  assert(sys);
  assert(itr);
  if (!itr->valid) {
    return;
  }
  itr->valid = 0;
  itr->err = 0;
  closedir(itr->handle);
}
static int
sys__dir_excl(struct sys_dir_iter *itr) {
  int is_base = !str_cmp(itr->name, strv("."));
  int is_prev = !str_cmp(itr->name, strv(".."));
  return itr->valid && (is_base || is_prev);
}
static void
sys_dir_nxt(struct sys *sys, struct sys_dir_iter *itr) {
  assert(sys);
  assert(itr);
  if (!itr->valid) {
    return;
  }
  do {
    struct dirent *ent = readdir(itr->handle);
    if (!ent) {
      sys__dir_free(sys, itr);
      return;
    }
    itr->name = str0(ent->d_name);
    itr->isdir = ent->d_type & DT_DIR;
  } while (sys__dir_excl(itr));
}
static void
sys_dir_lst(struct sys *sys, struct sys_dir_iter *itr, struct str path) {
  assert(sys);
  assert(itr);
  struct sys_file_path sfp;
  if (!sys__file_path_push(&sfp, path)) {
    itr->valid = 0;
    itr->err = 1;
    return;
  }
  mset(itr, 0, szof(*itr));
  DIR *dir = opendir(sfp.ospath);
  sys__file_path_pop(&sfp);
  if (!dir) {
    itr->valid = 0;
    itr->err = 1;
    return;
  }
  itr->handle = dir;
  itr->valid = 1;
  sys_dir_nxt(sys, itr);
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
  if (g_mac.keyup_monitor != nil) {
    [NSEvent removeMonitor:g_mac.keyup_monitor];
    g_mac.keyup_monitor = nil;
  }
  SYS__OBJ_REL(g_mac.track_area);
  SYS__OBJ_REL(g_mac.app_dlg);
  SYS__OBJ_REL(g_mac.win_dlg);
  SYS__OBJ_REL(g_mac.win);
  SYS__OBJ_REL(g_mac.view);
}
static void
sys__mac_on_frame(void) {
  g_sys.seq++;
  g_sys.txt_mod = !!g_sys.txt_len;
  g_sys.dpi_scale = castf([g_mac.win screen].backingScaleFactor);

  const NSRect bounds = [g_mac.view bounds];
  g_sys.win.w = math_roundi(castf(bounds.size.width) * g_sys.dpi_scale);
  g_sys.win.h = math_roundi(castf(bounds.size.height) * g_sys.dpi_scale);
  g_sys.win.x = casti(bounds.origin.x * g_sys.dpi_scale);
  g_sys.win.y = casti(bounds.origin.y * g_sys.dpi_scale);
  if (g_sys.drw) {
    g_sys.gfx.begin(&g_sys, g_sys.win.w, g_sys.win.h);
  }
  app_run(&g_sys);

  if (g_sys.cursor != g_mac.cursor) {
    switch (g_sys.cursor) {
    case SYS_CUR_CNT: default: break;
    case SYS_CUR_ARROW: [[NSCursor arrowCursor] set]; break;
    case SYS_CUR_NO: [[NSCursor operationNotAllowedCursor] set]; break;
    case SYS_CUR_CROSS: [[NSCursor crosshairCursor] set]; break;
    case SYS_CUR_HAND: [[NSCursor pointingHandCursor] set]; break;
    case SYS_CUR_IBEAM: [[NSCursor IBeamCursor] set]; break;
    case SYS_CUR_MOVE: [[NSCursor closedHandCursor] set]; break;
    case SYS_CUR_SIZE_NS: [[NSCursor resizeUpDownCursor] set]; break;
    case SYS_CUR_SIZE_WE: [[NSCursor resizeLeftRightCursor] set]; break;}
    g_mac.cursor = g_sys.cursor;
  }
  unsigned long long tooltip_id = str_hash(g_sys.tooltip.str);
  if (tooltip_id != g_mac.tooltip) {
    if (str_len(g_sys.tooltip.str)) {
      NSString *str = [[NSString alloc]
        initWithBytes: (const void*)str_beg(g_sys.tooltip.str)
        length:(NSUInteger)str_len(g_sys.tooltip.str) encoding:NSUTF8StringEncoding];
      [g_mac.view setToolTip: str];
    } else {
      [g_mac.view setToolTip: nil];
    }
    g_mac.tooltip = tooltip_id;
  }
  g_sys.tooltip.str = str_nil;
  if(g_sys.repaint) {
    [g_mac.view setNeedsDisplay:YES];
  }
  if (g_sys.drw) {
    g_sys.gfx.end(&g_sys, (__bridge void*)g_mac.view);
  }
  g_sys.drw = 0;
  g_sys.repaint = 0;
  g_sys.focus = 0;
  g_sys.keymod = 0;
  g_sys.txt_len = 0;
  g_sys.btn_mod = 0;
  g_sys.txt_mod = 0;
  g_sys.key_mod = 0;
  g_sys.scrl_mod = 0;
  g_sys.mouse_mod = 0;
  g_sys.style_mod = 0;
  g_sys.mouse.pos_last[0] = g_sys.mouse.pos[0];
  g_sys.mouse.pos_last[1] = g_sys.mouse.pos[1];
  for arr_loopv(i, g_sys.keys){
    g_sys.keys[i] = 0;
  }
  for arr_loopv(i, g_sys.mouse.btns) {
    g_sys.mouse.btns[i].pressed = 0;
    g_sys.mouse.btns[i].released = 0;
    g_sys.mouse.btns[i].doubled = 0;
  }
}
static void
sys_mac__resize(void) {
  NSRect bounds = [g_mac.view bounds];
  int width = max(1, casti(bounds.size.width));
  int hight = max(1, casti(bounds.size.height));
  if (g_mac.win_w == width && g_mac.win_h == hight) {
    return;
  }
  int fbw = math_roundi(castf(width) * g_sys.dpi_scale);
  int fbh = math_roundi(castf(hight) * g_sys.dpi_scale);

  g_mac.win_w = fbw;
  g_mac.win_h = fbh;
  g_sys.gfx.resize(&g_sys, fbw, fbh);

  sys__mac_on_frame();
  [g_mac.view setNeedsDisplay:YES];
}
static void
sys__mac_on_btn(struct sys_btn *btn, int down) {
  assert(btn);
  int was_down = btn->down;
  btn->down = !!down;
  btn->pressed = !was_down && down;
  btn->released = was_down && !down;
  btn->doubled = 0;
}
static void
sys__mac_on_key(unsigned long *keys, int scan) {
  // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
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
  // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
}
static unsigned
sys__mac_mods(const NSEvent *const evt) {
  unsigned res = 0u;
  const NSEventModifierFlags flg = evt.modifierFlags;
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
sys_mac__mouse_pos(const NSEvent *const evt) {
  assert(g_sys.platform);
  NSPoint pos = evt.locationInWindow;
  float new_x = castf(pos.x) * g_sys.dpi_scale;
  float new_y = castf(pos.y) * g_sys.dpi_scale;

  g_sys.mouse.pos[0] = casti(new_x);
  g_sys.mouse.pos[1] = g_sys.win.h - casti(new_y) - 1;

  g_sys.mouse.pos_delta[0] = g_sys.mouse.pos[0] - g_sys.mouse.pos_last[0];
  g_sys.mouse.pos_delta[1] = g_sys.mouse.pos[1] - g_sys.mouse.pos_last[1];
}

@implementation sys__mac_app_delegate
- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  /* style */
  g_sys.has_style = 1;
  g_sys.style_mod = 1;
  g_sys.col[SYS_COL_HOV] = sys__mac_col([NSColor controlBackgroundColor]);
  g_sys.col[SYS_COL_WIN] = sys__mac_col([NSColor windowBackgroundColor]);
  g_sys.col[SYS_COL_BG] = sys__mac_col([NSColor controlBackgroundColor]);
  g_sys.col[SYS_COL_CTRL] = sys__mac_col([NSColor controlColor]);
  g_sys.col[SYS_COL_SEL] = sys__mac_col([NSColor selectedControlColor]);
  g_sys.col[SYS_COL_TXT] = sys__mac_col([NSColor controlTextColor]);
  g_sys.col[SYS_COL_TXT_SEL] = sys__mac_col([NSColor selectedControlTextColor]);
  g_sys.col[SYS_COL_TXT_DISABLED] = sys__mac_col([NSColor tertiaryLabelColor]);
  g_sys.col[SYS_COL_ICO] = sys__mac_col([NSColor controlTextColor]);
  g_sys.col[SYS_COL_LIGHT] = sys__mac_col([NSColor unemphasizedSelectedContentBackgroundColor]);
  g_sys.col[SYS_COL_SHADOW] = sys__mac_col([NSColor underPageBackgroundColor]);
  g_sys.fnt_pnt_size = castf([NSFont systemFontSize]);

  g_sys.op = SYS_SETUP;
  app_run(&g_sys);

  /* create window */
  const NSUInteger style =
    NSWindowStyleMaskTitled |
    NSWindowStyleMaskClosable |
    NSWindowStyleMaskMiniaturizable |
    NSWindowStyleMaskResizable;
  NSRect win_rect = NSMakeRect(0, 0, g_sys.win.w, g_sys.win.h);
  g_mac.win = [[NSWindow alloc] initWithContentRect:win_rect styleMask:style
    backing:NSBackingStoreBuffered defer:NO];

  g_mac.win.releasedWhenClosed = NO;
  g_mac.win.title = [NSString stringWithUTF8String:g_sys.win.title];
  g_mac.win.restorable = YES;
  g_mac.win_dlg = [[sys__mac_window_delegate alloc] init];
  g_mac.win.delegate = g_mac.win_dlg;
  g_mac.win.acceptsMouseMovedEvents = YES;
  [g_mac.win setOpaque: YES];
  g_mac.win_w = g_sys.win.w;
  g_mac.win_h = g_sys.win.h;

  /* setup metal view */
  g_mac.view = [[sys__mac_view alloc] initWithFrame:win_rect];
  g_mac.view_dlg = [[sys__mac_view_delegate alloc] init];
  g_mac.view.delegate = g_mac.view_dlg;
  g_mac.view.autoresizingMask = NSViewWidthSizable|NSViewHeightSizable;
  g_mac.view.enableSetNeedsDisplay = YES;
  g_mac.view.paused = YES;

  /* init gfx */
  gfx_api(&g_sys.gfx, 0);
  g_sys.ren = &g_mac.mtl;
  g_sys.gfx.init(&g_sys, (__bridge void*)g_mac.view);
  g_mac.view.device = g_mac.mtl.dev;

  [g_mac.view updateTrackingAreas];
  [g_mac.win setContentView:g_mac.view];
  [g_mac.win center];

  if (g_sys.win.max_w != 0 && g_sys.win.max_h != 0) {
    float max_w = castf(g_sys.win.max_w);
    float max_h = castf(g_sys.win.max_h);
    g_mac.win.contentMaxSize = NSMakeSize(max_w, max_h);
  }
  if (g_sys.win.min_w != 0 && g_sys.win.min_h != 0) {
    float min_w = castf(g_sys.win.min_w);
    float min_h = castf(g_sys.win.min_h);
    g_mac.win.contentMinSize = NSMakeSize(min_w, min_h);
  }
  if (g_sys.win.max_w != 0 && g_sys.win.max_h != 0) {
    float max_w = castf(g_sys.win.max_w);
    float max_h = castf(g_sys.win.max_h);
    g_mac.win.maxFullScreenContentSize = NSMakeSize(max_w, max_h);
  }
  g_sys.dpi_scale = castf([g_mac.win screen].backingScaleFactor);
  g_sys.op = SYS_INIT;
  app_run(&g_sys);
  g_sys.op = SYS_RUN;

  NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
  [NSApp activateIgnoringOtherApps:YES];
  [g_mac.win makeKeyAndOrderFront:nil];
  [g_mac.win makeMainWindow];

  [NSEvent setMouseCoalescingEnabled:NO];
  NSEvent *focusevent = [NSEvent otherEventWithType:NSEventTypeAppKitDefined
    location:NSZeroPoint
    modifierFlags:0x40
    timestamp:0
    windowNumber:0
    context:nil
    subtype:NSEventSubtypeApplicationActivated
    data1:0
    data2:0];
  [NSApp postEvent:focusevent atStart:YES];
  [NSApp finishLaunching];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
  unused(sender);
  return YES;
}
- (void)applicationWillTerminate:(NSNotification*)notification {
  g_sys.op = SYS_QUIT;
  sys__mac_on_frame();
  g_sys.gfx.shutdown(&g_sys);
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
  g_mac.col_mod = 1;
  [g_mac.view setNeedsDisplay:YES];
}
- (BOOL)isOpaque {return YES;}
- (BOOL)canBecomeKeyView {return YES;}
- (BOOL)acceptsFirstResponder {return YES;}
- (void)updateTrackingAreas {
  if (g_mac.track_area != nil) {
    [self removeTrackingArea:g_mac.track_area];
    SYS__OBJ_REL(g_mac.track_area);
  }
  const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
    NSTrackingActiveInKeyWindow | NSTrackingEnabledDuringMouseDrag |
    NSTrackingCursorUpdate | NSTrackingInVisibleRect | NSTrackingAssumeInside;
  g_mac.track_area = [[NSTrackingArea alloc] initWithRect:[self bounds]
                     options:options owner:self userInfo:nil];
  [self addTrackingArea:g_mac.track_area];
  [super updateTrackingAreas];
}
- (void)mouseEntered:(NSEvent*)e {

}
- (void)mouseExited:(NSEvent*)e {

}
- (void)mouseDown:(NSEvent*)evt {
  sys__mac_on_btn(&g_sys.mouse.btn.left, 1);
  if (evt.clickCount == 2) {
    g_sys.mouse.btn.left.doubled = 1;
  }
  g_sys.btn_mod = 1;
  g_sys.mouse_mod = 1;
  g_sys.keymod |= sys__mac_mods(evt);
  sys__mac_on_frame();
}
- (void)mouseUp:(NSEvent*)evt {
  sys__mac_on_btn(&g_sys.mouse.btn.left, 0);

  g_sys.btn_mod = 1;
  g_sys.mouse_mod = 1;
  g_sys.keymod |= sys__mac_mods(evt);
  sys__mac_on_frame();
}
- (void)rightMouseDown:(NSEvent*)evt {
  sys__mac_on_btn(&g_sys.mouse.btn.right, 1);

  g_sys.btn_mod = 1;
  g_sys.mouse_mod = 1;
  g_sys.keymod |= sys__mac_mods(evt);
  sys__mac_on_frame();
}
- (void)rightMouseUp:(NSEvent*)evt {
  sys__mac_on_btn(&g_sys.mouse.btn.right, 0);

  g_sys.btn_mod = 1;
  g_sys.mouse_mod = 1;
  g_sys.keymod |= sys__mac_mods(evt);
  sys__mac_on_frame();
}
- (void)otherMouseDown:(NSEvent*)evt {
  if (2 == evt.buttonNumber) {
    sys__mac_on_btn(&g_sys.mouse.btn.middle, 1);
    sys__mac_on_frame();
  }
}
- (void)otherMouseUp:(NSEvent*)evt {
  if (2 == evt.buttonNumber) {
    sys__mac_on_btn(&g_sys.mouse.btn.middle, 0);
    sys__mac_on_frame();
  }
}
- (void)mouseMoved:(NSEvent*)evt {
  sys_mac__mouse_pos(evt);
  g_sys.mouse_mod = 1;
  g_sys.keymod |= sys__mac_mods(evt);
  if (abs(g_sys.mouse.pos_delta[0]) > 0 ||
      abs(g_sys.mouse.pos_delta[1]) > 0) {
    sys__mac_on_frame();
  }
}
- (void)mouseDragged:(NSEvent*)evt {
  sys_mac__mouse_pos(evt);
  if (abs(g_sys.mouse.pos_delta[0]) > 0 ||
      abs(g_sys.mouse.pos_delta[1]) > 0) {
    g_sys.btn_mod = 1;
    sys__mac_on_frame();
  }
}
- (void)rightMouseDragged:(NSEvent*)evt {
  sys_mac__mouse_pos(evt);
  if (abs(g_sys.mouse.pos_delta[0]) > 0 ||
      abs(g_sys.mouse.pos_delta[1]) > 0) {
    g_sys.btn_mod = 1;
    sys__mac_on_frame();
  }
}
- (void)otherMouseDragged:(NSEvent*)evt {
  sys_mac__mouse_pos(evt);
  if (abs(g_sys.mouse.pos_delta[0]) > 0 ||
      abs(g_sys.mouse.pos_delta[1]) > 0) {
    g_sys.btn_mod = 1;
    sys__mac_on_frame();
  }
}
- (void)scrollWheel:(NSEvent*)evt {
  float dx = castf(evt.scrollingDeltaX);
  float dy = castf(evt.scrollingDeltaY);
  if (evt.hasPreciseScrollingDeltas) {
    dx *= 0.1f;
    dy *= 0.1f;
  }
  g_mac.scrl[0] += dx;
  g_mac.scrl[1] += dy;

  if ((fabs(g_mac.scrl[0]) >= 1.0f) ||
      (fabs(g_mac.scrl[1]) >= 1.0f)) {
    g_sys.keymod |= sys__mac_mods(evt);

    g_sys.mouse.scrl[0] = casti(g_mac.scrl[0]);
    g_sys.mouse.scrl[1] = casti(g_mac.scrl[1]);

    g_mac.scrl[0] -= castf(g_sys.mouse.scrl[0]);
    g_mac.scrl[1] -= castf(g_sys.mouse.scrl[1]);

    g_sys.scrl_mod = 1;
  }
  sys__mac_on_frame();
}
- (void)keyDown:(NSEvent*)evt {
  g_sys.keymod |= sys__mac_mods(evt);
  sys__mac_on_key(g_sys.keys, evt.keyCode);
  g_sys.key_mod = 1;

  const NSString* chars = evt.characters;
  const NSUInteger len = chars.length;
  if (len > 0) {
    for (NSUInteger i = 0; i < len; i++) {
      const unichar codepoint = [chars characterAtIndex:i];
      if ((codepoint & 0xFF00) == 0xF700) {
        continue;
      }
      if (codepoint == '\t') {
        continue;
      }
      char buf[UTF_SIZ+1];
      int cnt = utf_enc(buf, cntof(buf), codepoint);
      if (g_sys.txt_len + cnt < cntof(g_sys.txt)) {
        memcpy(g_sys.txt + g_sys.txt_len, buf, castsz(cnt));
        g_sys.txt_len += cnt;
      }
    }
    g_sys.txt_mod = 1;
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
  g_sys.drw = 1;
  sys__mac_on_frame();
}
- (void) mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
  unused(view);
  unused(size);
}
@end

static BOOL
sys__mac_dnd_files(NSArray *files, enum sys_dnd_state state) {
  assert(files);
  BOOL ret = YES;
  int dnd_file_cnt = casti([files count]);
  for (int at = 0; dnd_file_cnt; ++at) {

    struct str lst[64];
    g_sys.dnd.state = SYS_DND_DELIVERY;
    g_sys.dnd.response = SYS_DND_REJECT;
    g_sys.dnd.file_cnt = min(dnd_file_cnt, cntof(files));
    g_sys.dnd.files = lst;

    for loop(i, g_sys.dnd.file_cnt) {
      NSUInteger idx = cast(NSUInteger, at);
      NSURL *fileUrl = [NSURL fileURLWithPath:[[files objectAtIndex:idx] stringForType:NSPasteboardTypeFileURL]];
      lst[i] = str0(fileUrl.standardizedURL.path.UTF8String);
    }
    g_sys.dnd_mod = 1;
    g_sys.dnd.state = state;
    sys__mac_on_frame();
    if (g_sys.dnd.response == SYS_DND_REJECT) {
      ret = NO;
      break;
    }
    dnd_file_cnt -= g_sys.dnd.file_cnt;
  }
  g_sys.dnd.state = SYS_DND_NONE;
  g_sys.dnd.response = SYS_DND_REJECT;
  g_sys.dnd.files = 0;
  g_sys.dnd_mod = 0;
  return ret;
}
static BOOL
sys__mac_dnd_str(NSArray *strs, enum sys_dnd_state state) {
  BOOL ret = NO;
  NSString *str = [strs objectAtIndex:0];
  NSData *utf8Data = [str dataUsingEncoding:NSUTF8StringEncoding];
  int len = cast(int, [utf8Data length]);

  g_sys.dnd.str = strn(cast(const char*, [utf8Data bytes]), len);
  g_sys.dnd.state = state;
  g_sys.dnd_mod = 1;
  sys__mac_on_frame();
  ret = g_sys.dnd.response == SYS_DND_ACCEPT ? YES : NO;

  g_sys.dnd.state = SYS_DND_NONE;
  g_sys.dnd.response = SYS_DND_REJECT;
  g_sys.dnd.str = str_nil;
  g_sys.dnd_mod = 0;
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
  NSLog(@"Drag enter Operation\n");
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
  NSLog(@"Drag update Operation\n");
  NSPoint pos = [g_mac.win mouseLocationOutsideOfEventStream];
  float new_x = castf(pos.x) * g_mac.dpi_scale[0];
  float new_y = castf(pos.y) * g_mac.dpi_scale[1];

  g_sys.mouse.pos[0] = casti(new_x);
  g_sys.mouse.pos[1] = g_sys.win.h - casti(new_y) - 1;

  g_sys.mouse.pos_delta[0] = g_sys.mouse.pos[0] - g_sys.mouse.pos_last[0];
  g_sys.mouse.pos_delta[1] = g_sys.mouse.pos[1] - g_sys.mouse.pos_last[1];

  g_sys.mouse_mod = 1;
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
  NSLog(@"Drag exit Operation\n");
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
  NSLog(@"Perform Drag Operation\n");
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
  g_sys.platform = &g_mac;
  g_sys.argc = argc;
  for loopn(i,argc, SYS_MAX_CMD_ARGS) {
    g_sys.argv[i] = str0(argv[i]);
  }
  cpu_info(&g_sys.cpu);

  /* memory */
  g_sys.mem.page_siz = sysconf(_SC_PAGE_SIZE);
  g_sys.mem.phy_siz = sysconf(_SC_PHYS_PAGES) * g_sys.mem.page_siz;

  /* api */
  g_sys.dir.lst = sys_dir_lst;
  g_sys.dir.nxt = sys_dir_nxt;
  g_sys.dir.exists = sys_dir_exists;
  g_sys.file.info = sys_file_info;
  g_sys.time.timestamp = sys_mac_timestamp;

  g_sys.con.log = sys__mac_log;
  g_sys.con.warn = sys__mac_warn;
  g_sys.con.err = sys__mac_err;

  g_sys.rnd.open = sys__rnd_open;
  g_sys.rnd.close = sys__rnd_close;
  g_sys.rnd.gen32 = sys__rnd_gen32;
  g_sys.rnd.gen64 = sys__rnd_gen64;
  g_sys.rnd.gen128 = sys__rnd_gen128;

  /* constants */
  g_mac.dpi_scale[0] = 1.0f;
  g_mac.dpi_scale[1] = 1.0f;

  /* application: act as a bundled app, even if not bundled */
  [NSApplication sharedApplication];
  g_mac.app_dlg = [[sys__mac_app_delegate alloc] init];
  NSApp.delegate = g_mac.app_dlg;

  NSEvent* (^keyup_monitor)(NSEvent*) = ^NSEvent* (NSEvent* event) {
    if ([event modifierFlags] & NSEventModifierFlagCommand) {
      [[NSApp keyWindow] sendEvent:event];
    }
    return event;
  };
  g_mac.keyup_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp handler:keyup_monitor];

  [NSApp run];
  return 0;
}

