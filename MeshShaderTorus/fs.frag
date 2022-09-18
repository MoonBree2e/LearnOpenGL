#version 460 core
layout(location=0) out vec4 FragColor;
layout(location=0) in PerVertexData{
    vec4 color;
} fragIn;
void main()
{
    FragColor = fragIn.color;
}