/* =============================================================================
 *
 *                                File Picker
 *
 * =============================================================================
 */
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
  struct str suffix;
  struct str name;
  struct str mime;
  unsigned id;
  int grp;
};
struct file_group_def {
  struct str name;
  enum res_ico_id icon;
};
// clang-format off
static const struct file_group_def file_groups[] = {
  [FILE_GRP_OTHER]      = {.name = strv("Other"),         .icon = RES_ICO_FILE},
  [FILE_GRP_DIR]        = {.name = strv("Folder"),        .icon = RES_ICO_FOLDER_OPEN},
  [FILE_GRP_TXT]        = {.name = strv("Text"),          .icon = RES_ICO_FILE_TXT},
  [FILE_GRP_PROG]       = {.name = strv("Programming"),   .icon = RES_ICO_FILE_CODE},
  [FILE_GRP_AUDIO]      = {.name = strv("Audio"),         .icon = RES_ICO_FILE_AUDIO},
  [FILE_GRP_VIDEO]      = {.name = strv("Video"),         .icon = RES_ICO_FILE_VIDEO},
  [FILE_GRP_IMAGE]      = {.name = strv("Image"),         .icon = RES_ICO_FILE_IMG},
  [FILE_GRP_FONT]       = {.name = strv("Font"),          .icon = RES_ICO_FILE},
  [FILE_GRP_3D]         = {.name = strv("3DModel"),       .icon = RES_ICO_FILE},
  [FILE_GRP_EXEC]       = {.name = strv("Executable"),    .icon = RES_ICO_FILE},
  [FILE_GRP_ARCHIVE]    = {.name = strv("Archive"),       .icon = RES_ICO_FILE_ARCHIVE},
};
static const struct file_def file_defs[] = {
  /* unkown */
  {strv(""),                .name = strv("Unknown"),                                              .mime = strv("application/octet-stream"),     .grp = FILE_GRP_OTHER},
  {str_inv,                 .name = strv("Folder"),                                               .mime = strv("application/octet-stream"),     .grp = FILE_GRP_DIR},
  {str_inv,                 .name = strv("Link"),                                                 .mime = strv("application/octet-stream"),     .grp = FILE_GRP_OTHER},
  {str_inv,                 .name = strv("Socket"),                                               .mime = strv("application/octet-stream"),     .grp = FILE_GRP_OTHER},
  {str_inv,                 .name = strv("Fifo"),                                                 .mime = strv("application/octet-stream"),     .grp = FILE_GRP_OTHER},
  /* files */
  {.suffix = strv("c"),     .id = 'c',              .name = strv("C Source"),                     .mime = strv("text/x-c"),                     .grp = FILE_GRP_TXT},
  {.suffix = strv("d"),     .id = 'd',              .name = strv("D Source"),                     .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("h"),     .id = 'h',              .name = strv("C Header"),                     .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("m"),     .id = 'm',              .name = strv("ObjC Source"),                  .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("o"),     .id = 'o',              .name = strv("Unix Object Format"),           .mime = strv("application/octet-stream"),     .grp = FILE_GRP_EXEC},
  {.suffix = strv("r"),     .id = 'r',              .name = strv("R Script"),                     .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("s"),     .id = 's',              .name = strv("Assembly Source"),              .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("z"),     .id = 'z',              .name = strv("Z Archive"),                    .mime = strv("application/x-compress"),       .grp = FILE_GRP_ARCHIVE},

  {.suffix = strv("7z"),    .id = twocc("7z"),      .name = strv("7Zip Archive"),                 .mime = strv("application/x-7z-compressed"),  .grp = FILE_GRP_ARCHIVE},
  {.suffix = strv("cs"),    .id = twocc("cs"),      .name = strv("C# Source"),                    .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("el"),    .id = twocc("el"),      .name = strv("Emacs Lisp"),                   .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("go"),    .id = twocc("go"),      .name = strv("Golang Source"),                .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("gz"),    .id = twocc("gz"),      .name = strv("Compressed TAR Archive"),       .mime = strv("application/x-gtar"),           .grp = FILE_GRP_ARCHIVE},
  {.suffix = strv("hs"),    .id = twocc("hs"),      .name = strv("Haskell Source"),               .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("js"),    .id = twocc("js"),      .name = strv("JavaScript"),                   .mime = strv("application/javascript"),       .grp = FILE_GRP_TXT},
  {.suffix = strv("md"),    .id = twocc("md"),      .name = strv("Markdown"),                     .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("pl"),    .id = twocc("pl"),      .name = strv("Perl Script"),                  .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("py"),    .id = twocc("py"),      .name = strv("Python Script"),                .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("rb"),    .id = twocc("rb"),      .name = strv("Ruby Script"),                  .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("sh"),    .id = twocc("sh"),      .name = strv("Shell Script"),                 .mime = strv("application/x-sh"),             .grp = FILE_GRP_EXEC},
  {.suffix = strv("so"),    .id = twocc("so"),      .name = strv("Shared Object Format"),         .mime = strv("application/octet-stream"),     .grp = FILE_GRP_EXEC},

  {.suffix = strv("3ds"),   .id = threecc("3ds"),   .name = strv("Legacy 3DS Max Model"),         .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("abf"),   .id = threecc("abf"),   .name = strv("Adobe Binary Screen Font"),     .mime = strv("application/octet-stream"),     .grp = FILE_GRP_FONT},
  {.suffix = strv("ada"),   .id = threecc("ada"),   .name = strv("Ada Source"),                   .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("afm"),   .id = threecc("afm"),   .name = strv("Adobe Font Metrics"),           .mime = strv("application/octet-stream"),     .grp = FILE_GRP_FONT},
  {.suffix = strv("apk"),   .id = threecc("apk"),   .name = strv("Android Application Package"),  .mime = strv("application/vnd.android.package-archive"),  .grp = FILE_GRP_EXEC},
  {.suffix = strv("asf"),   .id = threecc("asf"),   .name = strv("ASF Video Container"),          .mime = strv("video/x-ms-asf"),               .grp = FILE_GRP_VIDEO},
  {.suffix = strv("asm"),   .id = threecc("asm"),   .name = strv("Assembly Source"),              .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("avi"),   .id = threecc("avi"),   .name = strv("Audio Video Interleave"),       .mime = strv("video/x-msvideo"),              .grp = FILE_GRP_VIDEO},
  {.suffix = strv("b3d"),   .id = threecc("b3d"),   .name = strv("Blitz3D Model"),                .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("bdl"),   .id = threecc("bdl"),   .name = strv("Nintendo 3d Model"),            .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("bik"),   .id = threecc("bik"),   .name = strv("Bink Video"),                   .mime = strv("application/octet-stream"),     .grp = FILE_GRP_VIDEO},
  {.suffix = strv("bin"),   .id = threecc("bin"),   .name = strv("Binary File"),                  .mime = strv("application/octet-stream"),     .grp = FILE_GRP_ARCHIVE},
  {.suffix = strv("blp"),   .id = threecc("blp"),   .name = strv("Blizzard Texture"),             .mime = strv("application/octet-stream"),     .grp = FILE_GRP_IMAGE},
  {.suffix = strv("bmd"),   .id = threecc("bmd"),   .name = strv("Nintendo 3d Model"),            .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("bmp"),   .id = threecc("bmp"),   .name = strv("Bitmap"),                       .mime = strv("image/bmp"),                    .grp = FILE_GRP_IMAGE},
  {.suffix = strv("bti"),   .id = threecc("bti"),   .name = strv("Nintendo Texture"),             .mime = strv("application/octet-stream"),     .grp = FILE_GRP_IMAGE},
  {.suffix = strv("bz2"),   .id = threecc("bz2"),   .name = strv("Compressed TAR Archive"),       .mime = strv("application/x-bzip2"),          .grp = FILE_GRP_ARCHIVE},
  {.suffix = strv("c4d"),   .id = threecc("c4d"),   .name = strv("Cinema 4d Model"),              .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("chm"),   .id = threecc("chm"),   .name = strv("Compiled HTML Help"),           .mime = strv("application/vnd.ms-htmlhelp"),  .grp = FILE_GRP_TXT},
  {.suffix = strv("clj"),   .id = threecc("clj"),   .name = strv("Clojure Source"),               .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("cob"),   .id = threecc("cob"),   .name = strv("Caligary Object"),              .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("cpp"),   .id = threecc("cpp"),   .name = strv("C++ Source"),                   .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("css"),   .id = threecc("css"),   .name = strv("CSS"),                          .mime = strv("text/css"),                     .grp = FILE_GRP_TXT},
  {.suffix = strv("csv"),   .id = threecc("csv"),   .name = strv("Comma-separated Values"),       .mime = strv("text/csv"),                     .grp = FILE_GRP_TXT},
  {.suffix = strv("cxx"),   .id = threecc("cxx"),   .name = strv("C++ Source"),                   .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("dae"),   .id = threecc("dae"),   .name = strv("Collada Model"),                .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("dll"),   .id = threecc("dll"),   .name = strv("Windows Dynamic Library"),      .mime = strv("application/octet-stream"),     .grp = FILE_GRP_EXEC},
  {.suffix = strv("exe"),   .id = threecc("exe"),   .name = strv("Windows Executable"),           .mime = strv("application/x-msdownload"),     .grp = FILE_GRP_EXEC},
  {.suffix = strv("fbx"),   .id = threecc("fbx"),   .name = strv("Autodesk FBX"),                 .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("flv"),   .id = threecc("flv"),   .name = strv("Flash Video"),                  .mime = strv("video/x-flv"),                  .grp = FILE_GRP_VIDEO},
  {.suffix = strv("gif"),   .id = threecc("gif"),   .name = strv("Graphic Interchange"),          .mime = strv("image/gif"),                    .grp = FILE_GRP_IMAGE},
  {.suffix = strv("hpp"),   .id = threecc("hpp"),   .name = strv("C++ Header"),                   .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("hxx"),   .id = threecc("hxx"),   .name = strv("C++ Header"),                   .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("ico"),   .id = threecc("ico"),   .name = strv("Windows Icon"),                 .mime = strv("image/x-icon"),                 .grp = FILE_GRP_IMAGE},
  {.suffix = strv("iso"),   .id = threecc("iso"),   .name = strv("Optical Media"),                .mime = strv("application/octet-stream"),     .grp = FILE_GRP_ARCHIVE},
  {.suffix = strv("jar"),   .id = threecc("jar"),   .name = strv("Jave Zipped Container"),        .mime = strv("application/java-archive"),     .grp = FILE_GRP_ARCHIVE},
  {.suffix = strv("jp2"),   .id = threecc("jp2"),   .name = strv("JPEG 2000"),                    .mime = strv("image/jpeg"),                   .grp = FILE_GRP_IMAGE},
  {.suffix = strv("jpg"),   .id = threecc("jpg"),   .name = strv("JPEG"),                         .mime = strv("image/jpeg"),                   .grp = FILE_GRP_IMAGE},
  {.suffix = strv("lua"),   .id = threecc("lua"),   .name = strv("Lua Script"),                   .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("max"),   .id = threecc("max"),   .name = strv("Autodesk 3DS Max"),             .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("md2"),   .id = threecc("md2"),   .name = strv("Quake 2 Model"),                .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("md3"),   .id = threecc("md3"),   .name = strv("Quake 3 Model"),                .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("md5"),   .id = threecc("md4"),   .name = strv("Doom 3 Model"),                 .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("mdx"),   .id = threecc("mdx"),   .name = strv("Blizzard Model "),              .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("mkv"),   .id = threecc("mkv"),   .name = strv("Matroska Video Container"),     .mime = strv("application/octet-stream"),     .grp = FILE_GRP_VIDEO},
  {.suffix = strv("mov"),   .id = threecc("mov"),   .name = strv("QuickTime Video Container"),    .mime = strv("video/quicktime"),              .grp = FILE_GRP_VIDEO},
  {.suffix = strv("mp3"),   .id = threecc("mp3"),   .name = strv("MP3 Audio File"),               .mime = strv("audio/mpeg"),                   .grp = FILE_GRP_AUDIO},
  {.suffix = strv("mp4"),   .id = threecc("mp4"),   .name = strv("MPEG Video"),                   .mime = strv("video/mp4"),                    .grp = FILE_GRP_VIDEO},
  {.suffix = strv("mpe"),   .id = threecc("mpe"),   .name = strv("MPEG Video"),                   .mime = strv("video/mpeg"),                   .grp = FILE_GRP_VIDEO},
  {.suffix = strv("mpg"),   .id = threecc("mpg"),   .name = strv("MPEG Video"),                   .mime = strv("video/mpeg"),                   .grp = FILE_GRP_VIDEO},
  {.suffix = strv("mpq"),   .id = threecc("mpq"),   .name = strv("Blizzard Archive"),             .mime = strv("application/octet-stream"),     .grp = FILE_GRP_ARCHIVE},
  {.suffix = strv("obj"),   .id = threecc("obj"),   .name = strv("Wavefront .obj"),               .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("ogg"),   .id = threecc("ogg"),   .name = strv("Ogg Video"),                    .mime = strv("video/ogg"),                    .grp = FILE_GRP_VIDEO},
  {.suffix = strv("otf"),   .id = threecc("otf"),   .name = strv("OpenType Font"),                .mime = strv("application/x-font-otf"),       .grp = FILE_GRP_FONT},
  {.suffix = strv("pbm"),   .id = threecc("pbm"),   .name = strv("Portable Bitmap"),              .mime = strv("image/x-portable-bitmap"),      .grp = FILE_GRP_IMAGE},
  {.suffix = strv("pcx"),   .id = threecc("pcx"),   .name = strv("Picture Exchange Format"),      .mime = strv("image/x-pcx"),                  .grp = FILE_GRP_IMAGE},
  {.suffix = strv("pdf"),   .id = threecc("pdf"),   .name = strv("Portable Document Format"),     .mime = strv("application/pdf"),              .grp = FILE_GRP_TXT},
  {.suffix = strv("php"),   .id = threecc("php"),   .name = strv("PHP Script"),                   .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("pk3"),   .id = threecc("pk3"),   .name = strv("Quake 3 Archive"),              .mime = strv("application/zip"),              .grp = FILE_GRP_ARCHIVE},
  {.suffix = strv("pk4"),   .id = threecc("pk4"),   .name = strv("Doom 3 Archive"),               .mime = strv("application/zip"),              .grp = FILE_GRP_ARCHIVE},
  {.suffix = strv("png"),   .id = threecc("png"),   .name = strv("PNG"),                          .mime = strv("image/png"),                    .grp = FILE_GRP_IMAGE},
  {.suffix = strv("psd"),   .id = threecc("psd"),   .name = strv("Adobe Photoshop"),              .mime = strv("application/octet-stream"),     .grp = FILE_GRP_IMAGE},
  {.suffix = strv("rar"),   .id = threecc("rar"),   .name = strv("RAR Archive"),                  .mime = strv("application/x-rar-compressed"), .grp = FILE_GRP_ARCHIVE},
  {.suffix = strv("raw"),   .id = threecc("raw"),   .name = strv("Raw Image Data"),               .mime = strv("application/octet-stream"),     .grp = FILE_GRP_IMAGE},
  {.suffix = strv("roq"),   .id = threecc("roq"),   .name = strv("Quake 3 Video"),                .mime = strv("application/octet-stream"),     .grp = FILE_GRP_VIDEO},
  {.suffix = strv("smd"),   .id = threecc("smd"),   .name = strv("Valves Studiomdl"),             .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("swf"),   .id = threecc("swf"),   .name = strv("Macromedia Flash"),             .mime = strv("application/octet-stream"),     .grp = FILE_GRP_VIDEO},
  {.suffix = strv("tar"),   .id = threecc("tar"),   .name = strv("TAR Archive"),                  .mime = strv("application/x-gtar"),           .grp = FILE_GRP_ARCHIVE},
  {.suffix = strv("tcl"),   .id = threecc("tcl"),   .name = strv("TCL Script"),                   .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("tga"),   .id = threecc("tga"),   .name = strv("Truevision TGA"),               .mime = strv("application/octet-stream"),     .grp = FILE_GRP_IMAGE},
  {.suffix = strv("thp"),   .id = threecc("thp"),   .name = strv("Nintendo Video"),               .mime = strv("application/octet-stream"),     .grp = FILE_GRP_VIDEO},
  {.suffix = strv("tif"),   .id = threecc("tif"),   .name = strv("Tagged Image Format"),          .mime = strv("image/tiff"),                   .grp = FILE_GRP_IMAGE},
  {.suffix = strv("ttf"),   .id = threecc("ttf"),   .name = strv("TrueType Font"),                .mime = strv("application/x-font-ttf"),       .grp = FILE_GRP_FONT},
  {.suffix = strv("txt"),   .id = threecc("txt"),   .name = strv("Text"),                         .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("vtf"),   .id = threecc("vtf"),   .name = strv("Valve Texture"),                .mime = strv("application/octet-stream"),     .grp = FILE_GRP_IMAGE},
  {.suffix = strv("wav"),   .id = threecc("wav"),   .name = strv("WAV Audio File"),               .mime = strv("audio/x-wav"),                  .grp = FILE_GRP_AUDIO},
  {.suffix = strv("xbe"),   .id = threecc("xbe"),   .name = strv("Xbox Executable"),              .mime = strv("application/x-msdownload"),     .grp = FILE_GRP_EXEC},
  {.suffix = strv("xbm"),   .id = threecc("xbm"),   .name = strv("X Pixmap"),                     .mime = strv("image/x-xbitmap"),              .grp = FILE_GRP_IMAGE},
  {.suffix = strv("xcf"),   .id = threecc("xcf"),   .name = strv("GIMP Image"),                   .mime = strv("application/octet-stream"),     .grp = FILE_GRP_IMAGE},
  {.suffix = strv("xex"),   .id = threecc("xex"),   .name = strv("Xbox 360 Executable"),          .mime = strv("application/x-msdownload"),     .grp = FILE_GRP_EXEC},
  {.suffix = strv("xml"),   .id = threecc("xml"),   .name = strv("Extensive Markup Language"),    .mime = strv("application/xml"),              .grp = FILE_GRP_TXT},
  {.suffix = strv("xpm"),   .id = threecc("xpm"),   .name = strv("X Pixmap"),                     .mime = strv("image/x-xpixmap"),              .grp = FILE_GRP_IMAGE},
  {.suffix = strv("zip"),   .id = threecc("zip"),   .name = strv("Zip Archive"),                  .mime = strv("application/zip"),              .grp = FILE_GRP_ARCHIVE},

  {.suffix = strv("blend"), .id = fourcc("blend"),  .name = strv("Blender Model"),                .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("brres"), .id = fourcc("brres"),  .name = strv("Nintendo 3d Model"),            .mime = strv("application/octet-stream"),     .grp = FILE_GRP_3D},
  {.suffix = strv("html"),  .id = fourcc("html"),   .name = strv("Hyper Text Markup Language"),   .mime = strv("text/html"),                    .grp = FILE_GRP_TXT},
  {.suffix = strv("java"),  .id = fourcc("java"),   .name = strv("Java Source"),                  .mime = strv("text/plain"),                   .grp = FILE_GRP_TXT},
  {.suffix = strv("json"),  .id = fourcc("json"),   .name = strv("JavaScript Object Notation"),   .mime = strv("application/json"),             .grp = FILE_GRP_TXT},
  {.suffix = strv("mpeg"),  .id = fourcc("mpeg"),   .name = strv("MPEG Video"),                   .mime = strv("video/mpeg"),                   .grp = FILE_GRP_VIDEO},
  {.suffix = strv("webm"),  .id = fourcc("webm"),   .name = strv("Web Video"),                    .mime = strv("application/octet-stream"),     .grp = FILE_GRP_VIDEO},
};
static const struct file_tbl_col_def file_tbl_def[FILE_TBL_MAX] = {
  [FILE_TBL_NAME] =  {.title = strv("Name"),           .ui = {.type = GUI_LAY_SLOT_DYN, .size =   1, .con = {200, 800}}},
  [FILE_TBL_TYPE] =  {.title = strv("Type"),           .ui = {.type = GUI_LAY_SLOT_FIX, .size = 100, .con = {100, 800}}},
  [FILE_TBL_SIZE] =  {.title = strv("Size"),           .ui = {.type = GUI_LAY_SLOT_FIX, .size = 120, .con = {100, 800}}},
  [FILE_TBL_PERM] =  {.title = strv("Permission"),     .ui = {.type = GUI_LAY_SLOT_FIX, .size = 160, .con = {100, 800}}},
  [FILE_TBL_DATE] =  {.title = strv("Date Modified"),  .ui = {.type = GUI_LAY_SLOT_FIX, .size = 300, .con = {200, 800}}},
};
static const struct gui_split_lay_slot file_split_def[FILE_SPLIT_MAX] = {
  {.type = GUI_LAY_SLOT_FIX, .size = 400, .con = {100, 1000}},
  {.type = GUI_LAY_SLOT_DYN, .size =   1, .con = {100, 10000}},
};
// clang-format on

