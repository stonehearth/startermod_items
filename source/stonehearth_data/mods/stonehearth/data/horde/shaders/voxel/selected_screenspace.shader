[[FX]]

sampler2D outlineSampler = sampler_state
{
  Address = Clamp;
  Filter = None;
};

[[VS]]
#include "shaders/fsquad_vs.glsl"


[[FS]]
#include "shaders/utilityLib/outline.glsl"

uniform sampler2D outlineSampler;

varying vec2 texCoords;

void main(void)
{
  gl_FragColor = compute_outline_color(outlineSampler, texCoords);
}
