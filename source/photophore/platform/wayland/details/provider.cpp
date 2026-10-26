// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/wayland/details/provider.hpp"

#ifdef TOOLCHAIN_LINUX

#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>

#include "perimortem/core/data.hpp"
#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/thread/bytes.hpp"

#include "photophore/platform/details/state.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Photophore::Platform;

static constexpr Count interface_name_capacity = 256;

static auto provider(void* context) -> Wayland::Details::Provider& {
  return *Data::cast<Wayland::Details::Provider>(context);
}

auto Wayland::Details::Provider::callback_provider(const void* context)
    -> Wayland::Details::Provider& {
  const auto* callback = Data::cast<const CallbackContext>(context);
  return *callback->value;
}

auto Wayland::Details::Provider::create(
    U32 width,
    U32 height,
    View::Bytes title) -> Wayland::Details::Provider* {
  auto* window = State::create<Wayland::Details::Provider>(
      [] { return Wayland::Details::Provider(); });
  if (!window) {
    return nullptr;
  }

  window->initialize(width, height, title);
  if (window->failed || !window->surface) {
    window->destroy();
    return nullptr;
  }

  return window;
}

auto Wayland::Details::Provider::initialize(
    U32 width,
    U32 height,
    View::Bytes title) -> void {
  initial_width = logical_width = width;
  initial_height = logical_height = height;
  callback_context.value = this;
  for (Count index = 0; index < title.get_size(); index++) {
    if (title[index] == 0) {
      failed = True;
      return;
    }
  }

  display = Wayland::Details::Client::connect();
  if (!display) {
    failed = True;
    return;
  }

  registry = Wayland::Details::Client::get_registry(display);
  if (!registry) {
    failed = True;
    return;
  }

  static const auto registry_callback =
      +[](const void* context, void* target, U32 opcode,
          const Wayland::Details::ABI::Message*,
          Wayland::Details::ABI::Argument* arguments) -> S32 {
    auto& window = callback_provider(context);
    switch (Wayland::Details::Client::RegistryEvent(opcode)) {
    case Wayland::Details::Client::RegistryEvent::Global:
      window.register_global(
          Data::cast<Wayland::Details::ABI::Proxy>(target), arguments[0].u,
          View::NullTerminated(arguments[1].s, interface_name_capacity)
              .get_view(),
          arguments[2].u);
      return 0;

    case Wayland::Details::Client::RegistryEvent::GlobalRemove:
      window.remove_global(arguments[0].u);
      return 0;
    }

    return -1;
  };

  if (Wayland::Details::Client::listen(
          registry, registry_callback, &callback_context, this) ||
      Wayland::Details::Client::roundtrip(display) < 0 || failed ||
      !compositor || !shell) {
    failed = True;
    return;
  }

  surface = Wayland::Details::Client::create_surface(compositor);
  if (!surface) {
    failed = True;
    return;
  }

  static const auto surface_callback =
      +[](const void* context, void* target, U32 opcode,
          const Wayland::Details::ABI::Message*,
          Wayland::Details::ABI::Argument* arguments) -> S32 {
    if (opcode == U32(Wayland::Details::Client::SurfaceEvent::Enter) ||
        opcode == U32(Wayland::Details::Client::SurfaceEvent::Leave) ||
        opcode == U32(Wayland::Details::Client::SurfaceEvent::
                          PreferredBufferTransform)) {
      return 0;
    }

    if (opcode !=
        U32(Wayland::Details::Client::SurfaceEvent::PreferredBufferScale)) {
      return -1;
    }

    auto& window = callback_provider(context);
    const S32 factor = arguments[0].i;
    if (factor > 0 && U32(factor) != window.scale) {
      window.scale = U32(factor);
      window.needs_resize = True;
      Wayland::Details::Client::set_buffer_scale(
          Data::cast<Wayland::Details::ABI::Proxy>(target), factor);
    }

    return 0;
  };

  if (Wayland::Details::Client::listen(
          surface, surface_callback, &callback_context, this)) {
    failed = True;
    return;
  }

  shell_surface = Wayland::Details::XdgShellV1::get_surface(shell, surface);
  if (!shell_surface) {
    failed = True;
    return;
  }

  static const auto shell_surface_callback =
      +[](const void* context, void* target, U32 opcode,
          const Wayland::Details::ABI::Message*,
          Wayland::Details::ABI::Argument* arguments) -> S32 {
    if (opcode != U32(Wayland::Details::XdgShellV1::SurfaceEvent::Configure)) {
      return -1;
    }

    auto& window = callback_provider(context);
    Wayland::Details::XdgShellV1::acknowledge(
        Data::cast<Wayland::Details::ABI::Proxy>(target), arguments[0].u);
    Wayland::Details::Client::commit(window.surface);
    return 0;
  };

  if (Wayland::Details::Client::listen(
          shell_surface, shell_surface_callback, &callback_context, this)) {
    failed = True;
    return;
  }

  toplevel = Wayland::Details::XdgShellV1::get_toplevel(shell_surface);
  if (!toplevel) {
    failed = True;
    return;
  }

  static const auto toplevel_callback =
      +[](const void* context, void*, U32 opcode,
          const Wayland::Details::ABI::Message*,
          Wayland::Details::ABI::Argument* arguments) -> S32 {
    auto& window = callback_provider(context);
    switch (Wayland::Details::XdgShellV1::ToplevelEvent(opcode)) {
    case Wayland::Details::XdgShellV1::ToplevelEvent::Configure: {
      const S32 width = arguments[0].i;
      const S32 height = arguments[1].i;
      const U32 selected_width = width > 0 ? U32(width) : window.initial_width;
      const U32 selected_height =
          height > 0 ? U32(height) : window.initial_height;
      if (selected_width != window.logical_width ||
          selected_height != window.logical_height) {
        window.logical_width = selected_width;
        window.logical_height = selected_height;
        window.needs_resize = True;
      }

      return 0;
    }

    case Wayland::Details::XdgShellV1::ToplevelEvent::Close:
      window.close_requested = True;
      return 0;
    }

    return -1;
  };

  if (Wayland::Details::Client::listen(
          toplevel, toplevel_callback, &callback_context, this)) {
    failed = True;
    return;
  }

  Thread::Bytes terminated_title(title);
  terminated_title.append(0);

  constexpr auto application = "photophore\0"_view;
  if (!Wayland::Details::XdgShellV1::set_title(
          toplevel, terminated_title.get_view()) ||
      !Wayland::Details::XdgShellV1::set_application(toplevel, application)) {
    failed = True;
    return;
  }

  Wayland::Details::Client::commit(surface);
  if (Wayland::Details::Client::roundtrip(display) < 0) {
    failed = True;
  }
}

