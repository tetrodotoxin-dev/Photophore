// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/vulkan/context.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "implementation/vulkan/dispatch.hpp"

using namespace Perimortem::Core;

#ifdef PERI_DEBUG
constexpr Static::Vector<const char*, 1> validation_layers = {
  {"VK_LAYER_KHRONOS_validation"}};
#endif

static auto require_success(VkResult result, View::Bytes message) -> void {
  if (result != VK_SUCCESS) {
    Diagnostics::Log::fatal(message);
  }
}

#ifdef PERI_DEBUG
static auto instance_layer_available(View::Bytes layer_name) -> Bool {
  U32 layer_count = 0;
  if (Photophore::Vulkan::vkEnumerateInstanceLayerProperties(
          &layer_count, nullptr) != VK_SUCCESS) {
    return False;
  }

  if (layer_count == 0) {
    return False;
  }

  Perimortem::Memory::Dynamic::Vector<VkLayerProperties> layers;
  layers.forgetful_resize(layer_count);
  U32 read_layer_count = layer_count;
  const VkResult layer_read_result =
      Photophore::Vulkan::vkEnumerateInstanceLayerProperties(
          &read_layer_count, layers.get_data());
  if (layer_read_result != VK_SUCCESS && layer_read_result != VK_INCOMPLETE) {
    return False;
  }

  return layers.get_view()
      .slice(0, read_layer_count)
      .contains([layer_name](const VkLayerProperties& layer) {
        return NullTerminated::convert_cstring(
                   layer.layerName, sizeof(layer.layerName)) == layer_name;
      });
}
#endif

static auto read_extensions(VkPhysicalDevice device = VK_NULL_HANDLE)
    -> Perimortem::Memory::Dynamic::Vector<VkExtensionProperties> {
  // Vulkan publishes an upper bound before filling caller storage. The second
  // count is authoritative because the extension inventory can change between
  // calls, so the vector shrinks to the entries actually written.
  auto enumerate = [device](U32* count, VkExtensionProperties* properties) {
    return device ? Photophore::Vulkan::vkEnumerateDeviceExtensionProperties(
                        device, nullptr, count, properties)
                  : Photophore::Vulkan::vkEnumerateInstanceExtensionProperties(
                        nullptr, count, properties);
  };
  Perimortem::Memory::Dynamic::Vector<VkExtensionProperties> extensions;
  U32 count = 0;
  if (enumerate(&count, nullptr) != VK_SUCCESS || count == 0) {
    return extensions;
  }

  extensions.forgetful_resize(count);
  VkResult result = enumerate(&count, extensions.get_data());
  if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
    extensions.clear();
    return extensions;
  }
  extensions.forgetful_resize(count);
  return extensions;
}

static auto supports_extension(
    View::Vector<VkExtensionProperties> extensions,
    const char* name) -> Bool {
  // Both names cross fixed C buffers. Bounded conversion preserves the Vulkan
  // limit while comparing the authored bytes rather than their addresses.
  return extensions.contains([name](const VkExtensionProperties& extension) {
    return NullTerminated::convert_cstring(
               extension.extensionName, sizeof(extension.extensionName)) ==
           NullTerminated::convert_cstring(name, VK_MAX_EXTENSION_NAME_SIZE);
  });
}

