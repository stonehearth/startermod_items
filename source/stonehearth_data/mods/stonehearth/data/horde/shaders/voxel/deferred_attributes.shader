[[FX]]
float4 gridlineColor = { 0.0, 0.0, 0.0, 0.0 };

float4 glossy = { 0.0, 0.0, 0.0, 0.0 };

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
attribute vec3 normal;
attribute vec3 color;

varying float vsDepth;
varying vec3 albedo;
varying vec3 tsbNormal;
varying float worldScale;
varying vec3 gridLineCoords;

void main( void )
{
  vec4 pos = calcWorldPos(vec4(vertPos, 1.0));
  vec4 vsPos = calcViewPos(pos);

  gridLineCoords = pos.xyz + vec3(0.5, 0, 0.5);
  vsDepth = -vsPos.z;
  albedo = color;
  tsbNormal = calcWorldVec(normal);
  worldScale = getWorldScale();
  gl_Position = viewProjMat * pos;
}

[[FS]]
#include "shaders/utilityLib/psCommon.glsl"

uniform mat4 viewMat;
uniform vec4 lodLevels;
uniform sampler3D gridMap;
uniform vec4 gridlineColor;
uniform vec4 glossy;

varying float vsDepth;
varying vec3 albedo;
varying vec3 tsbNormal;
varying float worldScale;
varying vec3 gridLineCoords;

void main(void)
{
  gl_FragData[0].r = vsDepth; //toLinearDepth(gl_FragCoord.z);
  gl_FragData[0].g = worldScale;
  gl_FragData[0].b = glossy.z;
  gl_FragData[0].a = glossy.a;

  gl_FragData[1] = vec4(normalize(tsbNormal), 1.0);

  float f = 1.0;
  if (lodLevels.x == 0.0) {
    if (vsDepth > lodLevels.z) {
      f = 0.0;
    }
  } else {
    if (vsDepth < lodLevels.y) {
      // By the time we see this pixel in the second LOD level, it has already been painted by the first LOD level,
      // just discard, and don't bother contributing.  Maybe we should just stencil mask, since putting discard
      // in a shader can have perf implications?
      discard;
    }
  }
  vec3 color = albedo * f;

  // gridlineAlpha is a single float containing the global opacity of gridlines for all
  // nodes.  gridlineColor is the per-material color of the gridline to use.  Only draw
  // them if both are > 0.0.
  #ifdef DRAW_GRIDLINES
    float gridline = texture3D(gridMap, gridLineCoords).a;
    color = mix(gridlineColor.rgb, color, mix(1.0, gridline, gridlineColor.a));
  #endif

  gl_FragData[2] = vec4(color, 1.0);
}
