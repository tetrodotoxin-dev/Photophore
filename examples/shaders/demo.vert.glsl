// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texture_coordinate;
layout(location = 0) out vec2 texture_uv;

layout(push_constant) uniform DrawInputs {
  vec4 transform_x;
  vec4 transform_y;
  vec4 tone;
} draw;

void main() {
  vec2 pixel = vec2(
      dot(draw.transform_x.xy, position) + draw.transform_x.z,
      dot(draw.transform_y.xy, position) + draw.transform_y.z);
  vec2 clip = vec2(
      pixel.x / draw.transform_x.w * 2.0 - 1.0,
      pixel.y / draw.transform_y.w * 2.0 - 1.0);
  gl_Position = vec4(clip, 0.0, 1.0);
  texture_uv = texture_coordinate;
}
