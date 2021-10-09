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

/* x11 */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XShm.h>

/* usr */
#include "cpu.h"
#include "fmt.h"
#include "fmt.c"
#include "std.h"
#include "dbg.h"
#include "ren.h"
#include "sys.h"
#include "std.c"

#ifndef SYS_DOUBLE_CLICK_LO
#define SYS_DOUBLE_CLICK_LO 20000000lu
#endif
#ifndef SYS_DOUBLE_CLICK_HI
#define SYS_DOUBLE_CLICK_HI 300000000llu
#endif

struct sys_mem_blk {
  struct mem_blk blk;
  struct lst_elm hook;
  unsigned long long tags;
};
struct sys_x11_module {
  unsigned valid : 1;
  struct str path;
  void *lib;
  ino_t fileId;

  int sym_cnt;
  char **sym_names;
  void **syms;
  void(*dlExport)(void *export, void *import);
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
struct sys_x11_fb {
  XShmSegmentInfo shm_info;
  XImage *ximg;
  unsigned *data;
};
struct sys_x11_win {
  int w,h;
  GC gc;
  Window window;
  XWindowAttributes attr;
  XSetWindowAttributes swa;
  struct sys_mouse mouse;
  struct sys_x11_fb fb;
};
struct sys_x11_clipboard {
  char *data;
  int len;
};
struct sys_x11 {
  Display *dpy;
  Window root;
  Visual *vis;
  Colormap cmap;
  Window helper;
  int screen;
  int fd;

  int quit;
  struct lst_elm mem_blks;
  struct lck mem_lck;
  int col_mod;

  struct arena mem;
  struct arena tmp;
  struct str exe_path;
  struct sys_x11_win win;
  enum sys_cur_style cursor;
  struct sys_x11_clipboard clip;
  int grab_cnt;

  /* modules */
  struct str dbg_path;
  struct sys_dbg_sym_table dbg;
  struct sys_x11_module dbg_lib;

  struct str ren_path;
  struct sys_ren_sym_table ren;
  struct sys_x11_module ren_lib;

  struct str app_path;
  struct sys_app_sym_table app;
  struct sys_x11_module app_lib;

  int mod_cnt;
  #define SYS_X11_MAX_MODS 32
  struct sys_x11_module mods[SYS_X11_MAX_MODS];

  /* cursor */
  Cursor cursors[SYS_CUR_CNT];

