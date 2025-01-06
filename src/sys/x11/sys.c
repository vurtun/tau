#define _GNU_SOURCE
#define VK_USE_PLATFORM_XLIB_KHR

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
#include <assert.h>
#include <locale.h>

/* os */
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <time.h>

/* x11 */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

struct gfx_param {
  Display *dpy;
  Window win;
};

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

#ifndef SYS_DOUBLE_CLICK_LO
#define SYS_DOUBLE_CLICK_LO 20000000lu
#endif
#ifndef SYS_DOUBLE_CLICK_HI
#define SYS_DOUBLE_CLICK_HI 300000000llu
#endif

struct sys_x11_win {
  int w,h;
  GC gc;
  Window window;
  XWindowAttributes attr;
  XSetWindowAttributes swa;
  struct sys_mouse mouse;
};
struct sys_x11_clipboard {
  char *data;
  int len;
};
struct sys_x11 {
  int quit;
  int col_mod;

  struct sys_x11_win win;
  struct sys_x11_clipboard clip;
  enum sys_cur_style cursor;
  int grab_cnt;
  struct gfx_vk vk;

  float dpi_scale[2];
  int win_w, win_h;

  Display *dpy;
  Window root;
  Visual *vis;
  Colormap cmap;
  Window helper;
  int screen;
  int fd;

  Cursor cursors[SYS_CUR_CNT];

  /* atoms */
  Atom xa_wm_protocols;
  Atom xa_wm_del_win;
  Atom xa_txt_uri_lst;
  Atom xa_sys_tray;
  Atom xa_wm_hints;
  Atom xa_wm_icon;

  /* window type */
  Atom xa_win_type;
  Atom xa_wm_desk;
  Atom xa_wm_dock;
  Atom xa_wm_tool;
  Atom xa_wm_menu;
  Atom xa_wm_util;
  Atom xa_wm_splsh;
  Atom xa_wm_pop;
  Atom xa_wm_norm;

  Atom xa_wm_st;
  Atom xa_wm_st_maximized_horz;
  Atom xa_wm_st_maximized_vert;

  Atom xa_clip;
  Atom xa_tar;
  Atom xa_txt;
  Atom xa_utf_str;
};
static struct sys_x11 g_x11;
static struct sys g_sys;

/* ---------------------------------------------------------------------------
 *                                Util
 * ---------------------------------------------------------------------------
 */
