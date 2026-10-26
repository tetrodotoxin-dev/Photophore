// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/math/size.hpp"
#include "perimortem/core/storage/option.hpp"

#include "photophore/image/texture_2d.hpp"
#include "photophore/scene/transform_2d.hpp"

namespace Photophore::Scene {

// Sprite is an editable application value for an optional textured image. It
// groups that explicit Option with pixel size, Transform2D, visibility and z
// index for Frame::Batch collection. Copying a Sprite creates independent
// placement and visibility values while a selected Texture2D keeps its Pixels2D
// content accessible.
class Sprite {
 public:
  Sprite() = default;

  EXPORTED(PHOTOPHORE) auto try_texture() const
      -> Perimortem::Core::Storage::Option<const Image::Texture2D&>;
  EXPORTED(PHOTOPHORE) auto set_texture(const Image::Texture2D& texture)
      -> void;
  EXPORTED(PHOTOPHORE) auto get_size() const
      -> Perimortem::Core::Math::Size<U32, 2>;
  EXPORTED(PHOTOPHORE) auto set_size(Perimortem::Core::Math::Size<U32, 2> size)
      -> void;
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
  Perimortem::Core::Storage::Option<Image::Texture2D> texture;
  Perimortem::Core::Math::Size<U32, 2> size;
};

}  // namespace Photophore::Scene
