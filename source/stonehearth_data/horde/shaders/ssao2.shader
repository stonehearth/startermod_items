[[FX]]

// Samplers

sampler2D depthbuff = sampler_state
{
	Address = Clamp;
  Filter = None;
};

sampler2D ssaobuff = sampler_state
{
	Address = Clamp;
  Filter = None;
};

sampler2D randomVectorLookup = sampler_state
{
  Address = Wrap;
  Filter = None;
};

context SSAO
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_SSAO;
}


context DEPTH_OUTPUT
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_DEPTH_OUTPUT;
}

context BLUR_Y
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_BLUR_Y;
}

context BLUR_X
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_BLUR_X;
}

[[VS_FSQUAD]]

uniform mat4 projMat;
attribute vec3 vertPos;
varying vec2 texCoords;
				
void main( void )
{
	texCoords = vertPos.xy; 
	gl_Position = projMat * vec4( vertPos, 1 );
}


[[FS_DEPTH_OUTPUT]]

varying vec2 texCoords;
uniform sampler2D gbuf0;
        
void main( void )
{
  gl_FragColor = vec4(texture2D(gbuf0, texCoords).rrr, 1);
}

[[FS_SSAO]]
uniform sampler2D randomVectorLookup;
uniform sampler2D depthbuff;
uniform vec2 frameBufSize;
varying vec2 texCoords;

/** Unoptimized ShaderX7 code.... */
float ComputeSSAO (
  sampler2D sRotSampler4x4, 
  sampler2D sSceneDepthSampler,
  vec2 screenTC,
  vec2 screenSize,
  float farClipDist)
{

  // Get a random vector to create a rotation from.
  vec2 rotationTC = screenTC * screenSize / 4;
  vec3 rotation = (texture2D(sRotSampler4x4, rotationTC).rgb * 2) - 1;

  mat3 rotMat;
  float h = 1 / (1 + rotation.z);
  rotMat._m00 = h * rotation.y * rotation.y + rotation.z;
  rotMat._m01 = -h * rotation.y * rotation.x;
  rotMat._m02 = -rotation.x;
  rotMat._m10 = -h * rotation.y * rotation.x;
  rotMat._m11 = h * rotation.x * rotation.x + rotation.z;
  rotMat._m12 = -rotation.y;
  rotMat._m20 = rotation.x;
  rotMat._m21 = rotation.y;
  rotMat._m22 = rotation.z;

  float fSceneDepthP = texture2D(sSceneDepthSampler, screenTC).r * farClipDist;

  const int nSamplesNum = 8;
  float offsetScale = 0.003;
  const float offsetScaleStep = 1 + 2.4/nSamplesNum;
  float result = 0;

  for(int i = 0; i < (nSamplesNum / 8); i++)
  {
    for (int x = -1; x <= 1; x += 2)
    {
      for (int y = -1; y <= 1; y += 2)
      {
        for (int z = -1; z <= 1; z += 2)
        {
          vec3 offset = normalize(vec3(x, y, z)) * (offsetScale *= offsetScaleStep);

          vec3 rotatedOffset = rotMat * offset;

          vec3 samplePos = vec3(screenTC, fSceneDepthP);
          samplePos += vec3(rotatedOffset.xy, rotatedOffset.z);
          float fSceneDepthSampled = texture2D(sSceneDepthSampler, samplePos.xy) * farClipDist;
          
          float fRangeIsInvalid = clamp((fSceneDepthP - fSceneDepthSampled) / fSceneDepthSampled, 0.2, 1.0);

          result += mix(fSceneDepthSampled > samplePos.z ? 1 : 0, 1, fRangeIsInvalid);
        }
      } 
    }
  }
  result = result / nSamplesNum;

  return clamp(result * result + result, 0.0, 1.0);
}

void main()
{
  float ao = ComputeSSAO(randomVectorLookup, depthbuff, texCoords, frameBufSize, 2000);
  gl_FragColor = vec4(ao, ao, ao, 1);
}


[[FS_BLUR_Y]]
#include "shaders/utilityLib/blur.glsl"

varying vec2 texCoords;
uniform sampler2D depthbuff;
uniform sampler2D ssaobuff;
uniform vec2 frameBufSize;

void main()
{
    float b = 0;
    float w_total = 0;
    float center_c = texture2D(ssaobuff, texCoords).r;
    float center_d = fetch_eye_z(texCoords, depthbuff);

    float g_BlurRadius = 4.0;
    vec2 g_InvResolution = 1.0 / frameBufSize;
    
    for (float r = -g_BlurRadius; r <= g_BlurRadius; ++r)
    {
        vec2 uv = texCoords + vec2(0, r*g_InvResolution.y); 
        b += BlurFunction(uv, r, center_c, center_d, w_total, depthbuff, ssaobuff);
    }

    float result = b / w_total;
    gl_FragColor = vec4(result, result, result, 1);
}


[[FS_BLUR_X]]
#include "shaders/utilityLib/blur.glsl"

varying vec2 texCoords;
uniform sampler2D depthbuff;
uniform sampler2D ssaobuff;
uniform vec2 frameBufSize;

void main()
{
    float b = 0;
    float w_total = 0;
    float center_c = texture2D(ssaobuff, texCoords).r;
    float center_d = fetch_eye_z(texCoords, depthbuff);

    float g_BlurRadius = 4.0;
    vec2 g_InvResolution = 1.0 / frameBufSize;
    
    for (float r = -g_BlurRadius; r <= g_BlurRadius; ++r)
    {
        vec2 uv = texCoords + vec2(r*g_InvResolution.x, 0); 
        b += BlurFunction(uv, r, center_c, center_d, w_total, depthbuff, ssaobuff);
    }

    float result = b / w_total;
    gl_FragColor = vec4(result, result, result, 1);
}
