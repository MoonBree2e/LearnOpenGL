#version 450 core

struct PointCloundVboEntry {
	vec4 position;
};
layout(std430, binding = 1) restrict readonly buffer points {
	PointCloundVboEntry vbo[];
};

out Geometry {
	vec4 position_cs;
	float radius;
} geo;

uniform float uOuterOverInnerRadius = 1.0 / 0.5;
uniform float uGrainRadius;
uniform float uGrainInnerRadiusRatio;
uniform float uOccluderMapSpriteScale = 0.2;

uniform uint uFrameCount;
uniform uint uPointCount;
uniform float uTime;
uniform float uFps = 25.0;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;

layout (std140, binding = 0) uniform Camera {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 inverseViewMatrix;
    vec2 resolution;
    float uNear;
    float uFar;
    float uLeft;
    float uRight;
    float uTop;
    float uBottom;
};

//////////////////////////////////////////////////////
// Misc utility functions

const float pi = 3.1415926535897932384626433832795;
const float PI = 3.1415926535897932384626433832795;
const float HALF_SQRT2 = 0.7071067811865476;



bool isOrthographic(mat4 projectionMatrix) {
	return abs(projectionMatrix[3][3]) > 0.01;
}


/**
 * Estimate projection of sphere on screen to determine sprite size (diameter).
 * (TODO: actually, this is the diameter...)
 */
float SpriteSize(float radius, vec4 position_clipspace) {
    if (isOrthographic(projectionMatrix)) {
        float a = projectionMatrix[0][0] * resolution.x;
        float c = projectionMatrix[1][1] * resolution.y;
        return max(a, c) * radius;
    } else {
        return max(resolution.x, resolution.y) * projectionMatrix[1][1] * radius / position_clipspace.w;
    }
}

// better: pointCount is the point count per frame, not the total size of the buffer
uint AnimatedPointId2(uint pointId, uint frameCount, uint pointCount, float time, float fps) {
	uint frame = uint(time * fps) % max(1, frameCount);
	return pointId + pointCount * frame;
}


void main() {
	uint pointId = AnimatedPointId2(gl_VertexID, uFrameCount, uPointCount, uTime, uFps);

	vec4 position_ms = vec4(vbo[pointId].position.xyz, 1.0);
	geo.position_cs = viewModelMatrix * position_ms;
	geo.radius = uGrainRadius * uGrainInnerRadiusRatio; // inner radius
	gl_Position = projectionMatrix * geo.position_cs;
	// The *.15 has no explaination, but it empirically increases occlusion culling
	gl_PointSize = SpriteSize(geo.radius, gl_Position) * uOccluderMapSpriteScale;
}

