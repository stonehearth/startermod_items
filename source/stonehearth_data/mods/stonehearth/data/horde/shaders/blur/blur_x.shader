[[FX]]

sampler2D image = sampler_state
{
  Address = Clamp;
  Filter = None;
};

[[VS]]
#include "shaders/fsquad_vs.glsl"

[[FS]]
varying vec2 texCoords;
uniform sampler2D image;
uniform vec2 frameBufSize;

void main()
{
  float u_offset = 1.0 / frameBufSize.x;

  vec3 result = texture2D(image, texCoords + vec2(u_offset * -2.0, 0)).xyz * 0.061;
  result += texture2D(image, texCoords + vec2(u_offset * -1.0, 0)).xyz * 0.242;
  result += texture2D(image, texCoords).xyz * 0.383;
  result += texture2D(image, texCoords + vec2(u_offset * 1.0, 0)).xyz * 0.242;
  result += texture2D(image, texCoords + vec2(u_offset * 2.0, 0)).xyz * 0.061;

  gl_FragColor = vec4(result, 1.0);
}
