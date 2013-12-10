[[FX]]

float4 skycolor_start;
float4 skycolor_end;

// Contexts
context SKY
{
  VertexShader = compile GLSL VS_SKYSPHERE;
  PixelShader  = compile GLSL FS_SKYSPHERE;
  ZWriteEnable = false;
  ZEnable = false;
  CullMode = None;
}

[[VS_SKYSPHERE]]

uniform mat4 projMat;

uniform vec4 skycolor_start;
uniform vec4 skycolor_end;

attribute vec3 vertPos;
attribute vec2 texCoords0;

varying float colorT;
varying vec4 startCol;
varying vec4 endCol;

void main() {
  // Compress the gradient to occupy the most visible band of the sky.

  colorT = (10 * texCoords0.y) - 2.9, 0.0, 1.0;
  
  startCol = skycolor_start;
  endCol = skycolor_end;
  
  gl_Position = projMat * vec4(vertPos, 1.0);
}

[[FS_SKYSPHERE]]

attribute float colorT;
attribute vec4 startCol;
attribute vec4 endCol;

void main() {
  float clampedT = clamp(colorT, 0.0, 1.0);
  gl_FragColor = startCol * (1.0 - clampedT) + clampedT * endCol;
}
