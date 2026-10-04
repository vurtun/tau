#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>

#define GFX_VK_BUF_DEPTH 2
#define GFX_VK_STAGING_SIZE MB(16)

#include "gfx_shdr.h"
#include "gfx_spv.h"

enum {
  GFX_VK_BUF_SIZE       = KB(512), /* Strictly larger than CFG_GUI_MAX_MEMORY */
  GFX_VK_GPU_TIMEOUT_MS = 33,      /* Hard WCET deadline for GPU frame retirement */
};
struct gfx_vk_target {
  Display *dpy;
  Window win;
  int w, h;
};
struct gfx_tex {
  int act;
  int w, h;
  VkImage img;
  VkDeviceMemory mem;
  VkImageView view;
};
struct gfx_vk {
  struct sys *sys;

  VkInstance instance;
  VkSurfaceKHR surface;
  VkPhysicalDevice phys_dev;
  VkDevice dev;
  VkQueue queue;
  uint32_t queue_family;

  VkPhysicalDeviceMemoryProperties mem_props;
  VkPhysicalDeviceLimits limits;
  size_t min_ssbo_align;

  /* Swapchain */
  VkSwapchainKHR swapchain;
  VkFormat swapchain_fmt;
  VkExtent2D swapchain_extent;
  uint32_t swapchain_img_cnt;
  VkImage swapchain_imgs[8];
  VkImageView swapchain_views[8];
  VkFramebuffer framebuffers[8];
  VkRenderPass render_pass;

  /* Pipeline */
  VkDescriptorSetLayout desc_layout_buf;
  VkDescriptorSetLayout desc_layout_tex;
  VkPipelineLayout pipe_layout;
  VkPipeline pipe;
  VkSampler sampler;

  /* Descriptor Pools & Sets */
  VkDescriptorPool desc_pool;
  VkDescriptorSet set_buf[GFX_VK_BUF_DEPTH];
  VkDescriptorSet set_tex[GFX_VK_BUF_DEPTH];

  /* Command Buffers & Static Ring Buffers */
  VkCommandPool cmd_pool;
  VkCommandBuffer cmd_bufs[GFX_VK_BUF_DEPTH];
  VkBuffer buf[GFX_VK_BUF_DEPTH];
  VkDeviceMemory buf_mem[GFX_VK_BUF_DEPTH];
  void *buf_ptr[GFX_VK_BUF_DEPTH];

  /* Synchronization */
  int cur_buf;
  VkFence fences[GFX_VK_BUF_DEPTH];
  VkSemaphore sem_img_avail[GFX_VK_BUF_DEPTH];
  VkSemaphore sem_ren_fin[GFX_VK_BUF_DEPTH];

  /* Staging Buffer for Texture Uploads */
  VkBuffer stg_buf;
  VkDeviceMemory stg_mem;
  void *stg_ptr;
  VkCommandBuffer stg_cmd;

  /* Textures */
  struct gfx_uniform uni;
  int tex_cnt;
  struct tbl(GFX_MAX_TEX_CNT) tex_cache;
  VkImageView tex_views[GFX_MAX_TEX_CNT];
  struct gfx_tex tex[GFX_TEX_MAX];
};

/* ---------------------------------------------------------------------------
 *                              Buffer2D
 * ---------------------------------------------------------------------------
 */
static const int gfx_box_seq[] = {0, 1, 3, 3, 2, 0};
#define gfx__vk_resv(b,v,i) ((b)->vbytes += szof(v), (b)->icnt += (i))
#define gfx__vk_idx(o,c) ((castu(o) & 0x00ffffff) | (castu(c) << 24u))
#define gfx__vk_elms(b,o)\
  for arr_loopv(i, gfx_box_seq)\
    (b)->idx[(b)->icnt + i] = gfx__vk_idx(o, gfx_box_seq[i])

