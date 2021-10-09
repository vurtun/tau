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
#include "std.h"
#include "dbg.h"
#include "ren.h"
#include "sys.h"
#include "res.h"
#include "gui.h"

/* ---------------------------------------------------------------------------
 *                              File Picker
 * ---------------------------------------------------------------------------*/
enum app_file_type { APP_FILE_DEFAULT, APP_FILE_FOLDER };
enum app_file_groups {
  APP_FILE_GRP_OTHER,
  APP_FILE_GRP_DIR,
  APP_FILE_GRP_TXT,
  APP_FILE_GRP_PROG,
  APP_FILE_GRP_AUDIO,
  APP_FILE_GRP_VIDEO,
  APP_FILE_GRP_IMAGE,
  APP_FILE_GRP_FONT,
  APP_FILE_GRP_3D,
  APP_FILE_GRP_EXEC,
  APP_FILE_GRP_ARCHIVE,
  APP_FILE_GRP_CNT,
};
struct app_file_def {
  const char *suffix;
  const char *name;
  const char *mime;
  int grp;
};
struct app_file_group_def {
  const char *name;
  const char *description;
  const char *icon;
  const struct app_file_def *files;
  int cnt;
};
#define FS_FILE_PERM_LEN 10
struct app_file_elm {
  const struct app_file_def *type;
  struct str fullpath;
  struct str path;
  struct str name;
  struct str ext;

  mode_t mode;
  off_t size;
  time_t mtime;

  const char *ico;
  char perms[FS_FILE_PERM_LEN];

  unsigned isdir : 1;
  unsigned islnk : 1;
  unsigned issock : 1;
  unsigned isfifo : 1;
  unsigned isvalid : 1;
};
typedef int (*app_tbl_sort_f)(const void *a, const void *b);
struct app_file_tbl_col_def {
  struct str title;
  struct gui_split_lay_slot ui;
  app_tbl_sort_f sort[2];
};
enum app_file_tbl_hdr_col {
  APP_FILE_TBL_NAME,
  APP_FILE_TBL_TYPE,
  APP_FILE_TBL_SIZE,
  APP_FILE_TBL_PERM,
  APP_FILE_TBL_DATE,
  APP_FILE_TBL_MAX,
};
struct app_file_tbl {
  int cnt;
  struct gui_tbl_sort sort;
  int state[GUI_TBL_CAP(APP_FILE_TBL_MAX)];
};
enum app_file_view_state {
  APP_FILE_VIEW_LIST,
  APP_FILE_VIEW_MENU,
};
enum app_file_con_menu {
  APP_FILE_CON_MENU_MAIN,
  APP_FILE_CON_MENU_VIEW,
  APP_FILE_CON_MENU_EDIT,
};
#define APP_MAX_FILTER 64
struct app_file_list_view {
  unsigned rev;
  struct arena mem;

  enum app_file_view_state state;
  enum app_file_con_menu con;

  dyn(struct app_file_elm) elms;
  dyn(unsigned long) fltr;
  int sel_idx;

  double off[2];
  struct app_file_tbl tbl;

  /* navigation */
  dyn(char) full_path;
  dyn(char) nav_path;
  dyn(char) fnd_buf;

  struct gui_txt_ed nav_ed;
  struct gui_txt_ed fnd_ed;
};
struct app_file_tree_node {
  struct app_file_tree_node *parent;
  struct lst_elm hook;
  struct lst_elm sub;

  uintptr_t id;
  struct str fullpath;
  int depth;
};
struct app_file_tree_view {
  unsigned rev;
  struct arena mem;
  struct app_file_tree_node root;
  dyn(struct app_file_tree_node*) lst;
  struct lst_elm del_lst;
  uintptr_t *exp;

  int sel;
  double off[2];

  unsigned jmp:1;
  unsigned long jmp_to;
};
#define APP_FILE_SPLIT_MAX 2
struct app_file_view {
  int state;
  struct arena *tmp_arena;
  struct str home;
  int split[GUI_SPLIT_CAP(APP_FILE_SPLIT_MAX)];

  unsigned lst_rev;
  struct app_file_list_view lst;

  unsigned tree_rev;
  struct app_file_tree_view tree;
};

/* ---------------------------------------------------------------------------
 *                                  App
 * ---------------------------------------------------------------------------*/
struct app;
struct gui_app_key {
  int code;
  unsigned mod;
};
struct app_ui_shortcut {
  struct gui_app_key key;
  struct gui_app_key alt;
};
enum app_key_id {
  APP_KEY_NONE,
  APP_KEY_CNT
};
enum app_op_id {
  APP_OP_QUIT,
  APP_OP_CNT
};
union app_param {
  int i;
  float f;
  const char *s;
  void *p;
  enum app_key_id k;
};
typedef void(*app_op_f)(struct app*, const union app_param*);
struct app_op {
  struct gui_app_key key;
  struct gui_app_key alt;
  app_op_f handler;
  union app_param arg;
};
enum app_state {
  APP_STATE_FILE,
};
struct app {
  struct res res;
  struct gui_ctx gui;

  int quit;
  enum app_state state;
  unsigned long ops[bits_to_long(APP_KEY_CNT)];

  struct app_file_view file;
  dyn(char) file_path;
};
static void app_op_quit(struct app* app, const union app_param *arg);

#include "cfg.h"
#include "fmt.c"
#include "std.c"

static struct res_api res;
static struct gui_api gui;

/* ============================================================================
 *
 *                              File Picker
 *
 * ===========================================================================
 */
