// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\deferred-shader.frag.glsl
#version 450 core
// _AUGEN_BEGIN_INCLUDE sys:defines
// _AUGEN_END_INCLUDE sys:defines

#pragma variant SHOW_NORMAL SHOW_BASECOLOR SHOW_POSITION SHOW_RAW_BUFFER1 SHOW_ROUGHNESS
#pragma variant HDR REINHART
#pragma opt WHITE_BACKGROUND
#pragma opt TRANSPARENT_BACKGROUND
#pragma opt OLD_BRDF
#pragma opt DEBUG_VECTORS

#define HDR
#define WHITE_BACKGROUND

// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/uniform/camera.inc.glsl

layout (std140) uniform Camera {
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
// _AUGEN_END_INCLUDE
// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/utils.inc.glsl
//////////////////////////////////////////////////////
// Misc utility functions

const float pi = 3.1415926535897932384626433832795;
const float PI = 3.1415926535897932384626433832795;
const float HALF_SQRT2 = 0.7071067811865476;

vec3 color2normal(in vec4 color) {
    return normalize(vec3(color) * 2.0 - vec3(1.0));
}

vec4 normal2color(in vec3 normal, in float alpha) {
    return vec4(normal * 0.5 + vec3(0.5), alpha);
}

bool isOrthographic(mat4 projectionMatrix) {
	return abs(projectionMatrix[3][3]) > 0.01;
}

vec3 cameraPosition(mat4 viewMatrix) {
	return transpose(mat3(viewMatrix)) * vec3(viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2]);
}

float getNearDistance(mat4 projectionMatrix) {
	return isOrthographic(projectionMatrix)
		? (projectionMatrix[3][2] - 1) / projectionMatrix[2][2]
		: projectionMatrix[3][2] / (projectionMatrix[2][2] - 1);
}

float getFarDistance(mat4 projectionMatrix) {
	return isOrthographic(projectionMatrix)
		? (projectionMatrix[3][2] - 1) / projectionMatrix[2][2]
		: projectionMatrix[3][2] / (projectionMatrix[2][2] + 1);
}
// _AUGEN_END_INCLUDE

in vec2 uv_coords;

layout (location = 0) out vec4 out_color;

#define IN_GBUFFER
// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/gbuffer2.inc.glsl
//////////////////////////////////////////////////////
// G-Buffer related functions
//
// define either OUT_GBUFFER or IN_GBUFFER before including this file,
// to respectivaly write or read to gbuffer.

struct GFragment {
	vec3 baseColor;
	vec3 normal;
	vec3 ws_coord;
	uint material_id;
	float roughness;
	float metallic;
	vec3 emission;
	float alpha;
	uint count;

	// Used for G-impostors but not in gbuffer (converted to regular normal)
	vec4 lean1;
	vec4 lean2;

};

//////////////////////////////////////////////////////
// Reserved material IDs

const uint noMaterial = 0;
const uint skyboxMaterial = 1;
const uint forwardAlbedoMaterial = 2;
const uint forwardBaseColorMaterial = 3;
const uint forwardNormalMaterial = 4;
const uint pbrMaterial = 5;
const uint pbrMetallicRoughnessMaterial = 6;
const uint iridescentMaterial = 7;
const uint worldMaterial = 8;
const uint iblMaterial = 9;
const uint farCloudTestMaterial = 10;
const uint colormapDebugMaterial = 11;
const uint accumulatedPbrMaterial = 12; // must always be last, cause with accumulation it is actually more than this value

//////////////////////////////////////////////////////
// GFragment methods

void initGFragment(out GFragment fragment) {
    fragment.baseColor = vec3(0.0);
    fragment.material_id = pbrMaterial;
    fragment.normal = vec3(0.0);
    fragment.ws_coord = vec3(0.0);
    fragment.roughness = 0.0;
    fragment.metallic = 0.0;
    fragment.emission = vec3(0.0);
    fragment.alpha = 0.0;
    fragment.count = 0;
    fragment.lean1 = vec4(0.0);
    fragment.lean2 = vec4(0.0);
}

/**
 * Apply mix to all components of a GFragment
 */
GFragment LerpGFragment(GFragment ga, GFragment gb, float t) {
	GFragment g;
	g.baseColor = mix(ga.baseColor, gb.baseColor, t);
	g.ws_coord = mix(ga.ws_coord, gb.ws_coord, t);
	g.metallic = mix(ga.metallic, gb.metallic, t);
	g.roughness = mix(ga.roughness, gb.roughness, t);
	g.emission = mix(ga.emission, gb.emission, t);
	g.alpha = mix(ga.alpha, gb.alpha, t);
	g.lean1 = mix(ga.lean1, gb.lean1, t);
	g.lean2 = mix(ga.lean2, gb.lean2, t);
	g.material_id = ga.material_id; // cannot be interpolated

	// Normal interpolation
	float wa = (1 - t) * ga.alpha;
	float wb = t * gb.alpha;

	// Avoid trouble with NaNs in normals
	if (wa == 0) ga.normal = vec3(0.0);
	if (wb == 0) gb.normal = vec3(0.0);

	g.normal = normalize(wa * ga.normal + wb * gb.normal);

	// Naive interpolation
	//g.normal = normalize(mix(ga.normal, gb.normal, t));
	return g;
}

