[[FX]]


[[VS]]
// Cubemitters can take two different paths for rendering (instancing, and the fallback: batching),
// so always use the cubemitter interface to get your data!

#include "shaders/utilityLib/cubemitterCommon.glsl"

uniform mat4 viewProjMat;

varying vec4 color;

void main(void)
{
  color = cubemitter_getColor();
  gl_Position = viewProjMat * cubemitter_getWorldspacePos();
}


[[FS]]
varying vec4 color;

void main( void )
{
  gl_FragColor = color;
}
