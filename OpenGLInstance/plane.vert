#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectMatrix;

out vec3 ourColor;
out vec2 TexCoord;

void main()
{
    gl_Position = projectMatrix * viewMatrix * modelMatrix * vec4(aPos.xyz, 1.0f);

    ourColor = aColor;
    TexCoord = aTexCoord;
}