static unsigned
gfx_vk_d2d_clip(struct gfx_buf2d *buf, int lhs, int top, int rhs, int bot) {
  unsigned off = castu(buf->vbytes);
  struct gfx_clip *clp = recast(struct gfx_clip*, buf->vtx + buf->vbytes);
  *clp = (struct gfx_clip){.l = casts(lhs), .t = casts(top), .r = casts(rhs), .b = casts(bot)};
  gfx__vk_resv(buf, *clp, 0);
  return off;
}
static void
gfx_vk_d2d_box(struct gfx_buf2d *buf, int px0, int py0, int px1, int py1,
               unsigned col, unsigned clp) {
  struct gfx_prim *cmd = recast(struct gfx_prim*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_prim){
    .p0 = {castf(px0), castf(py0)},
    .p1 = {castf(px1), castf(py1)},
    .p2 = {0.0f, 0.0f},
    .uv0 = {0.0f, 0.0f},
    .uv1 = {0.0f, 0.0f},
    .thickness = 0.0f,
    .circle_mask = 0.0f,
    .col = col,
    .clip = clp,
    .tex = 0,
    .flags = GFX_FLAG_NONE,
  };
  gfx__vk_elms(buf, buf->vbytes);
  gfx__vk_resv(buf, *cmd, cntof(gfx_box_seq));
}
static void
gfx_vk_d2d_ln(struct gfx_buf2d *buf, int px0, int py0, int px1, int py1,
              int thickness, unsigned col, unsigned clp) {
  struct gfx_prim *cmd = recast(struct gfx_prim*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_prim){
    .p0 = {castf(px0), castf(py0)},
    .p1 = {castf(px1), castf(py1)},
    .p2 = {0.0f, 0.0f},
    .uv0 = {0.0f, 0.0f},
    .uv1 = {0.0f, 0.0f},
    .thickness = castf(thickness),
    .circle_mask = 0.0f,
    .col = col,
    .clip = clp,
    .tex = 0,
    .flags = GFX_FLAG_LN,
  };
  gfx__vk_elms(buf, buf->vbytes);
  gfx__vk_resv(buf, *cmd, cntof(gfx_box_seq));
}
static void
gfx_vk_d2d_circle(struct gfx_buf2d *buf, int ctrx, int ctry, int rad,
                  unsigned col, unsigned clp) {
  int px0 = ctrx - rad;
  int py0 = ctry - rad;
  int px1 = ctrx + rad;
  int py1 = ctry + rad;

  struct gfx_prim *cmd = recast(struct gfx_prim*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_prim){
    .p0 = {castf(px0), castf(py0)},
    .p1 = {castf(px1), castf(py1)},
    .p2 = {0.0f, 0.0f},
    .uv0 = {0.0f, 0.0f},
    .uv1 = {0.0f, 0.0f},
    .thickness = 0.0f,
    .circle_mask = 1.0f,
    .col = col,
    .clip = clp,
    .tex = 0,
    .flags = GFX_FLAG_CIR,
  };
  gfx__vk_elms(buf, buf->vbytes);
  gfx__vk_resv(buf, *cmd, cntof(gfx_box_seq));
}
static void
gfx_vk_d2d_tri(struct gfx_buf2d *buf, int px0, int py0, int px1, int py1,
               int px2, int py2, unsigned col, unsigned clp) {
  struct gfx_prim *cmd = recast(struct gfx_prim*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_prim){
    .p0 = {castf(px0), castf(py0)},
    .p1 = {castf(px1), castf(py1)},
    .p2 = {castf(px2), castf(py2)},
    .uv0 = {0.0f, 0.0f},
    .uv1 = {0.0f, 0.0f},
    .thickness = 0.0f,
    .circle_mask = 0.0f,
    .col = col,
    .clip = clp,
    .tex = 0,
    .flags = GFX_FLAG_TRI,
  };
  buf->idx[buf->icnt + 0] = gfx__vk_idx(buf->vbytes, 0);
  buf->idx[buf->icnt + 1] = gfx__vk_idx(buf->vbytes, 1);
  buf->idx[buf->icnt + 2] = gfx__vk_idx(buf->vbytes, 2);
  gfx__vk_resv(buf, *cmd, 3);
}
static void
gfx_vk_d2d_ico(struct gfx_buf2d *buf, int px0, int py0, int px1, int py1,
               int img_u, int img_v, unsigned col, unsigned clp) {
  struct gfx_vk *vk = cast(struct gfx_vk*, buf->intern);
  int font_tex = vk->sys->gfx.d2d.tex;
  float inv_w = 1.0f / castf(vk->tex[font_tex].w);
  float inv_h = 1.0f / castf(vk->tex[font_tex].h);

  struct gfx_prim *cmd = recast(struct gfx_prim*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_prim){
    .p0 = {castf(px0), castf(py0)},
    .p1 = {castf(px1), castf(py1)},
    .p2 = {0.0f, 0.0f},
    .uv0 = {castf(img_u) * inv_w, castf(img_v) * inv_h},
    .uv1 = {castf(img_u + (px1 - px0)) * inv_w, castf(img_v + (py1 - py0)) * inv_h},
    .thickness = 0.0f,
    .circle_mask = 0.0f,
    .col = col,
    .clip = clp,
    .tex = 0,
    .flags = GFX_FLAG_TEX | GFX_FLAG_MASK,
  };
  gfx__vk_elms(buf, buf->vbytes);
  gfx__vk_resv(buf, *cmd, cntof(gfx_box_seq));
}
static void
gfx_vk_d2d_img(struct gfx_buf2d *buf, int tex, int dstx, int dsty, int dstw,
               int dsth, int srcx, int srcy, int srcw, int srch, unsigned clp) {
  struct gfx_vk *vk = cast(struct gfx_vk*, buf->intern);
  assert(tex >= 0 && tex < GFX_TEX_MAX);
  assert(vk->tex[tex].act);
  assert(srcx <= vk->tex[tex].w);
  assert(srcy <= vk->tex[tex].h);
  assert(srcx + srcw <= vk->tex[tex].w);
  assert(srcy + srch <= vk->tex[tex].h);

  float inv_w = 1.0f / castf(vk->tex[tex].w);
  float inv_h = 1.0f / castf(vk->tex[tex].h);

  unsigned long long tex_id = castull(tex);
  unsigned long long hash = fnv1au64(tex_id, FNV1A64_HASH_INITIAL);
  int tok = tbl_fnd(&vk->tex_cache, hash);
  unsigned tex_slot = 0;
  if (tbl_inval(&vk->tex_cache, tok)) {
    int tex_idx = vk->tex_cnt++;
    vk->tex_views[tex_idx] = vk->tex[tex].view;
    tbl_put(&vk->tex_cache, hash, tex_idx);
    tex_slot = castu(tex_idx);
  } else {
    tex_slot = castu(tbl_unref(&vk->tex_cache, tok, 0));
  }
  struct gfx_prim *cmd = recast(struct gfx_prim*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_prim){
    .p0 = {castf(dstx), castf(dsty)},
    .p1 = {castf(dstx + dstw), castf(dsty + dsth)},
    .p2 = {0.0f, 0.0f},
    .uv0 = {castf(srcx) * inv_w, castf(srcy) * inv_h},
    .uv1 = {castf(srcx + srcw) * inv_w, castf(srcy + srch) * inv_h},
    .thickness = 0.0f,
    .circle_mask = 0.0f,
    .col = 0xffffffffu,
    .clip = clp,
    .tex = castus(tex_slot),
    .flags = GFX_FLAG_TEX,
  };
  gfx__vk_elms(buf, buf->vbytes);
  gfx__vk_resv(buf, *cmd, cntof(gfx_box_seq));
}