auto Wayland::Details::Provider::register_global(
    Wayland::Details::ABI::Proxy* registry,
    U32 name,
    View::Bytes interface,
    U32 version) -> void {
  if (interface == "wl_compositor"_view && !compositor) {
    const U32 selected_version = Wayland::Details::Client::select_version(
        version, Wayland::Details::ABI::wl_compositor_interface, 6);
    if (selected_version == 0) {
      return;
    }

    compositor = Wayland::Details::Client::bind(
        registry, name, Wayland::Details::ABI::wl_compositor_interface,
        selected_version);
    return;
  }

  if (Wayland::Details::XdgShellV1::matches(interface) && !shell) {
    shell = Wayland::Details::XdgShellV1::bind(registry, name);
    if (!shell) {
      failed = True;
      return;
    }

    static const auto shell_callback =
        +[](const void*, void* target, U32 opcode,
            const Wayland::Details::ABI::Message*,
            Wayland::Details::ABI::Argument* arguments) -> S32 {
      if (opcode != U32(Wayland::Details::XdgShellV1::WmBaseEvent::Ping)) {
        return -1;
      }

      Wayland::Details::XdgShellV1::pong(
          Data::cast<Wayland::Details::ABI::Proxy>(target), arguments[0].u);
      return 0;
    };

    if (Wayland::Details::Client::listen(
            shell, shell_callback, &callback_context, this)) {
      Wayland::Details::XdgShellV1::destroy(shell);
      shell = nullptr;
      failed = True;
    }

    return;
  }

  if (interface != "wl_seat"_view || seat) {
    return;
  }

  const U32 selected_version = Wayland::Details::Client::select_version(
      version, Wayland::Details::ABI::wl_seat_interface, 9);
  if (selected_version == 0) {
    return;
  }

  seat = Wayland::Details::Client::bind(
      registry, name, Wayland::Details::ABI::wl_seat_interface,
      selected_version);
  if (!seat) {
    return;
  }

  seat_name = name;

  static const auto seat_callback =
      +[](const void* context, void*, U32 opcode,
          const Wayland::Details::ABI::Message*,
          Wayland::Details::ABI::Argument* arguments) -> S32 {
    switch (Wayland::Details::Client::SeatEvent(opcode)) {
    case Wayland::Details::Client::SeatEvent::Capabilities:
      callback_provider(context).update_seat(arguments[0].u);
      return 0;

    case Wayland::Details::Client::SeatEvent::Name:
      return 0;
    }

    return -1;
  };

  if (Wayland::Details::Client::listen(
          seat, seat_callback, &callback_context, this)) {
    clear_input();
  }
}

