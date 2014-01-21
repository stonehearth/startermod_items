[[FX]]

// Contexts
context FOW_RESET_FRONT_EXPLORED {
  VertexShader = compile GLSL VS_FOW;
  PixelShader  = compile GLSL FS_FOW;
  ZWriteEnable = false;
  ZEnable = true;
  CullMode = Front;
  StencilOp = Keep_Dec_Dec;
  ColorWrite = false;
  AlphaWrite = false;
}

context FOW_RESET_BACK_EXPLORED {
  VertexShader = compile GLSL VS_FOW;
  PixelShader  = compile GLSL FS_FOW;
  ZWriteEnable = false;
  ZEnable = true;
  CullMode = Back;
  StencilOp = Keep_Inc_Inc;
  ColorWrite = false;
  AlphaWrite = false;
}

context FOW_FRONT_EXPLORED {
  VertexShader = compile GLSL VS_FOW;
  PixelShader  = compile GLSL FS_FOW;
  ZWriteEnable = false;
  ZEnable = true;
  CullMode = Front;
  StencilOp = Keep_Keep_Inc;
  ColorWrite = false;
  AlphaWrite = false;
}

context FOW_BACK_EXPLORED {
  VertexShader = compile GLSL VS_FOW;
  PixelShader  = compile GLSL FS_FOW;
  ZWriteEnable = false;
  ZEnable = true;
  CullMode = Back;
  StencilOp = Keep_Keep_Dec;
  ColorWrite = false;
  AlphaWrite = false;
}

context FOW_DRAW_ALPHA_EXPLORED {
  VertexShader = compile GLSL VS_FOW;
  PixelShader  = compile GLSL FS_DRAW_ALPHA;
  ZWriteEnable = false;
  ZEnable = false;
  CullMode = Front;
  ColorWrite = false;
  AlphaWrite = true;
  StencilFunc = Greater;
  StencilRef = 128;
}

[[VS_FOW]]

uniform mat4 worldMat;
uniform mat4 viewProjMat;

attribute vec3 vertPos;
void main() {
  gl_Position = viewProjMat * worldMat * vec4(vertPos, 1.0);
}

[[FS_FOW]]

void main() {
  gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
}

[[FS_DRAW_ALPHA]]

void main() {
  gl_FragColor = vec4(0.0, 0.0, 0.0, 0.5 );
}