static auto select_physical_device(
    VkInstance instance,
    VkSurfaceKHR surface,
    Bool khr_surface,
    Bool ext_surface,
    const char*& maintenance_extension) -> VkPhysicalDevice {
  U32 physical_device_count = 0;
  if (Photophore::Vulkan::vkEnumeratePhysicalDevices(
          instance, &physical_device_count, nullptr) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }
  if (physical_device_count == 0) {
    return VK_NULL_HANDLE;
  }

  Perimortem::Memory::Dynamic::Vector<VkPhysicalDevice> physical_devices;
  physical_devices.forgetful_resize(physical_device_count);
  U32 read_physical_device_count = physical_device_count;
  const auto read = Photophore::Vulkan::vkEnumeratePhysicalDevices(
      instance, &read_physical_device_count, physical_devices.get_data());
  if (read != VK_SUCCESS && read != VK_INCOMPLETE) {
    return VK_NULL_HANDLE;
  }
  if (read_physical_device_count == 0) {
    return VK_NULL_HANDLE;
  }

  // Prefer a discrete GPU that can present to this surface. Integrated and
  // software devices remain eligible fallbacks, giving the same renderer path
  // to development machines and software Vulkan implementations.
  VkPhysicalDevice fallback = VK_NULL_HANDLE;
  for (U32 i = 0; i < read_physical_device_count; i++) {
    VkPhysicalDeviceProperties properties = {};
    Photophore::Vulkan::vkGetPhysicalDeviceProperties(
        physical_devices[i], &properties);
    if (properties.apiVersion < VK_API_VERSION_1_3) {
      continue;
    }
    auto extensions = read_extensions(physical_devices[i]);
    if (!supports_extension(
            extensions.get_view(), VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
      continue;
    }
    const char* maintenance = nullptr;
    if (khr_surface && supports_extension(
                           extensions.get_view(),
                           VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME)) {
      maintenance = VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME;
    } else if (
        ext_surface && supports_extension(
                           extensions.get_view(),
                           VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME)) {
      maintenance = VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME;
    }
    if (!maintenance) {
      continue;
    }

    VkPhysicalDeviceSwapchainMaintenance1FeaturesKHR presentation = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_KHR};
    VkPhysicalDeviceVulkan13Features capabilities = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceFeatures2 features = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    features.pNext = &capabilities;
    capabilities.pNext = &presentation;
    Photophore::Vulkan::vkGetPhysicalDeviceFeatures2(
        physical_devices[i], &features);
    if (!capabilities.dynamicRendering || !capabilities.synchronization2 ||
        !presentation.swapchainMaintenance1) {
      continue;
    }

    // This renderer uses one queue for drawing and presentation. Requiring
    // both capabilities here avoids ownership transfers between queue families
    // throughout the frame and swapchain paths.
    U32 queue_family_count = 0;
    Photophore::Vulkan::vkGetPhysicalDeviceQueueFamilyProperties(
        physical_devices[i], &queue_family_count, nullptr);
    if (queue_family_count == 0) {
      continue;
    }

    Perimortem::Memory::Dynamic::Vector<VkQueueFamilyProperties> queue_families;
    queue_families.forgetful_resize(queue_family_count);
    Photophore::Vulkan::vkGetPhysicalDeviceQueueFamilyProperties(
        physical_devices[i], &queue_family_count, queue_families.get_data());

    Bool usable = False;
    for (U32 queue_family = 0; queue_family < queue_family_count;
         queue_family++) {
      if (!(queue_families[queue_family].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
        continue;
      }

      VkBool32 present_supported = VK_FALSE;
      Photophore::Vulkan::vkGetPhysicalDeviceSurfaceSupportKHR(
          physical_devices[i], queue_family, surface, &present_supported);
      if (present_supported) {
        usable = True;
        break;
      }
    }

    if (!usable) {
      continue;
    }

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      maintenance_extension = maintenance;
      return physical_devices[i];
    }

    fallback = physical_devices[i];
    maintenance_extension = maintenance;
  }

  return fallback;
}

static auto find_graphics_queue_family(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface) -> U32 {
  // Renderer uses one queue family for drawing and presentation. Selecting the
  // first family with both capabilities keeps swapchain images in exclusive
  // sharing mode throughout their lifetime.
  U32 queue_family_count = 0;
  Photophore::Vulkan::vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_count, nullptr);
  if (queue_family_count == 0) {
    return UINT32_MAX;
  }

  Perimortem::Memory::Dynamic::Vector<VkQueueFamilyProperties> queue_families;
  queue_families.forgetful_resize(queue_family_count);
  Photophore::Vulkan::vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_count, queue_families.get_data());
  for (U32 i = 0; i < queue_family_count; i++) {
    if (!(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      continue;
    }

    VkBool32 present = VK_FALSE;
    Photophore::Vulkan::vkGetPhysicalDeviceSurfaceSupportKHR(
        physical_device, i, surface, &present);
    if (present) {
      return i;
    }
  }

  return UINT32_MAX;
}