void unpackGFragment(
	in sampler2D gbuffer0,
	in usampler2D gbuffer1,
	in usampler2D gbuffer2,
	in ivec2 coords, 
	out GFragment fragment)
{
	vec4 data1 = texelFetch(gbuffer0, coords, 0);
	uvec4 data2 = texelFetch(gbuffer1, coords, 0);
	uvec4 data3 = texelFetch(gbuffer2, coords, 0);
	vec2 tmp;

	tmp = unpackHalf2x16(data2.y);
	fragment.baseColor = vec3(unpackHalf2x16(data2.x), tmp.x);
	fragment.normal = normalize(vec3(tmp.y, unpackHalf2x16(data2.z)));
	fragment.ws_coord = data1.xyz;
	fragment.material_id = data2.w;
	fragment.roughness = data1.w;

	tmp = unpackHalf2x16(data3.y);
	fragment.lean2.xyz = vec3(unpackHalf2x16(data3.x), tmp.x);
	fragment.metallic = tmp.y;
	fragment.count = data3.w;
}

void packGFragment(
	in GFragment fragment,
	out vec4 gbuffer_color0,
	out uvec4 gbuffer_color1,
	out uvec4 gbuffer_color2)
{
	gbuffer_color0 = vec4(fragment.ws_coord, fragment.roughness);
	gbuffer_color1 = uvec4(
		packHalf2x16(fragment.baseColor.xy),
		packHalf2x16(vec2(fragment.baseColor.z, fragment.normal.x)),
		packHalf2x16(fragment.normal.yz),
		fragment.material_id
	);
	gbuffer_color2 = uvec4(
		packHalf2x16(fragment.lean2.xy),
		packHalf2x16(vec2(fragment.lean2.z, fragment.metallic)),
		packHalf2x16(vec2(0.0, 0.0)),
		fragment.count
	);
}

/**
 * Linear packing of a g-fragment does not hold as much information as regular packing,
 * but it is compatible with additive rendering.
 */
void unpackLinearGFragment(
	in sampler2D lgbuffer0,
	in sampler2D lgbuffer1,
	in ivec2 coords, 
	out GFragment fragment)
{
	vec4 data0 = texelFetch(lgbuffer0, coords, 0);
	vec4 data1 = texelFetch(lgbuffer1, coords, 0);
	fragment.baseColor.rgb = data0.rgb;
	fragment.alpha = data0.a;
	fragment.normal.xyz = data1.rgb;
	fragment.roughness = data1.a;
}

/**
 * Linear packing of a g-fragment does not hold as much information as regular packing,
 * but it is compatible with additive rendering.
 */
void packLinearGFragment(
	in GFragment fragment,
	out vec4 lgbuffer_color0,
	out vec4 lgbuffer_color1)
{
	lgbuffer_color0 = vec4(fragment.baseColor.rgb, fragment.alpha);
	lgbuffer_color1 = vec4(fragment.normal.xyz, fragment.roughness);
}

/**
 * Linear packing of a g-fragment does not hold as much information as regular packing,
 * but it is compatible with additive rendering.
 */
void unpackLeanLinearGFragment(
	in sampler2D lgbuffer0,
	in sampler2D lgbuffer1,
	in sampler2D lgbuffer2,
	in ivec2 coords, 
	out GFragment fragment)
{
	vec4 data0 = texelFetch(lgbuffer0, coords, 0);
	vec4 data1 = texelFetch(lgbuffer1, coords, 0);
	vec4 data2 = texelFetch(lgbuffer2, coords, 0);
	fragment.baseColor.rgb = data0.rgb;
	fragment.alpha = data0.a;
	fragment.lean1.xyz = data1.rgb;
	fragment.lean2.x = data1.a;
	fragment.lean2.yz = data2.rg;
	fragment.roughness = data2.b;
	fragment.metallic = data2.a;
}

/**
 * Linear packing of a g-fragment does not hold as much information as regular packing,
 * but it is compatible with additive rendering.
 */
void packLeanLinearGFragment(
	in GFragment fragment,
	out vec4 lgbuffer_color0,
	out vec4 lgbuffer_color1,
	out vec4 lgbuffer_color2)
{
	lgbuffer_color0 = vec4(fragment.baseColor.rgb, fragment.alpha);
	lgbuffer_color1 = vec4(fragment.lean1.xyz, fragment.lean2.x);
	lgbuffer_color2 = vec4(fragment.lean2.yz, fragment.roughness, fragment.metallic);
}

/////////////////////////////////////////////////////////////////////
#ifdef OUT_GBUFFER

layout (location = 0) out vec4 gbuffer_color0;
layout (location = 1) out uvec4 gbuffer_color1;
layout (location = 2) out uvec4 gbuffer_color2;

void autoPackGFragment(in GFragment fragment) {
	packGFragment(fragment, gbuffer_color0, gbuffer_color1, gbuffer_color2);
}

#endif // OUT_GBUFFER


/////////////////////////////////////////////////////////////////////
#ifdef IN_GBUFFER

layout (binding = 0) uniform sampler2D gbuffer0;
layout (binding = 1) uniform usampler2D gbuffer1;
layout (binding = 2) uniform usampler2D gbuffer2;

void autoUnpackGFragment(inout GFragment fragment) {
	unpackGFragment(gbuffer0, gbuffer1, gbuffer2, ivec2(gl_FragCoord.xy), fragment);
}

void autoUnpackGFragmentWithOffset(inout GFragment fragment, vec2 offset) {
	unpackGFragment(gbuffer0, gbuffer1, gbuffer2, ivec2(gl_FragCoord.xy - offset), fragment);
}

#endif // IN_GBUFFER

/////////////////////////////////////////////////////////////////////
#ifdef OUT_LINEAR_GBUFFER

layout (location = 0) out vec4 lgbuffer_color0;
layout (location = 1) out vec4 lgbuffer_color1;

void autoPackGFragment(in GFragment fragment) {
	packLinearGFragment(fragment, lgbuffer_color0, lgbuffer_color1);
}

#endif // OUT_LINEAR_GBUFFER


