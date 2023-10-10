#version 460

layout (points) in;
layout (triangle_strip) out;
layout (max_vertices = 4) out;

out vec2 TexCoord;//输出的纹理坐标

uniform vec3 gCameraPos;//相机位置

uniform mat4 view;
uniform mat4 projection;
uniform vec3 UpAxis = vec3(0, 1, 0);

in VS_OUT {
    float scaleX;
    float scaleY;
    mat4 model;
} gs_in[];

mat4 BillboardMatrix(vec3 centerPosition, vec3 worldCameraPos, vec3 worldUp)
{
    vec3 look = normalize(centerPosition - worldCameraPos);
    vec3 right = normalize(cross( look, worldUp ));
    vec3 up = normalize(cross(right,look));

    mat4 result;
    result[0] = vec4( right, 0.0 );
    result[1] = vec4( worldUp, 0.0 );
    result[2] = vec4( look, 0.0 );
    result[3] = vec4( 0.0, 0.0, 0.0, 1.0 );
    return result;
}

mat4 TranslateMatrix(float x, float y, float z){
    mat4 result = mat4(1);
    result[3] = vec4(x, y, z, 1);
    return result;
}

void main() {
    vec3 Pos = gl_in[0].gl_Position.xyz;

    float halfScaleX = gs_in[0].scaleX * 0.5;
    float halfScaleY = gs_in[0].scaleY * 0.5;

    vec4 t;
    mat4 b = BillboardMatrix(Pos, gCameraPos, UpAxis);
    mat4 vp = projection * view;

    t = vec4(-halfScaleX, -halfScaleY, 0, 0);
    gl_Position = vp * (vec4(Pos, 1.0) + b * gs_in[0].model * t);
    TexCoord = vec2(0.0, 0.0);
    EmitVertex();

    t = vec4(-halfScaleX, halfScaleY, 0, 0);
    gl_Position = vp * (vec4(Pos, 1.0) + b * gs_in[0].model * t);
    TexCoord = vec2(0.0, 1.0);
    EmitVertex();

    t = vec4(halfScaleX, -halfScaleY, 0, 0);
    gl_Position = vp * (vec4(Pos, 1.0) + b * gs_in[0].model * t);
    TexCoord = vec2(1.0, 0.0);
    EmitVertex();

    t = vec4(halfScaleX, halfScaleY, 0, 0);
    gl_Position = vp * (vec4(Pos, 1.0) + b * gs_in[0].model * t);
    TexCoord = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}