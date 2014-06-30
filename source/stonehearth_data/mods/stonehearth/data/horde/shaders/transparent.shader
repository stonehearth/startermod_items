[[FX]]

// Contexts
context TRANSLUCENT
{
   VertexShader = compile GLSL VS_TRANSLUCENT;
   PixelShader  = compile GLSL FS_TRANSLUCENT;
   ZWriteEnable = false;
   ZEnable = true;
   BlendMode = Blend;
   CullMode = Back;
}

context SELECTED_SCREENSPACE
{
  VertexShader = compile GLSL VS_TRANSLUCENT;
  PixelShader = compile GLSL FS_SELECTED_SCREENSPACE;
  ZWriteEnable = true;
  CullMode = None;
}

context SELECTED_FAST
{
  VertexShader = compile GLSL VS_TRANSLUCENT;
  PixelShader = compile GLSL FS_SELECTED_FAST;
  ZWriteEnable = false;
  BlendMode = Add;
  CullMode = Back;
}

context SELECTED_SCREENSPACE_OUTLINER
{
  VertexShader = compile GLSL VS_SELECTED_SCREENSPACE_OUTLINER;
  PixelShader = compile GLSL FS_SELECTED_SCREENSPACE_OUTLINER;
  BlendMode = Blend;
  ZWriteEnable = false;
}

[[VS_TRANSLUCENT]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;
attribute vec4 color;

varying vec4 outColor;

void main() {
   outColor = color;
   gl_Position = viewProjMat * calcWorldPos(vec4(vertPos, 1.0));
}

[[FS_TRANSLUCENT]]

varying vec4 outColor;

void main() {
   gl_FragColor = outColor.rgba;
}

[[FS_SELECTED_SCREENSPACE]]
void main(void)
{
  gl_FragColor = vec4(1, 1, 0, 1);
}


[[VS_SELECTED_SCREENSPACE_OUTLINER]]
#include "shaders/fsquad_vs.glsl"


[[FS_SELECTED_SCREENSPACE_OUTLINER]]
#include "shaders/utilityLib/outline.glsl"

uniform sampler2D outlineSampler;
uniform sampler2D outlineDepth;

varying vec2 texCoords;

void main(void)
{
  gl_FragColor = compute_outline_color(outlineSampler, texCoords);
  gl_FragDepth = compute_outline_depth(outlineDepth, texCoords);
}


[[FS_SELECTED_FAST]]
// =================================================================================================

void main( void )
{
  gl_FragColor = vec4(0.5, 0.4, 0.0, 1.0);
}


