[[FX]]

// Samplers

sampler2D gbuf0 = sampler_state
{
	Address = Clamp;
};

sampler2D gbuf1 = sampler_state
{
	Address = Clamp;
};

sampler2D gbuf2 = sampler_state
{
	Address = Clamp;
};

sampler2D gbuf3 = sampler_state
{
	Address = Clamp;
};

sampler2D randomVectorLookup = sampler_state
{
  Address = Wrap;
  Filter = None;
};

sampler2D buf0 = sampler_state
{
  Address = Clamp;
  Filter = None;
};

context SSAO
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_SSAO;
}

context VERTICALBLUR
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_VERTICALBLUR;
}

context HORIZONTALBLUR
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_HORIZONTALBLUR;
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


[[FS_SSAO]]
#include "shaders/utilityLib/fragDeferredRead.glsl"

uniform sampler2D randomVectorLookup;
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

  const int nSamplesNum = 16;
  float offsetScale = 0.01;
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
          samplePos += rotatedOffset;  //vec3(rotatedOffset.xy, rotatedOffset.z * fSceneDepthP * 2);
          float fSceneDepthS = texture2D(sSceneDepthSampler, samplePos.xy) * farClipDist;
          
          //result += fSceneDepthS > samplePos.z ? 1 : 0;

          float fRangeIsInvalid = clamp((fSceneDepthP - fSceneDepthS) / fSceneDepthS, 0.0, 1.0);

          result += mix(fSceneDepthS > samplePos.z ? 1 : 0, 0.5, fRangeIsInvalid);
        }
      } 
    }
  }
  result = result / nSamplesNum;
  return clamp(result + result, 0.0, 1.0);
}

void main()
{
  float ao = ComputeSSAO(randomVectorLookup, gbuf0, texCoords, frameBufSize, 2000);
  gl_FragColor = vec4(ao, ao, ao, 1);
  //vec2 f = texCoords * frameBufSize / 4;
  //gl_FragColor = texture2D(randomVectorLookup, f);
}

[[FS_VERTICALBLUR]]
uniform sampler2D buf0;
uniform vec2 frameBufSize;
varying vec2 texCoords;

void main( void )
{
  float blur[7];
  blur[0] = 1;
  blur[1] = 1;
  blur[2] = 2;
  blur[3] = 3;
  blur[4] = 2;
  blur[5] = 1;
  blur[6] = 1;

  float dev = 11.0;
  float width = 3.5;

  vec4 outCol = vec4(0.0, 0.0, 0.0, 0.0);
  for (int i = 0; i < 7; ++i ) {
    outCol += texture2D(buf0, texCoords+vec2(0, width*(i-3))/frameBufSize)/dev*blur[i];
  }
  gl_FragColor = outCol;
}

[[FS_HORIZONTALBLUR]]
uniform sampler2D buf0;
uniform vec2 frameBufSize;
varying vec2 texCoords;

void main( void )
{
  float blur[7];
  blur[0] = 1;
  blur[1] = 1;
  blur[2] = 2;
  blur[3] = 3;
  blur[4] = 2;
  blur[5] = 1;
  blur[6] = 1;
  float dev = 11.0;
  float width = 3.5;

  vec4 outCol = vec4(0.0, 0.0, 0.0, 0.0);
  for (int i = 0; i < 7; ++i ) {
    outCol += texture2D(buf0, texCoords+vec2(width*(i-3), 0)/frameBufSize)/dev*blur[i];
  }
  gl_FragColor = outCol;
}
