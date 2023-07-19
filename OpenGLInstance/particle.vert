#version 430 core
layout (location = 0) in vec4 position;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectMatrix;
uniform float pointScale;
uniform float pointSize;
uniform int u_nParticles;

out vec3 eyeSpacePos;
out vec3 f_Color;

uniform vec3 colorRamp[] = vec3[] (vec3(1.0, 0.0, 0.0),
                                   vec3(1.0, 0.5, 0.0),
                                   vec3(1.0, 1.0, 0.0),
                                   vec3(1.0, 0.0, 1.0),
                                   vec3(0.0, 1.0, 0.0),
                                   vec3(0.0, 1.0, 1.0),
                                   vec3(0.0, 0.0, 1.0));

vec3 generateVertexColor()
{

    float segmentSize = float(u_nParticles) / 6.0f;
    float segment     = floor(float(gl_VertexID) / segmentSize);
    float t           = (float(gl_VertexID) - segmentSize * segment) / segmentSize;
    vec3  startVal    = colorRamp[int(segment)];
    vec3  endVal      = colorRamp[int(segment) + 1];
    return mix(startVal, endVal, t);
}

void main(){
	eyeSpacePos = (viewMatrix * modelMatrix * vec4(position.xyz, 1.0f)).xyz;
	f_Color = generateVertexColor();
    gl_PointSize = -pointScale * pointSize / eyeSpacePos.z;
    gl_Position = projectMatrix * viewMatrix * modelMatrix * vec4(position.xyz, 1.0f);
}