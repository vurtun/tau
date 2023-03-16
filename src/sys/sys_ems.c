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
#include "../lib/fmt.h"
#include "../lib/fmt.c"
#include "../lib/std.h"
#include "ren.h"
#include "sys.h"
#include "../lib/std.c"
#include "../lib/math.c"
#include "../app.h"
#include "ren_gl2.c"

#define SYS_EMS_TITLE_LEN 128

struct sys_ren_sym_table ren;
struct sys_app_sym_table app;

struct sys_mem_blk {
  struct mem_blk blk;
  struct lst_elm hook;
  unsigned long long tags;
};
struct sys_app_sym_table {
  void (*dlEntry)(struct sys *s);
  void (*dlRegister)(struct sys *s);
};
struct sys_ren_sym_table {
  void (*dlInit)(struct sys *s);
  void (*dlBegin)(struct sys *s);
  void (*dlEnd)(struct sys *s);
  void (*dlShutdown)(struct sys *s);
};
struct sys_ems {
  struct lst_elm mem_blks;
  struct arena mem;
  struct arena tmp;

  unsigned textfield_created:1;
  unsigned show_keyboard:1;
  unsigned hide_keyboard:1;
  unsigned has_keyboard:1;
  unsigned mouse_lock_req:1;
  unsigned html5_ask_leave_site:1;

  struct sys_ren_sym_table ren;
  struct sys_app_sym_table app;

  enum sys_cur_style cursor;
  unsigned short mouse_buttons;
  float dpi_scale;
  char html5_canvas_selector[SYS_EMS_TITLE_LEN];
};
static struct sys_ems _ems;
static struct sys _sys;

static void sys__ems_on_frame(void);

EMSCRIPTEN_KEEPALIVE void
sys__ems_notify_keyboard_hidden(void) {
  _ems.show_keyboard = 0;
}

/* ---------------------------------------------------------------------------
 *
 *                                OpenGL
 *
 * ---------------------------------------------------------------------------
 */
static EM_BOOL
sys__ems_webgl_context_cb(int emsc_type, const void* resv, void *usr) {
  unused(resv);
  unused(usr);
  return true;
}
static void
sys_ems_webgl_init(void) {
  EmscriptenWebGLContextAttributes attrs;
  emscripten_webgl_init_context_attributes(&attrs);
  attrs.alpha = false;
  attrs.depth = true;
  attrs.stencil = true;
  attrs.antialias = false;
  attrs.premultipliedAlpha = false;
  attrs.preserveDrawingBuffer = false;
  attrs.enableExtensionsByDefault = true;

  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(_ems.html5_canvas_selector, &attrs);
  if (!ctx) {
    attrs.majorVersion = 1;
    ctx = emscripten_webgl_create_context(_ems.html5_canvas_selector, &attrs);
  }
  emscripten_webgl_make_context_current(ctx);
  emscripten_webgl_enable_extension(ctx, "WEBKIT_WEBGL_compressed_texture_pvrtc");
}

/* ---------------------------------------------------------------------------
 *
 *                              Input
 *
 * ---------------------------------------------------------------------------
 */
