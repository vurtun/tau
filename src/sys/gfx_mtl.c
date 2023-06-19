#import <MetalKit/MetalKit.h>

#define GFX_MTL_BUF_DEPTH 2

struct gfx_tex {
  int act;
  id<MTLTexture> hdl;
  int w, h;
};
struct gfx_mtl {
  id<MTLDevice> dev;
  id<MTLLibrary> lib;
  id<MTLCommandQueue> cmd_que;
  id<MTLRenderPipelineState> pipe_state;
  id<MTLBuffer> buf[GFX_MTL_BUF_DEPTH];
  dispatch_semaphore_t sem;
  int cur_buf;
  vector_uint2 viewportSize;
  int tex_cnt;
  struct gfx_tex tex[GFX_TEX_MAX];
};

/* ---------------------------------------------------------------------------
 *                              Buffer2D
 * ---------------------------------------------------------------------------
 */
#include "gfx_com.h"

static const int gfx_box_seq[] = {0,1,3,3,2,0};
#define gfx__mtl_resv(b,v,i) ((b)->vbytes += szof(v), (b)->icnt += (i))
#define gfx__mtl_idx(o,p,c) ((castu(o)&0x0fffffff)|(castu(c) << 24u)|(castu(p) << 26u))
#define gfx__mtl_elms(b,o,p)\
  fori_arrv(i, gfx_box_seq)\
    (buf)->idx[buf->icnt+i] = gfx__mtl_idx(o, p, gfx_box_seq[i]);

static unsigned
gfx_mtl_d2d_clip(struct gfx_buf2d *buf, int l, int t, int r, int b) {
  unsigned off = castu(buf->vbytes);
  struct gfx_clip *c = recast(struct gfx_clip*, buf->vtx + buf->vbytes);
  *c = (struct gfx_clip){.l = casts(l), .t = casts(t), .r = casts(r), .b = casts(b)};
  gfx__mtl_resv(buf, *c, 0);
  return off;
}
static void
gfx_mtl_d2d_box(struct gfx_buf2d *buf, int x0, int y0, int x1, int y1,
                 unsigned col, unsigned clp) {
  struct gfx_box *b = recast(struct gfx_box*, buf->vtx + buf->vbytes);
  *b = (struct gfx_box){.l = casts(x0), .t = casts(y0), .r = casts(x1),
    .b = casts(y1), .clip = clp, .col = col};
  gfx__mtl_elms(buf->idx, buf->vbytes, GFX_PRIM_BOX);
  gfx__mtl_resv(buf, *b, cntof(gfx_box_seq));
}
static void
gfx_mtl_d2d_ln(struct gfx_buf2d *buf, int x0, int y0, int x1, int y1,
               int thickness, unsigned col, unsigned clp) {
  struct gfx_ln *c = recast(struct gfx_ln*, buf->vtx + buf->vbytes);
  *c = (struct gfx_ln){.x0 = casts(x0), .y0 = casts(y0), .x1 = casts(x1),
    .y1 = casts(y1), .clip = clp, .col = col, .thickness = thickness};
  gfx__mtl_elms(buf->idx, buf->vbytes, GFX_PRIM_LN);
  gfx__mtl_resv(buf, *c, cntof(gfx_box_seq));
}
static void
gfx_mtl_d2d_circle(struct gfx_buf2d *buf, int x, int y, int r,
                   unsigned col, unsigned clp) {
  int x0 = x - r, y0 = y - r, x1 = x + r, y1 = y + r;
  struct gfx_cir *c = recast(struct gfx_cir*, buf->vtx + buf->vbytes);
  *c = (struct gfx_cir){.x0 = casts(x0), .y0 = casts(y0), .x1 = casts(x1),
    .y1 = casts(y1), .clip = clp, .col = col};
  gfx__mtl_elms(buf->idx, buf->vbytes, GFX_PRIM_CIR);
  gfx__mtl_resv(buf, *c, cntof(gfx_box_seq));
}
static void
gfx_mtl_d2d_tri(struct gfx_buf2d *buf, int x0, int y0, int x1, int y1,
                int x2, int y2, unsigned col, unsigned clp) {
  struct gfx_tri *t = recast(struct gfx_tri*, buf->vtx + buf->vbytes);
  *t = (struct gfx_tri){.x0 = casts(x0), .y0 = casts(y0), .x1 = casts(x1),
    .y1 = casts(y1), .x2 = casts(x2), .y2 = casts(y2), .clip = clp, .col = col};
  buf->idx[buf->icnt+0] = gfx__mtl_idx(buf->vbytes, GFX_PRIM_TRI, 0);
  buf->idx[buf->icnt+1] = gfx__mtl_idx(buf->vbytes, GFX_PRIM_TRI, 1);
  buf->idx[buf->icnt+2] = gfx__mtl_idx(buf->vbytes, GFX_PRIM_TRI, 2);
  gfx__mtl_resv(buf, *t, 3);
}
static void
gfx_mtl_d2d_img(struct gfx_buf2d *buf, int x0, int y0, int x1, int y1,
                int u, int v, unsigned col, unsigned clp) {
  struct gfx_img *d = recast(struct gfx_img*, buf->vtx + buf->vbytes);
  *d = (struct gfx_img){.l = casts(x0), .t = casts(y0), .r = casts(x1),
    .b = casts(y1), .u = castus(u), .v = castus(v), .clip = clp, .col = col};
  gfx__mtl_elms(buf->idx, buf->vbytes, GFX_PRIM_IMG);
  gfx__mtl_resv(buf, *d, cntof(gfx_box_seq));
}

