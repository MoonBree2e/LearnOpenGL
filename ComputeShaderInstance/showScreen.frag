#version 460 core
out vec4 FragColor;
uniform sampler2D screenTexture;
in vec2 TexCoords;

void main()
{
    FragColor = texture(screenTexture, TexCoords);
}