EM_JS(void, sys_ems_js_create_textfield, (void), {
  /* Javascript helper functions for mobile virtual keyboard input */
  var _tau_in = document.createElement("input");
  _tau_in.type = "text";
  _tau_in.id = "tau_app_input_element";
  _tau_in.autocapitalize = "none";
  _tau_in.addEventListener("focusout", function(_sapp_event) {
    sys__ems_notify_keyboard_hidden()
  });
  document.body.append(_tau_in);
});
EM_JS(void, sys_ems_js_focus_textfield, (void), {
  document.getElementById("tau_input_element").focus();
});
static void
sys__ems_update_keyboard_state(void) {
  if (_ems.show_keyboard) {
    /* create input text field on demand */
    if (!_ems.textfield_created) {
      _ems.textfield_created = 1;
      sys_ems_js_create_textfield();
    }
    /* focus the text input field, this will bring up the keyboard */
    _ems.has_keyboard = 1;
    _ems.show_keyboard = 0;
    sys_ems_js_focus_textfield();
  }
  if (_ems.hide_keyboard) {
    /* unfocus the text input field */
    if (_ems.textfield_created) {
      _ems.has_keyboard = 0;
      _ems.hide_keyboard = 0;
      sys_ems_js_focus_textfield();
    }
  }
}
static void
sys_ems_show_keyboard(bool show) {
  if (show) {
    _ems.show_keyboard = 1;
  } else {
    _ems.hide_keyboard = 1;
  }
}
EM_JS(void, sys_ems_js_init, (const char* c_str_target), {
  // lookup and store canvas object by name
  var target_str = UTF8ToString(c_str_target);
  Module.tau_emsc_target = document.getElementById(target_str);
  if (!Module.tau_emsc_target) {
    console.log("sys.h: invalid target:" + target_str);
  }
});
static unsigned
sys__ems_mouse_key_mod(const EmscriptenMouseEvent* ev) {
  unsigned mod = 0;
  mod = (ev->ctrlKey) ? mod|SYS_KEYMOD_CTRL : mod;
  mod = (ev->shiftKey) ? mod|SYS_KEYMOD_SHIFT : mod;
  mod = (ev->altKey) ? mod|SYS_KEYMOD_ALT : mod;
  return mod;
}
static unsigned
sys__ems_keyboard_key_mod(const EmscriptenKeyboardEvent* ev) {
  unsigned mod = 0;
  mod = (ev->ctrlKey) ? mod|SYS_KEYMOD_CTRL : mod;
  mod = (ev->shiftKey) ? mod|SYS_KEYMOD_SHIFT : mod;
  mod = (ev->altKey) ? mod|SYS_KEYMOD_ALT : mod;
  return mod;
}
static void
sys__ems_on_btn(struct sys_btn *b, int down) {
  assert(b);
  int was_down = b->down;
  b->down = !!down;
  b->pressed = !was_down && down;
  b->released = was_down && !down;
  b->doubled = 0;
}
static EM_BOOL
sys__ems_on_mouse(int emsc_type, const EmscriptenMouseEvent *evt, void *usr) {
  /* update mouse position */
  _ems.mouse_buttons = evt->buttons;
  float new_x = castf(evt->targetX) * _ems.dpi_scale;
  float new_y = castf(evt->targetY) * _ems.dpi_scale;
  _sys.mouse.pos[0] = casti(new_x);
  _sys.mouse.pos[1] = casti(new_y);
  _sys.mouse.pos_delta[0] = _sys.mouse.pos[0] - _sys.mouse.pos_last[0];
  _sys.mouse.pos_delta[1] = _sys.mouse.pos[1] - _sys.mouse.pos_last[1];

  if ((evt->button >= 0) && (evt->button < SYS_MOUSE_BTN_CNT)) {
    switch (emsc_type) {
    default: break;
    case EMSCRIPTEN_EVENT_MOUSEUP:
    case EMSCRIPTEN_EVENT_MOUSEDOWN: {
      /* update button state */
      int is_down = emsc_type == EMSCRIPTEN_EVENT_MOUSEDOWN;
      switch (evt->button) {
      case 0: sys__ems_on_btn(&_sys.mouse.btn.left, is_down); break;
      case 1: sys__ems_on_btn(&_sys.mouse.btn.middle, is_down); break;
      case 2: sys__ems_on_btn(&_sys.mouse.btn.right, is_down); break;
      default: break;}

      _sys.btn_mod = 1;
      _sys.mouse_mod = 1;
      _sys.keymod |= sys__ems_mouse_key_mod(evt);
      emscripten_console_log("Mouse Button\n");
      sys__ems_on_frame();
    } break;
    case EMSCRIPTEN_EVENT_MOUSEENTER:
    case EMSCRIPTEN_EVENT_MOUSELEAVE:
    case EMSCRIPTEN_EVENT_MOUSEMOVE: {
      _sys.btn_mod = 1;
      _sys.mouse_mod = 1;
      _sys.keymod |= sys__ems_mouse_key_mod(evt);
      sys__ems_on_frame();
    } break;}
  }
  return true;
}
static EM_BOOL
sys__ems_mouse_wheel_cb(int type, const EmscriptenWheelEvent* evt, void *usr) {
  float scale;
  _ems.mouse_buttons = evt->mouse.buttons;
  switch (evt->deltaMode) {
  case DOM_DELTA_PIXEL: scale = -0.04f; break;
  case DOM_DELTA_LINE:  scale = -1.33f; break;
  case DOM_DELTA_PAGE:  scale = -10.0f; break;
  default:              scale = -0.1f; break;}

  float dx = scale * castf(evt->deltaX);
  float dy = scale * castf(evt->deltaY);
  if ((math_abs(dx) >= 1.0f) || (math_abs(dy) >= 1.0f)) {
    _sys.mouse.scrl[0] = casti(dx);
    _sys.mouse.scrl[1] = casti(dy);
    _sys.scrl_mod = 1;
    sys__ems_on_frame();
  }
  emscripten_console_log("Mouse Wheel\n");
  return true;
}
static EM_BOOL
sys__ems_key_cb(int evt_type, const EmscriptenKeyboardEvent* evt, void* usr) {
  EM_BOOL ret = true;
  switch (evt_type) {
  case EMSCRIPTEN_EVENT_KEYUP:
  default: break;
  case EMSCRIPTEN_EVENT_KEYDOWN: {
    int is_down = (evt_type == EMSCRIPTEN_EVENT_KEYDOWN);
    _sys.keymod |= sys__ems_keyboard_key_mod(evt);

    struct str code = str0(evt->code);
    if (str_eq(code, strv("Escape"))) bit_set(_sys.keys, SYS_KEY_ESCAPE);
    else if (str_eq(code, strv("Space"))) bit_set(_sys.keys, SYS_KEY_SPACE);
    else if (str_eq(code, strv("PageUp"))) bit_set(_sys.keys, SYS_KEY_PGUP);
    else if (str_eq(code, strv("PageDown"))) bit_set(_sys.keys, SYS_KEY_PGDN);
    else if (str_eq(code, strv("End"))) bit_set(_sys.keys, SYS_KEY_END);
    else if (str_eq(code, strv("Home"))) bit_set(_sys.keys, SYS_KEY_HOME);
    else if (str_eq(code, strv("Backspace"))) bit_set(_sys.keys, SYS_KEY_BACKSPACE);
    else if (str_eq(code, strv("Tab"))) bit_set(_sys.keys, SYS_KEY_TAB);
    else if (str_eq(code, strv("Enter"))) bit_set(_sys.keys, SYS_KEY_RETURN);
    else if (str_eq(code, strv("CapsLock"))) bit_set(_sys.keys, SYS_KEY_CAPS);
    else if (str_eq(code, strv("ArrowLeft"))) bit_set(_sys.keys, SYS_KEY_LEFT);
    else if (str_eq(code, strv("ArrowUp"))) bit_set(_sys.keys, SYS_KEY_UP);
    else if (str_eq(code, strv("ArrowRight"))) bit_set(_sys.keys, SYS_KEY_RIGHT);
    else if (str_eq(code, strv("ArrowDown"))) bit_set(_sys.keys, SYS_KEY_DOWN);
    else if (str_eq(code, strv("Delete"))) bit_set(_sys.keys, SYS_KEY_DEL);
    else if (str_eq(code, strv("Minus"))) bit_set(_sys.keys, SYS_KEY_MINUS);
    else if (str_eq(code, strv("Equal"))) bit_set(_sys.keys, SYS_KEY_PLUS);
    else if (str_eq(code, strv("F1"))) bit_set(_sys.keys, SYS_KEY_F1);
    else if (str_eq(code, strv("F2"))) bit_set(_sys.keys, SYS_KEY_F2);
    else if (str_eq(code, strv("F3"))) bit_set(_sys.keys, SYS_KEY_F3);
    else if (str_eq(code, strv("F4"))) bit_set(_sys.keys, SYS_KEY_F4);
    else if (str_eq(code, strv("F5"))) bit_set(_sys.keys, SYS_KEY_F5);
    else if (str_eq(code, strv("F6"))) bit_set(_sys.keys, SYS_KEY_F6);
    else if (str_eq(code, strv("F7"))) bit_set(_sys.keys, SYS_KEY_F7);
    else if (str_eq(code, strv("F8"))) bit_set(_sys.keys, SYS_KEY_F8);
    else if (str_eq(code, strv("F9"))) bit_set(_sys.keys, SYS_KEY_F9);
    else if (str_eq(code, strv("F10"))) bit_set(_sys.keys, SYS_KEY_F10);
    else if (str_eq(code, strv("F11"))) bit_set(_sys.keys, SYS_KEY_F11);
    else if (str_eq(code, strv("F12"))) bit_set(_sys.keys, SYS_KEY_F12);
    else ret = false;

    _sys.key_mod = 1;
    sys__ems_on_frame();
  } break;
  case EMSCRIPTEN_EVENT_KEYPRESS: {
    _sys.keymod |= sys__ems_keyboard_key_mod(evt);
    if ((evt->metaKey) && (evt->charCode == 118)) {
      ret = 0;
    }
    char buf[32] = {0};
    int codepoint = evt->charCode;
    int n = utf_enc(buf, cntof(buf), codepoint);
    if (_sys.txt_len + n < cntof(_sys.txt)) {
      mcpy(_sys.txt + _sys.txt_len, buf, castsz(n));
      _sys.txt_len += n;
      _sys.txt_mod = 1;
    }
    sys__ems_on_frame();
  } break;}
  return ret;
}
static EM_BOOL
sys__ems_resize_cb(int evt_type, const EmscriptenUiEvent* evt, void* usr) {
  unused(evt_type);
  unused(usr);

  double w, h;
  emscripten_get_element_css_size(_ems.html5_canvas_selector, &w, &h);
  if (w < 1.0) {
    w = evt->windowInnerWidth;
  } else {
    _sys.win.w = math_roundi(w);
  }
  if (h < 1.0) {
    h = evt->windowInnerHeight;
  } else {
    _sys.win.h = math_roundi(h);
  }
  _ems.dpi_scale = emscripten_get_device_pixel_ratio();
  _sys.ren_target.w = math_roundi(w * _ems.dpi_scale);
  _sys.ren_target.h = math_roundi(h * _ems.dpi_scale);
  _sys.ren_target.resized = 1;
  assert(_sys.ren_target.h > 0 && _sys.ren_target.w > 0);
  emscripten_set_canvas_element_size(_ems.html5_canvas_selector, _sys.ren_target.w, _sys.ren_target.h);
  sys_ems_webgl_init();
  _sys.win.resized = 1;
  sys__ems_on_frame();
  return true;
}
static void
sys_ems_reg_evt_cb(void) {
  emscripten_set_mousedown_callback(_ems.html5_canvas_selector, 0, true, sys__ems_on_mouse);
  emscripten_set_mouseup_callback(_ems.html5_canvas_selector, 0, true, sys__ems_on_mouse);
  emscripten_set_mousemove_callback(_ems.html5_canvas_selector, 0, true, sys__ems_on_mouse);
  emscripten_set_mouseenter_callback(_ems.html5_canvas_selector, 0, true, sys__ems_on_mouse);
  emscripten_set_mouseleave_callback(_ems.html5_canvas_selector, 0, true, sys__ems_on_mouse);
  emscripten_set_wheel_callback(_ems.html5_canvas_selector, 0, true, sys__ems_mouse_wheel_cb);
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, true, sys__ems_key_cb);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, true, sys__ems_key_cb);
  emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, true, sys__ems_key_cb);
  emscripten_set_webglcontextlost_callback(_ems.html5_canvas_selector, 0, true, sys__ems_webgl_context_cb);
  emscripten_set_webglcontextrestored_callback(_ems.html5_canvas_selector, 0, true, sys__ems_webgl_context_cb);
}
static void
sys_ems_unreg_evt_cb(void) {
  emscripten_set_mousedown_callback(_ems.html5_canvas_selector, 0, true, 0);
  emscripten_set_mouseup_callback(_ems.html5_canvas_selector, 0, true, 0);
  emscripten_set_mousemove_callback(_ems.html5_canvas_selector, 0, true, 0);
  emscripten_set_mouseenter_callback(_ems.html5_canvas_selector, 0, true, 0);
  emscripten_set_mouseleave_callback(_ems.html5_canvas_selector, 0, true, 0);
  emscripten_set_wheel_callback(_ems.html5_canvas_selector, 0, true, 0);
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, true, 0);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, true, 0);
  emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, true, 0);
  emscripten_set_webglcontextlost_callback(_ems.html5_canvas_selector, 0, true, 0);
  emscripten_set_webglcontextrestored_callback(_ems.html5_canvas_selector, 0, true, 0);
}
/* ---------------------------------------------------------------------------
 *
 *                              Memory
 *
 * ---------------------------------------------------------------------------
 */
