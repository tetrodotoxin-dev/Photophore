// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/sprite.hpp"

using namespace Perimortem;

auto Photophore::Sprite::get_texture() const -> const Texture2D& {
  return texture;
}

auto Photophore::Sprite::set_texture(const Texture2D& texture) -> void {
  this->texture = texture;
}

auto Photophore::Sprite::get_material() const
    -> const Perimortem::Core::Implementation& {
  return material;
}

auto Photophore::Sprite::set_material(
    const Perimortem::Core::Implementation& material) -> void {
  this->material = material;
}

auto Photophore::Sprite::get_size() const -> Perimortem::Graphics::Size2D {
  return size;
}

auto Photophore::Sprite::set_size(Perimortem::Graphics::Size2D size) -> void {
  this->size = size;
}

auto Photophore::Sprite::get_transform() const -> Transform2D {
  return transform;
}

auto Photophore::Sprite::set_transform(Transform2D transform) -> void {
  this->transform = transform;
}

auto Photophore::Sprite::is_visible() const -> Bool {
  return visible;
}

auto Photophore::Sprite::set_visible(Bool visible) -> void {
  this->visible = visible;
}

auto Photophore::Sprite::get_z_index() const -> S64 {
  return z_index;
}

auto Photophore::Sprite::set_z_index(S64 z_index) -> void {
  this->z_index = z_index;
}

auto Photophore::Sprite::is_drawable() const -> Bool {
  return visible && size.width != 0 && size.height != 0 &&
         texture.is_drawable() && material.is_valid();
}
