// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/perimortem.h"

#ifdef PERI_LINUX

#include <errno.h>
#include <poll.h>
#include <string.h>
#include <wayland-client.h>

#include "photophore/platform/wayland/seat.hpp"
#include "photophore/platform/wayland/xdg_shell.hpp"
#include "photophore/platform/window.hpp"

using namespace Perimortem;
using namespace Photophore::Platform;

// Native protocol ownership stays beside its callbacks. This allocation occurs
// once per window and keeps Wayland object shapes out of the shared header.
struct WaylandWindow {
  explicit WaylandWindow(Input& input) : seat(input) {}
  wl_registry* registry = nullptr;
  wl_compositor* compositor = nullptr;
  Wayland::XdgShell shell;
  Wayland::Seat seat;
};

static auto as_display(void* handle) -> wl_display* {
  return static_cast<wl_display*>(handle);
}

Window::Window(U32 width, U32 height, const char* title) {
  static auto configure = [](Window& window, U32 width, U32 height) {
    if (width != window.logical_width || height != window.logical_height) {
      window.logical_width = width;
      window.logical_height = height;
      window.needs_resize = True;
    }
  };
  initial_width = logical_width = width;
  initial_height = logical_height = height;
  auto* state = new WaylandWindow(input_collector);
  native = state;
  connection = wl_display_connect(nullptr);
  if (!connection) {
    failed = True;
    return;
  }
  state->registry = wl_display_get_registry(as_display(connection));
  if (!state->registry) {
    failed = True;
    return;
  }

  // Registry discovery must finish before the surface can attach its role or
  // devices. Callbacks retain this immovable Window until native teardown.
  static const auto registry_dispatch =
      +[](const void* context, void* target, uint32_t opcode, const wl_message*,
          wl_argument* arguments) -> int {
    enum class Event : uint32_t { Global, GlobalRemove };
    auto* window = static_cast<Window*>(const_cast<void*>(context));
    switch (static_cast<Event>(opcode)) {
    case Event::Global: {
      auto* registry = static_cast<wl_registry*>(target);
      U32 name = arguments[0].u;
      const char* interface = arguments[1].s;
      U32 version = arguments[2].u;
      auto* state = static_cast<WaylandWindow*>(window->native);
      state->seat.register_global(registry, name, interface, version);
      if (!strcmp(interface, wl_compositor_interface.name)) {
        state->compositor = static_cast<wl_compositor*>(wl_registry_bind(
            registry, name, &wl_compositor_interface,
            version < 6 ? version : 6));
      } else if (
          Wayland::XdgShell::recognizes(interface) &&
          !state->shell.bind(registry, name)) {
        window->failed = True;
      }
      return 0;
    }
    case Event::GlobalRemove: {
      U32 name = arguments[0].u;
      static_cast<WaylandWindow*>(window->native)->seat.remove_global(name);
      return 0;
    }
    }
    return -1;
  };
  if (wl_proxy_add_dispatcher(
          reinterpret_cast<wl_proxy*>(state->registry), registry_dispatch, this,
          this) ||
      wl_display_roundtrip(as_display(connection)) < 0 || failed ||
      !state->compositor) {
    failed = True;
    return;
  }
  surface = wl_compositor_create_surface(state->compositor);
  if (!surface) {
    failed = True;
    return;
  }
  static const auto surface_dispatch =
      +[](const void* context, void* target, uint32_t opcode, const wl_message*,
          wl_argument* arguments) -> int {
    enum class Event : uint32_t {
      Enter,
      Leave,
      PreferredBufferScale,
      PreferredBufferTransform
    };
    auto* window = static_cast<Window*>(const_cast<void*>(context));
    switch (static_cast<Event>(opcode)) {
    case Event::PreferredBufferScale: {
      auto* surface = static_cast<wl_surface*>(target);
      S32 factor = arguments[0].i;
      if (factor > 0 && U32(factor) != window->scale) {
        window->scale = U32(factor);
        window->needs_resize = True;
        wl_surface_set_buffer_scale(surface, factor);
      }
      return 0;
    }
    case Event::Enter:
    case Event::Leave:
    case Event::PreferredBufferTransform:
      return 0;
    }
    return -1;
  };
  if (wl_proxy_add_dispatcher(
          static_cast<wl_proxy*>(surface), surface_dispatch, this, this)) {
    failed = True;
    return;
  }
  if (!state->shell.create_toplevel(
          surface, title, "photophore",
          {
            this,
            [](void* data, S32 width, S32 height) {
              auto* window = static_cast<Window*>(data);
              configure(
                  *window, width > 0 ? U32(width) : window->initial_width,
                  height > 0 ? U32(height) : window->initial_height);
            },
            [](void* data) {
              static_cast<Window*>(data)->close_requested = True;
            },
          }) ||
      !state->seat.attach(surface)) {
    failed = True;
    return;
  }
  wl_surface_commit(static_cast<wl_surface*>(surface));
  if (wl_display_roundtrip(as_display(connection)) < 0) {
    failed = True;
  }
}

