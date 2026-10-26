// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/scene/sprite.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Scene;

// Sprite accessors keep the editable application model independent of frame
// construction. Texture assignment retains shared Matrix identity while the
// remaining properties are copied placement and ordering values.
auto Sprite::get_texture() const -> const Image::Texture2D& {
  return texture;
}

auto Sprite::set_texture(const Image::Texture2D& texture) -> void {
  this->texture = texture;
}

auto Sprite::get_size() const -> Math::Size<U32, 2> {
  return size;
}

auto Sprite::set_size(Math::Size<U32, 2> size) -> void {
  this->size = size;
}

auto Sprite::get_transform() const -> Transform2D {
  return transform;
}

auto Sprite::set_transform(Transform2D transform) -> void {
  this->transform = transform;
}

auto Sprite::is_visible() const -> Bool {
  return visible;
}

auto Sprite::set_visible(Bool visible) -> void {
  this->visible = visible;
}

auto Sprite::get_z_index() const -> S64 {
  return z_index;
}

auto Sprite::set_z_index(S64 z_index) -> void {
  this->z_index = z_index;
}

auto Sprite::is_drawable() const -> Bool {
  // Frame collection requires a visible nonempty area and a Texture2D with at
  // least one pixel. The predicate combines those independent authored values
  // before a caller creates a Batch.
  return visible && !size.is_empty() && texture.is_drawable();
}
