[[FX]]

sampler2D lighting = sampler_state
{
  Filter = None;
  Address = Clamp;
};

sampler2D albedo = sampler_state
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
#include "shaders/utilityLib/camera_transforms.glsl"
#include "shaders/utilityLib/fragLighting.glsl" 
#include "shaders/shadows.shader"

uniform sampler2D lighting;
uniform sampler2D albedo;
uniform vec3 camViewerPos;
uniform mat4 camProjMat;
uniform mat4 camViewMatInv;

varying vec2 texCoords;

void main(void)
{
  vec4 light = texture2D(lighting, texCoords);
  vec3 albedo = texture2D(albedo, texCoords).rgb;
  gl_FragColor = vec4((albedo * light.rgb) + light.aaa, 1.0);
}