// clang-format off
static const struct app_file_def app_file_unkown_defs[] = {{"","Unknown","application/octet-stream",APP_FILE_GRP_OTHER}};
static const struct app_file_def app_file_folder_defs[] = {{NULL,"Folder","application/octet-stream",APP_FILE_GRP_DIR}};
static const struct app_file_def app_file_link_defs[] = {{NULL,"Link","application/octet-stream",APP_FILE_GRP_DIR}};
static const struct app_file_def app_file_sock_defs[] = {{NULL,"Socket","application/octet-stream",APP_FILE_GRP_DIR}};
static const struct app_file_def app_file_fifo_defs[] = {{NULL,"Fifo","application/octet-stream",APP_FILE_GRP_DIR}};
static const struct app_file_def app_file_text_defs[] = {
  {.suffix = "txt",   .name = "Text",                         .mime = "text/plain",                   .grp = APP_FILE_GRP_TXT},
  {.suffix = "html",  .name = "Hyper Text Markup Language",   .mime = "text/html",                    .grp = APP_FILE_GRP_TXT},
  {.suffix = "css",   .name = "CSS",                          .mime = "text/css",                     .grp = APP_FILE_GRP_TXT},
  {.suffix = "md",    .name = "Markdown",                     .mime = "text/plain",                   .grp = APP_FILE_GRP_TXT},
  {.suffix = "json",  .name = "JavaScript Object Notation",   .mime = "application/json",             .grp = APP_FILE_GRP_TXT},
  {.suffix = "xml",   .name = "Extensive Markup Language",    .mime = "application/xml",              .grp = APP_FILE_GRP_TXT},
  {.suffix = "pdf",   .name = "Portable Document Format",     .mime = "application/pdf",              .grp = APP_FILE_GRP_TXT},
  {.suffix = "csv",   .name = "Comma-separated Values",       .mime = "text/csv",                     .grp = APP_FILE_GRP_TXT},
  {.suffix = "chm",   .name = "Compiled HTML Help",           .mime = "application/vnd.ms-htmlhelp",  .grp = APP_FILE_GRP_TXT},
};
static const struct app_file_def app_file_prog_defs[] = {
  {.suffix = "c",     .name = "C Source",         .mime = "text/x-c",                 .grp = APP_FILE_GRP_TXT},
  {.suffix = "cpp",   .name = "C++ Source",       .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "cxx",   .name = "C++ Source",       .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "m",     .name = "ObjC Source",      .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "h",     .name = "C Header",         .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "hpp",   .name = "C++ Header",       .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "hxx",   .name = "C++ Header",       .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "py",    .name = "Python Script",    .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "rb",    .name = "Ruby Script",      .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "java",  .name = "Java Source",      .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "lua",   .name = "Lua Script",       .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "js",    .name = "JavaScript",       .mime = "application/javascript",   .grp = APP_FILE_GRP_TXT},
  {.suffix = "php",   .name = "PHP Script",       .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "r",     .name = "R Script",         .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "tcl",   .name = "TCL Script",       .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "pl",    .name = "Perl Script",      .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "ada",   .name = "Ada Source",       .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "asm",   .name = "Assembly Source",  .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "clj",   .name = "Clojure Source",   .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "cs",    .name = "C# Source",        .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "d",     .name = "D Source",         .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "el",    .name = "Emacs Lisp",       .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "go",    .name = "Golang Source",    .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
  {.suffix = "hs",    .name = "Haskell Source",   .mime = "text/plain",               .grp = APP_FILE_GRP_TXT},
};
static const struct app_file_def app_file_audio_defs[] = {
  {.suffix = "mp3",   .name = "MP3 Audio File",   .mime = "audio/mpeg",   .grp = APP_FILE_GRP_AUDIO},
  {.suffix = "wav",   .name = "WAV Audio File",   .mime = "audio/x-wav",  .grp = APP_FILE_GRP_AUDIO},
  {.suffix = "ogg",   .name = "OGG Audio File",   .mime = "audio/ogg",    .grp = APP_FILE_GRP_AUDIO},
};
static const struct app_file_def app_file_video_defs[] = {
  {.suffix = "asf",   .name = "ASF Video Container",          .mime = "video/x-ms-asf",               .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "AVI",   .name = "Audio Video Interleave",       .mime = "video/x-msvideo",              .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "BIK",   .name = "Bink Video",                   .mime = "application/octet-stream",     .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "flv",   .name = "Flash Video",                  .mime = "video/x-flv",                  .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "mkv",   .name = "Matroska Video Container",     .mime = "application/octet-stream",     .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "mov",   .name = "QuickTime Video Container",    .mime = "video/quicktime",              .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "mpeg",  .name = "MPEG Video",                   .mime = "video/mpeg",                   .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "mpg",   .name = "MPEG Video",                   .mime = "video/mpeg",                   .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "mp4",   .name = "MPEG Video",                   .mime = "video/mp4",                    .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "mpe",   .name = "MPEG Video",                   .mime = "video/mpeg",                   .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "thp",   .name = "Nintendo Video",               .mime = "application/octet-stream",     .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "roq",   .name = "Quake 3 Video",                .mime = "application/octet-stream",     .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "swf",   .name = "Macromedia Flash",             .mime = "application/octet-stream",     .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "webm",  .name = "Web Video",                    .mime = "application/octet-stream",     .grp = APP_FILE_GRP_VIDEO},
  {.suffix = "ogg",   .name = "Ogg Video",                    .mime = "video/ogg",                    .grp = APP_FILE_GRP_VIDEO},
};
static const struct app_file_def app_file_images_defs[] = {
  {.suffix = "bmp",   .name = "Bitmap",                   .mime = "image/bmp",                  .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "pbm",   .name = "Portable Bitmap",          .mime = "image/x-portable-bitmap",    .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "png",   .name = "PNG",                      .mime = "image/png",                  .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "jpg",   .name = "JPEG",                     .mime = "image/jpeg",                 .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "jp2",   .name = "JPEG 2000",                .mime = "image/jpeg",                 .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "pcx",   .name = "Picture Exchange Format",  .mime = "image/x-pcx",                .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "tga",   .name = "Truevision TGA",           .mime = "application/octet-stream",   .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "gif",   .name = "Graphic Interchange",      .mime = "image/gif",                  .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "psd",   .name = "Adobe Photoshop",          .mime = "application/octet-stream",   .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "raw",   .name = "Raw Image Data",           .mime = "application/octet-stream",   .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "tif",   .name = "Tagged Image Format",      .mime = "image/tiff",                 .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "xbm",   .name = "X Pixmap",                 .mime = "image/x-xbitmap",            .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "xpm",   .name = "X Pixmap",                 .mime = "image/x-xpixmap",            .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "xcf",   .name = "GIMP Image",               .mime = "application/octet-stream",   .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "ico",   .name = "Windows Icon",             .mime = "image/x-icon",               .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "bti",   .name = "Nintendo Texture",         .mime = "application/octet-stream",   .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "blp",   .name = "Blizzard Texture",         .mime = "application/octet-stream",   .grp = APP_FILE_GRP_IMAGE},
  {.suffix = "vtf",   .name = "Valve Texture",            .mime = "application/octet-stream",   .grp = APP_FILE_GRP_IMAGE},
};
static const struct app_file_def app_file_3d_defs[] = {
  {.suffix = "3ds",   .name = "Legacy 3DS Max Model", .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "b3d",   .name = "Blitz3D Model",        .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "blend", .name = "Blender Model",        .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "bmd",   .name = "Nintendo 3d Model",    .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "bdl",   .name = "Nintendo 3d Model",    .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "brres", .name = "Nintendo 3d Model",    .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "c4d",   .name = "Cinema 4d Model",      .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "cob",   .name = "Caligary Object",      .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "dae",   .name = "Collada Model",        .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "fbx",   .name = "Autodesk FBX",         .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "max",   .name = "Autodesk 3DS Max",     .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "md2",   .name = "Quake 2 Model",        .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "md3",   .name = "Quake 3 Model",        .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "md5",   .name = "Doom 3 Model",         .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "mdx",   .name = "Blizzard Model ",      .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "obj",   .name = "Wavefront .obj",       .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
  {.suffix = "smd",   .name = "Valves Studiomdl",     .mime = "application/octet-stream", .grp = APP_FILE_GRP_3D},
};
static const struct app_file_def app_file_font_defs[] = {
  {.suffix = "ttf",   .name = "TrueType Font",            .mime = "application/x-font-ttf",   .grp = APP_FILE_GRP_FONT},
  {.suffix = "abf",   .name = "Adobe Binary Screen Font", .mime = "application/octet-stream", .grp = APP_FILE_GRP_FONT},
  {.suffix = "afm",   .name = "Adobe Font Metrics",       .mime = "application/octet-stream", .grp = APP_FILE_GRP_FONT},
  {.suffix = "otf",   .name = "OpenType Font",            .mime = "application/x-font-otf",   .grp = APP_FILE_GRP_FONT},
};
static const struct app_file_def app_file_exe_defs[] = {
  {.suffix = "o",     .name = "Unix Object Format",           .mime = "application/octet-stream",                 .grp = APP_FILE_GRP_EXEC},
  {.suffix = "so",    .name = "Shared Object Format",         .mime = "application/octet-stream",                 .grp = APP_FILE_GRP_EXEC},
  {.suffix = "apk",   .name = "Android Application Package",  .mime = "application/vnd.android.package-archive",  .grp = APP_FILE_GRP_EXEC},
  {.suffix = "dll",   .name = "Windows Dynamic Library",      .mime = "application/octet-stream",                 .grp = APP_FILE_GRP_EXEC},
  {.suffix = "exe",   .name = "Windows Executable",           .mime = "application/x-msdownload",                 .grp = APP_FILE_GRP_EXEC},
  {.suffix = "xbe",   .name = "Xbox Executable",              .mime = "application/x-msdownload",                 .grp = APP_FILE_GRP_EXEC},
  {.suffix = "xex",   .name = "Xbox 360 Executable",          .mime = "application/x-msdownload",                 .grp = APP_FILE_GRP_EXEC},
  {.suffix = "sh",    .name = "Shell Script",                 .mime = "application/x-sh",                         .grp = APP_FILE_GRP_EXEC},
};
static const struct app_file_def app_file_archive_defs[] = {
  {.suffix = "zip",   .name = "Zip Archive",                  .mime = "application/zip",              .grp = APP_FILE_GRP_ARCHIVE},
  {.suffix = "7z",    .name = "7Zip Archive",                 .mime = "application/x-7z-compressed",  .grp = APP_FILE_GRP_ARCHIVE},
  {.suffix = "rar",   .name = "RAR Archive",                  .mime = "application/x-rar-compressed", .grp = APP_FILE_GRP_ARCHIVE},
  {.suffix = "gz",    .name = "Compressed TAR Archive",       .mime = "application/x-gtar",           .grp = APP_FILE_GRP_ARCHIVE},
  {.suffix = "bz2",   .name = "Compressed TAR Archive",       .mime = "application/x-bzip2",          .grp = APP_FILE_GRP_ARCHIVE},
  {.suffix = "tar",   .name = "TAR Archive",                  .mime = "application/x-gtar",           .grp = APP_FILE_GRP_ARCHIVE},
  {.suffix = "jar",   .name = "Jave Zipped Container",        .mime = "application/java-archive",     .grp = APP_FILE_GRP_ARCHIVE},
  {.suffix = "pk3",   .name = "Quake 3 Archive",              .mime = "application/zip",              .grp = APP_FILE_GRP_ARCHIVE},
  {.suffix = "pk4",   .name = "Doom 3 Archive",               .mime = "application/zip",              .grp = APP_FILE_GRP_ARCHIVE},
  {.suffix = "mpq",   .name = "Blizzard Archive",             .mime = "application/octet-stream",     .grp = APP_FILE_GRP_ARCHIVE},
  {.suffix = "bin",   .name = "Binary File",                  .mime = "application/octet-stream",     .grp = APP_FILE_GRP_ARCHIVE},
  {.suffix = "iso",   .name = "Optical Media",                .mime = "application/octet-stream",     .grp = APP_FILE_GRP_ARCHIVE},
};
static const struct app_file_group_def app_file_groups[] = {
  [APP_FILE_GRP_OTHER]      = {.name = "Other",         .icon = ICO_FILE,         .files = app_file_unkown_defs,  .cnt = cntof(app_file_unkown_defs)},
  [APP_FILE_GRP_DIR]        = {.name = "Folder",        .icon = ICO_FOLDER_OPEN,  .files = app_file_folder_defs,  .cnt = cntof(app_file_folder_defs)},
  [APP_FILE_GRP_TXT]        = {.name = "Text",          .icon = ICO_FILE_ALT,     .files = app_file_text_defs,    .cnt = cntof(app_file_text_defs)},
  [APP_FILE_GRP_PROG]       = {.name = "Programming",   .icon = ICO_FILE_CODE,    .files = app_file_prog_defs,    .cnt = cntof(app_file_prog_defs)},
  [APP_FILE_GRP_AUDIO]      = {.name = "Audio",         .icon = ICO_FILE_AUDIO,   .files = app_file_audio_defs,   .cnt = cntof(app_file_audio_defs)},
  [APP_FILE_GRP_VIDEO]      = {.name = "Video",         .icon = ICO_FILE_VIDEO,   .files = app_file_video_defs,   .cnt = cntof(app_file_video_defs)},
  [APP_FILE_GRP_IMAGE]      = {.name = "Image",         .icon = ICO_FILE_IMAGE,   .files = app_file_images_defs,  .cnt = cntof(app_file_images_defs)},
  [APP_FILE_GRP_FONT]       = {.name = "Font",          .icon = ICO_FONT,         .files = app_file_font_defs,    .cnt = cntof(app_file_font_defs)},
  [APP_FILE_GRP_3D]         = {.name = "3DModel",       .icon = ICO_CUBES,        .files = app_file_3d_defs,      .cnt = cntof(app_file_3d_defs)},
  [APP_FILE_GRP_EXEC]       = {.name = "Executable",    .icon = ICO_FILE,         .files = app_file_exe_defs,     .cnt = cntof(app_file_exe_defs)},
  [APP_FILE_GRP_ARCHIVE]    = {.name = "Archive",       .icon = ICO_FILE_ARCHIVE, .files = app_file_archive_defs, .cnt = cntof(app_file_archive_defs)},
};
// clang-format on

static int file_cmp_asc(const void *a, const void *b);
static int file_cmp_desc(const void *a, const void *b);
static int file_cmp_name_asc(const void *a, const void *b);
static int file_cmp_name_desc(const void *a, const void *b);
static int file_cmp_type_asc(const void *a, const void *b);
static int file_cmp_type_desc(const void *a, const void *b);
static int file_cmp_size_asc(const void *a, const void *b);
static int file_cmp_size_desc(const void *a, const void *b);
static int file_cmp_perm_asc(const void *a, const void *b);
static int file_cmp_perm_desc(const void *a, const void *b);
static int file_cmp_time_asc(const void *a, const void *b);
static int file_cmp_time_desc(const void *a, const void *b);

// clang-format off
static const struct app_file_tbl_col_def app_file_tbl_def[APP_FILE_TBL_MAX] = {
  [APP_FILE_TBL_NAME] =  {.title = strv("Name"),           .sort = {file_cmp_asc,      file_cmp_desc},       .ui = {.type = GUI_LAY_SLOT_DYN, .size =   1, .con = {100, 400}}},
  [APP_FILE_TBL_TYPE] =  {.title = strv("Type"),           .sort = {file_cmp_type_asc, file_cmp_type_desc},  .ui = {.type = GUI_LAY_SLOT_FIX, .size =  50, .con = { 50, 400}}},
  [APP_FILE_TBL_SIZE] =  {.title = strv("Size"),           .sort = {file_cmp_size_asc, file_cmp_size_desc},  .ui = {.type = GUI_LAY_SLOT_FIX, .size =  60, .con = { 50, 400}}},
  [APP_FILE_TBL_PERM] =  {.title = strv("Permission"),     .sort = {file_cmp_perm_asc, file_cmp_perm_desc},  .ui = {.type = GUI_LAY_SLOT_FIX, .size =  80, .con = { 50, 400}}},
  [APP_FILE_TBL_DATE] =  {.title = strv("Date Modified"),  .sort = {file_cmp_time_asc, file_cmp_time_desc},  .ui = {.type = GUI_LAY_SLOT_FIX, .size = 150, .con = {100, 400}}},
};
static const struct gui_split_lay_slot app_file_split_def[APP_FILE_SPLIT_MAX] = {
  {.type = GUI_LAY_SLOT_FIX, .size = 200, .con = {100, 800}},
  {.type = GUI_LAY_SLOT_DYN, .size =   1, .con = { 50, 800}},
};
// clang-format on

static const struct app_file_def *
file_type(struct str ext) {
  for (int i = 0; i < cntof(app_file_groups); ++i) {
    const struct app_file_group_def *grp = app_file_groups + i;
    const struct app_file_def *file_defs = grp->files;
    for (int j = 0; j < grp->cnt; ++j) {
      const struct app_file_def *def = file_defs + j;
      if (def->suffix && str_cmp(str0(def->suffix), ext) == 0) {
        return def;
      }
    }
  }
  return app_file_unkown_defs;
}
static const char*
file_icon(const struct app_file_def *type) {
  assert(type);
  assert(type->grp >= 0 && type->grp < cntof(app_file_groups));
  return app_file_groups[type->grp].icon;
}
static void
file_perm(char *mod, mode_t perm) {
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
file_cmp_asc(const void *a, const void *b) {
  const struct app_file_elm *fa = (const struct app_file_elm *)a;
  const struct app_file_elm *fb = (const struct app_file_elm *)b;
  if (fa->isdir && !fb->isdir) {
    return -1;
  } else if (!fa->isdir && fb->isdir) {
    return 1;
  }
  return str_cmp(fa->name, fb->name);
}
static int
file_cmp_desc(const void *a, const void *b) {
  const struct app_file_elm *fa = (const struct app_file_elm *)a;
  const struct app_file_elm *fb = (const struct app_file_elm *)b;
  if (fa->isdir && !fb->isdir) {
    return 1;
  } else if (!fa->isdir && fb->isdir) {
    return -1;
  }
  return str_cmp(fb->name, fa->name);
}
static int
file_cmp_name_asc(const void *a, const void *b) {
  const struct app_file_elm *fa = (const struct app_file_elm *)a;
  const struct app_file_elm *fb = (const struct app_file_elm *)b;
  return str_cmp(fa->name, fb->name);
}
static int
file_cmp_name_desc(const void *a, const void *b) {
  const struct app_file_elm *fa = (const struct app_file_elm *)a;
  const struct app_file_elm *fb = (const struct app_file_elm *)b;
  return str_cmp(fb->name, fa->name);
}
static int
file_cmp_type_asc(const void *a, const void *b) {
  const struct app_file_elm *fa = (const struct app_file_elm *)a;
  const struct app_file_elm *fb = (const struct app_file_elm *)b;
  if (fa->isdir && !fb->isdir) {
    return -1;
  } else if (!fa->isdir && fb->isdir) {
    return 1;
  } else if (fa->type == fb->type) {
    return str_cmp(fa->name, fb->name);
  }
  return strcmp(fa->type->suffix, fb->type->suffix);
}
static int
file_cmp_type_desc(const void *a, const void *b) {
  const struct app_file_elm *fa = (const struct app_file_elm *)a;
  const struct app_file_elm *fb = (const struct app_file_elm *)b;
  if (fa->isdir && !fb->isdir) {
    return 1;
  } else if (!fa->isdir && fb->isdir) {
    return -1;
  } else if (fa->type == fb->type) {
    return str_cmp(fa->name, fb->name);
  }
  return strcmp(fb->type->suffix, fa->type->suffix);
}
static int
file_cmp_size_asc(const void *a, const void *b) {
  const struct app_file_elm *fa = (const struct app_file_elm *)a;
  const struct app_file_elm *fb = (const struct app_file_elm *)b;
  return cast(int, fa->size - fb->size);
}
static int
file_cmp_size_desc(const void *a, const void *b) {
  const struct app_file_elm *fa = (const struct app_file_elm *)a;
  const struct app_file_elm *fb = (const struct app_file_elm *)b;
  return cast(int, fb->size - fa->size);
}
static int
file_cmp_perm_asc(const void *a, const void *b) {
  const struct app_file_elm *fa = (const struct app_file_elm *)a;
  const struct app_file_elm *fb = (const struct app_file_elm *)b;
  unsigned fa_perm = fa->mode & S_IRWXU;
  unsigned fb_perm = fb->mode & S_IRWXU;
  return cast(int, fa_perm - fb_perm);
}
static int
file_cmp_perm_desc(const void *a, const void *b) {
  const struct app_file_elm *fa = (const struct app_file_elm *)a;
  const struct app_file_elm *fb = (const struct app_file_elm *)b;
  unsigned fa_perm = fa->mode & S_IRWXU;
  unsigned fb_perm = fb->mode & S_IRWXU;
  return cast(int, fb_perm - fa_perm);
}
static int
file_cmp_time_asc(const void *a, const void *b) {
  const struct app_file_elm *fa = (const struct app_file_elm *)a;
  const struct app_file_elm *fb = (const struct app_file_elm *)b;
  return cast(int, fa->mtime - fb->mtime);
}
static int
file_cmp_time_desc(const void *a, const void *b) {
  const struct app_file_elm *fa = (const struct app_file_elm *)a;
  const struct app_file_elm *fb = (const struct app_file_elm *)b;
  return cast(int, fb->mtime - fa->mtime);
}
static struct lst_elm*
app_file_tree_node_sort_after_elm(struct app_file_tree_node *n,
                                  struct app_file_tree_node *s) {
  assert(s);
  assert(n);

  struct lst_elm *elm = 0;
  for_lst(elm, &n->sub) {
    struct app_file_tree_node *it;
    it = lst_get(elm, struct app_file_tree_node, hook);
    if (str_cmp(s->fullpath, it->fullpath) < 0) {
      break;
    }
  }
  return elm;
}
static void
app_file_tree_node_lnk(struct app_file_tree_node *n,
                       struct app_file_tree_node *s) {
  assert(s);
  assert(n);

  struct lst_elm *elm = app_file_tree_node_sort_after_elm(n, s);
  lst_del(&s->hook);
  lst_init(&s->hook);
  lst__add(&s->hook, elm->prv, elm);
}
static void
app_file_tree_node_setup(struct sys *sys, struct arena *mem,
                         struct app_file_tree_node *s,
                         struct app_file_tree_node *n, struct str p,
                         unsigned long long id) {
  assert(sys);
  assert(mem);
  assert(s);
  assert(n);

  s->id = id;
  s->parent = n;
  s->depth = n->depth + 1;
  s->fullpath = arena_str(mem, sys, p);

  lst_init(&s->hook);
  lst_init(&s->sub);
  app_file_tree_node_lnk(n, s);
}
static struct app_file_tree_node*
app_file_node_alloc(struct app_file_tree_view *tree, struct sys *sys,
                    struct arena *mem) {
  assert(tree);
  assert(sys);
  assert(mem);

  struct app_file_tree_node *s = 0;
  if (lst_has(&tree->del_lst)) {
    s = lst_get(tree->del_lst.nxt, struct app_file_tree_node, hook);
    lst_del(&s->hook);
    memset(s, 0, sizeof(*s));
  } else {
    s = arena_alloc(mem, sys, szof(*s));
  }
  return s;
}
static struct app_file_tree_node*
app_file_view_tree_node_new(struct app_file_tree_view *tree, struct sys *sys,
                            struct arena *mem, struct app_file_tree_node *n,
                            struct str path, unsigned long long id) {
  assert(tree);
  assert(sys);
  assert(mem);
  assert(n);

  struct app_file_tree_node *s = 0;
  s = app_file_node_alloc(tree, sys, mem);
  app_file_tree_node_setup(sys, mem, s, n, path, id);
  return s;
}
static void
app_file_view_tree_node_del(struct app_file_tree_view *tree,
                            struct app_file_tree_node *n) {
  assert(n);
  assert(tree);

  struct lst_elm *elm = 0;
  struct lst_elm *item = 0;
  for_lst_safe(elm, item, &n->sub) {
    struct app_file_tree_node *s = 0;
    s = lst_get(elm, struct app_file_tree_node, hook);
    app_file_view_tree_node_del(tree, s);
  }
  lst_del(&n->hook);
  lst_init(&n->hook);
  lst_add(&tree->del_lst, &n->hook);
}
static int
app_file_view_tree_build(struct app_file_tree_view *tree,
                         struct app_file_tree_node *n, struct sys *sys,
                         struct arena *mem, struct arena *tmp) {
  assert(tree);
  assert(sys);
  assert(mem);
  assert(tmp);
  assert(n);

  int upt = 0;
  if (n->parent && !set_fnd(tree->exp, n->parent->id)) {
    return upt;
  }
  struct scope scp = {0};
  scope_begin(&scp, tmp);
  {
    uintptr_t *set = arena_set(tmp, sys, 1024);
    struct lst_elm *elm = 0;
    for_lst(elm, &n->sub) {
      struct app_file_tree_node *s;
      s = lst_get(elm, struct app_file_tree_node, hook);
      set_put(set, sys, s->id);
    }
    /* validate child nodes */
    struct sys_dir_iter it = {0};
    for (sys->dir.lst(&it, tmp, n->fullpath); it.valid; sys->dir.nxt(&it, tmp)) {
      if (it.name.str[0] == '.') {
        continue;
      }
      struct stat stats;
      int err = stat(it.fullpath.str, &stats);
      if (err < 0 || !S_ISDIR(stats.st_mode)) {
        continue;
      }
      unsigned long long id = str_hash(it.fullpath);
      if (!set_fnd(set, id)) {
        app_file_view_tree_node_new(tree, sys, mem, n, it.fullpath, id);
        upt = 1;
      }
      set_del(set, id);
    }
    /* remove deleted directory nodes */
    struct lst_elm *safe = 0;
    for_lst_safe(elm, safe, &n->sub) {
      struct app_file_tree_node *s;
      s = lst_get(elm, struct app_file_tree_node, hook);
      if (!set_fnd(set, s->id)) {
        continue;
      }
      app_file_view_tree_node_del(tree, s);
    }
    set_free(set, sys);
  }
  scope_end(&scp, tmp, sys);

  /* recurse child directory nodes */
  struct lst_elm *it = 0;
  for_lst(it, &n->sub) {
    struct app_file_tree_node *s;
    s = lst_get(it, struct app_file_tree_node, hook);
    upt |= app_file_view_tree_build(tree, s, sys, mem, tmp);
  }
  return upt;
}
static struct app_file_tree_node**
app_file_view_tree_serial(struct app_file_tree_view *tree,
                          struct app_file_tree_node *n,
                          struct app_file_tree_node **lst,
                          struct sys *sys) {
  assert(tree);
  assert(lst);
  assert(sys);
  assert(n);

  dyn_add(lst, sys, n);
  if (set_fnd(tree->exp, n->id)) {
    struct lst_elm *it = 0;
    for_lst(it, &n->sub) {
      struct app_file_tree_node *s;
      s = lst_get(it, struct app_file_tree_node, hook);
      lst = app_file_view_tree_serial(tree, s, lst, sys);
    }
  }
  return lst;
}
static void
app_file_tree_update(struct app_file_view *fs, struct sys *sys) {
  assert(fs);
  assert(sys);

  struct arena *tmp = fs->tmp_arena;
  app_file_view_tree_build(&fs->tree, &fs->tree.root, sys, &fs->tree.mem, tmp);
  dyn_clr(fs->tree.lst);
  fs->tree.lst = app_file_view_tree_serial(&fs->tree, &fs->tree.root, fs->tree.lst, sys);
}
static struct app_file_tree_node*
app_file_tree_node_fnd(struct app_file_tree_node *n, struct str str) {
  assert(n);

  struct lst_elm *elm = 0;
  struct app_file_tree_node *s = 0;
  for_lst(elm, &n->sub) {
    s = lst_get(elm, struct app_file_tree_node, hook);
    struct str file = path_file(s->fullpath);
    if (str_eq(str, file)) {
      return s;
    }
  }
  return s;
}
static void
app_file_view_tree_open(struct app_file_view *fs, struct app_file_tree_view *tree,
                        struct sys *sys, struct str p) {
  assert(fs);
  assert(tree);
  assert(sys);

  int off = fs->home.len + 1;
  if (p.len < off) {
    return;
  }
  struct app_file_tree_node *n = &tree->root;
  struct str path = str_rhs(p, off);
  for_str_tok(it, _, path, str0("/")) {
    struct str fullpath = strp(p.str, it.end);
    if (!sys->dir.exists(fullpath, fs->tmp_arena)) {
      break;
    }
    /* create or find node to dig deeper */
    struct app_file_tree_node *s = 0;
    s = app_file_tree_node_fnd(n, it);
    if (!s) {
      unsigned long long id = 0;
      id = str_hash(strp(p.str, it.end));
      s = app_file_view_tree_node_new(tree, sys, &tree->mem, n, fullpath, id);
    }
    tree->jmp = 1;
    tree->jmp_to = s->id;
    set_put(tree->exp, sys, s->id);
    fs->tree_rev++;
    n = s;
  }
  app_file_tree_update(fs, sys);
}
static void
app_file_lst_view_clr(struct app_file_list_view *lst, struct sys *sys) {
  assert(lst);
  assert(sys);

  dyn_free(lst->fltr, sys);
  dyn_free(lst->elms, sys);
  dyn_free(lst->full_path, sys);
  dyn_free(lst->fnd_buf, sys);
  arena_free(&lst->mem, sys);

  zero2(lst->off);
  lst->fltr = arena_dyn(&lst->mem, sys, unsigned long, 128);
  lst->elms = arena_dyn(&lst->mem, sys, struct app_file_elm, 256);
  lst->nav_path = arena_dyn(&lst->mem, sys, char, 1024);
  lst->full_path = arena_dyn(&lst->mem, sys, char, 1024);
  lst->fnd_buf = arena_dyn(&lst->mem, sys, char, APP_MAX_FILTER);

  gui.edt.buf.reset(&lst->fnd_ed);
  gui.edt.buf.reset(&lst->nav_ed);
}
static void
app_file_tree_view_clr(struct app_file_tree_view *tree, struct sys *sys) {
  assert(tree);
  assert(sys);

  dyn_free(tree->lst, sys);
  set_free(tree->exp, sys);
  arena_free(&tree->mem, sys);

  tree->rev = 0;
  tree->lst = 0;
  tree->exp = 0;

  lst_init(&tree->del_lst);
  memset(tree->off, 0, sizeof(tree->off));
}
static void
app_file_lst_view_add_path(struct app_file_list_view *lst, struct sys *sys,
                           struct str path) {
  assert(lst);
  assert(sys);

  struct app_file_elm elm = {0};
  struct scope scp = {0};
  scope_begin(&scp, &lst->mem);

  /* parse file/directory name */
  elm.fullpath = arena_str(&lst->mem, sys, path);
  elm.name = path_file(elm.fullpath);
  elm.path = strp(elm.fullpath.str, elm.name.str);
  elm.ext = str_nil;

  /* extract file stat, extension and type */
  struct stat stats;
  int err = stat(elm.fullpath.str, &stats);
  if (err < 0) {
    scope_end(&scp, &lst->mem, sys);
    return;
  }
  if ((elm.isdir = S_ISDIR(stats.st_mode))) {
    elm.type = app_file_folder_defs;
  } else if ((elm.islnk = S_ISLNK(stats.st_mode))) {
    elm.type = app_file_link_defs;
  } else if ((elm.issock = S_ISSOCK(stats.st_mode))) {
    elm.type = app_file_sock_defs;
  } else if ((elm.isfifo = S_ISFIFO(stats.st_mode))) {
    elm.type = app_file_fifo_defs;
  } else {
    struct str ext = path_ext(elm.name);
    elm.ext = arena_str(&lst->mem, sys, ext);
    elm.type = file_type(elm.ext);
  }
  elm.mode = stats.st_mode;
  elm.size = stats.st_size;
  elm.mtime = stats.st_mtime;
  elm.isvalid = 0;

  /* select icon  */
  elm.ico = file_icon(elm.type);
  file_perm(elm.perms, elm.mode);

  /* add new file info */
  dyn_add(lst->elms, sys, elm);
  dyn_fit(lst->fltr, sys, bits_to_long(dyn_cnt(lst->elms)));
  bit_clr(lst->fltr, dyn_cnt(lst->elms) - 1);
}
static void
app_file_list_view_fltr(struct app_file_list_view *lst, struct str fltr) {
  assert(lst);

  int tbl[UCHAR_MAX + 1];
  str__fnd_tbl(tbl, fltr);
  for (int i = 0; i < dyn_cnt(lst->elms); ++i) {
    if (str_fnd_str(lst->elms[i].name, fltr, tbl) >= fltr.len) {
      bit_set(lst->fltr, i);
    }
  }
}
static void
app_file_view_cd(struct app_file_view *fs, struct sys *sys, struct str path) {
  assert(fs);
  assert(sys);

  struct scope scp = {0};
  struct arena *tmp = fs->tmp_arena;
  scope_begin(&scp, tmp);
  {
    struct str sys_path = arena_str(tmp, sys, path);
    app_file_lst_view_clr(&fs->lst, sys);
    dyn_asn_str(fs->lst.full_path, sys, sys_path);
    dyn_asn_str(fs->lst.nav_path, sys, sys_path);

    /* add all files in directory */
    struct sys_dir_iter it = {0};
    for (sys->dir.lst(&it, tmp, sys_path); it.valid; sys->dir.nxt(&it, tmp)) {
      app_file_lst_view_add_path(&fs->lst, sys, it.fullpath);
    }
    /* sort list by name */
    dyn_sort(fs->lst.elms, file_cmp_asc);
    fs->lst.sel_idx = -1;
  }
  scope_end(&scp, tmp, sys);
  fs->lst.state = APP_FILE_VIEW_LIST;
  fs->lst.con = APP_FILE_CON_MENU_MAIN;
}
static void
app_file_list_view_setup(struct app_file_view *fs, struct sys *sys,
                         struct gui_ctx *ctx) {
  assert(fs);
  assert(sys);
  assert(ctx);

  fs->lst.elms = arena_dyn(&fs->lst.mem, sys, struct app_file_elm, 1024);
  fs->lst.fltr = arena_dyn(&fs->lst.mem, sys, unsigned long, 128);
  fs->lst.nav_path = arena_dyn(&fs->lst.mem, sys, char, 1024);
  fs->lst.full_path = arena_dyn(&fs->lst.mem, sys, char, 1024);
  fs->lst.fnd_buf = arena_dyn(&fs->lst.mem, sys, char, APP_MAX_FILTER);

  /* setup list table */
  struct gui_split_lay_cfg tbl_cfg = {0};
  tbl_cfg.size = sizeof(struct app_file_tbl_col_def);
  tbl_cfg.off = offsetof(struct app_file_tbl_col_def, ui);
  tbl_cfg.slots = app_file_tbl_def;
  tbl_cfg.cnt = APP_FILE_TBL_MAX;
  gui.tbl.lay(fs->lst.tbl.state, ctx, &tbl_cfg);

  /* open home directory */
  app_file_view_cd(fs, sys, fs->home);
}
static void
app_file_tree_view_setup(struct app_file_view *fs, struct sys *sys) {
  assert(fs);
  assert(sys);

  fs->tree.lst = arena_dyn(&fs->tree.mem, sys, struct app_file_tree_node*, 1024);
  fs->tree.exp = arena_set(&fs->tree.mem, sys, 1024);
  lst_init(&fs->tree.del_lst);
  zero2(fs->tree.off);

  /* setup root node */
  fs->tree.root.fullpath = fs->home;
  fs->tree.root.id = str_hash(fs->home);
  lst_init(&fs->tree.root.hook);
  lst_init(&fs->tree.root.sub);
  set_put(fs->tree.exp, sys, fs->tree.root.id);
}
static void
app_file_view_setup(struct app_file_view *fs, struct sys *sys,
                    struct gui_ctx *ctx, struct arena *mem,
                    struct arena *tmp_arena) {
  assert(fs);
  assert(sys);
  assert(ctx);
  assert(mem);
  assert(tmp_arena);

  fs->lst_rev = 1;
  fs->tree_rev = 1;
  fs->tmp_arena = tmp_arena;
  fs->home = arena_str(mem, sys, str0(getenv("HOME")));

  struct gui_split_lay_cfg cfg = {0};
  cfg.slots = app_file_split_def;
  cfg.cnt = APP_FILE_SPLIT_MAX;
  gui.splt.lay.bld(fs->split, ctx, &cfg);

  app_file_list_view_setup(fs, sys, ctx);
  app_file_tree_view_setup(fs, sys);
}
static void
app_file_view_update(struct app_file_view *fs, struct sys *sys) {
  assert(fs);
  assert(sys);

  if (fs->tree_rev != fs->tree.rev) {
    app_file_tree_update(fs, sys);
    fs->tree.rev = fs->tree_rev;
  }
}
static void
app_file_view_free(struct app_file_view *fv, struct sys *sys) {
  assert(fv);
  assert(sys);

  app_file_lst_view_clr(&fv->lst , sys);
  app_file_tree_view_clr(&fv->tree, sys);
}

/* ---------------------------------------------------------------------------
 *                                  GUI
 * ---------------------------------------------------------------------------
 */
static int
ui_btn_menu(struct gui_ctx *ctx, struct gui_btn *btn, struct gui_panel *parent,
            struct str txt, const char *icon, int uline) {
  assert(ctx);
  assert(btn);
  assert(parent);
  assert(icon);

  static const struct gui_align align = {GUI_HALIGN_MID, GUI_VALIGN_MID};
  gui.btn.begin(ctx, btn, parent);
  {
    struct gui_panel lbl = {.box = btn->pan.box};
    lbl.box.x = gui.bnd.shrink(&btn->pan.box.x, ctx->cfg.pad[0]);
    gui.txt.uln(ctx, &lbl, &btn->pan, txt, &align, uline, 1);

    struct gui_icon ico = {0};
    ico.box.x = gui.bnd.max_ext(lbl.box.x.min - ctx->cfg.gap[0], ctx->cfg.ico);
    ico.box.y = gui.bnd.mid_ext(btn->box.y.mid, ctx->cfg.ico);
    gui.ico.icon(ctx, &ico, &btn->pan, icon);
  }
  gui.btn.end(ctx, btn, parent);
  return btn->clk;
}
static void
ui_edit_search(struct gui_ctx *ctx, struct gui_edit_box *edt,
               struct gui_panel *pan, struct gui_panel *parent,
               struct gui_txt_ed *ed, char **buf) {
  assert(ed);
  assert(buf);
  assert(ctx);
  assert(edt);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    static const int pad[2] = {3, 3};
    if (ctx->pass == GUI_RENDER &&
        pan->state != GUI_HIDDEN) {
      gui.edt.drw(ctx, pan);
    }
    /* icon */
    struct gui_icon ico = {0};
    ico.box.y = gui.bnd.shrink(&pan->box.y, pad[1]);
    ico.box.x = gui.bnd.min_ext(pan->box.x.min, ctx->cfg.item);
    gui.ico.icon(ctx, &ico, pan, ICO_SEARCH);

    /* edit */
    edt->pan.focusable = 1;
    edt->pan.box.x = gui.bnd.min_max(ico.box.x.max, pan->box.x.max);
    edt->pan.box.x = gui.bnd.shrink(&edt->pan.box.x, pad[0]);
    edt->pan.box.y = gui.bnd.shrink(&pan->box.y, pad[1]);
    gui.edt.fld(ctx, edt, &edt->pan, pan, ed, buf);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_file_lst_view_fnd(struct app *app, struct app_file_list_view *lst,
                     struct gui_ctx *ctx, struct gui_panel *pan,
                     struct gui_panel *parent) {
  assert(app);
  assert(lst);
  assert(ctx);
  assert(pan);
  assert(parent);

  struct gui_edit_box edt = {.box = pan->box};
  ui_edit_search(ctx, &edt, pan, parent, &lst->fnd_ed, &lst->fnd_buf);
  if (edt.mod) {
    /* filter files by name */
    bit_fill(lst->fltr, 0x00, dyn_cnt(lst->elms));
    if (dyn_has(lst->fnd_buf)) {
      app_file_list_view_fltr(lst, dyn_str(lst->fnd_buf));
    }
  }
}
static void
ui_file_lst_view_nav_bar(struct app *app, struct app_file_view *view,
                         struct app_file_list_view *lst,
                         struct gui_ctx *ctx, struct gui_panel *pan,
                         struct gui_panel *parent) {
  assert(app);
  assert(lst);
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    int gap = ctx->cfg.pan_gap[0];
    struct gui_box lay = pan->box;
    struct gui_btn home = {.box = gui.cut.rhs(&lay, ctx->cfg.item, gap)};
    if (gui.btn.ico(ctx, &home, pan, ICO_HOME)) {
      /* change the directory to home  */
      app_file_view_tree_open(view, &view->tree, ctx->sys, view->home);
      app_file_view_cd(view, ctx->sys, view->home);
    }
    struct gui_btn up = {.box = gui.cut.rhs(&lay, ctx->cfg.item, gap)};
    if (gui.btn.ico(ctx, &up, pan, ICO_ARROW_CIRCLE_UP)) {
      /* go up to parent directory  */
      struct str file = path_file(dyn_str(lst->full_path));
      if (file.str - lst->full_path > view->home.len) {
        app_file_view_cd(view, ctx->sys, strp(lst->full_path, file.str));
      }
    }
    struct gui_edit_box edt = {.box = lay};
    gui.edt.txt(ctx, &edt, pan, &lst->nav_ed, &lst->nav_path);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_file_view_tbl_elm(struct gui_ctx *ctx, struct gui_tbl *tbl,
                     const int *lay, struct gui_panel *elm,
                     const struct app_file_elm *fi, int is_sel) {
  assert(tbl);
  assert(lay);
  assert(elm);
  assert(fi);

  static const struct gui_align algn = {GUI_HALIGN_RIGHT, GUI_VALIGN_MID};
  static const unsigned dir_col = col_rgb_hex(0xeecd4a);
  unsigned long long elm_id = str_hash(fi->fullpath);
  gui.tbl.lst.elm.begin(ctx, tbl, elm, elm_id, is_sel);
  {
    struct gui_cfg_stk stk[1] = {0};
    gui.cfg.pushu_on(stk, &ctx->cfg.col[GUI_COL_ICO], dir_col, fi->isdir);
    {
      /* columns */
      struct tm *mod_time = localtime(&fi->mtime);
      gui.tbl.lst.elm.col.txt(ctx, tbl, lay, elm, fi->name, fi->ico, 0);
      gui.tbl.lst.elm.col.txt(ctx, tbl, lay, elm, str0(fi->type->name), 0, 0);
      gui.tbl.lst.elm.col.txtf(ctx, tbl, lay, elm, &algn, "%zu", fi->size);
      gui.tbl.lst.elm.col.txt(ctx, tbl, lay, elm, str0(fi->perms), 0, 0);
      gui.tbl.lst.elm.col.tm(ctx, tbl, lay, elm, "%d/%m/%Y %H:%M:%S", mod_time);
    }
    gui.cfg.pop_on(stk, fi->isdir);
  }
  gui.tbl.lst.elm.end(ctx, tbl, elm);
}
static void
ui_file_view_tbl(struct app *app, struct app_file_view *fs,
                 struct app_file_list_view *lst, struct gui_ctx *ctx,
                 struct gui_panel *pan, struct gui_panel *parent) {
  assert(fs);
  assert(app);
  assert(lst);
  assert(ctx);
  assert(pan);
  assert(parent);

  int chdir = 0, dir = 0;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, lst->off, &lst->tbl.sort);
    {
      /* header */
      int tbl_lay[GUI_TBL_COL(APP_FILE_TBL_MAX)];
      gui.tbl.hdr.begin(ctx, &tbl, tbl_lay, lst->tbl.state);
      for (int i = 0; i < tbl.cnt; ++i) {
        struct str title = app_file_tbl_def[i].title;
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_lay, lst->tbl.state, title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* sorting */
      if (tbl.resort && lst->elms) {
        dyn_sort(lst->elms, app_file_tbl_def[tbl.sort.col].sort[tbl.sort.order]);
        lst->tbl.sort = tbl.sort;
      }
      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, dyn_cnt(lst->elms));
      cfg.fltr.on = GUI_LST_FLTR_ON_ONE;
      cfg.fltr.bitset = lst->fltr;
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for (int i = tbl.lst.begin; i < tbl.lst.end; i = gui.tbl.lst.nxt(&tbl.lst, i)) {
        struct gui_panel elm = {0};
        int is_sel = lst->sel_idx == i;
        ui_file_view_tbl_elm(ctx, &tbl, tbl_lay, &elm, lst->elms + i, is_sel);

        /* input handling */
        struct gui_input in = {0};
        gui.pan.input(&in, ctx, &elm, GUI_BTN_LEFT|GUI_BTN_RIGHT);
        if (in.mouse.btn.left.doubled) {
          chdir = 1, dir = i;
        }
        if (in.mouse.btn.right.clk) {
          lst->state = APP_FILE_VIEW_MENU;
          gui.in.consume(ctx);
        }
      }
      gui.tbl.lst.end(ctx, &tbl);
      if (tbl.lst.sel.mod) {
        /* selection handling */
        struct app_file_elm *elm = lst->elms + tbl.lst.sel.idx;
        elm->isvalid = !elm->isdir;
        lst->sel_idx = tbl.lst.sel.idx;
      }
    }
    gui.tbl.end(ctx, &tbl, pan, lst->off);
  }
  gui.pan.end(ctx, pan, parent);

  if (chdir) {
    struct app_file_elm *fi = lst->elms + dir;
    if (fi->isdir) {
      app_file_view_tree_open(fs, &fs->tree, ctx->sys, fi->fullpath);
      app_file_view_cd(fs, ctx->sys, fi->fullpath);
      ctx->lst_state.cur_idx = -1;
    }
  }
}
static void
ui_file_view_tree_node(struct gui_ctx *ctx, struct gui_tree_node *node,
                       struct gui_panel *parent, struct app_file_tree_node *n){
  assert(n);
  assert(ctx);
  assert(node);
  assert(parent);

  gui.tree.begin(ctx, node, parent, n->depth);
  {
    struct gui_cfg_stk stk[1] = {0};
    gui.cfg.pushu(stk, &ctx->cfg.col[GUI_COL_ICO], col_rgb_hex(0xeecd4a));
    {
      struct gui_panel lbl = {.box = node->box};
      struct str txt = path_file(n->fullpath);
      gui.ico.box(ctx, &lbl, &node->pan, ICO_FOLDER_OPEN, txt);
    }
    gui.cfg.pop(stk);
  }
  gui.tree.end(ctx, node, parent);
}
static int
ui_file_view_tree_elm(struct gui_ctx *ctx, struct app_file_tree_view *tree,
                      struct app_file_tree_node *n, struct gui_lst_reg *reg,
                      struct gui_panel *elm, int is_sel) {
  assert(n);
  assert(ctx);
  assert(tree);
  assert(reg);
  assert(elm);

  int ret = 0;
  gui.lst.reg.elm.begin(ctx, reg, elm, n->id, is_sel);
  {
    struct gui_tree_node node = {0};
    node.type = lst_has(&n->sub) ? GUI_TREE_NODE : GUI_TREE_LEAF;
    node.open = set_fnd(tree->exp, n->id);
    ui_file_view_tree_node(ctx, &node, elm, n);
    if (node.changed) {
      if (node.open) {
        set_put(tree->exp, ctx->sys, n->id);
      } else {
        set_del(tree->exp, n->id);
      }
      ret = 1;
    }
  }
  gui.lst.reg.elm.end(ctx, reg, elm);
  return ret;
}
static void
ui_file_view_tree_key(struct app_file_view *fs, struct app_file_tree_view *tree,
                      struct gui_ctx *ctx, struct gui_lst *lst) {
  assert(fs);
  assert(tree);
  assert(ctx);
  assert(lst);

  /* tree node expansion */
  int plus = bit_tst_clr(ctx->keys, GUI_KEY_TREE_EXPAND);
  int opening = bit_tst_clr(ctx->keys, GUI_KEY_TREE_RIGHT);
  if (plus || opening) {
    struct app_file_tree_node *n = tree->lst[tree->sel];
    set_put(tree->exp, ctx->sys, n->id);
    fs->tree_rev++;
    return;
  }
  /* tree node collapse and jump to parent node */
  int minus = bit_tst_clr(ctx->keys, GUI_KEY_TREE_COLLAPSE);
  int closing = bit_tst_clr(ctx->keys, GUI_KEY_TREE_LEFT);
  if (minus || closing) {
    struct app_file_tree_node *n = tree->lst[tree->sel];
    if (closing && !set_fnd(tree->exp, n->id)) {
      for (int i = tree->sel; i > 0; --i) {
        int nidx = i - 1;
        struct app_file_tree_node *p = tree->lst[nidx];
        if (p->depth >= n->depth) {
          continue;
        }
        tree->off[1] = gui.lst.lay.clamps(&lst->lay, nidx);
        gui.lst.set_sel_idx(ctx, lst, nidx);
        gui.lst.set_cur_idx(ctx, lst, nidx);
        tree->sel = nidx;
        break;
      }
    } else set_del(tree->exp, n->id);
    fs->tree_rev++;
  }
}
static void
ui_file_view_tree(struct app *app, struct app_file_view *fs,
                  struct app_file_tree_view *tree, struct gui_ctx *ctx,
                  struct gui_panel *pan, struct gui_panel *parent) {
  assert(app);
  assert(fs);
  assert(tree);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    /* tree list */
    struct gui_lst_cfg cfg = {0};
    gui.lst.cfg(&cfg, dyn_cnt(tree->lst), tree->off[1]);
    cfg.sel.src = GUI_LST_SEL_SRC_EXT;

    struct gui_lst_reg reg = {.box = pan->box};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, tree->off);
    for (int i = reg.lst.begin; i < reg.lst.end; ++i) {
      struct gui_panel elm = {0};
      struct app_file_tree_node *n = tree->lst[i];
      if (ui_file_view_tree_elm(ctx, tree, n, &reg, &elm, tree->sel == i)) {
        fs->tree_rev++;
      }
    }
    if (tree->jmp) {
      /* jump to element */
      for (int i = 0; i < dyn_cnt(tree->lst); ++i) {
        struct app_file_tree_node *n = tree->lst[i];
        if (n->id != tree->jmp_to) continue;
        gui.lst.reg.center(&reg, i);
        tree->sel = i;
        break;
      }
      tree->jmp = 0;
    }
    gui.lst.reg.end(ctx, &reg, pan, tree->off);
    if (reg.lst.sel.mod) {
      /* selection */
      tree->sel = reg.lst.sel.idx;
      struct app_file_tree_node *n = tree->lst[tree->sel];
      app_file_view_cd(fs, ctx->sys, n->fullpath);
    }
    if (reg.lst.ctl.has_focus) {
      /* key handling */
      ui_file_view_tree_key(fs, tree, ctx, &reg.lst);
    }
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_file_con_close(struct app_file_list_view *lst) {
  assert(lst);
  lst->state = APP_FILE_VIEW_LIST;
  lst->con = APP_FILE_CON_MENU_MAIN;
}
static void
ui_file_con_menu(struct app *app, struct app_file_list_view *lst,
                 struct gui_ctx *ctx, struct gui_panel *pan,
                 struct gui_panel *parent) {
  unused(app);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_box lay = pan->box;
    switch (lst->con) {
    case APP_FILE_CON_MENU_MAIN: {
      struct gui_btn open = {.box = gui.box.div_y(&lay, ctx->cfg.gap[0], 3, 0)};
      if (ui_btn_menu(ctx, &open, pan, strv("Open"), ICO_FOLDER_OPEN, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn edt = {.box = gui.box.div_y(&lay, ctx->cfg.gap[0], 3, 1)};
      if (ui_btn_menu(ctx, &edt, pan, strv("Edit"), ICO_EDIT, 0)) {
        lst->con = APP_FILE_CON_MENU_EDIT;
      }
      struct gui_btn view = {.box = gui.box.div_y(&lay, ctx->cfg.gap[0], 3, 2)};
      if (ui_btn_menu(ctx, &view, pan, strv("View"), ICO_TABLE, 0)) {
        lst->con = APP_FILE_CON_MENU_VIEW;
      }
    } break;
    case APP_FILE_CON_MENU_VIEW: {
      struct gui_btn icos = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 0, 0)};
      if (ui_btn_menu(ctx, &icos, pan, strv("Icons"), ICO_TH_LIST, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn list = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 1, 0)};
      if (ui_btn_menu(ctx, &list, pan, strv("List"), ICO_LIST, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn col = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 0, 1)};
      if (ui_btn_menu(ctx, &col, pan, strv("Columns"), ICO_COLUMNS, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn gal = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 1, 1)};
      if (ui_btn_menu(ctx, &gal, pan, strv("Gallery"), ICO_TABLE, 0)) {
        ui_file_con_close(lst);
      }
    } break;
    case APP_FILE_CON_MENU_EDIT: {
      struct gui_btn cpy = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 0, 0)};
      if (ui_btn_menu(ctx, &cpy, pan, strv("Copy"), ICO_COPY, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn cut = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 1, 0)};
      if (ui_btn_menu(ctx, &cut, pan, strv("Cut"), ICO_CUT, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn put = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 0 ,1)};
      if (ui_btn_menu(ctx, &put, pan, strv("Paste"), ICO_PASTE, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn del = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 1, 1)};
      if (ui_btn_menu(ctx, &del, pan, strv("Delete"), ICO_TRASH, 0)) {
        ui_file_con_close(lst);
      }
    } break;}
  }
  gui.pan.end(ctx, pan, parent);

  struct gui_input in = {0};
  gui.pan.input(&in, ctx, pan, GUI_BTN_RIGHT);
  if (in.mouse.btn.right.clk) {
    if (lst->con == APP_FILE_CON_MENU_MAIN) {
      lst->state = APP_FILE_VIEW_LIST;
    } else {
      lst->con = APP_FILE_CON_MENU_MAIN;
    }
    gui.in.consume(ctx);
  }
}
static void
ui_file_sel_view(struct app *app, struct app_file_view *view,
                 struct app_file_list_view *lst,
                 struct gui_ctx *ctx, struct gui_panel *pan,
                 struct gui_panel *parent) {
  assert(app);
  assert(lst);
  assert(ctx);
  assert(pan);
  assert(view);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    switch (lst->state) {
    case APP_FILE_VIEW_LIST: {
      struct gui_panel fnd = {.box = pan->box};
      fnd.box.y = gui.bnd.min_ext(pan->box.y.min, ctx->cfg.item);
      ui_file_lst_view_fnd(app, lst, ctx, &fnd, pan);

      int gap = ctx->cfg.pan_gap[0];
      struct gui_panel tbl = {.box = pan->box};
      tbl.box.y = gui.bnd.min_max(fnd.box.y.max + gap, pan->box.y.max);
      ui_file_view_tbl(app, view, lst, ctx, &tbl, pan);
    } break;
    case APP_FILE_VIEW_MENU: {
      struct gui_panel menu = {.box = pan->box};
      ui_file_con_menu(app, lst, ctx, &menu, pan);
    } break;
    }
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_file_sel_split(struct app *app, struct app_file_view *fs,
                  struct gui_ctx *ctx, struct gui_panel *pan,
                  struct gui_panel *parent) {
  assert(app);
  assert(fs);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_split spt = {.box = pan->box};
    int lay[GUI_SPLIT_COL(APP_FILE_SPLIT_MAX)];
    gui.splt.begin(ctx, &spt, pan, GUI_SPLIT_FIT, GUI_HORIZONTAL, lay, fs->split);
    {
      struct gui_panel tree = {.box = spt.item};
      ui_file_view_tree(app, fs, &fs->tree, ctx, &tree, &spt.pan);
      gui.splt.sep(ctx, &spt, lay, fs->split);

      struct gui_panel tbl = {.box = spt.item};
      ui_file_sel_view(app, fs, &fs->lst, ctx, &tbl, &spt.pan);
    }
    gui.splt.end(ctx, &spt, pan);
  }
  gui.pan.end(ctx, pan, parent);
}
static int
ui_file_sel(dyn(char) *filepath, struct app *app, struct app_file_view *fs,
            struct gui_ctx *ctx, struct gui_panel *pan,
            struct gui_panel *parent) {
  assert(fs);
  assert(ctx);
  assert(pan);
  assert(parent);
  assert(filepath);

  int ret = 0;
  int pan_gap = ctx->cfg.pan_gap[1];
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_box lay = pan->box;
    struct gui_btn open = {.box = gui.cut.bot(&lay, ctx->cfg.item, pan_gap)};
    struct gui_panel nav = {.box = gui.cut.top(&lay, ctx->cfg.item, pan_gap)};
    struct gui_panel tbl = {.box = lay};
    ui_file_lst_view_nav_bar(app, fs, &fs->lst, ctx, &nav, pan);
    ui_file_sel_split(app, fs, ctx, &tbl, pan);

    /* file selection */
    int dis = fs->lst.sel_idx < 0 || fs->lst.sel_idx >= dyn_cnt(fs->lst.elms);
    if (!dis) {
      const struct app_file_elm *elm = 0;
      elm = fs->lst.elms + fs->lst.sel_idx;
      dis = elm->isdir || !elm->isvalid;
    }
    gui.disable(ctx, dis);
    if (ui_btn_menu(ctx, &open, pan, strv("Open"), ICO_FILE_IMPORT, 0)) {
      const struct app_file_elm *elm = 0;
      elm = fs->lst.elms + fs->lst.sel_idx;
      dyn_asn_str(*filepath, ctx->sys, elm->fullpath);
      ret = 1;
    }
    gui.enable(ctx, dis);
  }
  gui.pan.end(ctx, pan, parent);
  return ret;
}

