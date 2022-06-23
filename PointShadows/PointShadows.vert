#version 430 core
layout(location=0) in vec3 _Position;
layout(location=1) in vec3 _Normal;
layout(location=2) in vec2 _TexCoords;


out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform bool reverse_normals;

void main()
{
    gl_Position = projection * view * model * vec4(_Position, 1.f);
    vs_out.FragPos = vec3(model * vec4(_Position, 1.f));
    if(reverse_normals) // a slight hack to make sure the outer large cube displays lighting from the 'inside' instead of the default 'outside'.
        vs_out.Normal = transpose(inverse(mat3(model))) * (-1.0 * _Normal);
    else
        vs_out.Normal = transpose(inverse(mat3(model))) * _Normal;    
    vs_out.TexCoords = _TexCoords;
}