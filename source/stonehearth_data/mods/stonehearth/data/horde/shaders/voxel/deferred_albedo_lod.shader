[[FX]]
float4 gridlineColor = { 0.0, 0.0, 0.0, 0.0 };

// Samplers
sampler3D gridMap = sampler_state
{
   Texture = "textures/common/gridMap.dds";
   Filter = Trilinear;
};

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;
attribute vec3 color;

varying float vsDepth;
varying vec3 albedo;
varying vec3 gridLineCoords;

void main( void )
{
  vec4 pos = calcWorldPos(vec4(vertPos, 1.0));
  vec4 vsPos = calcViewPos(pos);

  gridLineCoords = pos.xyz + vec3(0.5, 0, 0.5);
  vsDepth = -vsPos.z;
  albedo = color;
  gl_Position = viewProjMat * pos;
}

[[FS]]
#include "shaders/utilityLib/psCommon.glsl"

uniform mat4 viewMat;
uniform vec4 lodLevels;
uniform sampler3D gridMap;
uniform vec4 gridlineColor;

varying float vsDepth;
varying vec3 albedo;
varying vec3 gridLineCoords;

void main(void)
{
  // First, compute our lod percentage function--this will look like:
  // 0 for the lod level 0, (0, 1) for between the lod levels, and 1 for lod level 1.
  float f = clamp((lodLevels.y - vsDepth) / lodLevels.w, 0.0, 1.0);

  // Next, if we're rendering at lod 1, we actually want to invert this.
  f = abs(lodLevels.x - f);

  // Finally, we actually want to remove the part where we're at lod 1 (since this will already
  // be drawn), so subtract out that piece.
  f = f - step(1.0, f);

  vec3 color = albedo * f;

  // gridlineAlpha is a single float containing the global opacity of gridlines for all
  // nodes.  gridlineColor is the per-material color of the gridline to use.  Only draw
  // them if both are > 0.0.
  #ifdef DRAW_GRIDLINES
    float gridline = texture3D(gridMap, gridLineCoords).a;
    color = mix(gridlineColor.rgb, color, mix(1.0, gridline, gridlineColor.a));
  #endif

  gl_FragColor = vec4(color, 1.0);
}
