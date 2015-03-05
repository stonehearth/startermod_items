[[FX]]

float4 gridlineColor = { 0.0, 0.0, 0.0, 0.0 };

// Samplers
sampler3D gridMap = sampler_state
{
   Texture = "textures/common/gridMap.dds";
   Filter = Trilinear;
};

sampler2D cloudMap = sampler_state
{
  Texture = "textures/environment/cloudmap.png";
  Address = Wrap;
  Filter = Pixely;
};

sampler2D fowRT = sampler_state
{
  Filter = None;
  Address = Clamp;
};

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;
uniform mat4 shadowMats[4];
uniform mat4 fowViewMat;

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec4 color;

varying vec4 pos;
varying vec3 tsbNormal;
varying vec3 albedo;
varying vec4 projShadowPos[3];
varying vec4 projFowPos;
varying vec3 gridLineCoords;

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  vec4 vsPos = calcViewPos(pos);
  tsbNormal = calcWorldVec(normal);
  albedo = color.rgb;

#ifndef DISABLE_SHADOWS
  projShadowPos[0] = shadowMats[0] * pos;
  projShadowPos[1] = shadowMats[1] * pos;
  projShadowPos[2] = shadowMats[2] * pos;
#endif

  projFowPos = fowViewMat * pos;

  gridLineCoords = pos.xyz + vec3(0.5, 0, 0.5);

  gl_Position = viewProjMat * pos;

  // Yuck!  But this saves us an entire vec4, which can kill older cards.
  pos.w = vsPos.z;
}


[[FS]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 
#include "shaders/shadows.shader"

uniform sampler3D gridMap;
uniform sampler2D cloudMap;
uniform sampler2D fowRT;
uniform vec4 gridlineColor;
uniform vec4 lodLevels;
uniform vec3 lightAmbientColor;
uniform float currentTime;

varying vec4 pos;
varying vec4 projFowPos;
varying vec3 albedo;
varying vec3 tsbNormal;
varying vec3 gridLineCoords;

void main( void )
{
  // Shadows.
  float shadowTerm = getShadowValue(pos.xyz);

  // Light Color.
  vec3 lightColor = calcSimpleDirectionalLight(normalize(tsbNormal));

  // Mix light and shadow and ambient light.
  lightColor = albedo * (shadowTerm * lightColor + lightAmbientColor);

  // Mix in cloud color.
  float cloudSpeed = currentTime / 80.0;
  vec2 fragCoord = pos.xz * 0.3;
  vec3 cloudColor = texture2D(cloudMap, fragCoord.xy / 128.0 + cloudSpeed).xyz;
  cloudColor = cloudColor * texture2D(cloudMap, fragCoord.yx / 192.0 + (cloudSpeed / 10.0)).xyz;
  lightColor *= cloudColor;

  // Mix in fog of war.
  float fowValue = texture2D(fowRT, projFowPos.xy).a;
  lightColor *= fowValue;


  // Do LOD blending.  Note that pos.w is view-space 'z' coordinate (see VS.)
  // First, compute our lod percentage function--this will look like:
  // 0 for the lod level 0, (0, 1) for between the lod levels, and 1 for lod level 1.
  float f = clamp((lodLevels.y + pos.w) / lodLevels.w, 0.0, 1.0);

  // Next, if we're rendering at lod 1, we actually want to invert this.
  f = abs(lodLevels.x - f);

  lightColor *= f; 

  // gridlineAlpha is a single float containing the global opacity of gridlines for all
  // nodes.  gridlineColor is the per-material color of the gridline to use.  Only draw
  // them if both are > 0.0.
  #ifdef DRAW_GRIDLINES
    float gridline = texture3D(gridMap, gridLineCoords).a;
    lightColor = mix(gridlineColor.rgb, lightColor, mix(1.0, gridline, gridlineColor.a));
  #endif

  gl_FragColor = vec4(lightColor, 1.0);
}