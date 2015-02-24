[[FX]]

sampler2D outlineSampler = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D outlineDepth = sampler_state
{
  Filter = None;
  Address = Clamp;
};

[[VS]]
#include "shaders/fsquad_vs.glsl"


[[FS]]
#include "shaders/utilityLib/outline.glsl"

uniform sampler2D outlineSampler;
uniform sampler2D outlineDepth;

varying vec2 texCoords;

void main(void)
{
  gl_FragColor = compute_outline_color(outlineSampler, texCoords);
  gl_FragDepth = compute_outline_depth(outlineDepth, texCoords);
}
