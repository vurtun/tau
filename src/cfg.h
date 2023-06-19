// clang-format off

#if 1
#define CFG_COLOR_SCHEME        GUI_COL_SCHEME_SYS
#define CFG_COLOR_TEXT          0xFFE0E0E0
#elif 0
#define CFG_COLOR_SCHEME        GUI_COL_SCHEME_DARK
#define CFG_COLOR_TEXT          0xFFE0E0E0
#elif 0
#define CFG_COLOR_SCHEME        GUI_COL_SCHEME_STEAM
#define CFG_COLOR_TEXT          0xFFF0F0F0
#else
#define CFG_COLOR_SCHEME        GUI_COL_SCHEME_GRAY
#define CFG_COLOR_TEXT          0xFF101010
#endif

/* shortcuts  */
static const struct app_ui_shortcut app_ui_key_tbl[GUI_KEY_CNT] = {
  [GUI_KEY_ACT]                = {.key = {SYS_KEY_RETURN, 0}},
  [GUI_KEY_DEACT]              = {.key = {SYS_KEY_ESCAPE, 0}},
  [GUI_KEY_NEXT_WIDGET]        = {.key = {SYS_KEY_TAB, (unsigned)-1}},
  [GUI_KEY_PREV_WIDGET]        = {.key = {SYS_KEY_TAB, SYS_KEYMOD_CTRL}},
  [GUI_KEY_TAB_NEXT]           = {.key = {SYS_KEY_LEFT, 0}},
  [GUI_KEY_TAB_PREV]           = {.key = {SYS_KEY_RIGHT, 0}},
  [GUI_KEY_SPIN_INC]           = {.key = {SYS_KEY_UP, 0}},
  [GUI_KEY_SPIN_DEC]           = {.key = {SYS_KEY_DOWN, 0}},
  [GUI_KEY_MENU_NXT]           = {.key = {SYS_KEY_RIGHT, 0}},
  /* list */
  [GUI_KEY_LST_LEFT]           = {.key = {SYS_KEY_LEFT, 0}},
  [GUI_KEY_LST_RIGHT]          = {.key = {SYS_KEY_RIGHT, 0}},
  [GUI_KEY_LST_UP]             = {.key = {SYS_KEY_UP,0}},
  [GUI_KEY_LST_DN]             = {.key = {SYS_KEY_DOWN,0}},
  [GUI_KEY_LST_SEL]            = {.key = {SYS_KEY_SPACE,0}},
  [GUI_KEY_LST_SEL_ALL]        = {.key = {'a',SYS_KEYMOD_CTRL}},
  [GUI_KEY_LST_CPY]            = {.key = {'c',SYS_KEYMOD_CTRL}},
  [GUI_KEY_LST_CUT]            = {.key = {'x',SYS_KEYMOD_CTRL}},
  [GUI_KEY_LST_PASTE]          = {.key = {'v',SYS_KEYMOD_CTRL}},
  [GUI_KEY_LST_DEL]            = {.key = {SYS_KEY_DEL,0}},
  [GUI_KEY_LST_RET]            = {.key = {SYS_KEY_RETURN,0}},
  /* tree */
  [GUI_KEY_TREE_EXPAND]        = {.key = {SYS_KEY_PLUS, 0}},
  [GUI_KEY_TREE_COLLAPSE]      = {.key = {SYS_KEY_MINUS, 0}},
  [GUI_KEY_TREE_LEFT]          = {.key = {SYS_KEY_LEFT, 0}},
  [GUI_KEY_TREE_RIGHT]         = {.key = {SYS_KEY_RIGHT, 0}},
  /* scroll */
  [GUI_KEY_SCRL_PGDN]          = {.key = {SYS_KEY_PGDN,0}},
  [GUI_KEY_SCRL_PGUP]          = {.key = {SYS_KEY_PGUP,0}},
  [GUI_KEY_SCRL_BEGIN]         = {.key = {SYS_KEY_HOME,0}},
  [GUI_KEY_SCRL_END]           = {.key = {SYS_KEY_END,0}},
  /* edit */
  [GUI_KEY_EDIT_SEL_ALL]       = {.key = {'a',SYS_KEYMOD_CTRL}},
  [GUI_KEY_EDIT_CUR_LEFT]      = {.key = {SYS_KEY_LEFT,(unsigned)-1}},
  [GUI_KEY_EDIT_CUR_RIGHT]     = {.key = {SYS_KEY_RIGHT,(unsigned)-1}},
  [GUI_KEY_EDIT_CUR_UP]        = {.key = {SYS_KEY_UP, (unsigned)-1}},
  [GUI_KEY_EDIT_CUR_DOWN]      = {.key = {SYS_KEY_DOWN, (unsigned)-1}},
  [GUI_KEY_EDIT_SEL_CUR_LEFT]  = {.key = {SYS_KEY_LEFT,SYS_KEYMOD_SHIFT}},
  [GUI_KEY_EDIT_SEL_CUR_RIGHT] = {.key = {SYS_KEY_RIGHT,SYS_KEYMOD_SHIFT}},
  [GUI_KEY_EDIT_SEL_CUR_UP]    = {.key = {SYS_KEY_UP,SYS_KEYMOD_SHIFT}},
  [GUI_KEY_EDIT_SEL_CUR_DOWN]  = {.key = {SYS_KEY_DOWN,SYS_KEYMOD_SHIFT}},
  [GUI_KEY_EDIT_DELETE]        = {.key = {SYS_KEY_DEL,0}},
  [GUI_KEY_EDIT_REMOVE]        = {.key = {SYS_KEY_BACKSPACE,0}},
  [GUI_KEY_EDIT_START]         = {.key = {SYS_KEY_HOME,(unsigned)-1}},
  [GUI_KEY_EDIT_END]           = {.key = {SYS_KEY_END,(unsigned)-1}},
  [GUI_KEY_EDIT_SEL_START]     = {.key = {SYS_KEY_HOME,SYS_KEYMOD_SHIFT}},
  [GUI_KEY_EDIT_SEL_END]       = {.key = {SYS_KEY_END,SYS_KEYMOD_SHIFT}},
  [GUI_KEY_EDIT_COMMIT]        = {.key = {SYS_KEY_RETURN,0}},
  [GUI_KEY_EDIT_ABORT]         = {.key = {SYS_KEY_ESCAPE,0}},
  [GUI_KEY_EDIT_COPY]          = {.key = {'c', SYS_KEYMOD_CTRL}},
  [GUI_KEY_EDIT_CUT]           = {.key = {'x', SYS_KEYMOD_CTRL}},
  [GUI_KEY_EDIT_PASTE]         = {.key = {'v', SYS_KEYMOD_CTRL}},
  [GUI_KEY_EDIT_UNDO]          = {.key = {'z', SYS_KEYMOD_CTRL}},
  [GUI_KEY_EDIT_REDO]          = {.key = {'z', SYS_KEYMOD_CTRL|SYS_KEYMOD_SHIFT}},
  [GUI_KEY_EDIT_CLR_MODE]      = {.key = {SYS_KEY_ESCAPE, 0}},
  [GUI_KEY_EDIT_WORD_LEFT]     = {.key = {SYS_KEY_LEFT, SYS_KEYMOD_CTRL}},
  [GUI_KEY_EDIT_WORD_RIGHT]    = {.key = {SYS_KEY_RIGHT, SYS_KEYMOD_CTRL}},
  [GUI_KEY_EDIT_SEL_WORD_LEFT] = {.key = {SYS_KEY_LEFT, SYS_KEYMOD_CTRL|SYS_KEYMOD_SHIFT}},
  [GUI_KEY_EDIT_SEL_WORD_RIGHT]= {.key = {SYS_KEY_RIGHT, SYS_KEYMOD_CTRL|SYS_KEYMOD_SHIFT}},
};
// clang-format on

