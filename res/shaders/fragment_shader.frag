#version 460 core

out vec4 frag_color;

in vec2 v_tex_coords;

uniform sampler2D u_texture;
uniform vec3 u_color;

void main() {
   frag_color = texture(u_texture, v_tex_coords) * vec4(u_color, 1.0);
}