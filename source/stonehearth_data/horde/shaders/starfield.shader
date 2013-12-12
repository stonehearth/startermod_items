[[FX]]

// Contexts
context STARFIELD
{
  VertexShader = compile GLSL VS_STARFIELD;
  PixelShader  = compile GLSL FS_STARFIELD;
  ZWriteEnable = false;
  ZEnable = false;
  CullMode = None;
  BlendMode = Blend;
}

[[VS_STARFIELD]]

uniform mat4 projMat;
uniform mat4 worldMat;
uniform mat4 viewProjMat;
uniform vec2 frameBufSize;

attribute vec3 vertPos;
attribute vec2 texCoords0;
attribute vec2 texCoords1;

varying float brightness;

void main() {
  vec4 clipPos = viewProjMat * worldMat * vec4(vertPos, 1.0);
  clipPos.x += (clipPos.w * 1.5 *  texCoords0.x / frameBufSize.x);
  clipPos.y += (clipPos.w * 1.5 * texCoords0.y / frameBufSize.y);

  brightness = texCoords1.x;
  gl_Position = clipPos;
}

[[FS_STARFIELD]]

in float brightness;

void main() {
  //gl_FragColor = vec4(brightness * 2.5, brightness * 2.5, brightness * 2.5, 1.0);
  gl_FragColor = vec4(1.0, 1.0, 1.0, brightness);
}
