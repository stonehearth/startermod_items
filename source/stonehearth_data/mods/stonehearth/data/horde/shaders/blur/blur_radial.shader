[[FX]]

sampler2D depthBuffer = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D ssaoImage = sampler_state
{
  Address = Clamp;
  Filter = None;
};

[[VS]]
#include "shaders/fsquad_vs.glsl"

[[FS]]
#include "shaders/utilityLib/blur.glsl"

varying vec2 texCoords;
uniform sampler2D depthBuffer;
uniform sampler2D ssaoImage;
uniform vec2 frameBufSize;

const int g_BlurRadius = 4;

void main()
{
    float b = 0.0;
    float w_total = 0.0;
    float center_c = texture2D(ssaoImage, texCoords).r;
    float pixelDepth = texture2D(depthBuffer, texCoords).r;

    vec2 g_InvResolution = 1.0 / frameBufSize;
    
    b = BlurFunction(texCoords, 0.0, pixelDepth, w_total, depthBuffer, ssaoImage);
    for (int r = -g_BlurRadius; r <= g_BlurRadius; ++r)
    {
      for (int s = -g_BlurRadius; s <= g_BlurRadius; ++s)
      {
        if (r != 0 && s != 0) {
          float rf = float(r);
          float sf = float(s);
          vec2 uv = texCoords + vec2(rf, sf) * g_InvResolution;
          b += BlurFunction(uv, abs(rf) + abs(sf), pixelDepth, w_total, depthBuffer, ssaoImage);
        }
      }
    }

    float result = b / w_total;
    gl_FragColor = vec4(vec3(result), 1.0);
}