static struct mem_blk*
sys_ems_mem_alloc(struct mem_blk* opt_old, int siz, unsigned flags,
                  unsigned long long tags) {
  int total = siz + szof(struct sys_mem_blk);
  int base_off = szof(struct sys_mem_blk);
  struct sys_mem_blk *blk = 0;
  size_t mapsiz = castsz(total);
  if (flags & SYS_MEM_GROWABLE) {
    if (opt_old){
      assert(opt_old->flags == flags);
      struct sys_mem_blk *sys_blk;
      sys_blk = containerof(opt_old, struct sys_mem_blk, blk);
      lst_del(&sys_blk->hook);
    }
    blk = realloc(opt_old, (size_t)total);
    if (!opt_old) {
      memset(blk, 0, castsz(total));
    }
    blk->blk.base = (unsigned char*)blk + base_off;
  } else {
    blk = calloc(mapsiz,1);
    blk->blk.base = (unsigned char*)blk + base_off;
  }
  lst_init(&blk->hook);

  blk->blk.size = siz;
  blk->blk.flags = flags;
  blk->tags = tags;

  lst_add(&_ems.mem_blks, &blk->hook);
  return &blk->blk;
}
static void
sys_ems_mem_free(struct mem_blk *mem_blk) {
  struct sys_mem_blk *blk;
  blk = containerof(mem_blk, struct sys_mem_blk, blk);
  int total = blk->blk.size + szof(struct sys_mem_blk);
  lst_del(&blk->hook);
  if (mem_blk->flags & SYS_MEM_GROWABLE) {
    free(blk);
  } else {
    free(blk);
  }
}
static void
sys_ems_mem_stats(struct sys_mem_stats *stats) {
  memset(stats, 0, sizeof(*stats));
  {
    struct lst_elm *elm = 0;
    for_lst(elm, &_ems.mem_blks) {
      struct sys_mem_blk *blk = lst_get(elm, struct sys_mem_blk, hook);
      stats->total += blk->blk.size;
      stats->used += blk->blk.used;
    }
  }
}
static void
sys_ems_mem_free_tag(unsigned long long tag) {
  struct lst_elm *elm = 0;
  for_lst(elm, &_ems.mem_blks) {
    struct sys_mem_blk *blk = 0;
    blk = lst_get(elm, struct sys_mem_blk, hook);
    if (blk->tags == tag) {
      sys_ems_mem_free(&blk->blk);
    }
  }
}
/* ---------------------------------------------------------------------------
 *
 *                              Console
 *
 * ---------------------------------------------------------------------------
 */
