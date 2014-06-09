[[FX]]

float4 gridlineColor = { 0.0, 0.0, 0.0, 1.0 };
float blueprintAlpha = 0.25;

sampler3D gridMap = sampler_state
{
   Texture = "textures/common/gridMap.dds";
   Filter = Trilinear;
};

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

uniform mat4 viewProjMat;
uniform mat4 worldMat;

attribute vec3 vertPos;
attribute vec3 color;

varying vec3 gridLineCoords;
varying vec3 outColor;

void main() {
   gridLineCoords = vertPos;
   outColor = color;
   gl_Position = viewProjMat * worldMat * vec4(vertPos, 1.0);
}

[[FS_BLUEPRINTS_DEPTH_PASS]]

varying vec3 gridLineCoords;

void main() {
   gl_FragColor = vec4(0, 0, 0, 0);
}

[[FS_BLUEPRINTS_COLOR_PASS]]

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