/* ---------------------------------------------------------------------------
 *                          Vulkan Helpers
 * ---------------------------------------------------------------------------
 */
static uint32_t
gfx_vk_mem_type(struct gfx_vk *vk, uint32_t filter, VkMemoryPropertyFlags props) {
  for (uint32_t i = 0; i < vk->mem_props.memoryTypeCount; ++i) {
    if ((filter & (1u << i)) && ((vk->mem_props.memoryTypes[i].propertyFlags & props) == props)) {
      return i;
    }
  }
  return 0xFFFFFFFFu;
}
static void
gfx_vk_buf_create(struct gfx_vk *vk, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags props, VkBuffer *buf, VkDeviceMemory *mem) {
  VkBufferCreateInfo bi = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  vkCreateBuffer(vk->dev, &bi, NULL, buf);

  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(vk->dev, *buf, &req);

  VkMemoryAllocateInfo ai = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = req.size,
    .memoryTypeIndex = gfx_vk_mem_type(vk, req.memoryTypeBits, props),
  };
  vkAllocateMemory(vk->dev, &ai, NULL, mem);
  vkBindBufferMemory(vk->dev, *buf, *mem, 0);
}

/* ---------------------------------------------------------------------------
 *                                  Texture
 * ---------------------------------------------------------------------------
 */
static const VkFormat gfx__vk_pix_fmt[] = {
  [GFX_PIX_FMT_R8]        = VK_FORMAT_R8_UNORM,
  [GFX_PIX_FMT_R8G8B8A8]  = VK_FORMAT_R8G8B8A8_UNORM,
};
static const int gfx__vk_pix_fmt_bytes[] = {
  [GFX_PIX_FMT_R8]        = 1,
  [GFX_PIX_FMT_R8G8B8A8]  = 4,
};
compiler_assert(cntof(gfx__vk_pix_fmt) == GFX_PIX_FMT_TYPE_CNT, "[vk] invalid pixel format");
compiler_assert(cntof(gfx__vk_pix_fmt_bytes) == GFX_PIX_FMT_TYPE_CNT, "[vk] invalid pixel format size");

static int
gfx_vk_tex_load(struct sys *sys, enum gfx_pix_fmt_type type, void *data,
                int img_w, int img_h) {
  assert(sys);
  assert(data);

  int id = 0;
  struct gfx_vk *vk = cast(struct gfx_vk*, sys->ren);
  for (id = 0; id < GFX_TEX_MAX; ++id) {
    if (!vk->tex[id].act) {
      break;
    }
  }
  if (id >= GFX_TEX_MAX) {
    return GFX_TEX_MAX;
  }
  size_t bpr = (size_t)gfx__vk_pix_fmt_bytes[type];
  VkFormat fmt = gfx__vk_pix_fmt[type];
  VkDeviceSize upload_size = (VkDeviceSize)(bpr * (size_t)img_w * (size_t)img_h);
  if (upload_size > GFX_VK_STAGING_SIZE) {
    return GFX_TEX_MAX;
  }
  mcpy(vk->stg_ptr, data, upload_size);

  VkImageCreateInfo ici = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = fmt,
    .extent = {.width = (uint32_t)img_w, .height = (uint32_t)img_h, .depth = 1},
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VkImage img;
  vkCreateImage(vk->dev, &ici, NULL, &img);

  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(vk->dev, img, &req);

  VkMemoryAllocateInfo mai = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = req.size,
    .memoryTypeIndex = gfx_vk_mem_type(vk, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
  };
  VkDeviceMemory mem;
  vkAllocateMemory(vk->dev, &mai, NULL, &mem);
  vkBindImageMemory(vk->dev, img, mem, 0);

  /* Immediate transfer via staging command buffer */
  VkCommandBufferBeginInfo cbbi = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(vk->stg_cmd, &cbbi);

  VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = img,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
    .srcAccessMask = 0,
    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
  };
  vkCmdPipelineBarrier(vk->stg_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

  VkBufferImageCopy region = {
    .bufferOffset = 0,
    .imageSubresource = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel = 0,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
    .imageOffset = {0, 0, 0},
    .imageExtent = {(uint32_t)img_w, (uint32_t)img_h, 1},
  };
  vkCmdCopyBufferToImage(vk->stg_cmd, vk->stg_buf, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  vkCmdPipelineBarrier(vk->stg_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
  vkEndCommandBuffer(vk->stg_cmd);

  VkSubmitInfo si = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &vk->stg_cmd,
  };
  vkQueueSubmit(vk->queue, 1, &si, VK_NULL_HANDLE);
  vkQueueWaitIdle(vk->queue);

  VkImageViewCreateInfo ivci = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = img,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = fmt,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
  };
  VkImageView view;
  vkCreateImageView(vk->dev, &ivci, NULL, &view);

  struct gfx_tex *tex = &vk->tex[id];
  tex->w = img_w;
  tex->h = img_h;
  tex->img = img;
  tex->mem = mem;
  tex->view = view;
  tex->act = 1;
  return id;
}
static void
gfx_vk_tex_siz(int *siz, struct sys *sys, int id) {
  assert(id >= 0 && id < GFX_TEX_MAX);
  assert(siz);
  assert(sys);

  struct gfx_vk *vk = cast(struct gfx_vk*, sys->ren);
  assert(vk->tex[id].act);
  siz[0] = vk->tex[id].w;
  siz[1] = vk->tex[id].h;
}
static void
gfx_vk_tex_del(struct sys *sys, int hdl) {
  assert(sys);
  assert(hdl >= 0 && hdl < GFX_TEX_MAX);
  struct gfx_vk *vk = cast(struct gfx_vk*, sys->ren);
  assert(vk->tex[hdl].act);

  vkDeviceWaitIdle(vk->dev);
  vkDestroyImageView(vk->dev, vk->tex[hdl].view, NULL);
  vkDestroyImage(vk->dev, vk->tex[hdl].img, NULL);
  vkFreeMemory(vk->dev, vk->tex[hdl].mem, NULL);
  mset(&vk->tex[hdl], 0, szof(vk->tex[hdl]));
}

