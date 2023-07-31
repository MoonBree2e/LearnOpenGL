// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\far-grain.frag.glsl
#version 450 core
// _AUGEN_BEGIN_INCLUDE sys:defines
#define PASS_EPSILON_DEPTH
#define PROCEDURAL_BASECOLOR
#define SHELL_CULLING
// _AUGEN_END_INCLUDE sys:defines

#pragma varopt PASS_DEPTH PASS_EPSILON_DEPTH PASS_BLIT_TO_MAIN_FBO
#pragma opt SHELL_CULLING
#pragma opt NO_DISCARD_IN_PASS_EPSILON_DEPTH
#pragma opt PSEUDO_LEAN

const int cDebugShapeNone = -1;
const int cDebugShapeRaytracedSphere = 0; // directly lit sphere, not using ad-hoc lighting
const int cDebugShapeDisc = 1;
const int cDebugShapeSquare = 2;
uniform int uDebugShape = cDebugShapeDisc;

const int cWeightNone = -1;
const int cWeightLinear = 0;
uniform int uWeightMode = cWeightNone;

uniform float uEpsilon = 0.5;
uniform bool uShellDepthFalloff = false;

uniform bool uCheckboardSprites;
uniform bool uShowSampleCount;

uniform sampler2D uDepthTexture;

uniform bool uDebugRenderType = false;
uniform vec3 uDebugRenderColor = vec3(32.0/255.0, 64.0/255.0, 161.0/255.0);

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

// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/zbuffer.inc.glsl
//////////////////////////////////////////////////////
// Z-Buffer utils
// Requires a "include/uniform/camera.inc.glsl" and "include/utils.inc.glsl"

// CHange these uniforms if using glDepthRange()
uniform float uDepthRangeMin = 0.0f;
uniform float uDepthRangeMax = 1.0f;

float linearizeDepth(float logDepth) {
	float in01 = (logDepth - uDepthRangeMin) / (uDepthRangeMax - uDepthRangeMin);
	return (2.0 * uNear * uFar) / (uFar + uNear - (in01 * 2.0 - 1.0) * (uFar - uNear));
}

float unlinearizeDepth(float linearDepth) {
	//return (uFar + uNear - (2.0 * uNear * uFar) / linearDepth) / (uFar - uNear) * 0.5 + 0.5;
	float in01 = (uFar + uNear - (2.0 * uNear * uFar) / linearDepth) / (uFar - uNear) * 0.5 + 0.5;
	return uDepthRangeMin + in01 * (uDepthRangeMax - uDepthRangeMin);
}
// _AUGEN_END_INCLUDE

// Compute influence weight of a grain fragment
// sqDistToCenter = dot(uv, uv);
float computeWeight(vec2 uv, float sqDistToCenter) {
    float depthWeight = 1.0;
    if (uShellDepthFalloff) {
        float d = texelFetch(uDepthTexture, ivec2(gl_FragCoord.xy), 0).x;
        float limitDepth = linearizeDepth(d);
        float depth = linearizeDepth(gl_FragCoord.z);
        float fac = (limitDepth - depth) / uEpsilon;
        depthWeight = fac;
    }

    float blendWeight = 1.0;
    switch (uWeightMode) {
    case cWeightLinear:
        if (uDebugShape == cDebugShapeSquare) {
            blendWeight = 1.0 - max(abs(uv.x), abs(uv.y));
            break;
        } else {
            blendWeight = 1.0 - sqrt(sqDistToCenter);
            break;
        }
    case cWeightNone:
    default:
        blendWeight = 1.0;
        break;
    }

    return depthWeight * blendWeight;
}

///////////////////////////////////////////////////////////////////////////////
#if defined(PASS_BLIT_TO_MAIN_FBO)

#ifdef PSEUDO_LEAN
#define IN_LEAN_LINEAR_GBUFFER
#else // PSEUDO_LEAN
#define IN_LINEAR_GBUFFER
#endif // PSEUDO_LEAN
#define OUT_GBUFFER
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

// depth attachement of the target fbo
// (roughly safe to use despite feedback loops because we know that there is
// exactly one fragment per pixel)
//uniform sampler2D uDepthTexture; // nope, depth test used as is, just write to gl_FragDepth