/* -----------------------------------------------------------------------------
 *                                  FILE
 * -----------------------------------------------------------------------------
 */
static int
file_type(struct str ext) {
  unsigned u = 0;
  unsigned int i = 0;
  if (ext.len == 1) {
    u = castu(ext.str[0]);
    unsigned k = (u * 0x3d7a774e) >> 29u;
    i = 5 + ((0x20465731ull >> (unsigned long long)(k*4)) & 0xf);
  } else if (ext.len == 2) {
    u = twocc(ext.str);
    unsigned k = (u * 0x7172a30bu) >> 28u;
    i = 13 + (0xa6314728ull >> (unsigned long long)(k*4)) & 0xf;
  } else if (ext.len == 3) {
    static const unsigned char t[256] = {72, 0, 0, 0, 0, 0, 14, 78, 0, 0, 0, 0,
      45, 0, 0, 0, 0, 0, 80, 35, 56, 82, 0, 3, 0, 0, 48, 42, 0, 61, 0, 8, 2, 0,
      51, 63, 0, 0, 43, 11, 0, 0, 0, 22, 0, 7, 0, 0, 0, 79, 0, 33, 4, 0, 0, 21,
      0, 5, 0, 0, 0, 12, 0, 70, 0, 59, 0, 0, 73, 0, 0, 0, 0, 0, 6, 81, 0, 0, 20,
      77, 37, 0, 0, 0, 0, 0, 0, 18, 0, 0, 0, 0, 0, 0, 71, 41, 0, 76, 0, 58, 0,
      36, 0, 0, 0, 0, 0, 0, 24, 31, 30, 0, 0, 0, 0, 15, 26, 52, 0, 0, 69, 0, 0,
      75, 0, 67, 0, 9, 50, 40, 16, 66, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0,
      0, 0, 54, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 32, 0, 0, 27, 0, 47,
      0, 0, 0, 0, 0, 0, 0, 74, 28, 0, 0, 0, 0, 0, 0, 0, 38, 0, 0, 0, 0, 0, 0, 0,
      17, 34, 0, 25, 0, 23, 0, 0, 0, 46, 53, 60, 39, 29, 62, 0, 0, 0, 0, 0, 0, 0,
      49, 0, 0, 0, 0, 0, 68, 0, 0, 0, 0, 19, 57, 1, 55, 0, 0, 0, 0, 44, 0, 0, 64,
      0, 0, 65
    };
    u = threecc(ext.str);
    unsigned k = (u * 0xDDC8D774) >> 24u;
    i = t[k];
  } else {
    u = fourcc(ext.str);
    unsigned k = (u * 0x76e2bba) >> 29u;
    i = 95 + ((0x5142036ull >> (unsigned long long)(k*4)) & 0xf);
  }
  return (file_defs[i].id == u) ? casti(i) : 0;
}
static enum res_ico_id
file_icon(int file_type) {
  assert(file_type >= 0 && file_type < cntof(file_defs));
  const struct file_def *def = file_defs + file_type;
  return file_groups[def->grp].icon;
}

