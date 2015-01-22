[[FX]]

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;
attribute vec4 color;

varying vec4 outColor;

void main() {
   outColor = color;
   gl_Position = viewProjMat * calcWorldPos(vec4(vertPos, 1.0));
}

[[FS_TRANSLUCENT]]

varying vec4 outColor;

void main() {
   gl_FragColor = outColor.rgba;
}
