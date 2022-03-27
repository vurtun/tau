#ifdef DEBUG_MODE
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

/* sys */
#include "sys/cpu.h"
#include "lib/fmt.h"
#include "lib/fmt.c"
#include "lib/std.h"
#include "sys/dbg.h"
#include "sys/ren.h"
#include "sys/sys.h"
#include "lib/std.c"

/* app */
#include "res.h"
#include "gui.h"
#include "pck.h"

static struct gui_api gui;
#endif

/* -----------------------------------------------------------------------------
 *                              File Picker
 * ---------------------------------------------------------------------------*/
enum file_type { FILE_DEFAULT, FILE_FOLDER };
enum file_groups {
  FILE_GRP_OTHER,
  FILE_GRP_DIR,
  FILE_GRP_TXT,
  FILE_GRP_PROG,
  FILE_GRP_AUDIO,
  FILE_GRP_VIDEO,
  FILE_GRP_IMAGE,
  FILE_GRP_FONT,
  FILE_GRP_3D,
  FILE_GRP_EXEC,
  FILE_GRP_ARCHIVE,
  FILE_GRP_CNT,
};
struct file_def {
  const char *suffix;
  const char *name;
  const char *mime;
  int grp;
};
struct file_group_def {
  const char *name;
  const char *description;
  const char *icon;
  const struct file_def *files;
  int cnt;
};
#define FS_FILE_PERM_LEN 10
enum file_elm_type {
  FILE_DEF,
  FILE_DIR,
  FILE_SOCK,
  FILE_LNK,
  FILE_FIFO,
};
struct file_elm {
  const struct file_def *type;
  struct str fullpath;
  struct str path;
  struct str name;
  struct str ext;

  size_t size;
  time_t mtime;

  const char *ico;
  char perms[FS_FILE_PERM_LEN];
  enum file_elm_type file_type;
  unsigned isvalid : 1;
  unsigned isdir : 1;
};
struct file_tbl_col_def {
  struct str title;
  struct gui_split_lay_slot ui;
  sort_f sort[2];
};
enum file_tbl_hdr_col {
  FILE_TBL_NAME,
  FILE_TBL_TYPE,
  FILE_TBL_SIZE,
  FILE_TBL_PERM,
  FILE_TBL_DATE,
  FILE_TBL_MAX,
};
struct file_tbl {
  int cnt;
  struct gui_tbl_sort sort;
  int state[GUI_TBL_CAP(FILE_TBL_MAX)];
};
enum file_view_state {
  FILE_VIEW_LIST,
  FILE_VIEW_MENU,
};
enum file_con_menu {
  FILE_CON_MENU_MAIN,
  FILE_CON_MENU_VIEW,
  FILE_CON_MENU_EDIT,
};
#define MAX_FILTER 64
struct file_list_view {
  unsigned rev;
  struct arena mem;

  enum file_view_state state;
  enum file_con_menu con;

  dyn(struct file_elm) elms;
  dyn(unsigned long) fltr;
  int sel_idx;

  double off[2];
  struct file_tbl tbl;

  /* navigation */
  dyn(char) full_path;
  dyn(char) nav_path;
  dyn(char) fnd_buf;

  struct gui_txt_ed nav_ed;
  struct gui_txt_ed fnd_ed;
};
struct file_tree_node {
  struct file_tree_node *parent;
  struct lst_elm hook;
  struct lst_elm sub;

  unsigned long long id;
  struct str fullpath;
  int depth;
};
struct file_tree_view {
  unsigned rev;
  struct arena mem;
  struct file_tree_node root;
  dyn(struct file_tree_node*) lst;
  struct lst_elm del_lst;
  unsigned long long *exp;

  int sel;
  double off[2];

  unsigned jmp:1;
  unsigned long jmp_to;
};
#define FILE_SPLIT_MAX 2
struct file_view {
  int state;
  struct arena *tmp_arena;
  struct str home;
  int split[GUI_SPLIT_CAP(FILE_SPLIT_MAX)];

  unsigned lst_rev;
  struct file_list_view lst;

  unsigned tree_rev;
  struct file_tree_view tree;
};

/* =============================================================================
 *
 *                                File Picker
 *
 * =============================================================================
 */
