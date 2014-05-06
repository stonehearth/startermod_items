[[FX]]

float4 gridlineColor = { 0.0, 0.0, 0.0, 0.0 };

// Samplers
sampler3D gridMap = sampler_state
{
   Texture = "textures/common/gridMap.dds";
   Filter = Bilinear;
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

sampler2D ssaoImage = sampler_state
{
  Filter = None;
  Address = Clamp;
};

context OMNI_LIGHTING_FORWARD
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_OMNI_LIGHTING_FORWARD;
  
  ZWriteEnable = false;
  BlendMode = Add;
  CullMode = Back;
}

context DIRECTIONAL_LIGHTING_FORWARD
{
  VertexShader = compile GLSL VS_GENERAL_SHADOWS;
  PixelShader = compile GLSL FS_DIRECTIONAL_LIGHTING_FORWARD;
  
  ZWriteEnable = false;
  BlendMode = Add;
  CullMode = Back;
}

context OMNI_LIGHTING_FORWARD_POSTPROCESS
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_OMNI_LIGHTING_POSTPROCESS;
  
  ZWriteEnable = false;
  BlendMode = Add;
  CullMode = Back;
}

context DIRECTIONAL_LIGHTING_FORWARD_POSTPROCESS
{
  VertexShader = compile GLSL VS_GENERAL_SHADOWS;
  PixelShader = compile GLSL FS_DIRECTIONAL_LIGHTING_POSTPROCESS;
  
  ZWriteEnable = false;
  BlendMode = Add;
  CullMode = Back;
}

context DEPTH_LINEAR
{
  VertexShader = compile GLSL VS_GENERAL_POSTPROCESS;
  PixelShader = compile GLSL FS_DEPTH_LINEAR;
  CullMode = Back;
}

context DEPTH_LINEAR_BACK
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_DEPTH_LINEAR_BACK;
  ColorWriteMask = A;
  CullMode = Front;
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


