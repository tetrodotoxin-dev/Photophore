// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/presentation.hpp"

#include "toolchain/validation/test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/view/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Platform;
using namespace Toolchain::Validation;

static Harness PlatformPresentation = {
  .name = "Photophore::Platform::Presentation",
};

struct PresentationProvider {
  U64 prefix = 0x6f70617175650000;
  Count extension_requests = 0;
  Count surface_requests = 0;
};

static VKAPI_ATTR auto VKAPI_CALL load_surface(VkInstance, const char*)
    -> PFN_vkVoidFunction {
  return nullptr;
}

// The provider operations interpret a context whose layout has no relationship
// to Presentation. The test would fail if the consumer recovered native handles
// or treated the context as the capability record itself.
VALIDATION_TEST(PlatformPresentation, provider_operations) {
  static const Presentation::Operations operations = {
    .get_instance_extension =
        [](void* context) {
          auto& provider = *Data::cast<PresentationProvider>(context);
          ++provider.extension_requests;
          return "VK_TEST_provider_surface"_view;
        },

    .create_surface =
        [](void* context, VkInstance instance, PFN_vkGetInstanceProcAddr load,
           VkSurfaceKHR* surface) {
          auto& provider = *Data::cast<PresentationProvider>(context);
          ++provider.surface_requests;
          return instance && load == load_surface && surface
                     ? VK_SUCCESS
                     : VK_ERROR_INITIALIZATION_FAILED;
        },
  };

  PresentationProvider provider;
  auto presentation = Presentation::create(&provider, operations);
  EXPECT(presentation.is_valid());
  EXPECT_EQ(presentation.get_instance_extension()[3], 'T');
  EXPECT_EQ(provider.extension_requests, Count(1));

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  const auto instance = Data::bit_cast<VkInstance>(&provider);
  EXPECT_EQ(
      presentation.create_surface(instance, load_surface, &surface),
      VK_SUCCESS);
  EXPECT_EQ(provider.surface_requests, Count(1));
}

VALIDATION_TEST(PlatformPresentation, unavailable) {
  Presentation presentation;
  EXPECT_NOT(presentation.is_valid());
  EXPECT(presentation.get_instance_extension().is_empty());

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  EXPECT_EQ(
      presentation.create_surface(VK_NULL_HANDLE, load_surface, &surface),
      VK_ERROR_INITIALIZATION_FAILED);
}
