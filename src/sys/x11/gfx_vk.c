#include <vulkan/vulkan.h>

struct gfx_vk {
  struct sys *sys;
  VkInstance ini;
  VkSurfaceKHR surf;
  VkPhysicalDevice phy_dev;
  int quu_fmly;
  VkDevice dev;
  VkSwapchainKHR swp_chn;
};
enum {
  GFX_VK_MAX_DEV    = 64,
  GFX_VK_MAX_QF     = 64,
  GFX_VK_MAX_FMTS   = (1024),
};
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
  return 0;
}
static void
gfx_vk_shutdown(struct sys *sys) {
  struct gfx_vk *vk = cast(struct gfx_vk*, sys->ren);
  vkDestroySwapchainKHR(vk->dev, vk->swp_chn, 0);
  vkDestroySurfaceKHR(vk->ini, vk->surf, 0);
  vkDestroyDevice(vk->dev, NULL);
  vkDestroyInstance(vk->ini, 0);
}

/* ---------------------------------------------------------------------------
 *                                  API
 * ---------------------------------------------------------------------------
 */
static const struct gfx_api gfx_vk_api = {
  .version = GFX_VERSION,
  .init = gfx_vk_init,
  .shutdown = gfx_vk_shutdown,
#if 0
  .begin = gfx_vk_begin,
  .end = gfx_vk_end,
  .resize = gfx_vk_resize,
  .tex = {
    .load = gfx_vk_tex_load,
    .info = gfx_vk_tex_siz,
    .del = gfx_vk_tex_del,
  },
#endif
#if 0
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
#endif
};
static void
gfx_api(void *export, void *import) {
  unused(import);
  struct gfx_api *api = (struct gfx_api*)export;
  *api = gfx_vk_api;
}