static void no_return
xpanic(const char *fmt, ...) {
  assert(fmt);
  va_list args;
  fflush(stdout);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  exit(EXIT_FAILURE);
}
static void *
xrealloc(void *p, int size) {
  assert(p);
  p = realloc(p, (size_t)size);
  if (!p) {
    perror("xrealloc failed!\n");
    exit(1);
  }
  return p;
}
static void *
xmalloc(int size) {
  void *p = calloc(1, (size_t)size);
  if (!p) {
    perror("xmalloc failed!\n");
    exit(1);
  }
  return p;
}
static void *
xcalloc(int n, int size) {
  assert(n);
  void *p = calloc((size_t)n, (size_t)size);
  if (!p) {
    perror("xmalloc failed!\n");
    exit(1);
  }
  return p;
}
static void
sys__x11_dpi_scale(float *scale) {
  const int dpyw = DisplayWidth(g_x11.dpy, g_x11.screen);
  const int dpyh = DisplayHeight(g_x11.dpy, g_x11.screen);
  const int dpywmm = DisplayWidthMM(g_x11.dpy, g_x11.screen);
  const int dpyhmm = DisplayHeightMM(g_x11.dpy, g_x11.screen);

  float xdpi = cast(float, dpyw) * 25.4f / cast(float, dpywmm);
  float ydpi = cast(float, dpyh) * 25.4f / cast(float, dpyhmm);

  char *rms = XResourceManagerString(g_x11.dpy);
  if (rms) {
    XrmDatabase db = XrmGetStringDatabase(rms);
    if (db) {
      XrmValue value;
      char *type = 0, *ep = 0;
      if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value)) {
        if (type && strcmp(type, "String") == 0) {
          float dpi = strtof(value.addr, &ep);
          if (*ep == '\0' && ep != value.addr) {
            xdpi = ydpi = dpi;
          }
        }
      }
      XrmDestroyDatabase(db);
    }
  }
  scale[0] = xdpi / 96.0f;
  scale[1] = ydpi / 96.0f;
}
static unsigned long long
sys_x11_timestamp(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  unsigned long long sec = cast(unsigned long long, ts.tv_sec) * 1000000llu;
  unsigned long long nsec = cast(unsigned long long, ts.tv_nsec);
  unsigned long long ret = sec + nsec;
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
sys_dir_exists(struct sys *sys, struct str path) {
  assert(sys);
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
  struct sys_file_path fp;
  if (!sys__file_path_push(&fp, path)) {
    itr->valid = 0;
    itr->err = 1;
    return;
  }
  mset(itr, 0, szof(*itr));
  DIR *dir = opendir(fp.ospath);
  sys__file_path_pop(&fp);
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
sys__x11_log(const char *fmt, ...) {
  char buf[2*1024];
  va_list args;
  va_start(args, fmt);
  fmtvsn(buf, szof(buf), fmt, args);
  va_end(args);
  fprintf(stdout, "%s", buf);
}
static void
sys__x11_warn(const char *fmt, ...) {
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
sys__x11_err(const char *fmt, ...) {
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
 *                                Clipboard
 * ---------------------------------------------------------------------------
 */
static int
sys_x11_clip_supports_type(Atom type) {
  if (type == g_x11.xa_txt) {
    return True;
  }
  if (type == g_x11.xa_utf_str) {
    return True;
  }
  if (type == XA_STRING) {
    return True;
  }
  return False;
}
static void
sys_x11_clip_clear(void) {
  free(g_x11.clip.data);
  g_x11.clip.data = 0;
  g_x11.clip.len = 0;
}
static void
sys_x11_clip_set_str(const char *str, int len) {
  free(g_x11.clip.data);
  g_x11.clip.len = len;
  g_x11.clip.data = xmalloc(len + 1);
  strcpy(g_x11.clip.data, str);

  XSetSelectionOwner(g_x11.dpy, XA_PRIMARY, g_x11.helper, CurrentTime);
  XSetSelectionOwner(g_x11.dpy, g_x11.xa_clip, g_x11.helper, CurrentTime);
}
static char *
sys_x11_clip_get(struct sys *sys, Atom selection, Atom target) {
  assert(sys);
  XConvertSelection(g_x11.dpy, selection, target, selection, g_x11.helper, CurrentTime);
  /* blocking wait for clipboard data */
  XEvent evt;
  while (!XCheckTypedWindowEvent(g_x11.dpy, g_x11.helper, SelectionNotify, &evt)) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(g_x11.fd, &fds);
    if (select(g_x11.fd + 1, &fds, 0, 0, 0) == -1 || errno == EINTR) {
      return 0;
    }
  }
  if (evt.xselection.property == None) {
    return 0;
  }
  char *data = 0;
  int actual_fmt;
  Atom actual_type;
  unsigned long item_cnt;
  unsigned long bytes_after;

  /* get clipboard data out of window property */
  XGetWindowProperty(g_x11.dpy, evt.xselection.requestor, evt.xselection.property,
    0, LONG_MAX, True, AnyPropertyType, &actual_type, &actual_fmt, &item_cnt,
    &bytes_after, (unsigned char **)&data);
  if (actual_type != target) {
    XFree(data);
    return 0;
  }
  return data;
}
static char *
sys_x11_clip_get_str(struct sys *sys) {
  assert(sys);
  if (XGetSelectionOwner(g_x11.dpy, g_x11.xa_clip) == g_x11.helper) {
    /* Don't do any round trips. Instead just provide data */
    if (!g_x11.clip.data) {
      return 0;
    }
    char *str = xmalloc(g_x11.clip.len + 1);
    strcpy(str, g_x11.clip.data);
    return str;
  }
  const Atom targets[] = {g_x11.xa_utf_str, XA_STRING};
  for (int i = 0; i < cntof(targets); ++i) {
    char *data = sys_x11_clip_get(sys, g_x11.xa_clip, targets[i]);
    if (!data) {
      continue;
    }
    char *str = xmalloc(cast(int, strlen(data) + 1));
    strcpy(str, data);
    XFree(data);
    return str;
  }
  return 0;
}
#if 0
static void
sys_x11_clipboard_set(struct str s, struct arena *a) {
  unused(a);
  sys_x11_clip_set_str(s.str, s.len);
}
static struct str
sys_x11_clipboard_get(struct arena *a) {
  struct str ret;
  ret = arena_str(a, &sys, str(x11.clip.data, x11.clip.len));
  return ret;
}
#endif

/* ---------------------------------------------------------------------------
 *                                  Events
 * ---------------------------------------------------------------------------
 */
static void
sys_x11_on_win_resize(struct sys_x11_win *xwin) {
#if 0
  int w = xwin->attr.width;
  int h = xwin->attr.height;
#endif
  XGetWindowAttributes(g_x11.dpy, xwin->window, &xwin->attr);
}
static void
sys_x11_on_win_expose(struct sys_x11_win *xwin, XEvent *evt) {
#if 0
  int w = xwin->attr.width;
  int h = xwin->attr.height;
#endif
  XGetWindowAttributes(g_x11.dpy, xwin->window, &xwin->attr);
}
static void
sys_x11_btn(struct sys_btn *btn, int down) {
  assert(btn);
  int was_down = btn->down;
  btn->down = down ? 1u : 0u;
  btn->pressed = !was_down && down;
  btn->released = was_down && !down;
  btn->doubled = 0;

  if (down) {
    /* Double-Click Button handler */
    unsigned long long dt = sys_x11_timestamp() - btn->timestamp;
    if (dt > SYS_DOUBLE_CLICK_LO && dt < SYS_DOUBLE_CLICK_HI) {
      btn->doubled = True;
    } else {
      btn->timestamp = sys_x11_timestamp();
    }
  }
}
static unsigned
sys_x11_keymod_map(unsigned keymod) {
  unsigned ret = 0;
  if (keymod & ControlMask) {
    ret |= SYS_KEYMOD_CTRL;
  }
  if (keymod & ShiftMask) {
    ret |= SYS_KEYMOD_SHIFT;
  }
  if (keymod & Mod1Mask) {
    ret |= SYS_KEYMOD_ALT;
  }
  return ret;
}
static void
sys_x11_on_btn(struct sys_x11_win *xwin, XEvent *evt, int down) {
  if (evt->xbutton.button == Button4 ||
      evt->xbutton.button == Button5) {
    /* mouse scrolling */
    if (evt->xbutton.button == Button4 && down) {
      g_sys.mouse.scrl[1] += 1;
      g_sys.btn_mod = 1;
    } else if (evt->xbutton.button == Button5 && down) {
      g_sys.mouse.scrl[1] -= 1;
      g_sys.btn_mod = 1;
    }
    return;
  }
  if (down) {
    /* grab/ungrap mouse pointer */
    if (g_x11.grab_cnt++ == 0) {
      XGrabPointer(g_x11.dpy, xwin->window, False,
          PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
          GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    }
  } else {
    if (--g_x11.grab_cnt == 0) {
      XUngrabPointer(g_x11.dpy, CurrentTime);
    }
  }
  if (evt->xbutton.button == Button1) {
    sys_x11_btn(&g_sys.mouse.btn.left, down);
  } else if (evt->xbutton.button == Button2) {
    sys_x11_btn(&g_sys.mouse.btn.middle, down);
  } else if (evt->xbutton.button == Button3) {
    sys_x11_btn(&g_sys.mouse.btn.right, down);
  }
  g_sys.mouse.pos[0] = evt->xbutton.x;
  g_sys.mouse.pos[1] = evt->xbutton.y;
  g_sys.mouse_mod = 1;

  g_sys.keymod = sys_x11_keymod_map(evt->xbutton.state);
  g_sys.btn_mod = 1;
}
static void
sys_x11_on_motion(XEvent *evt) {
  g_sys.mouse.pos[0] = evt->xmotion.x;
  g_sys.mouse.pos[1] = evt->xmotion.y;
  g_sys.mouse.pos_delta[0] = g_sys.mouse.pos[0] - g_sys.mouse.pos_last[0];
  g_sys.mouse.pos_delta[1] = g_sys.mouse.pos[1] - g_sys.mouse.pos_last[1];
  g_sys.keymod = sys_x11_keymod_map(evt->xmotion.state);
  g_sys.mouse_mod = 1;
}
static void
sys_on_key(int key) {
  bit_set(g_sys.keys, key);
  g_sys.key_mod = 1;
}
static void
sys_x11_on_key(XEvent *evt) {
  int ret = 0;
  KeySym *code = XGetKeyboardMapping(g_x11.dpy, (KeyCode)evt->xkey.keycode, 1, &ret);
  if (*code > 0 && *code < SYS_KEY_END) {
    bit_set(g_sys.keys, casti(*code));
  }
  g_sys.keymod = sys_x11_keymod_map(evt->xkey.state);

  char buf[32] = {0};
  KeySym keysym = 0;
  int len = XLookupString((XKeyEvent *)evt, buf, 32, &keysym, NULL);
  if (keysym == XK_BackSpace) {
    sys_on_key(SYS_KEY_BACKSPACE);
  } else if (keysym == XK_Delete) {
    sys_on_key(SYS_KEY_DEL);
  } else if (keysym == XK_Caps_Lock) {
    sys_on_key(SYS_KEY_CAPS);
  } else if (keysym == XK_Return) {
    sys_on_key(SYS_KEY_RETURN);
  } else if (keysym == XK_Tab) {
    sys_on_key(SYS_KEY_TAB);
  } else if (keysym == XK_Left) {
    sys_on_key(SYS_KEY_LEFT);
  } else if (keysym == XK_Right) {
    sys_on_key(SYS_KEY_RIGHT);
  } else if (keysym == XK_Up) {
    sys_on_key(SYS_KEY_UP);
  } else if (keysym == XK_Down) {
    sys_on_key(SYS_KEY_DOWN);
  } else if (keysym == XK_Page_Down) {
    sys_on_key(SYS_KEY_PGDN);
  } else if (keysym == XK_Page_Up) {
    sys_on_key(SYS_KEY_PGUP);
  } else if (keysym == XK_Home) {
    sys_on_key(SYS_KEY_HOME);
  } else if (keysym == XK_KP_Add) {
    sys_on_key(SYS_KEY_PLUS);
  } else if (keysym == XK_KP_Subtract) {
    sys_on_key(SYS_KEY_MINUS);
  } else if (keysym == XK_End) {
    sys_on_key(SYS_KEY_END);
  } else if (keysym == XK_Escape) {
    sys_on_key(SYS_KEY_ESCAPE);
  } else if (*code == XK_space) {
    sys_on_key(SYS_KEY_SPACE);
    g_sys.txt[g_sys.txt_len] = ' ';
    g_sys.txt_len += 1;
  } else if (len != NoSymbol) {
    if (!g_sys.keymod) {
      memcpy(g_sys.txt + g_sys.txt_len, buf, castsz(len));
      g_sys.txt_len += len;
    } else if (isalpha(buf[0])) {
      g_sys.txt[g_sys.txt_len++] = cast(char, toupper(buf[0]));
    }
  }
  XFree(code);
}
static void
sys_quit(void) {
  g_sys.quit = True;
  g_sys.op = SYS_QUIT;
}
static void
sys_x11_on_client_msg(XEvent *evt) {
  if (evt->xclient.message_type == g_x11.xa_wm_protocols) {
    /* handle window close message */
    Atom protocol = cast(Atom, evt->xclient.data.l[0]);
    if (protocol == g_x11.xa_wm_del_win) {
      sys_quit();
    }
  }
}
static void
sys_x11_on_sel_req(XEvent *evt) {
  XEvent reply = {0};
  reply.xselection.type = SelectionNotify;
  reply.xselection.requestor = evt->xselectionrequest.requestor;
  reply.xselection.selection = evt->xselectionrequest.selection;
  reply.xselection.target = evt->xselectionrequest.target;
  reply.xselection.time = evt->xselectionrequest.time;
  reply.xselection.property = None;

  if (g_x11.clip.data) {
    if (reply.xselection.target == g_x11.xa_tar) {
      /* handle request for supported types */
      int tar_cnt = 3;
      Atom tar_list[4] = {0};
      tar_list[0] = g_x11.xa_tar;
      tar_list[1] = g_x11.xa_utf_str;
      tar_list[2] = XA_STRING;

      reply.xselection.property = evt->xselectionrequest.property;
      XChangeProperty(evt->xselection.display, evt->xselectionrequest.requestor,
        reply.xselection.property, XA_ATOM, 32, PropModeReplace,
        (unsigned char *)&tar_list, tar_cnt);
    } else if (sys_x11_clip_supports_type(reply.xselection.target)) {
      /* provide access to this apps clipboard data */
      reply.xselection.property = evt->xselectionrequest.property;
      XChangeProperty(evt->xselection.display, evt->xselectionrequest.requestor,
        reply.xselection.property, reply.xselection.target, 8, PropModeReplace,
        (unsigned char *)g_x11.clip.data, g_x11.clip.len);
    }
  }
  XSendEvent(evt->xselection.display, evt->xselectionrequest.requestor, True, 0, &reply);
  XFlush(evt->xselection.display);
}
extern int
main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  XSetLocaleModifiers("@im=none");
  XrmInitialize();

  g_x11.dpy = XOpenDisplay(0);
  if (!g_x11.dpy) {
    xpanic("Could not open a display; perhaps $DISPLAY is not set?");
  }
  /* create x11 resources */
  g_x11.root = DefaultRootWindow(g_x11.dpy);
  g_x11.screen = XDefaultScreen(g_x11.dpy);
  g_x11.vis = XDefaultVisual(g_x11.dpy, g_x11.screen);
  g_x11.cmap = XCreateColormap(g_x11.dpy, g_x11.root, g_x11.vis, AllocNone);

  /* lookup atoms */
  g_x11.xa_wm_protocols = XInternAtom(g_x11.dpy, "WM_PROTOCOLS", False);
  g_x11.xa_wm_del_win = XInternAtom(g_x11.dpy, "WM_DELETE_WINDOW", False);
  g_x11.xa_txt_uri_lst = XInternAtom(g_x11.dpy, "text/uri-list", False);
  g_x11.xa_clip = XInternAtom(g_x11.dpy, "CLIPBOARD", False);
  g_x11.xa_tar = XInternAtom(g_x11.dpy, "TARGETS", False);
  g_x11.xa_txt = XInternAtom(g_x11.dpy, "TEXT", False);
  g_x11.xa_utf_str = XInternAtom(g_x11.dpy, "UTF8_STRING", False);
  g_x11.xa_wm_hints = XInternAtom(g_x11.dpy, "_MOTIF_WM_HINTS", False);
  g_x11.xa_sys_tray = XInternAtom(g_x11.dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
  g_x11.xa_wm_icon = XInternAtom(g_x11.dpy, "_NET_WM_ICON", False);

  g_x11.xa_win_type = XInternAtom(g_x11.dpy, "_NET_WM_WINDOW_TYPE", False);
  g_x11.xa_wm_desk = XInternAtom(g_x11.dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
  g_x11.xa_wm_dock = XInternAtom(g_x11.dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
  g_x11.xa_wm_tool = XInternAtom(g_x11.dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
  g_x11.xa_wm_menu = XInternAtom(g_x11.dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
  g_x11.xa_wm_util = XInternAtom(g_x11.dpy, "_NET_WM_WINDOW_TYPE_UTILITY", False);
  g_x11.xa_wm_splsh = XInternAtom(g_x11.dpy, "_NET_WM_WINDOW_TYPE_SPLASH", False);
  g_x11.xa_wm_pop = XInternAtom(g_x11.dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
  g_x11.xa_wm_norm = XInternAtom(g_x11.dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False);

  g_x11.xa_wm_st = XInternAtom(g_x11.dpy, "_NET_WM_STATE", False);
  g_x11.xa_wm_st_maximized_horz = XInternAtom(g_x11.dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  g_x11.xa_wm_st_maximized_vert = XInternAtom(g_x11.dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);

  /* create cursors */
  static const unsigned x11_cur_map[] = {
    [SYS_CUR_DEFAULT] = XC_left_ptr,
    [SYS_CUR_NO] = XC_X_cursor,
    [SYS_CUR_CROSS] = XC_crosshair,
    [SYS_CUR_HAND] = XC_hand2,
    [SYS_CUR_IBEAM] = XC_xterm,
    [SYS_CUR_MOVE] = XC_fleur,
    [SYS_CUR_SIZE_NS] = XC_sb_v_double_arrow,
    [SYS_CUR_SIZE_WE] = XC_sb_h_double_arrow,
  };
  for (int i = 0; i < cntof(x11_cur_map); ++i) {
    g_x11.cursors[i] = XCreateFontCursor(g_x11.dpy, x11_cur_map[i]);
  }
  /* create helper window */
  g_x11.helper = XCreateSimpleWindow(g_x11.dpy, g_x11.root, -10, -10, 1, 1, 0, 0, 0);
  XSelectInput(g_x11.dpy, g_x11.helper, SelectionNotify);

  g_sys.quit = False;
  g_x11.fd = ConnectionNumber(g_x11.dpy);

  g_sys.app = 0;
  g_sys.argc = argc;
  g_sys.argv = argv;
  g_sys.platform = &g_x11;
  cpu_info(&g_sys.cpu);

  /* api */
  g_sys.dir.lst = sys_dir_lst;
  g_sys.dir.nxt = sys_dir_nxt;
  g_sys.dir.exists = sys_dir_exists;
  g_sys.file.info = sys_file_info;
  g_sys.time.timestamp = sys_x11_timestamp;

  g_sys.con.log = sys__x11_log;
  g_sys.con.warn = sys__x11_warn;
  g_sys.con.err = sys__x11_err;

  g_sys.rnd.open = sys__rnd_open;
  g_sys.rnd.close = sys__rnd_close;
  g_sys.rnd.gen32 = sys__rnd_gen32;
  g_sys.rnd.gen64 = sys__rnd_gen64;
  g_sys.rnd.gen128 = sys__rnd_gen128;

  /* constants */
  g_x11.dpi_scale[0] = 1.0f;
  g_x11.dpi_scale[1] = 1.0f;
  sys__x11_dpi_scale(g_x11.dpi_scale);

  g_sys.op = SYS_SETUP;
  app_run(&g_sys);

  /* create window */
  struct sys_x11_win *xwin = &g_x11.win;
  memset(xwin, 0, sizeof(*xwin));
  xwin->swa.bit_gravity = StaticGravity;
  xwin->swa.background_pixel = 0;
  xwin->swa.colormap = g_x11.cmap;
  xwin->w = g_sys.win.w;
  xwin->h = g_sys.win.h;

  unsigned long attrmsk = CWColormap | CWBackPixel | CWEventMask | CWBitGravity;
  xwin->swa.border_pixel = 0;
  xwin->swa.background_pixel = 0;
  xwin->swa.event_mask = ExposureMask | KeyPressMask | Mod1Mask |
     PointerMotionMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
     KeymapStateMask | StructureNotifyMask;
  xwin->window = XCreateWindow(g_x11.dpy, g_x11.root, 0, 0, castu(xwin->w),
    castu(xwin->h), 0, XDefaultDepth(g_x11.dpy, g_x11.screen), InputOutput,
    g_x11.vis, attrmsk, &xwin->swa);

  xwin->gc = XCreateGC(g_x11.dpy, xwin->window, 0, 0);
  XSetWMProtocols(g_x11.dpy, xwin->window, &g_x11.xa_wm_del_win, 1);
  XMapWindow(g_x11.dpy, xwin->window);
  XGetWindowAttributes(g_x11.dpy, xwin->window, &xwin->attr);
  XFlush(g_x11.dpy);

  XChangeProperty(g_x11.dpy, xwin->window, g_x11.xa_win_type, XA_ATOM, 32,
    PropModeReplace, (unsigned char *)&g_x11.xa_wm_pop, 1);
  XSetTransientForHint(g_x11.dpy, xwin->window, g_x11.root);

  /* init gfx */
  struct gfx_param gfx;
  gfx.dpy = g_x11.dpy;
  gfx.win = xwin->window;

  gfx_api(&g_sys.gfx, 0);
  g_sys.ren = &g_x11.vk;
  g_sys.gfx.init(&g_sys, cast(void*,&gfx));

  g_sys.op = SYS_INIT;
  app_run(&g_sys);
  g_sys.op = SYS_RUN;

  /* run application */
  while (!g_sys.quit) {
    /* Handle window events */
    int skip = True;
    XEvent evt = {0};
    while (XCheckWindowEvent(g_x11.dpy, xwin->window, xwin->swa.event_mask, &evt)) {
      switch (evt.type) {
      default: break;
      case NoExpose: break;
      case ConfigureNotify:
        sys_x11_on_win_resize(xwin);
        skip = False;
        break;
      case Expose:
        sys_x11_on_win_expose(xwin, &evt);
        skip = False;
        break;
      case ButtonPress:
      case ButtonRelease:
        sys_x11_on_btn(xwin, &evt, evt.type == ButtonPress);
        skip = False;
        break;
      case MotionNotify:
        sys_x11_on_motion(&evt);
        skip = False;
        break;
      case KeymapNotify:
        XRefreshKeyboardMapping(&evt.xmapping);
        break;
      case KeyPress:
        sys_x11_on_key(&evt);
        skip = False;
        break;
      }
    }
    /* Handle ClientMessages */
    while (XPending(g_x11.dpy)) {
      XNextEvent(g_x11.dpy, &evt);
      switch (evt.type) {
        case SelectionNotify: break;
        case ClientMessage:
          sys_x11_on_client_msg(&evt);
          skip = False;
          break;
        case SelectionRequest:
          sys_x11_on_sel_req(&evt);
          skip = False;
          break;
        case SelectionClear:
          sys_x11_clip_clear();
          skip = False;
          break;
      }
    }
    g_sys.txt_mod = !!g_sys.txt_len;
    g_sys.win.w = g_x11.win.w;
    g_sys.win.h = g_x11.win.h;
    if (!skip) {
      app_run(&g_sys);
    }
    /* cleanup */
    g_sys.seq++;
    g_sys.style_mod = 0;
    g_sys.keymod = 0;
    g_sys.txt_len = 0;
    g_sys.focus = 0;

    g_sys.btn_mod = 0;
    g_sys.txt_mod = 0;
    g_sys.key_mod = 0;
    g_sys.scrl_mod = 0;

    g_sys.mouse_mod = 0;
    g_sys.mouse.pos_last[0] = g_sys.mouse.pos[0];
    g_sys.mouse.pos_last[1] = g_sys.mouse.pos[1];

    for (int i = 0; i < cntof(g_sys.keys); ++i){
      g_sys.keys[i] = 0;
    }
    for (int i = 0; i < SYS_MOUSE_BTN_CNT; ++i) {
      g_sys.mouse.btns[i].pressed = 0;
      g_sys.mouse.btns[i].released = 0;
      g_sys.mouse.btns[i].doubled = 0;
    }

    /* handle cursor changes */
    if (g_sys.cursor != g_x11.cursor) {
      g_x11.cursor = g_sys.cursor;
      XUndefineCursor(g_x11.dpy, g_x11.win.window);
      if (g_sys.cursor != SYS_CUR_ARROW) {
        XDefineCursor(g_x11.dpy, g_x11.win.window, g_x11.cursors[g_sys.cursor]);
      }
    }
    XFlush(g_x11.dpy);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(g_x11.fd, &fds);
    pselect(g_x11.fd + 1, &fds, 0, 0, 0, 0);
  }
  g_sys.gfx.shutdown(&g_sys);

  /* ungrab all global key shortcuts */
  KeyCode keycode = XKeysymToKeycode(g_x11.dpy, XK_p);
  XUngrabKey(g_x11.dpy, keycode, Mod1Mask, g_x11.root);

  /* make sure to ungrab mouse */
  XUngrabPointer(g_x11.dpy, CurrentTime);
  XUngrabKeyboard(g_x11.dpy, CurrentTime);

  /* unmap/destroy window */
  XUnmapWindow(g_x11.dpy, xwin->window);
  XDestroyWindow(g_x11.dpy, xwin->window);

  /* clear x11 resources */
  for (int i = 0; i < SYS_CUR_CNT; ++i) {
    XFreeCursor(g_x11.dpy, g_x11.cursors[i]);
  }
  free(g_x11.clip.data);

  XDestroyWindow(g_x11.dpy, g_x11.helper);
  XFreeColormap(g_x11.dpy, g_x11.cmap);
  XCloseDisplay(g_x11.dpy);
  return 0;
}
