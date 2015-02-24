[[FX]]

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;

varying vec4 pos;

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  gl_Position = viewProjMat * pos;
}

[[FS]]
#include "shaders/utilityLib/psCommon.glsl"

void main(void)
{
  gl_FragData[0].a = toLinearDepth(gl_FragCoord.z);
}
