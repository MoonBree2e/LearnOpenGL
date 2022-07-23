#version 460 core

out vec4 FragColor;

uniform vec3 baseColor;

void main() {
    vec2 uv = 2 * gl_PointCoord - vec2(1);
    float r = length(uv);
    vec3 color = baseColor * exp(-10.0 * r);
    FragColor = vec4(color, 1.0f);
}
