[[FX]]
float4 alpha = {0, 0, 0, 0.5};

context TRANSLUCENT
{
	VertexShader = compile GLSL VS_GENERAL;
	PixelShader = compile GLSL FS_AMBIENT;
	ZWriteEnable = false;
	BlendMode = Blend;
}


[[VS_GENERAL]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform   mat4    viewProjMat;
uniform   vec4    alpha;

attribute vec3    vertPos;
attribute vec3    color;
varying   vec4    vs_color;

void main() {
  vs_color = vec4(color, alpha.a);
  gl_Position = viewProjMat * calcWorldPos(vec4(vertPos, 1.0));
}

[[FS_AMBIENT]]	

varying vec4 vs_color;

void main() {
  gl_FragColor = vs_color;
}
