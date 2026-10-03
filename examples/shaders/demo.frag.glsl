// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#version 450

layout(location = 0) in vec2 texture_uv;
layout(location = 0) out vec4 output_color;
layout(set = 0, binding = 0) uniform sampler2D image;
layout(set = 1, binding = 0) uniform sampler2D overlay;

layout(push_constant) uniform DrawInputs {
  vec4 transform_x;
  vec4 transform_y;
  vec4 tone;
} draw;

void main() {
  vec4 base_color = texture(image, texture_uv);
  vec4 overlay_color = texture(overlay, vec2(1.0 - texture_uv.x, texture_uv.y));
  output_color = mix(base_color, overlay_color, 0.35) * draw.tone;
}
