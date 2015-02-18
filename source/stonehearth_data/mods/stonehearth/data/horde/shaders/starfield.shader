[[FX]]

float4 brightness;

// Samplers
sampler2D twinkleMap = sampler_state
{
  Texture = "textures/environment/twinkle.png";
  Address = Wrap;
  Filter = None;
};

[[VS]]

uniform mat4 worldMat;
uniform mat4 viewProjMat;
uniform vec4 frameBufSize;
uniform float currentTime;
uniform vec4 brightness;

attribute vec3 vertPos;
attribute vec2 texCoords0;
attribute vec2 texCoords1;

varying float oBrightness;
varying vec2 texCoords;

void main() {
  vec4 clipPos = viewProjMat * worldMat * vec4(vertPos, 1.0);
  clipPos.x += (clipPos.w * 1.5 *  texCoords0.x * frameBufSize.z);
  clipPos.y += (clipPos.w * 1.5 * texCoords0.y * frameBufSize.w);

  texCoords = vertPos.xy + vec2(currentTime, currentTime);
  oBrightness = texCoords1.x * brightness.x;
  gl_Position = clipPos;
}

[[FS]]

uniform sampler2D twinkleMap;

varying float oBrightness;
varying vec2 texCoords;

void main() {
  float finalBrightness = oBrightness * (texture2D(twinkleMap, texCoords).x * 2.0);
  gl_FragColor = vec4(vec3(1.0), finalBrightness);
}
