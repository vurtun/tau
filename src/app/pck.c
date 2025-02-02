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
// clang-format on

/* -----------------------------------------------------------------------------
 *                                  VIEW
 * -----------------------------------------------------------------------------
 */
static int no_sanitize_int
file_type(struct str ext) {
  unsigned hash = 0;
  unsigned idx = 0;
  switch (str_len(ext)) {
  case 0: break;
  case 1: {
    hash = castu(str_at(ext,0));
    unsigned key = (hash * 0x3d7a774eU) >> 29U;
    idx = 5U + castu((0x20465731ULL >> (unsigned long long)(key*4)) & 0xfU);
  } break;
  case 2: {
    hash = twocc(str_beg(ext));
    unsigned key = (hash * 0x7172a30bU) >> 28U;
    idx = 13U + castu(0xa6314728ULL >> (unsigned long long)(key*4)) & 0xfU;
  } break;
  case 3: {
    static const unsigned char ltbl[256] = {72, 0, 0, 0, 0, 0, 14, 78, 0, 0, 0, 0,
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
    hash = threecc(str_beg(ext));
    idx = ltbl[(hash * 0xddc8d774U) >> 24U];
  } break;
  default: {
    hash = fourcc(str_beg(ext));
    unsigned key = (hash * 0x76e2bba) >> 29U;
    idx = 95U + castu((0x5142036ULL >> (unsigned long long)(key*4)) & 0xfU);
  } break; }
  assert(idx < cntof(file_defs));
  idx = min(idx, cntof(file_defs) -1);
  return (file_defs[idx].id == hash) ? casti(idx) : 0;
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
file_view_lst_elm_cmp_name_asc(const void *aptr, const void *bptr) {
  requires(aptr);
  requires(bptr);

  const struct file_elm *elm_a = (const struct file_elm*)aptr;
  const struct file_elm *elm_b = (const struct file_elm*)bptr;
  int eql = elm_b->isdir - elm_a->isdir;
  if (eql != 0) {
    return eql;
  }
  return str_cmp(elm_a->name, elm_b->name);
}
static int
file_view_lst_elm_cmp_name_dec(const void *aptr, const void *bptr) {
  requires(aptr);
  requires(bptr);

  const struct file_elm *elm_a = (const struct file_elm*)aptr;
  const struct file_elm *elm_b = (const struct file_elm*)bptr;
  int eql = elm_a->isdir - elm_b->isdir;
  if (eql != 0) {
    return eql;
  }
  return str_cmp(elm_b->name, elm_b->name);
}
/*@
  requires aptr != NULL;
  requires bptr != NULL;
  assigns *aptr, *bptr;
  ensures memcmp((char*)aptr, (char*)&old_a) != 0;
  ensures memcmp((char*)bptr, (char*)&old_b) != 0;
  ensures memcmp((char*)bptr, (char*)&old_a) == 0;
  ensures memcmp((char*)aptr, (char*)&old_b) == 0;
*/
static void
file_view_lst_elm_swp(void *aptr, void *bptr) {
  requires(aptr);
  requires(bptr);

  struct file_elm *elm_a = (struct file_elm*)aptr;
  struct file_elm *elm_b = (struct file_elm*)bptr;

  struct file_elm old_a = *elm_a;
  struct file_elm old_b = *elm_b;

  *elm_a = old_b;
  *elm_b = old_a;

  ensures(memcmp(elm_a, &old_a, sizeof(old_a)));
  ensures(memcmp(elm_b, &old_b, sizeof(old_a)));
  ensures(!memcmp(elm_b, &old_a, sizeof(old_a)));
  ensures(!memcmp(elm_a, &old_b, sizeof(old_a)));
}
/*@
  requires sys != NULL;
  requires elm != NULL;
  requires str_is_val(name);
  requires str_is_val(path);

  assigns elm->sys_type, elm->file_type, elm->name, elm->size, elm->perm, elm->mtime;

  ensures elm->sys_type >= 0;  // Ensures non-negative system type
  ensures (elm->sys_type == SYS_FILE_DEF || elm->sys_type == SYS_FILE_LNK || elm->sys_type == SYS_FILE_DIR);
  if (old(sys->file.info(sys, &info, ospath))) {
    ensures elm->size == info.siz;
    ensures elm->perm == info.perm;
    ensures elm->mtime == info.mtime;
  } else {
    ensures elm->size == 0;
    ensures elm->perm == 0;
    ensures elm->mtime == 0;
  }
  ensures str_cmp(elm->name, name) == 0;
  if (elm->sys_type == SYS_FILE_DEF || elm->sys_type == SYS_FILE_LNK) {
    ensures elm->file_type == castu(file_type(path_ext(ospath)));
  } else if (elm->sys_type == SYS_FILE_DIR) {
    ensures elm->file_type == 1;
  } else {
    ensures elm->file_type == 0;  // Default for unknown types
  }
*/
static void
file_view_lst_elm_init(struct file_elm *elm, struct sys *sys,
                       struct str path, struct str name) {
  requires(sys);
  requires(elm);

  requires(str_is_val(name));
  requires(str_is_val(path));

  char buf[MAX_FILE_PATH];
  struct str ospath = str_fmtsn(buf, cntof(buf), "%.*s/%.*s", strf(path), strf(name));
  mset(elm,0,szof(elm[0]));

  struct sys_file_info info = {0};
  if (!sys->file.info(sys, &info, ospath)) {
    elm->sys_type = SYS_FILE_DEF;
    elm->file_type = 0;
    elm->name = name;
    elm->perm = 0;
    return;
  }
  elm->sys_type = info.type;
  if (elm->sys_type == SYS_FILE_DEF ||
      elm->sys_type == SYS_FILE_LNK) {
    struct str ext = path_ext(ospath);
    elm->file_type = castu(file_type(ext));
  } else if (elm->sys_type == SYS_FILE_DIR) {
    elm->file_type = 1;
    elm->isdir = 1;
  }
  elm->name = name;
  elm->size = info.siz;
  elm->perm = info.perm;
  elm->mtime = info.mtime;
  elm->sys_type = info.type;
}
/*@
  requires arr != NULL;
  requires cmp != NULL;
  assigns arr[0..FILE_LIST_ELM_BUF_CNT-1];
  ensures \forall integer i, j; 0 <= i < rhs && rhs <= j < FILE_LIST_ELM_BUF_CNT;
          cmp(&arr[i], &arr[j]) <= 0;
  ensures \forall integer i, j; 0 <= i < rhs && 0 <= j < rhs;
          cmp(&arr[i], &arr[j]) <= 0;
  ensures \forall integer i, j; rhs <= i < FILE_LIST_ELM_BUF_CNT && rhs <= j < FILE_LIST_ELM_BUF_CNT;
          cmp(&arr[i], &arr[j]) >= 0;
*/
static int
file_view_lst_partition(struct file_elm *arr, int(*cmp)(const void*, const void*)) {
  requires(arr);
  requires(cmp);

  int num = FILE_LIST_ELM_BUF_CNT;
  int piv_idx = FILE_LIST_ELM_CNT;

  struct file_elm piv = arr[piv_idx];
  arr[piv_idx] = arr[0];
  piv_idx = (cmp(&arr[num-1], &piv) < 0) ? num-1 : 0;
  arr[0] = arr[piv_idx];
  arr[piv_idx] = piv;

  int lhs = 0;
  int rhs = num-1;
  for (;;) {
    do {lhs++;} while (cmp(&arr[lhs], &piv) < 0);
    do {rhs--;} while (cmp(&piv, &arr[rhs]) < 0);
    if (lhs >= rhs) {break;}
    file_view_lst_elm_swp(&arr[lhs], &arr[rhs]);
  }
  rhs += (piv_idx != 0);
  file_view_lst_elm_swp(&arr[rhs], &arr[piv_idx]);
  return rhs;
}
/*@
  requires lst != NULL;
  requires 0 <= cur && cur <= cntof(lst->page.txt.buf);
  assigns lst->page.txt.cnt, lst->page.txt.buf[cur][0..FILE_LIST_STR_BUF_SIZ-1];
  ensures \result.len <= cap;
  ensures \result.len <= FILE_LIST_STR_BUF_SIZ;
  ensures lst->page.txt.cnt <= FILE_LIST_STR_BUF_SIZ;
*/
static struct str
file_view_lst_str(struct file_list_view *lst, struct str name, int cur) {
  requires(lst);
  requires(cur >= 0 && cur <= cntof(lst->page.txt.buf));

  int cap = cntof(lst->page.txt.buf[0]) - lst->page.txt.cnt;
  char *buf = lst->page.txt.buf[cur] + lst->page.txt.cnt;
  struct str ret = str_set(buf, cap, name);
  if (str_is_val(name)) {
    lst->page.txt.cnt += str_len(name);
  }
  ensures(ret.rng.cnt <= cap);
  ensures(ret.rng.cnt <= FILE_LIST_STR_BUF_SIZ);
  ensures(lst->page.txt.cnt <= FILE_LIST_STR_BUF_SIZ);
  return ret;
}
/*@
  requires sys != NULL;
  requires qry != NULL;
  requires lst != NULL;

  assigns lst->page.cnt, lst->page.total, lst->page.elms[*], lst->page.txt.cnt, lst->page.txt.cur, lst->page_cnt, lst->page.idx;

  ensures lst->page_cnt >= 0;
  ensures lst->page.idx == qry->page;
  ensures lst->page.total >= 0;
  ensures lst->page.idx < lst->page_cnt;
*/
static void
file_view_lst_qry(struct file_list_view *lst, struct sys *sys,
                  const struct file_view_lst_qry *qry) {
  requires(sys);
  requires(qry);
  requires(lst);

  lst->page.cnt = 0;
  lst->page.total = 0;

  /* setup pivot */
  char lbl[MAX_FILE_NAME];
  int pidx = (qry->page >= lst->page.idx) ? 0 : lst->page.cnt-1;
  struct file_elm piv = lst->page.elms[pidx];
  piv.name = str_set(lbl, cntof(lbl), piv.name);
  lst->page.cur = (qry->page < lst->page.idx);

  /* iterate directory */
  struct sys_dir_iter itr = {0};
  for sys_dir_lst_each(sys, &itr, qry->fullpath) {
    lst->page.total++;
    /* apply filter */
    struct file_elm elm = {0};
    file_view_lst_elm_init(&elm, sys, qry->fullpath, itr.name);
    if ((str_len(qry->fltr) && str_fnd(elm.name, qry->fltr) >= str_len(elm.name)) ||
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
      cntof(lst->page.elms) - ++lst->page.cnt : lst->page.cnt++;
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
  qsort(ptr, castsz(lst->page.cnt), szof(lst->page.elms[0]), qry->cmp);
  lst->page_cnt = (lst->page.total + FILE_LIST_ELM_CNT - 1) / FILE_LIST_ELM_CNT;
  lst->page.idx = qry->page;

  ensures(lst->page_cnt >= 0);
  ensures(lst->page.idx == qry->page);
  ensures(lst->page.total >= 0);
  ensures(lst->page.idx < lst->page_cnt);
}
/*@
  requires lst != NULL;
  assigns lst->off[*], lst->sel_idx, lst->page_cnt, lst->page.idx, lst->page.cur,
         lst->page.txt.cnt, lst->page.txt.cur, lst->page.txt.buf[*][*];

  ensures lst->page_cnt >= 0;
  ensures lst->page.idx <= lst->page_cnt;  // Ensures valid page index
  ensures lst->page.total >= 0;  // Total count (unchanged, can't be negative)
  ensures lst->page.cnt == 0;  // Ensures empty page
  ensures lst->page.txt.cnt == 0;  // Ensures empty text buffer
  ensures lst->page.txt.cur == 0;  // Ensures initial text buffer index
  ensures lst->page.txt.buf[0][0] == 0;  // Ensures null termination of first buffer
  ensures lst->page.txt.buf[1][0] == 0;  // Ensures null termination of second buffer
*/
static void
file_view_lst_clr(struct file_list_view *lst) {
  requires(lst);
  zero2(lst->off);
  lst->sel_idx = -1;

  lst->page_cnt = 0;
  lst->page.idx = 0;
  lst->page.cur = 0;

  lst->page.txt.cnt = 0;
  lst->page.txt.cur = 0;
  lst->page.txt.buf[0][0] = 0;
  lst->page.txt.buf[1][0] = 0;

  ensures(lst->page_cnt >= 0);
  ensures(lst->page.idx <= lst->page_cnt);
  ensures(lst->page.total >= 0);
  ensures(lst->page.cnt <= lst->page.total);
  ensures(lst->page.txt.cnt == 0);
  ensures(lst->page.txt.cur == 0);
  ensures(lst->page.txt.cnt <= FILE_LIST_STR_BUF_SIZ);
}
/*@
  requires fpk != NULL;
  requires sys != NULL;
  requires lst != NULL;
  requires str_is_val(fullpath);  // Ensures valid fullpath argument

  assigns lst->nav_path, fpk->lst[*], lst->page_cnt, lst->page.total, lst->page.idx, lst->page.cnt;

  ensures lst->page_cnt >= 0;
  ensures lst->page.total >= 0;
  ensures lst->page.idx < lst->page_cnt;  // Ensures valid page index after update
  ensures lst->page.cnt <= lst->page.total;

  // if branch
  if (str_is_val(lst->nav_path)) {
    ensures(fpk->lst.page_cnt >= 0);  // Inherited from callees
    ensures(fpk->lst.page.total >= 0);  // Inherited from callees
    ensures(fpk->lst.page.idx <= fpk->lst.page_cnt);  // Inherited from callees
    ensures(fpk->lst.page.cnt <= fpk->lst.page.total);  // Inherited from callees
  }
*/
static void
file_view_lst_cd(struct file_view *fpk, struct file_list_view *lst,
                 struct sys *sys, struct str fullpath) {
  requires(fpk);
  requires(sys);
  requires(lst);

  lst->nav_path = str_set(lst->nav_buf, cntof(lst->nav_buf), fullpath);
  if (str_is_val(lst->nav_path)) {
    file_view_lst_clr(&fpk->lst);

    struct file_view_lst_qry qry = {0};
    qry.fltr = lst->fltr;
    qry.fullpath = lst->nav_path;
    qry.cmp = file_view_lst_elm_cmp_name_asc;
    file_view_lst_qry(lst, sys, &qry);
  }
  ensures(lst->page_cnt >= 0);
  ensures(lst->page.total >= 0);
  ensures(lst->page.idx < lst->page_cnt);
  ensures(lst->page.cnt <= lst->page.total);
}
/*@
  requires ctx != NULL;
  requires fpk != NULL;
  requires sys != NULL;

  assigns fpk->lst.tbl.state[*], fpk->lst.page_cnt, fpk->lst.page.total, fpk->lst.page.idx, fpk->lst.page.cnt;

  ensures lst->page_cnt >= 0;
  ensures lst->page.total >= 0;
  ensures lst->page.idx < lst->page_cnt;  // Ensures valid page index after update
  ensures lst->page.cnt <= lst->page.total;
*/
static void
file_view_lst_init(struct file_view *fpk, struct sys *sys, struct gui_ctx *ctx) {
  requires(ctx);
  requires(fpk);
  requires(sys);

  /* setup list table */
  struct gui_split_lay_cfg tbl_cfg = {0};
  tbl_cfg.size = szof(struct file_tbl_col_def);
  tbl_cfg.off = offsetof(struct file_tbl_col_def, ui);
  tbl_cfg.slots = file_tbl_def;
  tbl_cfg.cnt = FILE_TBL_MAX;
  gui.tbl.lay(fpk->lst.tbl.state, ctx, &tbl_cfg);
  /* open home directory */
  file_view_lst_cd(fpk, &fpk->lst, sys, fpk->home);

  ensures(fpk->lst.page_cnt >= 0);
  ensures(fpk->lst.page.total >= 0);
  ensures(fpk->lst.page.idx < fpk->lst.page_cnt);
  ensures(fpk->lst.page.cnt <= fpk->lst.page.total);
}
/*@
  requires sys != NULL;
  requires ctx != NULL;
  requires fpk != NULL;

  assigns fpk->lst_rev, fpk->home, fpk->lst[*];

  if (str_is_inv(fpk->home)) {
    ensures \result == -ENAMETOOLONG;
  } else {
    ensures lst->page_cnt >= 0;
    ensures lst->page.total >= 0;
    ensures lst->page.idx < lst->page_cnt;
    ensures lst->page.cnt <= lst->page.total;
  }
  ensures \result == 0 || \result == -ENAMETOOLONG;
*/
static int
file_view_init(struct file_view *fpk, struct sys *sys, struct gui_ctx *ctx) {
  requires(sys);
  requires(ctx);
  requires(fpk);

  fpk->lst_rev = 1;
  fpk->home = str_set(fpk->home_path, cntof(fpk->home_path), str0(getenv("HOME")));
  if (str_is_inv(fpk->home)) {
    return -ENAMETOOLONG;
  }
  file_view_lst_init(fpk, sys, ctx);

  ensures(fpk->lst.page_cnt >= 0);
  ensures(fpk->lst.page.total >= 0);
  ensures(fpk->lst.page.idx < fpk->lst.page_cnt);
  ensures(fpk->lst.page.cnt <= fpk->lst.page.total);
  return 0;
}
static void
file_view_free(struct file_view *fpk, struct sys *sys) {
  requires(fpk);
  requires(sys);

  unused(sys);
  file_view_lst_clr(&fpk->lst);
}

/* -----------------------------------------------------------------------------
 *                                  GUI
 * -----------------------------------------------------------------------------
 */
static struct str
ui_edit_search(struct gui_ctx *ctx, struct gui_edit_box *edt,
               struct gui_panel *pan, struct gui_panel *parent,
               struct gui_txt_ed *ted, char *buf, int cap, struct str str) {

  requires(ted);
  requires(buf);
  requires(ctx);
  requires(edt);
  requires(pan);
  requires(parent);

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
    ted->buf = buf;
    ted->cap = cap;
    ted->str = str;

    edt->pan.focusable = 1;
    edt->pan.box.x = gui.bnd.min_max(ico.box.x.max, pan->box.x.max);
    edt->pan.box.x = gui.bnd.shrink(&edt->pan.box.x, pad[0]);
    edt->pan.box.y = gui.bnd.shrink(&pan->box.y, pad[1]);
    gui.edt.fld(ctx, edt, &edt->pan, pan, ted);
  }
  gui.pan.end(ctx, pan, parent);
  return ted->str;
}
static void
ui_file_lst_view_fnd(struct file_view *fpk, struct file_list_view *lst,
                     struct gui_ctx *ctx, struct gui_panel *pan,
                     struct gui_panel *parent) {
  requires(fpk);
  requires(lst);
  requires(ctx);
  requires(pan);
  requires(parent);

  struct gui_edit_box edt = {.box = pan->box};
  lst->fltr = ui_edit_search(ctx, &edt, pan, parent, &lst->fltr_ed,
                             lst->fltr_buf, cntof(lst->fltr_buf), lst->fltr);
  if (edt.mod) {
    struct file_view_lst_qry qry = {0};
    qry.cmp = file_view_lst_elm_cmp_name_asc;
    qry.fullpath = lst->nav_path;
    qry.page = lst->page.idx;
    qry.fltr = lst->fltr;
    file_view_lst_qry(lst, ctx->sys, &qry);
  }
}
static void
ui_file_lst_view_nav_bar(struct file_view *fpk, struct file_list_view *lst,
                         struct gui_ctx *ctx, struct gui_panel *pan,
                         struct gui_panel *parent) {
  requires(lst);
  requires(ctx);
  requires(pan);
  requires(parent);

  gui.pan.begin(ctx, pan, parent);
  {
    int gap = ctx->cfg.gap[0];
    struct gui_box lay = pan->box;
    struct gui_btn home = {.box = gui.cut.rhs(&lay, ctx->cfg.item, gap)};
    if (gui.btn.ico(ctx, &home, pan, RES_ICO_HOME)) {
      /* change the directory to home  */
      lst->fltr = str_nil;
      file_view_lst_cd(fpk, &fpk->lst, ctx->sys, fpk->home);
    }
    gui.tooltip(ctx, &home.pan, strv("Goto Home Directory"));

    struct gui_btn dup = {.box = gui.cut.rhs(&lay, ctx->cfg.item, gap)};
    if (gui.btn.ico(ctx, &dup, pan, RES_ICO_FOLDER)) {
      /* go up to parent directory  */
      struct str file_name = path_file(lst->nav_path);
      struct str file_path = strp(str_beg(lst->nav_path), str_beg(file_name));
      if (str_len(file_path) > str_len(fpk->home)) {
        lst->fltr = str_nil;
        struct str dir = strp(str_beg(lst->nav_path), str_beg(file_name));
        file_view_lst_cd(fpk, &fpk->lst, ctx->sys, dir);
      }
    }
    gui.tooltip(ctx, &dup.pan, strv("Move to Parent Directory"));

    /* navigation bar */
    struct gui_edit_box edt = {.box = lay};
    lst->nav_ed.buf = lst->nav_buf;
    lst->nav_ed.cap = cntof(lst->nav_buf);
    lst->nav_ed.str = lst->nav_path;
    lst->nav_path = gui.edt.txt(ctx, &edt, pan, &lst->nav_ed);
  }
  gui.pan.end(ctx, pan, parent);
}
static struct str
ui__file_view_tbl_elm_perm(char *mod, unsigned perm, int cnt) {
  assert(mod);
  mod[0] = (perm & SYS_FILE_PERM_USR_READ)  ? 'r' : '-';
  mod[1] = (perm & SYS_FILE_PERM_USR_WRITE) ? 'w' : '-';
  mod[2] = (perm & SYS_FILE_PERM_USR_EXEC)  ? 'x' : '-';

  mod[3] = (perm & SYS_FILE_PERM_GRP_READ)  ? 'r' : '-';
  mod[4] = (perm & SYS_FILE_PERM_GRP_WRITE) ? 'w' : '-';
  mod[5] = (perm & SYS_FILE_PERM_GRP_EXEC)  ? 'x' : '-';

  mod[6] = (perm & SYS_FILE_PERM_ALL_READ)  ? 'r' : '-';
  mod[7] = (perm & SYS_FILE_PERM_ALL_WRITE) ? 'w' : '-';
  mod[8] = (perm & SYS_FILE_PERM_ALL_EXEC)  ? 'x' : '-';
  return strn(mod,cnt);
}
static void
ui_file_view_tbl_elm(struct gui_ctx *ctx, struct gui_tbl *tbl,
                     const int *lay, struct gui_panel *pan,
                     const struct file_elm *elm, int is_sel) {
  requires(elm);
  requires(tbl);
  requires(lay);
  requires(pan);

  static const struct gui_align algn = {GUI_HALIGN_RIGHT, GUI_VALIGN_MID};
  static const unsigned dir_col = col_rgb_hex(0xeecd4a);
  unsigned long long elm_id = str_hash(elm->name);
  gui.tbl.lst.elm.begin(ctx, tbl, pan, gui_id64(elm_id), is_sel);
  {
    struct gui_cfg_stk stk[1] = {0};
    confine gui_cfg_pushu_on_scope(&gui,stk,&ctx->cfg.col[GUI_COL_ICO], dir_col, elm->isdir) {
      char buf[9u];
      struct str perm = ui__file_view_tbl_elm_perm(buf, elm->perm, cntof(buf));
      struct tm *mod_time = localtime(&elm->mtime);
      const struct file_def *fd = &file_defs[elm->file_type];
      enum res_ico_id ico = file_icon(elm->file_type);

      /* columns */
      gui.tbl.lst.elm.col.txt_ico(ctx, tbl, lay, pan, elm->name, ico);
      gui.tbl.lst.elm.col.txt(ctx, tbl, lay, pan, fd->name, 0);
      gui.tbl.lst.elm.col.txtf(ctx, tbl, lay, pan, &algn, "%zu", elm->size);
      gui.tbl.lst.elm.col.txt(ctx, tbl, lay, pan, perm, 0);
      gui.tbl.lst.elm.col.tm(ctx, tbl, lay, pan, "%d/%m/%Y %H:%M:%S", mod_time);
    }
  }
  gui.tbl.lst.elm.end(ctx, tbl, pan);
  gui.tooltip(ctx, pan, elm->name);
}
static struct str
ui_file_view_tbl(char *filepath, int cnt, struct file_view *fpk,
                 struct file_list_view *lst, struct gui_ctx *ctx,
                 struct gui_panel *pan, struct gui_panel *parent) {
  requires(fpk);
  requires(lst);
  requires(ctx);
  requires(pan);
  requires(parent);

  int dir = 0;
  int chdir = 0;
  struct str ret = str_inv;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_tbl tbl = {.box = pan->box};
    gui.tbl.begin(ctx, &tbl, pan, lst->off, &lst->tbl.sort);
    {
      /* header */
      int tbl_lay[GUI_TBL_COL(FILE_TBL_MAX)];
      gui.tbl.hdr.begin(ctx, &tbl, arrv(tbl_lay), arrv(lst->tbl.state));
      for arr_loopv(i, file_tbl_def) {
        assert(i < cntof(file_tbl_def));
        struct str title = file_tbl_def[i].title;
        gui.tbl.hdr.slot.txt(ctx, &tbl, tbl_lay, lst->tbl.state, title);
      }
      gui.tbl.hdr.end(ctx, &tbl);

      /* list */
      struct gui_tbl_lst_cfg cfg = {0};
      gui.tbl.lst.cfg(ctx, &cfg, lst->page.cnt);
      cfg.sel.src = GUI_LST_SEL_SRC_EXT;

      gui.tbl.lst.begin(ctx, &tbl, &cfg);
      for gui_tbl_lst_loopn(i,_,gui,&tbl,FILE_LIST_ELM_CNT) {
        int is_sel = (lst->sel_idx == i);
        int idx = lst->page.cur * FILE_LIST_ELM_CNT + i;
        assert(idx < cntof(lst->page.elms));

        struct gui_panel item = {0};
        struct file_elm *elm = &lst->page.elms[idx];
        ui_file_view_tbl_elm(ctx, &tbl, tbl_lay, &item, elm, is_sel);

        /* input handling */
        struct gui_input pin = {0};
        gui.pan.input(&pin, ctx, &item, GUI_BTN_LEFT);
        if (pin.mouse.btn.left.doubled) {
          chdir = 1;
          dir = i;
        }
      }
      gui.tbl.lst.end(ctx, &tbl);
      if (tbl.lst.sel.mod) {
        /* selection handling */
        assert(tbl.lst.sel.idx < cntof(lst->page.elms));
        struct file_elm *elm = lst->page.elms + tbl.lst.sel.idx;
        elm->isvalid = !elm->isdir;
        lst->sel_idx = tbl.lst.sel.idx;
      }
    }
    gui.tbl.end(ctx, &tbl, pan, lst->off);
  }
  gui.pan.end(ctx, pan, parent);

  if (chdir) {
    assert(dir < cntof(lst->page.elms));
    struct file_elm *elm = &lst->page.elms[dir];
    if (elm->isdir) {
      char buf[MAX_FILE_PATH];
      struct str file_path = str_fmtsn(buf, cntof(buf), "%.*s/%.*s", strf(lst->nav_path), strf(elm->name));

      lst->fltr = str_nil;
      file_view_lst_cd(fpk, &fpk->lst, ctx->sys, file_path);
      ctx->lst_state.cur_idx = -1;
    } else {
      ret = str_fmtsn(filepath, cnt, "%.*s/%.*s", strf(fpk->lst.nav_path), strf(elm->name));
    }
  }
  return ret;
}
static struct str
ui_file_sel_view(char *filepath, int cnt, struct file_view *fpk,
                 struct file_list_view *lst, struct gui_ctx *ctx,
                 struct gui_panel *pan, struct gui_panel *parent) {
  requires(fpk);
  requires(lst);
  requires(ctx);
  requires(pan);
  requires(parent);

  struct str ret = str_inv;
  gui.pan.begin(ctx, pan, parent);
  {
    struct gui_panel fnd = {.box = pan->box};
    fnd.box.y = gui.bnd.min_ext(pan->box.y.min, ctx->cfg.item);
    ui_file_lst_view_fnd(fpk, lst, ctx, &fnd, pan);

    int gap = ctx->cfg.pan_gap[0];
    struct gui_panel tbl = {.box = pan->box};
    tbl.box.y = gui.bnd.min_max(fnd.box.y.max + gap, pan->box.y.max);
    ret = ui_file_view_tbl(filepath, cnt, fpk, lst, ctx, &tbl, pan);
  }
  gui.pan.end(ctx, pan, parent);
  return ret;
}
static void
ui_file_view_page(struct file_list_view *lst, struct gui_ctx *ctx,
                  struct gui_tab_ctl *tab) {
  requires(lst);
  requires(ctx);
  requires(tab);

  confine gui_disable_on_scope(&gui, ctx, lst->page.idx <= 0) {
    struct gui_btn prv = {.box = tab->hdr};
    prv.box.x = gui.bnd.min_ext(tab->hdr.x.min, ctx->cfg.scrl);
    if (gui__scrl_btn(ctx, &prv, &tab->pan, GUI_WEST)) {
      assert(lst->page.idx >= 0);

      /* query previous page */
      struct file_view_lst_qry qry = {0};
      qry.cmp = file_view_lst_elm_cmp_name_asc;
      qry.fullpath = lst->nav_path;
      qry.page = lst->page.idx - 1;
      qry.fltr = lst->fltr;
      file_view_lst_qry(lst, ctx->sys, &qry);
    }
    tab->hdr.x = gui_min_max(prv.box.x.max, tab->hdr.x.max);
  }
  struct gui_tab_ctl_hdr hdr = {.box = tab->hdr};
  gui.tab.hdr.begin(ctx, tab, &hdr);
  {
    char buf[FILE_MAX_PAGE_BUF];
    struct str info = str_fmtsn(buf, cntof(buf), "Page %d of %d", lst->page.idx + 1, lst->page_cnt);
    confine gui_disable_on_scope(&gui, ctx, lst->page_cnt == 1) {
      struct gui_panel slot = {0};
      gui.tab.hdr.slot.txt(ctx, tab, &hdr, &slot, gui_id64(str_hash(info)), info);
      gui.tooltip(ctx, &slot, info);
    }
  }
  gui.tab.hdr.end(ctx, tab, &hdr);

  confine gui_disable_on_scope(&gui, ctx, lst->page_cnt <= lst->page.idx+1) {
    struct gui_btn nxt = {.box = hdr.pan.box};
    nxt.box.x = gui.bnd.min_ext(tab->off, ctx->cfg.scrl);
    if (gui__scrl_btn(ctx, &nxt, &tab->pan, GUI_EAST)) {
      assert(lst->page.idx < lst->page_cnt);

      /* query next page  */
      struct file_view_lst_qry qry = {0};
      qry.cmp = file_view_lst_elm_cmp_name_asc;
      qry.fullpath = lst->nav_path;
      qry.page = lst->page.idx + 1;
      qry.fltr = lst->fltr;
      file_view_lst_qry(lst, ctx->sys, &qry);
    }
    tab->off = nxt.box.x.max;
  }
}
static struct str
ui_file_sel(char *filepath, int cnt, struct file_view *fpk, struct gui_ctx *ctx,
            struct gui_panel *pan, struct gui_panel *parent) {

  requires(fpk);
  requires(ctx);
  requires(pan);
  requires(parent);
  requires(filepath);

  struct str ret = str_inv;
  gui.pan.begin(ctx, pan, parent);
  {
    /* navigation bar */
    int gap = ctx->cfg.pan_gap[1];
    struct gui_box lay = pan->box;
    struct gui_panel nav = {.box = gui.cut.top(&lay, ctx->cfg.item, gap)};
    ui_file_lst_view_nav_bar(fpk, &fpk->lst, ctx, &nav, pan);
    {
      /* file table */
      struct gui_tab_ctl tab = {.box = lay, .hdr_pos = GUI_TAB_HDR_BOT};
      gui.tab.begin(ctx, &tab, pan, 1, 0);
      {
        struct gui_panel bdy = {.box = tab.bdy};
        ui_file_view_page(&fpk->lst, ctx, &tab);
        ret = ui_file_sel_view(filepath, cnt, fpk, &fpk->lst, ctx, &bdy, &tab.pan);
      }
      gui.tab.end(ctx, &tab, pan);

      /* file selection */
      int dis = fpk->lst.sel_idx < 0 || fpk->lst.sel_idx >= fpk->lst.page.cnt;
      if (!dis) {
        struct file_elm *elm = &fpk->lst.page.elms[fpk->lst.sel_idx];
        dis = elm->isdir || !elm->isvalid;
      }
      confine gui_disable_on_scope(&gui, ctx, dis) {
        struct gui_btn open = {.box = tab.hdr};
        open.box.x = gui.bnd.min_max(tab.off + ctx->cfg.gap[0], tab.hdr.x.max);
        if (gui.btn.ico_txt(ctx, &open, pan, strv("Open"), RES_ICO_IMPORT, -1)) {
          struct file_elm *elm = &fpk->lst.page.elms[fpk->lst.sel_idx];
          ret = str_fmtsn(filepath, cnt, "%.*s/%.*s", strf(fpk->lst.nav_path), strf(elm->name));
        }
        gui.tooltip(ctx, &open.pan, strv("Open SQLite Database"));
      }
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
  .shutdown = file_view_free,
  .ui = ui_file_sel,
};
static void
pck_api(void *export, void *import) {
  unused(import);
  struct pck_api *exp = cast(struct pck_api*, export);
  *exp = pck__api;
}