[[VS_GENERAL_SHADOWS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;
uniform mat4 shadowMats[4];
uniform mat4 fowViewMat;

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec3 color;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 tsbNormal;
varying vec3 albedo;
varying vec4 projShadowPos[3];
varying vec4 projFowPos;
varying vec3 gridLineCoords;

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  vsPos = calcViewPos(pos);
  tsbNormal = calcWorldVec(normal);
  albedo = color;

  projShadowPos[0] = shadowMats[0] * pos;
  projShadowPos[1] = shadowMats[1] * pos;
  projShadowPos[2] = shadowMats[2] * pos;

  projFowPos = fowViewMat * pos;

  gridLineCoords = vertPos;

  gl_Position = viewProjMat * pos;
}


[[FS_OMNI_LIGHTING_FORWARD]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 

uniform vec3 viewerPos;
uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 albedo;
varying vec3 tsbNormal;

void main( void )
{
  vec3 normal = tsbNormal;
  vec3 newPos = pos.xyz;
  gl_FragColor = vec4(calcPhongOmniLight(viewerPos, newPos, normalize(normal)) * albedo, 1.0);
}


[[FS_DIRECTIONAL_LIGHTING_FORWARD]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 
#include "shaders/shadows.shader"

uniform vec3 lightAmbientColor;
uniform sampler2D cloudMap;
uniform sampler2D fowRT;
uniform float currentTime;
uniform vec4 lodLevels;

varying vec4 vsPos;
varying vec4 pos;
varying vec3 albedo;
varying vec3 tsbNormal;
varying vec4 projFowPos;

void main( void )
{
  // Shadows.
  float shadowTerm = getShadowValue(pos.xyz);

  // Light Color.
  vec3 lightColor = calcSimpleDirectionalLight(normalize(tsbNormal));

  // Mix light and shadow and ambient light.
  lightColor = (shadowTerm * lightColor * albedo) + (lightAmbientColor * albedo);

  // Mix in cloud color.
  float cloudSpeed = currentTime / 80.0;
  vec2 fragCoord = pos.xz * 0.3;
  vec3 cloudColor = texture2D(cloudMap, fragCoord.xy / 128.0 + cloudSpeed).xyz;
  cloudColor = cloudColor * texture2D(cloudMap, fragCoord.yx / 192.0 + (cloudSpeed / 10.0)).xyz;
  lightColor *= cloudColor;

  // Mix in fog of war.
  float fowValue = texture2D(fowRT, projFowPos.xy).a;
  lightColor *= fowValue;

  // Do LOD blending.
  if (lodLevels.x == 0.0) {
    lightColor *= clamp((lodLevels.y - -vsPos.z) / lodLevels.w, 0.0, 1.0);
  } else {
    lightColor *= clamp((-vsPos.z - lodLevels.z) / lodLevels.w, 0.0, 1.0);
  }

  gl_FragColor = vec4(lightColor, 1.0);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// POST-PROCESS SPECIFIC CONTEXTS
////////////////////////////////////////////////////////////////////////////////////////////////////


[[VS_GENERAL_POSTPROCESS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec3 color;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 tsbNormal;
varying vec3 albedo;
varying float worldScale;

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  vsPos = calcViewPos(pos);
  tsbNormal = calcWorldVec(normal);
  albedo = color;
  worldScale = getWorldScale();
  gl_Position = viewProjMat * pos;
}



[[FS_OMNI_LIGHTING_POSTPROCESS]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 

uniform vec3 viewerPos;
uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 albedo;
varying vec3 tsbNormal;

void main( void )
{
  vec3 normal = tsbNormal;
  vec3 newPos = pos.xyz;
  gl_FragColor = vec4(calcPhongOmniLight(viewerPos, newPos, normalize(normal)) * albedo, 1.0);
}


[[FS_DIRECTIONAL_LIGHTING_POSTPROCESS]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 
#include "shaders/shadows.shader"

uniform vec3 lightAmbientColor;
uniform sampler2D cloudMap;
uniform sampler2D fowRT;
uniform sampler2D ssaoImage;
uniform vec2 viewPortSize;
uniform vec2 viewPortPos;
uniform float currentTime;
uniform vec4 lodLevels;
uniform sampler3D gridMap;
uniform float gridlineAlpha;
uniform vec4 gridlineColor;

varying vec4 vsPos;
varying vec4 pos;
varying vec3 albedo;
varying vec3 tsbNormal;
varying vec4 projFowPos;
varying vec3 gridLineCoords;

void main( void )
{
  // Shadows.
  float shadowTerm = getShadowValue(pos.xyz);
  float ssao = texture2D(ssaoImage, (gl_FragCoord.xy - viewPortPos) / viewPortSize).r;

  // Light Color.
  vec3 lightColor = calcSimpleDirectionalLight(normalize(tsbNormal));

  float cloudSpeed = currentTime / 80.0;
  vec2 fragCoord = pos.xz * 0.3;
  vec3 cloudColor = texture2D(cloudMap, fragCoord.xy / 128.0 + cloudSpeed).xyz;
  cloudColor = cloudColor * texture2D(cloudMap, fragCoord.yx / 192.0 + (cloudSpeed / 10.0)).xyz;

  // Mix light and shadow and ambient light.
  lightColor = cloudColor * ((shadowTerm * lightColor * albedo) + (lightAmbientColor - ssao) * albedo);
  
  // Mix in fog of war.
  float fowValue = texture2D(fowRT, projFowPos.xy).a;
  lightColor *= fowValue;

  // Do LOD blending.

  if (lodLevels.x == 0.0) {
    lightColor *= clamp((lodLevels.y - -vsPos.z) / lodLevels.w, 0.0, 1.0);
  } else {
    lightColor *= clamp((-vsPos.z - lodLevels.z) / lodLevels.w, 0.0, 1.0);
  }

  // gridlineAlpha is a single float containing the global opacity of gridlines for all
  // nodes.  gridlineColor is the per-material color of the gridline to use.  Only draw
  // them if both are > 0.0.
  if (gridlineAlpha * gridlineColor.a > 0.0) {
    vec4 gridline = texture3D(gridMap, gridLineCoords + vec3(0.5, 0.0, 0.5));
    gridline = vec4(1.0, 1.0, 1.0, 1.0) - gridline;
    gridline *= gridlineAlpha;
    lightColor = lightColor * (1.0 - gridline.a) + gridline.rgb * gridlineColor.rgb;
  }

  gl_FragColor = vec4(lightColor, 1.0);


}


[[FS_DEPTH_LINEAR]]
#include "shaders/utilityLib/psCommon.glsl"

uniform mat4 viewMat;

varying vec3 tsbNormal;
varying float worldScale;

void main(void)
{
  gl_FragData[0].r = toLinearDepth(gl_FragCoord.z);
  gl_FragData[0].g = worldScale;
  gl_FragData[1] = viewMat * vec4(normalize(tsbNormal), 0.0);
}

[[FS_DEPTH_LINEAR_BACK]]
#include "shaders/utilityLib/psCommon.glsl"

void main(void)
{
  gl_FragData[0].a = toLinearDepth(gl_FragCoord.z);
}