/* ---------------------------------------------------------------------------
 *                          Pipeline & Swapchain
 * ---------------------------------------------------------------------------
 */
static void
gfx_vk_swapchain_destroy(struct gfx_vk *vk) {
  for (uint32_t i = 0; i < vk->swapchain_img_cnt; ++i) {
    if (vk->framebuffers[i]) {
      vkDestroyFramebuffer(vk->dev, vk->framebuffers[i], NULL);
    }
    if (vk->swapchain_views[i]) {
      vkDestroyImageView(vk->dev, vk->swapchain_views[i], NULL);
    }
  }
  if (vk->swapchain) {
    vkDestroySwapchainKHR(vk->dev, vk->swapchain, NULL);
    vk->swapchain = VK_NULL_HANDLE;
  }
}
static void
gfx_vk_swapchain_create(struct gfx_vk *vk, uint32_t width, uint32_t height) {
  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk->phys_dev, vk->surface, &caps);

  uint32_t image_cnt = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && image_cnt > caps.maxImageCount) {
    image_cnt = caps.maxImageCount;
  }
  vk->swapchain_extent.width = clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
  vk->swapchain_extent.height = clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);

  VkSwapchainCreateInfoKHR sci = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = vk->surface,
    .minImageCount = image_cnt,
    .imageFormat = vk->swapchain_fmt,
    .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    .imageExtent = vk->swapchain_extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .preTransform = caps.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = VK_PRESENT_MODE_FIFO_KHR, /* Real-time bounded vsync presentation */
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE,
  };
  vkCreateSwapchainKHR(vk->dev, &sci, NULL, &vk->swapchain);
  vkGetSwapchainImagesKHR(vk->dev, vk->swapchain, &vk->swapchain_img_cnt, NULL);
  vkGetSwapchainImagesKHR(vk->dev, vk->swapchain, &vk->swapchain_img_cnt, vk->swapchain_imgs);

  for (uint32_t i = 0; i < vk->swapchain_img_cnt; ++i) {
    VkImageViewCreateInfo ivci = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = vk->swapchain_imgs[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = vk->swapchain_fmt,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
    };
    vkCreateImageView(vk->dev, &ivci, NULL, &vk->swapchain_views[i]);

    VkFramebufferCreateInfo fci = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = vk->render_pass,
      .attachmentCount = 1,
      .pAttachments = &vk->swapchain_views[i],
      .width = vk->swapchain_extent.width,
      .height = vk->swapchain_extent.height,
      .layers = 1,
    };
    vkCreateFramebuffer(vk->dev, &fci, NULL, &vk->framebuffers[i]);
  }
}
static VkShaderModule
gfx_vk_shdr_module(VkDevice dev, const void *code, size_t size) {
  VkShaderModuleCreateInfo ci = {
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = size,
    .pCode    = (const uint32_t*)code,
  };
  VkShaderModule mod = VK_NULL_HANDLE;
  if (vkCreateShaderModule(dev, &ci, NULL, &mod) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }
  return mod;
}

/* ---------------------------------------------------------------------------
 *                                  System
 * ---------------------------------------------------------------------------
 */
