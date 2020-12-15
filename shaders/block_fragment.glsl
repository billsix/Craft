#version 330 core


uniform sampler2D sampler;
uniform sampler2D sky_sampler;
uniform float timer;
uniform float daylight;
uniform int ortho;
uniform bool enable_ambient_occlusion;


in VS_OUT {
    vec2 fragment_uv;
    float fragment_ambient_occlusion;
    float fragment_light;
    float fog_factor;
    float fog_height;
    float diffuse;
} fs_in;

out vec4 fragColor;

const float pi = 3.14159265;

void main() {
    vec3 color = vec3(texture(sampler, fs_in.fragment_uv));
    if (color == vec3(1.0, 0.0, 1.0)) {
        discard;
    }
    bool cloud = color == vec3(1.0, 1.0, 1.0);
    if (cloud && bool(ortho)) {
        discard;
    }
    float df = cloud ? 1.0 - fs_in.diffuse * 0.2 : fs_in.diffuse;
    float ambient_occlusion = cloud ? 1.0 - (1.0 - fs_in.fragment_ambient_occlusion) * 0.2 : fs_in.fragment_ambient_occlusion;
    if(enable_ambient_occlusion)
        ambient_occlusion = min(1.0, ambient_occlusion + fs_in.fragment_light);
    else
        ambient_occlusion = 1.0;
    df = min(1.0, df + fs_in.fragment_light);
    float value = min(1.0, daylight + fs_in.fragment_light);
    vec3 light_color = vec3(value * 0.3 + 0.2);
    vec3 ambient = vec3(value * 0.3 + 0.2);
    vec3 light = ambient + light_color * df;
    color = clamp(color * light * ambient_occlusion, vec3(0.0), vec3(1.0));
    vec3 sky_color = vec3(texture(sky_sampler, vec2(timer, fs_in.fog_height)));
    color = mix(color, sky_color, fs_in.fog_factor);
    fragColor = vec4(color, 1.0);
}
