// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\impostor-grain.vert.glsl
#version 430 core
// _AUGEN_BEGIN_INCLUDE sys:defines
#define PRECOMPUTE_IN_VERTEX
#define PROCEDURAL_BASECOLOR
// _AUGEN_END_INCLUDE sys:defines

#pragma varopt PASS_BLIT_TO_MAIN_FBO

///////////////////////////////////////////////////////////////////////////////
#ifdef PASS_BLIT_TO_MAIN_FBO

// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/standard-posteffect.vert.inc.glsl
// just a regular post effect

layout (location = 0) in vec4 position;

out vec2 vertUv;
out flat int vertLayer;

void main() {
	gl_Position = vec4(position.xyz, 1.0);
	vertUv = position.xy * .5 + .5;
	vertLayer = gl_InstanceID;
}
// _AUGEN_END_INCLUDE

///////////////////////////////////////////////////////////////////////////////
#else // PASS_BLIT_TO_MAIN_FBO

out VertexData {
	uint id;
} vert;

// All the vertex shader is in the geometry shader to allow for whole poitn discard
void main() {
	vert.id = gl_VertexID;
}

///////////////////////////////////////////////////////////////////////////////
#endif // PASS
// _AUGEN_END_INCLUDE
