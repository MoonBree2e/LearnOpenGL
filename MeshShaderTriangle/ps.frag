#version 450


// https://www.khronos.org/registry/OpenGL/extensions/NV/NV_fragment_shader_barycentric.txt
#extension GL_NV_fragment_shader_barycentric : enable


layout(location = 0) out vec4 FragColor;


in PerVertexData
{
  vec4 color;
} fragIn;  


void main()
{
  FragColor = fragIn.color;
//  FragColor = vec4(gl_BaryCoordNV, 1.0);
}
    