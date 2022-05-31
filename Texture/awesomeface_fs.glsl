#version 330 core

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform float mixCofficient;
in vec2 TexCoord;

void main()
{
    vec2 NewCoord = vec2(1 - TexCoord.x, TexCoord.y);
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, NewCoord), mixCofficient);
}
    