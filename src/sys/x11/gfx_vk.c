#include <vulkan/vulkan.h>

#define GFX_VK_BUF_DEPTH    2
#define GFX_VK_MAX_SWP_IMG 16

#include "gfx_shdr.h"

struct gfx_tex {
  int act;
  int w, h;
  VkImage image;
  VkImageView view;
  VkDeviceMemory memory;
};
struct gfx_vk {
  struct sys *sys;

  VkInstance ini;
  VkSurfaceKHR surf;
  VkPhysicalDevice phy_dev;
  int quu_fmly;
  VkDevice dev;
  VkSwapchainKHR swp_chn;

  int swap_image_count;
  VkImage swap_images[GFX_VK_MAX_SWP_IMG];
  VkImageView swap_image_views[GFX_VK_MAX_SWP_IMG];
  VkFramebuffer framebuffers[GFX_VK_MAX_SWP_IMG];

  VkQueue queue;
  VkCommandPool cmd_pool;
  VkRenderPass render_pass;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
  VkDescriptorSetLayout desc_set_layout;
  VkDescriptorPool desc_pool;
  VkDescriptorSet desc_sets[GFX_VK_BUF_DEPTH];
  VkSemaphore img_available_sem[GFX_VK_BUF_DEPTH];
  VkSemaphore render_finished_sem[GFX_VK_BUF_DEPTH];
  VkFence fences[GFX_VK_BUF_DEPTH];
  VkBuffer buf[GFX_VK_BUF_DEPTH];
  VkDeviceMemory buf_mem[GFX_VK_BUF_DEPTH];
  VkBuffer uniform_buf[GFX_VK_BUF_DEPTH];
  VkDeviceMemory uniform_mem[GFX_VK_BUF_DEPTH];
  unsigned cur_buf;
  unsigned viewportSize[2];

  int tex_cnt;
  struct tbl(int, GFX_MAX_TEX_CNT) tex_cache;
  VkImageView tex_views[GFX_MAX_TEX_CNT];
  struct gfx_tex tex[GFX_MAX_TEX_CNT];
};
enum {
  GFX_VK_MAX_DEV    = 64,
  GFX_VK_MAX_QF     = 64,
  GFX_VK_MAX_FMTS   = (1024),
};

/* ---------------------------------------------------------------------------
 *                              Buffer2D
 * ---------------------------------------------------------------------------
 */
static const int gfx_box_seq[] = {0,1,3,3,2,0};
#define gfx__vk_resv(b,v,i) ((b)->vbytes += szof(v), (b)->icnt += (i))
#define gfx__vk_idx(o,p,c) ((castu(o)&0x0fffffff)|(castu(c) << 24u)|(castu(p) << 26u))
#define gfx__vk_elms(b,o,p)\
  for arr_loopv(i, gfx_box_seq)\
    (buf)->idx[buf->icnt+i] = gfx__vk_idx(o, p, gfx_box_seq[i])

