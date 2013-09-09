[[FX]]

// Samplers
sampler2D cloudMap = sampler_state
{
	Texture = "textures/environment/cloudmap.png";
	Address = Wrap;
   Filter = None;
};

// Contexts
context AMBIENT
{
	VertexShader = compile GLSL VS_GENERAL;
	PixelShader = compile GLSL FS_FORWARD_AMBIENT;
	
	ZWriteEnable = true;
}

context ATTRIBPASS
{
	VertexShader = compile GLSL VS_GENERAL;
	PixelShader = compile GLSL FS_AMBIENT;
   CullMode = Back;
}

context SHADOWMAP
{
	VertexShader = compile GLSL VS_SHADOWMAP;
	PixelShader = compile GLSL FS_SHADOWMAP;
   CullMode = Back;
}

context DIRECTIONAL_SHADOWMAP
{
	VertexShader = compile GLSL VS_DIRECTIONAL_SHADOWMAP;
	PixelShader = compile GLSL FS_DIRECTIONAL_SHADOWMAP;
   CullMode = Back;
}

context LIGHTING
{
	VertexShader = compile GLSL VS_GENERAL;
	PixelShader = compile GLSL FS_LIGHTING;
	
	ZWriteEnable = false;
	BlendMode = Add;
   CullMode = Back;
}

context DIRECTIONAL_LIGHTING
{
	VertexShader = compile GLSL VS_GENERAL;
	PixelShader = compile GLSL FS_DIRECTIONAL_LIGHTING;
	
	ZWriteEnable = false;
	BlendMode = Add;
   CullMode = Back;
}

context CLOUDS
{
   VertexShader = compile GLSL VS_GENERAL;
   PixelShader = compile GLSL FS_CLOUDS;

   ZWriteEnable = false;
   BlendMode = Mult;
   CullMode = Back;
}

[[VS_GENERAL]]

#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;
uniform vec3 viewerPos;
attribute vec3 vertPos;
attribute vec3 normal;
attribute vec3 color;
varying vec4 pos, vsPos;
varying vec3 tsbNormal;
varying vec3 albedo;

void main( void )
{
	pos = calcWorldPos( vec4( vertPos, 1.0 ) );
	vsPos = calcViewPos( pos );
   tsbNormal = normal;
   albedo = color;

	// Calculate texture coordinates and clip space position
	gl_Position = viewProjMat * pos;
}


[[FS_AMBIENT]]	

#include "shaders/utilityLib/fragDeferredWrite.glsl" 

uniform vec3 viewerPos;
uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;
uniform sampler2D albedoMap;

varying vec4 pos;
varying vec3 tsbNormal;
varying vec3 albedo;

void main( void )
{
#ifdef _F01_Topsoil
#else
#endif
	vec3 newPos = pos.xyz;
	vec3 normal = tsbNormal;

	setMatID( 1.0 );
	setPos( newPos - viewerPos );
	setNormal( normalize( normal ) );
	setAlbedo( albedo.rgb );
	setSpecParams( matSpecParams.rgb, matSpecParams.a );
}


[[FS_FORWARD_AMBIENT]]	

uniform vec3 viewerPos;
uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;
uniform sampler2D albedoMap;
vec3 ambientLightColor = vec3(0.4, 0.4, 0.4);

varying vec4 pos;
varying vec3 tsbNormal;
varying vec3 albedo;

void main( void )
{
   gl_FragColor.rgb = ambientLightColor * albedo;
}

[[VS_SHADOWMAP]]
// =================================================================================================
	
#include "shaders/utilityLib/vertCommon.glsl"
#include "shaders/utilityLib/vertSkinning.glsl"

uniform mat4 viewProjMat;
uniform vec4 lightPos;
attribute vec3 vertPos;
varying vec3 lightVec;

void main( void )
{
	vec4 pos = calcWorldPos( vec4( vertPos, 1.0 ) );
	lightVec = lightPos.xyz - pos.xyz;
	gl_Position = viewProjMat * pos;
}
	
	
[[FS_SHADOWMAP]]
// =================================================================================================

uniform vec4 lightPos;
uniform float shadowBias;
varying vec3 lightVec;

void main( void )
{
	float dist = length( lightVec ) / lightPos.w;
	gl_FragDepth = dist + shadowBias;
	
	// Clearly better bias but requires SM 3.0
	//gl_FragDepth = dist + abs( dFdx( dist ) ) + abs( dFdy( dist ) ) + shadowBias;
}

[[VS_DIRECTIONAL_SHADOWMAP]]
// =================================================================================================
	
#include "shaders/utilityLib/vertCommon.glsl"
#include "shaders/utilityLib/vertSkinning.glsl"

uniform mat4 viewProjMat;
attribute vec3 vertPos;

void main( void )
{
	vec4 pos = calcWorldPos( vec4( vertPos, 1.0 ) );
	gl_Position = viewProjMat * pos;
}
	
	
[[FS_DIRECTIONAL_SHADOWMAP]]
// =================================================================================================

uniform float shadowBias;
varying vec3 lightVec;

void main( void )
{
	gl_FragDepth = gl_FragCoord.z + 2 * shadowBias;
	// Clearly better bias but requires SM 3.0
	//gl_FragDepth = dist + abs( dFdx( dist ) ) + abs( dFdy( dist ) ) + shadowBias;
}

[[FS_LIGHTING]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 

uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;
uniform sampler2D albedoMap;

varying vec4 pos, vsPos;
varying vec3 albedo;
varying vec3 tsbNormal;

void main( void )
{
	vec3 normal = tsbNormal;
	vec3 newPos = pos.xyz;

	gl_FragColor.rgb = 
         calcPhongSpotLight( newPos, normalize( normal ), albedo, matSpecParams.rgb,
		                    matSpecParams.a, -vsPos.z, 0.3 );
}

[[FS_DIRECTIONAL_LIGHTING]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 

uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;
uniform sampler2D albedoMap;

varying vec4 pos, vsPos;
varying vec3 albedo;
varying vec3 tsbNormal;

void main( void )
{
	gl_FragColor.rgb = 
		calcPhongDirectionalLight( pos, normalize( tsbNormal ), albedo, vec3(0,0,0),
		                    0.0, -vsPos.z, 0.3 );
}

[[FS_CLOUDS]]

varying vec4 pos;
uniform sampler2D cloudMap;
uniform float currentTime;

void main( void )
{
	vec2 fragCoord = pos.xz * 2.0;
   float cloudSpeed = currentTime / 80.0;
   vec4 cloudColor = texture2D(cloudMap, fragCoord.xy / 128.0 + cloudSpeed);
   cloudColor *= texture2D(cloudMap, fragCoord.yx / 192.0 + (cloudSpeed / 10.0));

	gl_FragColor.rgb = cloudColor;
}