// attachments of the secondary fbo
uniform sampler2D uFboDepthTexture;

// Problem: semi transparency requires to read from current color attachment
// but this creates dangerous feedback loops
// (glBlend cannot be used because of packing)

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
// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/raytracing.inc.glsl
//////////////////////////////////////////////////////
// Ray-tracing routines

// requires that you include utils.inc.glsl for isOrthographic() and uniforms/camera.inc.glsl for resolution

struct Ray {
    vec3 origin;
    vec3 direction;
};

/**
 * Apply an affine transformation matrix to a ray
 */
Ray TransformRay(Ray ray, mat4 transform) {
    Ray transformed_ray;
    transformed_ray.origin = (transform * vec4(ray.origin, 1)).xyz;
    transformed_ray.direction = mat3(transform) * ray.direction;
    return transformed_ray;
}

/**
 * Get the ray corresponding to a pixel, in camera space
 */
Ray fragmentRay(in vec4 fragCoord, in mat4 projectionMatrix) {
    vec2 uv = fragCoord.xy / resolution.xy * 2.f - vec2(1.f);
    
    if (isOrthographic(projectionMatrix)) {
        float a = 1.0 / projectionMatrix[0][0];
        float b = a * projectionMatrix[3][0];
        float c = 1.0 / projectionMatrix[1][1];
        float d = c * projectionMatrix[3][1];
        return Ray(
            vec3(a * uv.x + b, c * uv.y + d, 0.0),
            vec3(0.0, 0.0, -1.f)
        );
    } else {
        float fx = projectionMatrix[0][0];
        float fy = projectionMatrix[1][1];
        return Ray(
            vec3(0.0),
            vec3(uv.x / fx, uv.y / fy, -1.f)
        );
    }
}


bool intersectRaySphere(out vec3 hitPosition, in Ray ray, in vec3 center, in float radius) {
    vec3 o = center - ray.origin;
    float d2 = dot(ray.direction, ray.direction);
    float r2 = radius * radius;
    float l2 = dot(o, o);
    float dp = dot(ray.direction, o);
    float delta = dp * dp / d2 - l2 + r2;

    if (delta >= 0) {
        float lambda = dp / d2 - sqrt(delta / d2);
        if (lambda < 0) lambda += 2. * sqrt(delta / d2);
        if (lambda >= 0) {
            hitPosition = ray.origin + lambda * ray.direction;
            return true;
        }
    }

    return false;
}


vec3 intersectRayPlane(in Ray ray, in vec3 pointOnPlane, in vec3 planeNormal) {
    vec3 o = pointOnPlane - ray.origin;
    float lambda = dot(o, planeNormal) / dot(ray.direction, planeNormal);
    return ray.origin + lambda * ray.direction;
}

// _AUGEN_END_INCLUDE

// This stage gathers additive measures and turn them into regular g-buffer fragments
void main() {
    float d = texelFetch(uFboDepthTexture, ivec2(gl_FragCoord.xy), 0).x;
    gl_FragDepth = d;
    // add back uEpsilon here -- benchmark-me
    //gl_FragDepth = unlinearizeDepth(linearizeDepth(d) - uEpsilon);

    GFragment fragment;
    autoUnpackGFragment(fragment);

#ifdef NO_DISCARD_IN_PASS_EPSILON_DEPTH
    if (fragment.alpha < 0.00001) discard;
#endif // NO_DISCARD_IN_PASS_EPSILON_DEPTH
    float weightNormalization = 1.0 / fragment.alpha; // inverse sum of integration weights

    fragment.baseColor *= weightNormalization;
    fragment.normal *= weightNormalization;
    fragment.roughness *= weightNormalization;
    fragment.material_id = pbrMaterial;

#ifdef PSEUDO_LEAN
    // LEAN mapping
    fragment.lean1 *= weightNormalization; // first order moments
    fragment.lean2 *= weightNormalization; // second order moments
    vec2 B = fragment.lean1.xy;
    vec3 M = fragment.lean2.xyz;
    float s = 48.0; // phong exponent must be used but we use ggx :/
    M.xy += 1./s; // bake roughness into normal distribution
    vec2 diag = M.xy - B.xy * B.xy;
    float e = M.z - B.x * B.y;
    mat2 inv_sigma = inverse(mat2(diag.x, e, e, diag.y));

    fragment.normal = normalize(vec3(fragment.lean1.xy, 1));
    fragment.normal = mat3(inverseViewMatrix) * fragment.normal;
    fragment.roughness = length(diag);
#else // PSEUDO_LEAN
    float dev = clamp(pow(1. - length(fragment.normal), 0.2), 0, 1);
    fragment.roughness = mix(fragment.roughness, 1.0, dev);
#endif // PSEUDO_LEAN

    // Fix depth
    Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
    vec3 cs_coord = (linearizeDepth(d) - uEpsilon) * ray_cs.direction;
    fragment.ws_coord = (inverseViewMatrix * vec4(cs_coord, 1.0)).xyz;

    //fragment.baseColor *= 0.8;

    if (uShowSampleCount) {
        fragment.material_id = colormapDebugMaterial;
        fragment.baseColor = vec3(fragment.alpha);
    }

    if (uDebugRenderType) {
        fragment.baseColor = uDebugRenderColor;
    }


    autoPackGFragment(fragment);
}