/* -----------------------------------------------------------------------------
 *                                  LIST
 * -----------------------------------------------------------------------------
 */
struct file_view_lst_qry {
  int page;
  struct str fltr;
  struct str fullpath;
  int(*cmp)(const void*, const void*);
};
static int
file_view_lst_elm_cmp_name_asc(const void *ap, const void *bp) {
  const struct file_elm *a = (const struct file_elm*)ap;
  const struct file_elm *b = (const struct file_elm*)bp;
  int eq = b->isdir - a->isdir;
  if (eq != 0) {
    return eq;
  }
  return str_cmp(a->name, b->name);
}
static int
file_view_lst_elm_cmp_name_dec(const void *ap, const void *bp) {
  const struct file_elm *a = (const struct file_elm*)ap;
  const struct file_elm *b = (const struct file_elm*)bp;
  int eq = a->isdir - b->isdir;
  if (eq != 0) {
    return eq;
  }
  return str_cmp(b->name, b->name);
}
static void
file_view_lst_elm_swp(void *ap, void *bp) {
  struct file_elm *a = (struct file_elm*)ap;
  struct file_elm *b = (struct file_elm*)bp;
  struct file_elm tmp = *a;
  *a = *b, *b = tmp;
}
static void
file_view_lst_elm_init(struct file_elm *elm, struct sys *s,
                       struct str path, struct str name) {
  assert(s);
  assert(elm);
  assert(str_is_val(name));
  assert(str_is_val(path));

  char buf[MAX_FILE_PATH];
  struct str ospath = str_fmtsn(buf, cntof(buf), "%.*s/%.*s", strf(path), strf(name));
  struct str ext = path_ext(ospath);
  mset(elm,0,szof(elm[0]));

  struct sys_file_info info = {0};
  if (!s->file.info(s, &info, ospath, s->mem.tmp)) {
    elm->perm = 0;
    elm->name = name;
    elm->file_type = 0;
    elm->sys_type = SYS_FILE_DEF;
    return;
  }
  elm->name = name;
  elm->size = info.siz;
  elm->mtime = info.mtime;
  elm->sys_type = info.type;
  elm->perm = info.perm;
  if (elm->sys_type == SYS_FILE_DEF ||
      elm->sys_type == SYS_FILE_LNK) {
    elm->file_type = castu(file_type(ext));
  } else if (elm->sys_type == SYS_FILE_DIR) {
    elm->file_type = 1;
    elm->isdir = 1;
  }
}
static int
file_view_lst_partition(struct file_elm *a, int(*cmp)(const void*, const void*)) {
  assert(a);
  assert(cmp);
  int n = FILE_LIST_ELM_BUF_CNT;
  int piv_idx = FILE_LIST_ELM_CNT;

  struct file_elm piv = a[piv_idx];
  a[piv_idx] = a[0];
  piv_idx = (cmp(&a[n-1], &piv) < 0) ? n-1 : 0;
  a[0] = a[piv_idx];
  a[piv_idx] = piv;

  int i = 0, j = n-1;
  for (;;) {
    do {i++;} while (cmp(&a[i], &piv) < 0);
    do {j--;} while (cmp(&piv, &a[j]) < 0);
    if (i >= j) {break;}
    file_view_lst_elm_swp(&a[i], &a[j]);
  }
  j += (piv_idx != 0);
  file_view_lst_elm_swp(&a[j], &a[piv_idx]);
  return j;
}
static struct str
file_view_lst_str(struct file_list_view *lst, struct str name, int cur) {
  assert(lst);
  int cap = cntof(lst->page.txt.buf[0]) - lst->page.txt.cnt;
  char *buf = lst->page.txt.buf[cur] + lst->page.txt.cnt;
  struct str ret = str_set(buf, cap, name);
  if (str_is_val(name)) {
    lst->page.txt.cnt += name.len;
  }
  return ret;
}
static void
file_view_lst_qry(struct file_list_view *lst, struct sys *s, struct arena *tmp,
                  const struct file_view_lst_qry *qry) {
  assert(s);
  assert(fs);
  assert(qry);
  assert(lst);

  lst->page.cnt = 0;
  lst->page.total = 0;

  /* setup pivot */
  char lbl[MAX_FILE_NAME];
  int pidx = (qry->page >= lst->page.idx) ? 0 : lst->page.cnt-1;
  struct file_elm piv = lst->page.elms[pidx];
  piv.name = str_set(lbl, cntof(lbl), piv.name);
  lst->page.cur = qry->page < lst->page.idx;

  /* iterate directory */
  struct sys_dir_iter it = {0};
  for sys_dir_lst_each(s, &it, tmp, qry->fullpath) {
    if (str_eq(it.name, strv(".")) ||
        str_eq(it.name, strv(".."))) {
      continue;
    }
    lst->page.total++;
    /* apply filter */
    struct file_elm elm = {0};
    file_view_lst_elm_init(&elm, s, qry->fullpath, it.name);
    if ((qry->fltr.len && str_fnd(elm.name, qry->fltr) >= elm.name.len) ||
        (qry->page > lst->page.idx && qry->cmp(&elm, &piv) < 0) ||
        (qry->page < lst->page.idx && qry->cmp(&elm, &piv) > 0)) {
      continue;
    }
    elm.name = file_view_lst_str(lst, elm.name, lst->page.txt.cur);
    if (str_is_inv(elm.name)) {
      continue;
    }
    /* add file element */
    int idx = (qry->page < lst->page.idx) ?
      cntof(lst->page.elms) - ++lst->page.cnt :
      lst->page.cnt++;
    int at = lst->page.cur * FILE_LIST_ELM_CNT + idx;
    lst->page.elms[at] = elm;

    if (lst->page.cnt >= FILE_LIST_ELM_BUF_CNT) {
      /* apply partition sorting */
      lst->page.txt.cnt = 0;
      lst->page.txt.cur = !lst->page.txt.cur;
      file_view_lst_partition(lst->page.elms, qry->cmp);
      for loop(i, lst->page.cnt) {
        int off = lst->page.cur * FILE_LIST_ELM_CNT + i;
        struct file_elm *cur = &lst->page.elms[off];
        cur->name = file_view_lst_str(lst, cur->name, lst->page.txt.cur);
      }
      lst->page.cnt = FILE_LIST_ELM_CNT;
    }
  }
  /* sort current page */
  void *ptr = lst->page.elms + lst->page.cur * FILE_LIST_ELM_CNT;
  if (lst->page.cnt >= FILE_LIST_ELM_BUF_CNT) {
    file_view_lst_partition(ptr, qry->cmp);
  }
  qsort(ptr, lst->page.cnt, szof(lst->page.elms[0]), qry->cmp);
  lst->page_cnt = (lst->page.total + FILE_LIST_ELM_CNT - 1) / FILE_LIST_ELM_CNT;
  lst->page.idx = qry->page;
}
static void
file_view_lst_clr(struct file_list_view *lst) {
  assert(lst);
  zero2(lst->off);
  lst->sel_idx = -1;

  lst->page_cnt = 0;
  lst->page.cnt = 0;
  lst->page.idx = 0;
  lst->page.cur = 0;

  lst->page.txt.cnt = 0;
  lst->page.txt.cur = 0;
  lst->page.txt.buf[0][0] = 0;
  lst->page.txt.buf[1][0] = 0;
}
static void
file_view_lst_cd(struct file_view *fs, struct file_list_view *lst,
                 struct sys *s, struct str fullpath) {
  assert(s);
  assert(fs);
  assert(lst);

  lst->nav_path = str_set(lst->nav_buf, cntof(lst->nav_buf), fullpath);
  if (str_is_val(lst->nav_path)) {
    file_view_lst_clr(&fs->lst);

    struct file_view_lst_qry qry = {0};
    qry.fltr = lst->fltr;
    qry.fullpath = lst->nav_path;
    qry.cmp = file_view_lst_elm_cmp_name_asc;
    file_view_lst_qry(lst, s, fs->tmp_arena, &qry);
  }
}
static void
file_view_lst_init(struct file_view *fs, struct sys *s, struct gui_ctx *ctx) {
  assert(ctx);
  assert(fs);
  assert(s);

  /* setup list table */
  struct gui_split_lay_cfg tbl_cfg = {0};
  tbl_cfg.size = szof(struct file_tbl_col_def);
  tbl_cfg.off = offsetof(struct file_tbl_col_def, ui);
  tbl_cfg.slots = file_tbl_def;
  tbl_cfg.cnt = FILE_TBL_MAX;
  gui.tbl.lay(fs->lst.tbl.state, ctx, &tbl_cfg);
  /* open home directory */
  file_view_lst_cd(fs, &fs->lst, s, fs->home);
}

