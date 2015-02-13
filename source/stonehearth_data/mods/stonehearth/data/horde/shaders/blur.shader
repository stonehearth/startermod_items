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

sampler2D image = sampler_state
{
  Address = Clamp;
  Filter = None;
};

context BLUR
{
  VertexShader = compile GLSL VS_BLUR;
  PixelShader = compile GLSL FS_BLUR;
}

context BLUR_GAUSSIAN_X
{
  VertexShader = compile GLSL VS_BLUR;
  PixelShader = compile GLSL FS_BLUR_GAUSSIAN_X;
}

context BLUR_GAUSSIAN_Y
{
  VertexShader = compile GLSL VS_BLUR;
  PixelShader = compile GLSL FS_BLUR_GAUSSIAN_Y;
}

[[VS_BLUR]]
#include "shaders/fsquad_vs.glsl"

[[FS_BLUR]]
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


[[FS_BLUR_GAUSSIAN_X]]
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

[[FS_BLUR_GAUSSIAN_Y]]

varying vec2 texCoords;
uniform sampler2D image;
uniform vec2 frameBufSize;

void main()
{
  float v_offset = 1.0 / frameBufSize.y;

  vec3 result = texture2D(image, texCoords + vec2(0, v_offset * -2.0)).xyz * 0.061;
  result += texture2D(image, texCoords + vec2(0, v_offset * -1.0)).xyz * 0.242;
  result += texture2D(image, texCoords).xyz * 0.383;
  result += texture2D(image, texCoords + vec2(0, v_offset * 1.0)).xyz * 0.242;
  result += texture2D(image, texCoords + vec2(0, v_offset * 2.0)).xyz * 0.061;

  gl_FragColor = vec4(result, 1.0);
}
