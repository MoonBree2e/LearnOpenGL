#version 450 core

layout (location = 0) in vec3 position;

out vec3 direction;

layout (std140, binding = 0) uniform Camera {
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

void main() {
	vec4 pos = projectionMatrix * mat4(mat3(viewMatrix)) * vec4(position, 0.1);
    gl_Position = pos;
    direction = position;
}