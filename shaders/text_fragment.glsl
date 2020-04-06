#version 330 core

uniform sampler2D sampler;
uniform bool is_sign;

in VS_OUT {
    vec2 fragment_uv;
} fs_in;

out vec4 fragColor;

void main() {
    vec4 color = texture2D(sampler, fs_in.fragment_uv);
    if (is_sign) {
        if (color == vec4(1.0)) {
            discard;
        }
    }
    else {
        color.a = max(color.a, 0.4);
    }
    fragColor = color;
}
