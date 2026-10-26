// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/toolchain.h"

#ifdef TOOLCHAIN_LINUX

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "toolchain/validation/test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/view/null_terminated.hpp"

#include "photophore/platform/window.hpp"

using namespace Photophore::Platform;
using namespace Toolchain::Validation;
using namespace Perimortem::Core;
using namespace Photophore::Input;

static Harness WaylandDispatch = {.name = "Photophore::Wayland::Dispatch"};

// A private protocol peer supplies real wire messages to the shipped client
// library. No compositor, display device or Vulkan driver participates. Testing
// through the socket catches incorrect argument positions that calling our
// dispatcher functions directly would leave untested.
struct ProtocolPeer {
  enum Object {
    Display,
    Registry,
    Compositor,
    Shell,
    Surface,
    XdgSurface,
    Toplevel,
    Seat,
    Keyboard,
    Pointer,
    Count
  };

  U32 objects[Count] = {1};
  S32 socket = -1;
  S32 keymap = -1;
  pthread_t thread;
  Bool started = false;
  Bool okay = true;
  Bool published = false;
  Bool pong = false;
  Bool acknowledged = false;
  Bool committed = false;
  U8 completed = false;

  ~ProtocolPeer() {
    if (socket >= 0) {
      shutdown(socket, SHUT_RDWR);
    }

    if (started) {
      pthread_join(thread, nullptr);
    }

    if (socket >= 0) {
      close(socket);
    }

    if (keymap >= 0) {
      close(keymap);
    }
  }
};

static auto send_packet(ProtocolPeer& peer, const U32* packet, CppSize size)
    -> void {
  peer.okay &= send(peer.socket, packet, size, MSG_NOSIGNAL) == ssize_t(size);
}

template <class... Words>
static auto event(
    ProtocolPeer& peer,
    ProtocolPeer::Object object,
    U32 opcode,
    Words... words) -> void {
  U32 packet[] = {
    peer.objects[object],
    U32((sizeof...(Words) * 4 + 8) << 16) | opcode,
    U32(words)...,
  };

  send_packet(peer, packet, sizeof(packet));
}

static auto
    global(ProtocolPeer& peer, U32 name, const char* interface, U32 version)
        -> void {
  U32 packet[32] = {};
  CppSize length = strlen(interface) + 1;
  CppSize words = (length + 3) / 4;
  packet[0] = peer.objects[ProtocolPeer::Registry];
  packet[1] = U32((5 + words) * 4) << 16;
  packet[2] = name;
  packet[3] = U32(length);
  memcpy(packet + 4, interface, length);
  packet[4 + words] = version;
  send_packet(peer, packet, (5 + words) * 4);
}

static auto publish_input(ProtocolPeer& peer) -> void {
  using P = ProtocolPeer;
  if (peer.published || !peer.objects[P::Keyboard] ||
      !peer.objects[P::Pointer] || !peer.objects[P::Surface] ||
      !peer.objects[P::Toplevel]) {
    return;
  }

  peer.published = true;
  event(peer, P::Shell, 0, 77);
  event(peer, P::Toplevel, 0, 400, 300, 0);
  event(peer, P::XdgSurface, 0, 42);
  event(peer, P::Surface, 2, 2);

  // File descriptors travel as socket ancillary data rather than inline words.
  // The read end reaches EOF only after the receiver releases its delivered FD.
  U32 keymap[] = {peer.objects[P::Keyboard], 16u << 16, 1, 0};
  iovec bytes = {keymap, sizeof(keymap)};
  alignas(cmsghdr) char control[CMSG_SPACE(sizeof(S32))] = {};

  msghdr message = {};
  message.msg_iov = &bytes;
  message.msg_iovlen = 1;
  message.msg_control = control;
  message.msg_controllen = sizeof(control);

  auto* descriptor = CMSG_FIRSTHDR(&message);
  descriptor->cmsg_level = SOL_SOCKET;
  descriptor->cmsg_type = SCM_RIGHTS;
  descriptor->cmsg_len = CMSG_LEN(sizeof(S32));
  memcpy(CMSG_DATA(descriptor), &peer.keymap, sizeof(S32));
  peer.okay &= sendmsg(peer.socket, &message, MSG_NOSIGNAL) == sizeof(keymap);
  close(peer.keymap);
  peer.keymap = -1;

  event(peer, P::Keyboard, 1, 101, peer.objects[P::Surface], 4, 30);
  event(peer, P::Keyboard, 3, 102, 5, 48, 1);
  event(peer, P::Pointer, 0, 103, peer.objects[P::Surface], 320, -640);
  event(peer, P::Pointer, 2, 6, 2304, -1792);
  event(peer, P::Pointer, 3, 104, 7, 272, 1);
  event(peer, P::Pointer, 4, 8, 0, 768);
  event(peer, P::Pointer, 5);
}

