#version 460 core
layout(location = 0) in vec3 _Pos;

uniform mat4 model;
layout (std140, binding = 0) uniform uniforms_t
{ 
  mat4 ViewProjectionMatrix;
  mat4 ModelMatrix;
} transform_ub;

layout (location = 0) out PerVertexData
{
  vec4 color;
} v_out; 
void main()
{
    gl_Position = transform_ub.ViewProjectionMatrix * transform_ub.ModelMatrix * model * vec4(_Pos, 1.f);

    v_out.color = vec4(1.0, 0.7, 0.0, 1.0);
}