/* ---------------------------------------------------------------------------
 *                                  Texture
 * ---------------------------------------------------------------------------
 */
static const MTLPixelFormat gfx__mtl_pix_fmt[] = {
  [GFX_PIX_FMT_R8]        = MTLPixelFormatR8Unorm,
  [GFX_PIX_FMT_R8G8B8A8]  = MTLPixelFormatRGBA8Unorm,
};
static const int gfx__mtl_pix_fmt_bytes[] = {
  [GFX_PIX_FMT_R8]        = 1,
  [GFX_PIX_FMT_R8G8B8A8]  = 4,
};
compiler_assert(cntof(gfx__mtl_pix_fmt) == GFX_PIX_FMT_TYPE_CNT);
compiler_assert(cntof(gfx__mtl_pix_fmt_bytes) == GFX_PIX_FMT_TYPE_CNT);

static int
gfx_mtl_tex_load(struct sys *s, enum gfx_pix_fmt_type type, void *data, int w, int h) {
  assert(s);
  assert(data);

  int i = 0;
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, s->ren);
  for (i = 0; i < GFX_TEX_MAX; ++i) {
    if (!mtl->tex[i].act) {
      break;
    }
  }
  if (i >= GFX_TEX_MAX) {
    return GFX_TEX_MAX;
  }
  int bpr = gfx__mtl_pix_fmt_bytes[type];
  MTLPixelFormat fmt = gfx__mtl_pix_fmt[type];
  MTLTextureDescriptor *tex_desc = [MTLTextureDescriptor
    texture2DDescriptorWithPixelFormat:fmt
    width:(NSUInteger)w height:(NSUInteger)h mipmapped:false];
  tex_desc.usage = MTLTextureUsageShaderRead;

  id<MTLTexture> tex = [mtl->dev newTextureWithDescriptor:tex_desc];
  MTLRegion region = MTLRegionMake2D(0, 0, (NSUInteger)w, (NSUInteger)h);
  [tex replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:(NSUInteger)(bpr*w)];

  struct gfx_tex *img = mtl->tex + i;
  img->w = w, img->h = h;
  img->act = 1;
  img->hdl = tex;
  return i;
}
static void
gfx_mtl_tex_siz(int *siz, struct sys *s, int id) {
  assert(s);
  assert(siz);
  assert(id >= 0 && id < GFX_TEX_MAX);

  struct gfx_mtl *mtl = cast(struct gfx_mtl*, s->ren);
  assert(mtl->tex[id].act);
  struct gfx_tex *tex = mtl->tex + id;

  siz[0] = tex->w;
  siz[1] = tex->h;
}
static void
gfx_mtl_tex_del(struct sys *s, int hdl) {
  assert(hdl >= 0 && hdl < GFX_TEX_MAX);
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, s->ren);

  assert(mtl->tex[hdl].act);
  struct gfx_tex *tex = mtl->tex + hdl;
  mset(tex, 0, szof(*tex));
}

/* ---------------------------------------------------------------------------
 *                                  System
 * ---------------------------------------------------------------------------
 */
