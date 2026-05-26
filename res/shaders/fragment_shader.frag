#version 460 core
out vec4 frag_color;
in vec2 v_tex_coords;

void main() {
   frag_color = vec4(v_tex_coords, 0.0, 1.0);
}