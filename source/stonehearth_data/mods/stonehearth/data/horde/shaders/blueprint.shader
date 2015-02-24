[[FX]]

float4 gridlineColor = { 0.0, 0.0, 0.0, 1.0 };
float blueprintAlpha = 0.25;

sampler3D gridMap = sampler_state
{
   Texture = "textures/common/gridMap.dds";
   Filter = Trilinear;
};

[[VS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;
attribute vec3 color;

varying vec3 gridLineCoords;
varying vec3 outColor;

void main() {
   gridLineCoords = vertPos;
   outColor = color;
   gl_Position = viewProjMat * calcWorldPos(vec4(vertPos, 1.0));
}

[[FS]]

uniform sampler3D gridMap;
uniform vec4 gridlineColor;
uniform float blueprintAlpha;
varying vec3 gridLineCoords;
varying vec3 outColor;

void main() {
   vec4 theColor = vec4(outColor, 1);
   vec4 gridline = texture3D(gridMap, gridLineCoords);
   gridline = vec4(1.0, 1.0, 1.0, 1.0) - gridline;
   float blendAlpha = gridline.a * gridlineColor.a;
   gl_FragColor.rgb = theColor.rgb * (1.0 - blendAlpha) + (gridlineColor.rgb * blendAlpha);
   gl_FragColor.a = blueprintAlpha;
}