static unsigned
gfx_vk_d2d_clip(struct gfx_buf2d *buf, int lhs, int top, int rhs, int bot) {
  unsigned off = castu(buf->vbytes);
  struct gfx_clip *clp = recast(struct gfx_clip*, buf->vtx + buf->vbytes);
  *clp = (struct gfx_clip){.l = casts(lhs), .t = casts(top), .r = casts(rhs), .b = casts(bot)};
  gfx__vk_resv(buf, *clp, 0);
  return off;
}
static void
gfx_ vk_d2d_box(struct gfx_buf2d *buf, int px0, int py0, int px1, int py1,
                 unsigned col, unsigned clp) {
  struct gfx_box *cmd = recast(struct gfx_box*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_box){.l = casts(px0), .t = casts(py0), .r = casts(px1),
    .b = casts(py1), .clip = clp, .col = col};
  gfx__vk_elms(buf->idx, buf->vbytes, GFX_PRIM_BOX);
  gfx__vk_resv(buf, *cmd, cntof(gfx_box_seq));
}
static void
gfx_vk_d2d_ln(struct gfx_buf2d *buf, int px0, int py0, int px1, int py1,
               int thickness, unsigned col, unsigned clp) {
  struct gfx_ln *cmd = recast(struct gfx_ln*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_ln){.x0 = casts(px0), .y0 = casts(py0), .x1 = casts(px1),
    .y1 = casts(py1), .clip = clp, .col = col, .thickness = castf(thickness)};
  gfx__vk_elms(buf->idx, buf->vbytes, GFX_PRIM_LN);
  gfx__vk_resv(buf, *cmd, cntof(gfx_box_seq));
}
static void
gfx_vk_d2d_circle(struct gfx_buf2d *buf, int ctrx, int ctry, int rad,
                  unsigned col, unsigned clp) {
  int px0 = ctrx - rad;
  int py0 = ctry - rad;
  int px1 = ctrx + rad;
  int py1 = ctry + rad;

  struct gfx_cir *cmd = recast(struct gfx_cir*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_cir){.x0 = casts(px0), .y0 = casts(py0), .x1 = casts(px1),
    .y1 = casts(py1), .clip = clp, .col = col};
  gfx__vk_elms(buf->idx, buf->vbytes, GFX_PRIM_CIR);
  gfx__vk_resv(buf, *cmd, cntof(gfx_box_seq));
}
static void
gfx_vk_d2d_tri(struct gfx_buf2d *buf, int px0, int py0, int px1, int py1,
                int px2, int py2, unsigned col, unsigned clp) {
  struct gfx_tri *cmd = recast(struct gfx_tri*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_tri){.x0 = casts(px0), .y0 = casts(py0), .x1 = casts(px1),
    .y1 = casts(py1), .x2 = casts(px2), .y2 = casts(py2), .clip = clp, .col = col};
  buf->idx[buf->icnt+0] = gfx__mtl_idx(buf->vbytes, GFX_PRIM_TRI, 0);
  buf->idx[buf->icnt+1] = gfx__mtl_idx(buf->vbytes, GFX_PRIM_TRI, 1);
  buf->idx[buf->icnt+2] = gfx__mtl_idx(buf->vbytes, GFX_PRIM_TRI, 2);
  gfx__vk_resv(buf, *cmd, 3);
}
static void
gfx_mtl_d2d_ico(struct gfx_buf2d *buf, int px0, int py0, int px1, int py1,
                int img_u, int img_v, unsigned col, unsigned clp) {
  struct gfx_ico *cmd = recast(struct gfx_ico*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_ico){.l = casts(px0), .t = casts(py0), .r = casts(px1),
    .b = casts(py1), .u = castus(img_u), .v = castus(img_v), .clip = clp, .col = col};
  gfx__vk_elms(buf->idx, buf->vbytes, GFX_PRIM_ICO);
  gfx__vk_resv(buf, *cmd, cntof(gfx_box_seq));
}
static void
gfx_vk_d2d_img(struct gfx_buf2d *buf, int tex, int dstx, int dsty, int dstw,
               int dsth, int srcx, int srcy, int srcw, int srch, unsigned clp) {

  struct gfx_mtl *mtl = cast(struct gfx_mtl*, buf->intern);
  assert(tex >= 0 && tex < GFX_TEX_MAX);
  assert(mtl->tex[tex].act);
  assert(srcx <= mtl->tex[tex].w);
  assert(srcy <= mtl->tex[tex].h);
  assert(srcx + srcw <= mtl->tex[tex].w);
  assert(srcy + srch <= mtl->tex[tex].h);

  struct gfx_img *cmd = recast(struct gfx_img*, buf->vtx + buf->vbytes);
  *cmd = (struct gfx_img){
    .l = casts(dstx), .t = casts(dsty), .r = casts(dstx + dstw),
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
    tbl_put(&mtl->tex_cache, hash, &tex_idx);
    cmd->tex = castu(tex_idx);
  } else {
    cmd->tex = castu(tbl_unref(&mtl->tex_cache,tok,0));
  }
  gfx__vk_elms(buf->idx, buf->vbytes, GFX_PRIM_IMG);
  gfx__vk_resv(buf, *cmd, cntof(gfx_box_seq));
}

/* ---------------------------------------------------------------------------
 *                                  System
 * ---------------------------------------------------------------------------
 */
