[[FX]]

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;
attribute vec3 normal;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 tsbNormal;
varying float worldScale;

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  tsbNormal = calcWorldVec(normal);
  worldScale = getWorldScale();
  gl_Position = viewProjMat * pos;
}

[[FS]]
#include "shaders/utilityLib/psCommon.glsl"

uniform mat4 viewMat;

varying vec3 tsbNormal;
varying float worldScale;

void main(void)
{
  gl_FragData[0].r = toLinearDepth(gl_FragCoord.z);
  gl_FragData[0].g = worldScale;
  gl_FragData[1] = viewMat * vec4(normalize(tsbNormal), 0.0);
}
