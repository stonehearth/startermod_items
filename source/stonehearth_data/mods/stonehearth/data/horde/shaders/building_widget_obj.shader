[[FX]]

float4 widgetColor = { 1.0, 1.0, 0.0, 0.5 };

context BLUEPRINTS_DEPTH_PASS
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_BLUEPRINTS_DEPTH_PASS;
  ZWriteEnable = true; 
  ZEnable = true;
  BlendMode = Add;
  CullMode = Back;
}

context BLUEPRINTS_COLOR_PASS
{
   VertexShader = compile GLSL VS_GENERAL;
   PixelShader  = compile GLSL FS_BLUEPRINTS_COLOR_PASS;
   ZWriteEnable = false;
   ZEnable = true;
   BlendMode = Blend;
   CullMode = Back;
}

[[VS_GENERAL]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;
attribute vec3 vertPos;
varying vec4 pos;
uniform vec4 blueprintColor;
varying vec4 outColor;

void main( void )
{
  gl_Position = viewProjMat * calcWorldPos(vec4(vertPos, 1.0));
}

[[FS_BLUEPRINTS_DEPTH_PASS]]

varying vec3 gridLineCoords;

void main() {
   gl_FragColor = vec4(0, 0, 0, 0);
}


[[FS_BLUEPRINTS_COLOR_PASS]]
// =================================================================================================

uniform vec4 widgetColor;

void main( void )
{
  gl_FragColor = widgetColor;
}