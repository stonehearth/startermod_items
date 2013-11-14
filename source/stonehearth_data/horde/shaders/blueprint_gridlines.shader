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
#version 130

uniform mat4 viewProjMat;
uniform mat4 worldMat;

in vec3 vertPos;
in vec3 color;

out vec3 gridLineCoords;
out vec3 outColor;

void main() {
   gridLineCoords = vertPos;
   outColor = color;
   gl_Position = viewProjMat * worldMat * vec4(vertPos, 1.0);
}

[[FS_BLUEPRINTS_DEPTH_PASS]]
#version 130

in vec3 gridLineCoords;

out vec4 fragColor;

void main() {
   fragColor = vec4(0, 0, 0, 0);
};

[[FS_BLUEPRINTS_COLOR_PASS]]
#version 130

uniform sampler3D gridMap;

in vec3 gridLineCoords;
in vec3 outColor;

out vec4 fragColor;

void main() {
   vec4 theColor = vec4(outColor, 1);
   vec4 gridlineColor = vec4(0, .4, .8, .4);
   vec4 gridline = texture(gridMap, gridLineCoords + vec3(0.5, 0, 0.5));
   gridline = vec4(1, 1, 1, 1) - gridline;
   fragColor = vec4(theColor.rgb, 0.3) * (1-gridline.a) + gridline * gridlineColor;
};

