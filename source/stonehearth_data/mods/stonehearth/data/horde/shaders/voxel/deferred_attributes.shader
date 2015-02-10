[[FX]]

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec3 color;

varying float vsDepth;
varying vec3 albedo;
varying vec3 tsbNormal;
varying float worldScale;

void main( void )
{
  vec4 pos = calcWorldPos(vec4(vertPos, 1.0));
  vec4 vsPos = calcViewPos(pos);

  vsDepth = vsPos.z;
  albedo = color;
  tsbNormal = calcWorldVec(normal);
  worldScale = getWorldScale();
  gl_Position = viewProjMat * pos;
}

[[FS]]
#include "shaders/utilityLib/psCommon.glsl"

uniform mat4 viewMat;
uniform vec4 lodLevels;

varying float vsDepth;
varying vec3 albedo;
varying vec3 tsbNormal;
varying float worldScale;

void main(void)
{
  gl_FragData[0].r = toLinearDepth(gl_FragCoord.z);
  gl_FragData[0].g = worldScale;
  gl_FragData[1] = vec4(normalize(tsbNormal), 1.0);
  gl_FragData[2] = vec4(albedo, 1.0);
}
