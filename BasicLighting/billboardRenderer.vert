#version 460

layout(location = 0) in vec3 pos;
layout(location = 1) in float scaleX;
layout(location = 2) in float scaleY;
layout(location = 3) in mat4 model;

out VS_OUT {
    float scaleX;
    float scaleY;
    mat4 model;
} vs_out;

void main() {
    gl_Position = vec4(pos, 1);
    vs_out.scaleX = scaleX;
    vs_out.scaleY = scaleY;
    vs_out.model = model;
}