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

/* usr */
#include "cpu.h"
#include "fmt.h"
#include "fmt.c"
#include "std.h"
#include "dbg.h"
#include "ren.h"
#include "sys.h"
#include "std.c"

/* os */
#include <io.h>
#include <errno.h>
#include <windows.h>

struct sys_mem_blk {
  struct mem_blk blk;
  struct lst_elm hook;
  unsigned long long tags;
};
struct sys_win32_module {
  unsigned valid : 1;
  struct str path;
  void *lib;

  int sym_cnt;
  char **sym_names;
  void **syms;
  void (*dlExport)(void *export, void *import);
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
struct sys_win32_win {
  int w, h;
  HWND wnd;
  struct sys_mouse mouse;
};
struct sys_win32 {
  int screen;
  int fd;

  int quit;
  struct lst_elm mem_blks;
  struct lck mem_lck;
  int col_mod;

  struct arena mem;
  struct arena tmp;
  struct str exe_path;
  struct sys_win32_win win;
  enum sys_cur_style cursor;

  /* modules */
  struct str dbg_path;
  struct sys_dbg_sym_table dbg;
  struct sys_win32_module dbg_lib;

  struct str ren_path;
  struct sys_ren_sym_table ren;
  struct sys_win32_module ren_lib;

  struct str app_path;
  struct sys_app_sym_table app;
  struct sys_win32_module app_lib;