static int
gfx__vk_def_fmly(VkPhysicalDevice dev, VkSurfaceKHR surf) {
  unsigned cnt = GFX_VK_MAX_QF;
  VkQueueFamilyProperties fmlys[GFX_VK_MAX_QF];
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &cnt, fmlys);
  int num = casti(cnt);

  // try to find suitable queue family
  int sel_fmly = (num == 1) ? 0 : INT_MAX;
  for arr_loopn(i, fmlys, num) {
    VkQueueFamilyProperties fmly = fmlys[i];
    if (!(fmly.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      continue;
    }
    VkBool32 has_present = False;
    vkGetPhysicalDeviceSurfaceSupportKHR(dev, castu(i), surf, &has_present);
    if (has_present) {
      sel_fmly = i;
      break;
    }
  }
  return sel_fmly;
}
static int
gfx__vk_def_phy_dev(VkPhysicalDevice *dev, int *fmly, VkInstance ini,
                    VkSurfaceKHR surf) {
  int ret = -1;
  *fmly = INT_MAX;
  *dev = VK_NULL_HANDLE;

  unsigned cnt = GFX_VK_MAX_DEV;
  VkPhysicalDevice devs[GFX_VK_MAX_DEV];
  vkEnumeratePhysicalDevices(ini, &cnt, devs);
  int num = casti(cnt);

  if (!cnt) {
    printf("[VK] No Vulkan-capable physical devices found!\n");
    return ret;
  } else if (cnt > GFX_VK_MAX_DEV) {
    printf("[VK] to many devices to choose from skipping some!\n");
    return ret;
  }
  int hi_score = -1;
  int sel_fmly = INT_MAX;
  VkPhysicalDevice sel_dev = VK_NULL_HANDLE;
  for arr_loopn(i, devs, num) {

    VkPhysicalDeviceProperties prop = {0};
    vkGetPhysicalDeviceProperties(devs[i], &prop);

    int score = 0;
    score += (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) * 2;
    score += (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
    if (score <= hi_score) {
      continue;
    }
    int gfx_fmly = gfx__vk_def_fmly(devs[i], surf);
    if (gfx_fmly != INT32_MAX) {
      sel_fmly = gfx_fmly;
      sel_dev = devs[i];
      hi_score = score;
      ret = 0;
    }
  }
  *fmly = sel_fmly;
  *dev = sel_dev;
  return ret;
}
static int
gfx__vk_fnd_surf_fmt(VkSurfaceFormatKHR *fmt, VkPhysicalDevice dev,
                     VkSurfaceKHR surf) {

  unsigned fmt_cnt = 0;
  VkSurfaceFormatKHR fmts[GFX_VK_MAX_FMTS] = {0};
  VkResult ret = vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &fmt_cnt, fmts);
  if (ret != VK_SUCCESS) {
    return -1;
  }
  int fmt_num = casti(fmt_cnt);
  for arr_loopn(i, fmts, fmt_num) {
    VkSurfaceFormatKHR *surf_fmt = &fmts[i];
    if (surf_fmt->format == VK_FORMAT_B8G8R8A8_SRGB &&
        surf_fmt->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      *fmt = *surf_fmt;
      return 0;
    }
  }
  for arr_loopn(i, fmts, fmt_num) {
    VkSurfaceFormatKHR *surf_fmt = &fmts[i];
    if (surf_fmt->format == VK_FORMAT_R8G8B8A8_UNORM) {
      *fmt = *surf_fmt;
      return 0;
    }
  }
  return -1;
}
static VkShaderModule
gfx_vk_create_shader_module(VkDevice dev, const uint32_t *code, size_t size) {
  VkShaderModuleCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = size,
    .pCode = code
  };
  VkShaderModule module;
  if (vkCreateShaderModule(dev, &create_info, NULL, &module) != VK_SUCCESS) {
    printf("[VK] Failed to create shader module\n");
    return VK_NULL_HANDLE;
  }
  return module;
}
static int
gfx_vk_init(struct sys *sys, void *view_ptr) {
  static const char* instance_extensions[] = {
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME
  };
  struct gfx_param *param = cast(struct gfx_param*, view_ptr);
  struct gfx_vk *vk = cast(struct gfx_vk*, sys->ren);
  vk->sys = sys;
  int ret = 0;

  /* create instance */
  VkApplicationInfo app = {0};
  app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app.pApplicationName = "Tau";
  app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app.pEngineName = "Tau";
  app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo ini = {0};
  const char *val_lay[] = {"VK_LAYER_KHRONOS_validation"};
  ini.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ini.pApplicationInfo = &app;
  ini.enabledExtensionCount = 1;
  ini.ppEnabledExtensionNames = instance_extensions;
  ini.enabledLayerCount = 1;
  ini.ppEnabledLayerNames = val_lay;

  if (vkCreateInstance(&ini, 0, &vk->ini) != VK_SUCCESS) {
    printf("[VK] Failed to create Vulkan instance\n");
    return -1;
  }

  /* create surface */
  VkXlibSurfaceCreateInfoKHR surf = {0};
  surf.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
  surf.window = param->win;
  surf.dpy = param->dpy;

  ret = vkCreateXlibSurfaceKHR(vk->ini, &surf, NULL, &vk->surf);
  if (ret != VK_SUCCESS) {
    printf("[VK] Failed to create surface\n");
    return -1;
  }

  /* select physical device */
  ret = gfx__vk_def_phy_dev(&vk->phy_dev, &vk->quu_fmly, vk->ini, vk->surf);
  if (ret < 0) {
    printf("[VK] Failed to find a suitable GPU!\n");
    return -1;
  }

  /* create logical device */
  float quu_prio = 1.0f;
  VkDeviceQueueCreateInfo quu = {0};
  quu.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  quu.queueFamilyIndex = castu(vk->quu_fmly);
  quu.pQueuePriorities = &quu_prio;
  quu.queueCount = 1;

  VkDeviceCreateInfo dev = {0};
  static const char *dev_ext[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  dev.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  dev.pQueueCreateInfos = &quu;
  dev.queueCreateInfoCount = 1;
  dev.enabledExtensionCount = 1;
  dev.ppEnabledExtensionNames = dev_ext;

  ret = vkCreateDevice(vk->phy_dev, &dev, 0, &vk->dev);
  if (ret != VK_SUCCESS) {
    printf("[VK] Failed to create logical device\n");
    return -1;
  }

  /* choose the surface format */
  VkSurfaceFormatKHR surf_fmt;
  ret = gfx__vk_fnd_surf_fmt(&surf_fmt, vk->phy_dev, vk->surf);
  if (ret < 0) {
    printf("[VK] Failed to find suitable surface format!\n");
    return -1;
  }

  /* setup swapchain */
  VkSurfaceCapabilitiesKHR cap;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk->phy_dev, vk->surf, &cap);
  unsigned img_cnt = cap.minImageCount + 1u;
  if (cap.maxImageCount > 0 && img_cnt > cap.maxImageCount) {
    img_cnt = cap.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swp_chn = {0};
  swp_chn.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swp_chn.surface = vk->surf;
  swp_chn.minImageCount = img_cnt;
  swp_chn.imageFormat = surf_fmt.format;
  swp_chn.imageColorSpace = surf_fmt.colorSpace;
  swp_chn.imageExtent = cap.currentExtent;
  swp_chn.imageArrayLayers = 1;
  swp_chn.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swp_chn.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swp_chn.queueFamilyIndexCount = 0;
  swp_chn.pQueueFamilyIndices = 0;
  swp_chn.preTransform = cap.currentTransform;
  swp_chn.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swp_chn.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swp_chn.clipped = VK_TRUE;
  swp_chn.oldSwapchain = VK_NULL_HANDLE;

  ret = vkCreateSwapchainKHR(vk->dev, &swp_chn, NULL, &vk->swp_chn);
  if (ret != VK_SUCCESS) {
    printf("Failed to create swapchain\n");
    return -1;
  }
  vkGetSwapchainImagesKHR(vk->dev, vk->swp_chn, &vk->swap_image_count, NULL);
  vk->swap_image_count = min(vk->swap_image_count, GFX_VK_MAX_SWP_IMG);
  vkGetSwapchainImagesKHR(vk->dev, vk->swp_chn, &vk->swap_image_count, vk->swap_images);

  /* Create image views for swapchain images */
  for (int i = 0; i < vk->swap_image_count; i++) {
    VkImageViewCreateInfo view_info = {0};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = vk->swap_images[i];
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = surf_fmt.format; // From your surface format selection
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    ret = vkCreateImageView(vk->dev, &view_info, NULL, &vk->swap_image_views[i]);
    if (ret != VK_SUCCESS) {
      printf("[VK] Failed to create image view for swapchain image %u\n", i);
      return -1;
    }
  }
  /* Create framebuffers */
  for (int i = 0; i < vk->swap_image_count; i++) {
    VkFramebufferCreateInfo fb_info = {0};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.renderPass = vk->render_pass; // From gfx_vk_init
    fb_info.attachmentCount = 1;
    fb_info.pAttachments = &vk->swap_image_views[i];
    fb_info.width = cap.currentExtent.width;
    fb_info.height = cap.currentExtent.height;
    fb_info.layers = 1;

    ret = vkCreateFramebuffer(vk->dev, &fb_info, NULL, &vk->framebuffers[i]);
    if (ret != VK_SUCCESS) {
      printf("[VK] Failed to create framebuffer for swapchain image %u\n", i);
      return -1;
    }
  }
  // Command pool
  VkCommandPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .queueFamilyIndex = vk->quu_fmly,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
  };
  vkCreateCommandPool(vk->dev, &pool_info, 0, &vk->cmd_pool);

  // Render pass
  VkAttachmentDescription color_attachment = {
    .format = VK_FORMAT_B8G8R8A8_UNORM,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  };
  VkAttachmentReference color_ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkSubpassDescription subpass = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &color_ref
  };
  VkRenderPassCreateInfo rp_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &color_attachment,
    .subpassCount = 1,
    .pSubpasses = &subpass
  };
  vkCreateRenderPass(vk->dev, &rp_info, NULL, &vk->render_pass);

  // Descriptor set layout
  VkDescriptorSetLayoutBinding bindings[2] = {
    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL},
    {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, GFX_MAX_TEX_CNT, VK_SHADER_STAGE_FRAGMENT_BIT, NULL}
  };
  VkDescriptorSetLayoutCreateInfo dsl_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 2,
    .pBindings = bindings
  };
  vkCreateDescriptorSetLayout(vk->dev, &dsl_info, NULL, &vk->desc_set_layout);

  // Pipeline layout
  VkPipelineLayoutCreateInfo pl_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &vk->desc_set_layout
  };
  vkCreatePipelineLayout(vk->dev, &pl_info, NULL, &vk->pipeline_layout);

  // Create shader modules from global SPIR-V arrays
  VkShaderModule vert_module = create_shader_module(vk->dev, gfx__vk_shdr_vert, gfx__vk_shdr_vert_size);
  VkShaderModule frag_module = create_shader_module(vk->dev, gfx__vk_shdr_frag, gfx__vk_shdr_frag_size);
  if (vert_module == VK_NULL_HANDLE ||
      frag_module == VK_NULL_HANDLE) {
    printf("[VK] Failed to create shader modules\n");
    return -1;
  }
  // Pipeline shader stages
  VkPipelineShaderStageCreateInfo stages[2] = {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vert_module,
      .pName = "main"
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = frag_module,
      .pName = "main"
    }
  };

  // Pipeline (vertex/fragment shaders compiled to SPIR-V externally)
  VkPipelineShaderStageCreateInfo stages[2] = {
    {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, create_shader_module(vk->dev, vert_spv, vert_spv_size), "main", 0, NULL},
    {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, create_shader_module(vk->dev, frag_spv, frag_spv_size), "main", 0, NULL}
  };
  VkPipelineVertexInputStateCreateInfo vi_info = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  VkPipelineInputAssemblyStateCreateInfo ia_info = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, NULL, 0, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE};
  VkViewport viewport = {0, 0, (float)vk->viewportSize[0], (float)vk->viewportSize[1], 0, 1};
  VkRect2D scissor = {{0, 0}, {vk->viewportSize[0], vk->viewportSize[1]}};
  VkPipelineViewportStateCreateInfo vp_info = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, NULL, 0, 1, &viewport, 1, &scissor};
  VkPipelineRasterizationStateCreateInfo rs_info = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, NULL, 0, VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0};
  VkPipelineMultisampleStateCreateInfo ms_info = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, NULL, 0, VK_SAMPLE_COUNT_1_BIT};
  VkPipelineColorBlendAttachmentState blend_attachment = {VK_TRUE, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, 0xf};
  VkPipelineColorBlendStateCreateInfo cb_info = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, NULL, 0, VK_FALSE, VK_LOGIC_OP_COPY, 1, &blend_attachment};
  VkGraphicsPipelineCreateInfo pipeline_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages = stages,
    .pVertexInputState = &vi_info,
    .pInputAssemblyState = &ia_info,
    .pViewportState = &vp_info,
    .pRasterizationState = &rs_info,
    .pMultisampleState = &ms_info,
    .pColorBlendState = &cb_info,
    .layout = vk->pipeline_layout,
    .renderPass = vk->render_pass,
    .subpass = 0
  };
  vkCreateGraphicsPipelines(vk->dev, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &vk->pipeline);

  // Buffers and synchronization
  for (int i = 0; i < GFX_VK_BUF_DEPTH; i++) {
    VkBufferCreateInfo buf_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL, 0, 256 * 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, 0, NULL};
    vkCreateBuffer(vk->dev, &buf_info, NULL, &vk->buf[i]);
    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(vk->dev, vk->buf[i], &mem_reqs);
    VkMemoryAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, mem_reqs.size, 0}; // Assume memory type 0
    vkAllocateMemory(vk->dev, &alloc_info, NULL, &vk->buf_mem[i]);
    vkBindBufferMemory(vk->dev, vk->buf[i], vk->buf_mem[i], 0);

    buf_info.size = sizeof(struct gfx_uniform);
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    vkCreateBuffer(vk->dev, &buf_info, NULL, &vk->uniform_buf[i]);
    vkGetBufferMemoryRequirements(vk->dev, vk->uniform_buf[i], &mem_reqs);
    alloc_info.allocationSize = mem_reqs.size;
    vkAllocateMemory(vk->dev, &alloc_info, NULL, &vk->uniform_mem[i]);
    vkBindBufferMemory(vk->dev, vk->uniform_buf[i], vk->uniform_mem[i], 0);

    VkSemaphoreCreateInfo sem_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(vk->dev, &sem_info, NULL, &vk->img_available_sem[i]);
    vkCreateSemaphore(vk->dev, &sem_info, NULL, &vk->render_finished_sem[i]);
    VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT};
    vkCreateFence(vk->dev, &fence_info, NULL, &vk->fences[i]);
  }
  // Descriptor pool and sets
  VkDescriptorPoolSize pool_sizes[2] = {
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, GFX_VK_BUF_DEPTH},
    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, GFX_VK_BUF_DEPTH * GFX_MAX_TEX_CNT}
  };
  VkDescriptorPoolCreateInfo dp_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = GFX_VK_BUF_DEPTH,
    .poolSizeCount = 2,
    .pPoolSizes = pool_sizes
  };
  vkCreateDescriptorPool(vk->dev, &dp_info, NULL, &vk->desc_pool);

  VkDescriptorSetAllocateInfo ds_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = vk->desc_pool,
    .descriptorSetCount = GFX_VK_BUF_DEPTH,
    .pSetLayouts = &vk->desc_set_layout
  };
  vkAllocateDescriptorSets(vk->dev, &ds_info, vk->desc_sets);
  return 0;
}
static void
gfx_vk_shutdown(struct sys *sys) {
  assert(sys);
  struct gfx_vk *vk = cast(struct gfx_vk*, sys->ren);
  vkDeviceWaitIdle(vk->dev);

  if (vk->framebuffers) {
    for (uint32_t i = 0; i < vk->swap_image_count; i++) {
      vkDestroyFramebuffer(vk->dev, vk->framebuffers[i], NULL);
    }
  }
  if (vk->swap_image_views) {
    for (uint32_t i = 0; i < vk->swap_image_count; i++) {
      vkDestroyImageView(vk->dev, vk->swap_image_views[i], NULL);
    }
  }
  if (vk->swp_chn != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(vk->dev, vk->swp_chn, NULL);
  }
  if (vk->surf != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(vk->ini, vk->surf, NULL);
  }
  for (int i = 0; i < GFX_VK_BUF_DEPTH; i++) {
    if (vk->buf[i] != VK_NULL_HANDLE) {
      vkDestroyBuffer(vk->dev, vk->buf[i], NULL);
    }
    if (vk->buf_mem[i] != VK_NULL_HANDLE) {
      vkFreeMemory(vk->dev, vk->buf_mem[i], NULL);
    }
    if(vk->uniform_buf[i] != VK_NULL_HANDLE) {
        vkDestroyBuffer(vk->dev, vk->uniform_buf[i], NULL);
    }
    if (vk->uniform_mem[i] != VK_NULL_HANDLE) {
      vkFreeMemory(vk->dev, vk->uniform_mem[i], NULL);
    }
  }
  for (int i = 0; i < GFX_VK_BUF_DEPTH; i++) {
    if (vk->img_available_sem[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(vk->dev, vk->img_available_sem[i], NULL);
    }
    if (vk->render_finished_sem[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(vk->dev, vk->render_finished_sem[i], NULL);
    }
    if (vk->fences[i] != VK_NULL_HANDLE) {
      vkDestroyFence(vk->dev, vk->fences[i], NULL);
    }
  }
  for (int i = 0; i < GFX_MAX_TEX_CNT; i++) {
    if (vk->tex[i].act) {
      if (vk->tex[i].view != VK_NULL_HANDLE) {
        vkDestroyImageView(vk->dev, vk->tex[i].view, NULL);
      }
      if (vk->tex[i].image != VK_NULL_HANDLE) {
        vkDestroyImage(vk->dev, vk->tex[i].image, NULL);
      }
      if (vk->tex[i].memory != VK_NULL_HANDLE) {
        vkFreeMemory(vk->dev, vk->tex[i].memory, NULL);
      }
    }
  }
  if (vk->desc_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(vk->dev, vk->desc_pool, NULL);
  }
  if (vk->desc_set_layout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(vk->dev, vk->desc_set_layout, NULL);
  }
  if (vk->pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(vk->dev, vk->pipeline, NULL);
  }
  if (vk->pipeline_layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(vk->dev, vk->pipeline_layout, NULL);
  }
  if (vk->render_pass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(vk->dev, vk->render_pass, NULL);
  }
  if (vk->cmd_pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(vk->dev, vk->cmd_pool, NULL);
  }
  if (vk->dev != VK_NULL_HANDLE) {
    vkDestroyDevice(vk->dev, NULL);
  }
  if (vk->ini != VK_NULL_HANDLE) {
    vkDestroyInstance(vk->ini, NULL);
  }
  memset(vk, 0, sizeof(struct gfx_vk));
}
static void
gfx_vk_begin(struct sys *sys, int scrn_w, int scrn_h) {
  struct gfx_vk *vk = (struct gfx_vk *)sys->ren;
  vk->viewportSize[0] = scrn_w;
  vk->viewportSize[1] = scrn_h;
  vk->tex_cnt = 1;

  tbl_clr(&vk->tex_cache);
  sys->gfx.buf2d.vbytes = 0;
  sys->gfx.buf2d.vtx = 0;
  sys->gfx.buf2d.idx = 0;
  sys->gfx.buf2d.icnt = 0;
}
static void
gfx_vk_end(struct sys *sys, void *swapchain) {
  struct gfx_vk *vk = (struct gfx_vk *)sys->ren;
  if (!sys->gfx.buf2d.icnt || !sys->gfx.buf2d.vbytes) {
    return;
  }

  uint32_t image_index;
  VkResult ret = vkAcquireNextImageKHR(vk->dev, vk->swp_chn, UINT64_MAX,
    vk->img_available_sem[vk->cur_buf], VK_NULL_HANDLE, &image_index);
  if (ret == VK_ERROR_OUT_OF_DATE_KHR) {
    // Handle swapchain recreation if needed
  } else if (ret != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    printf("[VK] Failed to acquire swapchain image\n");
    return;
  }
  vkWaitForFences(vk->dev, 1, &vk->fences[vk->cur_buf], VK_TRUE, UINT64_MAX);
  vkResetFences(vk->dev, 1, &vk->fences[vk->cur_buf]);

  VkCommandBufferAllocateInfo cmd_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = vk->cmd_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1
  };
  VkCommandBuffer cmd_buf;
  vkAllocateCommandBuffers(vk->dev, &cmd_info, &cmd_buf);

  VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  vkBeginCommandBuffer(cmd_buf, &begin_info);

  // Upload buffers
  void *data;
  uint32_t ibytes = sys->gfx.buf2d.icnt * sizeof(unsigned);
  uint32_t vtx_off = (ibytes + 15) & ~15; // Align to 16
  vkMapMemory(vk->dev, vk->buf_mem[vk->cur_buf], 0, vtx_off + sys->gfx.buf2d.vbytes, 0, &data);
  memcpy(data, sys->gfx.buf2d.idx, ibytes);
  memcpy((char *)data + vtx_off, sys->gfx.buf2d.vtx, sys->gfx.buf2d.vbytes);
  vkUnmapMemory(vk->dev, vk->buf_mem[vk->cur_buf]);

  // Uniforms
  struct gfx_uniform uni = {{vk->viewportSize[0], vk->viewportSize[1]}, {vk->tex[sys->gfx.d2d.tex].w, vk->tex[sys->gfx.d2d.tex].h}};
  vkMapMemory(vk->dev, vk->uniform_mem[vk->cur_buf], 0, sizeof(uni), 0, &data);
  memcpy(data, &uni, sizeof(uni));
  vkUnmapMemory(vk->dev, vk->uniform_mem[vk->cur_buf]);

  // Update descriptor sets
  VkDescriptorBufferInfo buf_info = {vk->uniform_buf[vk->cur_buf], 0, sizeof(struct gfx_uniform)};
  VkDescriptorImageInfo img_infos[GFX_MAX_TEX_CNT];
  for (int i = 0; i < vk->tex_cnt; i++) {
    img_infos[i] = (VkDescriptorImageInfo){VK_NULL_HANDLE, vk->tex_views[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  }
  VkWriteDescriptorSet writes[2] = {
    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, vk->desc_sets[vk->cur_buf], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, NULL, &buf_info, NULL},
    {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, vk->desc_sets[vk->cur_buf], 1, 0, vk->tex_cnt, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, img_infos, NULL, NULL}
  };
  vkUpdateDescriptorSets(vk->dev, 2, writes, 0, NULL);

  // Render pass
  VkClearValue clear_color = {{{sys->gfx.clear_color[0], sys->gfx.clear_color[1], sys->gfx.clear_color[2], 1.0f}}};
  VkRenderPassBeginInfo rp_begin = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = vk->render_pass,
    .framebuffer = vk->framebuffers[image_index]
    .renderArea = {{0, 0}, {vk->viewportSize[0], vk->viewportSize[1]}},
    .clearValueCount = 1,
    .pClearValues = &clear_color
  };
  vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipeline);
  vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipeline_layout, 0, 1, &vk->desc_sets[vk->cur_buf], 0, NULL);
  VkBuffer vertex_buffers[] = {vk->buf[vk->cur_buf], vk->buf[vk->cur_buf]};
  VkDeviceSize offsets[] = {0, vtx_off};
  vkCmdBindVertexBuffers(cmd_buf, 0, 2, vertex_buffers, offsets);
  vkCmdDraw(cmd_buf, sys->gfx.buf2d.icnt, 1, 0, 0);
  vkCmdEndRenderPass(cmd_buf);
  vkEndCommandBuffer(cmd_buf);

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &vk->img_available_sem[vk->cur_buf],
    .pWaitDstStageMask = (VkPipelineStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    .commandBufferCount = 1,
    .pCommandBuffers = &cmd_buf,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &vk->render_finished_sem[vk->cur_buf]
  };
  vkQueueSubmit(vk->queue, 1, &submit_info, vk->fences[vk->cur_buf]);

  VkPresentInfoKHR present_info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &vk->render_finished_sem[vk->cur_buf],
    .swapchainCount = 1,
    .pSwapchains = &vk->swp_chn,
    .pImageIndices = &image_index
  };
  vkQueuePresentKHR(vk->queue, &present_info);

  vk->cur_buf = (vk->cur_buf + 1) % GFX_VK_BUF_DEPTH;
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
static const struct gfx_api gfx_vk_api = {
  .version = GFX_VERSION,
  .init = gfx_vk_init,
  .shutdown = gfx_vk_shutdown,
  .begin = gfx_vk_begin,
  .end = gfx_vk_end,
#if 0
  .resize = gfx_vk_resize,
  .tex = {
    .load = gfx_vk_tex_load,
    .info = gfx_vk_tex_siz,
    .del = gfx_vk_tex_del,
  },
#endif
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
    .clip = gfx_vk_d2d_clip,
    .box = gfx_vk_d2d_box,
    .ln = gfx_vk_d2d_ln,
    .circle = gfx_vk_d2d_circle,
    .tri = gfx_vk_d2d_tri,
    .ico = gfx_vk_d2d_ico,
    .img = gfx_vk_d2d_img,
  },
};
static void
gfx_api(void *export, void *import) {
  unused(import);
  struct gfx_api *api = (struct gfx_api*)export;
  *api = gfx_vk_api;
}

