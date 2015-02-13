[[FX]]

context DIRECTIONAL_LIGHTING_FORWARD_POSTPROCESS
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_DIRECTIONAL_LIGHTING_POSTPROCESS;
  
  ZWriteEnable = false;
  BlendMode = Add;
  CullMode = Back;
}


[[VS_GENERAL]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;

varying vec4 pos;

void main( void )
{
  gl_Position = viewProjMat * calcWorldPos(vec4(vertPos, 1.0));
}



[[FS_DIRECTIONAL_LIGHTING_POSTPROCESS]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 
#include "shaders/shadows.shader"

void main( void )
{

  gl_FragColor = vec4(1.0);

}