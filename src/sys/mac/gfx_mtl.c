#import <MetalKit/MetalKit.h>

#define GFX_MTL_BUF_DEPTH 2

#include "gfx_shdr.h"

struct gfx_tex {
  int act;
  int w, h;
  id<MTLTexture> hdl;
};
struct gfx_mtl {
  struct sys *sys;

  id<MTLDevice> dev;
  id<MTLLibrary> lib;
  id<MTLCommandQueue> cmd_que;
  id<MTLRenderPipelineState> pipe_state;
  id<MTLFunction> fshdr_f;
  vector_uint2 viewportSize;

  int cur_buf;
  dispatch_semaphore_t sem;
  id<MTLBuffer> buf[GFX_MTL_BUF_DEPTH];

  int tex_cnt;
  struct tbl(GFX_MAX_TEX_CNT) tex_cache;
  id<MTLTexture> tex_buf[GFX_MAX_TEX_CNT];
  id<MTLBuffer> arg_buf[GFX_MTL_BUF_DEPTH];
  struct gfx_tex tex[GFX_TEX_MAX];
};

/* ---------------------------------------------------------------------------
 *                              Buffer2D
 * ---------------------------------------------------------------------------
 */
static const int gfx_box_seq[] = {0,1,3,3,2,0};
#define gfx__mtl_resv(b,v,i) ((b)->vbytes += szof(v), (b)->icnt += (i))
#define gfx__mtl_idx(o,p,c) ((castu(o)&0x0fffffff)|(castu(c) << 24u)|(castu(p) << 26u))
#define gfx__mtl_elms(b,o,p)\
  for arr_loopv(i, gfx_box_seq)\
    (buf)->idx[buf->icnt+i] = gfx__mtl_idx(o, p, gfx_box_seq[i])

