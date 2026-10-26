// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/identity/uuid.hpp"

namespace Photophore::Frame {

// Program identifies one authored Description::Program selected by a Batch.
// The description producer assigns its UUID, and Pipelines requires every UUID
// to be set and unique within the supplied inventory. Recording validates the
// matching description before using its shader and layout contracts.
//
// The identifier carries no address or lifetime. Equality selects the same
// inventory entry. Independently supplied descriptions can use different
// identities even when their contents match.
class Program {
 public:
  constexpr Program() = default;
  explicit constexpr Program(Perimortem::Core::Identity::Uuid identity)
      : identity(identity) {}

  constexpr auto is_valid() const -> Bool { return identity.is_set(); }
  constexpr auto get_identity() const -> Perimortem::Core::Identity::Uuid {
    return identity;
  }

  constexpr auto operator==(const Program& rhs) const -> Bool {
    return identity == rhs.identity;
  }

 private:
  Perimortem::Core::Identity::Uuid identity;
};

}  // namespace Photophore::Frame
