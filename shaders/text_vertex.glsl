#version 330 core

uniform mat4 matrix;

layout (location = 0) in vec4 position;
layout (location = 1) in vec2 uv;



out VS_OUT {
    vec2 fragment_uv;
} vs_out;


void main() {
    gl_Position = matrix * position;
    vs_out.fragment_uv = uv;
}
