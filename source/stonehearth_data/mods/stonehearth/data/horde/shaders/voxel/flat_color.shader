[[FX]]

float4 flat_color;

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

uniform vec4 flat_color;

void main(void)
{
  gl_FragColor = flat_color;
}
