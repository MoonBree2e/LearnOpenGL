// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\impostor-grain.geo.glsl
#version 450 core
// _AUGEN_BEGIN_INCLUDE sys:defines
#define PRECOMPUTE_IN_VERTEX
#define PROCEDURAL_BASECOLOR
// _AUGEN_END_INCLUDE sys:defines

#pragma varopt PASS_BLIT_TO_MAIN_FBO
#pragma opt PRECOMPUTE_IN_VERTEX

///////////////////////////////////////////////////////////////////////////////
#ifdef PASS_BLIT_TO_MAIN_FBO

// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/standard-posteffect.geo.inc.glsl

in vec2 vertUv[];
in flat int vertLayer[];
out vec2 uv;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// this is just a passthrough geo shader
void main() {
	gl_Layer = vertLayer[0];
	for (int i = 0; i < gl_in.length(); i++) {
		gl_Position = gl_in[i].gl_Position;
		uv = vertUv[i];
		EmitVertex();
	}
	EndPrimitive();
}
// _AUGEN_END_INCLUDE

///////////////////////////////////////////////////////////////////////////////
#else // PASS_BLIT_TO_MAIN_FBO

layout(points) in;
layout(points, max_vertices = 1) out;

struct PointCloundVboEntry {
    vec4 position;
};
layout(std430, binding = 0) restrict readonly buffer pointsSsbo {
    PointCloundVboEntry pointVertexAttributes[];
};
layout (std430, binding = 1) restrict readonly buffer pointElementsSsbo {
    uint pointElements[];
};

in VertexData {
	uint id;
} vert[];

out GeometryData {
	flat uint id;
	float radius;
	vec3 position_ws;
	mat4 gs_from_ws;
#ifdef PRECOMPUTE_IN_VERTEX
	flat uvec4 i;
	vec2 alpha;
#endif // PRECOMPUTE_IN_VERTEX
} geo;

uniform uint uFrameCount = 1;
uniform uint uPointCount;
uniform float uFps = 25.0;
uniform float uTime;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
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

uniform bool uUseAnimation = true;
uniform bool uUsePointElements = true;

uniform float uGrainRadius;

uniform sampler2D uOcclusionMap;
uniform bool uUseOcclusionMap = false;

uniform int uPrerenderSurfaceStep = 0; // 0: prerender, 1: render remaining
uniform bool uPrerenderSurface = false;

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
// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/random.inc.glsl
//////////////////////////////////////////////////////
// Random generation functions

float randv2(vec2 co){
    return fract(sin(dot(co ,vec2(12.9898,78.233))) * 43758.5453);
}
float rand(vec3 co){
    return randv2(vec2(randv2(co.xy/100.0f), randv2(co.yz/100.0f)));
}
float rand2(vec3 co){
    float a = rand(co);
    return rand(vec3(a,a+1,a+2));
}
float rand3(vec3 co){
    float a = rand(co);
    float b = rand(co);
    return rand(vec3(a,b,a+b));
}
vec3 randVec(vec3 co) {
    return vec3(rand(co), rand2(co), rand3(co)) * 2.f - vec3(1.f);
}

float length2(vec3 v) {
    return dot(v,v);
}
// _AUGEN_END_INCLUDE
// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/sprite.inc.glsl
// Requires a "include/uniform/camera.inc.glsl" and "include/utils.inc.glsl"

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

float SpriteSize_Botsch03(float radius, vec4 position_cameraspace) {
    if (isOrthographic(projectionMatrix)) {
        return 2.0 * radius * resolution.y / (uTop - uBottom);
    }
	return 2.0 * radius * uNear / abs(position_cameraspace.z) * resolution.y / (uTop - uBottom);
}

/**
 * Corrects to take into account the fact that projected bounding spheres are
 * ellipses, not circles. Returns ellipse's largest radius.
 */
