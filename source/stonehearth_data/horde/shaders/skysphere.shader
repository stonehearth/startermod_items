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

varying vec2 texCoord;
varying vec4 startCol;
varying vec4 endCol;

void main() {
  texCoord.x = 0;

  // Compress the 
  float t = 1.0 - texCoords0.y;
  texCoord.y = clamp((t * 10.0) - 2.9, 0.0, 1.0);
  
  startCol = skycolor_start;
  endCol = skycolor_end;
  
  gl_Position = projMat * vec4(vertPos, 1.0);
}

[[FS_SKYSPHERE]]

attribute vec4 startCol;
attribute vec4 endCol;
attribute vec2 texCoord;

void main() {
  gl_FragColor = startCol * (1.0 - texCoord.y) + texCoord.y * endCol;
}
