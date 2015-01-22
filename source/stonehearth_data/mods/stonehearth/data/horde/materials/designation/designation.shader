[[FX]]
float4 alpha = {0, 0, 0, 0.5};

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform    mat4    viewProjMat;
uniform    vec4    alpha;
attribute  vec3    vertPos;
attribute  vec3    color;
varying    vec4    vs_color;

void main() {
   vs_color = vec4(color, alpha.a);
   gl_Position = viewProjMat * calcWorldPos(vec4(vertPos, 1.0));
}

[[FS]]	
varying vec4 vs_color;
void main() {
  gl_FragColor = vs_color;
}
