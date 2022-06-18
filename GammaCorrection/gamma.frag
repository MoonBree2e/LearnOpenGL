#version 330 core

out vec4 FragColor;

uniform float SCR_WIDTH;
uniform float SCR_HEIGHT;
uniform bool gammaCorrection;
void main()
{
    vec3 f = vec3(gl_FragCoord.x/SCR_WIDTH, gl_FragCoord.x/SCR_WIDTH, gl_FragCoord.x/SCR_WIDTH);
    FragColor = vec4(f, 1.0f);
    if(gammaCorrection)
    {
        float gamma = 2.2;
        FragColor.rgb = pow(FragColor.rgb, vec3(1.0/gamma));
    }
}