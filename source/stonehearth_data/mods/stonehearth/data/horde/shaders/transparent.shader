[[FX]]

// Contexts
context TRANSLUCENT
{
   VertexShader = compile GLSL VS_TRANSLUCENT;
   PixelShader  = compile GLSL FS_TRANSLUCENT;
   ZWriteEnable = false;
   ZEnable = true;
   BlendMode = Blend;
   CullMode = Back;
}

[[VS_TRANSLUCENT]]

uniform mat4 viewProjMat;
uniform mat4 worldMat;

attribute vec3 vertPos;
attribute vec4 color;

varying vec4 outColor;

void main() {
   outColor = color;
   gl_Position = viewProjMat * worldMat * vec4(vertPos, 1.0);
}

[[FS_TRANSLUCENT]]

varying vec4 outColor;

void main() {
   gl_FragColor = outColor.rgba;
}

