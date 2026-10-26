// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "toolchain/toolchain.hpp"

namespace Photophore::Frame {

// Pipeline is the fixed draw state paired with a Frame::Program. Pipelines keys
// each VkPipeline by both values, keeping compiled shader identity and fixed
// draw state independently selectable. The current profile is a unit quad
// rendered as a triangle list with source alpha blending.
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