// clang-format off
static const struct file_def file_unkown_defs[] = {{"","Unknown","application/octet-stream",FILE_GRP_OTHER}};
static const struct file_def file_folder_defs[] = {{NULL,"Folder","application/octet-stream",FILE_GRP_DIR}};
static const struct file_def file_link_defs[] = {{NULL,"Link","application/octet-stream",FILE_GRP_DIR}};
static const struct file_def file_sock_defs[] = {{NULL,"Socket","application/octet-stream",FILE_GRP_DIR}};
static const struct file_def file_fifo_defs[] = {{NULL,"Fifo","application/octet-stream",FILE_GRP_DIR}};
static const struct file_def file_text_defs[] = {
  {.suffix = "txt",   .name = "Text",                         .mime = "text/plain",                   .grp = FILE_GRP_TXT},
  {.suffix = "html",  .name = "Hyper Text Markup Language",   .mime = "text/html",                    .grp = FILE_GRP_TXT},
  {.suffix = "css",   .name = "CSS",                          .mime = "text/css",                     .grp = FILE_GRP_TXT},
  {.suffix = "md",    .name = "Markdown",                     .mime = "text/plain",                   .grp = FILE_GRP_TXT},
  {.suffix = "json",  .name = "JavaScript Object Notation",   .mime = "application/json",             .grp = FILE_GRP_TXT},
  {.suffix = "xml",   .name = "Extensive Markup Language",    .mime = "application/xml",              .grp = FILE_GRP_TXT},
  {.suffix = "pdf",   .name = "Portable Document Format",     .mime = "application/pdf",              .grp = FILE_GRP_TXT},
  {.suffix = "csv",   .name = "Comma-separated Values",       .mime = "text/csv",                     .grp = FILE_GRP_TXT},
  {.suffix = "chm",   .name = "Compiled HTML Help",           .mime = "application/vnd.ms-htmlhelp",  .grp = FILE_GRP_TXT},
};
static const struct file_def file_prog_defs[] = {
  {.suffix = "c",     .name = "C Source",         .mime = "text/x-c",                 .grp = FILE_GRP_TXT},
  {.suffix = "cpp",   .name = "C++ Source",       .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "cxx",   .name = "C++ Source",       .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "m",     .name = "ObjC Source",      .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "h",     .name = "C Header",         .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "hpp",   .name = "C++ Header",       .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "hxx",   .name = "C++ Header",       .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "py",    .name = "Python Script",    .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "rb",    .name = "Ruby Script",      .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "java",  .name = "Java Source",      .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "lua",   .name = "Lua Script",       .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "js",    .name = "JavaScript",       .mime = "application/javascript",   .grp = FILE_GRP_TXT},
  {.suffix = "php",   .name = "PHP Script",       .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "r",     .name = "R Script",         .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "tcl",   .name = "TCL Script",       .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "pl",    .name = "Perl Script",      .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "ada",   .name = "Ada Source",       .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "asm",   .name = "Assembly Source",  .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "clj",   .name = "Clojure Source",   .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "cs",    .name = "C# Source",        .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "d",     .name = "D Source",         .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "el",    .name = "Emacs Lisp",       .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "go",    .name = "Golang Source",    .mime = "text/plain",               .grp = FILE_GRP_TXT},
  {.suffix = "hs",    .name = "Haskell Source",   .mime = "text/plain",               .grp = FILE_GRP_TXT},
};
static const struct file_def file_audio_defs[] = {
  {.suffix = "mp3",   .name = "MP3 Audio File",   .mime = "audio/mpeg",   .grp = FILE_GRP_AUDIO},
  {.suffix = "wav",   .name = "WAV Audio File",   .mime = "audio/x-wav",  .grp = FILE_GRP_AUDIO},
  {.suffix = "ogg",   .name = "OGG Audio File",   .mime = "audio/ogg",    .grp = FILE_GRP_AUDIO},
};
static const struct file_def file_video_defs[] = {
  {.suffix = "asf",   .name = "ASF Video Container",          .mime = "video/x-ms-asf",               .grp = FILE_GRP_VIDEO},
  {.suffix = "AVI",   .name = "Audio Video Interleave",       .mime = "video/x-msvideo",              .grp = FILE_GRP_VIDEO},
  {.suffix = "BIK",   .name = "Bink Video",                   .mime = "application/octet-stream",     .grp = FILE_GRP_VIDEO},
  {.suffix = "flv",   .name = "Flash Video",                  .mime = "video/x-flv",                  .grp = FILE_GRP_VIDEO},
  {.suffix = "mkv",   .name = "Matroska Video Container",     .mime = "application/octet-stream",     .grp = FILE_GRP_VIDEO},
  {.suffix = "mov",   .name = "QuickTime Video Container",    .mime = "video/quicktime",              .grp = FILE_GRP_VIDEO},
  {.suffix = "mpeg",  .name = "MPEG Video",                   .mime = "video/mpeg",                   .grp = FILE_GRP_VIDEO},
  {.suffix = "mpg",   .name = "MPEG Video",                   .mime = "video/mpeg",                   .grp = FILE_GRP_VIDEO},
  {.suffix = "mp4",   .name = "MPEG Video",                   .mime = "video/mp4",                    .grp = FILE_GRP_VIDEO},
  {.suffix = "mpe",   .name = "MPEG Video",                   .mime = "video/mpeg",                   .grp = FILE_GRP_VIDEO},
  {.suffix = "thp",   .name = "Nintendo Video",               .mime = "application/octet-stream",     .grp = FILE_GRP_VIDEO},
  {.suffix = "roq",   .name = "Quake 3 Video",                .mime = "application/octet-stream",     .grp = FILE_GRP_VIDEO},
  {.suffix = "swf",   .name = "Macromedia Flash",             .mime = "application/octet-stream",     .grp = FILE_GRP_VIDEO},
  {.suffix = "webm",  .name = "Web Video",                    .mime = "application/octet-stream",     .grp = FILE_GRP_VIDEO},
  {.suffix = "ogg",   .name = "Ogg Video",                    .mime = "video/ogg",                    .grp = FILE_GRP_VIDEO},
};
static const struct file_def file_images_defs[] = {
  {.suffix = "bmp",   .name = "Bitmap",                   .mime = "image/bmp",                  .grp = FILE_GRP_IMAGE},
  {.suffix = "pbm",   .name = "Portable Bitmap",          .mime = "image/x-portable-bitmap",    .grp = FILE_GRP_IMAGE},
  {.suffix = "png",   .name = "PNG",                      .mime = "image/png",                  .grp = FILE_GRP_IMAGE},
  {.suffix = "jpg",   .name = "JPEG",                     .mime = "image/jpeg",                 .grp = FILE_GRP_IMAGE},
  {.suffix = "jp2",   .name = "JPEG 2000",                .mime = "image/jpeg",                 .grp = FILE_GRP_IMAGE},
  {.suffix = "pcx",   .name = "Picture Exchange Format",  .mime = "image/x-pcx",                .grp = FILE_GRP_IMAGE},
  {.suffix = "tga",   .name = "Truevision TGA",           .mime = "application/octet-stream",   .grp = FILE_GRP_IMAGE},
  {.suffix = "gif",   .name = "Graphic Interchange",      .mime = "image/gif",                  .grp = FILE_GRP_IMAGE},
  {.suffix = "psd",   .name = "Adobe Photoshop",          .mime = "application/octet-stream",   .grp = FILE_GRP_IMAGE},
  {.suffix = "raw",   .name = "Raw Image Data",           .mime = "application/octet-stream",   .grp = FILE_GRP_IMAGE},
  {.suffix = "tif",   .name = "Tagged Image Format",      .mime = "image/tiff",                 .grp = FILE_GRP_IMAGE},
  {.suffix = "xbm",   .name = "X Pixmap",                 .mime = "image/x-xbitmap",            .grp = FILE_GRP_IMAGE},
  {.suffix = "xpm",   .name = "X Pixmap",                 .mime = "image/x-xpixmap",            .grp = FILE_GRP_IMAGE},
  {.suffix = "xcf",   .name = "GIMP Image",               .mime = "application/octet-stream",   .grp = FILE_GRP_IMAGE},
  {.suffix = "ico",   .name = "Windows Icon",             .mime = "image/x-icon",               .grp = FILE_GRP_IMAGE},
  {.suffix = "bti",   .name = "Nintendo Texture",         .mime = "application/octet-stream",   .grp = FILE_GRP_IMAGE},
  {.suffix = "blp",   .name = "Blizzard Texture",         .mime = "application/octet-stream",   .grp = FILE_GRP_IMAGE},
  {.suffix = "vtf",   .name = "Valve Texture",            .mime = "application/octet-stream",   .grp = FILE_GRP_IMAGE},
};
static const struct file_def file_3d_defs[] = {
  {.suffix = "3ds",   .name = "Legacy 3DS Max Model", .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "b3d",   .name = "Blitz3D Model",        .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "blend", .name = "Blender Model",        .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "bmd",   .name = "Nintendo 3d Model",    .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "bdl",   .name = "Nintendo 3d Model",    .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "brres", .name = "Nintendo 3d Model",    .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "c4d",   .name = "Cinema 4d Model",      .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "cob",   .name = "Caligary Object",      .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "dae",   .name = "Collada Model",        .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "fbx",   .name = "Autodesk FBX",         .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "max",   .name = "Autodesk 3DS Max",     .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "md2",   .name = "Quake 2 Model",        .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "md3",   .name = "Quake 3 Model",        .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "md5",   .name = "Doom 3 Model",         .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "mdx",   .name = "Blizzard Model ",      .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "obj",   .name = "Wavefront .obj",       .mime = "application/octet-stream", .grp = FILE_GRP_3D},
  {.suffix = "smd",   .name = "Valves Studiomdl",     .mime = "application/octet-stream", .grp = FILE_GRP_3D},
};
static const struct file_def file_font_defs[] = {
  {.suffix = "ttf",   .name = "TrueType Font",            .mime = "application/x-font-ttf",   .grp = FILE_GRP_FONT},
  {.suffix = "abf",   .name = "Adobe Binary Screen Font", .mime = "application/octet-stream", .grp = FILE_GRP_FONT},
  {.suffix = "afm",   .name = "Adobe Font Metrics",       .mime = "application/octet-stream", .grp = FILE_GRP_FONT},
  {.suffix = "otf",   .name = "OpenType Font",            .mime = "application/x-font-otf",   .grp = FILE_GRP_FONT},
};
static const struct file_def file_exe_defs[] = {
  {.suffix = "o",     .name = "Unix Object Format",           .mime = "application/octet-stream",                 .grp = FILE_GRP_EXEC},
  {.suffix = "so",    .name = "Shared Object Format",         .mime = "application/octet-stream",                 .grp = FILE_GRP_EXEC},
  {.suffix = "apk",   .name = "Android Application Package",  .mime = "application/vnd.android.package-archive",  .grp = FILE_GRP_EXEC},
  {.suffix = "dll",   .name = "Windows Dynamic Library",      .mime = "application/octet-stream",                 .grp = FILE_GRP_EXEC},
  {.suffix = "exe",   .name = "Windows Executable",           .mime = "application/x-msdownload",                 .grp = FILE_GRP_EXEC},
  {.suffix = "xbe",   .name = "Xbox Executable",              .mime = "application/x-msdownload",                 .grp = FILE_GRP_EXEC},
  {.suffix = "xex",   .name = "Xbox 360 Executable",          .mime = "application/x-msdownload",                 .grp = FILE_GRP_EXEC},
  {.suffix = "sh",    .name = "Shell Script",                 .mime = "application/x-sh",                         .grp = FILE_GRP_EXEC},
};
static const struct file_def file_archive_defs[] = {
  {.suffix = "zip",   .name = "Zip Archive",                  .mime = "application/zip",              .grp = FILE_GRP_ARCHIVE},
  {.suffix = "7z",    .name = "7Zip Archive",                 .mime = "application/x-7z-compressed",  .grp = FILE_GRP_ARCHIVE},
  {.suffix = "rar",   .name = "RAR Archive",                  .mime = "application/x-rar-compressed", .grp = FILE_GRP_ARCHIVE},
  {.suffix = "gz",    .name = "Compressed TAR Archive",       .mime = "application/x-gtar",           .grp = FILE_GRP_ARCHIVE},
  {.suffix = "bz2",   .name = "Compressed TAR Archive",       .mime = "application/x-bzip2",          .grp = FILE_GRP_ARCHIVE},
  {.suffix = "tar",   .name = "TAR Archive",                  .mime = "application/x-gtar",           .grp = FILE_GRP_ARCHIVE},
  {.suffix = "jar",   .name = "Jave Zipped Container",        .mime = "application/java-archive",     .grp = FILE_GRP_ARCHIVE},
  {.suffix = "pk3",   .name = "Quake 3 Archive",              .mime = "application/zip",              .grp = FILE_GRP_ARCHIVE},
  {.suffix = "pk4",   .name = "Doom 3 Archive",               .mime = "application/zip",              .grp = FILE_GRP_ARCHIVE},
  {.suffix = "mpq",   .name = "Blizzard Archive",             .mime = "application/octet-stream",     .grp = FILE_GRP_ARCHIVE},
  {.suffix = "bin",   .name = "Binary File",                  .mime = "application/octet-stream",     .grp = FILE_GRP_ARCHIVE},
  {.suffix = "iso",   .name = "Optical Media",                .mime = "application/octet-stream",     .grp = FILE_GRP_ARCHIVE},
};
static const struct file_group_def file_groups[] = {
  [FILE_GRP_OTHER]      = {.name = "Other",         .icon = ICO_FILE,         .files = file_unkown_defs,  .cnt = cntof(file_unkown_defs)},
  [FILE_GRP_DIR]        = {.name = "Folder",        .icon = ICO_FOLDER_OPEN,  .files = file_folder_defs,  .cnt = cntof(file_folder_defs)},
  [FILE_GRP_TXT]        = {.name = "Text",          .icon = ICO_FILE_ALT,     .files = file_text_defs,    .cnt = cntof(file_text_defs)},
  [FILE_GRP_PROG]       = {.name = "Programming",   .icon = ICO_FILE_CODE,    .files = file_prog_defs,    .cnt = cntof(file_prog_defs)},
  [FILE_GRP_AUDIO]      = {.name = "Audio",         .icon = ICO_FILE_AUDIO,   .files = file_audio_defs,   .cnt = cntof(file_audio_defs)},
  [FILE_GRP_VIDEO]      = {.name = "Video",         .icon = ICO_FILE_VIDEO,   .files = file_video_defs,   .cnt = cntof(file_video_defs)},
  [FILE_GRP_IMAGE]      = {.name = "Image",         .icon = ICO_FILE_IMAGE,   .files = file_images_defs,  .cnt = cntof(file_images_defs)},
  [FILE_GRP_FONT]       = {.name = "Font",          .icon = ICO_FONT,         .files = file_font_defs,    .cnt = cntof(file_font_defs)},
  [FILE_GRP_3D]         = {.name = "3DModel",       .icon = ICO_CUBES,        .files = file_3d_defs,      .cnt = cntof(file_3d_defs)},
  [FILE_GRP_EXEC]       = {.name = "Executable",    .icon = ICO_FILE,         .files = file_exe_defs,     .cnt = cntof(file_exe_defs)},
  [FILE_GRP_ARCHIVE]    = {.name = "Archive",       .icon = ICO_FILE_ARCHIVE, .files = file_archive_defs, .cnt = cntof(file_archive_defs)},
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
static const struct file_tbl_col_def file_tbl_def[FILE_TBL_MAX] = {
  [FILE_TBL_NAME] =  {.title = strv("Name"),           .sort = {file_cmp_asc,      file_cmp_desc},       .ui = {.type = GUI_LAY_SLOT_DYN, .size =   1, .con = {100, 400}}},
  [FILE_TBL_TYPE] =  {.title = strv("Type"),           .sort = {file_cmp_type_asc, file_cmp_type_desc},  .ui = {.type = GUI_LAY_SLOT_FIX, .size =  50, .con = { 50, 400}}},
  [FILE_TBL_SIZE] =  {.title = strv("Size"),           .sort = {file_cmp_size_asc, file_cmp_size_desc},  .ui = {.type = GUI_LAY_SLOT_FIX, .size =  60, .con = { 50, 400}}},
  [FILE_TBL_PERM] =  {.title = strv("Permission"),     .sort = {file_cmp_perm_asc, file_cmp_perm_desc},  .ui = {.type = GUI_LAY_SLOT_FIX, .size =  80, .con = { 50, 400}}},
  [FILE_TBL_DATE] =  {.title = strv("Date Modified"),  .sort = {file_cmp_time_asc, file_cmp_time_desc},  .ui = {.type = GUI_LAY_SLOT_FIX, .size = 150, .con = {100, 400}}},
};
static const struct gui_split_lay_slot file_split_def[FILE_SPLIT_MAX] = {
  {.type = GUI_LAY_SLOT_FIX, .size = 200, .con = {100, 800}},
  {.type = GUI_LAY_SLOT_DYN, .size =   1, .con = { 50, 800}},
};
// clang-format on

static const struct file_def *
file_type(struct str ext) {
  const struct file_group_def *grp = 0;
  for_arrv(grp, file_groups) {
    const struct file_def *def = 0;
    for_arr(def, grp->files, grp->cnt) {
      if (def->suffix && str_cmp(str0(def->suffix), ext) == 0) {
        return def;
      }
    }
  }
  return file_unkown_defs;
}
static const char*
file_icon(const struct file_def *type) {
  assert(type);
  assert(type->grp >= 0 && type->grp < cntof(file_groups));
  return file_groups[type->grp].icon;
}
static int
file_cmp_asc(const void *a, const void *b) {
  const struct file_elm *fa = (const struct file_elm *)a;
  const struct file_elm *fb = (const struct file_elm *)b;
  if (fa->isdir && !fb->isdir) {
    return -1;
  } else if (!fa->isdir && fb->isdir) {
    return 1;
  }
  return str_cmp(fa->name, fb->name);
}
static int
file_cmp_desc(const void *a, const void *b) {
  const struct file_elm *fa = (const struct file_elm *)a;
  const struct file_elm *fb = (const struct file_elm *)b;
  if (fa->isdir && !fb->isdir) {
    return 1;
  } else if (!fa->isdir && fb->isdir) {
    return -1;
  }
  return str_cmp(fb->name, fa->name);
}
static int
file_cmp_name_asc(const void *a, const void *b) {
  const struct file_elm *fa = (const struct file_elm *)a;
  const struct file_elm *fb = (const struct file_elm *)b;
  return str_cmp(fa->name, fb->name);
}
static int
file_cmp_name_desc(const void *a, const void *b) {
  const struct file_elm *fa = (const struct file_elm *)a;
  const struct file_elm *fb = (const struct file_elm *)b;
  return str_cmp(fb->name, fa->name);
}
static int
file_cmp_type_asc(const void *a, const void *b) {
  const struct file_elm *fa = (const struct file_elm *)a;
  const struct file_elm *fb = (const struct file_elm *)b;
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
  const struct file_elm *fa = (const struct file_elm *)a;
  const struct file_elm *fb = (const struct file_elm *)b;
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
  const struct file_elm *fa = (const struct file_elm *)a;
  const struct file_elm *fb = (const struct file_elm *)b;
  return cast(int, fa->size - fb->size);
}
static int
file_cmp_size_desc(const void *a, const void *b) {
  const struct file_elm *fa = (const struct file_elm *)a;
  const struct file_elm *fb = (const struct file_elm *)b;
  return cast(int, fb->size - fa->size);
}
static int
file_cmp_perm_asc(const void *a, const void *b) {
  const struct file_elm *fa = (const struct file_elm *)a;
  const struct file_elm *fb = (const struct file_elm *)b;
  return strcmp(fa->perms, fb->perms);
}
static int
file_cmp_perm_desc(const void *a, const void *b) {
  const struct file_elm *fa = (const struct file_elm *)a;
  const struct file_elm *fb = (const struct file_elm *)b;
  return strcmp(fb->perms, fa->perms);
}
static int
file_cmp_time_asc(const void *a, const void *b) {
  const struct file_elm *fa = (const struct file_elm *)a;
  const struct file_elm *fb = (const struct file_elm *)b;
  return cast(int, fa->mtime - fb->mtime);
}
static int
file_cmp_time_desc(const void *a, const void *b) {
  const struct file_elm *fa = (const struct file_elm *)a;
  const struct file_elm *fb = (const struct file_elm *)b;
  return cast(int, fb->mtime - fa->mtime);
}
static struct lst_elm*
file_tree_node_sort_after_elm(struct file_tree_node *n,
                                  struct file_tree_node *s) {
  assert(s);
  assert(n);

  struct lst_elm *elm = 0;
  for_lst(elm, &n->sub) {
    struct file_tree_node *it = 0;
    it = lst_get(elm, struct file_tree_node, hook);
    if (str_cmp(s->fullpath, it->fullpath) < 0) {
      break;
    }
  }
  return elm;
}
static void
file_tree_node_lnk(struct file_tree_node *n,
                       struct file_tree_node *s) {
  assert(s);
  assert(n);

  struct lst_elm *elm = file_tree_node_sort_after_elm(n, s);
  lst_del(&s->hook);
  lst_init(&s->hook);
  lst__add(&s->hook, elm->prv, elm);
}
static void
file_tree_node_init(struct sys *sys, struct arena *mem, struct file_tree_node *s,
                    struct file_tree_node *n, struct str p, unsigned long long id) {
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
  file_tree_node_lnk(n, s);
}
static struct file_tree_node*
file_node_alloc(struct file_tree_view *tree, struct sys *sys, struct arena *mem) {
  assert(tree);
  assert(sys);
  assert(mem);

  struct file_tree_node *s = 0;
  if (lst_any(&tree->del_lst)) {
    s = lst_get(tree->del_lst.nxt, struct file_tree_node, hook);
    lst_del(&s->hook);
    memset(s, 0, sizeof(*s));
  } else {
    s = arena_alloc(mem, sys, szof(*s));
  }
  return s;
}
static struct file_tree_node*
file_view_tree_node_new(struct file_tree_view *tree, struct sys *sys,
                        struct arena *mem, struct file_tree_node *n,
                        struct str path, unsigned long long id) {
  assert(tree);
  assert(sys);
  assert(mem);
  assert(n);

  struct file_tree_node *s = 0;
  s = file_node_alloc(tree, sys, mem);
  file_tree_node_init(sys, mem, s, n, path, id);
  return s;
}
static void
file_view_tree_node_del(struct file_tree_view *tree, struct file_tree_node *n) {
  assert(n);
  assert(tree);

  struct lst_elm *elm = 0;
  struct lst_elm *item = 0;
  for_lst_safe(elm, item, &n->sub) {
    struct file_tree_node *s = 0;
    s = lst_get(elm, struct file_tree_node, hook);
    file_view_tree_node_del(tree, s);
  }
  lst_del(&n->hook);
  lst_init(&n->hook);
  lst_add(&tree->del_lst, &n->hook);
}
static int
file_view_tree_build(struct file_tree_view *tree,
                     struct file_tree_node *n, struct sys *sys,
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
  struct mem_scp scp = {0};
  scp_mem(tmp, &scp, sys) {
    unsigned long long *set = arena_set(tmp, sys, 1024);
    struct lst_elm *elm = 0;
    for_lst(elm, &n->sub) {
      struct file_tree_node *s;
      s = lst_get(elm, struct file_tree_node, hook);
      set_put(set, sys, s->id);
    }
    /* validate child nodes */
    struct sys_dir_iter it = {0};
    for_dir_lst(sys, &it, tmp, n->fullpath) {
      if (it.name.str[0] == '.' || !sys->dir.exists(it.fullpath, tmp)) {
        continue;
      }
      unsigned long long id = str_hash(it.fullpath);
      if (!set_fnd(set, id)) {
        file_view_tree_node_new(tree, sys, mem, n, it.fullpath, id);
        upt = 1;
      }
      set_del(set, id);
    }
    /* remove deleted directory nodes */
    struct lst_elm *safe = 0;
    for_lst_safe(elm, safe, &n->sub) {
      struct file_tree_node *s;
      s = lst_get(elm, struct file_tree_node, hook);
      if (!set_fnd(set, s->id)) {
        continue;
      }
      file_view_tree_node_del(tree, s);
    }
    set_free(set, sys);
  }

  /* recurse child directory nodes */
  struct lst_elm *it = 0;
  for_lst(it, &n->sub) {
    struct file_tree_node *s;
    s = lst_get(it, struct file_tree_node, hook);
    upt |= file_view_tree_build(tree, s, sys, mem, tmp);
  }
  return upt;
}
static struct file_tree_node**
file_view_tree_serial(struct file_tree_view *tree,
                      struct file_tree_node *n,
                      struct file_tree_node **lst,
                      struct sys *sys) {
  assert(tree);
  assert(lst);
  assert(sys);
  assert(n);

  dyn_add(lst, sys, n);
  if (set_fnd(tree->exp, n->id)) {
    struct lst_elm *it = 0;
    for_lst(it, &n->sub) {
      struct file_tree_node *s = 0;
      s = lst_get(it, struct file_tree_node, hook);
      lst = file_view_tree_serial(tree, s, lst, sys);
    }
  }
  return lst;
}
static void
file_tree_update(struct file_view *fs, struct sys *sys) {
  assert(fs);
  assert(sys);

  struct arena *tmp = fs->tmp_arena;
  file_view_tree_build(&fs->tree, &fs->tree.root, sys, &fs->tree.mem, tmp);
  dyn_clr(fs->tree.lst);
  fs->tree.lst = file_view_tree_serial(&fs->tree, &fs->tree.root, fs->tree.lst, sys);
}
static struct file_tree_node*
file_tree_node_fnd(struct file_tree_node *n, struct str str) {
  assert(n);
  struct lst_elm *elm = 0;
  struct file_tree_node *s = 0;
  for_lst(elm, &n->sub) {
    s = lst_get(elm, struct file_tree_node, hook);
    struct str file_name = path_file(s->fullpath);
    if (str_eq(str, file_name)) {
      return s;
    }
  }
  return s;
}
static void
file_view_tree_open(struct file_view *fs, struct file_tree_view *tree,
                    struct sys *sys, struct str p) {
  assert(tree);
  assert(sys);
  assert(fs);

  int off = fs->home.len + 1;
  if (p.len < off) {
    return;
  }
  struct file_tree_node *n = &tree->root;
  struct str path = str_rhs(p, off);
  for_str_tok(it, _, path, strv("/")) {
    struct str fullpath = strp(p.str, it.end);
    if (!sys->dir.exists(fullpath, fs->tmp_arena)) {
      break;
    }
    /* create or find node to dig deeper */
    struct file_tree_node *s = file_tree_node_fnd(n, it);
    if (!s) {
      unsigned long long id = str_hash(strp(p.str, it.end));
      s = file_view_tree_node_new(tree, sys, &tree->mem, n, fullpath, id);
    }
    tree->jmp = 1;
    tree->jmp_to = s->id;
    set_put(tree->exp, sys, s->id);
    fs->tree_rev++;
    n = s;
  }
  file_tree_update(fs, sys);
}
static void
file_lst_view_clr(struct file_list_view *lst, struct sys *sys) {
  assert(lst);
  assert(sys);

  dyn_free(lst->fltr, sys);
  dyn_free(lst->elms, sys);
  dyn_free(lst->full_path, sys);
  dyn_free(lst->fnd_buf, sys);
  arena_free(&lst->mem, sys);

  zero2(lst->off);
  lst->fltr = arena_dyn(&lst->mem, sys, unsigned long, 128);
  lst->elms = arena_dyn(&lst->mem, sys, struct file_elm, 256);
  lst->nav_path = arena_dyn(&lst->mem, sys, char, 256);
  lst->full_path = arena_dyn(&lst->mem, sys, char, 256);
  lst->fnd_buf = arena_dyn(&lst->mem, sys, char, MAX_FILTER);

  gui.edt.buf.reset(&lst->fnd_ed);
  gui.edt.buf.reset(&lst->nav_ed);
}
static void
file_tree_view_clr(struct file_tree_view *tree, struct sys *sys) {
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
file_lst_view_add_path(struct file_list_view *lst, struct sys *sys,
                       struct str path) {
  assert(lst);
  assert(sys);

  struct file_elm elm = {0};
  struct mem_scp scp = {0};
  mem_scp_begin(&scp, &lst->mem);

  /* parse file/directory name */
  elm.fullpath = arena_str(&lst->mem, sys, path);
  elm.name = path_file(elm.fullpath);
  elm.path = strp(elm.fullpath.str, elm.name.str);
  elm.ext = str_nil;

  /* extract file stat, extension and type */
  struct sys_file_info info;
  if (!sys->file.info(&info, elm.fullpath, &lst->mem)) {
    mem_scp_end(&scp, &lst->mem, sys);
    return;
  }
  elm.size = info.siz;
  elm.mtime = info.mtime;
  elm.isvalid = 0;
  memcpy(elm.perms, info.perm, sizeof(elm.perms));

  switch (info.type) {
  case SYS_FILE_DEF: {
    struct str ext = path_ext(elm.name);
    elm.ext = arena_str(&lst->mem, sys, ext);
    elm.type = file_type(elm.ext);
  } break;
  case SYS_FILE_DIR: {
    elm.isdir = 1;
    elm.type = file_folder_defs;
    elm.file_type = FILE_DIR;
  } break;
  case SYS_FILE_SOCK: {
    elm.type = file_sock_defs;
    elm.file_type = FILE_SOCK;
  } break;
  case SYS_FILE_LNK: {
    elm.type = file_link_defs;
    elm.file_type = FILE_LNK;
  } break;
  case SYS_FILE_FIFO: {
    elm.type = file_fifo_defs;
    elm.file_type = FILE_FIFO;
  } break;}
  elm.ico = file_icon(elm.type);

  /* add new file info */
  dyn_add(lst->elms, sys, elm);
  dyn_fit(lst->fltr, sys, bits_to_long(dyn_cnt(lst->elms)));
  bit_clr(lst->fltr, dyn_cnt(lst->elms) - 1);
}
static void
file_list_view_fltr(struct file_list_view *lst, struct str fltr) {
  assert(lst);
  struct str_fnd_tbl tbl;
  str_fnd_tbl(&tbl, fltr);
  fori_dyn(i, lst->elms) {
    assert(i < dyn_cnt(lst->elms));
    int has = str_fnd_tbl_has(lst->elms[i].name, fltr, &tbl);
    bit_set_on(lst->fltr, i, has);
  }
}
static void
file_view_cd(struct file_view *fs, struct sys *sys, struct str path) {
  assert(fs);
  assert(sys);

  struct mem_scp scp = {0};
  struct arena *tmp = fs->tmp_arena;
  scp_mem(tmp, &scp, sys) {
    struct str sys_path = arena_str(tmp, sys, path);
    file_lst_view_clr(&fs->lst, sys);
    dyn_asn_str(fs->lst.full_path, sys, sys_path);
    dyn_asn_str(fs->lst.nav_path, sys, sys_path);

    /* add all files in directory */
    struct sys_dir_iter it = {0};
    for_dir_lst(sys, &it, tmp, sys_path) {
      file_lst_view_add_path(&fs->lst, sys, it.fullpath);
    }
    /* sort list by name */
    dyn_sort(fs->lst.elms, file_cmp_asc);
    fs->lst.sel_idx = -1;
  }
  fs->lst.state = FILE_VIEW_LIST;
  fs->lst.con = FILE_CON_MENU_MAIN;
}
static void
file_list_view_init(struct file_view *fs, struct sys *sys, struct gui_ctx *ctx) {
  assert(fs);
  assert(sys);
  assert(ctx);

  fs->lst.elms = arena_dyn(&fs->lst.mem, sys, struct file_elm, 1024);
  fs->lst.fltr = arena_dyn(&fs->lst.mem, sys, unsigned long, 128);
  fs->lst.nav_path = arena_dyn(&fs->lst.mem, sys, char, 1024);
  fs->lst.full_path = arena_dyn(&fs->lst.mem, sys, char, 1024);
  fs->lst.fnd_buf = arena_dyn(&fs->lst.mem, sys, char, MAX_FILTER);

  /* setup list table */
  struct gui_split_lay_cfg tbl_cfg = {0};
  tbl_cfg.size = sizeof(struct file_tbl_col_def);
  tbl_cfg.off = offsetof(struct file_tbl_col_def, ui);
  tbl_cfg.slots = file_tbl_def;
  tbl_cfg.cnt = FILE_TBL_MAX;
  gui.tbl.lay(fs->lst.tbl.state, ctx, &tbl_cfg);

  /* open home directory */
  file_view_cd(fs, sys, fs->home);
}
static void
file_tree_view_init(struct file_view *fs, struct sys *sys) {
  assert(fs);
  assert(sys);

  fs->tree.lst = arena_dyn(&fs->tree.mem, sys, struct file_tree_node*, 1024);
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
static struct file_view*
file_view_init(struct sys *sys, struct gui_ctx *ctx, struct arena *mem,
                struct arena *tmp_arena) {
  assert(sys);
  assert(ctx);
  assert(mem);
  assert(tmp_arena);

  struct file_view *fs = arena_obj(mem, sys, struct file_view);
  fs->lst_rev = 1;
  fs->tree_rev = 1;
  fs->tmp_arena = tmp_arena;
  fs->home = arena_str(mem, sys, str0(getenv("HOME")));

  struct gui_split_lay_cfg cfg = {0};
  cfg.slots = file_split_def;
  cfg.cnt = FILE_SPLIT_MAX;
  gui.splt.lay.bld(fs->split, ctx, &cfg);

  file_list_view_init(fs, sys, ctx);
  file_tree_view_init(fs, sys);
  return fs;
}
static void
file_view_update(struct file_view *fs, struct sys *sys) {
  assert(sys);
  if (fs->tree_rev != fs->tree.rev) {
    file_tree_update(fs, sys);
    fs->tree.rev = fs->tree_rev;
  }
}
static void
file_view_free(struct file_view *fs, struct sys *sys) {
  assert(sys);
  file_lst_view_clr(&fs->lst, sys);
  file_tree_view_clr(&fs->tree, sys);
}

/* -----------------------------------------------------------------------------
 *                                  GUI
 * -----------------------------------------------------------------------------
 */
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
    struct gui_panel ico = {0};
    ico.box.y = gui.bnd.shrink(&pan->box.y, pad[1]);
    ico.box.x = gui.bnd.min_ext(pan->box.x.min, ctx->cfg.item);
    gui.ico.img(ctx, &ico, pan, ICO_SEARCH);

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
ui_file_lst_view_fnd(struct file_list_view *lst,
                     struct gui_ctx *ctx, struct gui_panel *pan,
                     struct gui_panel *parent) {
  assert(lst);
  assert(ctx);
  assert(pan);
  assert(parent);

  struct gui_edit_box edt = {.box = pan->box};
  ui_edit_search(ctx, &edt, pan, parent, &lst->fnd_ed, &lst->fnd_buf);
  if (edt.mod) {
    /* filter files by name */
    bit_fill(lst->fltr, 0x00, dyn_cnt(lst->elms));
    if (dyn_any(lst->fnd_buf)) {
      file_list_view_fltr(lst, dyn_str(lst->fnd_buf));
    }
  }
}
static void
ui_file_lst_view_nav_bar(struct file_view *fs, struct file_list_view *lst,
                         struct gui_ctx *ctx, struct gui_panel *pan,
                         struct gui_panel *parent) {
  assert(lst);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    int gap = ctx->cfg.gap[0];
    struct gui_box lay = pan->box;
    struct gui_btn home = {.box = gui.cut.rhs(&lay, ctx->cfg.item, gap)};
    if (gui.btn.ico(ctx, &home, pan, ICO_HOME)) {
      /* change the directory to home  */
      file_view_tree_open(fs, &fs->tree, ctx->sys, fs->home);
      file_view_cd(fs, ctx->sys, fs->home);
    }
    gui.tooltip(ctx, &home.pan, strv("Goto Home Directory"));

    struct gui_btn up = {.box = gui.cut.rhs(&lay, ctx->cfg.item, gap)};
    if (gui.btn.ico(ctx, &up, pan, ICO_ARROW_CIRCLE_UP)) {
      /* go up to parent directory  */
      struct str file_name = path_file(dyn_str(lst->full_path));
      struct str file_path = strp(lst->full_path, file_name.str);
      if (file_path.len > fs->home.len) {
        file_view_cd(fs, ctx->sys, strp(lst->full_path, file_name.str));
      }
    }
    gui.tooltip(ctx, &up.pan, strv("Move to Parent Directory"));

    struct gui_edit_box edt = {.box = lay};
    gui.edt.txt(ctx, &edt, pan, &lst->nav_ed, &lst->nav_path);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_file_view_tbl_elm(struct gui_ctx *ctx, struct gui_tbl *tbl,
                     const int *lay, struct gui_panel *elm,
                     const struct file_elm *fi, int is_sel) {
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
    scp_gui_cfg_pushu_on(&gui,stk,&ctx->cfg.col[GUI_COL_ICO], dir_col, fi->isdir) {
      /* columns */
      struct tm *mod_time = localtime(&fi->mtime);
      gui.tbl.lst.elm.col.txt(ctx, tbl, lay, elm, fi->name, fi->ico, 0);
      gui.tbl.lst.elm.col.txt(ctx, tbl, lay, elm, str0(fi->type->name), 0, 0);
      gui.tbl.lst.elm.col.txtf(ctx, tbl, lay, elm, &algn, "%zu", fi->size);
      gui.tbl.lst.elm.col.txt(ctx, tbl, lay, elm, str0(fi->perms), 0, 0);
      gui.tbl.lst.elm.col.tm(ctx, tbl, lay, elm, "%d/%m/%Y %H:%M:%S", mod_time);
    }
  }
  gui.tbl.lst.elm.end(ctx, tbl, elm);
  gui.tooltip(ctx, elm, fi->name);
}
static void
ui_file_view_tbl(struct file_view *fs, struct file_list_view *lst,
                 struct gui_ctx *ctx, struct gui_panel *pan,
                 struct gui_panel *parent) {
  assert(lst);
  assert(ctx);
  assert(pan);
  assert(parent);

  int chdir = 0, dir = 0;
  dbg_blk_begin(ctx->sys, "app:gui:file:table");
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, lst->off, &lst->tbl.sort);
    {
      /* header */
      int tbl_lay[GUI_TBL_COL(FILE_TBL_MAX)];
      gui.tbl.hdr.begin(ctx, &tbl, tbl_lay, lst->tbl.state);
      for_cnt(i, tbl.cnt) {
        assert(i < cntof(file_tbl_def));
        struct str title = file_tbl_def[i].title;
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_lay, lst->tbl.state, title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* sorting */
      if (tbl.resort && lst->elms) {
        assert(tbl.sort.col < cntof(file_tbl_def));
        assert(tbl.sort.order < cntof(file_tbl_def[tbl.sort.col].sort));
        dyn_sort(lst->elms, file_tbl_def[tbl.sort.col].sort[tbl.sort.order]);
        lst->tbl.sort = tbl.sort;
      }
      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, dyn_cnt(lst->elms));
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;
      cfg.fltr.on = GUI_LST_FLTR_ON_ONE;
      cfg.fltr.bitset = lst->fltr;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for_gui_tbl_lst(i,gui,&tbl) {
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
          lst->state = FILE_VIEW_MENU;
          gui.in.eat(ctx);
        }
      }
      gui.tbl.lst.end(ctx, &tbl);
      if (tbl.lst.sel.mod) {
        /* selection handling */
        struct file_elm *elm = lst->elms + tbl.lst.sel.idx;
        elm->isvalid = !elm->isdir;
        lst->sel_idx = tbl.lst.sel.idx;
      }
    }
    gui.tbl.end(ctx, &tbl, pan, lst->off);
  }
  gui.pan.end(ctx, pan, parent);
  dbg_blk_end(ctx->sys);

  if (chdir) {
    struct file_elm *fi = lst->elms + dir;
    if (fi->isdir) {
      file_view_tree_open(fs, &fs->tree, ctx->sys, fi->fullpath);
      file_view_cd(fs, ctx->sys, fi->fullpath);
      ctx->lst_state.cur_idx = -1;
    }
  }
}
static void
ui_file_view_tree_node(struct gui_ctx *ctx, struct gui_tree_node *node,
                       struct gui_panel *parent, struct file_tree_node *n){
  assert(n);
  assert(ctx);
  assert(node);
  assert(parent);

  gui.tree.begin(ctx, node, parent, n->depth);
  {
    struct gui_cfg_stk stk[1] = {0};
    scp_gui_cfg_pushu(&gui, stk, &ctx->cfg.col[GUI_COL_ICO], col_rgb_hex(0xeecd4a)) {
      struct gui_panel lbl = {.box = node->box};
      struct str txt = path_file(n->fullpath);
      const char *ico = node->open ? ICO_FOLDER_OPEN : ICO_FOLDER;
      gui.ico.box(ctx, &lbl, &node->pan, ico, txt);
      gui.tooltip(ctx, &node->pan, n->fullpath);
    }
  }
  gui.tree.end(ctx, node, parent);
}
static int
ui_file_view_tree_elm(struct gui_ctx *ctx, struct file_tree_view *tree,
                      struct file_tree_node *n, struct gui_lst_reg *reg,
                      struct gui_panel *elm, int is_sel) {
  assert(n);
  assert(ctx);
  assert(reg);
  assert(elm);
  assert(tree);

  int ret = 0;
  gui.lst.reg.elm.begin(ctx, reg, elm, n->id, is_sel);
  {
    struct gui_tree_node node = {0};
    node.type = lst_any(&n->sub) ? GUI_TREE_NODE : GUI_TREE_LEAF;
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
ui_file_view_tree_key(struct file_view *fs, struct file_tree_view *tree,
                      struct gui_ctx *ctx, struct gui_lst *lst) {
  assert(tree);
  assert(ctx);
  assert(lst);
  assert(fs);

  /* tree node expansion */
  int plus = bit_tst_clr(ctx->keys, GUI_KEY_TREE_EXPAND);
  int opening = bit_tst_clr(ctx->keys, GUI_KEY_TREE_RIGHT);
  if (plus || opening) {
    struct file_tree_node *n = tree->lst[tree->sel];
    set_put(tree->exp, ctx->sys, n->id);
    fs->tree_rev++;
    return;
  }
  /* tree node collapse and jump to parent node */
  int minus = bit_tst_clr(ctx->keys, GUI_KEY_TREE_COLLAPSE);
  int closing = bit_tst_clr(ctx->keys, GUI_KEY_TREE_LEFT);
  if (minus || closing) {
    struct file_tree_node *n = tree->lst[tree->sel];
    if (closing && !set_fnd(tree->exp, n->id)) {
      for (int i = tree->sel; i > 0; --i) {
        int nidx = i - 1;
        struct file_tree_node *p = tree->lst[nidx];
        if (p->depth >= n->depth) {
          continue;
        }
        tree->off[1] = gui.lst.lay.clamps(&lst->lay, nidx);
        gui.lst.set_sel_idx(ctx, lst, nidx);
        gui.lst.set_cur_idx(ctx, lst, nidx);
        tree->sel = nidx;
        break;
      }
    } else {
      set_del(tree->exp, n->id);
    }
    fs->tree_rev++;
  }
}
static int
ui_file_view_tree_jmp_elm(struct file_tree_view *tree,
                          unsigned long long jmp_id) {
  fori_dyn(i, tree->lst) {
    assert(i < dyn_cnt(tree->lst));
    struct file_tree_node *n = tree->lst[i];
    if (n->id == jmp_id) {
      return i;
    }
  }
  return dyn_cnt(tree->lst);
}
static void
ui_file_view_tree(struct file_view *fs, struct file_tree_view *tree,
                  struct gui_ctx *ctx, struct gui_panel *pan,
                  struct gui_panel *parent) {
  assert(tree);
  assert(ctx);
  assert(pan);
  assert(parent);

  dbg_blk_begin(ctx->sys, "app:gui:file:tree");
  gui.pan.begin(ctx, pan, parent);
  {
    /* tree list */
    struct gui_lst_cfg cfg = {0};
    gui.lst.cfg(&cfg, dyn_cnt(tree->lst), tree->off[1]);
    cfg.sel.src = GUI_LST_SEL_SRC_EXT;

    struct gui_lst_reg reg = {.box = pan->box};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, tree->off);
    for_gui_reg_lst(i,gui,&reg) {
      /* tree nodes */
      struct gui_panel elm = {0};
      struct file_tree_node *n = tree->lst[i];
      if (ui_file_view_tree_elm(ctx, tree, n, &reg, &elm, tree->sel == i)) {
        fs->tree_rev++;
      }
    }
    if (tree->jmp) {
      /* jump to list element */
      int idx = ui_file_view_tree_jmp_elm(tree, tree->jmp_to);
      if (idx < dyn_cnt(tree->lst)) {
        gui.lst.reg.ctr(&reg, idx);
        tree->sel = idx;
      }
      tree->jmp = 0;
    }
    gui.lst.reg.end(ctx, &reg, pan, tree->off);
    if (reg.lst.sel.mod) {
      /* selection */
      tree->sel = reg.lst.sel.idx;
      struct file_tree_node *n = tree->lst[tree->sel];
      file_view_cd(fs, ctx->sys, n->fullpath);
    }
    if (reg.lst.ctl.has_focus) {
      /* key handling */
      ui_file_view_tree_key(fs, tree, ctx, &reg.lst);
    }
  }
  gui.pan.end(ctx, pan, parent);
  dbg_blk_end(ctx->sys);
}
static void
ui_file_con_close(struct file_list_view *lst) {
  assert(lst);
  lst->state = FILE_VIEW_LIST;
  lst->con = FILE_CON_MENU_MAIN;
}
static void
ui_file_con_menu(struct file_list_view *lst, struct gui_ctx *ctx,
                 struct gui_panel *pan, struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(parent);

  dbg_blk_begin(ctx->sys, "app:gui:file:contextual");
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_box lay = pan->box;
    switch (lst->con) {
    case FILE_CON_MENU_MAIN: {
      struct gui_btn open = {.box = gui.box.div_y(&lay, ctx->cfg.gap[0], 3, 0)};
      if (gui.btn.ico_txt(ctx, &open, pan, strv("Open"), ICO_FOLDER_OPEN, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn edt = {.box = gui.box.div_y(&lay, ctx->cfg.gap[0], 3, 1)};
      if (gui.btn.ico_txt(ctx, &edt, pan, strv("Edit"), ICO_EDIT, 0)) {
        lst->con = FILE_CON_MENU_EDIT;
      }
      struct gui_btn view = {.box = gui.box.div_y(&lay, ctx->cfg.gap[0], 3, 2)};
      if (gui.btn.ico_txt(ctx, &view, pan, strv("View"), ICO_TABLE, 0)) {
        lst->con = FILE_CON_MENU_VIEW;
      }
    } break;
    case FILE_CON_MENU_VIEW: {
      struct gui_btn icos = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 0, 0)};
      if (gui.btn.ico_txt(ctx, &icos, pan, strv("Icons"), ICO_TH_LIST, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn list = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 1, 0)};
      if (gui.btn.ico_txt(ctx, &list, pan, strv("List"), ICO_LIST, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn col = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 0, 1)};
      if (gui.btn.ico_txt(ctx, &col, pan, strv("Columns"), ICO_COLUMNS, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn gal = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 1, 1)};
      if (gui.btn.ico_txt(ctx, &gal, pan, strv("Gallery"), ICO_TABLE, 0)) {
        ui_file_con_close(lst);
      }
    } break;
    case FILE_CON_MENU_EDIT: {
      struct gui_btn cpy = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 0, 0)};
      if (gui.btn.ico_txt(ctx, &cpy, pan, strv("Copy"), ICO_COPY, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn cut = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 1, 0)};
      if (gui.btn.ico_txt(ctx, &cut, pan, strv("Cut"), ICO_CUT, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn put = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 0, 1)};
      if (gui.btn.ico_txt(ctx, &put, pan, strv("Paste"), ICO_PASTE, 0)) {
        ui_file_con_close(lst);
      }
      struct gui_btn del = {.box = gui.box.div(&lay, ctx->cfg.gap, 2, 2, 1, 1)};
      if (gui.btn.ico_txt(ctx, &del, pan, strv("Delete"), ICO_TRASH, 0)) {
        ui_file_con_close(lst);
      }
    } break;}
  }
  gui.pan.end(ctx, pan, parent);

  struct gui_input in = {0};
  gui.pan.input(&in, ctx, pan, GUI_BTN_RIGHT);
  if (in.mouse.btn.right.clk) {
    if (lst->con == FILE_CON_MENU_MAIN) {
      lst->state = FILE_VIEW_LIST;
    } else {
      lst->con = FILE_CON_MENU_MAIN;
    }
    gui.in.eat(ctx);
  }
  dbg_blk_end(ctx->sys);
}
static void
ui_file_sel_view(struct file_view *fs, struct file_list_view *lst,
                 struct gui_ctx *ctx, struct gui_panel *pan,
                 struct gui_panel *parent) {
  assert(fs);
  assert(lst);
  assert(ctx);
  assert(pan);
  assert(parent);

  dbg_blk_begin(ctx->sys, "app:gui:file:content");
  gui.pan.begin(ctx, pan, parent);
  {
    switch (lst->state) {
    case FILE_VIEW_LIST: {
      struct gui_panel fnd = {.box = pan->box};
      fnd.box.y = gui.bnd.min_ext(pan->box.y.min, ctx->cfg.item);
      ui_file_lst_view_fnd(lst, ctx, &fnd, pan);

      int gap = ctx->cfg.pan_gap[0];
      struct gui_panel tbl = {.box = pan->box};
      tbl.box.y = gui.bnd.min_max(fnd.box.y.max + gap, pan->box.y.max);
      ui_file_view_tbl(fs, lst, ctx, &tbl, pan);
    } break;
    case FILE_VIEW_MENU: {
      struct gui_panel menu = {.box = pan->box};
      ui_file_con_menu(lst, ctx, &menu, pan);
    } break;
    }
  }
  gui.pan.end(ctx, pan, parent);
  dbg_blk_end(ctx->sys);
}
static void
ui_file_sel_split(struct file_view *fs, struct gui_ctx *ctx,
                  struct gui_panel *pan, struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(parent);

  dbg_blk_begin(ctx->sys, "app:gui:file:split");
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_split spt = {.box = pan->box};
    int lay[GUI_SPLIT_COL(FILE_SPLIT_MAX)];
    gui.splt.begin(ctx, &spt, pan, GUI_SPLIT_FIT, GUI_HORIZONTAL, lay, fs->split);
    {
      struct gui_panel tree = {.box = spt.item};
      ui_file_view_tree(fs, &fs->tree, ctx, &tree, &spt.pan);
      gui.splt.sep(ctx, &spt, lay, fs->split);

      struct gui_panel tbl = {.box = spt.item};
      ui_file_sel_view(fs, &fs->lst, ctx, &tbl, &spt.pan);
    }
    gui.splt.end(ctx, &spt, pan);
  }
  gui.pan.end(ctx, pan, parent);
  dbg_blk_end(ctx->sys);
}
static int
ui_file_sel(dyn(char) *filepath, struct file_view *fs, struct gui_ctx *ctx,
            struct gui_panel *pan, struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(parent);
  assert(filepath);

  int ret = 0;
  dbg_blk_begin(ctx->sys, "app:gui:file:main");
  gui.pan.begin(ctx, pan, parent);
  {
    int gap = ctx->cfg.pan_gap[1];
    struct gui_box lay = pan->box;
    struct gui_btn open = {.box = gui.cut.bot(&lay, ctx->cfg.item, gap)};
    struct gui_panel nav = {.box = gui.cut.top(&lay, ctx->cfg.item, gap)};
    struct gui_panel tbl = {.box = lay};
    ui_file_lst_view_nav_bar(fs, &fs->lst, ctx, &nav, pan);
    ui_file_sel_split(fs, ctx, &tbl, pan);

    /* file selection */
    int dis = fs->lst.sel_idx < 0 || fs->lst.sel_idx >= dyn_cnt(fs->lst.elms);
    if (!dis) {
      const struct file_elm *elm = 0;
      elm = fs->lst.elms + fs->lst.sel_idx;
      dis = elm->isdir || !elm->isvalid;
    }
    scp_gui_disable_on(&gui, ctx, dis) {
      if (gui.btn.ico_txt(ctx, &open, pan, strv("Open"), ICO_FILE_IMPORT, -1)) {
        const struct file_elm *elm = fs->lst.elms + fs->lst.sel_idx;
        dyn_asn_str(*filepath, ctx->sys, elm->fullpath);
        ret = 1;
      }
      gui.tooltip(ctx, &open.pan, strv("Open SQLite Database"));
    }
  }
  gui.pan.end(ctx, pan, parent);
  dbg_blk_end(ctx->sys);
  return ret;
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
extern void dlExport(void *export, void *import);
static const struct file_picker_api pck_api = {
  .init = file_view_init,
  .update = file_view_update,
  .shutdown = file_view_free,
  .ui = ui_file_sel,
};
static void
pck_get_api(void *export, void *import) {
  unused(import);
  struct file_picker_api *exp = cast(struct file_picker_api*, export);
  *exp = pck_api;
}
#ifdef DEBUG_MODE
extern void
dlExport(void *export, void *import) {
  struct gui_api *im = cast(struct gui_api*, import);
  pck_get_api(export, import);
  gui = *im;
}
#endif