///////////////////////////////////////////////////////////////////////////////
#elif defined(PASS_EPSILON_DEPTH)

void main() {
#ifndef NO_DISCARD_IN_PASS_EPSILON_DEPTH
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    if (uDebugShape != cDebugShapeSquare && dot(uv, uv) > 1.0) {
        discard;
    }
#endif // not NO_DISCARD_IN_PASS_EPSILON_DEPTH
}

///////////////////////////////////////////////////////////////////////////////
#else // DEFAULT PASS

// If not using shell culling, we render directly to G-buffer, otherwise
// there is an extra blitting pass to normalize cumulated values.
#ifdef SHELL_CULLING
#  ifdef PSEUDO_LEAN
#    define OUT_LEAN_LINEAR_GBUFFER
#  else // PSEUDO_LEAN
#    define OUT_LINEAR_GBUFFER
#  endif // PSEUDO_LEAN
#else // SHELL_CULLING
#  define OUT_GBUFFER
#endif // SHELL_CULLING
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

in FragmentData {
    vec3 position_ws;
    vec3 baseColor;
    float radius;
    float screenSpaceDiameter;
    float diameterOvershot;
    float metallic;
    float roughness;
} inData;

uniform float uTime;

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
// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/raytracing.inc.glsl
//////////////////////////////////////////////////////
// Ray-tracing routines

// requires that you include utils.inc.glsl for isOrthographic() and uniforms/camera.inc.glsl for resolution

struct Ray {
    vec3 origin;
    vec3 direction;
};

/**
 * Apply an affine transformation matrix to a ray
 */
Ray TransformRay(Ray ray, mat4 transform) {
    Ray transformed_ray;
    transformed_ray.origin = (transform * vec4(ray.origin, 1)).xyz;
    transformed_ray.direction = mat3(transform) * ray.direction;
    return transformed_ray;
}

/**
 * Get the ray corresponding to a pixel, in camera space
 */
Ray fragmentRay(in vec4 fragCoord, in mat4 projectionMatrix) {
    vec2 uv = fragCoord.xy / resolution.xy * 2.f - vec2(1.f);
    
    if (isOrthographic(projectionMatrix)) {
        float a = 1.0 / projectionMatrix[0][0];
        float b = a * projectionMatrix[3][0];
        float c = 1.0 / projectionMatrix[1][1];
        float d = c * projectionMatrix[3][1];
        return Ray(
            vec3(a * uv.x + b, c * uv.y + d, 0.0),
            vec3(0.0, 0.0, -1.f)
        );
    } else {
        float fx = projectionMatrix[0][0];
        float fy = projectionMatrix[1][1];
        return Ray(
            vec3(0.0),
            vec3(uv.x / fx, uv.y / fy, -1.f)
        );
    }
}