/////////////////////////////////////////////////////////////////////
#ifdef IN_LINEAR_GBUFFER

layout (binding = 0) uniform sampler2D lgbuffer0;
layout (binding = 1) uniform sampler2D lgbuffer1;

void autoUnpackGFragment(inout GFragment fragment) {
	unpackLinearGFragment(lgbuffer0, lgbuffer1, ivec2(gl_FragCoord.xy), fragment);
}

#endif // IN_LINEAR_GBUFFER

/////////////////////////////////////////////////////////////////////
#ifdef OUT_LEAN_LINEAR_GBUFFER

layout (location = 0) out vec4 lgbuffer_color0;
layout (location = 1) out vec4 lgbuffer_color1;
layout (location = 2) out vec4 lgbuffer_color2;

void autoPackGFragment(in GFragment fragment) {
	packLeanLinearGFragment(fragment, lgbuffer_color0, lgbuffer_color1, lgbuffer_color2);
}

#endif // OUT_LEAN_LINEAR_GBUFFER

/////////////////////////////////////////////////////////////////////
#ifdef IN_LEAN_LINEAR_GBUFFER

layout (binding = 0) uniform sampler2D lgbuffer0;
layout (binding = 1) uniform sampler2D lgbuffer1;
layout (binding = 2) uniform sampler2D lgbuffer2;

void autoUnpackGFragment(inout GFragment fragment) {
	unpackLeanLinearGFragment(lgbuffer0, lgbuffer1, lgbuffer2, ivec2(gl_FragCoord.xy), fragment);
}

#endif // IN_LEAN_LINEAR_GBUFFER
// _AUGEN_END_INCLUDE

layout (binding = 7) uniform sampler2D in_depth;
layout (binding = 8) uniform samplerCubeArray filteredCubemaps;

uniform sampler2D iblBsdfLut;
uniform sampler3D iridescenceLut;

uniform float uTime;

uniform sampler2D uColormap;
uniform bool uHasColormap = false;

// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/light.inc.glsl
//////////////////////////////////////////////////////
// Light related functions

struct PointLight {
	sampler2D shadowMap;
	sampler2D richShadowMap;
	vec3 position_ws;
	vec3 color;
	int isRich;
	int hasShadowMap;
	mat4 matrix;
};


float shadowBiasFromNormal(const in PointLight light, const in vec3 normal) {
	if (light.isRich == 1) {
		return 0.0;
	} else {
		return max(0.00005 * (1.0 - dot(normal, vec3(1.0, 0.0, 0.0))), 0.00001);
	}
}


vec4 richLightTest(const in PointLight light, vec3 position_ws, vec3 position_cs, float shadowBias) {
	vec4 shadowCoord = light.matrix * vec4(position_ws, 1.0);
	shadowCoord = shadowCoord / shadowCoord.w * 0.5 + 0.5;

	float shadowLimitDepth = texture(light.shadowMap, shadowCoord.xy).r;
	//shadow += d < shadowCoord.z - shadowBias ? 1.0 : 0.0;

	vec2 s = vec2(textureSize(light.richShadowMap, 0));
	vec2 roundedShadowCoord = round(shadowCoord.xy * s) / s;

	vec3 shadowLimitTexelCenter = vec3(roundedShadowCoord, shadowLimitDepth);

	vec3 normal = normalize(texture(light.richShadowMap, shadowCoord.xy).xyz);
	if (normal.z > 0) {
		//normal = -normal;  // point toward camera
	}

	vec2 dp = (shadowCoord.xy - roundedShadowCoord.xy) * shadowCoord.w;
	// need to get the proj mat w/o the light view mat
	vec2 dv = (inverse(light.matrix) * vec4(dp, 0.0, shadowCoord.w)).xy;
	//vec2 grad = vec2(dFdx(position_cs.x), dFdy(position_cs.y));
	//vec2 dv = (shadowCoord.xy - roundedShadowCoord.xy) * grad;
	//return vec4(grad * 400.0, 0.0, 1.0);

	float d0 = texture(light.shadowMap, roundedShadowCoord).r;
	float d = d0 + dot(dv, normal.xy);

	//return vec4(normal.xy * 0.5 + 0.5, 0.0, 0.0);

	shadowBias = 0.0;
	return vec4((d - shadowCoord.z) * 1000.0 + 0.5);
	return vec4(d > shadowCoord.z + 0.00001 ? 1.0 : 0.0);

	//return vec4(dv * s.x + 0.5, 1.0);

	//return vec4(fac * 0.5 + 0.5);
	//return fac <= 0.01 ? 0.0 : 1.0;
}


float richShadowAt(const in PointLight light, vec3 position_ws, float shadowBias) {
	vec4 shadowCoord = light.matrix * vec4(position_ws, 1.0);
	shadowCoord = shadowCoord / shadowCoord.w * 0.5 + 0.5;

	float shadowLimitDepth = texture(light.shadowMap, shadowCoord.xy).r;
	//shadow += d < shadowCoord.z - shadowBias ? 1.0 : 0.0;

	vec2 s = vec2(textureSize(light.richShadowMap, 0));
	vec2 roundedShadowCoord = round(shadowCoord.xy * s) / s;

	vec3 shadowLimitTexelCenter = vec3(roundedShadowCoord, shadowLimitDepth);

	vec3 normal = normalize(texture(light.richShadowMap, shadowCoord.xy).xyz);
	if (normal.z > 0) {
		normal = -normal;  // point toward camera
	}

	float fac = dot(normalize(vec3(shadowCoord) - shadowLimitTexelCenter), normal);
	return fac;
	return fac <= 0.01 ? 0.0 : 1.0;
}