static unsigned
gfx_mtl_d2d_clip(struct gfx_buf2d *buf, int lhs, int top, int rhs, int bot) {
  unsigned off = castu(buf->vbytes);
  struct gfx_clip *clp = recast(struct gfx_clip*, buf->vtx + buf->vbytes);
  *clp = (struct gfx_clip){.l = casts(lhs), .t = casts(top), .r = casts(rhs), .b = casts(bot)};
  gfx__mtl_resv(buf, *clp, 0);
  return off;
}
static void
gfx_mtl_d2d_box(struct gfx_buf2d *buf, int px0, int py0, int px1, int py1,
                 unsigned col, unsigned clp) {
  struct gfx_box *cmd = recast(struct gfx_box*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_box){.l = casts(px0), .t = casts(py0), .r = casts(px1),
    .b = casts(py1), .clip = clp, .col = col};
  gfx__mtl_elms(buf->idx, buf->vbytes, GFX_PRIM_BOX);
  gfx__mtl_resv(buf, *cmd, cntof(gfx_box_seq));
}
static void
gfx_mtl_d2d_ln(struct gfx_buf2d *buf, int px0, int py0, int px1, int py1,
               int thickness, unsigned col, unsigned clp) {
  struct gfx_ln *cmd = recast(struct gfx_ln*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_ln){.x0 = casts(px0), .y0 = casts(py0), .x1 = casts(px1),
    .y1 = casts(py1), .clip = clp, .col = col, .thickness = castf(thickness)};
  gfx__mtl_elms(buf->idx, buf->vbytes, GFX_PRIM_LN);
  gfx__mtl_resv(buf, *cmd, cntof(gfx_box_seq));
}
static void
gfx_mtl_d2d_circle(struct gfx_buf2d *buf, int ctrx, int ctry, int rad,
                   unsigned col, unsigned clp) {
  int px0 = ctrx - rad;
  int py0 = ctry - rad;
  int px1 = ctrx + rad;
  int py1 = ctry + rad;

  struct gfx_cir *cmd = recast(struct gfx_cir*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_cir){.x0 = casts(px0), .y0 = casts(py0), .x1 = casts(px1),
    .y1 = casts(py1), .clip = clp, .col = col};
  gfx__mtl_elms(buf->idx, buf->vbytes, GFX_PRIM_CIR);
  gfx__mtl_resv(buf, *cmd, cntof(gfx_box_seq));
}
static void
gfx_mtl_d2d_tri(struct gfx_buf2d *buf, int px0, int py0, int px1, int py1,
                int px2, int py2, unsigned col, unsigned clp) {
  struct gfx_tri *cmd = recast(struct gfx_tri*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_tri){.x0 = casts(px0), .y0 = casts(py0), .x1 = casts(px1),
    .y1 = casts(py1), .x2 = casts(px2), .y2 = casts(py2), .clip = clp, .col = col};
  buf->idx[buf->icnt+0] = gfx__mtl_idx(buf->vbytes, GFX_PRIM_TRI, 0);
  buf->idx[buf->icnt+1] = gfx__mtl_idx(buf->vbytes, GFX_PRIM_TRI, 1);
  buf->idx[buf->icnt+2] = gfx__mtl_idx(buf->vbytes, GFX_PRIM_TRI, 2);
  gfx__mtl_resv(buf, *cmd, 3);
}
static void
gfx_mtl_d2d_ico(struct gfx_buf2d *buf, int px0, int py0, int px1, int py1,
                int img_u, int img_v, unsigned col, unsigned clp) {
  struct gfx_ico *cmd = recast(struct gfx_ico*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_ico){.l = casts(px0), .t = casts(py0), .r = casts(px1),
    .b = casts(py1), .u = castus(img_u), .v = castus(img_v), .clip = clp, .col = col};
  gfx__mtl_elms(buf->idx, buf->vbytes, GFX_PRIM_ICO);
  gfx__mtl_resv(buf, *cmd, cntof(gfx_box_seq));
}
static void
gfx_mtl_d2d_img(struct gfx_buf2d *buf, int tex, int dstx, int dsty, int dstw,
                int dsth, int srcx, int srcy, int srcw, int srch, unsigned clp) {

  struct gfx_mtl *mtl = cast(struct gfx_mtl*, buf->intern);
  assert(tex >= 0 && tex < GFX_TEX_MAX);
  assert(mtl->tex[tex].act);
  assert(srcx <= mtl->tex[tex].w);
  assert(srcy <= mtl->tex[tex].h);
  assert(srcx + srcw <= mtl->tex[tex].w);
  assert(srcy + srch <= mtl->tex[tex].h);

  struct gfx_img *cmd = recast(struct gfx_img*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_img){.l = casts(dstx), .t = casts(dsty), .r = casts(dstx + dstw),
    .b = casts(dsty + dsth), .clip = clp, .texcoord = {
      castf(srcx)/castf(mtl->tex[tex].w),
      castf(srcy)/castf(mtl->tex[tex].h),
      castf(srcx + srcw)/castf(mtl->tex[tex].w),
      castf(srcy + srch)/castf(mtl->tex[tex].h),
    }
  };
  unsigned long long tex_id = castull(tex);
  unsigned long long hash = fnv1au64(tex_id, FNV1A64_HASH_INITIAL);
  int tok = tbl_fnd(&mtl->tex_cache, hash);
  if (tbl_inval(&mtl->tex_cache, tok)) {
    int tex_idx = mtl->tex_cnt++;
    mtl->tex_buf[tex_idx] = mtl->tex[tex].hdl;
    tbl_put(&mtl->tex_cache, hash, tex_idx);
    cmd->tex = castu(tex_idx);
  } else {
    cmd->tex = castu(tbl_unref(&mtl->tex_cache,tok,0));
  }
  gfx__mtl_elms(buf->idx, buf->vbytes, GFX_PRIM_IMG);
  gfx__mtl_resv(buf, *cmd, cntof(gfx_box_seq));
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
compiler_assert(cntof(gfx__mtl_pix_fmt) == GFX_PIX_FMT_TYPE_CNT, "[metal] invalid pixel format");
compiler_assert(cntof(gfx__mtl_pix_fmt_bytes) == GFX_PIX_FMT_TYPE_CNT, "[metal] invalid pixel format size");

static int
gfx_mtl_tex_load(struct sys *sys, enum gfx_pix_fmt_type type, void *data,
                 int img_w, int img_h) {
  assert(sys);
  assert(data);

  int i = 0;
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, sys->ren);
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
    width:(NSUInteger)img_w height:(NSUInteger)img_h mipmapped:false];
  tex_desc.usage = MTLTextureUsageShaderRead;

  id<MTLTexture> tex = [mtl->dev newTextureWithDescriptor:tex_desc];
  MTLRegion region = MTLRegionMake2D(0, 0, (NSUInteger)img_w, (NSUInteger)img_h);
  [tex replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:(NSUInteger)(bpr*img_w)];

  struct gfx_tex *img = mtl->tex + i;
  img->w = img_w;
  img->h = img_h;
  img->hdl = tex;
  img->act = 1;
  return i;
}
static void
gfx_mtl_tex_siz(int *siz, struct sys *sys, int id) {
  assert(id >= 0 && id < GFX_TEX_MAX);
  assert(siz);
  assert(sys);

  struct gfx_mtl *mtl = cast(struct gfx_mtl*, sys->ren);
  assert(mtl->tex[id].act);
  struct gfx_tex *tex = mtl->tex + id;

  siz[0] = tex->w;
  siz[1] = tex->h;
}
static void
gfx_mtl_tex_del(struct sys *sys, int hdl) {
  assert(sys);
  assert(hdl >= 0 && hdl < GFX_TEX_MAX);
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, sys->ren);

  assert(mtl->tex[hdl].act);
  struct gfx_tex *tex = mtl->tex + hdl;
  mset(tex, 0, szof(*tex));
}

