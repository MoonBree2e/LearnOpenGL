#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 ourColor;
void main()
{
    vec3 Temp = aPos;
    Temp.y = - Temp.y;
    gl_Position = vec4(Temp, 1.0);
    // ourColor = aColor;
    ourColor = gl_Position.xyz;

}