static void
sys__ems_log(const char *fmt, ...) {
  char buf[2*1024];
  va_list args;
  va_start(args, fmt);
  fmtvsn(buf, szof(buf), fmt, args);
  va_end(args);
  emscripten_console_log(buf);
}
static void
sys__ems_warn(const char *fmt, ...) {
  char buf[2*1024];
  va_list args;
  va_start(args, fmt);
  fmtvsn(buf, szof(buf), fmt, args);
  va_end(args);
  emscripten_console_warn(buf);
}
static void
sys__ems_err(const char *fmt, ...) {
  char buf[2*1024];
  va_list args;
  va_start(args, fmt);
  fmtvsn(buf, szof(buf), fmt, args);
  va_end(args);
  emscripten_console_error(buf);
}
/* ---------------------------------------------------------------------------
 *
 *                              Main
 *
 * ---------------------------------------------------------------------------
 */
EM_JS(void, sys_ems_js_set_cursor, (int cursor_type, int shown), {
  if (Module.tau_ems_target) {
    var cursor;
    if (shown === 0) {
      cursor = "none";
    } else {
      switch (cursor_type) {
        case 0: cursor = "auto"; break;
        case 1: cursor = "not-allowed"; break;
        case 2: cursor = "crosshair"; break;
        case 3: cursor = "pointer"; break;
        case 4: cursor = "text"; break;
        case 5: cursor = "all-scroll"; break;
        case 6: cursor = "ns-resize"; break;
        case 7: cursor = "ew-resize"; break;
      }
    }
    Module.tau_emsc_target.style.cursor = cursor;
  }
});
static void
sys__ems_on_frame(void) {
  _sys.txt_mod = !!_sys.txt_len;
  if (_ems.ren.dlBegin) {
    _ems.ren.dlBegin(&_sys);
  }
  if (_ems.app.dlEntry)  {
    _ems.app.dlEntry(&_sys);
  }
  if (_ems.ren.dlEnd) {
    _ems.ren.dlEnd(&_sys);
  }
  _sys.seq++;
  _sys.style_mod = 0;
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
  if (_sys.cursor != _ems.cursor) {
    sys_ems_js_set_cursor(_sys.cursor, 1);
    _ems.cursor = _sys.cursor;
  }
#if 0
  unsigned long long tooltip_id = str_hash(_sys.tooltip.str);
  if (tooltip_id != _mac.tooltip) {
    if (_sys.tooltip.str.len) {
      NSString *str = [[NSString alloc]
        initWithBytes: (const void*)_sys.tooltip.str.str
        length:(NSUInteger)_sys.tooltip.str.len encoding:NSUTF8StringEncoding];
      [_mac.view setToolTip: str];
    } else {
      [_mac.view setToolTip: nil];
    }
    _mac.tooltip = tooltip_id;
  }
  _sys.tooltip.str = str_nil;
#endif
  sys__ems_update_keyboard_state();
  if (_sys.quit) {
    sys_ems_unreg_evt_cb();
  }
}
extern int
main(int argc, char **argv) {
  _sys.app = 0;
  _sys.platform = &_ems;
  _sys.argc = argc;
  _sys.argv = argv;

  /* javascript  */
  char canvas_name[] = "#canvas";
  mcpy(_ems.html5_canvas_selector, canvas_name, cntof(canvas_name));
  sys_ems_js_init(&_ems.html5_canvas_selector[1]);

  /* memory */
  _sys.mem.alloc = sys_ems_mem_alloc;
  _sys.mem.free = sys_ems_mem_free;
  _sys.mem.info = sys_ems_mem_stats;
  _sys.mem.free_tag = sys_ems_mem_free_tag;
  _sys.mem.arena = &_ems.mem;
  _sys.mem.tmp = &_ems.tmp;
  _sys.mem.page_siz = 4*1024;
  _sys.mem.phy_siz = 0;
  lst_init(&_ems.mem_blks);

  /* console */
  _sys.con.log = sys__ems_log;
  _sys.con.warn = sys__ems_warn;
  _sys.con.err = sys__ems_err;

  /* renderer */
  double w, h;
  emscripten_get_element_css_size(_ems.html5_canvas_selector, &w, &h);
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, false, sys__ems_resize_cb);
  _ems.dpi_scale = emscripten_get_device_pixel_ratio();
  _sys.win.w = math_roundi(w);
  _sys.win.h = math_roundi(h);
  _sys.ren_target.resized = 1;
  _sys.ren_target.w = math_roundi(w * _ems.dpi_scale);
  _sys.ren_target.h = math_roundi(h * _ems.dpi_scale);
  emscripten_set_canvas_element_size(_ems.html5_canvas_selector, _sys.ren_target.w, _sys.ren_target.h);

  _mac.ren.dlInit = ren_init;
  _mac.ren.dlBegin = ren_begin;
  _mac.ren.dlEnd = ren_end;
  _mac.ren.dlShutdown = ren_shutdown;

#if 0
  /* app */
  _ems.app.dlRegister = app_on_api;
  _ems.app.dlEntry = app_run;
  _ems.app.dlRegister(&_sys);
#endif

  sys_ems_reg_evt_cb();
  return 0;
}