/* ============================================================================
 *
 *                                  App
 *
 * ===========================================================================
 */
extern void dlEntry(struct sys *s);
extern void dlRegister(struct sys *sys);

static int
app_on_mod(unsigned mod, unsigned keymod) {
  if (mod == 0) {
    return 1;
  } else if (mod == (unsigned)-1) {
    return keymod == 0;
  } else {
    return keymod == mod;
  }
}
static void
app_op_quit(struct app *app, const union app_param *arg) {
  unused(arg);
  app->quit = 1;
}
static void
app_init(struct app *app, struct sys *sys) {
  assert(app);
  assert(sys);

  app->res.sys = sys;
  app->gui.sys = sys;
  app->gui.res = &app->res;

  res.init(&app->res);
  gui.init(&app->gui, sys->mem.arena, CFG_COLOR_SCHEME);

  app_file_view_setup(&app->file, sys, &app->gui, sys->mem.arena, sys->mem.tmp);
  app->file_path = arena_dyn(sys->mem.arena, sys, char, 256);
}
static void
app_shutdown(struct app *app, struct sys *sys) {
  assert(app);
  assert(sys);
  app_file_view_free(&app->file, sys);
}
static void
app_ui_main(struct app *app, struct gui_ctx *ctx, struct gui_panel *pan,
            struct gui_panel *parent) {
  assert(app);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_panel bdy = {.box = pan->box};
    if (ui_file_sel(&app->file_path, app, &app->file, ctx, &bdy, pan)) {

    }
  }
  gui.pan.end(ctx, pan, parent);
}
extern void
dlRegister(struct sys *sys) {
  assert(sys);
  sys->plugin.add(&res, 0, strv("res"));
  if (res.version != RES_VERSION) {

  }
  sys->plugin.add(&gui, &res, strv("gui"));
  if (gui.version != GUI_VERSION) {

  }
}
extern void
dlEntry(struct sys *sys) {
  struct app *app = sys->app;
  if (!sys->app) {
    sys->app = arena_obj(sys->mem.arena, sys, struct app);
    app = sys->app;
    app_init(app, sys);
  }
  if (sys->quit) {
    app_shutdown(app, sys);
    return;
  }

#ifdef SYS_LINUX
  gui.color_scheme(&app->gui, GUI_COL_SCHEME_DARK);
#else
  if (sys->col_mod) {
    gui.color_scheme(&app->gui, CFG_COLOR_SCHEME);
  }
#endif

  memset(app->ops, 0, sizeof(app->ops));
  for (int i = 0; i < cntof(app_ops); ++i) {
    /* handle app shortcuts */
    const struct app_op *op = app_ops + i;
    if ((bit_tst(sys->keys, op->key.code) && sys->keymod == op->key.mod) ||
        (bit_tst(sys->keys, op->alt.code) && sys->keymod == op->alt.mod))
      op->handler(app, &op->arg);
  }
  for (int i = 0; i < cntof(app_ui_key_tbl); ++i) {
    /* map system keys to ui shortcuts */
    struct gui_ctx *ui = &app->gui;
    const struct app_ui_shortcut *s = app_ui_key_tbl + i;
    int keymod = app_on_mod(s->key.mod, sys->keymod);
    if (bit_tst(sys->keys, s->key.code) && keymod) {
      bit_set(ui->keys, i);
    } else if (bit_tst(sys->keys, s->alt.code) && keymod) {
      bit_set(ui->keys, i);
    }
  }
  switch (app->state) {
  case APP_STATE_FILE:
    app_file_view_update(&app->file, sys);
    break;
  }
  /* gui */
  dbg_blk_begin(sys, "app:gui");
  while (gui.begin(&app->gui)) {
    dbg_blk_begin(sys, "app:gui:pass");
    struct gui_panel pan = {.box = app->gui.box};
    app_ui_main(app, &app->gui, &pan, &app->gui.root);
    gui.end(&app->gui);
    dbg_blk_end(sys);
  }
  dbg_blk_end(sys);
}

