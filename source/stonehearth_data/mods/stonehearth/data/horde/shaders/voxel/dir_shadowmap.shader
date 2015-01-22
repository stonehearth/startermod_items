[[FX]]

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;

void main(void)
{
  vec4 pos = calcWorldPos(vec4(vertPos, 1.0));
  gl_Position = viewProjMat * pos;
}
  
  
[[FS]]
void main(void)
{
  gl_FragColor = vec4(0.0);
}
