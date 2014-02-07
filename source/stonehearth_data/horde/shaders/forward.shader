[[FX]]

// Samplers
sampler2D cloudMap = sampler_state
{
  Texture = "textures/environment/cloudmap.png";
  Address = Wrap;
  Filter = Pixely;
};


context OMNI_LIGHTING_FORWARD
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_OMNI_LIGHTING;
  
  ZWriteEnable = false;
  BlendMode = Add;
  CullMode = Back;
}

context DIRECTIONAL_LIGHTING_FORWARD
{
  VertexShader = compile GLSL VS_GENERAL_SHADOWS;
  PixelShader = compile GLSL FS_DIRECTIONAL_LIGHTING;
  
  ZWriteEnable = false;
  BlendMode = Add;
  CullMode = Back;
}

[[VS_GENERAL]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec3 color;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 tsbNormal;
varying vec3 albedo;

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  vsPos = calcViewPos(pos);
  tsbNormal = calcWorldVec(normal);
  albedo = color;

  gl_Position = viewProjMat * pos;
}


[[VS_GENERAL_SHADOWS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;
uniform mat4 shadowMats[4];

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec3 color;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 tsbNormal;
varying vec3 albedo;
varying vec4 projShadowPos[3];

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  vsPos = calcViewPos(pos);
  tsbNormal = calcWorldVec(normal);
  albedo = color;

  projShadowPos[0] = shadowMats[0] * pos;
  projShadowPos[1] = shadowMats[1] * pos;
  projShadowPos[2] = shadowMats[2] * pos;

  gl_Position = viewProjMat * pos;
}


[[FS_OMNI_LIGHTING]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 

uniform vec3 viewerPos;
uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 albedo;
varying vec3 tsbNormal;

void main( void )
{
  vec3 normal = tsbNormal;
  vec3 newPos = pos.xyz;
  gl_FragColor = vec4(calcPhongOmniLight(viewerPos, newPos, normalize(normal)) * albedo, 1.0);
}


[[FS_DIRECTIONAL_LIGHTING]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 
#include "shaders/shadows.shader"

uniform vec3 lightAmbientColor;
uniform sampler2D cloudMap;
uniform float currentTime;

varying vec4 pos;
varying vec3 albedo;
varying vec3 tsbNormal;

void main( void )
{
  // Shadows.
  float shadowTerm = getShadowValue(pos.xyz);

  // Light Color.
  vec3 lightColor = calcSimpleDirectionalLight(normalize(tsbNormal));

  float cloudSpeed = currentTime / 80.0;
  vec2 fragCoord = pos.xz * 0.3;
  vec3 cloudColor = texture2D(cloudMap, fragCoord.xy / 128.0 + cloudSpeed).xyz;
  cloudColor = cloudColor * texture2D(cloudMap, fragCoord.yx / 192.0 + (cloudSpeed / 10.0)).xyz;

  // Mix light and shadow and ambient light.
  lightColor = cloudColor *((shadowTerm * (lightColor * albedo)) + (lightAmbientColor * albedo));
  
  gl_FragColor = vec4(lightColor, 1.0);
}
