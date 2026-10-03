// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Frame {

// A compiled Program can be drawn with different fixed state. Pipeline keeps
// that selection beside the batch so Pipelines can key each Vulkan realization
// by both the program locator and this value. The current choices describe a
// unit quad drawn as a triangle list with alpha blending.
class Pipeline {
 public:
  enum class Topology : U8 {
    TriangleList,
  };

  enum class Blend : U8 {
    Alpha,
  };

  enum class Geometry : U8 {
    UnitQuad2D,
  };

  constexpr Pipeline() = default;

  constexpr Pipeline(Topology topology, Blend blend, Geometry geometry)
      : topology(topology), blend(blend), geometry(geometry) {}

  constexpr auto get_topology() const -> Topology { return topology; }
  constexpr auto get_blend() const -> Blend { return blend; }
  constexpr auto get_geometry() const -> Geometry { return geometry; }

  constexpr auto operator==(const Pipeline& rhs) const -> Bool {
    return topology == rhs.topology && blend == rhs.blend &&
           geometry == rhs.geometry;
  }

 private:
  Topology topology = Topology::TriangleList;
  Blend blend = Blend::Alpha;
  Geometry geometry = Geometry::UnitQuad2D;
};

}  // namespace Photophore::Frame