float SpriteSize_Botsch03_corrected(float radius, vec4 position_cameraspace) {
	if (isOrthographic(projectionMatrix)) {
        return 2.0 * radius * resolution.y / (uTop - uBottom);
    }
	float l2 = dot(position_cameraspace.xyz, position_cameraspace.xyz);
	float z = position_cameraspace.z;
	float z2 = z * z;
	float r2 = radius * radius;
	return 2.0 * radius * uNear * sqrt(l2 - r2) / abs(r2 - z2) * resolution.y / (uTop - uBottom);
}
// _AUGEN_END_INCLUDE
// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\grain/random-grains.inc.glsl
// requires random.inc.glsl
#pragma variant NO_GRAIN_ROTATION

mat3 randomGrainOrientation(int id) {
    vec3 vx = normalize(randVec(vec3(id, id, id)));
    vec3 vy = normalize(cross(vec3(0,0,sign(-vx) * 1 + .001), vx));
    vec3 vz = normalize(cross(vx, vy));
    float t = 0.0;
    return mat3(vx, vy * cos(t) - vz * sin(t), vy * sin(t) + vz * cos(t));
}

mat4 randomGrainMatrix(int id, vec3 position_ws) {
#ifdef NO_GRAIN_ROTATION
    mat3 rot = mat3(1.0);
#else // NO_GRAIN_ROTATION
    mat3 rot = randomGrainOrientation(id);
#endif // NO_GRAIN_ROTATION
    return mat4(
        vec4(rot[0], 0.0),
        vec4(rot[1], 0.0),
        vec4(rot[2], 0.0),
        vec4(- rot * position_ws, 1.0)
    );
}

// Used to query colorramp image
float randomGrainColorFactor(int id) {
    //return float(id) * .1;
    return fract(dot(vec2(id*.01, id*.213) ,vec2(12.9898,78.233))*.01);
    return randv2(vec2(id*.01, id*.213));
}
// _AUGEN_END_INCLUDE

#ifdef PRECOMPUTE_IN_VERTEX
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
// _AUGEN_BEGIN_INCLUDE D:\Repo\GrainViewer\share\shaders\include/impostor.inc.glsl
//////////////////////////////////////////////////////
// Light related functions
// requires gbuffer.inc.glsl and raytracing.inc.glsl

// requires that all impostors use the same number of views
#pragma opt PRECOMPUTE_IMPOSTOR_VIEW_MATRICES

#ifdef PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
layout(std430, binding = 4) restrict readonly buffer impostorViewMatricesSsbo {
    mat4 impostorViewMatrices[];
};
#endif // PRECOMPUTE_IMPOSTOR_VIEW_MATRICES

uniform float uGrainScale = 1.0;

struct SphericalImpostor {
	sampler2DArray normalAlphaTexture;
	sampler2DArray baseColorTexture;
	sampler2DArray metallicRoughnessTexture;
	sampler2DArray lean1Texture;
	sampler2DArray lean2Texture;
	uint viewCount; // number of precomputed views
	vec3 baseColor;
	float metallic;
	float roughness;
	bool hasBaseColorMap;
	bool hasMetallicRoughnessMap;
	bool hasLeanMapping;
};

struct SphericalImpostorHit {
	vec3 position;
	vec3 textureCoords;
};

/**
 * Return only the index of the closest view to the direction
 */
uint DirectionToViewIndex(vec3 d, uint n) {
	d = d / dot(vec3(1, 1, 1), abs(d));
	vec2 uv = (vec2(1, -1) * d.y + d.x + 1) * (float(n) - 1) * 0.5;
	float i = dot(round(uv), vec2(n, 1));
	if (d.z > 0) {
		i += n * n;
	}
	return uint(i);
}

/**
 * Return the indices of the four closest views to the direction, as well as the
 * interpolation coefficients alpha.
 */
