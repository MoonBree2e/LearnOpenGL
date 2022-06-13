#version 330 core
layout(location=0) in vec3 _Pos;
layout(location=1) in vec3 _Normal;
layout(location=2) in vec2 _TexCoord;

out VS_OUT {
    vec3 normal;
} vs_out;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;

void main()
{
    mat3 normalMatrix = mat3(transpose(inverse(view * model)));
    vs_out.normal = normalize(vec3(projection * vec4(normalMatrix * _Normal,0.0)));
    gl_Position = projection * view * model * vec4(_Pos,1.f);
}