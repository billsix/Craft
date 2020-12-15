#version 330 core

uniform mat4 matrix;
uniform vec3 camera;
uniform float fog_distance;
uniform int ortho;

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 uv;


out VS_OUT {
    vec2 fragment_uv;
    float fragment_ambient_occlusion;
    float fragment_light;
    float fog_factor;
    float fog_height;
    float diffuse;
} vs_out;


const float pi = 3.14159265;
const vec3 light_direction = normalize(vec3(-1.0, 1.0, -1.0));

void main() {
    gl_Position = matrix * position;
    vs_out.fragment_uv = uv.xy;
    vs_out.fragment_ambient_occlusion = 0.3 + (1.0 - uv.z) * 0.7;
    vs_out.fragment_light = uv.w;
    vs_out.diffuse = max(0.0, dot(normal, light_direction));
    if (bool(ortho)) {
        vs_out.fog_factor = 0.0;
        vs_out.fog_height = 0.0;
    }
    else {
        float camera_distance = distance(camera, vec3(position));
        vs_out.fog_factor = pow(clamp(camera_distance / fog_distance, 0.0, 1.0), 4.0);
        float dy = position.y - camera.y;
        float dx = distance(position.xz, camera.xz);
        vs_out.fog_height = (atan(dy, dx) + pi / 2) / pi;
    }
}