void DirectionToViewIndices(vec3 d, uint n, out uvec4 i, out vec2 alpha) {
	d = d / dot(vec3(1,1,1), abs(d));
	vec2 uv = (vec2(1, -1) * d.y + d.x + 1) * (n - 1) / 2;
	uvec2 fuv = uvec2(floor(uv)) * uvec2(n, 1);
	uvec2 cuv = uvec2(ceil(uv)) * uvec2(n, 1);
	i.x = fuv.x + fuv.y;
	i.y = cuv.x + fuv.y;
	i.z = fuv.x + cuv.y;
	i.w = cuv.x + cuv.y;
	if (d.z > 0) {
		i += n * n;
	}
	alpha = fract(uv);
}

/**
 * Return the direction of the i-th plane in an octahedric division of the unit
 * sphere of n subdivisions.
 */
vec3 ViewIndexToDirection(uint i, uint n) {
#ifdef PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
	mat4 m = impostorViewMatrices[i];
	return vec3(m[0].z, m[1].z, m[2].z);
#else // PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
	float eps = -1;
	uint n2 = n * n;
	if (i >= n2) {
		eps = 1;
		i -= n2;
	}
	vec2 uv = vec2(i / n, i % n) / float(n - 1);
	float x = uv.x + uv.y - 1;
	float y = uv.x - uv.y;
	float z = eps * (1. - abs(x) - abs(y));
	// break symmetry in redundant parts. TODO: find a mapping without redundancy, e.g. full octahedron
	if (z == 0) z = 0.0001 * eps;
	return normalize(vec3(x, y, z));
#endif // PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
}

/**
 * Build the inverse of the view matrix used to bake the i-th G-Impostor
 */
mat4 InverseBakingViewMatrix(uint i, uint n) {
#ifdef PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
	return impostorViewMatrices[i];
#else // PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
	vec3 z = ViewIndexToDirection(i, n);
	vec3 x = normalize(cross(vec3(0, 0, 1), z));
	vec3 y = normalize(cross(z, x));
	vec3 w = vec3(0.0);
	return transpose(mat4(
		vec4(x, 0.0),
		vec4(y, 0.0),
		vec4(z, 0.0),
		vec4(w, 1.0)
	));
#endif // PRECOMPUTE_IMPOSTOR_VIEW_MATRICES
}

/**
 * Return the position and texture coordinates intersected by the ray in the i-th G-billboard of a spherical G-impostor
 * Basic "PlaneHit" sampling scheme
 */
SphericalImpostorHit IntersectRayBillboard(Ray ray, uint i, float radius, uint n) {
	mat4 bs_from_ws = InverseBakingViewMatrix(i, n);
	Ray ray_bs = TransformRay(ray, bs_from_ws);

	float l = -(ray_bs.origin.z / ray_bs.direction.z);
	vec3 uv_bs = ray_bs.origin + l * ray_bs.direction;
	vec2 uv_ts = uv_bs.xy * vec2(1,-1) * 0.5 / radius + 0.5;

	SphericalImpostorHit hit;
	hit.position = ray.origin + l * ray.direction;
	hit.textureCoords = vec3(clamp(uv_ts, vec2(0.0), vec2(1.0)), i);
	return hit;
}

/**
 * "SphereHit" sampling scheme
 */
SphericalImpostorHit IntersectRayBillboard_SphereHit(Ray ray, uint i, float radius, uint n, float hitSphereCorrectionFactor) {
	vec3 hitPosition;
	if (!intersectRaySphere(hitPosition, ray, vec3(0.0), radius * hitSphereCorrectionFactor)) {
		SphericalImpostorHit hit;
		hit.position = vec3(-1.0);
		hit.textureCoords = vec3(-1);
		return hit; // No hit
	}
	ray.origin = hitPosition;
	ray.direction = -ViewIndexToDirection(i, n);
	
	return IntersectRayBillboard(ray, i, radius, n);
}

/**
 * Mix of "PlaneHit" and "SphereHit" sampling schemes
 */
