[[FX]]

// Samplers
sampler2D cloudMap = sampler_state
{
  Texture = "textures/environment/cloudmap.png";
  Address = Wrap;
   Filter = None;
};

sampler2D lightingBuffer = sampler_state
{
  Address = Clamp;
  Filter = Bilinear;
};

sampler2D ssaoBuffer = sampler_state
{
  Address = Clamp;
  Filter = Bilinear;
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

context DEPTH_AND_LIGHT_ATTRIBUTES
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_DEFERRED_DEPTH_AND_LIGHT;
}

context MATERIAL
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_DEFERRED_MATERIAL;  
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

context OMNI_LIGHTING
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_OMNI_LIGHTING;
  
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
#version 150
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;
uniform vec3 viewerPos;
in vec3 vertPos;
in vec3 normal;
in vec3 color;
out vec4 pos;
out vec4 vsPos;
out vec3 tsbNormal;
out vec3 albedo;

void main( void )
{
  pos = calcWorldPos( vec4( vertPos, 1.0 ) );
  vsPos = calcViewPos( pos );
  tsbNormal = calcWorldVec(normal);
  albedo = color;

  gl_Position = viewProjMat * pos;;
}


[[FS_AMBIENT]]  
#version 150
#include "shaders/utilityLib/fragDeferredWrite.glsl" 

uniform vec3 viewerPos;
uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;
uniform sampler2D albedoMap;

in vec4 pos;
in vec3 tsbNormal;
in vec3 albedo;

void main( void )
{
  vec3 newPos = pos.xyz;
  vec3 normal = tsbNormal;

  setMatID( 1.0 );
  setPos( newPos - viewerPos );
  setNormal( normalize( normal ) );
  setAlbedo( albedo.rgb );
  setSpecParams( matSpecParams.rgb, matSpecParams.a );
}


[[FS_DEFERRED_DEPTH_AND_LIGHT]] 
#version 150
uniform vec3 viewerPos;
in vec4 pos;
in vec3 tsbNormal;

void main( void )
{
  gl_FragData[0].rgb = normalize(tsbNormal).xyz;
  gl_FragData[1].rgb = pos.xyz - viewerPos;
}


[[FS_FORWARD_AMBIENT]]  

uniform vec3 viewerPos;
uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;
uniform sampler2D albedoMap;

void main( void )
{
   gl_FragColor.rgb = vec3(0, 0, 0);
}

[[VS_SHADOWMAP]]
// =================================================================================================
  
#include "shaders/utilityLib/vertCommon.glsl"
#include "shaders/utilityLib/vertSkinning.glsl"

uniform mat4 viewProjMat;
uniform vec4 lightPos;
in vec3 vertPos;
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
in vec3 lightVec;

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
in vec3 vertPos;

void main( void )
{
  vec4 pos = calcWorldPos( vec4( vertPos, 1.0 ) );
  gl_Position = viewProjMat * pos;
}
  
  
[[FS_DIRECTIONAL_SHADOWMAP]]
// =================================================================================================

uniform float shadowBias;
//varying vec3 lightVec;

void main( void )
{
  gl_FragDepth = gl_FragCoord.z + 2.0 * shadowBias;
  // Clearly better bias but requires SM 3.0
  //gl_FragDepth = dist + abs( dFdx( dist ) ) + abs( dFdy( dist ) ) + shadowBias;
}

[[FS_OMNI_LIGHTING]]
// =================================================================================================
#version 150
#include "shaders/utilityLib/fragLighting.glsl" 

uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;
uniform sampler2D albedoMap;

in vec4 pos;
in vec4 vsPos;
in vec3 albedo;
in vec3 tsbNormal;

void main( void )
{
  vec3 normal = tsbNormal;
  vec3 newPos = pos.xyz;

  gl_FragColor.rgb = 
         calcPhongOmniLight( newPos, normalize( normal ), albedo );
}

[[FS_DIRECTIONAL_LIGHTING]]
// =================================================================================================
#version 150
#include "shaders/utilityLib/fragLighting.glsl" 

uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;
uniform sampler2D albedoMap;
uniform vec3 lightAmbientColor;

in vec4 pos;
in vec4 vsPos;
in vec3 albedo;
in vec3 tsbNormal;

void main( void )
{
  vec3 lightColor = 
    calcPhongDirectionalLight( pos.xyz, normalize( tsbNormal ), albedo, vec3(0,0,0),
                        0.0, -vsPos.z, 0.3 ) + (lightAmbientColor * albedo);
  gl_FragColor = vec4(lightColor, 1.0);
}

[[FS_CLOUDS]]
#version 150
in vec4 pos;
uniform sampler2D cloudMap;
uniform float currentTime;

void main( void )
{
  vec2 fragCoord = pos.xz * 0.3;
  float cloudSpeed = currentTime / 80.0;
  vec4 cloudColor = texture2D(cloudMap, fragCoord.xy / 128.0 + cloudSpeed);
  cloudColor *= texture2D(cloudMap, fragCoord.yx / 192.0 + (cloudSpeed / 10.0));

  gl_FragColor = cloudColor;
}


[[FS_DEFERRED_MATERIAL]]

uniform sampler2D lightingBuffer;
uniform sampler2D ssaoBuffer;
uniform vec2 frameBufSize;
in vec3 albedo;

void main(void)
{
  vec2 fragCoord = vec2(gl_FragCoord.xy / frameBufSize);
  vec3 lightColor = texture2D(lightingBuffer, fragCoord).xyz;
  float ssaoIntensity = texture2D(ssaoBuffer, fragCoord).x;
  gl_FragColor = vec4(lightColor * albedo * ssaoIntensity, 1.0);
}
