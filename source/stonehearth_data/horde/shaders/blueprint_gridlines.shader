[[FX]]

sampler3D gridMap = sampler_state
{
   Texture = "textures/common/gridMap.dds";
   Filter = Bilinear;
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
varying vec3 gridLineCoords;
varying vec3 outColor;

void main() {
   vec4 theColor = vec4(outColor, 1);
   vec4 gridlineColor = vec4(0.0, .4, .8, .4);
   vec4 gridline = texture3D(gridMap, gridLineCoords + vec3(0.5, 0.0, 0.5));
   gridline = vec4(1.0, 1.0, 1.0, 1.0) - gridline;
   gl_FragColor = vec4(theColor.rgb, 0.3) * (1 - gridline.a) + gridline * gridlineColor;
}