SphericalImpostorHit IntersectRayBillboard_MixedHit(Ray ray, uint i, float radius, uint n, float hitSphereCorrectionFactor) {
	SphericalImpostorHit sphereHit = IntersectRayBillboard_SphereHit(ray, i, radius, n, hitSphereCorrectionFactor);
	SphericalImpostorHit planeHit = IntersectRayBillboard(ray, i, radius, n);

	if (sphereHit.textureCoords.x < 0) {
		return planeHit;
	}

	float distanceToCenter = length(ray.origin - dot(ray.origin, ray.direction) / dot(ray.direction,ray.direction) * ray.direction) / radius;
	float t = smoothstep(hitSphereCorrectionFactor * .2, hitSphereCorrectionFactor, distanceToCenter);

	SphericalImpostorHit hit;
	hit.position = mix(sphereHit.position, planeHit.position, t);
	hit.textureCoords = mix(sphereHit.textureCoords, planeHit.textureCoords, t);
	return hit;
}

/**
 * Sample the billboard textures to return a GFragment
 */
GFragment SampleBillboard(SphericalImpostor impostor, SphericalImpostorHit hit) {
	// If invalid hit, return transparent fragment
	if (hit.textureCoords.x <= 0 || hit.textureCoords.x >= 1 || hit.textureCoords.y <= 0 || hit.textureCoords.y >= 1) {
		GFragment g;
		g.alpha = 0.0;
		return g;
	}
	
	vec3 uvw = vec3((hit.textureCoords.xy - 0.5) * uGrainScale + 0.5, hit.textureCoords.z);

	// Otherwise, sample textures
	vec4 normalAlpha = texture(impostor.normalAlphaTexture, uvw);
	//vec4 lean1 = vec4(0.0);
	//vec4 lean2 = vec4(0.0);
	vec4 baseColor = vec4(0.0);
	vec2 metallicRoughnes = vec2(impostor.metallic, impostor.roughness);
	if (normalAlpha.a > 0) {
		baseColor = texture(impostor.baseColorTexture, uvw);
		//lean1 = texture(impostor.lean1Texture, uvw);
		//lean2 = texture(impostor.lean2Texture, uvw);
		if (impostor.hasMetallicRoughnessMap) {
			metallicRoughnes = texture(impostor.metallicRoughnessTexture, uvw).xy;
		}
	}

	GFragment g;
	g.baseColor = baseColor.rgb;
	g.normal = normalAlpha.xyz * 2. - 1.;
	//g.lean1 = lean1;
	//g.lean2 = lean2;
	g.ws_coord = hit.position;
	g.metallic = metallicRoughnes.x;
	g.roughness = metallicRoughnes.y;
	//g.emission = __;
	g.alpha = normalAlpha.a;
	return g;
}

/**
 * Sample a Spherical G-impostor of radius radius and located at world position p
 * (Argl, stupid code duplication incoming)
 */
