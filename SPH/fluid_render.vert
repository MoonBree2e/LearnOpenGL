#version 460 core
layout(location = 0) in vec3 _Position;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance;
};

uniform mat4 viewProjection;

void main() {
    gl_Position = viewProjection * vec4(_Position.xyz, 1.0);
    gl_PointSize = 50.0;
}