bool intersectRaySphere(out vec3 hitPosition, in Ray ray, in vec3 center, in float radius) {
    vec3 o = center - ray.origin;
    float d2 = dot(ray.direction, ray.direction);
    float r2 = radius * radius;
    float l2 = dot(o, o);
    float dp = dot(ray.direction, o);
    float delta = dp * dp / d2 - l2 + r2;

    if (delta >= 0) {
        float lambda = dp / d2 - sqrt(delta / d2);
        if (lambda < 0) lambda += 2. * sqrt(delta / d2);
        if (lambda >= 0) {
            hitPosition = ray.origin + lambda * ray.direction;
            return true;
        }
    }

    return false;
}


vec3 intersectRayPlane(in Ray ray, in vec3 pointOnPlane, in vec3 planeNormal) {
    vec3 o = pointOnPlane - ray.origin;
    float lambda = dot(o, planeNormal) / dot(ray.direction, planeNormal);
    return ray.origin + lambda * ray.direction;
}

// _AUGEN_END_INCLUDE
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

void main() {
    GFragment fragment;
    initGFragment(fragment);

    vec2 uv = (gl_PointCoord * 2.0 - 1.0) * inData.diameterOvershot;
    float sqDistToCenter = dot(uv, uv);

    float antialiasing = 1.0;
    float weight = 1.0;
#ifdef SHELL_CULLING
    weight = computeWeight(uv, sqDistToCenter);
    if (uDebugShape != cDebugShapeSquare) {
        antialiasing = smoothstep(1.0, 1.0 - 5.0 / inData.screenSpaceDiameter, sqDistToCenter);
    }
    weight *= antialiasing;
#else // SHELL_CULLING
    if (uDebugShape != cDebugShapeSquare && sqDistToCenter > 1.0) {
        discard;
    }
#endif // SHELL_CULLING

    vec3 n;
    Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
    Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);
    if (uDebugShape == cDebugShapeRaytracedSphere) {
        vec3 sphereHitPosition_ws;
        bool intersectSphere = intersectRaySphere(sphereHitPosition_ws, ray_ws, inData.position_ws.xyz, inData.radius);
        if (intersectSphere) {
            n = normalize(sphereHitPosition_ws - inData.position_ws);
        } else {
            //discard;
            n = -ray_ws.direction;
        }
        //n = mix(-ray_ws.direction, n, antialiasing);
    } else if (uDebugShape == cDebugShapeDisc) {
        if (antialiasing > 0.0) {
            n = mat3(inverseViewMatrix) * vec3(uv, sqrt(1.0 - sqDistToCenter));
            n = mix(-ray_ws.direction, n, antialiasing);
        } else {
            n = -ray_ws.direction;
        }
        //n = mat3(inverseViewMatrix) * vec3(uv, sqrt(max(0.0, 1.0 - sqDistToCenter)));
    } else {
        n = mat3(inverseViewMatrix) * normalize(-ray_cs.direction);
    }

    vec3 baseColor = inData.baseColor;

    // DEBUG checkerboard for mipmap test
    if (uCheckboardSprites) {
        baseColor = vec3(abs(step(uv.x, 0.0) - step(uv.y, 0.0)));
    }

    fragment.baseColor.rgb = baseColor * weight;
    fragment.alpha = weight;
    fragment.normal = n * weight;
    fragment.metallic = inData.metallic * weight;
    fragment.roughness = inData.roughness * weight;

    // LEAN mapping
#ifdef PSEUDO_LEAN
    fragment.normal = mat3(viewMatrix) * fragment.normal;
    vec2 slopes = fragment.normal.xy / fragment.normal.z;
    if (fragment.normal.z == 0.0) {
        slopes = vec2(0.0);
    }
    fragment.lean1.xy = slopes.xy * weight;
    fragment.lean2.xy = slopes.xy * slopes.xy * weight;
    fragment.lean2.z = slopes.x * slopes.y * weight;
#endif // PSEUDO_LEAN

    if (uShowSampleCount) {
        fragment.alpha = clamp(ceil(antialiasing), 0, 1);
    }

#ifndef SHELL_CULLING
    fragment.ws_coord = inData.position_ws;
    fragment.material_id = pbrMaterial;

    if (uDebugRenderType) {
        fragment.baseColor = uDebugRenderColor;
    }
#endif // not SHELL_CULLING

    autoPackGFragment(fragment);
}

///////////////////////////////////////////////////////////////////////////////
#endif // STAGE
// _AUGEN_END_INCLUDE
