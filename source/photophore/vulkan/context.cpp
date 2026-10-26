// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/vulkan/context.hpp"

#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/storage/vector.hpp"
#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/thread/bytes.hpp"
#include "perimortem/memory/thread/vector.hpp"

#include "photophore/vulkan/dispatch.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Photophore::Platform;
using namespace Photophore::Vulkan;

#ifdef PERI_DEBUG

// Debug builds request the standard validation layer when the host installed
// it. Validation remains optional so development diagnostics never become a
// runtime requirement of the Photophore SDK.
static constexpr auto validation_layer = "VK_LAYER_KHRONOS_validation\0"_bytes;
static const Storage::Vector<const char*, 1> validation_layers = {{
  Data::cast<const char>(validation_layer.get_data()),
}};

#endif

static constexpr auto application_name = "Photophore\0"_bytes;

static auto require_success(VkResult result, View::Bytes message) -> void {
  if (result != VK_SUCCESS) {
    Diagnostics::Log::fatal(message);
  }
}

#ifdef PERI_DEBUG

static auto instance_layer_available(View::Bytes layer_name) -> Bool {
  U32 layer_count = 0;
  if (Dispatch::get().vkEnumerateInstanceLayerProperties(
          &layer_count, nullptr) != VK_SUCCESS) {
    return False;
  }

  if (layer_count == 0) {
    return False;
  }

  Thread::Vector<VkLayerProperties> layers;
  layers.forgetful_resize(layer_count);

  U32 read_layer_count = layer_count;
  const VkResult layer_read_result =
      Dispatch::get().vkEnumerateInstanceLayerProperties(
          &read_layer_count, layers.get_data());
  if (layer_read_result != VK_SUCCESS && layer_read_result != VK_INCOMPLETE) {
    return False;
  }

  const auto available = layers.get_view().slice(0, read_layer_count);
  return Bool(
      Algorithm::Search::match(
          available, [layer_name](const VkLayerProperties& layer) {
            return View::NullTerminated(
                       layer.layerName, sizeof(layer.layerName))
                       .get_view() == layer_name;
          }) != Count(-1));
}

#endif

