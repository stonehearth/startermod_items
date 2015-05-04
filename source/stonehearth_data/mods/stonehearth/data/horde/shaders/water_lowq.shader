[[FX]]

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;
uniform float lowq_waterAmbientLightScale;

attribute vec3 vertPos;
attribute vec4 color;

varying vec4 outColor;

void main() {
   outColor = vec4(color.rgb * lowq_waterAmbientLightScale, color.a);
   gl_Position = viewProjMat * calcWorldPos(vec4(vertPos, 1.0));
}

[[FS]]

varying vec4 outColor;

void main() {
   gl_FragColor = outColor;
}
