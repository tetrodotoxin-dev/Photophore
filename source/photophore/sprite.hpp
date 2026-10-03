// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/math/size.hpp"
#include "photophore/texture_2d.hpp"
#include "photophore/transform_2d.hpp"

namespace Photophore {

// Sprite stores the values needed to place a textured image. Copying one gives
// the caller independent placement and visibility, while its Texture2D retains
// the pixel matrix it refers to. Sharing a Sprite itself belongs to the owner
// that publishes it.
class Sprite {
 public:
  Sprite() = default;

  EXPORTED(PHOTOPHORE) auto get_texture() const -> const Texture2D&;
  EXPORTED(PHOTOPHORE) auto set_texture(const Texture2D& texture) -> void;
  EXPORTED(PHOTOPHORE) auto get_size() const -> Perimortem::Math::Size<U32, 2>;
  EXPORTED(PHOTOPHORE)
  auto set_size(Perimortem::Math::Size<U32, 2> size) -> void;
  EXPORTED(PHOTOPHORE) auto get_transform() const -> Transform2D;
  EXPORTED(PHOTOPHORE) auto set_transform(Transform2D transform) -> void;
  EXPORTED(PHOTOPHORE) auto is_visible() const -> Bool;
  EXPORTED(PHOTOPHORE) auto set_visible(Bool visible) -> void;
  EXPORTED(PHOTOPHORE) auto get_z_index() const -> S64;
  EXPORTED(PHOTOPHORE) auto set_z_index(S64 z_index) -> void;
  EXPORTED(PHOTOPHORE) auto is_drawable() const -> Bool;

 private:
  Transform2D transform;
  Bool visible = True;
  S64 z_index = 0;
  Texture2D texture;
  Perimortem::Math::Size<U32, 2> size;
};

}  // namespace Photophore
