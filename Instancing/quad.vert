#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aOffset;

out vec3 fColor;

uniform vec2 offsets[100];

void main()
{
//    vec2 offset = offsets[gl_InstanceID];
    vec2 offset = aOffset;
    vec2 pos = aPos * (gl_InstanceID / 100.0);
    gl_Position = vec4(pos + offset, 0.0, 1.0);
    fColor = aColor;
}