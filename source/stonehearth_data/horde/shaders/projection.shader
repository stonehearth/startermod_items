[[FX]]

// Samplers
sampler2D image = sampler_state
{
  Texture = "textures/environment/twinkle.png";
  Address = Clamp;
  Filter = Pixely;
};

// Contexts
context PROJECTION
{
  VertexShader = compile GLSL VS_PROJECTION;
  PixelShader  = compile GLSL FS_PROJECTION;
  ZWriteEnable = false;
  BlendMode = Mult;
}

[[VS_PROJECTION]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 projectorMat;
uniform mat4 viewProjMat;

attribute vec3 vertPos;

varying vec4 pos;
varying vec2 texCoords;

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  texCoords = (projectorMat * pos).xz;

  gl_Position = viewProjMat * pos;
}


[[FS_PROJECTION]]
uniform sampler2D image;

varying vec2 texCoords;

void main() {
  gl_FragColor = texture2D(image, texCoords);
}
