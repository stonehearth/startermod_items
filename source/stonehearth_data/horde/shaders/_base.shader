[[FX]]

sampler2D skySampler = sampler_state
{
  Address = Clamp;
  Filter = Trilinear;
};

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

sampler2D depthBuffer = sampler_state
{
  Filter = None;
  Address = Clamp;
};

// Contexts
context DEPTH
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_DEPTH;
  
  ZWriteEnable = true;
}

context SELECTED_SCREENSPACE
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_SELECTED_SCREENSPACE;
  ZWriteEnable = true;
  CullMode = None;
}

context SELECTED_FAST
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_SELECTED_FAST;
  ZWriteEnable = false;
  BlendMode = Add;
  CullMode = None;
}

context SELECTED_SCREENSPACE_OUTLINER
{
  VertexShader = compile GLSL VS_SELECTED_SCREENSPACE_OUTLINER;
  PixelShader = compile GLSL FS_SELECTED_SCREENSPACE_OUTLINER;
  BlendMode = Blend;
  ZWriteEnable = false;
}

context FOG
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_FOG;
  
  ZWriteEnable = false;
  BlendMode = Blend;
  CullMode = Back;
}

context DIRECTIONAL_SHADOWMAP
{
  VertexShader = compile GLSL VS_DIRECTIONAL_SHADOWMAP;
  PixelShader = compile GLSL FS_DIRECTIONAL_SHADOWMAP;
  CullMode = Back;
}

[[VS_GENERAL]]
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


[[FS_DEPTH]]
void main( void )
{
  gl_FragColor = vec4(0, 0, 0, 1);
}


[[FS_FOG]]
// =================================================================================================
#include "shaders/utilityLib/fog.glsl"

uniform sampler2D skySampler;
uniform vec2 frameBufSize;
uniform float farPlane;

varying vec4 vsPos;

void main( void )
{
  float fogFac = calcFogFac(vsPos.z, farPlane);
  vec3 fogColor = texture2D(skySampler, gl_FragCoord.xy / frameBufSize.xy).rgb;

  gl_FragColor = vec4(fogColor, fogFac);
}


[[VS_DIRECTIONAL_SHADOWMAP]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;

void main( void )
{
  vec4 pos = calcWorldPos( vec4( vertPos, 1.0 ) );
  gl_Position = viewProjMat * pos;
}
  
  
[[FS_DIRECTIONAL_SHADOWMAP]]
void main( void )
{
  gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
}


[[FS_SELECTED_SCREENSPACE]]
void main(void)
{
  gl_FragColor = vec4(1, 1, 0, 1);
}


[[VS_SELECTED_SCREENSPACE_OUTLINER]]
uniform mat4 projMat;

attribute vec3 vertPos;

varying vec2 texCoords;
        
void main( void )
{
  texCoords = vertPos.xy;
  gl_Position = projMat * vec4(vertPos, 1.0);
}


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
