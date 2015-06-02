[[FX]]

sampler2D selections = sampler_state
{
  Address = Clamp;
  Filter = None;
};

[[VS]]
#include "shaders/fsquad_vs.glsl"


[[FS]]

uniform sampler2D selections;
uniform vec4 frameBufSize;

varying vec2 texCoords;

void main(void)
{
  vec2 offset = frameBufSize.zw;
  vec4 sampleColor = texture2D(selections, texCoords);

  if (sampleColor.a > 0.0) {
	gl_FragColor = sampleColor;
  	return;
  }

  vec4 sampleLeft = texture2D(selections, texCoords + vec2(-offset.x, 0.0));
  vec4 sampleRight = texture2D(selections, texCoords + vec2(offset.x, 0.0));

  if (sampleLeft.a > 0.0 && sampleRight.a > 0.0) {
  	gl_FragColor = sampleLeft;
  	return;
  }

  gl_FragColor = sampleColor;
  return;
}