float shadowAt(const in PointLight light, vec3 position_ws, float shadowBias) {
	if (light.hasShadowMap == 0) {
		return 0.0;
	}

	if (light.isRich == 1) {
		return richShadowAt(light, position_ws, shadowBias);
	}

	float shadow = 0.0;
	vec4 shadowCoord = light.matrix * vec4(position_ws, 1.0);
	shadowCoord = shadowCoord / shadowCoord.w;
	if (abs(shadowCoord.x) >= 1.0 || abs(shadowCoord.y) >= 1.0) {
		return 1.0;
	}
	shadowCoord = shadowCoord * 0.5 + 0.5;

	// PCF
	vec2 texelSize = 1.0 / textureSize(light.shadowMap, 0);
	vec2 dcoord;
	for(int x = -1; x <= 1; ++x) {
		for(int y = -1; y <= 1; ++y) {
			dcoord = vec2(x, y) * texelSize;
			float d = texture(light.shadowMap, shadowCoord.xy + dcoord).r;
			shadow += d < shadowCoord.z - shadowBias ? 1.0 : 0.0;
		}
	}

	return shadow / 9.0;
}
// _AUGEN_END_INCLUDE

uniform PointLight light[3];
uniform float lightPowerScale = 1.0;

// Global switches
uniform bool uIsShadowMapEnabled = true;
uniform bool uTransparentFilm = false;
uniform int uShadingMode = 0; // 0: BEAUTY, 1: NORMAL, 2: BASE_COLOR

// Accumulated samples info display
uniform float uMaxSampleCount = 40;
uniform bool uShowSampleCount = false;

uniform float uShadowMapBias;
uniform vec2 uBlitOffset = vec2(0.0);

uniform float uDebugVectorsScale = 0.1;
uniform float uDebugVectorsGrid = 20;

#ifdef OLD_BRDF
// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/bsdf-old.inc.glsl
//////////////////////////////////////////////////////
// Microfacet distributions and utility functions

struct SurfaceAttributes {
    vec3 baseColor;
    float metallic;
    float roughness;
    float reflectance;
    vec3 emissive;
    float occlusion;
#ifdef ANISOTROPIC
    float anisotropy;
#endif
};

float DistributionGGX(vec3 N, vec3 H, float a) {
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = pi * denom * denom;
    
    return a2 / denom;
}


float GeometrySchlickGGX(float NdotV, float k) {
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}


float GeometrySmith(float NdotV, float NdotL, float k) {
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
    
    return ggx1 * ggx2;
}


vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


vec3 bsdf(in vec3 toCam, in vec3 toLight, in vec3 n, in vec3 albedo, in float Kd, in float Ks, in float a) {
    vec3 color = vec3(0.f);

    vec3 Wo = toCam;
    vec3 Wi = toLight;
    float NdotWo = max(dot(n, Wo), 0.0);
    float NdotWi = max(dot(n, Wi), 0.0);

    float Kdirect = (a + 1) * (a + 1) / 8;
    vec3 F0 = mix(vec3(0.04), albedo, a);

    float cosTheta = dot(n, Wi);
    if (cosTheta <= 0 || dot(n, Wo) <= 0) {
        return color;
    }
    vec3 h = normalize(Wo + Wi);
    vec3 L = vec3(3.0);
    vec3 F = fresnelSchlick(dot(n, Wo), F0);
    float D = DistributionGGX(n, h, a);
    float G = GeometrySmith(NdotWo, NdotWi, Kdirect);
    vec3 specular = D * F * G / (4.f * dot(n, Wo) * cosTheta);
    vec3 diffuse = albedo / pi;
    color += (Kd * diffuse + Ks * specular) * L * cosTheta;

    return color;
}


/*
vec3 bsdfPbr(in vec3 Wo, in vec3 n, in vec3 albedo, in float a) {
    float NdotWo = max(dot(n, Wo), 0.0);
    int minSteps = 400;
    vec3 sum = vec3(0.0f);
    float dW  = 1.0f / minSteps;
    float Kdirect = (a + 1) * (a + 1) / 8;
    float Ks = 1.f;
    float Kd = 1.f;
    vec3 F0 = mix(vec3(0.04), albedo, a);
    for(int i = 0, j = 0; i < maxSteps && j < minSteps; ++i) 
    {
        vec3 Wi = sphere[i];
        float cosTheta = dot(n, Wi);
        if (cosTheta <= 0) {
            // Cheaper to discard than to compute matrix product
            continue;
        }
        vec3 h = normalize(Wo + Wi);

        vec3 L = texture(cubemap, transpose(normalMatrix) * Wi).rgb;
        vec3 F = fresnelSchlick(NdotWo, F0);
        float D = DistributionGGX(n, h, a);
        float G = GeometrySmith(NdotWo, cosTheta, Kdirect);
        vec3 specular = D * F * G / (4.f * NdotWo * cosTheta);
        vec3 diffuse = albedo / pi;
        sum += (Kd * diffuse + Ks * specular) * L * cosTheta * dW;
        ++j;
    }
    return sum;
}
*/



vec3 bsdfPbrMetallicRoughness(in vec3 toCam, in vec3 toLight, in vec3 n, in vec3 baseColor, in float roughness, in float metallic) {
    vec3 h = normalize(toCam + toLight);
    float NdotWo = dot(n, toCam);
    float NdotWi = dot(n, toLight);

    float k = (roughness + 1) * (roughness + 1) / 8;
    vec3 F0 = mix(vec3(0.04), baseColor, metallic);

    if (NdotWi <= 0 || NdotWo <= 0) {
        return vec3(0.0);
    }

    vec3 F = fresnelSchlick(NdotWo, F0);
    float D = DistributionGGX(n, h, roughness);
    float G = GeometrySmith(NdotWo, NdotWi, k);
    vec3 specular = baseColor * D * F * G / (4.f * NdotWo * NdotWi);
    vec3 diffuse = baseColor / pi;

    return (metallic * specular + (1.0 - metallic) * diffuse) * NdotWi;
}



