[[FX]]

sampler2D normals = sampler_state
{
  Filter = None;
  Address = Clamp;
};

sampler2D depths = sampler_state
{
  Filter = None;
  Address = Clamp;
};

float4 glossy = { 0.0, 0.0, 0.0, 0.65 };

[[VS]]

uniform mat4 projMat;
attribute vec3 vertPos;
varying vec2 texCoords;
        
void main(void)
{
  texCoords = vertPos.xy; 
  gl_Position = projMat * vec4(vertPos, 1.0);
}


[[FS]]
#version 120
#include "shaders/utilityLib/camera_transforms.glsl"
#include "shaders/utilityLib/fragLighting.glsl" 
#include "shaders/shadows.shader"

uniform vec3 lightAmbientColor;
uniform sampler2D normals;
uniform sampler2D depths;
uniform vec3 camViewerPos;
uniform mat4 camProjMat;
uniform mat4 camViewMatInv;
uniform vec4 glossy;

varying vec2 texCoords;

void main(void)
{
  vec4 normal = texture2D(normals, texCoords);

  // Check to see if a valid normal was even written!
  if (normal.w == 0.0) {
  	discard;
  }

  float shadowTerm = 1.0;
  vec4 depthInfo = texture2D(depths, texCoords);

  vec3 pos = toWorldSpace(camViewerPos, camProjMat, mat3(camViewMatInv), texCoords, depthInfo.r);
  #ifndef DISABLE_SHADOWS
    shadowTerm = getShadowValue_deferred(pos);
  #endif

  // Light Color.
  //vec3 lightColor = calcSimpleDirectionalLight(normal.xyz) * shadowTerm;
  //gl_FragColor = vec4(lightColor + lightAmbientColor, 0.0);

  vec4 lightColor = calcPhongDirectionalLight(camViewerPos, pos, normal.xyz, depthInfo.a) * shadowTerm;
  gl_FragColor = vec4(lightColor.rgb + lightAmbientColor, lightColor.a);
}