auto Wayland::Details::Provider::update_seat(U32 capabilities) -> void {
  const Bool has_keyboard =
      Bool(capabilities & Wayland::Details::Client::keyboard_capability);
  const Bool has_pointer =
      Bool(capabilities & Wayland::Details::Client::pointer_capability);

  if (has_keyboard && !keyboard) {
    keyboard = Wayland::Details::Client::get_keyboard(seat);
    if (keyboard) {
      static const auto keyboard_callback =
          +[](const void* context, void*, U32 opcode,
              const Wayland::Details::ABI::Message*,
              Wayland::Details::ABI::Argument* arguments) -> S32 {
        auto& window = callback_provider(context);
        switch (Wayland::Details::Client::KeyboardEvent(opcode)) {
        case Wayland::Details::Client::KeyboardEvent::Keymap:
          if (arguments[1].h >= 0) {
            close(arguments[1].h);
          }

          return 0;

        case Wayland::Details::Client::KeyboardEvent::Enter: {
          if (arguments[1].o != window.surface) {
            return 0;
          }

          window.input_collector.clear_keyboard();
          window.keyboard_focused = True;

          const Wayland::Details::ABI::Array* keys = arguments[2].a;
          const auto* codes = Data::cast<U32>(keys->data);
          for (Count index = 0; index < keys->size / sizeof(U32); index++) {
            window.input_collector.set_key(
                Collector::translate_keyboard(codes[index]), True);
          }

          return 0;
        }

        case Wayland::Details::Client::KeyboardEvent::Leave:
          if (arguments[1].o == window.surface) {
            window.keyboard_focused = False;
            window.input_collector.clear_keyboard();
          }

          return 0;

        case Wayland::Details::Client::KeyboardEvent::Key:
          if (window.keyboard_focused) {
            window.input_collector.set_key(
                Collector::translate_keyboard(arguments[2].u),
                Bool(arguments[3].u != 0));
          }

          return 0;

        case Wayland::Details::Client::KeyboardEvent::Modifiers:
        case Wayland::Details::Client::KeyboardEvent::RepeatInfo:
          return 0;
        }

        return -1;
      };

      if (Wayland::Details::Client::listen(
              keyboard, keyboard_callback, &callback_context, this)) {
        Wayland::Details::Client::release_keyboard(keyboard);
        keyboard = nullptr;
      }
    }
  } else if (!has_keyboard && keyboard) {
    Wayland::Details::Client::release_keyboard(keyboard);
    keyboard = nullptr;
    keyboard_focused = False;
    input_collector.clear_keyboard();
  }

  if (has_pointer && !pointer) {
    pointer = Wayland::Details::Client::get_pointer(seat);
    if (pointer) {
      static const auto pointer_callback =
          +[](const void* context, void*, U32 opcode,
              const Wayland::Details::ABI::Message*,
              Wayland::Details::ABI::Argument* arguments) -> S32 {
        auto& window = callback_provider(context);
        switch (Wayland::Details::Client::PointerEvent(opcode)) {
        case Wayland::Details::Client::PointerEvent::Enter:
          if (arguments[1].o == window.surface) {
            window.input_collector.set_pointer(
                R32(arguments[2].f) / 256.0f, R32(arguments[3].f) / 256.0f);
          }

          return 0;

        case Wayland::Details::Client::PointerEvent::Leave:
          if (arguments[1].o == window.surface) {
            window.input_collector.clear_pointer();
          }

          return 0;

        case Wayland::Details::Client::PointerEvent::Motion:
          if (window.input_collector.is_pointer_active()) {
            window.input_collector.set_pointer(
                R32(arguments[1].f) / 256.0f, R32(arguments[2].f) / 256.0f);
          }

          return 0;

        case Wayland::Details::Client::PointerEvent::Button:
          if (window.input_collector.is_pointer_active()) {
            window.input_collector.set_key(
                Collector::translate_button(arguments[2].u),
                Bool(arguments[3].u == Wayland::Details::Client::pressed));
          }

          return 0;

        case Wayland::Details::Client::PointerEvent::Axis:
          if (window.input_collector.is_pointer_active()) {
            const R32 amount = R32(arguments[2].f) / 256.0f;
            if (arguments[1].u == Wayland::Details::Client::horizontal_scroll) {
              window.input_collector.scroll(amount, 0);
            } else {
              window.input_collector.scroll(0, amount);
            }
          }

          return 0;

        case Wayland::Details::Client::PointerEvent::Frame:
        case Wayland::Details::Client::PointerEvent::AxisSource:
        case Wayland::Details::Client::PointerEvent::AxisStop:
        case Wayland::Details::Client::PointerEvent::AxisDiscrete:
        case Wayland::Details::Client::PointerEvent::AxisValue120:
        case Wayland::Details::Client::PointerEvent::AxisRelativeDirection:
          return 0;
        }

        return -1;
      };

      if (Wayland::Details::Client::listen(
              pointer, pointer_callback, &callback_context, this)) {
        Wayland::Details::Client::release_pointer(pointer);
        pointer = nullptr;
      }
    }
  } else if (!has_pointer && pointer) {
    Wayland::Details::Client::release_pointer(pointer);
    pointer = nullptr;
    input_collector.clear_pointer();
  }
}