/*
int invRoughnessLut(float roughness) {
    if (roughness <= 0.01f) {
        return 0;
    } else if (roughness <= 0.05f) {
        return 1;
    } else if (roughness <= 0.1f) {
        return 2;
    } else if (roughness <= 0.5f) {
        return 3;
    } else if (roughness <= 0.8f) {
        return 4;
    } else if (roughness <= 0.9f) {
        return 5;
    } else if (roughness <= 0.95f) {
        return 6;
    } else if (roughness <= 1.f) {
        return 7;
    } else {
        return 0;
    }
}
*/

int invRoughnessLut(float roughness) {
    if (roughness <= 0.01f) {
        return 0;
    } else if (roughness <= 0.1f) {
        return 1;
    } else if (roughness <= 0.8f) {
        return 2;
    } else {
        return 0;
    }
}


vec3 ApproximateSpecularIBL(vec3 specularColor , float roughness, vec3 N, vec3 V, sampler2D bsdfLut, samplerCubeArray filteredCubemaps) {
    float NoV = clamp(dot(N, V), 0.0, 1.0);
    vec3 R = reflect(-V, N);
    float l = float(invRoughnessLut(roughness));
    vec3 prefilteredColor = texture(filteredCubemaps, vec4(R, l)).rgb;
    vec2 envBrdf = texture(bsdfLut, vec2(roughness, NoV)).rg;
    return prefilteredColor * (specularColor * envBrdf.x + envBrdf.y);
}

// _AUGEN_END_INCLUDE
#else
// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/bsdf.inc.glsl
//////////////////////////////////////////////////////
// Microfacet distributions and utility functions
// Largely from https://google.github.io/filament/Filament.html
#pragma variant BURLEY_DIFFUSE, FAST_GGX, ANISOTROPIC, UNCORRELATED_GGX

struct SurfaceAttributes {
    vec3 baseColor;
    float metallic;
    float roughness;
    float reflectance;
    vec3 emissive;
    float occlusion;
#ifdef ANISOTROPIC
    float anisotropy;
#endif
};

#define MEDIUMP_FLT_MAX    65504.0
#define saturateMediump(x) min(x, MEDIUMP_FLT_MAX)
float D_GGX(float roughness, float NoH, const vec3 n, const vec3 h) {
    vec3 NxH = cross(n, h);
    float a = NoH * roughness;
    float k = roughness / (dot(NxH, NxH) + a * a);
    float d = k * k * (1.0 / PI);
    return saturateMediump(d);
}

float D_GGX_Anisotropic(float NoH, const vec3 h, const vec3 t, const vec3 b, float at, float ab) {
    float ToH = dot(t, h);
    float BoH = dot(b, h);
    float a2 = at * ab;
    highp vec3 v = vec3(ab * ToH, at * BoH, a2 * NoH);
    highp float v2 = dot(v, v);
    float w2 = a2 / v2;
    return a2 * w2 * w2 * (1.0 / PI);

    a2 = at * ab;
    return NoH / (PI * NoH*NoH*NoH*NoH * 4);
}

float D_Beckmann(float NoH, const vec3 h, const vec3 t, const vec3 b, float at, float ab) {
    float ToH = dot(t, h);
    float BoH = dot(b, h);
    float a2 = at * ab;
    highp vec3 v = vec3(ab * ToH, at * BoH, a2 * NoH);
    highp float v2 = dot(v, v);
    float w2 = a2 / v2;
    return a2 * w2 * w2 * (1.0 / PI);
}

float V_SmithGGX(float NoV, float NoL, float roughness) {
    float a = roughness;
    float GGXL = NoL / (NoL * (1.0 - a) + a);
    float GGXV = NoV / (NoV * (1.0 - a) + a);
    return GGXL * GGXV;
}

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

float V_SmithGGXCorrelatedFast(float NoV, float NoL, float roughness) {
    float a = roughness;
    float GGXV = NoL * (NoV * (1.0 - a) + a);
    float GGXL = NoV * (NoL * (1.0 - a) + a);
    return 0.5 / (GGXV + GGXL);
}

float V_SmithGGXCorrelated_Anisotropic(float at, float ab, float ToV, float BoV,
        float ToL, float BoL, float NoV, float NoL) {
    float lambdaV = NoL * length(vec3(at * ToV, ab * BoV, NoV));
    float lambdaL = NoV * length(vec3(at * ToL, ab * BoL, NoL));
    float v = 0.5 / (lambdaV + lambdaL);
    return saturateMediump(v);
}

vec3 F_Schlick(float LoH, vec3 f0) {
    float f = pow(1.0 - LoH, 5.0);
    return f + f0 * (1.0 - f);
}

float F_Schlick_Burley(float VoH, float f0, float f90) {
    return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0);
}

float Fd_Lambert() {
    return 1 / PI;
}

float Fd_Burley(float NoV, float NoL, float LoH, float roughness) {
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_Schlick_Burley(NoL, 1.0, f90);
    float viewScatter = F_Schlick_Burley(NoV, 1.0, f90);
    return lightScatter * viewScatter * (1.0 / PI);
}

float DistributionGGX(vec3 N, vec3 H, float a) {
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = pi * denom * denom;
    
    return a2 / denom;
}


//#define DUPUY
#ifdef DUPUY

vec3 fresnel(float cos_theta_d, const SurfaceAttributes surface)
{
    vec3 f0 = 0.16 * surface.reflectance * surface.reflectance * (1.0 - surface.metallic) + surface.baseColor * surface.metallic;
    return F_Schlick(cos_theta_d, f0);
}

