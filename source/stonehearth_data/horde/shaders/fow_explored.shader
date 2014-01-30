[[FX]]

context FOW_RT_EXPLORED {
  VertexShader = compile GLSL VS_FOW;
  PixelShader  = compile GLSL FS_DRAW_ALPHA;
  ZWriteEnable = false;
  ZEnable = false;
  CullMode = None;
}

[[VS_FOW]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;

void main() {
  gl_Position = viewProjMat * calcWorldPos(vec4(vertPos, 1.0));
}

[[FS_FOW]]

void main() {
  gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
}

[[FS_DRAW_ALPHA]]

void main() {
  gl_FragColor = vec4(1.0, 0.0, 0.0, 0.35);
}