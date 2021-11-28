#define REN_VERSION 1

#define REN_TEX_MAX 1024

struct ren_cmd_buf;
struct ren_cmd_api {
  void (*col)(struct ren_cmd_buf*, unsigned col);
  void (*line_style)(struct ren_cmd_buf*, int thickness);
  void (*clip)(struct ren_cmd_buf*, int x, int y, int w, int h);
  void (*box)(struct ren_cmd_buf*, int x0, int y0, int x1, int y1);
  void (*hln)(struct ren_cmd_buf*, int y, int x0, int x1);
  void (*vln)(struct ren_cmd_buf*, int x, int y0, int y1);
  void (*sbox)(struct ren_cmd_buf*, int x0, int y0, int x1, int y1);
  void (*img)(struct ren_cmd_buf*, int dx, int dy, int sx, int sy,
              int w, int h, int img_id);
};
struct ren_cmd_que;
struct ren_queue_api {
  struct ren_cmd_que *dev;
  struct ren_cmd_buf*(*mk)(struct ren_cmd_que*);
};
struct ren_img_api {
  int (*mk)(void *ren, const unsigned int *mem, int w, int h);
  void (*del)(void *ren, int img_id);
  void (*siz)(int *siz, void *ren_hdl, int img_id);
};
struct ren_api {
  int version;
  struct ren_queue_api que;
  struct ren_cmd_api drw;
  struct ren_img_api tex;
};

static void ren_init(struct sys *sys);
static void ren_begin(struct sys *sys);
static void ren_end(struct sys *sys);

