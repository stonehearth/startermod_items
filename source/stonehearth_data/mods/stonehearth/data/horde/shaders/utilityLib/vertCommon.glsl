// *************************************************************************************************
// Horde3D Shader Utility Library
// --------------------------------------
//		- Common functions -
//
// Copyright (C) 2006-2011 Nicolas Schulz
//
// You may use the following code in projects based on the Horde3D graphics engine.
//
// *************************************************************************************************

uniform mat4 viewMat;
uniform mat4 worldMat;
uniform	mat3 worldNormalMat;

#ifdef DRAW_WITH_INSTANCING
attribute mat4 transform;
#endif

vec4 calcWorldPos( const vec4 pos )
{

#ifdef DRAW_WITH_INSTANCING
	return transform * pos;
#else	
	return worldMat * pos;
#endif
}

vec4 calcViewPos( const vec4 pos )
{
	return viewMat * pos;
}

vec3 calcWorldVec( const vec3 vec )
{
#ifdef DRAW_WITH_INSTANCING
	return (transform * vec4(vec, 0)).xyz;
#else
	return (worldMat * vec4(vec, 0)).xyz;
#endif
}

float getWorldScale()
{
  return length(calcWorldVec(normalize(vec3(1.0, 0.0, 0.0))));
}

mat3 calcTanToWorldMat( const vec3 tangent, const vec3 bitangent, const vec3 normal )
{
	return mat3( tangent, bitangent, normal );
}

vec3 calcTanVec( const vec3 vec, const vec3 tangent, const vec3 bitangent, const vec3 normal )
{
	vec3 v;
	v.x = dot( vec, tangent );
	v.y = dot( vec, bitangent );
	v.z = dot( vec, normal );
	return v;
}


uniform float nearPlane;
uniform float farPlane;

// Takes a normalized depth value (as written to the depth buffer), and maps it back into [near, far].
float toLinearDepth(float d)
{
  float z_n = 2.0 * d - 1.0;
  float z_e = 2.0 * nearPlane * farPlane / (farPlane + nearPlane - z_n * (farPlane - nearPlane));
  return z_e;
}  

vec3 toCameraSpace(const mat4 camProjMat, const vec2 fragCoord, float depth)
{
  vec3 result;
  vec4 projInfo = vec4(
    -2.0 / (camProjMat[0][0]),
    -2.0 / (camProjMat[1][1]),
    (1.0 - camProjMat[0][2]) / camProjMat[0][0],
    (1.0 + camProjMat[1][2]) / camProjMat[1][1]);

  result.z = depth;
  result.xy = vec2((fragCoord.xy * projInfo.xy + projInfo.zw) * result.z);

  return result;
}


vec3 toWorldSpace(const vec3 viewerPos, const mat4 camProjMat, const mat3 camRotInv, const vec2 fragCoord, float linear_depth)
{
  vec3 viewPos = toCameraSpace(camProjMat, fragCoord, linear_depth);  
  return -(camRotInv * (viewPos + viewerPos));
}
