#version 460

out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D Tex;

void main() {
    FragColor = texture(Tex, TexCoord);
}