/* ---------------------------------------------------------------------------
 *                                  System
 * ---------------------------------------------------------------------------
 */
static int
gfx_mtl_init(struct sys *sys, void *view_ptr) {
  assert(sys);
  assert(view_ptr);

  MTKView *view = (__bridge MTKView*)view_ptr;
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, sys->ren);
  mtl->sys = sys;

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
  sys->gfx.buf2d.intern = mtl;
  mtl->sem = dispatch_semaphore_create(GFX_MTL_BUF_DEPTH);
  mtl->cmd_que = [mtl->dev newCommandQueue];

  NSString *exe_path = [[NSBundle mainBundle] executablePath];
  NSString *dir_path = [exe_path stringByDeletingLastPathComponent];

  /* construct the full path to gfx.metallib */
  NSString *lib_path = [dir_path stringByAppendingPathComponent:@"gfx.metallib"];
  NSURL *lib_url = [NSURL fileURLWithPath:lib_path];
#if 1
  mtl->lib = [mtl->dev newLibraryWithURL:lib_url error:&err];
#else
  mtl->lib = [mtl->dev newDefaultLibrary];
#endif
  if (!mtl->lib) {
    NSLog(@"Failed to load library");
    exit(0);
  }
  id<MTLFunction> vshdr_f = [mtl->lib newFunctionWithName:@"ren_vtx"];
  if (!vshdr_f) {
    NSLog(@"Unable to get ren_vtx metal vertex shader function!\n");
    return -1;
  }
  mtl->fshdr_f = [mtl->lib newFunctionWithName:@"ren_frag"];
  if (!mtl->fshdr_f) {
    NSLog(@"Unable to get ren_frag metal framgent shader function!\n");
    return -1;
  }
  MTLRenderPipelineDescriptor *pld = [[MTLRenderPipelineDescriptor alloc] init];
  pld.label = @"tau_pipeline";
  pld.vertexFunction = vshdr_f;
  pld.fragmentFunction = mtl->fshdr_f;
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
  for arr_loopv(i, mtl->buf) {
    mtl->buf[i] = [mtl->dev newBufferWithLength:KB(256) options:MTLResourceStorageModeShared];
  }
  {
    /* allocate fragment argument buffers */
    id<MTLArgumentEncoder> enc = [mtl->fshdr_f newArgumentEncoderWithBufferIndex:0];
    for loop(i, GFX_MTL_BUF_DEPTH) {
      mtl->arg_buf[i] = [mtl->dev newBufferWithLength:enc.encodedLength options:(MTLResourceOptions)0];
      mtl->arg_buf[i].label = @"Argument Buffer";
    }
  }
  return 0;
}
static void
gfx_mtl_shutdown(struct sys *sys) {
  assert(sys);
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, sys->ren);
  unused(mtl);
}
static void
gfx_mtl_begin(struct sys *sys, int scrn_w, int scrn_h) {
  assert(sys);
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, sys->ren);
  mtl->viewportSize.x = castu(scrn_w);
  mtl->viewportSize.y = castu(scrn_h);

  mtl->tex_cnt = 1;
  tbl_clr(&mtl->tex_cache);
  sys->gfx.buf2d.vbytes = 0;
  sys->gfx.buf2d.vtx = 0;
  sys->gfx.buf2d.idx = 0;
  sys->gfx.buf2d.icnt = 0;
}
static void
gfx_mtl_end(struct sys *sys, void *view_ptr) {
  assert(sys);
  assert(view_ptr);

  MTKView *view = (__bridge MTKView*)view_ptr;
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, sys->ren);
  if (!sys->gfx.buf2d.icnt || !sys->gfx.buf2d.vbytes) {
    return;
  }
  dispatch_semaphore_wait(mtl->sem, DISPATCH_TIME_FOREVER);
  mtl->cur_buf = (mtl->cur_buf + 1) % GFX_MTL_BUF_DEPTH;

  float col[4]; col_flt_paq(col, sys->gfx.clear_color);
  view.clearColor = MTLClearColorMake(col[0], col[1], col[2], 1.0);

  int siz = sys->gfx.buf2d.vbytes + sys->gfx.buf2d.icnt * szof(int);
  if (mtl->buf[mtl->cur_buf].length + 16 < (NSUInteger)siz) {
    mtl->buf[mtl->cur_buf] = [mtl->dev newBufferWithLength:(NSUInteger)npow2(siz)
      options:MTLResourceStorageModeShared];
  }
  int ibytes = sys->gfx.buf2d.icnt * szof(unsigned);
  NSUInteger vtx_off = cast(NSUInteger, align_up(ibytes, 16));
  void *idx_buf = mtl->buf[mtl->cur_buf].contents;
  void *vtx_buf = cast(char*, mtl->buf[mtl->cur_buf].contents) + vtx_off;

  mcpy(idx_buf, sys->gfx.buf2d.idx, ibytes);
  mcpy(vtx_buf, sys->gfx.buf2d.vtx, sys->gfx.buf2d.vbytes);

  id<MTLCommandBuffer> cmd_buf = [mtl->cmd_que commandBuffer];
  cmd_buf.label = @"tau_cmd_buf";

  __block dispatch_semaphore_t blk_sem = mtl->sem;
  [cmd_buf addCompletedHandler:^(id<MTLCommandBuffer> buf) {
    unused(buf);
    dispatch_semaphore_signal(blk_sem);
  }];
  id<MTLTexture> tex = mtl->tex[sys->gfx.d2d.tex].hdl;
  mtl->tex_buf[0] = tex;

  struct gfx_uniform uni = {0};
  uni.tex_siz.x = castu([tex width]);
  uni.tex_siz.y = castu([tex height]);
  uni.viewport = mtl->viewportSize;
  {
    id<MTLArgumentEncoder> enc = [mtl->fshdr_f newArgumentEncoderWithBufferIndex:0];
    [enc setArgumentBuffer:mtl->arg_buf[mtl->cur_buf] offset:0];
    [enc setTextures:mtl->tex_buf withRange:NSMakeRange(0, (NSUInteger)mtl->tex_cnt)];
  }
  MTLRenderPassDescriptor* rpd = view.currentRenderPassDescriptor;
  if (rpd != nil) {
    id<MTLRenderCommandEncoder> enc = [cmd_buf renderCommandEncoderWithDescriptor:rpd];
    enc.label = @"tau_command_encoder";
    [enc setViewport:(MTLViewport){0.0, 0.0, mtl->viewportSize.x, mtl->viewportSize.y, 0.0, 1.0}];
    [enc setRenderPipelineState:mtl->pipe_state];
    [enc setVertexBuffer:mtl->buf[mtl->cur_buf] offset:0 atIndex:0];
    [enc setVertexBuffer:mtl->buf[mtl->cur_buf] offset:vtx_off atIndex:1];
    [enc setVertexBytes:&uni length:sizeof(uni) atIndex:2];
    for loop(i, mtl->tex_cnt) {
      [enc useResource:mtl->tex_buf[i] usage:MTLResourceUsageRead stages:MTLRenderStageFragment];
    }
    [enc setFragmentBuffer:mtl->arg_buf[mtl->cur_buf] offset:0 atIndex:0];
    [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:(NSUInteger)sys->gfx.buf2d.icnt];
    [enc endEncoding];
    [cmd_buf presentDrawable:cast(id<MTLDrawable>, view.currentDrawable)];
  }
  [cmd_buf commit];
}
static void
gfx_mtl_resize(struct sys *sys, int win_w, int win_h) {
  assert(sys);
  struct gfx_mtl *mtl = cast(struct gfx_mtl*, sys->ren);
  mtl->viewportSize.x = castu(win_w);
  mtl->viewportSize.y = castu(win_h);
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
      .ico    = {.vbytes  = szof(struct gfx_ico),   .icnt = 6},
      .img    = {.vbytes  = szof(struct gfx_img),   .icnt = 6},
    },
    .clip = gfx_mtl_d2d_clip,
    .box = gfx_mtl_d2d_box,
    .ln = gfx_mtl_d2d_ln,
    .circle = gfx_mtl_d2d_circle,
    .tri = gfx_mtl_d2d_tri,
    .ico = gfx_mtl_d2d_ico,
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

