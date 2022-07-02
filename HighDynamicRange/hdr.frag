#version 460 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D hdrBuffer;
uniform float exposure;
uniform bool hdr;

vec3 ReinhardColorMapping(vec3 hdrColor)
{
    vec3 mapped = hdrColor / (hdrColor + vec3(1.f));
    return mapped;
}

vec3 exposureColorMapping(vec3 hdrColor)
{
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    return mapped;
}

void main()
{
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;

    if(hdr)
    {
        vec3 mapped = exposureColorMapping(hdrColor);
    
        mapped = pow(mapped, vec3(1.0 / gamma));

        FragColor = vec4(mapped, 1.f);
    }
    else{
        vec3 result = pow(hdrColor, vec3(1.0 / gamma));
        FragColor = vec4(result, 1.0);
    }
}