  /* |-------- atoms --------- |*/
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
static struct sys sys;
static struct sys_x11 x11;

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

/* ---------------------------------------------------------------------------
 *
 *                                    File
 *
 * ---------------------------------------------------------------------------
 */
static struct str
sys_x11_get_exe_path(struct arena *a) {
  struct stat sb;
  static const char *exe_path = "/proc/self/exe";
  if (lstat(exe_path, &sb) == -1) {
   xpanic("Failed to get executable path!\n");
  }
  ssize_t bufsiz = sb.st_size + 1;
  if (sb.st_size == 0){
    bufsiz = PATH_MAX;
  }
  char *buf = arena_alloc(a, &sys, cast(int, bufsiz));
  ssize_t nbytes = readlink(exe_path, buf, cast(size_t, bufsiz));
  if (nbytes == -1) {
    xpanic("Failed to get executable path!\n");
  }
  return str(buf, cast(int, nbytes));
}
static struct str
sys_x11_get_exe_file_path(struct str exe_path, struct str file,
                          struct str suffix, struct arena *a) {
  struct str exe_file = path_file(exe_path);
  struct str path = strp(exe_path.str, exe_file.str);
  return arena_fmt(a, &sys, "%.*s%.*s%.*s", strf(path), strf(file), strf(suffix));
}
static void
sys_x11_file_write(int fd, const char *buf, int cnt) {
  const char *p = buf;
  ssize_t out = 0;
  do {
    out = write(fd, p, cast(size_t, cnt));
    if (out >= 0) {
      cnt -= out;
      p += out;
    } else if (errno != EINTR) {
      xpanic("Could not copy shared library while loading");
    }
  } while (cnt > 0);
}
static void
sys_mac_file_cpy(int fd, int old_fd) {
  char buf[4096];
  ssize_t cnt = read(old_fd, buf, sizeof(buf));
  while (cnt > 0) {
    sys_x11_file_write(fd, buf, cast(int, cnt));
    cnt = read(fd, buf, sizeof(buf));
  }
  if (cnt != 0) {
    xpanic("Could not copy whole shared library while loading.");
  }
}


/* ---------------------------------------------------------------------------
 *
 *                                  Directory
 *
 * ---------------------------------------------------------------------------
 */
static void
sys__dir_free(struct sys_dir_iter *it, struct arena *a) {
  scope_end(&it->scp_base, a, &sys);
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
  char *fullpath = arena_cstr(tmp, &sys, path);
  int res = stat(fullpath, &stats);
  scope_end(&scp, tmp, &sys);
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
  scope_end(&it->scp, a, &sys);
  scope_begin(&it->scp, a);
  do {
    struct dirent *ent = readdir(it->handle);
    if (!ent) {
      scope_end(&it->scp, a, &sys);
      sys__dir_free(it, a);
      return;
    }
    it->fullpath = arena_fmt(a, &sys, "%.*s/%s", strf(it->base), ent->d_name);
    it->name = str0(ent->d_name);
    it->isdir = ent->d_type & DT_DIR;
  } while (sys__dir_excl(it));
}
static void
sys_dir_lst(struct sys_dir_iter *it, struct arena *a, struct str path) {
  memset(it, 0, sizeof(*it));
  scope_begin(&it->scp_base, a);
  it->base = arena_str(a, &sys, path);

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
sys_x11_mem_alloc(struct mem_blk* opt_old, int siz, unsigned flags,
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

      lck_acq(&x11.mem_lck);
      lst_del(&sys_blk->hook);
      lck_rel(&x11.mem_lck);
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

  lck_acq(&x11.mem_lck);
  lst_add(&x11.mem_blks, &blk->hook);
  lck_rel(&x11.mem_lck);
  return &blk->blk;
}
static void
sys_x11_mem_free(struct mem_blk *mem_blk) {
  struct sys_mem_blk *blk;
  blk = containerof(mem_blk, struct sys_mem_blk, blk);
  int total = blk->blk.size + szof(struct sys_mem_blk);

  lck_acq(&x11.mem_lck);
  lst_del(&blk->hook);
  lck_rel(&x11.mem_lck);

  if (mem_blk->flags & SYS_MEM_GROWABLE) {
    free(blk);
  } else {
    size_t unmapsiz = cast(size_t, total);
    munmap(blk, unmapsiz);
  }
}
static void
sys_x11_mem_stats(struct sys_mem_stats *stats) {
  memset(stats, 0, sizeof(*stats));
  lck_acq(&x11.mem_lck);
  {
    struct lst_elm *elm = 0;
    for_lst(elm, &x11.mem_blks) {
      struct sys_mem_blk *blk;
      blk = lst_get(elm, struct sys_mem_blk, hook);
      stats->total += blk->blk.size;
      stats->used += blk->blk.used;
    }
  }
  lck_rel(&x11.mem_lck);
}
static void
sys_x11_mem_free_tag(unsigned long long tag) {
  lck_acq(&x11.mem_lck);
  struct lst_elm *elm = 0;
  for_lst(elm, &x11.mem_blks) {
    struct sys_mem_blk *blk = 0;
    blk = lst_get(elm, struct sys_mem_blk, hook);
    if (blk->tags == tag) {
      sys_x11_mem_free(&blk->blk);
    }
  }
  lck_rel(&x11.mem_lck);
}

/* ---------------------------------------------------------------------------
 *
 *                            Dynamic Library
 *
 * ---------------------------------------------------------------------------
 */
static void *
sys_x11_lib_sym(void *lib, const char *s) {
  void *sym = dlsym(lib, s);
  if (!sym) {
    fprintf(stderr, "failed to load dynamic library symbol: %s\n", dlerror());
  }
  return sym;
}
static void *
sys_x11_lib_open(const char *path) {
  void *lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!lib) {
    fprintf(stderr, "failed to open dynamic library: %s\n", dlerror());
  }
  return lib;
}
static void
sys_x11_lib_close(void *lib) {
  if (lib != 0) {
    dlclose(lib);
  }
}
static ino_t
sys_x11_file_id(struct str path) {
  struct stat attr = {0};
  if (stat(path.str, &attr)) {
    attr.st_ino = 0;
  }
  return attr.st_ino;
}
static void
sys_x11_mod_close(struct sys_x11_module *mod) {
  if (mod->lib) {
    sys_x11_lib_close(mod->lib);
    mod->lib = 0;
  }
  mod->fileId = 0;
  mod->valid = 0;
}
static void
sys_x11_mod_open(struct sys_x11_module *mod, struct arena *mem) {
  ino_t fileID = sys_x11_file_id(mod->path);
  if (mod->fileId == fileID) {
    if(!mod->valid) {
      sys_x11_mod_close(mod);
    }
    return;
  }
  struct scope scp = {0};
  scope_begin(&scp, mem);
  {
    mod->fileId = fileID;
    mod->valid = 1;
    mod->lib = sys_x11_lib_open(mod->path.str);
    if (mod->lib) {
      for (int i = 0; i < mod->sym_cnt; ++i) {
        void *sym = sys_x11_lib_sym(mod->lib, mod->sym_names[i]);
        if (sym) {
          mod->syms[i] = sym;
        } else {
          mod->valid = 0;
        }
      }
    }
    if(!mod->valid) {
      sys_x11_mod_close(mod);
    }
  }
  scope_end(&scp, mem, &sys);
}
static int
sys_x11_mod_chk(struct sys_x11_module *mod) {
  ino_t libId = sys_x11_file_id(mod->path);
  return libId != mod->fileId;
}
static void
sys_x11_mod_reload(struct sys_x11_module *mod, struct arena *a) {
  sys_x11_mod_close(mod);
  for(int i = 0; !mod->valid && i < 100; ++i) {
    sys_x11_mod_open(mod, a);
    usleep(100000);
  }
}

/* ---------------------------------------------------------------------------
 *
 *                                Clipboard
 *
 * ---------------------------------------------------------------------------
 */
static int
sys_x11_clip_supports_type(Atom type) {
  if (type == x11.xa_txt) {
    return True;
  }
  if (type == x11.xa_utf_str) {
    return True;
  }
  if (type == XA_STRING) {
    return True;
  }
  return False;
}
static void
sys_x11_clip_clear(void) {
  free(x11.clip.data);
  x11.clip.data = 0;
  x11.clip.len = 0;
}
static void
sys_x11_clip_set_str(const char *str, int len) {
  free(x11.clip.data);
  x11.clip.len = len;
  x11.clip.data = xmalloc(len + 1);
  strcpy(x11.clip.data, str);

  XSetSelectionOwner(x11.dpy, XA_PRIMARY, x11.helper, CurrentTime);
  XSetSelectionOwner(x11.dpy, x11.xa_clip, x11.helper, CurrentTime);
}
static char *
sys_x11_clip_get(struct sys *s, Atom selection, Atom target) {
  assert(s);
  XConvertSelection(x11.dpy, selection, target, selection, x11.helper,
                    CurrentTime);
  /* blocking wait for clipboard data */
  XEvent e;
  while (!XCheckTypedWindowEvent(x11.dpy, x11.helper, SelectionNotify, &e)) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(x11.fd, &fds);
    if (select(x11.fd + 1, &fds, 0, 0, 0) == -1 || errno == EINTR) return 0;
  }
  if (e.xselection.property == None) {
    return 0;
  }
  char *data = 0;
  int actual_fmt;
  Atom actual_type;
  unsigned long item_cnt;
  unsigned long bytes_after;