GFragment aux_IntersectRaySphericalGBillboard(SphericalImpostor impostor, Ray ray, float radius, uvec4 i, vec2 alpha) {
	uint n = impostor.viewCount;
	GFragment g1 = SampleBillboard(impostor, IntersectRayBillboard(ray, i.x, radius, n));
	GFragment g2 = SampleBillboard(impostor, IntersectRayBillboard(ray, i.y, radius, n));
	GFragment g3 = SampleBillboard(impostor, IntersectRayBillboard(ray, i.z, radius, n));
	GFragment g4 = SampleBillboard(impostor, IntersectRayBillboard(ray, i.w, radius, n));
	
	return LerpGFragment(
		LerpGFragment(g1, g2, alpha.x),
		LerpGFragment(g3, g4, alpha.x),
		alpha.y
	);
}
GFragment aux_IntersectRaySphericalGBillboard_SphereHit(SphericalImpostor impostor, Ray ray, float radius, float hitSphereCorrectionFactor, uvec4 i, vec2 alpha) {
	uint n = impostor.viewCount;
	GFragment g1 = SampleBillboard(impostor, IntersectRayBillboard_SphereHit(ray, i.x, radius, n, hitSphereCorrectionFactor));
	GFragment g2 = SampleBillboard(impostor, IntersectRayBillboard_SphereHit(ray, i.y, radius, n, hitSphereCorrectionFactor));
	GFragment g3 = SampleBillboard(impostor, IntersectRayBillboard_SphereHit(ray, i.z, radius, n, hitSphereCorrectionFactor));
	GFragment g4 = SampleBillboard(impostor, IntersectRayBillboard_SphereHit(ray, i.w, radius, n, hitSphereCorrectionFactor));
	
	return LerpGFragment(
		LerpGFragment(g1, g2, alpha.x),
		LerpGFragment(g3, g4, alpha.x),
		alpha.y
	);
}
GFragment aux_IntersectRaySphericalGBillboard_MixedHit(SphericalImpostor impostor, Ray ray, float radius, float hitSphereCorrectionFactor, uvec4 i, vec2 alpha) {
	uint n = impostor.viewCount;
	GFragment g1 = SampleBillboard(impostor, IntersectRayBillboard_MixedHit(ray, i.x, radius, n, hitSphereCorrectionFactor));
	GFragment g2 = SampleBillboard(impostor, IntersectRayBillboard_MixedHit(ray, i.y, radius, n, hitSphereCorrectionFactor));
	GFragment g3 = SampleBillboard(impostor, IntersectRayBillboard_MixedHit(ray, i.z, radius, n, hitSphereCorrectionFactor));
	GFragment g4 = SampleBillboard(impostor, IntersectRayBillboard_MixedHit(ray, i.w, radius, n, hitSphereCorrectionFactor));
	
	return LerpGFragment(
		LerpGFragment(g1, g2, alpha.x),
		LerpGFragment(g3, g4, alpha.x),
		alpha.y
	);
}
/**
 * Sample a Spherical G-impostor of radius radius and located at world position p
 */
GFragment IntersectRaySphericalGBillboard(SphericalImpostor impostor, Ray ray, float radius) {
	uvec4 i;
	vec2 alpha;
	DirectionToViewIndices(-ray.direction, impostor.viewCount, i, alpha);
	return aux_IntersectRaySphericalGBillboard(impostor, ray, radius, i, alpha);
}
GFragment IntersectRaySphericalGBillboard_SphereHit(SphericalImpostor impostor, Ray ray, float radius, float hitSphereCorrectionFactor) {
	uvec4 i;
	vec2 alpha;
	DirectionToViewIndices(-ray.direction, impostor.viewCount, i, alpha);
	return aux_IntersectRaySphericalGBillboard_SphereHit(impostor, ray, radius, hitSphereCorrectionFactor, i, alpha);
}
GFragment IntersectRaySphericalGBillboard_MixedHit(SphericalImpostor impostor, Ray ray, float radius, float hitSphereCorrectionFactor) {
	uvec4 i;
	vec2 alpha;
	DirectionToViewIndices(-ray.direction, impostor.viewCount, i, alpha);
	return aux_IntersectRaySphericalGBillboard_MixedHit(impostor, ray, radius, hitSphereCorrectionFactor, i, alpha);
}

/**
 * Same as IntersectRaySphericalGBillboard wthout interpolation
 */
 GFragment aux_IntersectRaySphericalGBillboardNoInterp(SphericalImpostor impostor, Ray ray, float radius, uint i) {
	uint n = impostor.viewCount;
	GFragment g = SampleBillboard(impostor, IntersectRayBillboard(ray, i, radius, n));
	return g;
}
GFragment IntersectRaySphericalGBillboardNoInterp(SphericalImpostor impostor, Ray ray, float radius) {
	uint n = impostor.viewCount;
	uint i = DirectionToViewIndex(-ray.direction, n);
	return aux_IntersectRaySphericalGBillboardNoInterp(impostor, ray, radius, i);
}

/**
 * Intersect with sphere (for debug)
 */
GFragment IntersectRaySphere(Ray ray, vec3 p, float radius) {
	GFragment g;
	g.material_id = forwardBaseColorMaterial;

	vec3 hitPosition;
	if (intersectRaySphere(hitPosition, ray, p, radius)) {
		g.baseColor.rgb = normalize(hitPosition - p) * .5 + .5;
		g.normal.rgb = normalize(hitPosition - p);
		g.alpha = 1.0;
	} else {
		g.alpha = 0.0;
	}

	return g;
}