float p22(float xslope, float yslope, const SurfaceAttributes surface)
{
    float xslope_sqr = xslope * xslope;
    float yslope_sqr = yslope * yslope;
    return exp(-(xslope_sqr + yslope_sqr)) / PI;
}

float ndf(const vec3 h, const vec3 n, const SurfaceAttributes surface)
{
    vec2 alpha = vec2(1.0);
    float rho = 0.0;
    float rho2 = rho * rho;

    float cos_theta_h_sqr = h.z * h.z;
    float cos_theta_h_sqr_sqr = cos_theta_h_sqr * cos_theta_h_sqr;
    float xslope = -h.x / h.z + n.x / n.z;
    float yslope = -h.y / h.z + n.y / n.z;

    float scale = 1.0 / (alpha.x * alpha.y * sqrt(1 - rho2));

    float anisotropic_xslope = xslope / alpha.x;
    float anisotropic_yslope = (alpha.x * yslope - rho * alpha.y * xslope) * scale;

    return p22(anisotropic_xslope, anisotropic_yslope, surface) / cos_theta_h_sqr_sqr * scale;
}

float erf(float x)
{
    return 0.0; // todo
}

float g1_beckmann(const vec3 h, const vec3 k, const SurfaceAttributes surface)
{
    float v = 1.0 / tan(k.z);
    float tan_v = tan(v);
    float tan_v_sqr = tan_v * tan_v;
    return 2.0 / (1 + sqrt(1 + tan_v_sqr));
}

float g1_ggx(const vec3 h, const vec3 k, const SurfaceAttributes surface)
{
    float v = 1.0 / tan(k.z);
    float v2 = v * v;
    return 2.0 / (1 + erf(v) + exp(-v2) / (v * sqrt(PI)));
}

float g1(const vec3 h, const vec3 k, const vec3 n, const SurfaceAttributes surface)
{
    vec2 alpha = vec2(1.0);
    float rho = 0.0;
    float rho2 = rho * rho;

    float a = k.x * alpha.x + k.y * alpha.y * rho;
    float b = alpha.y * k.y * sqrt(1.0 - rho2);
    float c = k.z + k.x * n.x / n.z + k.y * n.y / n.z;
    vec3 kprime = normalize(vec3(a, b, c));

    return g1_ggx(h, k, surface);

    float g = g1_ggx(h, kprime, surface);
    if (g > 0.0) {
        return g * (k.z / c);
    } else {
        return 0.0;
    }
}

float gaf(const vec3 h, const vec3 i, const vec3 o, const vec3 n, const SurfaceAttributes surface)
{
    float G1_o = g1(h, o, n, surface);
    float G1_i = g1(h, i, n, surface);
    float tmp = G1_o * G1_i;
    if (tmp > 0.0) {
        return tmp / (G1_i + G1_o - tmp);
    } else {
        return 0.0;
    }
}

vec3 brdf(const vec3 v, vec3 n, const vec3 l, const SurfaceAttributes surface)
{
    // Convert to normal space
    vec3 o = mat3(viewMatrix) * v;
    vec3 i = mat3(viewMatrix) * l;
    n = mat3(viewMatrix) * n;

    vec3 h = normalize(i + o);
    float G = gaf(h, i, o, n, surface);

    if (G > 0.0) {
        float cos_theta_d = dot(o, h);
        vec3 F = fresnel(cos_theta_d, surface);
        float D = ndf(h, n, surface);
        return (F * D * G) / (4.0 * o.z * i.z);
    } else {
        return vec3(0.0);
    }
}

#else // DUPUY

#ifdef ANISOTROPIC
vec3 brdf(const vec3 v, const vec3 n, const vec3 l, const vec3 t, const vec3 b, const SurfaceAttributes surface)
#else
vec3 brdf(const vec3 v, const vec3 n, const vec3 l, SurfaceAttributes surface)
#endif
{
    if (dot(l, n) < 0.0) {
        return vec3(0.0);
    }

    vec3 diffuseColor = (1.0 - surface.metallic) * surface.baseColor.rgb;
    vec3 f0 = 0.16 * surface.reflectance * surface.reflectance * (1.0 - surface.metallic) + surface.baseColor * surface.metallic;
    vec3 h = normalize(v + l);

    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);
    float VoH = clamp(dot(v, h), 0.0, 1.0);

    float alpha = surface.roughness * surface.roughness;

#ifdef ANISOTROPIC
    float ToV = clamp(dot(t, v), 0.0, 1.0);
    float BoV = clamp(dot(b, v), 0.0, 1.0);
    float ToL = clamp(dot(t, l), 0.0, 1.0);
    float BoL = clamp(dot(b, l), 0.0, 1.0);
    
    float at = max(alpha * (1.0 + surface.anisotropy), 0.001);
    float ab = max(alpha * (1.0 - surface.anisotropy), 0.001);
    float D = D_GGX_Anisotropic(NoH, h, t, b, at, ab);
    float V = V_SmithGGXCorrelated_Anisotropic(at, ab, ToV, BoV, ToL, BoL, NoV, NoL);
#else
    float D = D_GGX(alpha, NoH, n, h);
  #ifdef UNCORRELATED_GGX
    float V = V_SmithGGX(NoV, NoL, alpha);
  #else // UNCORRELATED_GGX
    #ifdef FAST_GGX
      float V = V_SmithGGXCorrelatedFast(NoV, NoL, alpha);
    #else
      float V = V_SmithGGXCorrelated(NoV, NoL, alpha);
    #endif
  #endif // UNCORRELATED_GGX
