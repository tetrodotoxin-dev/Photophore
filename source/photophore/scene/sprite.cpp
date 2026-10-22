// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/scene/sprite.hpp"

// Sprite accessors keep the editable application model independent of frame
// construction. Texture assignment retains shared Matrix identity while the
// remaining properties are copied placement and ordering values.
auto Photophore::Scene::Sprite::get_texture() const -> const Image::Texture2D& {
  return texture;
}

auto Photophore::Scene::Sprite::set_texture(const Image::Texture2D& texture)
    -> void {
  this->texture = texture;
}

auto Photophore::Scene::Sprite::get_size() const
    -> Perimortem::Math::Size<U32, 2> {
  return size;
}

auto Photophore::Scene::Sprite::set_size(Perimortem::Math::Size<U32, 2> size)
    -> void {
  this->size = size;
}

auto Photophore::Scene::Sprite::get_transform() const -> Transform2D {
  return transform;
}

auto Photophore::Scene::Sprite::set_transform(Transform2D transform) -> void {
  this->transform = transform;
}

auto Photophore::Scene::Sprite::is_visible() const -> Bool {
  return visible;
}

auto Photophore::Scene::Sprite::set_visible(Bool visible) -> void {
  this->visible = visible;
}

auto Photophore::Scene::Sprite::get_z_index() const -> S64 {
  return z_index;
}

auto Photophore::Scene::Sprite::set_z_index(S64 z_index) -> void {
  this->z_index = z_index;
}

auto Photophore::Scene::Sprite::is_drawable() const -> Bool {
  // Collection needs one visible area backed by uploadable pixels. Keeping the
  // predicate on Sprite gives callers the same admission rule for every frame.
  return visible && !size.is_empty() && texture.is_drawable();
}
