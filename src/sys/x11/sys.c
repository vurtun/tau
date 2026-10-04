#define _GNU_SOURCE

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
#include <unistd.h>

/* x11 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

/* usr */
#include "../cpu.h"
#include "../../lib/fmt.h"
#include "../../lib/fmt.c"
#include "../../lib/std.h"
#include "../gfx.h"
#include "../sys.h"
#include "../../lib/std.c"
#include "../../lib/math.c"
#include "gfx_vk.c"
#include "../../app.h"

enum {
  SYS_X11_CLIP_MAX_BYTES = KB(64),
  SYS_X11_DND_MAX_FILES  = 64,
  SYS_X11_DBL_CLICK_US   = 300000ULL, /* 300 ms double click threshold */
};
struct sys_x11 {
  int quit;
  int col_mod;
  int needs_repaint;

  enum sys_cur_style cursor;
  struct str exe_path;
  struct gfx_vk vk;
  struct gfx_vk_target target;

  Display *dpy;
  int screen;
  Window root;
  Window win;
  XIM xim;
  XIC xic;

  /* Atoms */
  Atom wm_protocols;
  Atom wm_delete_window;
  Atom net_wm_name;
  Atom utf8_string;
  Atom targets;
  Atom clipboard;

  /* XDnD Atoms (Version 5) */
  Atom xdnd_aware;
  Atom xdnd_enter;
  Atom xdnd_position;
  Atom xdnd_status;
  Atom xdnd_leave;
  Atom xdnd_drop;
  Atom xdnd_finished;
  Atom xdnd_selection;
  Atom xdnd_action_copy;
  Atom xdnd_type_uri_list;
  Atom xdnd_type_plain;

  Window dnd_source;
  Atom dnd_target_type;

  /* Cursor Table */
  Cursor cursors[SYS_CUR_CNT];

  /* Clipboard Buffer */
  char clip_buf[SYS_X11_CLIP_MAX_BYTES];
  size_t clip_len;

  /* Mouse / Timing State */
  unsigned long long last_click_time[SYS_MOUSE_BTN_CNT];
  float scrl[2];
  float dpi_scale[2];
  int win_w, win_h;
};
static struct sys_x11 g_x11;
static struct sys g_sys;

/* ---------------------------------------------------------------------------
 *                                Util & Time
 * ---------------------------------------------------------------------------
 */
