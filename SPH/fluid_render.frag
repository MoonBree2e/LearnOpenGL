#version 460 core

out vec4 FragColor;

vec3 baseColor = vec3(0.2, 0.4, 0.8);
uniform vec3 cameraPos;
uniform vec3 lightPos;

uniform mat4 viewMatrix;

void main() {
    //clamps fragments to circle shape. 
    vec2 mapping = gl_PointCoord * 2.0F - 1.0F;
    float d = dot(mapping, mapping);

    if (d >= 1.0F)
    {//discard if the vectors length is more than 0.5
        discard;
    }

    vec2 uv = 2 * gl_PointCoord - vec2(1);
    float r = length(uv);
    vec3 color = baseColor * exp(-10.0 * r);


    vec3 t = cameraPos;
    vec4 tt = vec4(lightPos, 1);
    FragColor = vec4(color, 1.0f) + vec4(t,1) + tt;
}
