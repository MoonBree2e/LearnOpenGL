#version 430 core

uniform float pointSize;
uniform mat4 projectMatrix;

in vec3 eyeSpacePos;
in vec3 f_Color;

out vec4 fragColor;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} dirLight;

void initLight(){
    dirLight.direction = vec3(0, 1, 0);
    dirLight.ambient = vec3(0.3f);
    dirLight.diffuse  = vec3(0.55f);
    dirLight.specular = vec3(0.15f);
}

void main(){	
	initLight();

	vec3 normal;

	normal.xy = gl_PointCoord.xy * vec2(2.0, -2.0) + vec2(-1.0,1.0);
	float mag = dot(normal.xy, normal.xy);
	if(mag > 1.0) discard;
	normal.z = sqrt(1.0 - mag);

	//calculate lighting

	fragColor = vec4(f_Color, 1.0);

    vec3 ambient = dirLight.ambient * fragColor.xyz;
    // diffuse
    vec3 lightDir = dirLight.direction;
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * dirLight.diffuse * fragColor.xyz;

	fragColor.xyz = ambient + diffuse;
	// gamma correction.
	const float gamma = 2.2f;
	fragColor.rgb = pow(fragColor.rgb,vec3(1.0f/gamma));

	vec4 pixelEyePos = vec4(eyeSpacePos + vec3(normal.x, normal.y, -normal.z) * pointSize, 1.0f);
	vec4 pixelClipPos = projectMatrix * pixelEyePos;
	gl_FragDepth = pixelClipPos.z / pixelClipPos.w;
}