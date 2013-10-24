[[FX]]

context TRANSLUCENT
{
  VertexShader = compile GLSL VS_TRANSLUCENT;
  PixelShader = compile GLSL FS_TRANSLUCENT;
  
  ZWriteEnable = false;
  BlendMode = Blend;
  CullMode = Back;
}

[[VS_TRANSLUCENT]]
// =================================================================================================
// Cubemitters can take two different paths for rendering (instancing, and the fallback: batching),
// so always use the cubemitter interface to get your data!

#version 130

#include "shaders/utilityLib/cubemitterCommon.glsl"

uniform mat4 viewProjMat;

out vec4 color;

void main(void)
{
  color = cubemitter_getColor();
  gl_Position = viewProjMat * cubemitter_getWorldspacePos();
}


[[FS_TRANSLUCENT]]
// =================================================================================================

#version 130

in vec4 color;

out vec4 fragColor;

void main( void )
{
  fragColor.rgba = color;
}
