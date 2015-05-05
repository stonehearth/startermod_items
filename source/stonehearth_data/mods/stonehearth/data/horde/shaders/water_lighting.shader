[[FX]]

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;
uniform vec3 lightAmbientColor;
uniform vec3 lightColor;

attribute vec3 vertPos;
attribute vec4 color;
attribute vec3 normal;

varying vec4 outColor;
varying vec3 tsbNormal;

void main() {
   outColor = color;
   tsbNormal = -calcWorldVec(normal);
   gl_Position = viewProjMat * calcWorldPos(vec4(vertPos, 1.0));
}

[[FS]]

uniform vec4 lightDir;
uniform vec3 lightAmbientColor;
uniform vec3 lightColor;

varying vec4 outColor;
varying vec3 tsbNormal;

void main() {

   // We're trying to balance realism and effect, here.  Even if the angle between our dot
   // and the surface is small, make sure we don't go completely 0.
   float angleBias = 0.3;

   // Again, balancing realism and effect; only consider the rg components of the light lets us
   // have dark water at night, and bright blue water during the day.  The transition is
   // reasonable, too.
   float lightIntensity = length(lightAmbientColor.rg);
   float intensity = min(1.0, (lightIntensity * max(0.0, angleBias + dot(tsbNormal, lightDir.xyz))));
   gl_FragColor = vec4(outColor.rgb * intensity, 0.0);
}
