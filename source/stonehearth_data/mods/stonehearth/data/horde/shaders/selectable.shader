[[FX]]

sampler2D outlineSampler = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D outlineDepth = sampler_state
{
  Filter = None;
  Address = Clamp;
};

context SELECTED_SCREENSPACE
{
  VertexShader = compile GLSL VS_SELECTED_GENERAL;
  PixelShader = compile GLSL FS_SELECTED_SCREENSPACE;
  ZWriteEnable = true;
  CullMode = None;
}

context SELECTED_FAST
{
  VertexShader = compile GLSL VS_SELECTED_GENERAL;
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


[[VS_SELECTED_GENERAL]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec3 color;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 tsbNormal;
varying vec3 albedo;

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  vsPos = calcViewPos(pos);
  tsbNormal = calcWorldVec(normal);
  albedo = color;

  gl_Position = viewProjMat * pos;
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