/* -----------------------------------------------------------------------------
 *                                  TREE
 * -----------------------------------------------------------------------------
 */
static struct lst_elm*
file_tree_node_sort_after_elm(struct file_tree_node *n,
                              struct file_tree_node *s) {
  assert(s);
  assert(n);
  struct lst_elm *elm = 0;
  for lst_each(elm, &n->sub) {
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
file_tree_node_init(struct sys *_sys, struct arena *mem, struct file_tree_node *s,
                    struct file_tree_node *n, struct str p, unsigned long long id) {
  assert(_sys);
  assert(mem);
  assert(s);
  assert(n);

  s->id = id;
  s->parent = n;
  s->depth = n->depth + 1;
  s->fullpath = arena_str(mem, _sys, p);

  lst_init(&s->hook);
  lst_init(&s->sub);
  file_tree_node_lnk(n, s);
}
static struct file_tree_node*
file_node_alloc(struct file_tree_view *tree, struct sys *_sys, struct arena *mem) {
  assert(tree);
  assert(_sys);
  assert(mem);

  struct file_tree_node *s = 0;
  if (lst_any(&tree->del_lst)) {
    s = lst_get(lst_first(&tree->del_lst), struct file_tree_node, hook);
    lst_del(&s->hook);
    memset(s, 0, sizeof(*s));
  } else {
    s = arena_alloc(mem, _sys, szof(*s));
  }
  return s;
}
static struct file_tree_node*
file_view_tree_node_new(struct file_tree_view *tree, struct sys *_sys,
                        struct arena *mem, struct file_tree_node *n,
                        struct str path, unsigned long long id) {
  assert(tree);
  assert(_sys);
  assert(mem);
  assert(n);

  struct file_tree_node *s = 0;
  s = file_node_alloc(tree, _sys, mem);
  file_tree_node_init(_sys, mem, s, n, path, id);
  return s;
}
static void
file_view_tree_node_del(struct file_tree_view *tree, struct file_tree_node *n) {
  assert(n);
  assert(tree);

  struct lst_elm *elm = 0;
  struct lst_elm *item = 0;
  for lst_each_safe(elm, item, &n->sub) {
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
                     struct file_tree_node *n, struct sys *_sys,
                     struct arena *mem, struct arena *tmp) {
  assert(tree);
  assert(_sys);
  assert(mem);
  assert(tmp);
  assert(n);

  int upt = 0;
  if (n->parent && !set_fnd(tree->exp, n->parent->id)) {
    return upt;
  }
  struct arena_scope scp;
  confine arena_scope(tmp, &scp, _sys) {
    unsigned long long *set = arena_set(tmp, _sys, 1024);
    struct lst_elm *elm = 0;
    for lst_each(elm, &n->sub) {
      struct file_tree_node *s;
      s = lst_get(elm, struct file_tree_node, hook);
      set_put(set, _sys, s->id);
    }
    /* validate child nodes */
    struct sys_dir_iter it = {0};
    for sys_dir_lst_each(_sys, &it, tmp, n->fullpath) {
      if (it.name.str[0] == '.' || !_sys->dir.exists(_sys, it.fullpath, tmp)) {
        continue;
      }
      unsigned long long id = str_hash(it.fullpath);
      if (!set_fnd(set, id)) {
        file_view_tree_node_new(tree, _sys, mem, n, it.fullpath, id);
        upt = 1;
      }
      set_del(set, id);
    }
    /* remove deleted directory nodes */
    struct lst_elm *safe = 0;
    for lst_each_safe(elm, safe, &n->sub) {
      struct file_tree_node *s;
      s = lst_get(elm, struct file_tree_node, hook);
      if (!set_fnd(set, s->id)) {
        continue;
      }
      file_view_tree_node_del(tree, s);
    }
    set_free(set, _sys);
  }
  /* recurse child directory nodes */
  struct lst_elm *it = 0;
  for lst_each(it, &n->sub) {
    struct file_tree_node *s;
    s = lst_get(it, struct file_tree_node, hook);
    upt |= file_view_tree_build(tree, s, _sys, mem, tmp);
  }
  return upt;
}
static struct file_tree_node**
file_view_tree_serial(struct file_tree_view *tree,
                      struct file_tree_node *n,
                      struct file_tree_node **lst,
                      struct sys *_sys) {
  assert(tree);
  assert(lst);
  assert(_sys);
  assert(n);

  dyn_add(lst, _sys, n);
  if (set_fnd(tree->exp, n->id)) {
    struct lst_elm *it = 0;
    for lst_each(it, &n->sub) {
      struct file_tree_node *s = 0;
      s = lst_get(it, struct file_tree_node, hook);
      lst = file_view_tree_serial(tree, s, lst, _sys);
    }
  }
  return lst;
}
static void
file_tree_update(struct file_view *fs, struct sys *_sys) {
  assert(fs);
  assert(_sys);

  struct arena *tmp = fs->tmp_arena;
  file_view_tree_build(&fs->tree, &fs->tree.root, _sys, &fs->tree.mem, tmp);
  dyn_clr(fs->tree.lst);
  fs->tree.lst = file_view_tree_serial(&fs->tree, &fs->tree.root, fs->tree.lst, _sys);
}
static struct file_tree_node*
file_tree_node_fnd(struct file_tree_node *n, struct str str) {
  assert(n);
  struct lst_elm *elm = 0;
  struct file_tree_node *s = 0;
  for lst_each(elm, &n->sub) {
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
                    struct sys *_sys, struct str p) {
  assert(tree);
  assert(_sys);
  assert(fs);

  int off = fs->home.len + 1;
  if (p.len < off) {
    return;
  }
  struct file_tree_node *n = &tree->root;
  struct str path = str_rhs(p, off);
  for str_tok(it, _, path, strv("/")) {
    struct str fullpath = strp(p.str, it.end);
    if (!_sys->dir.exists(_sys, fullpath, fs->tmp_arena)) {
      break;
    }
    /* create or find node to dig deeper */
    struct file_tree_node *s = file_tree_node_fnd(n, it);
    if (!s) {
      unsigned long long id = str_hash(strp(p.str, it.end));
      s = file_view_tree_node_new(tree, _sys, &tree->mem, n, fullpath, id);
    }
    tree->jmp = 1;
    tree->jmp_to = s->id;
    set_put(tree->exp, _sys, s->id);
    n = s;
  }
  file_tree_update(fs, _sys);
}
static void
file_tree_view_clr(struct file_tree_view *tree, struct sys *_sys) {
  assert(tree);
  assert(_sys);

  dyn_free(tree->lst, _sys);
  set_free(tree->exp, _sys);
  arena_free(&tree->mem, _sys);

  tree->rev = 0;
  tree->lst = 0;
  tree->exp = 0;

  lst_init(&tree->del_lst);
  memset(tree->off, 0, sizeof(tree->off));
}
static void
file_tree_view_init(struct file_view *fs, struct sys *_sys) {
  assert(fs);
  assert(_sys);

  fs->tree.lst = arena_dyn(&fs->tree.mem, _sys, struct file_tree_node*, 1024);
  fs->tree.exp = arena_set(&fs->tree.mem, _sys, 1024);
  lst_init(&fs->tree.del_lst);
  zero2(fs->tree.off);

  /* setup root node */
  fs->tree.root.fullpath = fs->home;
  fs->tree.root.id = str_hash(fs->home);
  lst_init(&fs->tree.root.hook);
  lst_init(&fs->tree.root.sub);
  set_put(fs->tree.exp, _sys, fs->tree.root.id);
}
/* -----------------------------------------------------------------------------
 *                                  VIEW
 * -----------------------------------------------------------------------------
 */
static int
file_view_init(struct file_view *fs, struct sys *_sys, struct gui_ctx *ctx,
               struct arena *tmp_arena) {
  assert(_sys);
  assert(ctx);
  assert(mem);
  assert(tmp_arena);

  fs->lst_rev = 1;
  fs->tree_rev = 1;
  fs->tmp_arena = tmp_arena;
  fs->home = str_set(fs->home_path, cntof(fs->home_path), str0(getenv("HOME")));
  if (str_is_inv(fs->home)) {
    return -ENAMETOOLONG;
  }
  struct gui_split_lay_cfg cfg = {0};
  cfg.slots = file_split_def;
  cfg.cnt = FILE_SPLIT_MAX;
  gui.splt.lay.bld(fs->split, ctx, &cfg);

  file_view_lst_init(fs, _sys, ctx);
  file_tree_view_init(fs, _sys);
  return 0;
}
static void
file_view_update(struct file_view *fs, struct sys *_sys) {
  assert(_sys);
  if (fs->tree_rev != fs->tree.rev) {
    file_tree_update(fs, _sys);
    fs->tree.rev = fs->tree_rev;
  }
}
static void
file_view_free(struct file_view *fs, struct sys *_sys) {
  assert(_sys);
  file_view_lst_clr(&fs->lst);
  file_tree_view_clr(&fs->tree, _sys);
}

/* -----------------------------------------------------------------------------
 *                                  GUI
 * -----------------------------------------------------------------------------
 */
static struct str
ui_edit_search(struct gui_ctx *ctx, struct gui_edit_box *edt,
               struct gui_panel *pan, struct gui_panel *parent,
               struct gui_txt_ed *ed, char *buf, int cap, struct str s) {
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
    gui.ico.img(ctx, &ico, pan, RES_ICO_SEARCH);

    /* edit */
    ed->buf = buf;
    ed->cap = cap;
    ed->str = s;

    edt->pan.focusable = 1;
    edt->pan.box.x = gui.bnd.min_max(ico.box.x.max, pan->box.x.max);
    edt->pan.box.x = gui.bnd.shrink(&edt->pan.box.x, pad[0]);
    edt->pan.box.y = gui.bnd.shrink(&pan->box.y, pad[1]);
    gui.edt.fld(ctx, edt, &edt->pan, pan, ed);
  }
  gui.pan.end(ctx, pan, parent);
  return ed->str;
}
static void
ui_file_lst_view_fnd(struct file_view *fs, struct file_list_view *lst,
                     struct gui_ctx *ctx, struct gui_panel *pan,
                     struct gui_panel *parent) {
  assert(lst);
  assert(ctx);
  assert(pan);
  assert(parent);
  struct gui_edit_box edt = {.box = pan->box};
  lst->fltr = ui_edit_search(ctx, &edt, pan, parent, &lst->fltr_ed,
                             lst->fltr_buf, cntof(lst->fltr_buf), lst->fltr);
  if (edt.mod) {
    file_view_lst_cd(fs, &fs->lst, ctx->sys, fs->home);
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
    if (gui.btn.ico(ctx, &home, pan, RES_ICO_HOME)) {
      /* change the directory to home  */
      lst->fltr = str_nil;
      file_view_tree_open(fs, &fs->tree, ctx->sys, fs->home);
      file_view_lst_cd(fs, &fs->lst, ctx->sys, fs->home);
    }
    gui.tooltip(ctx, &home.pan, strv("Goto Home Directory"));

    struct gui_btn up = {.box = gui.cut.rhs(&lay, ctx->cfg.item, gap)};
    if (gui.btn.ico(ctx, &up, pan, RES_ICO_FOLDER)) {
      /* go up to parent directory  */
      struct str file_name = path_file(lst->nav_path);
      struct str file_path = strp(lst->nav_path.str, file_name.str);
      if (file_path.len > fs->home.len) {
        lst->fltr = str_nil;
        struct str dir = strp(lst->nav_path.str, file_name.str);
        file_view_lst_cd(fs, &fs->lst, ctx->sys, dir);
      }
    }
    gui.tooltip(ctx, &up.pan, strv("Move to Parent Directory"));

    /* navigation bar */
    struct gui_edit_box edt = {.box = lay};
    lst->nav_ed.buf = lst->nav_buf;
    lst->nav_ed.cap = cntof(lst->nav_buf);
    lst->nav_ed.str = lst->nav_path;
    lst->nav_path = gui.edt.txt(ctx, &edt, pan, &lst->nav_ed);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui__file_view_tbl_elm_perm(char *mod, unsigned perm) {
  assert(mod);
  mod[0] = (perm & 0x01) ? 'r' : '-';
  mod[1] = (perm & 0x02) ? 'w' : '-';
  mod[2] = (perm & 0x04) ? 'x' : '-';
  mod[3] = (perm & 0x08) ? 'r' : '-';
  mod[4] = (perm & 0x10) ? 'w' : '-';
  mod[5] = (perm & 0x20) ? 'x' : '-';
  mod[6] = (perm & 0x40) ? 'r' : '-';
  mod[7] = (perm & 0x80) ? 'w' : '-';
  mod[8] = (perm & 0x100) ? 'x' : '-';
  mod[9] = 0;
}
static void
ui_file_view_tbl_elm(struct gui_ctx *ctx, struct gui_tbl *tbl,
                     const int *lay, struct gui_panel *elm,
                     const struct file_elm *fi, int is_sel) {
  assert(fi);
  assert(tbl);
  assert(lay);
  assert(elm);

  static const struct gui_align algn = {GUI_HALIGN_RIGHT, GUI_VALIGN_MID};
  static const unsigned dir_col = col_rgb_hex(0xeecd4a);
  unsigned long long elm_id = str_hash(fi->name);
  gui.tbl.lst.elm.begin(ctx, tbl, elm, elm_id, is_sel);
  {
    struct gui_cfg_stk stk[1] = {0};
    confine gui_cfg_pushu_on_scope(&gui,stk,&ctx->cfg.col[GUI_COL_ICO], dir_col, fi->isdir) {
      char perm[10];
      ui__file_view_tbl_elm_perm(perm, fi->perm);
      struct tm *mod_time = localtime(&fi->mtime);
      const struct file_def *fd = &file_defs[fi->file_type];
      enum res_ico_id ico = file_icon(fi->file_type);

      /* columns */
      gui.tbl.lst.elm.col.txt_ico(ctx, tbl, lay, elm, fi->name, ico);
      gui.tbl.lst.elm.col.txt(ctx, tbl, lay, elm, fd->name, 0);
      gui.tbl.lst.elm.col.txtf(ctx, tbl, lay, elm, &algn, "%zu", fi->size);
      gui.tbl.lst.elm.col.txt(ctx, tbl, lay, elm, str0(perm), 0);
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
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, lst->off, &lst->tbl.sort);
    {
      /* header */
      int tbl_lay[GUI_TBL_COL(FILE_TBL_MAX)];
      gui.tbl.hdr.begin(ctx, &tbl, tbl_lay, lst->tbl.state);
      for loop(i, tbl.cnt) {
        assert(i < cntof(file_tbl_def));
        struct str title = file_tbl_def[i].title;
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_lay, lst->tbl.state, title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

#if 0
      /* sorting */
      if (tbl.resort && lst->elms) {
        assert(tbl.sort.col < cntof(file_tbl_def));
        assert(tbl.sort.order < cntof(file_tbl_def[tbl.sort.col].sort));
        dyn_sort(lst->elms, file_tbl_def[tbl.sort.col].sort[tbl.sort.order]);
        lst->tbl.sort = tbl.sort;
      }
#endif

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, lst->page.cnt);
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for gui_tbl_lst_loop(i,gui,&tbl) {
        struct gui_panel elm = {0};
        int is_sel = (lst->sel_idx == i);
        struct file_elm *e = lst->page.elms + lst->page.cur * FILE_LIST_ELM_CNT + i;
        ui_file_view_tbl_elm(ctx, &tbl, tbl_lay, &elm, e, is_sel);

        /* input handling */
        struct gui_input in = {0};
        gui.pan.input(&in, ctx, &elm, GUI_BTN_LEFT);
        if (in.mouse.btn.left.doubled) {
          chdir = 1, dir = i;
        }
      }
      gui.tbl.lst.end(ctx, &tbl);
      if (tbl.lst.sel.mod) {
        /* selection handling */
        struct file_elm *elm = lst->page.elms + tbl.lst.sel.idx;
        elm->isvalid = !elm->isdir;
        lst->sel_idx = tbl.lst.sel.idx;
      }
    }
    gui.tbl.end(ctx, &tbl, pan, lst->off);
  }
  gui.pan.end(ctx, pan, parent);

  if (chdir) {
    struct file_elm *fi = lst->page.elms + dir;
    if (fi->isdir) {
      char buf[MAX_FILE_PATH];
      struct str file_path = str_fmtsn(buf, cntof(buf), "%.*s/%.*s",
          strf(lst->nav_path), strf(fi->name));

      lst->fltr = str_nil;
      file_view_tree_open(fs, &fs->tree, ctx->sys, file_path);
      file_view_lst_cd(fs, &fs->lst, ctx->sys, file_path);
      ctx->lst_state.cur_idx = -1;
    }
  }
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

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_panel fnd = {.box = pan->box};
    fnd.box.y = gui.bnd.min_ext(pan->box.y.min, ctx->cfg.item);
    ui_file_lst_view_fnd(fs, lst, ctx, &fnd, pan);

    int gap = ctx->cfg.pan_gap[0];
    struct gui_panel tbl = {.box = pan->box};
    tbl.box.y = gui.bnd.min_max(fnd.box.y.max + gap, pan->box.y.max);
    ui_file_view_tbl(fs, lst, ctx, &tbl, pan);
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_file_view_page(struct file_view *fs, struct file_list_view *lst,
                  struct gui_ctx *ctx, struct gui_panel *pan,
                  struct gui_panel *parent) {
  assert(fs);
  assert(lst);
  assert(app);
  assert(ctx);
  assert(pan);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tab_ctl tab = {.box = pan->box};
    tab.hdr_pos = GUI_TAB_HDR_BOT;
    gui.tab.begin(ctx, &tab, pan, 1, 0);
    {
      confine gui_disable_on_scope(&gui, ctx, lst->page.idx <= 0) {
        struct gui_btn nxt = {.box = tab.hdr};
        nxt.box.x = gui.bnd.max_ext(tab.hdr.x.max, ctx->cfg.scrl);
        if (gui__scrl_btn(ctx, &nxt, &tab.pan, GUI_EAST)) {
          assert(lst->page.idx >= 0);
          /* query next page  */
          struct file_view_lst_qry qry = {0};
          qry.cmp = file_view_lst_elm_cmp_name_asc;
          qry.fullpath = lst->nav_path;
          qry.page = lst->page.idx - 1;
          qry.fltr = lst->fltr;
          file_view_lst_qry(lst, ctx->sys, ctx->sys->mem.tmp, &qry);
        }
        tab.hdr.x = gui_min_max(tab.hdr.x.min, nxt.pan.box.x.min);
      }
      struct gui_tab_ctl_hdr hdr = {.box = tab.hdr};
      gui.tab.hdr.begin(ctx, &tab, &hdr);
      {
        char buf[128];
        struct str ospath = str_fmtsn(buf, cntof(buf), "Page %d of %d", lst->page.idx + 1, lst->page_cnt);
        gui.tab.hdr.slot.txt(ctx, &tab, &hdr, ospath);
        gui.tooltip(ctx, &tab.pan, ospath);
      }
      gui.tab.hdr.end(ctx, &tab, &hdr);

      confine gui_disable_on_scope(&gui, ctx, lst->page_cnt <= lst->page.idx+1) {
        struct gui_btn prv = {.box = hdr.pan.box};
        prv.box.x = gui.bnd.max_ext(tab.off, ctx->cfg.scrl);
        if (gui__scrl_btn(ctx, &prv, &tab.pan, GUI_WEST)) {
          assert(lst->page.idx < pst->page_cnt);
          /* query previous page  */
          struct file_view_lst_qry qry = {0};
          qry.cmp = file_view_lst_elm_cmp_name_asc;
          qry.fullpath = lst->nav_path;
          qry.fltr = lst->fltr;
          qry.page = lst->page.idx + 1;
          file_view_lst_qry(lst, ctx->sys, ctx->sys->mem.tmp, &qry);
        }
      }
      struct gui_panel bdy = {.box = tab.bdy};
      ui_file_sel_view(fs, lst, ctx, &bdy, &tab.pan);
    }
    gui.tab.end(ctx, &tab, pan);
  }
  gui.pan.end(ctx, pan, parent);
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
    static const unsigned col = col_rgb_hex(0xeecd4a);
    confine gui_cfg_pushu_scope(&gui, stk, &ctx->cfg.col[GUI_COL_ICO], col) {
      struct gui_panel lbl = {.box = node->box};
      struct str txt = path_file(n->fullpath);
      enum res_ico_id ico = node->open ? RES_ICO_FOLDER_OPEN : RES_ICO_FOLDER;
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
  for dyn_loop(i, tree->lst) {
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
  assert(ctx);
  assert(pan);
  assert(tree);
  assert(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    /* tree list */
    struct gui_lst_cfg cfg = {0};
    gui.lst.cfg(&cfg, dyn_cnt(tree->lst), tree->off[1]);
    cfg.sel.src = GUI_LST_SEL_SRC_EXT;

    struct gui_lst_reg reg = {.box = pan->box};
    gui.lst.reg.begin(ctx, &reg, pan, &cfg, tree->off);
    for gui_lst_reg_loop(i,gui,&reg) {
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
      fs->lst.fltr = str_nil;
      file_view_lst_cd(fs, &fs->lst, ctx->sys, n->fullpath);
    }
    if (reg.lst.ctl.has_focus) {
      /* key handling */
      ui_file_view_tree_key(fs, tree, ctx, &reg.lst);
    }
  }
  gui.pan.end(ctx, pan, parent);
}
static void
ui_file_sel_split(struct file_view *fs, struct gui_ctx *ctx,
                  struct gui_panel *pan, struct gui_panel *parent) {
  assert(ctx);
  assert(pan);
  assert(parent);
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
      ui_file_view_page(fs, &fs->lst, ctx, &tbl, &spt.pan);
    }
    gui.splt.end(ctx, &spt, pan);
  }
  gui.pan.end(ctx, pan, parent);
}
static struct str
ui_file_sel(char *filepath, int n, struct file_view *fs, struct gui_ctx *ctx,
            struct gui_panel *pan, struct gui_panel *parent) {

  assert(ctx);
  assert(pan);
  assert(parent);
  assert(filepath);

  struct str ret = str_inv;
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
    int dis = fs->lst.sel_idx < 0 || fs->lst.sel_idx >= fs->lst.page.cnt;
    if (!dis) {
      struct file_elm *elm = &fs->lst.page.elms[fs->lst.sel_idx];
      dis = elm->isdir || !elm->isvalid;
    }
    confine gui_disable_on_scope(&gui, ctx, dis) {
      if (gui.btn.ico_txt(ctx, &open, pan, strv("Open"), RES_ICO_IMPORT, -1)) {
        struct file_elm *elm = &fs->lst.page.elms[fs->lst.sel_idx];
        ret = str_fmtsn(filepath, n, "%.*s/%.*s", strf(fs->lst.nav_path), strf(elm->name));
      }
      gui.tooltip(ctx, &open.pan, strv("Open SQLite Database"));
    }
  }
  gui.pan.end(ctx, pan, parent);
  return ret;
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
static const struct pck_api pck__api = {
  .init = file_view_init,
  .update = file_view_update,
  .shutdown = file_view_free,
  .ui = ui_file_sel,
};
static void
pck_api(void *export, void *import) {
  unused(import);
  struct pck_api *exp = cast(struct pck_api*, export);
  *exp = pck__api;
}

