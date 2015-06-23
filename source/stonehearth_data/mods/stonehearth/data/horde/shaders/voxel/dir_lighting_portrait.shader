[[FX]]

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;
uniform mat4 shadowMats[4];

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec4 color;

varying vec4 pos;
varying vec3 tsbNormal;
varying vec3 albedo;

#ifndef DISABLE_SHADOWS
varying vec4 projShadowPos[3];
#endif

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  vec4 vsPos = calcViewPos(pos);
  tsbNormal = calcWorldVec(normal);
  albedo = color.rgb;

#ifndef DISABLE_SHADOWS
  projShadowPos[0] = shadowMats[0] * pos;
  projShadowPos[1] = shadowMats[1] * pos;
  projShadowPos[2] = shadowMats[2] * pos;
#endif

  gl_Position = viewProjMat * pos;

  // Yuck!  But this saves us an entire vec4, which can kill older cards.
  pos.w = vsPos.z;
}


[[FS]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 

#ifndef DISABLE_SHADOWS
varying vec4 projShadowPos[3];
#include "shaders/shadows.shader"
#endif

uniform vec3 lightAmbientColor;

varying vec4 pos;
varying vec3 albedo;
varying vec3 tsbNormal;


void main( void )
{
  // Shadows.
  float shadowTerm = 1.0;

#ifndef DISABLE_SHADOWS
  shadowTerm = getShadowValue(pos.xyz);
#endif

  // Light Color.
  vec3 lightColor = calcSimpleDirectionalLight(normalize(tsbNormal));

  // Mix light and shadow and ambient light.
  lightColor = albedo * (shadowTerm * lightColor + lightAmbientColor);

  gl_FragColor = vec4(lightColor, 1.0);
}