auto Wayland::Details::Provider::remove_global(U32 name) -> void {
  if (seat_name == name) {
    clear_input();
  }
}

auto Wayland::Details::Provider::clear_input() -> void {
  if (keyboard) {
    Wayland::Details::Client::release_keyboard(keyboard);
    keyboard = nullptr;
  }

  if (pointer) {
    Wayland::Details::Client::release_pointer(pointer);
    pointer = nullptr;
  }

  if (seat) {
    Wayland::Details::Client::release_seat(seat);
    seat = nullptr;
  }

  seat_name = 0;
  keyboard_focused = False;
  input_collector.clear_keyboard();
  input_collector.clear_pointer();
}

auto Wayland::Details::Provider::destroy() -> void {
  clear_input();
  if (toplevel) {
    Wayland::Details::XdgShellV1::destroy(toplevel);
  }

  if (shell_surface) {
    Wayland::Details::XdgShellV1::destroy(shell_surface);
  }

  if (shell) {
    Wayland::Details::XdgShellV1::destroy(shell);
  }

  if (surface) {
    Wayland::Details::Client::destroy_surface(surface);
  }

  if (compositor) {
    Wayland::Details::Client::release_compositor(compositor);
  }

  if (registry) {
    Wayland::Details::Client::destroy_proxy(registry);
  }

  Wayland::Details::Client::disconnect(display);

  State::destroy(this);
}