#endif

    vec3 F = F_Schlick(LoH, f0);
    vec3 Fs = (D * V) * F;
#ifdef BURLEY_DIFFUSE
    vec3 Fd = Fd_Burley(NoV, NoL, LoH, alpha) * diffuseColor;
#else
    vec3 Fd = Fd_Lambert() * diffuseColor;
#endif

    return (Fs + Fd) * NoL;
}

#endif // DUPUY
// _AUGEN_END_INCLUDE
#endif

struct OutputFragment {
	vec4 radiance;
	vec3 normal;
	float depth;
};

void pbrShading(const in GFragment fragment, out OutputFragment out_fragment)
{
	vec3 camPos_ws = vec3(inverseViewMatrix[3]);
	vec3 toCam = normalize(camPos_ws - fragment.ws_coord);

	SurfaceAttributes surface;
	surface.baseColor = fragment.baseColor;
	surface.metallic = fragment.metallic;
	surface.roughness = fragment.roughness;
	surface.reflectance = 0.7;

	out_fragment.radiance = vec4(0.0, 0.0, 0.0, 1.0);

	for (int k = 0 ; k < 3 ; ++k) {
		float shadow = 0;
		if (uIsShadowMapEnabled) {
			float shadowBias = shadowBiasFromNormal(light[k], fragment.normal);
			shadowBias = 0.0001; // hardcoded hack
			//shadowBias = 0.00005;
			shadow = shadowAt(light[k], fragment.ws_coord, uShadowMapBias);
			shadow = clamp(shadow, 0.0, 1.0);
			//shadow *= .8;
		}
		
		vec3 toLight = normalize(light[k].position_ws - fragment.ws_coord);
		vec3 f = vec3(0.0);
#ifdef OLD_BRDF
		f = bsdfPbrMetallicRoughness(toCam, toLight, fragment.normal, surface.baseColor, surface.roughness, surface.metallic);
#else // OLD_BRDF
		f = brdf(toCam, fragment.normal, toLight, surface);
#endif // OLD_BRDF
		out_fragment.radiance.rgb += f * light[k].color * lightPowerScale * (1. - shadow);
	}
	out_fragment.radiance += vec4(fragment.emission, 0.0);
}

