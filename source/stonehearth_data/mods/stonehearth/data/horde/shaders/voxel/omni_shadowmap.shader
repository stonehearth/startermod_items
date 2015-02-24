[[FX]]

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;

varying vec3 worldPos;

void main(void)
{
  vec4 pos = calcWorldPos(vec4(vertPos, 1.0));
  worldPos = pos.xyz;
  gl_Position = viewProjMat * pos;
}
  
  
[[FS]]
uniform vec4 lightPos;
varying vec3 worldPos;

void main(void)
{
  gl_FragColor.r = length(worldPos - lightPos.xyz);
}