static auto serve(void* context) -> void* {
  auto& peer = *Data::cast<ProtocolPeer>(context);
  using P = ProtocolPeer;
  U32 packet[128];
  while (recv(peer.socket, packet, 8, MSG_WAITALL) == 8) {
    CppSize size = packet[1] >> 16;
    if (size < 8 || size > sizeof(packet) ||
        (size > 8 && recv(peer.socket, packet + 2, size - 8, MSG_WAITALL) !=
                         ssize_t(size - 8))) {
      peer.okay = false;
      break;
    }

    U32 object = packet[0];
    U32 opcode = packet[1] & 0xffff;
    if (object == 1 && opcode == 1) {
      peer.objects[P::Registry] = packet[2];
      global(peer, 1, "wl_compositor", 6);
      global(peer, 2, "xdg_wm_base", 1);
      global(peer, 3, "wl_seat", 9);
    } else if (object == 1 && opcode == 0) {
      U32 done[] = {packet[2], 12u << 16, 1};
      send_packet(peer, done, sizeof(done));
      event(peer, P::Display, 1, packet[2]);
    } else if (object == peer.objects[P::Registry] && opcode == 0) {
      U32 id = packet[5 + (packet[3] + 3) / 4];
      switch (packet[2]) {
      case 1:
        peer.objects[P::Compositor] = id;
        break;
      case 2:
        peer.objects[P::Shell] = id;
        break;
      case 3:
        peer.objects[P::Seat] = id;
        event(peer, P::Seat, 0, 3);
        break;
      }
    } else if (object == peer.objects[P::Compositor] && opcode == 0) {
      peer.objects[P::Surface] = packet[2];
    } else if (object == peer.objects[P::Shell] && opcode == 2) {
      peer.objects[P::XdgSurface] = packet[2];
    } else if (object == peer.objects[P::Shell] && opcode == 3) {
      peer.pong = packet[2] == 77;
    } else if (object == peer.objects[P::XdgSurface] && opcode == 1) {
      peer.objects[P::Toplevel] = packet[2];
    } else if (object == peer.objects[P::XdgSurface] && opcode == 4) {
      peer.acknowledged = packet[2] == 42;
    } else if (object == peer.objects[P::Surface] && opcode == 6) {
      peer.committed |= peer.acknowledged;
      if (peer.pong && peer.committed) {
        __atomic_store_n(&peer.completed, true, __ATOMIC_RELEASE);
      }
    } else if (object == peer.objects[P::Seat] && opcode <= 1) {
      peer.objects[opcode == 0 ? P::Pointer : P::Keyboard] = packet[2];
    }

    publish_input(peer);
  }

  shutdown(peer.socket, SHUT_RDWR);
  return nullptr;
}

VALIDATION_TEST(WaylandDispatch, protocol_events) {
  ProtocolPeer peer;
  S32 sockets[2];
  ASSERT(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
  peer.socket = sockets[1];

  S32 pipe_fds[2];
  ASSERT(pipe2(pipe_fds, O_NONBLOCK | O_CLOEXEC) == 0);
  peer.keymap = pipe_fds[1];

  timeval timeout = {2, 0};
  setsockopt(peer.socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  ASSERT(pthread_create(&peer.thread, nullptr, serve, &peer) == 0);
  peer.started = true;

  const char* previous = getenv("WAYLAND_SOCKET");
  char saved[64] = {};
  Bool restore_previous = Bool(previous != nullptr);
  if (restore_previous) {
    const auto previous_value =
        View::NullTerminated(previous, sizeof(saved) - 1).get_view();
    ASSERT(previous_value.get_size() < sizeof(saved) - 1);
    for (Count index = 0; index < previous_value.get_size(); index++) {
      saved[index] = char(previous_value[index]);
    }
  }

  char descriptor[32];
  snprintf(descriptor, sizeof(descriptor), "%d", sockets[0]);
  setenv("WAYLAND_SOCKET", descriptor, 1);
  {
    Window window(96, 64, "Private protocol check");
    if (restore_previous) {
      setenv("WAYLAND_SOCKET", saved, 1);
    } else {
      unsetenv("WAYLAND_SOCKET");
    }

    using Key = Key;
    Bool observed = false;
    for (U32 attempt = 0; attempt < 1000 && !observed; ++attempt) {
      window.poll_events();
      observed = window.get_input().get_pointer().scroll_y == 3;
      if (!observed) {
        usleep(1000);
      }
    }

    EXPECT(observed);
    EXPECT_EQ(window.get_logical_width(), 400u);
    EXPECT_EQ(window.get_logical_height(), 300u);
    EXPECT_EQ(window.get_scale(), 2u);
    EXPECT(window.get_input().is_current(Key::A));
    EXPECT(window.get_input().is_current(Key::B));
    EXPECT(window.get_input().is_current(Key::MousePrimary));
    EXPECT_EQ(window.get_input().get_pointer().x, 9.0f);
    EXPECT_EQ(window.get_input().get_pointer().y, -7.0f);
    char byte;
    EXPECT(read(pipe_fds[0], &byte, 1) == 0);
    close(pipe_fds[0]);

    // Flush replies before checking peer counters. Completion is published
    // after the acknowledgement and its following commit reach the peer.
    window.poll_events();
    for (U32 attempt = 0;
         attempt < 1000 && !__atomic_load_n(&peer.completed, __ATOMIC_ACQUIRE);
         ++attempt) {
      usleep(1000);
    }

    shutdown(peer.socket, SHUT_RD);
    pthread_join(peer.thread, nullptr);
    peer.started = false;
    EXPECT(peer.okay);
    EXPECT(peer.pong);
    EXPECT(peer.acknowledged);
    EXPECT(peer.committed);
  }
}

#endif
