#version 460 core
layout(location = 0) in vec3 _Position;
layout(location = 1) in vec3 _Normal;
layout(location = 2) in vec2 _TexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = view * model * vec4(_Position, 1.0f);
    FragPos = worldPos.xyz;
    gl_Position = projection * worldPos;
    TexCoords = _TexCoords;

    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
    Normal = normalMatrix * _Normal;
}