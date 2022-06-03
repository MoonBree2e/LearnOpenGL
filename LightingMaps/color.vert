#version 330 core
layout(location = 0) in vec3 _Pos;
layout(location = 1) in vec3 _Normal;
layout(location = 2) in vec2 _TexCoords;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

void main()
{
    gl_Position = projection * view * model * vec4(_Pos, 1.f);
    FragPos = vec3(model*vec4(_Pos, .1f));
    Normal = mat3(transpose(inverse(model))) * _Normal;
    TexCoords = _TexCoords;
}