  int mod_cnt;
#define SYS_WIN32_MAX_MODS 32
  struct sys_win32_module mods[SYS_WIN32_MAX_MODS];
};
static struct sys sys;
static struct sys_win32 win32;

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
sys_win32_get_exe_path(struct arena *a) {
  CHAR path[MAX_PATH];
  DWORD nbytes = GetModuleFileNameA(0, path, sizeof(path));
  return str(path, cast(int, nbytes));
}
static struct str
sys_win32_get_exe_file_path(struct str exe_path, struct str file,
                            struct str suffix, struct arena *a) {
  struct str exe_file = path_file(exe_path);
  struct str path = strp(exe_path.str, exe_file.str);
  return arena_fmt(a, &sys, "%.*s%.*s%.*s", strf(path), strf(file), strf(suffix));
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
  _findclose((intptr_t)it->handle);
}
static int
sys__dir_excl(struct sys_dir_iter *it) {
  int is_base = !str_cmp(it->name, str0("."));
  int is_prev = !str_cmp(it->name, str0(".."));
  return it->valid && (is_base || is_prev);
}
static void
sys__dir_update(struct sys_dir_iter *it, struct arena *a, int is_done,
                struct _finddata_t *info) {
  it->valid = !is_done;
  it->err = is_done && errno != ENOENT;
  if (!is_done) {
    it->fullpath = arena_fmt(a, &sys, "%.*s/%s", strf(it->base), info->name);
    it->name = arena_str(a, &sys, str0(info->name));
    it->isdir = fileinfo->attrib & _A_SUBDIR;
  }
}
static void
sys_dir_nxt(struct sys_dir_iter *it, struct arena *a) {
  if (!it->valid) {
    return;
  }
  scope_end(&it->scp, a, &sys);
  scope_begin(&it->scp, a);
  do {
    struct _finddata_t fileinfo;
    int ret = _findnext((intptr_t)it->handle, &fileinfo);
    sys__dir_update(it, a, ret != 0, &fileinfo);
    if (ret != 0) {
      scope_end(&it->scp, a, &sys);
      sys__dir_free(it, a);
      return;
    }
  } while (sys__dir_excl(it));
}
static void
sys_dir_lst(struct sys_dir_iter *it, struct arena *a, struct str path) {
  memset(it, 0, sizeof(*it));
  scope_begin(&it->scp_base, a);
  it->base = arena_str(a, &sys, path);

  struct scope scp;
  scope_begin(&scp, a);
  struct str fltr = arena_fmt(a, &sys, "%s/*", it->base.str);

  struct _finddata_t fileinfo;
  intptr_t hdl = _findfirst(fltr.str, &fileinfo);
  it->handle = (void *)hdl;
  scope_end(&scp, a, &sys);

  sys__dir_update(it, a, hdl == -1, &fileinfo);
  if (sys__dir_excl(it)) {
    sys_dir_nxt(it, a);
  }
}

/* ---------------------------------------------------------------------------
 *
 *                              Memory
 *
 * ---------------------------------------------------------------------------
 */
static struct mem_blk *
sys_win32_mem_alloc(struct mem_blk *opt_old, int siz, unsigned flags,
                    unsigned long long tags) {
  compiler_assert(szof(struct sys_mem_blk) == 64);
  int total = siz + szof(struct sys_mem_blk);
  int base_off = szof(struct sys_mem_blk);

  struct sys_mem_blk *blk = 0;
  size_t mapsiz = cast(size_t, total);
  if (flags & SYS_MEM_GROWABLE) {
    if (opt_old) {
      assert(opt_old->flags == flags);
      struct sys_mem_blk *sys_blk;
      sys_blk = containerof(opt_old, struct sys_mem_blk, blk);

      lck_acq(&win32.mem_lck);
      lst_del(&sys_blk->hook);
      lck_rel(&win32.mem_lck);
    }
    blk = realloc(opt_old, (size_t)total);
    blk->blk.base = (unsigned char *)blk + base_off;
  } else {
    blk = VirtualAlloc(0, (SIZE_T)total, MEM_RESERVE | MEM_COMMIT,
                       PAGE_READWRITE);
    ;
    blk->blk.base = (unsigned char *)blk + base_off;
  }
  lst_init(&blk->hook);

  blk->blk.size = siz;
  blk->blk.flags = flags;
  blk->tags = tags;

  lck_acq(&win32.mem_lck);
  lst_add(&win32.mem_blks, &blk->hook);
  lck_rel(&win32.mem_lck);
  return &blk->blk;
}
static void
sys_win32_mem_free(struct mem_blk *mem_blk) {
  struct sys_mem_blk *blk;
  blk = containerof(mem_blk, struct sys_mem_blk, blk);
  int total = blk->blk.size + szof(struct sys_mem_blk);

  lck_acq(&win32.mem_lck);
  lst_del(&blk->hook);
  lck_rel(&win32.mem_lck);

  if (mem_blk->flags & SYS_MEM_GROWABLE) {
    free(blk);
  } else {
    size_t unmapsiz = cast(size_t, total);
    VirtualFree(blk, unmapsiz);
  }
}
static void
sys_win32_mem_stats(struct sys_mem_stats *stats) {
  memset(stats, 0, sizeof(*stats));
  lck_acq(&win32.mem_lck);
  {
    struct lst_elm *elm = 0;
    for_lst(elm, &win32.mem_blks) {
      struct sys_mem_blk *blk;
      blk = lst_get(elm, struct sys_mem_blk, hook);
      stats->total += blk->blk.size;
      stats->used += blk->blk.used;
    }
  }
  lck_rel(&win32.mem_lck);
}
static void
sys_win32_mem_free_tag(unsigned long long tag) {
  lck_acq(&win32.mem_lck);
  struct lst_elm *elm = 0;
  for_lst(elm, &win32.mem_blks) {
    struct sys_mem_blk *blk = 0;
    blk = lst_get(elm, struct sys_mem_blk, hook);
    if (blk->tags == tag) {
      sys_win32_mem_free(&blk->blk);
    }
  }
  lck_rel(&win32.mem_lck);
}

/* ---------------------------------------------------------------------------
 *
 *                            Dynamic Library
 *
 * ---------------------------------------------------------------------------
 */
static void *
sys_win32_lib_sym(void *lib, const char *s) {
  void *sym = GetProcAddress(lib, s);
  if (!sym) {
    fprintf(stderr, "failed to load dynamic library symbol: %s\n", dlerror());
  }
  return sym;
}
static void *
sys_win32_lib_open(const char *path) {
  void *lib = LoadLibraryA(path);
  if (!lib) {
    fprintf(stderr, "failed to open dynamic library: %s\n", dlerror());
  }
  return lib;
}
static void
sys_win32_lib_close(void *lib) {
  if (lib != 0) {
    FreeLibrary(lib);
  }
}
static void
sys_win32_mod_close(struct sys_win32_module *mod) {
  if (mod->lib) {
    sys_win32_lib_close(mod->lib);
    mod->lib = 0;
  }
  mod->valid = 0;
}
static void
sys_win32_mod_open(struct sys_win32_module *mod, struct arena *mem) {
  struct scope scp = {0};
  scope_begin(&scp, mem);
  {
    mod->valid = 1;
    mod->lib = sys_win32_lib_open(mod->path.str);
    if (mod->lib) {
      for (int i = 0; i < mod->sym_cnt; ++i) {
        void *sym = sys_win32_lib_sym(mod->lib, mod->sym_names[i]);
        if (sym) {
          mod->syms[i] = sym;
        } else {
          mod->valid = 0;
        }
      }
    }
    if (!mod->valid) {
      sys_win32_mod_close(mod);
    }
  }
  scope_end(&scp, mem, &sys);
}

/* ---------------------------------------------------------------------------
 *
 *                                Clipboard
 *
 * ---------------------------------------------------------------------------
 */
static void
sys_win32_clipboard_set(struct str s, struct arena *a) {
  if (!OpenClipboard(0)) {
    return;
  }
  int wsize = MultiByteToWideChar(CP_UTF8, 0, s.str, s.len, 0, 0);
  if (wsize) {
    HGLOBAL mem =
        (HGLOBAL)GlobalAlloc(GMEM_MOVEABLE, (wsize + 1) * sizeof(wchar_t));
    if (mem) {
      wchar_t *wstr = (wchar_t *)GlobalLock(mem);
      if (wstr) {
        MultiByteToWideChar(CP_UTF8, 0, s.str, s.len, wstr, wsize);
        wstr[wsize] = 0;
        GlobalUnlock(mem);
        SetClipboardData(CF_UNICODETEXT, mem);
      }
    }
  }
  CloseClipboard();
}
static struct str
sys_win32_clipboard_get(struct arena *a) {
  if (!IsClipboardFormatAvailable(CF_UNICODETEXT) || !OpenClipboard(0)) {
    return str_nil;
  }
  struct str ret = str_nil;
  HGLOBAL mem = GetClipboardData(CF_UNICODETEXT);
  if (mem) {
    SIZE_T size = GlobalSize(mem) - 1;
    if (size) {
      LPCWSTR wstr = (LPCWSTR)GlobalLock(mem);
      if (wstr) {
        int utf8size =
            WideCharToMultiByte(CP_UTF8, 0, wstr, (int)(size / sizeof(wchar_t)),
                                NULL, 0, NULL, NULL);
        if (utf8size) {
          char *utf8 = (char *)malloc(utf8size);
          if (utf8) {
            WideCharToMultiByte(CP_UTF8, 0, wstr, (int)(size / sizeof(wchar_t)),
                                utf8, utf8size, NULL, NULL);
            ret = arena_str(a, &sys, str(utf8, utf8size));
            free(utf8);
          }
        }
        GlobalUnlock(mem);
      }
    }
  }
  CloseClipboard();
}

/* ---------------------------------------------------------------------------
 *
 *                                  Util
 *
 * ---------------------------------------------------------------------------
 */
static unsigned long long
sys_win32_timestamp(void) {
  return 0llu;
}
static int
sys_win32_plugin_add(void *exp, void *imp, struct str name) {
  assert(win32.mod_cnt < SYS_WIN32_MAX_MODS);
  int at = win32.mod_cnt++;
  struct sys_win32_module *mod = win32.mods + at;

  static char *sys_module_fn_sym[] = {"dlExport"};
  mod->path = sys_win32_get_exe_file_path(win32.exe_path, name, strv(".dll"),
                                          &win32.mem);
  mod->sym_cnt = cntof(sys_module_fn_sym);
  mod->sym_names = sys_module_fn_sym;
  mod->syms = (void **)&mod->dlExport;
  sys_win32_mod_open(mod, &win32.tmp);
  if (mod->dlExport) {
    mod->dlExport(exp, imp);
  }
  return mod->valid;
}

/* ---------------------------------------------------------------------------
 *
 *                                  Event
 *
 * ---------------------------------------------------------------------------
 */
static void
sys_quit(void) {
  sys.running = 0;
  sys.quit = 1;
}
static void
sys_win32_btn(struct sys_btn *b, int down) {
  assert(b);
  int was_down = b->down;
  b->down = down ? 1u : 0u;
  b->pressed = !was_down && down;
  b->released = was_down && !down;
  b->doubled = 0;
}
static unsigned
sys_win32_keymod_map(void) {
  unsigned ret = 0;
  if (GetKeyState(VK_CONTROL) & 0x8000) {
    ret |= SYS_KEYMOD_CTRL;
  }
  if (GetKeyState(VK_SHIFT) & 0x8000) {
    ret |= SYS_KEYMOD_SHIFT;
  }
  if (GetKeyState(VK_MENU) & 0x8000) {
    ret |= SYS_KEYMOD_ALT;
  }
  return ret;
}

/* ---------------------------------------------------------------------------
 *
 *                                  WIN32
 *
 * ---------------------------------------------------------------------------
 */
static void
sys_win32_on_btn(struct sys_btn *b, LPARAM lparam, int down, int doubled) {
  sys.mouse.pos[0] = LOWORD(lparam);
  sys.mouse.pos[1] = (short)HIWORD(lparam);
  sys.mouse_mod = 1;
  sys.btn_mod = 1;

  assert(b);
  int was_down = b->down;
  b->down = down ? 1u : 0u;
  b->pressed = !was_down && down;
  b->released = was_down && !down;
  b->doubled = doubled;
}
static void
sys_on_key(int key) {
  bit_set(sys.keys, key);
  sys.key_mod = 1;
}
static int
sys_win32_handle_event(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  sys.keymod = sys_win32_keymod_map();
  switch (msg) {
    case WM_SIZE: {
#if 0
    unsigned width = LOWORD( lparam );
    unsigned height = HIWORD( lparam );
    if( width != gdi.width || height != gdi.height )
    {
      DeleteObject( gdi.bitmap );
      gdi.bitmap = CreateCompatibleBitmap( gdi.window_dc, width, height );
      gdi.width = width;
      gdi.height = height;
      SelectObject( gdi.memory_dc, gdi.bitmap );
    }
#endif
    } break;
    case WM_PAINT: {
#if 0
    PAINTSTRUCT paint;
    HDC dc = BeginPaint( wnd, &paint );
    nk_gdi_blit( dc );
    EndPaint( wnd, &paint );
#endif
      return 1;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP: {
      int down = !((lparam >> 31) & 1);
      if (!down) {
        break;
      }
      switch (wparam) {
        case VK_DELETE:
          sys_on_key(SYS_KEY_DEL);
          return 1;
        case VK_RETURN:
          sys_on_key(SYS_KEY_RETURN);
          return 1;
        case VK_TAB:
          sys_on_key(SYS_KEY_TAB);
          return 1;
        case VK_LEFT:
          sys_on_key(SYS_KEY_LEFT);
          return 1;
        case VK_RIGHT:
          sys_on_key(SYS_KEY_RIGHT);
          return 1;
        case VK_UP:
          sys_on_key(SYS_KEY_UP);
          return 1;
        case VK_DOWN:
          sys_on_key(SYS_KEY_DOWN);
          return 1;
        case VK_BACK:
          sys_on_key(SYS_KEY_BACKSPACE);
          return 1;
        case VK_HOME:
          sys_on_key(SYS_KEY_HOME);
          return 1;
        case VK_END:
          sys_on_key(SYS_KEY_END);
          return 1;
        case VK_NEXT:
          sys_on_key(SYS_KEY_PGDN);
          return 1;
        case VK_PRIOR:
          sys_on_key(SYS_KEY_PGUP);
          return 1;
        case VK_ESCAPE:
          sys_on_key(SYS_KEY_ESCAPE);
          return 1;
        case VK_SUBTRACT:
          sys_on_key(SYS_KEY_MINUS);
          return 1;
        case VK_ADD:
          sys_on_key(SYS_KEY_PLUS);
          return 1;
        case VK_CAPITAL:
          sys_on_key(SYS_KEY_CAPS);
          return 1;
        case VK_SPACE:
          sys_on_key(SYS_KEY_SPACE);
          sys.txt[sys.txt_len++] = " ";
          return 1;
      }
      return 0;
    } break;
    case WM_CHAR:
      if (wparam >= 32) {
        bit_set(sys.keys, wparam);
        sys.txt[sys.txt_len++] = wparam;
        return 1;
      }
      break;

    case WM_LBUTTONDOWN:
      sys_win32_on_btn(&sys.mouse.btn.left, lparam, 1, 0);
      SetCapture(wnd);
      return 1;

    case WM_LBUTTONUP:
      sys_win32_on_btn(&sys.mouse.btn.left, lparam, 0, 0);
      ReleaseCapture();
      return 1;

    case WM_RBUTTONDOWN:
      sys_win32_on_btn(&sys.mouse.btn.right, lparam, 1, 0);
      SetCapture(wnd);
      return 1;

    case WM_RBUTTONUP:
      sys_win32_on_btn(&sys.mouse.btn.right, lparam, 0, 0);
      ReleaseCapture();
      return 1;

    case WM_MBUTTONDOWN:
      sys_win32_on_btn(&sys.mouse.btn.middle, lparam, 1, 0);
      SetCapture(wnd);
      return 1;

    case WM_MBUTTONUP:
      sys_win32_on_btn(&sys.mouse.btn.middle, lparam, 0, 0);
      ReleaseCapture();
      return 1;

    case WM_MOUSEWHEEL:
      sys.mouse.scrl[1] += ((short)HIWORD(wparam) / WHEEL_DELTA);
      sys.btn_mod = 1;

    case WM_LBUTTONDBLCLK:
      sys_win32_on_btn(&sys.mouse.btn.left, lparam, 0, 1);
      break;

    case WM_MOUSEMOVE: {
      sys.mouse.pos[0] = (short)LOWORD(lparam);
      sys.mouse.pos[1] = (short)HIWORD(lparam);
      sys.mouse.pos_delta[0] = sys.mouse.pos[0] - sys.mouse.pos_last[0];
      sys.mouse.pos_delta[1] = sys.mouse.pos[1] - sys.mouse.pos_last[1];
      sys.mouse_mod = 1;
    }
  }
  return 0;
}
static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  if (sys_win32_handle_event(wnd, msg, wparam, lparam)) {
    return 0;
  }
  return DefWindowProcW(wnd, msg, wparam, lparam);
}
static void
sys_win32_pull(struct sys_win32_win32 *xwin) {
  MSG msg;
  if (GetMessageW(&msg, NULL, 0, 0) <= 0) {
    sys_quit();
  } else {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) {
      sys_quit();
    }
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  sys.txt_mod = !!sys.txt_len;
  sys.win.w = win32.win.w;
  sys.win.h = win32.win.h;
}
static void
sys_win32_push(struct sys_win32_win *xwin) {
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

  for (int i = 0; i < cntof(sys.keys); ++i) {
    sys.keys[i] = 0;
  }
  for (int i = 0; i < SYS_MOUSE_BTN_CNT; ++i) {
    sys.mouse.btns[i].pressed = 0;
    sys.mouse.btns[i].released = 0;
    sys.mouse.btns[i].doubled = 0;
  }

#if 0
  /* blit dirty rects to window */
  for( int di = 0; di < dyn_cnt( sys.ren_target.dirty_rects ); ++di )
  {
    const struct sys_rect *rect = &sys.ren_target.dirty_rects[ di ];
    int x = rect->x, y = rect->y;
    int w = rect->w, h = rect->h;

    if( x < 0 ) x += w, w += rect->x;
    if( y < 0 ) y += h, h += rect->y;
    if( x + w > xwin->attr.width ) w = max( 0, xwin->attr.width - x );
    if( y + h > xwin->attr.height ) h = max( 0, xwin->attr.height - y );
    XShmPutImage( win32.dpy, xwin->window, xwin->gc, xwin->fb.ximg, x, y, x, y,
      ( unsigned )w, ( unsigned )h, False );
  }
#endif

  dyn_clr(sys.ren_target.dirty_rects);
  sys.ren_target.resized = 0;
}
int
main(int argc, char **argv) {
  sys.running = 1;

  sys.app = 0;
  sys.platform = &win32;
  sys.argc = argc;
  sys.argv = argv;
  cpu_info(&sys.cpu);

  SYSTEM_INFO sys_info;
  GetSystemInfo(&sys_info);

  /* memory */
  sys.mem.alloc = sys_win32_mem_alloc;
  sys.mem.free = sys_win32_mem_free;
  sys.mem.info = sys_win32_mem_stats;
  sys.mem.free_tag = sys_win32_mem_free_tag;
  sys.mem.arena = &win32.mem;
  sys.mem.tmp = &win32.tmp;
  sys.mem.page_siz = sys_info.dwPageSize;
  lst_init(&win32.mem_blks);

  /* directory */
  sys.dir.lst = sys_dir_lst;
  sys.dir.nxt = sys_dir_nxt;
  sys.dir.exists = sys_dir_exists;
  /* clipboard */
  sys.clipboard.set = sys_win32_clipboard_set;
  sys.clipboard.get = sys_win32_clipboard_get;
  /* plugin */
  sys.plugin.add = sys_win32_plugin_add;
  /* time */
  sys.time.timestamp = sys_win32_timestamp;

  /* constants */
  sys__win32_dpi_scale(sys.dpi_scale);
  win32.exe_path = sys_win32_get_exe_path(&win32.mem);
  win32.ren_path = sys_win32_get_exe_file_path(win32.exe_path, strv("ren"), strv(".dll"), &win32.mem);
  win32.app_path = sys_win32_get_exe_file_path(win32.exe_path, strv("app"), strv(".dll"), &win32.mem);
  win32.dbg_path = sys_win32_get_exe_file_path(win32.exe_path, strv("dbg"), strv(".dll"), &win32.mem);

  /* open dbg dynamic library */
  static char *sys_dbg_module_fn_sym[] = {"dlInit", "dlBegin", "dlEnd"};
  win32.dbg_lib.path = win32.dbg_path;
  win32.dbg_lib.sym_cnt = cntof(sys_dbg_module_fn_sym);
  win32.dbg_lib.sym_names = sys_dbg_module_fn_sym;
  win32.dbg_lib.syms = (void **)&win32.dbg;
  sys_win32_mod_open(&win32.dbg_lib, &win32.tmp);
  if (win32.dbg.dlInit) {
    win32.dbg.dlInit(&sys);
  }
  /* open ren dynamic library */
  static char *sys_ren_module_fn_sym[] = {"dlInit", "dlBegin", "dlEnd",
                                          "dlShutdown"};
  win32.ren_lib.path = win32.ren_path;
  win32.ren_lib.sym_cnt = cntof(sys_ren_module_fn_sym);
  win32.ren_lib.sym_names = sys_ren_module_fn_sym;
  win32.ren_lib.syms = (void **)&win32.ren;
  sys_win32_mod_open(&win32.ren_lib, &win32.tmp);
  if (win32.ren.dlInit) {
    win32.ren.dlInit(&sys);
  }
  /* open app dynamic library */
  static char *sys_app_module_fn_sym[] = {"dlEntry", "dlRegister"};
  win32.app_lib.path = win32.app_path;
  win32.app_lib.sym_cnt = cntof(sys_app_module_fn_sym);
  win32.app_lib.sym_names = sys_app_module_fn_sym;
  win32.app_lib.syms = (void **)&win32.app;
  sys_win32_mod_open(&win32.app_lib, &win32.tmp);
  if (win32.app.dlRegister) {
    win32.app.dlRegister(&sys);
  }

  /* window */
  WNDCLASSW wc = {0};
  memset(&wc, 0, sizeof(wc));
  wc.style = CS_DBLCLKS;
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = GetModuleHandleW(0);
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = L"MuonWindowClass";
  ATOM atom = RegisterClassW(&wc);

  RECT rect = {0, 0, 800, 600};
  DWORD exstyle = WS_EX_APPWINDOW;
  DWORD style = WS_OVERLAPPEDWINDOW;
  AdjustWindowRectEx(&rect, style, FALSE, exstyle);
  win32.win.wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"Muon",
                      style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                      rect.right - rect.left, rect.bottom - rect.top, NULL,
                      NULL, wc.hInstance, NULL);

  while (sys.running) {
    sys_win32_pull(&win32.win);
    if (win32.dbg.dlBegin) {
      win32.dbg.dlBegin(&sys);
    }
    if (win32.ren.dlBegin) {
      win32.ren.dlBegin(&sys);
    }
    if (win32.app.dlEntry) {
      win32.app.dlEntry(&sys);
    }
    if (win32.ren.dlEnd) {
      dyn_clr(sys.ren_target.dirty_rects);
      sys.ren_target.w = win32.win.w;
      sys.ren_target.h = win32.win.h;

      win32.ren.dlEnd(&sys);
      sys.ren_target.resized = 0;
    }
    if (win32.dbg.dlEnd) {
      win32.dbg.dlEnd(&sys);
    }
    sys_win32_push(&win32.win);
  }
  UnregisterClassW(wc.lpszClassName, wc.hInstance);
  return 0;
}
