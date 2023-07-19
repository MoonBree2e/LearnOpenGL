#version 430 core

uniform float pointSize;
uniform mat4 projectMatrix;

in vec3 eyeSpacePos;
in vec3 f_Color;

out vec4 fragColor;
void main(){	
	vec3 normal;

	normal.xy = gl_PointCoord.xy * vec2(2.0, -2.0) + vec2(-1.0,1.0);
	float mag = dot(normal.xy, normal.xy);
	if(mag > 1.0) discard;
	normal.z = sqrt(1.0 - mag);

	//calculate lighting

	fragColor = vec4(f_Color, 1.0);
	vec4 pixelEyePos = vec4(eyeSpacePos + vec3(normal.x, normal.y, -normal.z) * pointSize, 1.0f);
	vec4 pixelClipPos = projectMatrix * pixelEyePos;
	gl_FragDepth = pixelClipPos.z / pixelClipPos.w;
}