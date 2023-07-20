#version 430 core
layout(location=0) out vec4 FragColor;
layout(location=1) out float depth;
in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D tex;
void main()
{
    FragColor = texture(tex, TexCoord) * vec4(ourColor, 1.0);
    depth = gl_FragCoord.z;
}