auto Photophore::Vulkan::Context::create(
    Photophore::Platform::Presentation presentation) -> Context {
  Context ctx;
  const char* extension = surface_extension(presentation.get_kind());
  // Surface kind is known before loader acquisition. A valid Presentation and
  // one Vulkan reservation establish the native entry conditions for every
  // handle published through ctx.
  if (!presentation.is_valid() || !extension || !acquire()) {
    return ctx;
  }
  ctx.owns_loader = True;
  // Submission fences prove command completion. Presentation fences extend
  // that proof through the presentation engine's semaphore and swapchain use,
  // giving resize and shutdown a precise retirement point. KHR and EXT expose
  // the same structures through matching instance and device extensions.
  auto available = read_extensions();
  Bool khr_surface = supports_extension(
      available.get_view(), VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
  Bool ext_surface = supports_extension(
      available.get_view(), VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
  if ((!khr_surface && !ext_surface) ||
      !supports_extension(
          available.get_view(),
          VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME)) {
    return Context();
  }

  Static::Vector<const char*, 5> instance_extensions;
  Count extension_count = 0;
  instance_extensions[extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
  instance_extensions[extension_count++] = extension;
  instance_extensions[extension_count++] =
      VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME;
  if (khr_surface) {
    instance_extensions[extension_count++] =
        VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME;
  }
  if (ext_surface) {
    instance_extensions[extension_count++] =
        VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME;
  }

  VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app_info.pApplicationName = "Photophore";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  instance_info.pApplicationInfo = &app_info;
  instance_info.enabledExtensionCount = U32(extension_count);
  instance_info.ppEnabledExtensionNames = instance_extensions.get_data();

#ifdef PERI_DEBUG
  if (instance_layer_available("VK_LAYER_KHRONOS_validation"_view)) {
    instance_info.enabledLayerCount = U32(validation_layers.get_size());
    instance_info.ppEnabledLayerNames = validation_layers.get_data();
  }
#endif

  // ctx records each native handle immediately after creation. Returning an
  // empty Context transfers the partial state into its destructor, preserving
  // one cleanup path for every discovery stage.
  if (Photophore::Vulkan::vkCreateInstance(
          &instance_info, nullptr, &ctx.instance) != VK_SUCCESS) {
    ctx.instance = VK_NULL_HANDLE;
    return Context();
  }
  if (create_surface(ctx.instance, presentation, &ctx.surface) != VK_SUCCESS) {
    ctx.surface = VK_NULL_HANDLE;
    return Context();
  }
  const char* maintenance_extension = nullptr;
  // Device selection proves the feature and queue profile before logical
  // device creation. The selected extension spelling then follows that exact
  // physical device into VkDeviceCreateInfo.
  ctx.physical_device = select_physical_device(
      ctx.instance, ctx.surface, khr_surface, ext_surface,
      maintenance_extension);
  if (!ctx.physical_device) {
    return Context();
  }
  ctx.graphics_queue_family =
      find_graphics_queue_family(ctx.physical_device, ctx.surface);
  if (ctx.graphics_queue_family == UINT32_MAX) {
    return Context();
  }

  constexpr R32 queue_priority = 1.0f;
  VkDeviceQueueCreateInfo queue_info = {
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queue_info.queueFamilyIndex = ctx.graphics_queue_family;
  queue_info.queueCount = 1;
  queue_info.pQueuePriorities = &queue_priority;

  VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
  dynamic_rendering.dynamicRendering = VK_TRUE;

  VkPhysicalDeviceSynchronization2Features sync2 = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES};
  sync2.synchronization2 = VK_TRUE;
  sync2.pNext = &dynamic_rendering;
  VkPhysicalDeviceSwapchainMaintenance1FeaturesKHR presentation_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_KHR};
  presentation_features.swapchainMaintenance1 = VK_TRUE;
  dynamic_rendering.pNext = &presentation_features;
  const char* device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, maintenance_extension};

  // Double precision is an optional program capability. Context records and
  // enables the physical device answer, allowing Pipelines to admit programs
  // that require it while ordinary image programs use the broader device set.
  VkPhysicalDeviceFeatures available_features = {};
  Photophore::Vulkan::vkGetPhysicalDeviceFeatures(
      ctx.physical_device, &available_features);
  VkPhysicalDeviceFeatures enabled_features = {};
  enabled_features.shaderFloat64 = available_features.shaderFloat64;
  ctx.float64 = Bool(available_features.shaderFloat64);

  VkDeviceCreateInfo device_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  device_info.pNext = &sync2;
  device_info.queueCreateInfoCount = 1;
  device_info.pQueueCreateInfos = &queue_info;
  device_info.enabledExtensionCount = 2;
  device_info.ppEnabledExtensionNames = device_extensions;
  device_info.pEnabledFeatures = &enabled_features;

  if (Photophore::Vulkan::vkCreateDevice(
          ctx.physical_device, &device_info, nullptr, &ctx.device) !=
      VK_SUCCESS) {
    ctx.device = VK_NULL_HANDLE;
    return Context();
  }
  Photophore::Vulkan::vkGetDeviceQueue(
      ctx.device, ctx.graphics_queue_family, 0, &ctx.graphics_queue);

  // Uploads and Renderer frame slots allocate primary command buffers from one
  // resettable pool tied to the selected graphics queue family.
  VkCommandPoolCreateInfo pool_info = {
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = ctx.graphics_queue_family;
  if (Photophore::Vulkan::vkCreateCommandPool(
          ctx.device, &pool_info, nullptr, &ctx.command_pool) != VK_SUCCESS) {
    ctx.command_pool = VK_NULL_HANDLE;
    return Context();
  }
  return ctx;
}

// Renderer and Pipelines finish their GPU work before Context destruction. The
// native hierarchy then unwinds from command pool to loader, keeping each
// device, instance and dispatch function live through the operation that uses
// it.
Photophore::Vulkan::Context::~Context() {
  if (command_pool) {
    Photophore::Vulkan::vkDestroyCommandPool(device, command_pool, nullptr);
  }
  if (device) {
    Photophore::Vulkan::vkDestroyDevice(device, nullptr);
  }
  if (surface) {
    Photophore::Vulkan::vkDestroySurfaceKHR(instance, surface, nullptr);
  }
  if (instance) {
    Photophore::Vulkan::vkDestroyInstance(instance, nullptr);
  }
  if (owns_loader) {
    release();
  }
}

Photophore::Vulkan::Context::Context(Context&& other) noexcept {
  *this = Data::take(other);
}

auto Photophore::Vulkan::Context::operator=(Context&& other) noexcept
    -> Context& {
  if (this == &other) {
    return *this;
  }
  Data::swap(instance, other.instance);
  Data::swap(surface, other.surface);
  Data::swap(physical_device, other.physical_device);
  Data::swap(device, other.device);
  Data::swap(graphics_queue, other.graphics_queue);
  Data::swap(graphics_queue_family, other.graphics_queue_family);
  Data::swap(command_pool, other.command_pool);
  Data::swap(float64, other.float64);
  Data::swap(owns_loader, other.owns_loader);
  return *this;
}

auto Photophore::Vulkan::Context::get_instance() const -> VkInstance {
  return instance;
}

auto Photophore::Vulkan::Context::get_physical_device() const
    -> VkPhysicalDevice {
  return physical_device;
}

auto Photophore::Vulkan::Context::get_device() const -> VkDevice {
  return device;
}

auto Photophore::Vulkan::Context::get_surface() const -> VkSurfaceKHR {
  return surface;
}

auto Photophore::Vulkan::Context::get_graphics_queue() const -> VkQueue {
  return graphics_queue;
}

auto Photophore::Vulkan::Context::get_graphics_queue_family() const -> U32 {
  return graphics_queue_family;
}

auto Photophore::Vulkan::Context::get_command_pool() const -> VkCommandPool {
  return command_pool;
}

auto Photophore::Vulkan::Context::supports_float64() const -> Bool {
  return float64;
}

auto Photophore::Vulkan::Context::begin_immediate_commands() const
    -> VkCommandBuffer {
  // Immediate uploads borrow one primary buffer from the shared command pool.
  // One time submit communicates its single recording lifetime to the driver.
  VkCommandBufferAllocateInfo allocation = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocation.commandPool = command_pool;
  allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocation.commandBufferCount = 1;

  VkCommandBuffer command_buffer = VK_NULL_HANDLE;
  require_success(
      Photophore::Vulkan::vkAllocateCommandBuffers(
          device, &allocation, &command_buffer),
      "Vulkan: Failed to allocate immediate command buffer."_view);

  VkCommandBufferBeginInfo begin_info = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  require_success(
      Photophore::Vulkan::vkBeginCommandBuffer(command_buffer, &begin_info),
      "Vulkan: Failed to begin immediate command buffer."_view);
  return command_buffer;
}

auto Photophore::Vulkan::Context::submit_immediate_commands(
    VkCommandBuffer command_buffer) const -> void {
  // Queue idle is the completion proof used by TextureImage staging. Freeing
  // the command buffer here keeps that synchronous cost and lifetime in the
  // same operation that submits it.
  require_success(
      Photophore::Vulkan::vkEndCommandBuffer(command_buffer),
      "Vulkan: Failed to end immediate command buffer."_view);

  VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &command_buffer;
  require_success(
      Photophore::Vulkan::vkQueueSubmit(
          graphics_queue, 1, &submit, VK_NULL_HANDLE),
      "Vulkan: Failed to submit immediate command buffer."_view);
  require_success(
      Photophore::Vulkan::vkQueueWaitIdle(graphics_queue),
      "Vulkan: Failed to wait for immediate command buffer."_view);
  Photophore::Vulkan::vkFreeCommandBuffers(
      device, command_pool, 1, &command_buffer);
}

auto Photophore::Vulkan::Context::find_memory_type(
    U32 type_filter,
    VkMemoryPropertyFlags properties) const -> U32 {
  VkPhysicalDeviceMemoryProperties memory_properties = {};
  Photophore::Vulkan::vkGetPhysicalDeviceMemoryProperties(
      physical_device, &memory_properties);
  for (U32 i = 0; i < memory_properties.memoryTypeCount; i++) {
    const Bool type_matches = Bool((type_filter & (1u << i)) != 0);
    const Bool properties_match = Bool(
        (memory_properties.memoryTypes[i].propertyFlags & properties) ==
        properties);
    if (type_matches && properties_match) {
      return i;
    }
  }

  return UINT32_MAX;
}
