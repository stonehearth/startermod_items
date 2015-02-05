[[FX]]

sampler2D normals = sampler_state
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

#include "shaders/utilityLib/fragLighting.glsl" 

uniform vec3 lightAmbientColor;
uniform sampler2D normals;

varying vec2 texCoords;

void main(void)
{
  vec3 normal = texture2D(normals, texCoords).rgb;

  // Light Color.
  vec3 lightColor = calcSimpleDirectionalLight(normal);
  gl_FragColor = vec4(lightColor + lightAmbientColor, 1.0);
}