  /* get clipboard data out of window property */
  XGetWindowProperty(x11.dpy, e.xselection.requestor, e.xselection.property, 0,
                     LONG_MAX, True, AnyPropertyType, &actual_type, &actual_fmt,
                     &item_cnt, &bytes_after, (unsigned char **)&data);
  if (actual_type != target) {
    XFree(data);
    return 0;
  }
  return data;
}
static char *
sys_x11_clip_get_str(struct sys *s) {
  assert(s);
  if (XGetSelectionOwner(x11.dpy, x11.xa_clip) == x11.helper) {
    /* Don't do any round trips. Instead just provide data */
    if (!x11.clip.data) {
      return 0;
    }
    char *str = xmalloc(x11.clip.len + 1);
    strcpy(str, x11.clip.data);
    return str;
  }
  const Atom targets[] = {x11.xa_utf_str, XA_STRING};
  for (int i = 0; i < cntof(targets); ++i) {
    char *data = sys_x11_clip_get(s, x11.xa_clip, targets[i]);
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

/* ---------------------------------------------------------------------------
 *
 *                              Frame Buffer
 *
 * ---------------------------------------------------------------------------
 */
static unsigned *
sys_x11_fb_new(struct sys_x11_fb *fb, int w, int h) {
  const size_t siz = cast(size_t, szof(unsigned) * (w * h + 2));
  fb->shm_info.shmid = shmget(IPC_PRIVATE, siz, IPC_CREAT | 0777);
  if (fb->shm_info.shmid < 0) {
    xpanic("[sys]: failed to allocate framebuffer");
  }
  fb->shm_info.shmaddr = (char *)shmat(fb->shm_info.shmid, 0, 0);
  if (fb->shm_info.shmaddr == (char *)-1) {
    xpanic("[sys]: failed to map framebuffer into address space\n");
  }
  fb->shm_info.readOnly = 0;

  /* attach X Server to shared memory segment */
  XShmAttach(x11.dpy, &fb->shm_info);
  XSync(x11.dpy, 0);
  shmctl(fb->shm_info.shmid, IPC_RMID, 0);

  fb->ximg = XShmCreateImage(x11.dpy, x11.vis,
                             (unsigned)XDefaultDepth(x11.dpy, x11.screen),
                             ZPixmap, fb->shm_info.shmaddr, &fb->shm_info,
                             cast(unsigned, w), cast(unsigned, h));
  if (!fb->ximg) {
    XShmDetach(x11.dpy, &fb->shm_info);
    XSync(x11.dpy, 0);
    shmdt(fb->shm_info.shmaddr);
    xpanic("[sys]: could not create XImage framebuffer\n");
  }
  fb->data = cast(unsigned *, (void *)fb->shm_info.shmaddr);

  unsigned *img = fb->data + 2u;
  img_w(img) = cast(unsigned, w);
  img_h(img) = cast(unsigned, h);
  return img;
}
static void
sys_x11_fb_del(struct sys_x11_fb *fb) {
  if (!fb->ximg) {
    return;
  }
  XShmDetach(x11.dpy, &fb->shm_info);
  XDestroyImage(fb->ximg);
  fb->ximg = 0;

  shmdt(fb->shm_info.shmaddr);
  memset(&fb->shm_info, 0, sizeof(fb->shm_info));
}
static unsigned *
sys_x11_fb_resize(struct sys_x11_fb *fb, int w, int h) {
  sys_x11_fb_del(fb);
  return sys_x11_fb_new(fb, w, h);
}

/* ---------------------------------------------------------------------------
 *
 *                                  Util
 *
 * ---------------------------------------------------------------------------
 */
static void
sys__x11_dpi_scale(float *scale) {
  const int dpyw = DisplayWidth(x11.dpy, x11.screen);
  const int dpyh = DisplayHeight(x11.dpy, x11.screen);
  const int dpywmm = DisplayWidthMM(x11.dpy, x11.screen);
  const int dpyhmm = DisplayHeightMM(x11.dpy, x11.screen);

  float xdpi = cast(float, dpyw) * 25.4f / cast(float, dpywmm);
  float ydpi = cast(float, dpyh) * 25.4f / cast(float, dpyhmm);

  char *rms = XResourceManagerString(x11.dpy);
  if (rms) {
    XrmDatabase db = XrmGetStringDatabase(rms);
    if (db) {
      XrmValue value;
      char *type = 0, *ep = 0;
      if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value)) {
        if (type && strcmp(type, "String") == 0) {
          float dpi = strtof(value.addr, &ep);
          if (*ep == '\0' && ep != value.addr) xdpi = ydpi = dpi;
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
static int
sys_x11_plugin_add(void *exp, void *imp, struct str name) {
  assert(x11.mod_cnt < SYS_X11_MAX_MODS);
  int at = x11.mod_cnt++;
  struct sys_x11_module *mod = x11.mods + at;

  static char *sys_module_fn_sym[] = {"dlExport"};
  mod->path = sys_x11_get_exe_file_path(x11.exe_path, name, strv(".so"), &x11.mem);
  mod->sym_cnt = cntof(sys_module_fn_sym);
  mod->sym_names = sys_module_fn_sym;
  mod->syms = (void**)&mod->dlExport;
  sys_x11_mod_open(mod, &x11.tmp);
  if (mod->dlExport) {
    mod->dlExport(exp, imp);
  }
  return mod->valid;
}

/* ---------------------------------------------------------------------------
 *
 *                                  Events
 *
 * ---------------------------------------------------------------------------
 */
static void
sys_x11_on_win_resize(struct sys_x11_win *xwin) {
  int w = xwin->attr.width;
  int h = xwin->attr.height;
  XGetWindowAttributes(x11.dpy, xwin->window, &xwin->attr);
  if (xwin->attr.width != w || xwin->attr.height != h) {
    sys.ren_target.pixels = sys_x11_fb_resize(&xwin->fb, xwin->attr.width,
                                         xwin->attr.height);
  } else if (sys.ren_target.pixels) {
    XShmPutImage(x11.dpy, xwin->window, xwin->gc, xwin->fb.ximg, 0, 0, 0,
                 0, (unsigned)w, (unsigned)h, False);
  }
}
static void
sys_x11_on_win_expose(struct sys_x11_win *xwin, XEvent *e) {
  if (!sys.ren_target.pixels) {
    return;
  }
  int dw = cast(int, img_w(sys.ren_target.pixels));
  int dh = cast(int, img_h(sys.ren_target.pixels));

  int x = e->xexpose.x;
  int y = e->xexpose.y;
  int w = min(dw, e->xexpose.width);
  int h = min(dh, e->xexpose.height);

  if (x < 0) x += w, w += x;
  if (y < 0) y += h, h += y;
  if (x + w > dw) w = max(0, dw - x);
  if (y + h > dh) h = max(0, dh - y);
  XShmPutImage(x11.dpy, xwin->window, xwin->gc, xwin->fb.ximg, x, y, x,
               y, (unsigned)w, (unsigned)h, False);
}
static void
sys_x11_btn(struct sys_btn *b, int down) {
  assert(b);
  int was_down = b->down;
  b->down = down ? 1u : 0u;
  b->pressed = !was_down && down;
  b->released = was_down && !down;
  b->doubled = 0;
  if (down) {
    /* Double-Click Button handler */
    unsigned long long dt = sys_x11_timestamp() - b->timestamp;
    if (dt > SYS_DOUBLE_CLICK_LO && dt < SYS_DOUBLE_CLICK_HI) {
      b->doubled = True;
    } else {
      b->timestamp = sys_x11_timestamp();
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
sys_x11_on_btn(struct sys_x11_win *xwin, XEvent *e, int down) {
  if (e->xbutton.button == Button4 || e->xbutton.button == Button5) {
    /* mouse scrolling */
    if (e->xbutton.button == Button4 && down) {
      sys.mouse.scrl[1] += 1;
      sys.btn_mod = 1;
    } else if (e->xbutton.button == Button5 && down) {
      sys.mouse.scrl[1] -= 1;
      sys.btn_mod = 1;
    }
    return;
  }
  /* grab/ungrap mouse pointer */
  if (down) {
    if (x11.grab_cnt++ == 0) {
      XGrabPointer(x11.dpy, xwin->window, False,
          PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
          GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    }
  } else {
    if (--x11.grab_cnt == 0) {
      XUngrabPointer(x11.dpy, CurrentTime);
    }
  }
  if (e->xbutton.button == Button1) {
    sys_x11_btn(&sys.mouse.btn.left, down);
  } else if (e->xbutton.button == Button2) {
    sys_x11_btn(&sys.mouse.btn.middle, down);
  } else if (e->xbutton.button == Button3) {
    sys_x11_btn(&sys.mouse.btn.right, down);
  }
  sys.mouse.pos[0] = e->xbutton.x;
  sys.mouse.pos[1] = e->xbutton.y;
  sys.mouse_mod = 1;

  sys.keymod = sys_x11_keymod_map(e->xbutton.state);
  sys.btn_mod = 1;
}
static void
sys_x11_on_motion(XEvent *e) {
  sys.mouse.pos[0] = e->xmotion.x;
  sys.mouse.pos[1] = e->xmotion.y;
  sys.mouse.pos_delta[0] = sys.mouse.pos[0] - sys.mouse.pos_last[0];
  sys.mouse.pos_delta[1] = sys.mouse.pos[1] - sys.mouse.pos_last[1];
  sys.keymod = sys_x11_keymod_map(e->xmotion.state);
  sys.mouse_mod = 1;
}
static void
sys_on_key(int key) {
  bit_set(sys.keys, key);
  sys.key_mod = 1;
}
static void
sys_x11_on_key(XEvent *e) {
  int ret = 0;
  KeySym *code =
      XGetKeyboardMapping(x11.dpy, (KeyCode)e->xkey.keycode, 1, &ret);
  if (*code > 0 && *code < SYS_KEY_END)
    bit_set(sys.keys, (int)*code);
  sys.keymod = sys_x11_keymod_map(e->xkey.state);

  char buf[32] = {0};
  KeySym keysym = 0;
  int len = XLookupString((XKeyEvent *)e, buf, 32, &keysym, NULL);
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
    sys.txt[sys.txt_len] = ' ';
    sys.txt_len += 1;
  } else if (len != NoSymbol) {
    if (!sys.keymod) {
      memcpy(sys.txt + sys.txt_len, buf, cast(size_t, len));
      sys.txt_len += len;
    } else if (isalpha(buf[0])) {
      sys.txt[sys.txt_len++] = cast(char, toupper(buf[0]));
    }
  }
  XFree(code);
}
static void
sys_quit(void) {
  sys.running = False;
  sys.quit = True;
}
static void
sys_x11_on_client_msg(XEvent *e) {
  if (e->xclient.message_type == x11.xa_wm_protocols) {
    /* handle window close message */
    Atom protocol = cast(Atom, e->xclient.data.l[0]);
    if (protocol == x11.xa_wm_del_win) {
      sys_quit();
    }
  }
}
static void
sys_x11_on_sel_req(XEvent *e) {
  XEvent reply = {0};
  reply.xselection.type = SelectionNotify;
  reply.xselection.requestor = e->xselectionrequest.requestor;
  reply.xselection.selection = e->xselectionrequest.selection;
  reply.xselection.target = e->xselectionrequest.target;
  reply.xselection.time = e->xselectionrequest.time;
  reply.xselection.property = None;

  if (x11.clip.data) {
    if (reply.xselection.target == x11.xa_tar) {
      /* handle request for supported types */
      int tar_cnt = 0;
      Atom tar_list[4];
      tar_list[0] = x11.xa_tar;
      tar_list[1] = x11.xa_utf_str;
      tar_list[2] = XA_STRING;
      tar_cnt = 3;

      reply.xselection.property = e->xselectionrequest.property;
      XChangeProperty(e->xselection.display, e->xselectionrequest.requestor,
                      reply.xselection.property, XA_ATOM, 32,
                      PropModeReplace, (unsigned char *)&tar_list,
                      tar_cnt);
    } else if (sys_x11_clip_supports_type(reply.xselection.target)) {
      /* provide access to this apps clipboard data */
      reply.xselection.property = e->xselectionrequest.property;
      XChangeProperty(e->xselection.display, e->xselectionrequest.requestor,
                      reply.xselection.property, reply.xselection.target,
                      8, PropModeReplace, (unsigned char *)x11.clip.data,
                      x11.clip.len);
    }
  }
  XSendEvent(e->xselection.display, e->xselectionrequest.requestor, True, 0,
             &reply);
  XFlush(e->xselection.display);
}
static int
sys_x11_pull(struct sys_x11_win *xwin) {
  /* Handle window events */
  XEvent e = {0};
  int val = 1;
  while (XCheckWindowEvent(x11.dpy, xwin->window, xwin->swa.event_mask, &e)) {
    switch (e.type) {
    default: break;
    case NoExpose: return 0;
    case ConfigureNotify: val = 0; sys_x11_on_win_resize(xwin); break;
    case Expose: val = 0; sys_x11_on_win_expose(xwin, &e); break;
    case ButtonPress:
    case ButtonRelease: val = 0; sys_x11_on_btn(xwin, &e, e.type == ButtonPress); break;
    case MotionNotify: val = 0; sys_x11_on_motion(&e); break;
    case KeymapNotify: XRefreshKeyboardMapping(&e.xmapping); break;
    case KeyPress: val = 0; sys_x11_on_key(&e); break;
    }
  }
  /* Handle ClientMessages */
  while (XPending(x11.dpy)) {
    XNextEvent(x11.dpy, &e);
    switch (e.type) {
      case ClientMessage: val = 0; sys_x11_on_client_msg(&e); break;
      case SelectionRequest: val = 0; sys_x11_on_sel_req(&e); break;
      case SelectionClear: val = 0; sys_x11_clip_clear(); break;
      case SelectionNotify: break;
    }
  }
  sys.txt_mod = !!sys.txt_len;
  sys.win.w = x11.win.w;
  sys.win.h = x11.win.h;
  return !val;
}
static void
sys_x11_push(struct sys_x11_win *xwin) {
  /* cleanup */
  sys.seq++;
  sys.col_mod = 0;
  sys.keymod = 0;
  sys.txt_len = 0;
  sys.focus = 0;

  sys.btn_mod = 0;
  sys.txt_mod = 0;
  sys.key_mod = 0;
  sys.scrl_mod = 0;

  sys.mouse_mod = 0;
  sys.mouse.pos_last[0] = sys.mouse.pos[0];
  sys.mouse.pos_last[1] = sys.mouse.pos[1];

  for (int i = 0; i < cntof(sys.keys); ++i){
    sys.keys[i] = 0;
  }
  for (int i = 0; i < SYS_MOUSE_BTN_CNT; ++i) {
    sys.mouse.btns[i].pressed = 0;
    sys.mouse.btns[i].released = 0;
    sys.mouse.btns[i].doubled = 0;
  }
  /* handle cursor changes */
  if (sys.cursor != x11.cursor) {
    x11.cursor = sys.cursor;
    XUndefineCursor(x11.dpy, x11.win.window);
    if (sys.cursor != SYS_CUR_ARROW) {
      XDefineCursor(x11.dpy, x11.win.window, x11.cursors[sys.cursor]);
    }
  }
  /* blit dirty rects to window */
  for (int di = 0; di < dyn_cnt(sys.ren_target.dirty_rects); ++di) {
    const struct sys_rect *rect = &sys.ren_target.dirty_rects[di];
    int x = rect->x, y = rect->y;
    int w = rect->w, h = rect->h;

    if (x < 0) x += w, w += rect->x;
    if (y < 0) y += h, h += rect->y;
    if (x + w > xwin->attr.width) w = max(0, xwin->attr.width - x);
    if (y + h > xwin->attr.height) h = max(0, xwin->attr.height - y);
    XShmPutImage(x11.dpy, xwin->window, xwin->gc, xwin->fb.ximg, x, y, x, y,
                 (unsigned)w, (unsigned)h, False);
  }
  dyn_clr(sys.ren_target.dirty_rects);
  sys.ren_target.resized = 0;
  XFlush(x11.dpy);
}

/* ---------------------------------------------------------------------------
 *
 *                                  Main
 *
 * ---------------------------------------------------------------------------
 */
int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
  XSetLocaleModifiers("@im=none");
  XrmInitialize();

  x11.dpy = XOpenDisplay(0);
  if (!x11.dpy) {
    xpanic("Could not open a display; perhaps $DISPLAY is not set?");
  }
  /* extensions */
  if (!XShmQueryExtension(x11.dpy)) {
    XCloseDisplay(x11.dpy);
    xpanic("[sys]: X Server does not support XSHM extension\n");
  }
  /* create x11 resources */
  x11.root = DefaultRootWindow(x11.dpy);
  x11.screen = XDefaultScreen(x11.dpy);
  x11.vis = XDefaultVisual(x11.dpy, x11.screen);
  x11.cmap = XCreateColormap(x11.dpy, x11.root, x11.vis, AllocNone);

  /* lookup atoms */
  x11.xa_wm_protocols = XInternAtom(x11.dpy, "WM_PROTOCOLS", False);
  x11.xa_wm_del_win = XInternAtom(x11.dpy, "WM_DELETE_WINDOW", False);
  x11.xa_txt_uri_lst = XInternAtom(x11.dpy, "text/uri-list", False);
  x11.xa_clip = XInternAtom(x11.dpy, "CLIPBOARD", False);
  x11.xa_tar = XInternAtom(x11.dpy, "TARGETS", False);
  x11.xa_txt = XInternAtom(x11.dpy, "TEXT", False);
  x11.xa_utf_str = XInternAtom(x11.dpy, "UTF8_STRING", False);
  x11.xa_wm_hints = XInternAtom(x11.dpy, "_MOTIF_WM_HINTS", False);
  x11.xa_sys_tray = XInternAtom(x11.dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
  x11.xa_wm_icon = XInternAtom(x11.dpy, "_NET_WM_ICON", False);

  x11.xa_win_type = XInternAtom(x11.dpy, "_NET_WM_WINDOW_TYPE", False);
  x11.xa_wm_desk = XInternAtom(x11.dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
  x11.xa_wm_dock = XInternAtom(x11.dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
  x11.xa_wm_tool = XInternAtom(x11.dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
  x11.xa_wm_menu = XInternAtom(x11.dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
  x11.xa_wm_util = XInternAtom(x11.dpy, "_NET_WM_WINDOW_TYPE_UTILITY", False);
  x11.xa_wm_splsh = XInternAtom(x11.dpy, "_NET_WM_WINDOW_TYPE_SPLASH", False);
  x11.xa_wm_pop = XInternAtom(x11.dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
  x11.xa_wm_norm = XInternAtom(x11.dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False);

  x11.xa_wm_st = XInternAtom(x11.dpy, "_NET_WM_STATE", False);
  x11.xa_wm_st_maximized_horz =
      XInternAtom(x11.dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  x11.xa_wm_st_maximized_vert =
      XInternAtom(x11.dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);

  /* create cursors */
  static const unsigned x11_cur_map[] = {
    [SYS_CUR_ARROW] = XC_left_ptr,
    [SYS_CUR_NO] = XC_X_cursor,
    [SYS_CUR_CROSS] = XC_crosshair,
    [SYS_CUR_HAND] = XC_hand2,
    [SYS_CUR_HELP] = XC_question_arrow,
    [SYS_CUR_IBEAM] = XC_xterm,
    [SYS_CUR_MOVE] = XC_fleur,
    [SYS_CUR_SIZE_NS] = XC_sb_v_double_arrow,
    [SYS_CUR_SIZE_WE] = XC_sb_h_double_arrow,
    [SYS_CUR_UP_ARROW] = XC_sb_up_arrow,
    [SYS_CUR_DOWN_ARROW] = XC_sb_down_arrow,
    [SYS_CUR_LEFT_ARROW] = XC_sb_left_arrow,
    [SYS_CUR_RIGHT_ARROW] = XC_sb_right_arrow,
  };
  for (int i = 0; i < cntof(x11_cur_map); ++i) {
    x11.cursors[i] = XCreateFontCursor(x11.dpy, x11_cur_map[i]);
  }
  /* create helper window */
  x11.helper = XCreateSimpleWindow(x11.dpy, x11.root, -10, -10, 1, 1, 0, 0, 0);
  XSelectInput(x11.dpy, x11.helper, SelectionNotify);

  sys.running = True;
  x11.fd = ConnectionNumber(x11.dpy);

  sys.app = 0;
  sys.platform = &x11;
  sys.argc = argc;
  sys.argv = argv;
  cpu_info(&sys.cpu);

  /* memory */
  sys.mem.alloc = sys_x11_mem_alloc;
  sys.mem.free = sys_x11_mem_free;
  sys.mem.info = sys_x11_mem_stats;
  sys.mem.free_tag = sys_x11_mem_free_tag;
  sys.mem.arena = &x11.mem;
  sys.mem.tmp = &x11.tmp;
  sys.mem.page_siz = sysconf(_SC_PAGE_SIZE);
  sys.mem.phy_siz = sysconf(_SC_PHYS_PAGES) * sys.mem.page_siz;
  lst_init(&x11.mem_blks);

  /* directory */
  sys.dir.lst = sys_dir_lst;
  sys.dir.nxt = sys_dir_nxt;
  sys.dir.exists = sys_dir_exists;
  /* clipboard */
  sys.clipboard.set = sys_x11_clipboard_set;
  sys.clipboard.get = sys_x11_clipboard_get;
  /* plugin */
  sys.plugin.add = sys_x11_plugin_add;
  /* time */
  sys.time.timestamp = sys_x11_timestamp;

  /* constants */
  sys__x11_dpi_scale(sys.dpi_scale);
  x11.exe_path = sys_x11_get_exe_path(&x11.mem);
  x11.ren_path = sys_x11_get_exe_file_path(x11.exe_path, strv("ren"), strv(".so"), &x11.mem);
  x11.app_path = sys_x11_get_exe_file_path(x11.exe_path, strv("app"), strv(".so"), &x11.mem);
  x11.dbg_path = sys_x11_get_exe_file_path(x11.exe_path, strv("dbg"), strv(".so"), &x11.mem);

  /* open dbg dynamic library */
  static char *sys_dbg_module_fn_sym[] = {"dlInit","dlBegin","dlEnd"};
  x11.dbg_lib.path = x11.dbg_path;
  x11.dbg_lib.sym_cnt = cntof(sys_dbg_module_fn_sym);
  x11.dbg_lib.sym_names = sys_dbg_module_fn_sym;
  x11.dbg_lib.syms = (void**)&x11.dbg;
  sys_x11_mod_open(&x11.dbg_lib, &x11.tmp);
  if (x11.dbg.dlInit) {
    x11.dbg.dlInit(&sys);
  }
  /* open ren dynamic library */
  static char *sys_ren_module_fn_sym[] = {"dlInit","dlBegin","dlEnd","dlShutdown"};
  x11.ren_lib.path = x11.ren_path;
  x11.ren_lib.sym_cnt = cntof(sys_ren_module_fn_sym);
  x11.ren_lib.sym_names = sys_ren_module_fn_sym;
  x11.ren_lib.syms = (void**)&x11.ren;
  sys_x11_mod_open(&x11.ren_lib, &x11.tmp);
  if (x11.ren.dlInit) {
    x11.ren.dlInit(&sys);
  }
  /* open app dynamic library */
  static char *sys_app_module_fn_sym[] = {"dlEntry", "dlRegister"};
  x11.app_lib.path = x11.app_path;
  x11.app_lib.sym_cnt = cntof(sys_app_module_fn_sym);
  x11.app_lib.sym_names = sys_app_module_fn_sym;
  x11.app_lib.syms = (void**)&x11.app;
  sys_x11_mod_open(&x11.app_lib, &x11.tmp);
  if (x11.app.dlRegister)  {
    x11.app.dlRegister(&sys);
  }

  /* create window */
  struct sys_x11_win *xwin = &x11.win;
  memset(xwin, 0, sizeof(*xwin));
  xwin->swa.bit_gravity = StaticGravity;
  xwin->swa.background_pixel = 0;
  xwin->swa.colormap = x11.cmap;
  xwin->w = 800, xwin->h = 600;
  unsigned long attrmsk = CWColormap | CWBackPixel | CWEventMask | CWBitGravity;
  xwin->swa.event_mask = ExposureMask | KeyPressMask | Mod1Mask |
                         PointerMotionMask | ButtonPressMask |
                         ButtonReleaseMask | ButtonMotionMask |
                         KeymapStateMask | StructureNotifyMask;
  xwin->window = XCreateWindow(x11.dpy, x11.root, 0, 0, 800, 600, 0,
      XDefaultDepth(x11.dpy, x11.screen), InputOutput, x11.vis, attrmsk, &xwin->swa);

  sys.ren_target.pixels = sys_x11_fb_resize(&xwin->fb, xwin->w, xwin->h);
  xwin->gc = XCreateGC(x11.dpy, xwin->window, 0, 0);
  XSetWMProtocols(x11.dpy, xwin->window, &x11.xa_wm_del_win, 1);
  XMapWindow(x11.dpy, xwin->window);
  XGetWindowAttributes(x11.dpy, xwin->window, &xwin->attr);
  XFlush(x11.dpy);

  XChangeProperty(x11.dpy, xwin->window, x11.xa_win_type, XA_ATOM, 32,
                  PropModeReplace, (unsigned char *)&x11.xa_wm_pop, 1);
  XSetTransientForHint(x11.dpy, xwin->window, x11.root);

  /* run application */
  while (!sys.quit) {
    if (sys_x11_pull(xwin)) {
      if (x11.dbg.dlBegin) {
        x11.dbg.dlBegin(&sys);
      }
      if (x11.ren.dlBegin) {
        x11.ren.dlBegin(&sys);
      }
      if (x11.app.dlEntry)  {
        x11.app.dlEntry(&sys);
      }
      if (x11.ren.dlEnd) {
        dyn_clr(sys.ren_target.dirty_rects);
        sys.ren_target.w = x11.win.w;
        sys.ren_target.h = x11.win.h;

        x11.ren.dlEnd(&sys);
        sys.ren_target.resized = 0;
      }
      if (x11.dbg.dlEnd) {
        x11.dbg.dlEnd(&sys);
      }
      sys_x11_push(xwin);
    }
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(x11.fd, &fds);
    pselect(x11.fd + 1, &fds, 0, 0, 0, 0);
  }

  /* ungrab all global key shortcuts */
  KeyCode keycode = XKeysymToKeycode(x11.dpy, XK_p);
  XUngrabKey(x11.dpy, keycode, Mod1Mask, x11.root);

  /* make sure to ungrab mouse */
  XUngrabPointer(x11.dpy, CurrentTime);
  XUngrabKeyboard(x11.dpy, CurrentTime);

  /* unmap/destroy window */
  dyn_free(sys.ren_target.dirty_rects, &sys);
  sys_x11_fb_del(&xwin->fb);
  XUnmapWindow(x11.dpy, xwin->window);
  XDestroyWindow(x11.dpy, xwin->window);

  /* clear x11 resources */
  for (int i = 0; i < SYS_CUR_CNT; ++i) {
    XFreeCursor(x11.dpy, x11.cursors[i]);
  }
  free(x11.clip.data);

  XDestroyWindow(x11.dpy, x11.helper);
  XFreeColormap(x11.dpy, x11.cmap);
  XCloseDisplay(x11.dpy);
  return 0;
}