/**
 * Intersect with cube (for debug)
 */
GFragment IntersectRayCube(Ray ray, vec3 p, float radius) {
	GFragment g;
	g.material_id = forwardBaseColorMaterial;

	float minl = 9999999.9;
	g.alpha = 0.0;
	for (int j = 0; j < 6; ++j) {
		vec3 n;
		if (j < 2) n = vec3((j % 2) * 2 - 1, 0, 0);
		else if (j < 4) n = vec3(0, (j % 2) * 2 - 1, 0);
		else n = vec3(0, 0, (j % 2) * 2 - 1);
		vec3 pp = p + radius * n;
		vec3 dp = pp - ray.origin;
		float l = dot(dp, n) / dot(ray.direction, n);
		if (l > 0 && l < minl) {
			vec3 i = ray.origin + l * ray.direction;

			vec3 ii = i - pp;
			float linf = max(max(abs(ii.x), abs(ii.y)), abs(ii.z));
			if (linf <= radius) {
				g.baseColor.rgb = n * .5 + .5;
				g.alpha = 1.0;
				g.normal = n;
				minl = l;
			}
		}
	}

	return g;
}
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
uniform SphericalImpostor uImpostor[3];
#endif // PRECOMPUTE_IN_VERTEX

void main() {
	geo.id =
        uUsePointElements
        ? pointElements[vert[0].id]
        : vert[0].id;

    uint animPointId =
        uUseAnimation
        ? AnimatedPointId2(geo.id, uFrameCount, uPointCount, uTime, uFps)
        : geo.id;

	vec3 p = pointVertexAttributes[animPointId].position.xyz;

    geo.radius = uGrainRadius;

    geo.position_ws = (modelMatrix * vec4(p, 1.0)).xyz;
    vec4 position_cs = viewMatrix * vec4(geo.position_ws, 1.0);

    gl_Position = projectionMatrix * position_cs;
    
    gl_PointSize = SpriteSize(geo.radius, gl_Position);

    geo.id = animPointId%20; // WTF?
    geo.gs_from_ws = randomGrainMatrix(int(geo.id), geo.position_ws);

	if (uPrerenderSurface && uUseOcclusionMap) {
		vec2 fragCoord = resolution.xy * (gl_Position.xy / gl_Position.w * 0.5 + 0.5);
		fragCoord = clamp(fragCoord, vec2(0.5), resolution.xy - vec2(0.5));
		vec3 closerGrain_cs = texelFetch(uOcclusionMap, ivec2(fragCoord.xy), 0).xyz;
		//bool isSurface = linearizeDepth(position_cs.z) < linearizeDepth(closerGrain_cs.z) - geo.radius * 0.01;
		bool isSurface = position_cs.z < closerGrain_cs.z;
		if (uPrerenderSurfaceStep == 0 && isSurface) return;
		if (uPrerenderSurfaceStep == 1 && !isSurface) return;
	}

#ifdef PRECOMPUTE_IN_VERTEX
	Ray ray_ws;
	ray_ws.origin = (inverseViewMatrix * vec4(0, 0, 0, 1)).xyz;
	ray_ws.direction = normalize(geo.position_ws - ray_ws.origin);
    Ray ray_gs = TransformRay(ray_ws, geo.gs_from_ws);

    uint n = uImpostor[0].viewCount;
#  ifdef NO_INTERPOLATION
	geo.i.x = DirectionToViewIndex(-ray_gs.direction, n);
#  else // NO_INTERPOLATION
	DirectionToViewIndices(-ray_gs.direction, n, geo.i, geo.alpha);
#  endif // NO_INTERPOLATION
#endif // PRECOMPUTE_IN_VERTEX

	EmitVertex();
	EndPrimitive();
}

///////////////////////////////////////////////////////////////////////////////
#endif // PASS
// _AUGEN_END_INCLUDE
