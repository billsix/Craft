#version 330 core

uniform sampler2D sampler;
uniform float timer;

in VS_OUT {
    vec2 fragment_uv;
} fs_in;

out vec4 fragColor;

void main() {
    vec2 uv = vec2(timer, fs_in.fragment_uv.t);
    fragColor = texture2D(sampler, uv);
}