Window::~Window() {
  auto* state = static_cast<WaylandWindow*>(native);
  if (!state) {
    return;
  }
  // Listeners and shell roles refer to this surface and connection. Remove
  // them first so proxy destruction cannot leave a callback pointing through
  // a window whose native state has already gone away.
  state->seat.destroy();
  state->shell.destroy();
  if (surface) {
    wl_surface_destroy(static_cast<wl_surface*>(surface));
  }
  if (state->compositor) {
    wl_compositor_destroy(state->compositor);
  }
  if (state->registry) {
    wl_registry_destroy(state->registry);
  }
  if (connection) {
    wl_display_disconnect(as_display(connection));
  }
  delete state;
}

auto Window::poll_events() -> Window::EventStatus {
  EventStatus status = get_event_status();
  if (status != EventStatus::Ready) {
    return status;
  }

  wl_display* native_display = as_display(connection);
  if (wl_display_dispatch_pending(native_display) < 0) {
    failed = True;
    return EventStatus::Failed;
  }

  if (close_requested) {
    return EventStatus::Closed;
  }

  // Wayland permits a socket read only after its pending queue is drained.
  // A successful prepare_read reserves that read, so every later path either
  // reads events or cancels the reservation before returning.
  while (wl_display_prepare_read(native_display) != 0) {
    if (wl_display_dispatch_pending(native_display) < 0) {
      failed = True;
      return EventStatus::Failed;
    }

    if (close_requested) {
      return EventStatus::Closed;
    }
  }

  if (wl_display_flush(native_display) < 0 && errno != EAGAIN) {
    wl_display_cancel_read(native_display);
    failed = True;
    return EventStatus::Failed;
  }

  // Polling has a zero timeout because frame observation must return even
  // when no input arrives. Leave the compositor wait to the application's
  // scheduling policy rather than blocking this window's frame loop.
  pollfd display_fd = {
    wl_display_get_fd(native_display),
    POLLIN,
    0,
  };

  auto poll_result = 0;
  do {
    poll_result = poll(&display_fd, 1, 0);
  } while (poll_result < 0 && errno == EINTR);
  if (poll_result < 0) {
    wl_display_cancel_read(native_display);
    failed = True;
    return EventStatus::Failed;
  }

  if (display_fd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
    wl_display_cancel_read(native_display);
    failed = True;
    return EventStatus::Failed;
  }

  if (poll_result > 0 && (display_fd.revents & POLLIN) != 0) {
    int events_read = wl_display_read_events(native_display);
    if (events_read < 0) {
      failed = True;
      return EventStatus::Failed;
    }
  } else {
    wl_display_cancel_read(native_display);
  }

  int events_dispatched = wl_display_dispatch_pending(native_display);
  if (events_dispatched < 0) {
    failed = True;
    return EventStatus::Failed;
  }

  input_snapshot = input_collector.collect(input_mapping);
  return close_requested ? EventStatus::Closed : EventStatus::Ready;
}

auto Window::get_presentation() const -> Presentation {
  return Presentation::create(Presentation::Kind::Wayland, connection, surface);
}

#endif
