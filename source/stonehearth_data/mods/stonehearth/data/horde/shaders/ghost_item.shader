[[FX]]

// Contexts
context BLUEPRINTS_DEPTH_PASS
{
   VertexShader = compile GLSL VS_BLUEPRINTS;
   PixelShader  = compile GLSL FS_BLUEPRINTS_DEPTH_PASS;
   ZWriteEnable = true;
   ZEnable = true;
   BlendMode = Add;
   CullMode = Back;
}

context BLUEPRINTS_COLOR_PASS
{
   VertexShader = compile GLSL VS_BLUEPRINTS;
   PixelShader  = compile GLSL FS_BLUEPRINTS_COLOR_PASS;
   ZWriteEnable = false;
   ZEnable = true;
   BlendMode = Blend;
   CullMode = Back;
}

[[VS_BLUEPRINTS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;
attribute vec4 color;

varying vec4 outColor;

void main() {
   outColor = color;
   gl_Position = viewProjMat * calcWorldPos(vec4(vertPos, 1.0));
}

[[FS_BLUEPRINTS_DEPTH_PASS]]

void main() {
   gl_FragColor = vec4(0, 0, 0, 0);
}

[[FS_BLUEPRINTS_COLOR_PASS]]

varying vec4 outColor;

void main() {
   gl_FragColor = vec4(outColor.rgb, 0.3);
}

