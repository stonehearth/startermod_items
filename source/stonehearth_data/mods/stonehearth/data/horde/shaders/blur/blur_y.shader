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
uniform vec4 frameBufSize;

void main()
{
  float v_offset = frameBufSize.w;

  vec4 col = texture2D(image, texCoords + vec2(0, v_offset * -2.0));
  vec3 result = col.xyz * 0.061;
  result += texture2D(image, texCoords + vec2(0, v_offset * -1.0)).xyz * 0.242;
  result += texture2D(image, texCoords).xyz * 0.383;
  result += texture2D(image, texCoords + vec2(0, v_offset * 1.0)).xyz * 0.242;
  result += texture2D(image, texCoords + vec2(0, v_offset * 2.0)).xyz * 0.061;

  gl_FragColor = vec4(result, col.a);
}