static auto read_extensions(VkPhysicalDevice device = VK_NULL_HANDLE)
    -> Thread::Vector<VkExtensionProperties> {
  // Vulkan publishes an upper bound before filling caller storage. The second
  // count is authoritative because the extension inventory can change between
  // calls, so the vector shrinks to the entries actually written.
  auto enumerate = [device](U32* count, VkExtensionProperties* properties) {
    return device ? Dispatch::get().vkEnumerateDeviceExtensionProperties(
                        device, nullptr, count, properties)
                  : Dispatch::get().vkEnumerateInstanceExtensionProperties(
                        nullptr, count, properties);
  };

  Thread::Vector<VkExtensionProperties> extensions;
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

template <CppSize size>
static auto vulkan_name(const char (&name)[size]) -> View::Bytes {
  return View::Bytes(name[0], size - 1);
}

static auto supports_extension(
    View::Vector<VkExtensionProperties> extensions,
    View::Bytes name) -> Bool {
  // Both names cross fixed C buffers. Bounded conversion preserves the Vulkan
  // limit while comparing the authored bytes rather than their addresses.
  return Bool(
      Algorithm::Search::match(
          extensions, [name](const VkExtensionProperties& extension) {
            return View::NullTerminated(
                       extension.extensionName, sizeof(extension.extensionName))
                       .get_view() == name;
          }) != Count(-1));
}

static auto select_physical_device(
    VkInstance instance,
    VkSurfaceKHR surface,
    Bool khr_surface,
    Bool ext_surface,
    View::Bytes& maintenance_extension) -> VkPhysicalDevice {
  U32 physical_device_count = 0;
  if (Dispatch::get().vkEnumeratePhysicalDevices(
          instance, &physical_device_count, nullptr) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }

  if (physical_device_count == 0) {
    return VK_NULL_HANDLE;
  }

  Thread::Vector<VkPhysicalDevice> physical_devices;
  physical_devices.forgetful_resize(physical_device_count);

  U32 read_physical_device_count = physical_device_count;
  const auto read = Dispatch::get().vkEnumeratePhysicalDevices(
      instance, &read_physical_device_count, physical_devices.get_data());
  if (read != VK_SUCCESS && read != VK_INCOMPLETE) {
    return VK_NULL_HANDLE;
  }

  if (read_physical_device_count == 0) {
    return VK_NULL_HANDLE;
  }

  // Device selection requires Vulkan 1.3, the rendering and presentation
  // features used by Context, and one queue family that can both draw and
  // present to this surface. A discrete device returns immediately because it
  // is the explicit performance preference. The first other matching device
  // remains the deterministic fallback.
  VkPhysicalDevice fallback = VK_NULL_HANDLE;
  for (U32 i = 0; i < read_physical_device_count; i++) {
    VkPhysicalDeviceProperties properties = {};
    Dispatch::get().vkGetPhysicalDeviceProperties(
        physical_devices.get_data()[i], &properties);
    if (properties.apiVersion < VK_API_VERSION_1_3) {
      continue;
    }

    auto extensions = read_extensions(physical_devices.get_data()[i]);
    if (!supports_extension(
            extensions.get_view(),
            vulkan_name(VK_KHR_SWAPCHAIN_EXTENSION_NAME))) {
      continue;
    }

    View::Bytes maintenance;
    if (khr_surface &&
        supports_extension(
            extensions.get_view(),
            vulkan_name(VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME))) {
      maintenance = vulkan_name(VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME);
    } else if (
        ext_surface &&
        supports_extension(
            extensions.get_view(),
            vulkan_name(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME))) {
      maintenance = vulkan_name(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME);
    }

    if (maintenance.is_empty()) {
      continue;
    }

    // The KHR and EXT maintenance families expose the same presentation fence
    // capability under paired instance and device extension names. Selection
    // records the device spelling that matches the available instance family.
    VkPhysicalDeviceSwapchainMaintenance1FeaturesKHR presentation = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_KHR};

    VkPhysicalDeviceVulkan13Features capabilities = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};

    VkPhysicalDeviceFeatures2 features = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

    features.pNext = &capabilities;
    capabilities.pNext = &presentation;
    Dispatch::get().vkGetPhysicalDeviceFeatures2(
        physical_devices.get_data()[i], &features);
    if (!capabilities.dynamicRendering || !capabilities.synchronization2 ||
        !presentation.swapchainMaintenance1) {
      continue;
    }

    // This renderer uses one queue for drawing and presentation. Requiring
    // both capabilities here avoids ownership transfers between queue families
    // throughout the frame and swapchain paths.
    U32 queue_family_count = 0;
    Dispatch::get().vkGetPhysicalDeviceQueueFamilyProperties(
        physical_devices.get_data()[i], &queue_family_count, nullptr);
    if (queue_family_count == 0) {
      continue;
    }

    Thread::Vector<VkQueueFamilyProperties> queue_families;
    queue_families.forgetful_resize(queue_family_count);
    Dispatch::get().vkGetPhysicalDeviceQueueFamilyProperties(
        physical_devices.get_data()[i], &queue_family_count,
        queue_families.get_data());

    Bool usable = False;
    for (U32 queue_family = 0; queue_family < queue_family_count;
         queue_family++) {
      if (!(queue_families.get_data()[queue_family].queueFlags &
            VK_QUEUE_GRAPHICS_BIT)) {
        continue;
      }

      VkBool32 present_supported = VK_FALSE;
      Dispatch::get().vkGetPhysicalDeviceSurfaceSupportKHR(
          physical_devices.get_data()[i], queue_family, surface,
          &present_supported);
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
      return physical_devices.get_data()[i];
    }

    if (!fallback) {
      fallback = physical_devices.get_data()[i];
      maintenance_extension = maintenance;
    }
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
  Dispatch::get().vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_count, nullptr);
  if (queue_family_count == 0) {
    return U32(-1);
  }

  Thread::Vector<VkQueueFamilyProperties> queue_families;
  queue_families.forgetful_resize(queue_family_count);
  Dispatch::get().vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_count, queue_families.get_data());
  for (U32 i = 0; i < queue_family_count; i++) {
    if (!(queue_families.get_data()[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      continue;
    }

    VkBool32 present = VK_FALSE;
    Dispatch::get().vkGetPhysicalDeviceSurfaceSupportKHR(
        physical_device, i, surface, &present);
    if (present) {
      return i;
    }
  }

  return U32(-1);
}

// Instance creation enables every available surface maintenance family before
// physical device selection. This preserves devices that expose only the KHR
// or EXT half of the presentation fence contract. `platform_extension` remains
// accessible through vkCreateInstance. `create_instance` returns that complete
// handle after Vulkan consumes the temporary extension inventory.
static auto create_instance(
    View::Bytes platform_extension,
    Bool& khr_surface,
    Bool& ext_surface) -> VkInstance {
  // Swapchain maintenance supplies a fence for each presentation. Submission
  // fences alone prove command completion but cannot prove when the
  // presentation engine has released a semaphore or retired swapchain. KHR
  // and EXT expose the same capability through paired extension families.
  auto available = read_extensions();
  khr_surface = supports_extension(
      available.get_view(),
      vulkan_name(VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME));
  ext_surface = supports_extension(
      available.get_view(),
      vulkan_name(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME));
  if ((!khr_surface && !ext_surface) ||
      !supports_extension(
          available.get_view(),
          vulkan_name(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME))) {
    return VK_NULL_HANDLE;
  }

  Thread::Bytes terminated_extension(platform_extension);
  terminated_extension.append(0);

  // Vulkan create records take a counted array of null terminated extension
  // names. The base surface extension establishes presentation, and the
  // provider contributes the platform surface extension used to create its
  // native VkSurfaceKHR. Surface capabilities2 is required by both maintenance
  // families. Every available maintenance family is enabled because physical
  // device selection happens after instance creation and a device may expose
  // only its matching swapchain maintenance extension.
  Storage::Vector<const char*, 5> instance_extensions;
  Count extension_count = 0;
  instance_extensions[extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
  instance_extensions[extension_count++] =
      Data::cast<const char>(terminated_extension.get_data());
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
  app_info.pApplicationName =
      Data::cast<const char>(application_name.get_data());
  app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  instance_info.pApplicationInfo = &app_info;
  instance_info.enabledExtensionCount = U32(extension_count);
  instance_info.ppEnabledExtensionNames = instance_extensions.get_data();

#ifdef PERI_DEBUG

  if (instance_layer_available(
          validation_layer.slice(0, validation_layer.get_size() - 1))) {
    instance_info.enabledLayerCount = U32(validation_layers.get_size());
    instance_info.ppEnabledLayerNames = validation_layers.get_data();
  }

#endif

  VkInstance instance = VK_NULL_HANDLE;
  if (Dispatch::get().vkCreateInstance(&instance_info, nullptr, &instance) !=
      VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }

  return instance;
}

// Logical device creation enables the complete rendering profile selected from
// the physical device. The feature and extension arrays remain accessible
// through vkCreateDevice. `float64` reports the one optional shader feature.
// Every other requested capability is required before this call.
static auto create_device(
    VkPhysicalDevice physical_device,
    U32 queue_family,
    View::Bytes maintenance_extension,
    Bool& float64) -> VkDevice {
  // Context publishes one queue because every rendering and presentation
  // operation is serialized through the selected family. The priority requests
  // the highest scheduling priority within that family. Reserving additional
  // queues would add device state that the renderer never submits to.
  constexpr R32 queue_priority = 1.0f;
  VkDeviceQueueCreateInfo queue_info = {
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};

  queue_info.queueFamilyIndex = queue_family;
  queue_info.queueCount = 1;
  queue_info.pQueuePriorities = &queue_priority;

  // The feature chain is the minimum native profile implemented by the
  // renderer. Dynamic rendering removes render pass and framebuffer objects,
  // with the attachment format becoming part of each ShaderProgram.
  // Synchronization2 gives image transitions explicit stage and access scopes.
  // Swapchain maintenance adds presentation fences so semaphores and retired
  // chains have a direct completion proof instead of relying on a future image
  // acquisition.
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

  // The device extension inventory has exactly two null terminated names.
  // VK_KHR_swapchain supplies images and queue presentation. The selected KHR
  // or EXT maintenance name supplies the presentation fences required by
  // Renderer. The temporary copy remains accessible through vkCreateDevice.
  Thread::Bytes terminated_maintenance_extension(maintenance_extension);
  terminated_maintenance_extension.append(0);

  Storage::Vector<const char*, 2> device_extensions = {{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    Data::cast<const char>(terminated_maintenance_extension.get_data()),
  }};

  // Double precision is optional for image rendering. Context enables the
  // physical device feature when present and records the result. Program
  // validation then rejects a description requiring float64 on another device.
  VkPhysicalDeviceFeatures available_features = {};
  Dispatch::get().vkGetPhysicalDeviceFeatures(
      physical_device, &available_features);

  VkPhysicalDeviceFeatures enabled_features = {};
  enabled_features.shaderFloat64 = available_features.shaderFloat64;
  float64 = Bool(available_features.shaderFloat64);

  VkDeviceCreateInfo device_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  device_info.pNext = &sync2;
  device_info.queueCreateInfoCount = 1;
  device_info.pQueueCreateInfos = &queue_info;
  device_info.enabledExtensionCount = U32(device_extensions.get_size());
  device_info.ppEnabledExtensionNames = device_extensions.get_data();
  device_info.pEnabledFeatures = &enabled_features;

  VkDevice device = VK_NULL_HANDLE;
  if (Dispatch::get().vkCreateDevice(
          physical_device, &device_info, nullptr, &device) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }

  return device;
}

// The shared pool supplies primary buffers to immediate uploads and rotating
// Renderer slots. Individual reset support lets each completed slot reuse its
// command buffer without resetting other recorded work in the same pool.
static auto create_command_pool(VkDevice device, U32 queue_family)
    -> VkCommandPool {
  VkCommandPoolCreateInfo pool_info = {
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};

  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = queue_family;

  VkCommandPool command_pool = VK_NULL_HANDLE;
  if (Dispatch::get().vkCreateCommandPool(
          device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }

  return command_pool;
}

// Each phase publishes one handle only after its native creation succeeds.
// Returning an empty Context then destroys every completed earlier phase in
// reverse order, including the process loader reservation.
auto Context::create(Presentation presentation) -> Context {
  Context ctx;
  const View::Bytes extension = presentation.get_instance_extension();
  if (!presentation.is_valid() || extension.is_empty() ||
      !Dispatch::acquire()) {
    return ctx;
  }

  ctx.owns_loader = True;

  Bool khr_surface = False;
  Bool ext_surface = False;
  ctx.instance = create_instance(extension, khr_surface, ext_surface);
  if (!ctx.instance) {
    return Context();
  }

  if (presentation.create_surface(
          ctx.instance, Dispatch::get().vkGetInstanceProcAddr, &ctx.surface) !=
      VK_SUCCESS) {
    ctx.surface = VK_NULL_HANDLE;
    return Context();
  }

  View::Bytes maintenance_extension;
  ctx.physical_device = select_physical_device(
      ctx.instance, ctx.surface, khr_surface, ext_surface,
      maintenance_extension);
  if (!ctx.physical_device) {
    return Context();
  }

  ctx.graphics_queue_family =
      find_graphics_queue_family(ctx.physical_device, ctx.surface);
  if (ctx.graphics_queue_family == U32(-1)) {
    return Context();
  }

  ctx.device = create_device(
      ctx.physical_device, ctx.graphics_queue_family, maintenance_extension,
      ctx.float64);
  if (!ctx.device) {
    return Context();
  }

  Dispatch::get().vkGetDeviceQueue(
      ctx.device, ctx.graphics_queue_family, 0, &ctx.graphics_queue);
  ctx.command_pool = create_command_pool(ctx.device, ctx.graphics_queue_family);
  if (!ctx.command_pool) {
    return Context();
  }

  return ctx;
}

// Renderer and Pipelines finish their GPU work before Context destruction. The
// native hierarchy then unwinds from command pool to loader, keeping each
// device, instance and dispatch function live through the operation that uses
// it.
Context::~Context() {
  if (command_pool) {
    Dispatch::get().vkDestroyCommandPool(device, command_pool, nullptr);
  }

  if (device) {
    Dispatch::get().vkDestroyDevice(device, nullptr);
  }

  if (surface) {
    Dispatch::get().vkDestroySurfaceKHR(instance, surface, nullptr);
  }

  if (instance) {
    Dispatch::get().vkDestroyInstance(instance, nullptr);
  }

  if (owns_loader) {
    Dispatch::release();
  }
}

Context::Context(Context&& other) noexcept {
  *this = Data::take(other);
}

auto Context::operator=(Context&& other) noexcept -> Context& {
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

auto Context::get_instance() const -> VkInstance {
  return instance;
}

auto Context::get_physical_device() const -> VkPhysicalDevice {
  return physical_device;
}

auto Context::get_device() const -> VkDevice {
  return device;
}

auto Context::get_surface() const -> VkSurfaceKHR {
  return surface;
}

auto Context::get_graphics_queue() const -> VkQueue {
  return graphics_queue;
}

auto Context::get_graphics_queue_family() const -> U32 {
  return graphics_queue_family;
}

auto Context::get_command_pool() const -> VkCommandPool {
  return command_pool;
}

auto Context::supports_float64() const -> Bool {
  return float64;
}

auto Context::begin_immediate_commands() const -> VkCommandBuffer {
  // Immediate uploads allocate one primary buffer from the shared command
  // pool. One time submit communicates its single recording lifetime to the
  // driver.
  VkCommandBufferAllocateInfo allocation = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};

  allocation.commandPool = command_pool;
  allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocation.commandBufferCount = 1;

  VkCommandBuffer command_buffer = VK_NULL_HANDLE;
  require_success(
      Dispatch::get().vkAllocateCommandBuffers(
          device, &allocation, &command_buffer),
      "Vulkan: Failed to allocate immediate command buffer."_view);

  VkCommandBufferBeginInfo begin_info = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  require_success(
      Dispatch::get().vkBeginCommandBuffer(command_buffer, &begin_info),
      "Vulkan: Failed to begin immediate command buffer."_view);
  return command_buffer;
}

auto Context::submit_immediate_commands(VkCommandBuffer command_buffer) const
    -> void {
  // Queue idle is the completion proof used by TextureImage staging. Freeing
  // the command buffer here keeps that synchronous cost and lifetime in the
  // same operation that submits it.
  require_success(
      Dispatch::get().vkEndCommandBuffer(command_buffer),
      "Vulkan: Failed to end immediate command buffer."_view);

  VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &command_buffer;
  require_success(
      Dispatch::get().vkQueueSubmit(graphics_queue, 1, &submit, VK_NULL_HANDLE),
      "Vulkan: Failed to submit immediate command buffer."_view);
  require_success(
      Dispatch::get().vkQueueWaitIdle(graphics_queue),
      "Vulkan: Failed to wait for immediate command buffer."_view);
  Dispatch::get().vkFreeCommandBuffers(
      device, command_pool, 1, &command_buffer);
}

auto Context::find_memory_type(
    U32 type_filter,
    VkMemoryPropertyFlags properties) const -> U32 {
  VkPhysicalDeviceMemoryProperties memory_properties = {};
  Dispatch::get().vkGetPhysicalDeviceMemoryProperties(
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

  return U32(-1);
}