static int
gfx_mtl_init(struct sys *s, void *view_ptr) {
  MTKView *view = (__bridge MTKView*)view_ptr;
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, s->ren);
#ifndef NDEBUG
  /* enable metal validaton layer */
  setenv("MTL_SHADER_VALIDATION", "1", 1);
  setenv("MTL_SHADER_VALIDATION_GLOBAL_MEMORY", "1", 1);
  setenv("MTL_SHADER_VALIDATION_THREADGROUP_MEMORY", "1", 1);
  setenv("MTL_SHADER_VALIDATION_TEXTURE_USAGE", "1", 1);
  setenv("METAL_SHADER_LOG_LEVEL", "3", 1);
  setenv("METAL_SHADER_DIAGNOSTICS", "1", 1);
  setenv("METAL_DEVICE_WRAPPER_TYPE", "1", 1);
  setenv("METAL_ERROR_MODE", "5", 1);
  setenv("METAL_DEBUG_ERROR_MODE", "5", 1);
#endif
  NSError *err = 0x0;
  mtl->dev = MTLCreateSystemDefaultDevice();
  if (!mtl->dev) {
    NSLog(@"Metal is not supported on this device");
    return -1;
  }
  mtl->sem = dispatch_semaphore_create(GFX_MTL_BUF_DEPTH);
  mtl->cmd_que = [mtl->dev newCommandQueue];
  mtl->lib = [mtl->dev newLibraryWithFile: @"gfx.metallib" error:&err];
  //mtl->lib = [mtl->dev newDefaultLibrary];
  if (!mtl->lib) {
      NSLog(@"Failed to load library");
      exit(0);
  }
  id<MTLFunction> vshdr_f = [mtl->lib newFunctionWithName:@"ren_vtx"];
  if (!vshdr_f) {
    NSLog(@"Unable to get vertFunc!\n");
    return -1;
  }
  id<MTLFunction> fshdr_f = [mtl->lib newFunctionWithName:@"ren_frag"];
  if (!fshdr_f) {
    NSLog(@"Unable to get fragFunc!\n");
    return -1;
  }
  MTLRenderPipelineDescriptor *pld = [[MTLRenderPipelineDescriptor alloc] init];
  pld.label = @"tau_pipeline";
  pld.vertexFunction = vshdr_f;
  pld.fragmentFunction = fshdr_f;
  pld.colorAttachments[0].pixelFormat = view.colorPixelFormat;
  pld.colorAttachments[0].blendingEnabled = YES;
  pld.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
  pld.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
  pld.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
  pld.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
  pld.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
  pld.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
  pld.depthAttachmentPixelFormat = MTLPixelFormatInvalid;

  mtl->pipe_state = [mtl->dev newRenderPipelineStateWithDescriptor:pld error:&err];
  if (!mtl->pipe_state) {
    NSLog(@"Failed to created pipeline state, error %@", err);
    return -1;
  }
  fori_arrv(i, mtl->buf) {
    mtl->buf[i] = [mtl->dev newBufferWithLength:KB(256) options:MTLResourceStorageModeShared];
  }
  return 0;
}
static void
gfx_mtl_shutdown(struct sys *s) {
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, s->ren);
  unused(mtl);
}
static void
gfx_mtl_begin(struct sys *s, int w, int h) {
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, s->ren);
  mtl->viewportSize.x = castu(w);
  mtl->viewportSize.y = castu(h);

  s->gfx.buf2d.vtx = 0;
  s->gfx.buf2d.vbytes = 0;
  s->gfx.buf2d.idx = 0;
  s->gfx.buf2d.icnt = 0;
}
static void
gfx_mtl_end(struct sys *s, void *view_ptr) {
  MTKView *view = (__bridge MTKView*)view_ptr;
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, s->ren);
  if (!s->gfx.buf2d.icnt || !s->gfx.buf2d.vbytes) {
    return;
  }
  dispatch_semaphore_wait(mtl->sem, DISPATCH_TIME_FOREVER);
  mtl->cur_buf = (mtl->cur_buf + 1) % GFX_MTL_BUF_DEPTH;

  float c[4]; col_flt_paq(c, s->gfx.clear_color);
  view.clearColor = MTLClearColorMake(c[0], c[1], c[2], 1.0);

  int siz = s->gfx.buf2d.vbytes + s->gfx.buf2d.icnt * szof(int);
  if (mtl->buf[mtl->cur_buf].length + 16 < (NSUInteger)siz) {
    mtl->buf[mtl->cur_buf] = [mtl->dev newBufferWithLength:(NSUInteger)npow2(siz)
      options:MTLResourceStorageModeShared];
  }
  int ibytes = s->gfx.buf2d.icnt * szof(unsigned);
  NSUInteger vtx_off = cast(NSUInteger, align_up(ibytes, 16));
  void *idx_buf = mtl->buf[mtl->cur_buf].contents;
  void *vtx_buf = cast(char*, mtl->buf[mtl->cur_buf].contents) + vtx_off;

  printf("idx: %d vtx: %d\n", ibytes, s->gfx.buf2d.vbytes);

  mcpy(idx_buf, s->gfx.buf2d.idx, ibytes);
  mcpy(vtx_buf, s->gfx.buf2d.vtx, s->gfx.buf2d.vbytes);

  id<MTLCommandBuffer> cmd_buf = [mtl->cmd_que commandBuffer];
  cmd_buf.label = @"tau_cmd_buf";

  __block dispatch_semaphore_t blk_sem = mtl->sem;
  [cmd_buf addCompletedHandler:^(id<MTLCommandBuffer> buf) {
    unused(buf);
    dispatch_semaphore_signal(blk_sem);
  }];

  id<MTLTexture> tex = mtl->tex[s->gfx.d2d.tex].hdl;
  struct gfx_uniform uni = {0};
  uni.tex_siz.x = castu([tex width]);
  uni.tex_siz.y = castu([tex height]);
  uni.viewport = mtl->viewportSize;

  MTLRenderPassDescriptor* rpd = view.currentRenderPassDescriptor;
  if (rpd != nil) {
    id<MTLRenderCommandEncoder> enc = [cmd_buf renderCommandEncoderWithDescriptor:rpd];
    enc.label = @"tau_command_encoder";
    [enc setViewport:(MTLViewport){0.0, 0.0, mtl->viewportSize.x, mtl->viewportSize.y, 0.0, 1.0}];
    [enc setRenderPipelineState:mtl->pipe_state];
    [enc setVertexBuffer:mtl->buf[mtl->cur_buf] offset:0 atIndex:0];
    [enc setVertexBuffer:mtl->buf[mtl->cur_buf] offset:vtx_off atIndex:1];
    [enc setVertexBytes:&uni length:sizeof(uni) atIndex:2];
    [enc setFragmentTexture:tex atIndex:0];
    [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:(NSUInteger)s->gfx.buf2d.icnt];
    [enc endEncoding];
    [cmd_buf presentDrawable:view.currentDrawable];
  }
  [cmd_buf commit];
}
static void
gfx_mtl_resize(struct sys *s, int w, int h) {
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, s->ren);
  mtl->viewportSize.x = castu(w);
  mtl->viewportSize.y = castu(h);
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
static const struct gfx_api gfx_mtl_api = {
  .version = GFX_VERSION,
  .init = gfx_mtl_init,
  .begin = gfx_mtl_begin,
  .end = gfx_mtl_end,
  .shutdown = gfx_mtl_shutdown,
  .resize = gfx_mtl_resize,
  .d2d = {
    .cost = {
      .clip   = {.vbytes  = szof(struct gfx_clip),  .icnt = 0},
      .box    = {.vbytes  = szof(struct gfx_box),   .icnt = 6},
      .line   = {.vbytes  = szof(struct gfx_ln),    .icnt = 6},
      .tri    = {.vbytes  = szof(struct gfx_tri),   .icnt = 3},
      .circle = {.vbytes  = szof(struct gfx_cir),   .icnt = 6},
      .img    = {.vbytes  = szof(struct gfx_img),   .icnt = 6},
    },
    .clip = gfx_mtl_d2d_clip,
    .box = gfx_mtl_d2d_box,
    .ln = gfx_mtl_d2d_ln,
    .circle = gfx_mtl_d2d_circle,
    .tri = gfx_mtl_d2d_tri,
    .img = gfx_mtl_d2d_img,
  },
  .tex = {
    .load = gfx_mtl_tex_load,
    .info = gfx_mtl_tex_siz,
    .del = gfx_mtl_tex_del,
  },
};
static void
gfx_api(void *export, void *import) {
  unused(import);
  struct gfx_api *api = (struct gfx_api*)export;
  *api = gfx_mtl_api;
}

