[[FX]]

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

sampler2D depths = sampler_state
{
  Filter = None;
  Address = Clamp;
};

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 projMat;
attribute vec3 vertPos;
varying vec2 texCoords;

void main( void )
{
  texCoords = vertPos.xy; 
  gl_Position = projMat * vec4(vertPos, 1.0);
}


[[FS]]
// =================================================================================================
#version 120
#include "shaders/utilityLib/fragLighting.glsl" 
#include "shaders/utilityLib/camera_transforms.glsl"

uniform sampler2D cloudMap;
uniform sampler2D fowRT;
uniform sampler2D depths;
uniform float currentTime;
uniform vec3 camViewerPos;
uniform mat4 camProjMat;
uniform mat4 camViewMatInv;
uniform mat4 fowViewMat;

varying vec2 texCoords;

void main(void)
{
  vec3 pos = toWorldSpace(camViewerPos, camProjMat, mat3(camViewMatInv), texCoords, texture2D(depths, texCoords).r);
  vec4 projFowPos = fowViewMat * vec4(pos, 1.0);

  // Mix in cloud color.
  float cloudSpeed = currentTime / 80.0;
  vec2 fragCoord = pos.xz * 0.3;
  vec3 cloudColor = texture2D(cloudMap, fragCoord.xy / 128.0 + cloudSpeed).rgb;
  cloudColor = cloudColor * texture2D(cloudMap, fragCoord.yx / 192.0 + (cloudSpeed / 10.0)).rgb;

  // Mix in fog of war.
  vec3 fowValue = vec3(1.0 - texture2D(fowRT, projFowPos.xy).a);
  cloudColor = vec3(1.0) - cloudColor;

  gl_FragColor = vec4(cloudColor + fowValue, 0.0);
}