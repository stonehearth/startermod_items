[[FX]]

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;
uniform mat4 fowViewMat;

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec3 color;

varying vec4 pos;
varying vec3 tsbNormal;
varying vec3 albedo;

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  vec4 vsPos = calcViewPos(pos);
  tsbNormal = calcWorldVec(normal);
  albedo = color;

  gl_Position = viewProjMat * pos;
}


[[FS]]
#include "shaders/utilityLib/fragLighting.glsl" 
#include "shaders/omni_shadows.shader"

uniform vec3 viewerPos;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 albedo;
varying vec3 tsbNormal;

void main( void )
{
  float shadowTerm = getOmniShadowValue(lightPos.xyz, pos.xyz);
  gl_FragColor = vec4(calcPhongOmniLight(viewerPos, pos.xyz, normalize(tsbNormal)) * albedo * shadowTerm, 1.0);
}