[[FX]]

sampler2D skySampler = sampler_state
{
  Address = Clamp;
  Filter = Trilinear;
};


[[VS]]
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


[[FS]]
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