auto Wayland::Details::Provider::poll_events() -> EventStatus {
  EventStatus status = get_event_status();
  if (status != EventStatus::Ready) {
    return status;
  }

  if (Wayland::Details::Client::dispatch_pending(display) < 0) {
    failed = True;
    return EventStatus::Failed;
  }

  if (close_requested) {
    return EventStatus::Closed;
  }

  // Wayland permits a socket read only after its pending queue is drained.
  // A successful prepare_read reserves that read, so every later path either
  // reads events or cancels the reservation before returning.
  while (Wayland::Details::Client::prepare_read(display) != 0) {
    if (Wayland::Details::Client::dispatch_pending(display) < 0) {
      failed = True;
      return EventStatus::Failed;
    }

    if (close_requested) {
      return EventStatus::Closed;
    }
  }

  if (Wayland::Details::Client::flush(display) < 0 && errno != EAGAIN) {
    Wayland::Details::Client::cancel_read(display);
    failed = True;
    return EventStatus::Failed;
  }

  // A zero timeout makes each call one event observation for the application
  // frame loop. The application remains responsible for its broader scheduling
  // and compositor wait policy.
  pollfd display_fd = {
    Wayland::Details::Client::get_descriptor(display),
    POLLIN,
    0,
  };

  auto poll_result = 0;
  do {
    poll_result = poll(&display_fd, 1, 0);
  } while (poll_result < 0 && errno == EINTR);

  if (poll_result < 0) {
    Wayland::Details::Client::cancel_read(display);
    failed = True;
    return EventStatus::Failed;
  }

  if (display_fd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
    Wayland::Details::Client::cancel_read(display);
    failed = True;
    return EventStatus::Failed;
  }

  if (poll_result > 0 && (display_fd.revents & POLLIN)) {
    if (Wayland::Details::Client::read_events(display) < 0) {
      failed = True;
      return EventStatus::Failed;
    }
  } else {
    Wayland::Details::Client::cancel_read(display);
  }

  if (Wayland::Details::Client::dispatch_pending(display) < 0) {
    failed = True;
    return EventStatus::Failed;
  }

  input_snapshot = input_collector.collect(input_mapping);
  return close_requested ? EventStatus::Closed : EventStatus::Ready;
}

auto Wayland::Details::Provider::get_presentation() -> Presentation {
  static const Presentation::Operations operations = {
    .get_instance_extension =
        [](void*) {
          return View::Bytes(
              VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME[0],
              sizeof(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) - 1);
        },

    .create_surface =
        [](void* context, VkInstance instance, PFN_vkGetInstanceProcAddr load,
           VkSurfaceKHR* output) {
          auto& window = provider(context);
          if (!window.display || !window.surface) {
            return VK_ERROR_INITIALIZATION_FAILED;
          }

          constexpr auto function_name = "vkCreateWaylandSurfaceKHR\0"_view;
          auto create = Data::bit_cast<PFN_vkCreateWaylandSurfaceKHR>(
              load(instance, Data::cast<const char>(function_name.get_data())));
          if (!create) {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
          }

          VkWaylandSurfaceCreateInfoKHR info = {
            VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};

          info.display = Data::bit_cast<wl_display*>(window.display);
          info.surface = Data::bit_cast<wl_surface*>(window.surface);
          return create(instance, &info, nullptr, output);
        },
  };

  return Presentation::create(this, operations);
}

#endif
