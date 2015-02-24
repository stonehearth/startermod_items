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
#include "shaders/utilityLib/fragLighting.glsl" 
#include "shaders/utilityLib/camera_transforms.glsl"

uniform mat4 camProjMat;
uniform mat4 camViewMatInv;
uniform vec3 camViewerPos;
uniform vec3 lightAmbientColor;
uniform sampler2D normals;
uniform sampler2D depths;

varying vec2 texCoords;

void main(void)
{
  vec4 normal = texture2D(normals, texCoords);
  // Check to see if a valid normal was even written!
  if (normal.w == 0.0) {
  	discard;
  }

  vec4 depthAttribs = texture2D(depths, texCoords);
  vec3 pos = toWorldSpace(camProjMat, camViewMatInv, texCoords, depthAttribs.r);

  // Light Color.
  gl_FragColor = calcPhongOmniLight(camViewerPos, pos, normal.xyz, depthAttribs.b, depthAttribs.a);
}