static unsigned long long
sys_linux_timestamp(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  unsigned long long sec = castull(ts.tv_sec) * 1000000llu;
  unsigned long long usec = castull(ts.tv_nsec) / 1000llu;
  return sec + usec;
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
  unsigned val = 0;
  int fp = casti(hdl);
  ssize_t ret = read(fp, cast(char*, &val), szof(val));
  if (ret < casts(szof(val))) {
    fprintf(stderr, "failed to generate system random number\n");
    exit(1);
  }
  return val;
}
static unsigned long long
sys__rnd_gen64(uintptr_t hdl) {
  unsigned long long val = 0;
  int fp = casti(hdl);
  ssize_t ret = read(fp, cast(char*, &val), szof(val));
  if (ret < casts(szof(val))) {
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
 *                                   File
 * ---------------------------------------------------------------------------
 */
struct sys_file_path {
  char ospath[MAX_FILE_PATH];
};
static int
sys__file_path_push(struct sys_file_path *sfp, struct str path) {
  assert(sfp);
  if (str_len(path) + 1 > MAX_FILE_PATH) {
    return 0;
  }
  mcpy(sfp->ospath, str_beg(path), str_len(path));
  sfp->ospath[str_len(path)] = 0;
  return 1;
}
static void
sys__file_path_pop(struct sys_file_path *sfp) {
  assert(sfp);
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
 *                                Directory
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
  unused(sys);
  if (!itr->valid) {
    return;
  }
  itr->valid = 0;
  itr->err = 0;
  closedir(cast(DIR*, itr->handle));
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
    struct dirent *ent = readdir(cast(DIR*, itr->handle));
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
 *                                   Log
 * ---------------------------------------------------------------------------
 */
static void
sys__linux_log(const char *fmt, ...) {
  char buf[2 * 1024];
  va_list args;
  va_start(args, fmt);
  fmtvsn(buf, szof(buf), fmt, args);
  va_end(args);
  fprintf(stdout, "%s", buf);
}
static void
sys__linux_warn(const char *fmt, ...) {
  char buf[2 * 1024];
  va_list args;
  va_start(args, fmt);
  fmtvsn(buf, szof(buf), fmt, args);
  va_end(args);
  fprintf(stdout, "\033[33m%s\033[0m", buf);
}
static void
sys__linux_err(const char *fmt, ...) {
  char buf[2 * 1024];
  va_list args;
  va_start(args, fmt);
  fmtvsn(buf, szof(buf), fmt, args);
  va_end(args);
  fprintf(stdout, "\033[31m%s\033[0m", buf);
}
/* ---------------------------------------------------------------------------
 *                                Clipboard
 * ---------------------------------------------------------------------------
 */
static void
sys__clip_set(struct sys *s, struct str val) {
  assert(s);
  unused(s);
  size_t len = castsz(str_len(val));
  if (len >= SYS_X11_CLIP_MAX_BYTES) {
    len = SYS_X11_CLIP_MAX_BYTES - 1;
  }
  mcpy(g_x11.clip_buf, str_beg(val), len);
  g_x11.clip_buf[len] = '\0';
  g_x11.clip_len = len;
  XSetSelectionOwner(g_x11.dpy, g_x11.clipboard, g_x11.win, CurrentTime);
}
static struct str
sys__clip_get(struct sys *s) {
  assert(s);
  unused(s);
  if (XGetSelectionOwner(g_x11.dpy, g_x11.clipboard) == g_x11.win) {
    return strn(g_x11.clip_buf, casti(g_x11.clip_len));
  }
  Atom prop = XInternAtom(g_x11.dpy, "TAU_CLIPBOARD_PROP", False);
  XConvertSelection(g_x11.dpy, g_x11.clipboard, g_x11.utf8_string, prop, g_x11.win, CurrentTime);
  XFlush(g_x11.dpy);

  /* Bounded polling for SelectionNotify (Strict WCET limit: max 50ms) */
  unsigned long long start = sys_linux_timestamp();
  XEvent event;
  int acquired = 0;
  while ((sys_linux_timestamp() - start) < 50000ULL) {
    if (XCheckTypedWindowEvent(g_x11.dpy, g_x11.win, SelectionNotify, &event)) {
      if (event.xselection.property != None) {
        acquired = 1;
      }
      break;
    }
  }
  if (!acquired) {
    return str_nil;
  }
  Atom actual_type;
  int actual_fmt;
  unsigned long nitems = 0, bytes_after = 0;
  unsigned char *prop_data = NULL;
  XGetWindowProperty(g_x11.dpy, g_x11.win, prop, 0, (long)(SYS_X11_CLIP_MAX_BYTES / 4),
    True, AnyPropertyType, &actual_type, &actual_fmt, &nitems,
    &bytes_after, &prop_data);
  if (!prop_data || actual_fmt != 8) {
    if (prop_data) XFree(prop_data);
    return str_nil;
  }
  size_t len = min(nitems, (size_t)(SYS_X11_CLIP_MAX_BYTES - 1));
  mcpy(g_x11.clip_buf, prop_data, len);
  g_x11.clip_buf[len] = '\0';
  g_x11.clip_len = len;
  XFree(prop_data);
  return strn(g_x11.clip_buf, casti(g_x11.clip_len));
}

/* ---------------------------------------------------------------------------
 *                                  Frame
 * ---------------------------------------------------------------------------
 */
static void
sys__x11_on_frame(void) {
  g_sys.seq++;
  g_sys.txt_mod = !!g_sys.txt_len;
  g_sys.dpi_scale = g_x11.dpi_scale[0];
  g_sys.win.w = g_x11.win_w;
  g_sys.win.h = g_x11.win_h;
  if (g_sys.drw) {
    g_sys.gfx.begin(&g_sys, g_sys.win.w, g_sys.win.h);
  }
  app_run(&g_sys);
  if (g_sys.cursor != g_x11.cursor) {
    if (g_sys.cursor < SYS_CUR_CNT && g_x11.cursors[g_sys.cursor] != None) {
      XDefineCursor(g_x11.dpy, g_x11.win, g_x11.cursors[g_sys.cursor]);
    }
    g_x11.cursor = g_sys.cursor;
  }
  g_sys.tooltip.str = str_nil;
  if (g_sys.repaint) {
    g_x11.needs_repaint = 1;
  }
  if (g_sys.drw) {
    g_x11.target.w = g_sys.win.w;
    g_x11.target.h = g_sys.win.h;
    g_sys.gfx.end(&g_sys, cast(void*, &g_x11.target));
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

  for arr_loopv(i, g_sys.keys) {
    g_sys.keys[i] = 0;
  }
  for arr_loopv(i, g_sys.mouse.btns) {
    g_sys.mouse.btns[i].pressed = 0;
    g_sys.mouse.btns[i].released = 0;
    g_sys.mouse.btns[i].doubled = 0;
  }
}
static void
sys__x11_resize(int w, int h) {
  int width = max(1, w);
  int height = max(1, h);
  if (g_x11.win_w == width && g_x11.win_h == height) {
    return;
  }
  g_x11.win_w = width;
  g_x11.win_h = height;
  g_sys.gfx.resize(&g_sys, width, height);
  g_sys.drw = 1;
  sys__x11_on_frame();
}

/* ---------------------------------------------------------------------------
 *                                  Input
 * ---------------------------------------------------------------------------
 */
static void
sys__x11_on_btn(struct sys_btn *btn, int down, int btn_id) {
  assert(btn);
  int was_down = btn->down;
  btn->down = !!down;
  btn->pressed = !was_down && down;
  btn->released = was_down && !down;
  btn->doubled = 0;

  if (btn->pressed) {
    unsigned long long now = sys_linux_timestamp();
    if ((now - g_x11.last_click_time[btn_id]) < SYS_X11_DBL_CLICK_US) {
      btn->doubled = 1;
    }
    g_x11.last_click_time[btn_id] = now;
  }
}
static unsigned
sys__x11_mods(unsigned state) {
  unsigned res = 0u;
  if (state & ControlMask) {
    res |= SYS_KEYMOD_CTRL;
  }
  if (state & ShiftMask) {
    res |= SYS_KEYMOD_SHIFT;
  }
  if (state & Mod1Mask) {
    res |= SYS_KEYMOD_ALT;
  }
  return res;
}
static void
sys__x11_on_key(unsigned long *keys, KeySym sym) {
  if (sym >= 'a' && sym <= 'z') {
    bit_set(keys, sym);
    bit_set(keys, sym - ('a' - 'A'));
    return;
  }
  if (sym >= '0' && sym <= '9') {
    bit_set(keys, sym);
    return;
  }
  switch (sym) {
  default: break;
  case XK_Return: bit_set(keys, SYS_KEY_RETURN); break;
  case XK_Escape: bit_set(keys, SYS_KEY_ESCAPE); break;
  case XK_BackSpace: bit_set(keys, SYS_KEY_BACKSPACE); break;
  case XK_Tab: bit_set(keys, SYS_KEY_TAB); break;
  case XK_space: bit_set(keys, ' '); break;
  case XK_Delete: bit_set(keys, SYS_KEY_DEL); break;
  case XK_Home: bit_set(keys, SYS_KEY_HOME); break;
  case XK_End: bit_set(keys, SYS_KEY_END); break;
  case XK_Page_Up: bit_set(keys, SYS_KEY_PGUP); break;
  case XK_Page_Down: bit_set(keys, SYS_KEY_PGDN); break;
  case XK_Up: bit_set(keys, SYS_KEY_UP); break;
  case XK_Down: bit_set(keys, SYS_KEY_DOWN); break;
  case XK_Left: bit_set(keys, SYS_KEY_LEFT); break;
  case XK_Right: bit_set(keys, SYS_KEY_RIGHT); break;
  case XK_Caps_Lock: bit_set(keys, SYS_KEY_CAPS); break;
  case XK_F1: bit_set(keys, SYS_KEY_F1); break;
  case XK_F2: bit_set(keys, SYS_KEY_F2); break;
  case XK_F3: bit_set(keys, SYS_KEY_F3); break;
  case XK_F4: bit_set(keys, SYS_KEY_F4); break;
  case XK_F5: bit_set(keys, SYS_KEY_F5); break;
  case XK_F6: bit_set(keys, SYS_KEY_F6); break;
  case XK_F7: bit_set(keys, SYS_KEY_F7); break;
  case XK_F8: bit_set(keys, SYS_KEY_F8); break;
  case XK_F9: bit_set(keys, SYS_KEY_F9); break;
  case XK_F10: bit_set(keys, SYS_KEY_F10); break;
  case XK_F11: bit_set(keys, SYS_KEY_F11); break;
  case XK_F12: bit_set(keys, SYS_KEY_F12); break;
  case XK_KP_Enter: bit_set(keys, SYS_KEY_RETURN); break;
  case XK_KP_Add: bit_set(keys, '+'); break;
  case XK_KP_Subtract: bit_set(keys, '-'); break;
  case XK_KP_Multiply: bit_set(keys, '*'); break;
  case XK_KP_Divide: bit_set(keys, '/'); break;
  case XK_KP_Decimal: bit_set(keys, '.'); break;
  case XK_KP_Equal: bit_set(keys, '='); break;
  }
}

/* ---------------------------------------------------------------------------
 *                                  XDnD
 * ---------------------------------------------------------------------------
 */
static void
sys__x11_dnd_respond(Window source, int accept) {
  XEvent reply;
  mset(&reply, 0, szof(reply));
  reply.type = ClientMessage;
  reply.xclient.window = source;
  reply.xclient.message_type = g_x11.xdnd_status;
  reply.xclient.format = 32;
  reply.xclient.data.l[0] = (long)g_x11.win;
  reply.xclient.data.l[1] = accept ? 1 : 0;
  reply.xclient.data.l[2] = 0;
  reply.xclient.data.l[3] = 0;
  reply.xclient.data.l[4] = accept ? (long)g_x11.xdnd_action_copy : 0;
  XSendEvent(g_x11.dpy, source, False, NoEventMask, &reply);
  XFlush(g_x11.dpy);
}
static void
sys__x11_dnd_finish(Window source) {
  XEvent reply;
  mset(&reply, 0, szof(reply));
  reply.type = ClientMessage;
  reply.xclient.window = source;
  reply.xclient.message_type = g_x11.xdnd_finished;
  reply.xclient.format = 32;
  reply.xclient.data.l[0] = (long)g_x11.win;
  reply.xclient.data.l[1] = 1;
  reply.xclient.data.l[2] = (long)g_x11.xdnd_action_copy;
  XSendEvent(g_x11.dpy, source, False, NoEventMask, &reply);
  XFlush(g_x11.dpy);
}
static void
sys__x11_handle_dnd(XClientMessageEvent *cme) {
  if (cme->message_type == g_x11.xdnd_enter) {
    g_x11.dnd_source = (Window)cme->data.l[0];
    g_x11.dnd_target_type = None;
    int more_than_3 = (cme->data.l[1] & 1);
    if (!more_than_3) {
      for (int i = 2; i <= 4; ++i) {
        Atom type = (Atom)cme->data.l[i];
        if (type == g_x11.xdnd_type_uri_list || type == g_x11.xdnd_type_plain) {
          g_x11.dnd_target_type = type;
          break;
        }
      }
    } else {
      Atom actual_type;
      int actual_fmt;
      unsigned long nitems = 0, bytes_after = 0;
      Atom *types = NULL;
      XGetWindowProperty(g_x11.dpy, g_x11.dnd_source, g_x11.xdnd_type_list, 0, 16, False,
        XA_ATOM, &actual_type, &actual_fmt, &nitems, &bytes_after,
        recast(unsigned char**, &types));
      if (types) {
        for (unsigned long i = 0; i < nitems; ++i) {
          if (types[i] == g_x11.xdnd_type_uri_list || types[i] == g_x11.xdnd_type_plain) {
            g_x11.dnd_target_type = types[i];
            break;
          }
        }
        XFree(types);
      }
    }
    g_sys.dnd.state = SYS_DND_ENTER;
    g_sys.dnd_mod = 1;
    sys__x11_on_frame();
    return;
  }
  if (cme->message_type == g_x11.xdnd_position) {
    Window src = (Window)cme->data.l[0];
    int x = (int)(cme->data.l[2] >> 16);
    int y = (int)(cme->data.l[2] & 0xFFFF);

    int win_x = 0, win_y = 0;
    Window child;
    XTranslateCoordinates(g_x11.dpy, g_x11.root, g_x11.win, x, y, &win_x, &win_y, &child);
    g_sys.mouse.pos[0] = win_x;
    g_sys.mouse.pos[1] = g_sys.win.h - win_y - 1;
    g_sys.mouse_mod = 1;

    g_sys.dnd.state = SYS_DND_PREVIEW;
    g_sys.dnd_mod = 1;
    sys__x11_on_frame();
    sys__x11_dnd_respond(src, g_x11.dnd_target_type != None);
    return;
  }
  if (cme->message_type == g_x11.xdnd_leave) {
    g_sys.dnd.state = SYS_DND_LEFT;
    g_sys.dnd_mod = 1;
    sys__x11_on_frame();
    g_x11.dnd_source = None;
    return;
  }
  if (cme->message_type == g_x11.xdnd_drop) {
    Window src = (Window)cme->data.l[0];
    Time t = (Time)cme->data.l[2];
    if (g_x11.dnd_target_type != None) {
      XConvertSelection(g_x11.dpy, g_x11.xdnd_selection, g_x11.dnd_target_type,
        g_x11.xdnd_selection, g_x11.win, t);
    } else {
      sys__x11_dnd_finish(src);
    }
  }
}

/* ---------------------------------------------------------------------------
 *                               Event Loop
 * ---------------------------------------------------------------------------
 */
static void
sys__x11_handle_event(XEvent *event) {
  if (XFilterEvent(event, None)) {
    return;
  }
  switch (event->type) {
  default: break;
  case ClientMessage: {
    if ((Atom)event->xclient.data.l[0] == g_x11.wm_delete_window) {
      g_sys.quit = 1;
      g_x11.quit = 1;
    } else {
      sys__x11_handle_dnd(&event->xclient);
    }
  } break;
  case ConfigureNotify: {
    sys__x11_resize(event->xconfigure.width, event->xconfigure.height);
  } break;
  case Expose: {
    g_x11.needs_repaint = 1;
  } break;
  case FocusIn: {
    g_sys.focus = 1;
    sys__x11_on_frame();
  } break;
  case FocusOut: {
    g_sys.focus = 0;
    sys__x11_on_frame();
  } break;
  case MotionNotify: {
    g_sys.mouse.pos[0] = event->xmotion.x;
    g_sys.mouse.pos[1] = g_sys.win.h - event->xmotion.y - 1;
    g_sys.mouse.pos_delta[0] = g_sys.mouse.pos[0] - g_sys.mouse.pos_last[0];
    g_sys.mouse.pos_delta[1] = g_sys.mouse.pos[1] - g_sys.mouse.pos_last[1];
    g_sys.mouse_mod = 1;
    g_sys.keymod |= sys__x11_mods(event->xmotion.state);
    if (abs(g_sys.mouse.pos_delta[0]) > 0 || abs(g_sys.mouse.pos_delta[1]) > 0) {
      sys__x11_on_frame();
    }
  } break;

  case ButtonPress:
  case ButtonRelease: {
    int down = (event->type == ButtonPress);
    unsigned int btn = event->xbutton.button;
    g_sys.keymod |= sys__x11_mods(event->xbutton.state);
    if (btn == Button1) {
      sys__x11_on_btn(&g_sys.mouse.btn.left, down, SYS_MOUSE_LEFT);
      g_sys.btn_mod = 1;
    } else if (btn == Button2) {
      sys__x11_on_btn(&g_sys.mouse.btn.middle, down, SYS_MOUSE_MIDDLE);
      g_sys.btn_mod = 1;
    } else if (btn == Button3) {
      sys__x11_on_btn(&g_sys.mouse.btn.right, down, SYS_MOUSE_RIGHT);
      g_sys.btn_mod = 1;
    } else if (down && btn == Button4) {
      g_sys.mouse.scrl[1] = 1;
      g_sys.scrl_mod = 1;
    } else if (down && btn == Button5) {
      g_sys.mouse.scrl[1] = -1;
      g_sys.scrl_mod = 1;
    } else if (down && btn == Button6) {
      g_sys.mouse.scrl[0] = -1;
      g_sys.scrl_mod = 1;
    } else if (down && btn == Button7) {
      g_sys.mouse.scrl[0] = 1;
      g_sys.scrl_mod = 1;
    }
    sys__x11_on_frame();
  } break;

  case KeyPress: {
    g_sys.keymod |= sys__x11_mods(event->xkey.state);
    char buf[64];
    KeySym sym = None;
    Status status = 0;
    int len = 0;
    if (g_x11.xic) {
      len = Xutf8LookupString(g_x11.xic, &event->xkey, buf, szof(buf), &sym, &status);
    } else {
      len = XLookupString(&event->xkey, buf, szof(buf), &sym, NULL);
    }
    if (sym != None) {
      sys__x11_on_key(g_sys.keys, sym);
      g_sys.key_mod = 1;
    }
    if (len > 0) {
      for (int i = 0; i < len; ++i) {
        if (g_sys.txt_len < cntof(g_sys.txt) - 1) {
          g_sys.txt[g_sys.txt_len++] = buf[i];
        }
      }
      g_sys.txt_mod = 1;
    }
    sys__x11_on_frame();
  } break;

  case SelectionRequest: {
    XSelectionRequestEvent *req = &event->xselectionrequest;
    XEvent reply;
    mset(&reply, 0, szof(reply));
    reply.type = SelectionNotify;
    reply.xselection.display = req->display;
    reply.xselection.requestor = req->requestor;
    reply.xselection.selection = req->selection;
    reply.xselection.target = req->target;
    reply.xselection.property = None;
    reply.xselection.time = req->time;
    if (req->target == g_x11.targets) {
      Atom supported[] = {g_x11.targets, g_x11.utf8_string, XA_STRING};
      XChangeProperty(g_x11.dpy, req->requestor, req->property, XA_ATOM, 32,
        PropModeReplace, recast(unsigned char*, supported), cntof(supported));
      reply.xselection.property = req->property;
    } else if (req->target == g_x11.utf8_string || req->target == XA_STRING) {
      XChangeProperty(g_x11.dpy, req->requestor, req->property, req->target, 8,
        PropModeReplace, recast(unsigned char*, g_x11.clip_buf), casti(g_x11.clip_len));
      reply.xselection.property = req->property;
    }
    XSendEvent(g_x11.dpy, req->requestor, False, NoEventMask, &reply);
  } break;

  case SelectionNotify: {
    XSelectionEvent *sel = &event->xselection;
    if (sel->property == g_x11.xdnd_selection) {
      Atom actual_type;
      int actual_fmt;
      unsigned char *data = NULL;
      unsigned long nitems = 0;
      unsigned long bytes_after = 0;
      XGetWindowProperty(g_x11.dpy, g_x11.win, g_x11.xdnd_selection, 0,
        (long)(SYS_X11_CLIP_MAX_BYTES / 4), True, AnyPropertyType,
        &actual_type, &actual_fmt, &nitems, &bytes_after, &data);

      if (data && nitems > 0) {
        if (sel->target == g_x11.xdnd_type_uri_list) {
          struct str file_ptrs[SYS_X11_DND_MAX_FILES];
          int file_count = 0;
          char *token = strtok(recast(char*, data), "\r\n");
          while (token && file_count < SYS_X11_DND_MAX_FILES) {
            if (strncmp(token, "file://", 7) == 0) {
              file_ptrs[file_count++] = str0(token + 7);
            }
            token = strtok(NULL, "\r\n");
          }
          g_sys.dnd.files = file_ptrs;
          g_sys.dnd.file_cnt = file_count;
          g_sys.dnd.state = SYS_DND_DELIVERY;
          g_sys.dnd_mod = 1;
          sys__x11_on_frame();
        } else if (sel->target == g_x11.xdnd_type_plain) {
          g_sys.dnd.str = strn(recast(char*, data), casti(nitems));
          g_sys.dnd.state = SYS_DND_DELIVERY;
          g_sys.dnd_mod = 1;
          sys__x11_on_frame();
        }
        XFree(data);
      }
      sys__x11_dnd_finish(g_x11.dnd_source);
      g_x11.dnd_source = None;
    }
  } break;
  }
}

/* ---------------------------------------------------------------------------
 *                                Entry Point
 * ---------------------------------------------------------------------------
 */
int
main(int argc, char *argv[]) {
  g_sys.platform = &g_x11;
  g_sys.argc = argc;
  for loopn(i, argc, SYS_MAX_CMD_ARGS) {
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
  g_sys.time.timestamp = sys_linux_timestamp;
  g_sys.clipboard.set = sys__clip_set;
  g_sys.clipboard.get = sys__clip_get;

  g_sys.con.log = sys__linux_log;
  g_sys.con.warn = sys__linux_warn;
  g_sys.con.err = sys__linux_err;

  g_sys.rnd.open = sys__rnd_open;
  g_sys.rnd.close = sys__rnd_close;
  g_sys.rnd.gen32 = sys__rnd_gen32;
  g_sys.rnd.gen64 = sys__rnd_gen64;
  g_sys.rnd.gen128 = sys__rnd_gen128;

  /* default palette (deterministic dark theme matching Cocoa style) */
  g_sys.has_style = 1;
  g_sys.style_mod = 1;
  g_sys.col[SYS_COL_WIN]          = 0xff1e1e1eu;
  g_sys.col[SYS_COL_BG]           = 0xff252526u;
  g_sys.col[SYS_COL_CTRL]         = 0xff333333u;
  g_sys.col[SYS_COL_SEL]          = 0xff094771u;
  g_sys.col[SYS_COL_HOV]          = 0xff2a2d2eu;
  g_sys.col[SYS_COL_TXT]          = 0xffccccccu;
  g_sys.col[SYS_COL_TXT_SEL]      = 0xffffffffu;
  g_sys.col[SYS_COL_TXT_DISABLED] = 0xff6c6c6cu;
  g_sys.col[SYS_COL_ICO]          = 0xffccccccu;
  g_sys.col[SYS_COL_LIGHT]        = 0xff3c3c3cu;
  g_sys.col[SYS_COL_SHADOW]       = 0xff181818u;
  g_sys.fnt_pnt_size = 13.0f;
  g_x11.dpi_scale[0] = 1.0f;
  g_x11.dpi_scale[1] = 1.0f;

  g_sys.op = SYS_SETUP;
  app_run(&g_sys);

  /* X11 Setup */
  g_x11.dpy = XOpenDisplay(NULL);
  if (!g_x11.dpy) {
    sys__linux_err("[x11] failed to open display\n");
    return 1;
  }
  g_x11.screen = DefaultScreen(g_x11.dpy);
  g_x11.root = RootWindow(g_x11.dpy, g_x11.screen);

  /* High-DPI Scaling via Xft.dpi */
  char *dpi_str = XGetDefault(g_x11.dpy, "Xft", "dpi");
  if (dpi_str) {
    float dpi = castf(atof(dpi_str));
    if (dpi > 0.0f) {
      g_x11.dpi_scale[0] = dpi / 96.0f;
      g_x11.dpi_scale[1] = dpi / 96.0f;
    }
  }
  g_sys.dpi_scale = g_x11.dpi_scale[0];

  /* Atoms */
  g_x11.wm_protocols     = XInternAtom(g_x11.dpy, "WM_PROTOCOLS", False);
  g_x11.wm_delete_window = XInternAtom(g_x11.dpy, "WM_DELETE_WINDOW", False);
  g_x11.net_wm_name      = XInternAtom(g_x11.dpy, "_NET_WM_NAME", False);
  g_x11.utf8_string      = XInternAtom(g_x11.dpy, "UTF8_STRING", False);
  g_x11.targets          = XInternAtom(g_x11.dpy, "TARGETS", False);
  g_x11.clipboard        = XInternAtom(g_x11.dpy, "CLIPBOARD", False);

  /* XDnD Atoms */
  g_x11.xdnd_aware         = XInternAtom(g_x11.dpy, "XdndAware", False);
  g_x11.xdnd_enter         = XInternAtom(g_x11.dpy, "XdndEnter", False);
  g_x11.xdnd_position      = XInternAtom(g_x11.dpy, "XdndPosition", False);
  g_x11.xdnd_status        = XInternAtom(g_x11.dpy, "XdndStatus", False);
  g_x11.xdnd_leave         = XInternAtom(g_x11.dpy, "XdndLeave", False);
  g_x11.xdnd_drop          = XInternAtom(g_x11.dpy, "XdndDrop", False);
  g_x11.xdnd_finished      = XInternAtom(g_x11.dpy, "XdndFinished", False);
  g_x11.xdnd_selection     = XInternAtom(g_x11.dpy, "XdndSelection", False);
  g_x11.xdnd_action_copy   = XInternAtom(g_x11.dpy, "XdndActionCopy", False);
  g_x11.xdnd_type_uri_list = XInternAtom(g_x11.dpy, "text/uri-list", False);
  g_x11.xdnd_type_plain    = XInternAtom(g_x11.dpy, "text/plain", False);

  /* Window Creation */
  g_x11.win_w = g_sys.win.w;
  g_x11.win_h = g_sys.win.h;

  XSetWindowAttributes swa;
  swa.event_mask = StructureNotifyMask | ExposureMask | FocusChangeMask |
    KeyPressMask | KeyReleaseMask | ButtonPressMask |
    ButtonReleaseMask | PointerMotionMask;
  swa.background_pixel = BlackPixel(g_x11.dpy, g_x11.screen);
  g_x11.win = XCreateWindow(g_x11.dpy, g_x11.root, 0, 0,
    castu(g_x11.win_w), castu(g_x11.win_h), 0,
    CopyFromParent, InputOutput, CopyFromParent,
    CWEventMask | CWBackPixel, &swa);
  XSetWMProtocols(g_x11.dpy, g_x11.win, &g_x11.wm_delete_window, 1);
  XStoreName(g_x11.dpy, g_x11.win, g_sys.win.title);

  /* XDnD Awareness (Version 5) */
  long xdnd_version = 5;
  XChangeProperty(g_x11.dpy, g_x11.win, g_x11.xdnd_aware, XA_ATOM, 32,
    PropModeReplace, recast(unsigned char*, &xdnd_version), 1);

  /* Size Hints */
  XSizeHints hints;
  mset(&hints, 0, szof(hints));
  if (g_sys.win.min_w > 0 && g_sys.win.min_h > 0) {
    hints.flags |= PMinSize;
    hints.min_width = g_sys.win.min_w;
    hints.min_height = g_sys.win.min_h;
  }
  if (g_sys.win.max_w > 0 && g_sys.win.max_h > 0) {
    hints.flags |= PMaxSize;
    hints.max_width = g_sys.win.max_w;
    hints.max_height = g_sys.win.max_h;
  }
  XSetWMNormalHints(g_x11.dpy, g_x11.win, &hints);

  /* Cursors */
  g_x11.cursors[SYS_CUR_ARROW]   = XCreateFontCursor(g_x11.dpy, XC_left_ptr);
  g_x11.cursors[SYS_CUR_NO]      = XCreateFontCursor(g_x11.dpy, XC_X_cursor);
  g_x11.cursors[SYS_CUR_CROSS]   = XCreateFontCursor(g_x11.dpy, XC_crosshair);
  g_x11.cursors[SYS_CUR_HAND]    = XCreateFontCursor(g_x11.dpy, XC_hand2);
  g_x11.cursors[SYS_CUR_IBEAM]   = XCreateFontCursor(g_x11.dpy, XC_xterm);
  g_x11.cursors[SYS_CUR_MOVE]    = XCreateFontCursor(g_x11.dpy, XC_fleur);
  g_x11.cursors[SYS_CUR_SIZE_NS] = XCreateFontCursor(g_x11.dpy, XC_sb_v_double_arrow);
  g_x11.cursors[SYS_CUR_SIZE_WE] = XCreateFontCursor(g_x11.dpy, XC_sb_h_double_arrow);

  /* XIM / XIC Setup */
  g_x11.xim = XOpenIM(g_x11.dpy, NULL, NULL, NULL);
  if (g_x11.xim) {
    g_x11.xic = XCreateIC(g_x11.xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
      XNClientWindow, g_x11.win, XNFocusWindow, g_x11.win, NULL);
  }
  /* Target surface info for gfx_vk */
  g_x11.target.dpy = g_x11.dpy;
  g_x11.target.win = g_x11.win;
  g_x11.target.w = g_x11.win_w;
  g_x11.target.h = g_x11.win_h;

  /* Initialize Graphics API */
  gfx_api(&g_sys.gfx, 0);
  g_sys.ren = &g_x11.vk;
  if (g_sys.gfx.init(&g_sys, cast(void*, &g_x11.target)) != 0) {
    sys__linux_err("[sys] graphics initialization failed\n");
    return 1;
  }
  XMapWindow(g_x11.dpy, g_x11.win);
  XFlush(g_x11.dpy);

  g_sys.op = SYS_INIT;
  app_run(&g_sys);
  g_sys.op = SYS_RUN;

  /* Initial frame render */
  g_sys.drw = 1;
  sys__x11_on_frame();

  /* Main Loop */
  while (!g_sys.quit && !g_x11.quit) {
    /* 1. If idle (no repaint pending and no events in queue), sleep until the next event */
    if (!g_x11.needs_repaint && !XPending(g_x11.dpy)) {
      XEvent event;
      XNextEvent(g_x11.dpy, &event);
      sys__x11_handle_event(&event);
    }
    /* 2. Drain all remaining queued events */
    while (XPending(g_x11.dpy) > 0) {
      XEvent event;
      XNextEvent(g_x11.dpy, &event);
      sys__x11_handle_event(&event);
    }
    /* 3. If any event requested a repaint (or an animation is running), draw one frame */
    if (g_x11.needs_repaint) {
      g_x11.needs_repaint = 0;
      g_sys.drw = 1;
      sys__x11_on_frame();
    }
  }
  /* Teardown */
  g_sys.op = SYS_QUIT;
  sys__x11_on_frame();
  g_sys.gfx.shutdown(&g_sys);
  for loop(i, SYS_CUR_CNT) {
    if (g_x11.cursors[i] != None) {
      XFreeCursor(g_x11.dpy, g_x11.cursors[i]);
    }
  }
  if (g_x11.xic) {
    XDestroyIC(g_x11.xic);
  }
  if (g_x11.xim) {
    XCloseIM(g_x11.xim);
  }
  XDestroyWindow(g_x11.dpy, g_x11.win);
  XCloseDisplay(g_x11.dpy);
  return 0;
}