void main() {
	GFragment fragment;
	autoUnpackGFragmentWithOffset(fragment, uBlitOffset);

	// fix normals
	if ((mat3(viewMatrix) * fragment.normal).z < 0) fragment.normal = -fragment.normal;

	OutputFragment out_fragment;
	switch (fragment.material_id) {
	case pbrMaterial:
	{
		pbrShading(fragment, out_fragment);
		break;
	}

	case forwardBaseColorMaterial:
	case forwardAlbedoMaterial:
		out_fragment.radiance = vec4(fragment.baseColor, 1.0);
		break;

	case forwardNormalMaterial:
		out_fragment.radiance = normal2color(fragment.normal, 1.0);
		break;

	case worldMaterial:
	{
#ifdef WHITE_BACKGROUND
		out_fragment.radiance = vec4(1.0);
#else // WHITE_BACKGROUND
		vec3 camPos_ws = vec3(inverseViewMatrix[3]);
		vec3 toCam = normalize(camPos_ws - fragment.ws_coord);
		out_fragment.radiance = sampleSkybox(-toCam);
#endif // WHITE_BACKGROUND
		break;
	}

	case skyboxMaterial:
		out_fragment.radiance = vec4(fragment.baseColor, 1.0);
		break;

	case colormapDebugMaterial:
	{
		float sampleCount = fragment.baseColor.r;
		if (uHasColormap) {
			out_fragment.radiance.rgb = textureLod(uColormap, vec2(clamp(sampleCount / uMaxSampleCount, 0.0, 1.0), 0.5), 0).rgb;
		} else {
			out_fragment.radiance.rgb = vec3(sampleCount / uMaxSampleCount);
		}
	}

	default:
		if (fragment.material_id >= accumulatedPbrMaterial) {
			float sampleCount = fragment.roughness;
			float oneOverSampleCount = 1.0 / sampleCount;
			//GFragment normalizedFragment = fragment;
			//normalizedFragment.baseColor *= oneOverSampleCount;
			//normalizedFragment.roughness *= oneOverSampleCount;
			//normalizedFragment.metallic *= oneOverSampleCount;
			//normalizedFragment.ws_coord *= oneOverSampleCount;
			//normalizedFragment.normal *= oneOverSampleCount;
			//pbrShading(fragment, out_fragment);
			out_fragment.radiance.rgb = fragment.ws_coord * oneOverSampleCount;
			if (uShowSampleCount) {
				if (uHasColormap) {
					out_fragment.radiance.rgb = textureLod(uColormap, vec2(clamp(sampleCount / uMaxSampleCount, 0.0, 1.0), 0.5), 0).rgb;
				} else {
					out_fragment.radiance.rgb = vec3(sampleCount / uMaxSampleCount);
				}
			}
		}
	}
	//gl_FragDepth = texelFetch(in_depth, ivec2(gl_FragCoord.xy), 0).r;
//#define REINHART
	// Tone mapping
#ifdef HDR
// TODO: look at filmic tone mapping
#ifdef REINHART
	out_fragment.radiance = 4.*out_fragment.radiance / (out_fragment.radiance + 1.0);
#else
	const float exposure = 3.5;
	out_fragment.radiance = 1.0 - exp(-out_fragment.radiance * exposure);
#endif
	const float gamma = 0.8;
	out_fragment.radiance = pow(out_fragment.radiance, vec4(1.0 / gamma));
#endif

	/*/ Minimap Shadow Depth
	if (gl_FragCoord.x < 256 && gl_FragCoord.y < 256) {
		float depth = texelFetch(light[0].shadowMap, ivec2(gl_FragCoord.xy * 4.0), 0).r;
		out_fragment.radiance = vec4(vec3(pow(1. - depth, 0.1)), 1.0);
	}
	//*/

	switch (uShadingMode) {
	case 0: // BEAUTY
		break;
	case 1: // NORMAL
		out_fragment.radiance.rgb = normalize(fragment.normal) * .5 + .5;
		break;
	case 2: // BASE_COLOR
		out_fragment.radiance.rgb = fragment.baseColor.rgb;
		break;
	case 3: // METALLIC
		out_fragment.radiance.rgb = vec3(fragment.metallic);
		out_fragment.radiance.a = 1.0;
		break;
	case 4: // ROUGHNESS
		out_fragment.radiance.rgb = vec3(fragment.roughness);
		out_fragment.radiance.a = 1.0;
		break;
	case 5: // WORLD_POSITION
		out_fragment.radiance.rgb = fragment.ws_coord.rgb;
		break;
	case 6: // DEPTH
	{
		float depth = texelFetch(in_depth, ivec2(gl_FragCoord.xy), 0).r;
		float linearDepth = (2.0 * uNear * uFar) / (uFar + uNear - (depth * 2.0 - 1.0) * (uFar - uNear));
		out_fragment.radiance.rgb = vec3(linearDepth / uFar);
		break;
	}
	case 7: // RAW_GBUFFER1
		out_fragment.radiance.rgb = texelFetch(gbuffer0, ivec2(gl_FragCoord.xy), 0).rgb;
		if (uHasColormap) {
			out_fragment.radiance.rgb = textureLod(uColormap, vec2(clamp(out_fragment.radiance.r, 0.0, 1.0), 0.5), 0).rgb;
		}
		out_fragment.radiance.a = 1.0;
		break;
	case 8: // RAW_GBUFFER2
		out_fragment.radiance.rgb = texelFetch(gbuffer1, ivec2(gl_FragCoord.xy), 0).rgb;
		if (uHasColormap) {
			out_fragment.radiance.rgb = textureLod(uColormap, vec2(clamp(out_fragment.radiance.r, 0.0, 1.0), 0.5), 0).rgb;
		}
		out_fragment.radiance.a = 1.0;
		//out_fragment.radiance.rgb = 0.5 + 0.5 * cos(2.*3.1416 * (clamp(1.-out_fragment.radiance.r, 0.0, 1.0) * .5 + vec3(.0,.33,.67)));
		break;
	case 9: // RAW_GBUFFER3
		out_fragment.radiance.rgb = texelFetch(gbuffer2, ivec2(gl_FragCoord.xy), 0).rgb;
		if (uHasColormap) {
			out_fragment.radiance.rgb = textureLod(uColormap, vec2(clamp(out_fragment.radiance.r, 0.0, 1.0), 0.5), 0).rgb;
		}
		out_fragment.radiance.a = 1.0;
		break;
	}

	if (uTransparentFilm) {
		if (fragment.material_id == worldMaterial) {
			out_fragment.radiance = vec4(1, 1, 1, 0);
		} else {
			out_fragment.radiance.a = 1.0;
		}
	}

#ifdef DEBUG_VECTORS
	vec2 sampledPixel = resolution * 0.5;

	sampledPixel = (floor(gl_FragCoord.xy / uDebugVectorsGrid) + vec2(0.5)) * uDebugVectorsGrid;

	vec2 delta = gl_FragCoord.xy - sampledPixel;
	GFragment f2;
	autoUnpackGFragmentWithOffset(f2, delta);
	vec4 debug = vec4(0.0);

	vec4 position_cs = viewMatrix * vec4(normalize(f2.normal), 1.0);
	vec4 position_ps = projectionMatrix * vec4(position_cs.xyz, 1.0);
	vec2 fragCoord = resolution.xy * (position_ps.xy / position_ps.w * 0.5 + 0.5);

	vec4 zero_cs = viewMatrix * vec4(0.0, 0.0, 0.0, 1.0);
	vec4 zero_ps = projectionMatrix * vec4(zero_cs.xyz, 1.0);
	vec2 zeroCoord = resolution.xy * (zero_ps.xy / zero_ps.w * 0.5 + 0.5);

	//vec_cs.xy = vec2(1.0, 1.0);
	vec2 vec_cs = fragCoord - zeroCoord;
	float len = length(vec_cs);

	vec_cs = normalize(vec_cs);

	//debug.a = smoothstep(0.9, 1.0, dot(normalize(delta), vec_cs.xy));
	delta = transpose(mat2(vec_cs, vec_cs.yx * vec2(-1,1))) * delta;

	float arrow = smoothstep(1.0, 0.5, abs(delta.y)) * step(0, delta.x) * step(delta.x, len * uDebugVectorsScale);
	arrow += smoothstep(4.0, 2.0, dot(delta,delta));
	float shadow = smoothstep(1.0, 0.5, abs(delta.y - 1)) * step(0, delta.x) * step(delta.x, len * uDebugVectorsScale);

	debug.rgb = vec3(1. - arrow);

	debug.a = arrow + shadow * 0.4;

	// comp debug vector
	out_fragment.radiance.rgb = mix(out_fragment.radiance.rgb, debug.rgb, debug.a);
#endif // DEBUG_VECTORS

	out_fragment.normal = fragment.normal;
	out_fragment.radiance.a = 1.0;
	// out_fragment was designed to be sent to post effects, ne need here
	//out_fragment.depth = texelFetch(in_depth, ivec2(gl_FragCoord.xy), 0).r;
	out_color = out_fragment.radiance;
}
// _AUGEN_END_INCLUDE
