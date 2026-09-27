// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore {

// Generated adapters use Projection to collect a batch from a concrete
// instance layout. The program token selects a compiled description, and the
// parameter offset and size select bytes within that instance. Resource entries
// select either a texture supplied by the host or a Texture2D subobject at an
// instance offset, in the order expected by the program's descriptors.
//
// This metadata and its resource array have process lifetime. The adapter must
// use them only with the instance layout they describe, because offsets alone
// cannot prove that an arbitrary object has that layout. TTX agreement belongs
// at the interface that admits the instance, before this native projection.
struct Projection {
  enum class ResourceSource : Count {
    HostTexture,
    InstanceTexture,
  };

  struct Resource {
    ResourceSource source;
    Count offset;
  };

  const U8* program;
  Count parameters_offset;
  Count parameters_size;
  const Resource* resources;
  Count resource_count;
};

static_assert(__is_trivial(Projection::Resource));
static_assert(__is_trivial(Projection));

}  // namespace Photophore