static int
gfx_vk_init(struct sys *sys, void *view_ptr) {
  assert(sys);
  assert(view_ptr);

  struct gfx_vk_target *target = cast(struct gfx_vk_target*, view_ptr);
  struct gfx_vk *vk = cast(struct gfx_vk*, sys->ren);
  vk->sys = sys;
  sys->gfx.buf2d.intern = vk;

  /* 1. Vulkan Instance */
  const char *inst_exts[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
  };
  VkInstanceCreateInfo ici = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .enabledExtensionCount = cntof(inst_exts),
    .ppEnabledExtensionNames = inst_exts,
  };
  if (vkCreateInstance(&ici, NULL, &vk->instance) != VK_SUCCESS) {
    sys->con.err("[vk] failed to create instance\n");
    return -1;
  }
  /* 2. Xlib Surface */
  VkXlibSurfaceCreateInfoKHR xsci = {
    .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
    .dpy = target->dpy,
    .window = target->win,
  };
  if (vkCreateXlibSurfaceKHR(vk->instance, &xsci, NULL, &vk->surface) != VK_SUCCESS) {
    sys->con.err("[vk] failed to create Xlib surface\n");
    return -1;
  }
  /* 3. Physical Device */
  uint32_t dev_cnt = 0;
  vkEnumeratePhysicalDevices(vk->instance, &dev_cnt, NULL);
  VkPhysicalDevice pdevs[16];
  dev_cnt = min(dev_cnt, 16u);
  vkEnumeratePhysicalDevices(vk->instance, &dev_cnt, pdevs);
  vk->phys_dev = pdevs[0]; /* Pick first conforming adapter */

  vkGetPhysicalDeviceMemoryProperties(vk->phys_dev, &vk->mem_props);
  VkPhysicalDeviceProperties dev_props;
  vkGetPhysicalDeviceProperties(vk->phys_dev, &dev_props);
  vk->limits = dev_props.limits;
  vk->min_ssbo_align = max(16u, (size_t)vk->limits.minStorageBufferOffsetAlignment);

  /* 4. Logical Device */
  uint32_t q_cnt = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(vk->phys_dev, &q_cnt, NULL);
  VkQueueFamilyProperties q_props[16];
  q_cnt = min(q_cnt, 16u);
  vkGetPhysicalDeviceQueueFamilyProperties(vk->phys_dev, &q_cnt, q_props);
  for (uint32_t i = 0; i < q_cnt; ++i) {
    VkBool32 present = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(vk->phys_dev, i, vk->surface, &present);
    if ((q_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
      vk->queue_family = i;
      break;
    }
  }
  float q_prio = 1.0f;
  VkDeviceQueueCreateInfo qci = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = vk->queue_family,
    .queueCount = 1,
    .pQueuePriorities = &q_prio,
  };
  const char *dev_exts[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };
  VkPhysicalDeviceDescriptorIndexingFeatures indexing_features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
    .descriptorBindingPartiallyBound = VK_TRUE,
    .runtimeDescriptorArray = VK_TRUE,
    .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
  };
  VkDeviceCreateInfo dci = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &indexing_features,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &qci,
    .enabledExtensionCount = cntof(dev_exts),
    .ppEnabledExtensionNames = dev_exts,
  };
  if (vkCreateDevice(vk->phys_dev, &dci, NULL, &vk->dev) != VK_SUCCESS) {
    sys->con.err("[vk] failed to create device\n");
    return -1;
  }
  vkGetDeviceQueue(vk->dev, vk->queue_family, 0, &vk->queue);

  /* 5. Render Pass */
  vk->swapchain_fmt = VK_FORMAT_B8G8R8A8_UNORM;
  VkAttachmentDescription ad = {
    .format = vk->swapchain_fmt,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };
  VkAttachmentReference ar = {
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };
  VkSubpassDescription sd = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &ar,
  };
  VkRenderPassCreateInfo rpci = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &ad,
    .subpassCount = 1,
    .pSubpasses = &sd,
  };
  vkCreateRenderPass(vk->dev, &rpci, NULL, &vk->render_pass);

  gfx_vk_swapchain_create(vk, castu(target->w), castu(target->h));

  /* 6. Descriptors Layouts */
  // Set 0: SSBO Index Buffer & SSBO Vertex/Primitive Buffer
  VkDescriptorSetLayoutBinding b_bindings[] = {
    {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL},
    {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL},
  };
  VkDescriptorSetLayoutCreateInfo b_dci = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = cntof(b_bindings),
    .pBindings = b_bindings,
  };
  vkCreateDescriptorSetLayout(vk->dev, &b_dci, NULL, &vk->desc_layout_buf);

  // Set 1: Bindless Textures + Static Sampler
  VkDescriptorSetLayoutBinding t_bindings[] = {
    {0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, GFX_MAX_TEX_CNT, VK_SHADER_STAGE_FRAGMENT_BIT, NULL},
    {1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL},
  };
  VkDescriptorBindingFlags flags[] = {
    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
    0,
  };
  VkDescriptorSetLayoutBindingFlagsCreateInfo flags_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
    .bindingCount = cntof(flags),
    .pBindingFlags = flags,
  };
  VkDescriptorSetLayoutCreateInfo t_dci = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = &flags_info,
    .bindingCount = cntof(t_bindings),
    .pBindings = t_bindings,
  };
  vkCreateDescriptorSetLayout(vk->dev, &t_dci, NULL, &vk->desc_layout_tex);

  /* 7. Pipeline Layout & Pipeline */
  VkPushConstantRange pcr = {
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .offset = 0,
    .size = szof(struct gfx_uniform),
  };
  VkDescriptorSetLayout set_layouts[] = {vk->desc_layout_buf, vk->desc_layout_tex};
  VkPipelineLayoutCreateInfo plci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = cntof(set_layouts),
    .pSetLayouts = set_layouts,
    .pushConstantRangeCount = 1,
    .pPushConstantRanges = &pcr,
  };
  vkCreatePipelineLayout(vk->dev, &plci, NULL, &vk->pipe_layout);

  VkSamplerCreateInfo sci = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
  };
  vkCreateSampler(vk->dev, &sci, NULL, &vk->sampler);

  VkShaderModule vmod = gfx_vk_shdr_module(vk->dev, ren_vtx_spv, sizeof(ren_vtx_spv));
  VkShaderModule fmod = gfx_vk_shdr_module(vk->dev, ren_frag_spv, sizeof(ren_frag_spv));
  if (vmod == VK_NULL_HANDLE || fmod == VK_NULL_HANDLE) {
    sys->con.err("[vk] failed to create shader modules from embedded SPIR-V\n");
    if (vmod) vkDestroyShaderModule(vk->dev, vmod, NULL);
    if (fmod) vkDestroyShaderModule(vk->dev, fmod, NULL);
    return -1;
  }
  VkPipelineShaderStageCreateInfo ssci[] = {
    {
      .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage  = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vmod,
      .pName  = "ren_vtx",
    },
    {
      .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fmod,
      .pName  = "ren_frag",
    },
  };

  VkPipelineVertexInputStateCreateInfo visi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  VkPipelineInputAssemblyStateCreateInfo iasci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };
  VkPipelineViewportStateCreateInfo vsci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1,
  };
  VkPipelineRasterizationStateCreateInfo rsci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_NONE,
    .lineWidth = 1.0f,
  };
  VkPipelineMultisampleStateCreateInfo msci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };
  VkPipelineColorBlendAttachmentState cb_att = {
    .blendEnable = VK_TRUE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .alphaBlendOp = VK_BLEND_OP_ADD,
    .colorWriteMask = 0x0F,
  };
  VkPipelineColorBlendStateCreateInfo cbsci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &cb_att,
  };
  VkDynamicState dyn_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dsci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = cntof(dyn_states),
    .pDynamicStates = dyn_states,
  };
  VkGraphicsPipelineCreateInfo gpci = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = cntof(ssci),
    .pStages = ssci,
    .pVertexInputState = &visi,
    .pInputAssemblyState = &iasci,
    .pViewportState = &vsci,
    .pRasterizationState = &rsci,
    .pMultisampleState = &msci,
    .pColorBlendState = &cbsci,
    .pDynamicState = &dsci,
    .layout = vk->pipe_layout,
    .renderPass = vk->render_pass,
  };
  VkResult pipe_res = vkCreateGraphicsPipelines(vk->dev, VK_NULL_HANDLE, 1, &gpci, NULL, &vk->pipe);
  /* Shader modules are compiled into the pipeline state and can now be released */
  vkDestroyShaderModule(vk->dev, vmod, NULL);
  vkDestroyShaderModule(vk->dev, fmod, NULL);
  if (pipe_res != VK_SUCCESS) {
    sys->con.err("[vk] failed to create graphics pipeline\n");
    return -1;
  }
  /* 8. Command Pools, Synchronization & Static Ring Buffers */
  VkCommandPoolCreateInfo cpci = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = vk->queue_family,
  };
  vkCreateCommandPool(vk->dev, &cpci, NULL, &vk->cmd_pool);

  VkCommandBufferAllocateInfo cbai = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = vk->cmd_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = GFX_VK_BUF_DEPTH,
  };
  vkAllocateCommandBuffers(vk->dev, &cbai, vk->cmd_bufs);

  cbai.commandBufferCount = 1;
  vkAllocateCommandBuffers(vk->dev, &cbai, &vk->stg_cmd);

  /* Static host-visible coherent memory (Strict zero-allocation frame loop) */
  for loop(i, GFX_VK_BUF_DEPTH) {
    gfx_vk_buf_create(vk, GFX_VK_BUF_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &vk->buf[i], &vk->buf_mem[i]);
    vkMapMemory(vk->dev, vk->buf_mem[i], 0, GFX_VK_BUF_SIZE, 0, &vk->buf_ptr[i]);

    VkFenceCreateInfo fci = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    vkCreateFence(vk->dev, &fci, NULL, &vk->fences[i]);

    VkSemaphoreCreateInfo sci_sem = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(vk->dev, &sci_sem, NULL, &vk->sem_img_avail[i]);
    vkCreateSemaphore(vk->dev, &sci_sem, NULL, &vk->sem_ren_fin[i]);
  }
  /* Staging Memory */
  gfx_vk_buf_create(vk, GFX_VK_STAGING_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &vk->stg_buf, &vk->stg_mem);
  vkMapMemory(vk->dev, vk->stg_mem, 0, GFX_VK_STAGING_SIZE, 0, &vk->stg_ptr);

  /* Descriptors Pool */
  VkDescriptorPoolSize pool_sizes[] = {
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4},
    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, GFX_MAX_TEX_CNT * GFX_VK_BUF_DEPTH},
    {VK_DESCRIPTOR_TYPE_SAMPLER, 2},
  };
  VkDescriptorPoolCreateInfo dpci = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = 4,
    .poolSizeCount = cntof(pool_sizes),
    .pPoolSizes = pool_sizes,
  };
  vkCreateDescriptorPool(vk->dev, &dpci, NULL, &vk->desc_pool);

  for loop(i, GFX_VK_BUF_DEPTH) {
    VkDescriptorSetAllocateInfo dsai = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = vk->desc_pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &vk->desc_layout_buf,
    };
    vkAllocateDescriptorSets(vk->dev, &dsai, &vk->set_buf[i]);
    dsai.pSetLayouts = &vk->desc_layout_tex;
    vkAllocateDescriptorSets(vk->dev, &dsai, &vk->set_tex[i]);

    VkDescriptorImageInfo samp_info = {.sampler = vk->sampler};
    VkWriteDescriptorSet w = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = vk->set_tex[i],
      .dstBinding = 1,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
      .pImageInfo = &samp_info,
    };
    vkUpdateDescriptorSets(vk->dev, 1, &w, 0, NULL);
  }
  return 0;
}
static void
gfx_vk_shutdown(struct sys *sys) {
  assert(sys);
  struct gfx_vk *vk = cast(struct gfx_vk*, sys->ren);
  vkDeviceWaitIdle(vk->dev);
  for loop(i, GFX_VK_BUF_DEPTH) {
    vkDestroyFence(vk->dev, vk->fences[i], NULL);
    vkDestroySemaphore(vk->dev, vk->sem_img_avail[i], NULL);
    vkDestroySemaphore(vk->dev, vk->sem_ren_fin[i], NULL);
    vkUnmapMemory(vk->dev, vk->buf_mem[i]);
    vkDestroyBuffer(vk->dev, vk->buf[i], NULL);
    vkFreeMemory(vk->dev, vk->buf_mem[i], NULL);
  }
  vkUnmapMemory(vk->dev, vk->stg_mem);
  vkDestroyBuffer(vk->dev, vk->stg_buf, NULL);
  vkFreeMemory(vk->dev, vk->stg_mem, NULL);

  gfx_vk_swapchain_destroy(vk);
  vkDestroyRenderPass(vk->dev, vk->render_pass, NULL);
  vkDestroyPipeline(vk->dev, vk->pipe, NULL);
  vkDestroyPipelineLayout(vk->dev, vk->pipe_layout, NULL);
  vkDestroySampler(vk->dev, vk->sampler, NULL);
  vkDestroyDescriptorSetLayout(vk->dev, vk->desc_layout_buf, NULL);
  vkDestroyDescriptorSetLayout(vk->dev, vk->desc_layout_tex, NULL);
  vkDestroyDescriptorPool(vk->dev, vk->desc_pool, NULL);
  vkDestroyCommandPool(vk->dev, vk->cmd_pool, NULL);
  vkDestroyDevice(vk->dev, NULL);
  vkDestroySurfaceKHR(vk->instance, vk->surface, NULL);
  vkDestroyInstance(vk->instance, NULL);
}
static void
gfx_vk_begin(struct sys *sys, int scrn_w, int scrn_h) {
  assert(sys);
  struct gfx_vk *vk = cast(struct gfx_vk*, sys->ren);
  vk->uni.viewport.x = castu(scrn_w);
  vk->uni.viewport.y = castu(scrn_h);

  vk->tex_cnt = 1;
  tbl_clr(&vk->tex_cache);
  sys->gfx.buf2d.vbytes = 0;
  sys->gfx.buf2d.vtx = 0;
  sys->gfx.buf2d.idx = 0;
  sys->gfx.buf2d.icnt = 0;
}
static void
gfx_vk_end(struct sys *sys, void *view_ptr) {
  assert(sys);
  assert(view_ptr);
  struct gfx_vk *vk = cast(struct gfx_vk*, sys->ren);
  if (!sys->gfx.buf2d.icnt || !sys->gfx.buf2d.vbytes) {
    return;
  }
  /* Bounded GPU synchronization: never wait forever (Real-time deadline) */
  uint64_t timeout_ns = (uint64_t)GFX_VK_GPU_TIMEOUT_MS * 1000000ULL;
  VkResult res = vkWaitForFences(vk->dev, 1, &vk->fences[vk->cur_buf], VK_TRUE, timeout_ns);
  if (res != VK_SUCCESS) {
    return; /* GPU deadline missed: drop frame gracefully to preserve CPU schedule */
  }
  uint32_t img_idx = 0;
  res = vkAcquireNextImageKHR(vk->dev, vk->swapchain, timeout_ns,
    vk->sem_img_avail[vk->cur_buf], VK_NULL_HANDLE, &img_idx);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
    gfx_vk_swapchain_destroy(vk);
    gfx_vk_swapchain_create(vk, vk->uni.viewport.x, vk->uni.viewport.y);
    return;
  }
  vkResetFences(vk->dev, 1, &vk->fences[vk->cur_buf]);

  int ibytes = sys->gfx.buf2d.icnt * szof(unsigned);
  size_t vtx_off = align_up((size_t)ibytes, vk->min_ssbo_align);
  size_t total_req = vtx_off + (size_t)sys->gfx.buf2d.vbytes;
  if (total_req > GFX_VK_BUF_SIZE) {
    return; /* discard frame if memory exceeded */
  }
  void *idx_buf = vk->buf_ptr[vk->cur_buf];
  void *vtx_buf = cast(char*, vk->buf_ptr[vk->cur_buf]) + vtx_off;

  mcpy(idx_buf, sys->gfx.buf2d.idx, ibytes);
  mcpy(vtx_buf, sys->gfx.buf2d.vtx, sys->gfx.buf2d.vbytes);

  /* Update Descriptor Set 0 (Buffers) */
  VkDescriptorBufferInfo b_info[2] = {
    {.buffer = vk->buf[vk->cur_buf], .offset = 0, .range = (VkDeviceSize)vtx_off},
    {.buffer = vk->buf[vk->cur_buf], .offset = vtx_off, .range = (VkDeviceSize)sys->gfx.buf2d.vbytes},
  };
  VkWriteDescriptorSet writes[2] = {
    {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = vk->set_buf[vk->cur_buf], .dstBinding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &b_info[0]},
    {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = vk->set_buf[vk->cur_buf], .dstBinding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &b_info[1]},
  };
  vkUpdateDescriptorSets(vk->dev, 2, writes, 0, NULL);

  /* Update Descriptor Set 1 (Bindless Textures) */
  vk->tex_views[0] = vk->tex[sys->gfx.d2d.tex].view;
  VkDescriptorImageInfo img_infos[GFX_MAX_TEX_CNT];
  for loop(i, vk->tex_cnt) {
    img_infos[i] = (VkDescriptorImageInfo){
      .imageView = vk->tex_views[i],
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
  }
  VkWriteDescriptorSet tex_write = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = vk->set_tex[vk->cur_buf],
    .dstBinding = 0,
    .dstArrayElement = 0,
    .descriptorCount = (uint32_t)vk->tex_cnt,
    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    .pImageInfo = img_infos,
  };
  vkUpdateDescriptorSets(vk->dev, 1, &tex_write, 0, NULL);

  /* Record Command Buffer */
  VkCommandBuffer cmd = vk->cmd_bufs[vk->cur_buf];
  VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(cmd, &begin_info);

  float col[4]; col_flt_paq(col, sys->gfx.clear_color);
  VkClearValue clear_val = {.color = {.float32 = {col[0], col[1], col[2], 1.0f}}};

  VkRenderPassBeginInfo rp_bi = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = vk->render_pass,
    .framebuffer = vk->framebuffers[img_idx],
    .renderArea = {.offset = {0, 0}, .extent = vk->swapchain_extent},
    .clearValueCount = 1,
    .pClearValues = &clear_val,
  };
  vkCmdBeginRenderPass(cmd, &rp_bi, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport vp = {
    .x = 0.0f,
    .y = 0.0f,
    .width = castf(vk->swapchain_extent.width),
    .height = castf(vk->swapchain_extent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  vkCmdSetViewport(cmd, 0, 1, &vp);

  VkRect2D scissor = {.offset = {0, 0}, .extent = vk->swapchain_extent};
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipe);

  VkDescriptorSet sets[] = {vk->set_buf[vk->cur_buf], vk->set_tex[vk->cur_buf]};
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipe_layout, 0, cntof(sets), sets, 0, NULL);

  vk->uni.tex_siz.x = castu(vk->tex[sys->gfx.d2d.tex].w);
  vk->uni.tex_siz.y = castu(vk->tex[sys->gfx.d2d.tex].h);
  vkCmdPushConstants(cmd, vk->pipe_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, szof(vk->uni), &vk->uni);

  vkCmdDraw(cmd, castu(sys->gfx.buf2d.icnt), 1, 0, 0);
  vkCmdEndRenderPass(cmd);
  vkEndCommandBuffer(cmd);

  /* Submit Queue */
  VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &vk->sem_img_avail[vk->cur_buf],
    .pWaitDstStageMask = &wait_stages,
    .commandBufferCount = 1,
    .pCommandBuffers = &cmd,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &vk->sem_ren_fin[vk->cur_buf],
  };
  vkQueueSubmit(vk->queue, 1, &submit_info, vk->fences[vk->cur_buf]);

  /* Present */
  VkPresentInfoKHR present_info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &vk->sem_ren_fin[vk->cur_buf],
    .swapchainCount = 1,
    .pSwapchains = &vk->swapchain,
    .pImageIndices = &img_idx,
  };
  vkQueuePresentKHR(vk->queue, &present_info);
  vk->cur_buf = (vk->cur_buf + 1) % GFX_VK_BUF_DEPTH;
}
static void
gfx_vk_resize(struct sys *sys, int win_w, int win_h) {
  assert(sys);
  struct gfx_vk *vk = cast(struct gfx_vk*, sys->ren);
  vkDeviceWaitIdle(vk->dev);
  gfx_vk_swapchain_destroy(vk);
  gfx_vk_swapchain_create(vk, castu(win_w), castu(win_h));
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
static const struct gfx_api gfx_vk_api = {
  .version = GFX_VERSION,
  .init = gfx_vk_init,
  .begin = gfx_vk_begin,
  .end = gfx_vk_end,
  .shutdown = gfx_vk_shutdown,
  .resize = gfx_vk_resize,
  .d2d = {
    .cost = {
      .clip   = {.vbytes  = szof(struct gfx_clip),  .icnt = 0},
      .box    = {.vbytes  = szof(struct gfx_prim),  .icnt = 6},
      .line   = {.vbytes  = szof(struct gfx_prim),  .icnt = 6},
      .tri    = {.vbytes  = szof(struct gfx_prim),  .icnt = 3},
      .circle = {.vbytes  = szof(struct gfx_prim),  .icnt = 6},
      .ico    = {.vbytes  = szof(struct gfx_prim),  .icnt = 6},
      .img    = {.vbytes  = szof(struct gfx_prim),  .icnt = 6},
    },
    .clip   = gfx_vk_d2d_clip,
    .box    = gfx_vk_d2d_box,
    .ln     = gfx_vk_d2d_ln,
    .circle = gfx_vk_d2d_circle,
    .tri    = gfx_vk_d2d_tri,
    .ico    = gfx_vk_d2d_ico,
    .img    = gfx_vk_d2d_img,
  },
  .tex = {
    .load = gfx_vk_tex_load,
    .info = gfx_vk_tex_siz,
    .del  = gfx_vk_tex_del,
  },
};
static void
gfx_api(void *export, void *import) {
  unused(import);
  struct gfx_api *api = (struct gfx_api*)export;
  *api = gfx_vk_api;
}
