// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\far-grain.vert.glsl
#version 450 core
// _AUGEN_BEGIN_INCLUDE sys:defines
#define PASS_DEPTH
#define PROCEDURAL_BASECOLOR
#define SHELL_CULLING
// _AUGEN_END_INCLUDE sys:defines

layout (location = 0) in vec4 position;

struct PointCloundVboEntry {
    vec4 position;
};
layout(std430, binding = 0) restrict readonly buffer pointsSsbo {
    PointCloundVboEntry pointVertexAttributes[];
};
layout (std430, binding = 1) restrict readonly buffer pointElementsSsbo {
    uint pointElements[];
};

out VertexData {
	vec3 position_ws;
	vec3 originalPosition_ws; // for procedural color
	float radius;
	uint vertexId;
} outData;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;

uniform float uGrainRadius = 0.005;

uniform uint uFrameCount;
uniform uint uPointCount;
uniform float uFps = 25.0;
uniform float uTime;
uniform bool uUseAnimation = true;

uniform bool uUsePointElements = true;

// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/anim.inc.glsl

// depreciated
uint AnimatedPointId(uint pointId, uint frameCount, uint pointCount, float time, float fps) {
	uint pointCountPerFrame = pointCount / frameCount;
	uint frame = uint(time * fps) % max(1, frameCount);
	return pointId + pointCountPerFrame * frame;
}

// better: pointCount is the point count per frame, not the total size of the buffer
uint AnimatedPointId2(uint pointId, uint frameCount, uint pointCount, float time, float fps) {
	uint frame = uint(time * fps) % max(1, frameCount);
	return pointId + pointCount * frame;
}

// _AUGEN_END_INCLUDE

void main() {
    uint pointId =
    	uUsePointElements
    	? pointElements[gl_VertexID]
    	: gl_VertexID;

    uint animPointId =
        uUseAnimation
        ? AnimatedPointId2(pointId, uFrameCount, uPointCount, uTime, uFps)
        : pointId;

	vec3 p =
		uUsePointElements
		? pointVertexAttributes[animPointId].position.xyz
		: position.xyz;

#ifdef PROCEDURAL_ANIM0
	float t = uTime * 0.;
	p *= 1 + sin(t * 2.0 + p.y * 2.0) * 0.1 * sin(atan(p.x, p.z) * 10.0);
#endif // PROCEDURAL_ANIM0

	outData.radius = uGrainRadius;
	outData.position_ws = (modelMatrix * vec4(p, 1.0)).xyz;
	outData.originalPosition_ws = (modelMatrix * vec4(p, 1.0)).xyz;
	outData.vertexId = pointId;
	outData.vertexId = animPointId%20; // WTF?
}