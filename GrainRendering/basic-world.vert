// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\basic-world.vert.glsl
#version 330 core

layout (location = 0) in vec3 position;

out vec3 direction;

// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/uniform/camera.inc.glsl

layout (std140) uniform Camera {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 inverseViewMatrix;
    vec2 resolution;
    float uNear;
    float uFar;
    float uLeft;
    float uRight;
    float uTop;
    float uBottom;
};
// _AUGEN_END_INCLUDE

void main() {
	vec4 pos = projectionMatrix * mat4(mat3(viewMatrix)) * vec4(position, 0.001f);
    gl_Position = pos.xyww;
    direction = position;
}
// _AUGEN_END_INCLUDE
