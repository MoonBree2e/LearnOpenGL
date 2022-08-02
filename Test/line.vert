#version 460 core

layout(location = 0) in vec3 aPos;

uniform mat4 viewProj;
out gl_PerVertex {
    vec4 gl_Position;
};
void main(){
    gl_Position = viewProj * vec4(aPos.x, aPos.y, aPos.z, 1.0);
}