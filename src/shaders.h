const unsigned char block_vertex_glsl[] = "#version 120 \n\
\n\
uniform mat4 matrix; \n\
uniform vec3 camera;\n\
uniform float fog_distance; \n\
uniform int ortho;\n\
\n\
attribute vec4 position;\n\
attribute vec3 normal;\n\
attribute vec4 uv;\n\
\n\
varying vec2 fragment_uv;\n\
varying float fragment_ao;\n\
varying float fragment_light;\n\
varying float fog_factor;\n\
varying float fog_height;\n\
varying float diffuse;\n\
\n\
const float pi = 3.14159265;\n\
const vec3 light_direction = normalize(vec3(-1.0, 1.0, -1.0));\n\
\n\
void main() {\n\
    gl_Position = matrix * position;\n\
    fragment_uv = uv.xy;\n\
    fragment_ao = 0.3 + (1.0 - uv.z) * 0.7;\n\
    fragment_light = uv.w;\n\
    diffuse = max(0.0, dot(normal, light_direction));\n\
    if (bool(ortho)) {\n\
        fog_factor = 0.0;\n\
        fog_height = 0.0;\n\
    }\n\
    else {\n\
        float camera_distance = distance(camera, vec3(position));\n\
        fog_factor = pow(clamp(camera_distance / fog_distance, 0.0, 1.0), 4.0);\n\
        float dy = position.y - camera.y;\n\
        float dx = distance(position.xz, camera.xz);\n\
        fog_height = (atan(dy, dx) + pi / 2) / pi;\n\
    }\n\
}\n\
";
const unsigned char block_fragment_glsl[] = "#version 120\n\
\n\
uniform sampler2D sampler;\n\
uniform sampler2D sky_sampler;\n\
uniform float timer;\n\
uniform float daylight;\n\
uniform int ortho;\n\
\n\
varying vec2 fragment_uv;\n\
varying float fragment_ao;\n\
varying float fragment_light;\n\
varying float fog_factor;\n\
varying float fog_height;\n\
varying float diffuse;\n\
\n\
const float pi = 3.14159265;\n\
\n\
void main() {\n\
    vec3 color = vec3(texture2D(sampler, fragment_uv));\n\
    if (color == vec3(1.0, 0.0, 1.0)) {\n\
        discard;\n\
    }\n\
    bool cloud = color == vec3(1.0, 1.0, 1.0);\n\
    if (cloud && bool(ortho)) {\n\
        discard;\n\
    }\n\
    float df = cloud ? 1.0 - diffuse * 0.2 : diffuse;\n\
    float ao = cloud ? 1.0 - (1.0 - fragment_ao) * 0.2 : fragment_ao;\n\
    ao = min(1.0, ao + fragment_light);\n\
    df = min(1.0, df + fragment_light);\n\
    float value = min(1.0, daylight + fragment_light);\n\
    vec3 light_color = vec3(value * 0.3 + 0.2);\n\
    vec3 ambient = vec3(value * 0.3 + 0.2);\n\
    vec3 light = ambient + light_color * df;\n\
    color = clamp(color * light * ao, vec3(0.0), vec3(1.0));\n\
    vec3 sky_color = vec3(texture2D(sky_sampler, vec2(timer, fog_height)));\n\
    color = mix(color, sky_color, fog_factor);\n\
    gl_FragColor = vec4(color, 1.0);\n\
}\n\
";
const unsigned char line_vertex_glsl[] = "#version 120\n\
\n\
uniform mat4 matrix;\n\
\n\
attribute vec4 position;\n\
\n\
void main() {\n\
    gl_Position = matrix * position;\n\
}\n\
";

const unsigned char line_fragment_glsl[] = "#version 120\n\
\n\
void main() {\n\
    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n\
}\n\
";

const unsigned char sky_vertex_glsl[] = "#version 120\n\
\n\
uniform mat4 matrix;\n\
\n\
attribute vec4 position;\n\
attribute vec3 normal;\n\
attribute vec2 uv;\n\
\n\
varying vec2 fragment_uv;\n\
\n\
void main() {\n\
    gl_Position = matrix * position;\n\
    fragment_uv = uv;\n\
}\n\
";

const unsigned char sky_fragment_glsl[] = "#version 120\n\
\n\
uniform sampler2D sampler;\n\
uniform float timer;\n\
\n\
varying vec2 fragment_uv;\n\
\n\
void main() {\n\
    vec2 uv = vec2(timer, fragment_uv.t);\n\
    gl_FragColor = texture2D(sampler, uv);\n\
}\n\
";
const unsigned char text_vertex_glsl[] = "#version 120\n\
\n\
uniform mat4 matrix;\n\
\n\
attribute vec4 position;\n\
attribute vec2 uv;\n\
\n\
varying vec2 fragment_uv;\n\
\n\
void main() {\n\
    gl_Position = matrix * position;\n\
    fragment_uv = uv;\n\
}\n\
";
const unsigned char text_fragment_glsl[] = "#version 120\n\
\n\
uniform sampler2D sampler;\n\
uniform bool is_sign;\n\
\n\
varying vec2 fragment_uv;\n\
\n\
void main() {\n\
    vec4 color = texture2D(sampler, fragment_uv);\n\
    if (is_sign) {\n\
        if (color == vec4(1.0)) {\n\
            discard;\n\
        }\n\
    }\n\
    else {\n\
        color.a = max(color.a, 0.4);\n\
    }\n\
    gl_FragColor = color;\n\
}\n\
";
