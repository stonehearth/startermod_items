[[FX]]

// Samplers
sampler2D twinkleMap = sampler_state
{
  Texture = "textures/environment/twinkle.png";
  Address = Wrap;
  Filter = None;
};

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

uniform mat4 worldMat;
uniform mat4 viewProjMat;
uniform vec2 frameBufSize;
uniform float currentTime;

attribute vec3 vertPos;
attribute vec2 texCoords0;
attribute vec2 texCoords1;

varying float brightness;
varying vec2 texCoords;

void main() {
  vec4 clipPos = viewProjMat * worldMat * vec4(vertPos, 1.0);
  clipPos.x += (clipPos.w * 1.5 *  texCoords0.x / frameBufSize.x);
  clipPos.y += (clipPos.w * 1.5 * texCoords0.y / frameBufSize.y);

  texCoords = vertPos.xy + vec2(currentTime, currentTime);
  brightness = texCoords1.x;
  gl_Position = clipPos;
}

[[FS_STARFIELD]]

uniform sampler2D twinkleMap;

in float brightness;
in vec2 texCoords;

void main() {
  float b = brightness * (texture2D(twinkleMap, texCoords).x * 2);
  gl_FragColor = vec4(1.0, 1.0, 1.0, b);
}
