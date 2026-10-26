// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/storage/option.hpp"
#include "perimortem/core/view/bytes.hpp"

#include "photophore/platform/collector.hpp"
#include "photophore/platform/presentation.hpp"

namespace Photophore::Platform {

// Window owns one native top level surface and publishes its size, input
// Snapshots and Presentation. Only `create` publishes a Window. Some contains
// a usable native surface, while None reports a rejected title or extent,
// unavailable host service or native creation failure.
//
// `poll_events` advances native callbacks and replaces the stored input
// observation. EventStatus distinguishes normal operation, a user close
// request and a platform failure that occurs after creation.
//
// Dimensions and pointer positions share client coordinates. Multiplying by
// scale gives framebuffer dimensions. Wayland supplies an integer buffer
// scale, while Win32 uses physical client pixels with scale one. A size or
// scale change sets needs_resize until the caller handles it and calls
// `clear_resize`.
//
// Window is move only. Its host Provider has a stable address, so moving the
// Window transfers one typed provider pointer without changing native callback
// contexts or Presentation. The moved from state participates only in
// destruction or a later move assignment. A Renderer made from a Window's
// Presentation ends before that Window. Creation, moves, event polling and
// destruction occur on one thread.
class Window {
 public:
  // EventStatus keeps an ordinary compositor close separate from a platform
  // failure so an application can preserve its process outcome.
  enum class EventStatus : U8 {
    Ready,
    Closed,
    Failed,
  };

  // Provider is the typed internal boundary implemented by each supported
  // host. Its stable allocation owns the common window state and supplies the
  // host event and presentation operations. Native callbacks use the concrete
  // provider that registered them as their context.
  //
  // Window creates, moves and destroys providers inside the Photophore
  // runtime. Consumers receive Window rather than a Provider subclass.
  class Provider {
   public:
    Provider(Provider&&) = delete;
    auto operator=(Provider&&) -> Provider& = delete;
    Provider(const Provider&) = delete;
    auto operator=(const Provider&) -> Provider& = delete;

    virtual auto poll_events() -> EventStatus = 0;
    virtual auto get_presentation() -> Presentation = 0;
    virtual auto destroy() -> void = 0;

    auto get_event_status() const -> EventStatus;
    auto get_logical_width() const -> U32;
    auto get_logical_height() const -> U32;
    auto get_scale() const -> U32;
    auto get_needs_resize() const -> Bool;
    auto clear_resize() -> void;
    auto get_input() const -> const Photophore::Input::Snapshot&;
    auto get_input_mapping() -> Photophore::Input::Mapping&;
    auto get_input_mapping() const -> const Photophore::Input::Mapping&;

   protected:
    Provider() = default;
    ~Provider() = default;

    Collector input_collector;
    Photophore::Input::Mapping input_mapping;
    Photophore::Input::Snapshot input_snapshot;
    U32 logical_width = 0;
    U32 logical_height = 0;
    U32 initial_width = 0;
    U32 initial_height = 0;
    U32 scale = 1;
    Bool close_requested = False;
    Bool failed = False;
    Bool needs_resize = False;
  };

  // `title` contains UTF-8 bytes without an embedded null. Creation copies or
  // converts those bytes before returning.
  static EXPORTED(PHOTOPHORE) auto
      create(U32 width, U32 height, Perimortem::Core::View::Bytes title)
          -> Perimortem::Core::Storage::Option<Window>;

  EXPORTED(PHOTOPHORE) ~Window();
  EXPORTED(PHOTOPHORE) Window(Window&& source) noexcept;
  EXPORTED(PHOTOPHORE) auto operator=(Window&& source) noexcept -> Window&;
  Window(const Window&) = delete;
  auto operator=(const Window&) -> Window& = delete;

  EXPORTED(PHOTOPHORE) auto poll_events() -> EventStatus;
  EXPORTED(PHOTOPHORE) auto get_event_status() const -> EventStatus;

  EXPORTED(PHOTOPHORE) auto get_logical_width() const -> U32;
  EXPORTED(PHOTOPHORE) auto get_logical_height() const -> U32;
  EXPORTED(PHOTOPHORE) auto get_scale() const -> U32;
  EXPORTED(PHOTOPHORE) auto get_needs_resize() const -> Bool;
  EXPORTED(PHOTOPHORE) auto clear_resize() -> void;

  // A completed poll replaces the stored input Snapshot. `get_input` returns a
  // reference that remains valid through the next poll, while copying it
  // preserves that observation. Mapping changes apply to the next Snapshot.
  EXPORTED(PHOTOPHORE) auto get_input() const
      -> const Photophore::Input::Snapshot&;
  EXPORTED(PHOTOPHORE) auto get_input_mapping() -> Photophore::Input::Mapping&;
  EXPORTED(PHOTOPHORE) auto get_input_mapping() const
      -> const Photophore::Input::Mapping&;

  EXPORTED(PHOTOPHORE) auto get_presentation() const -> Presentation;

 private:
  // Explicit construction prevents an arbitrary Provider reference from
  // silently transferring its destruction obligation into a Window.
  explicit Window(Provider& provider) : provider(&provider) {}

  Provider* provider = nullptr;
};

}  // namespace Photophore::Platform
