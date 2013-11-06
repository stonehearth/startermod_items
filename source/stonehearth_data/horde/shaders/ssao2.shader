[[FX]]

// The material-wide sampler kernel for SSAO hemisphere sampling.
float4[16] samplerKernel;

// Samplers

sampler2D normals = sampler_state
{
	Address = Clamp;
  Filter = Trilinear;
};

sampler2D depth = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D positions = sampler_state
{
  Address = Clamp;
  Filter = Trilinear;
};

sampler2D ssao = sampler_state
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

context SSAO2
{
  VertexShader = compile GLSL VS_SSAO2;
  PixelShader = compile GLSL FS_SSAO2;
}

context DEPTH_OUTPUT
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_DEPTH_OUTPUT;
}

context BLUR
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_BLUR;
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
in vec3 vertPos;
varying vec2 texCoords;
				
void main( void )
{
	texCoords = vertPos.xy; 
	gl_Position = projMat * vec4( vertPos, 1.0 );
}


[[FS_DEPTH_OUTPUT]]

varying vec2 texCoords;
uniform sampler2D gbuf0;
        
void main( void )
{
  gl_FragColor = vec4(texture2D(gbuf0, texCoords).rrr, 1.0);
}

[[FS_SSAO]]

#include "shaders/utilityLib/vertCommon.glsl"

uniform sampler2D randomVectorLookup;
uniform sampler2D depth;
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
  vec2 rotationTC = screenTC * screenSize / 4.0;
  vec3 rotation = (texture2D(sRotSampler4x4, rotationTC).rgb * 2.0) - 1.0;

  mat3 rotMat;
  float h = 1.0 / (1.0 + rotation.z);
  rotMat._m00 = h * rotation.y * rotation.y + rotation.z;
  rotMat._m01 = -h * rotation.y * rotation.x;
  rotMat._m02 = -rotation.x;
  rotMat._m10 = -h * rotation.y * rotation.x;
  rotMat._m11 = h * rotation.x * rotation.x + rotation.z;
  rotMat._m12 = -rotation.y;
  rotMat._m20 = rotation.x;
  rotMat._m21 = rotation.y;
  rotMat._m22 = rotation.z;

  float fSceneDepthP = toLinearDepth(texture2D(sSceneDepthSampler, screenTC).r) * farClipDist;

  const int nSamplesNum = 27;
  //float offsetScale = 0.003;
  //const float offsetScaleStep = 1 + 2.4/nSamplesNum;
  float result = 0.0;
  float probeLength = (2.0 / screenSize.x );

  for (int x = -1; x <= 1; x += 1)
  {
    for (int y = -1; y <= 1; y += 1)
    {
      for (int z = -1; z <= 1; z += 1)
      {
        vec3 offset = normalize(vec3(float(x), float(y), float(z))) * probeLength;
        //(offsetScale *= offsetScaleStep);
        probeLength *= 0.9;

        vec3 rotatedOffset = rotMat * offset;

        vec3 samplePos = vec3(screenTC, fSceneDepthP);
        samplePos += vec3(rotatedOffset.xy, rotatedOffset.z);
        float fSceneDepthSampled = toLinearDepth(texture2D(sSceneDepthSampler, samplePos.xy)) * farClipDist;
        
        if (fSceneDepthP - fSceneDepthSampled < 500.5) {
          if (fSceneDepthSampled > samplePos.z) {
            result += 0.9;
          }
        } else {
          result += 0.9;
        }

      }
    } 
  }
  result = result / float(nSamplesNum);

  return clamp(result * 2.0, 0.0, 1.0);
}

void main()
{
  float ao = ComputeSSAO(randomVectorLookup, depth, texCoords, frameBufSize, 1000.0);
  gl_FragColor = vec4(ao, ao, ao, 1.0);
}


[[FS_BLUR]]
#version 150
uniform sampler2D ssao;

uniform int uBlurSize = 4; // use size of noise texture
uniform vec2 frameBufSize;

in vec2 texCoords;
out vec4 fragColor;

void main() {
  vec2 texelSize = 1.0 / frameBufSize;
  float result = 0.0;
  vec2 hlim = vec2(float(-4) * 0.5);
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      vec2 offset = (hlim + vec2(float(i), float(j))) * texelSize;
      result += texture2D(ssao, texCoords + offset).r;
    }
  }

  vec3 r = vec3(result / float(4 * 4));
  fragColor = vec4(r, 1.0);
}


[[FS_BLUR_Y]]
#include "shaders/utilityLib/blur.glsl"

varying vec2 texCoords;
uniform sampler2D depth;
uniform sampler2D ssao;
uniform vec2 frameBufSize;

void main()
{
    float b = 0.0;
    float w_total = 0.0;
    float center_c = texture2D(ssao, texCoords).r;
    float center_d = fetch_eye_z(texCoords, depth);

    float g_BlurRadius = 4.0;
    vec2 g_InvResolution = 1.0 / frameBufSize;
    
    for (float r = -g_BlurRadius; r <= g_BlurRadius; ++r)
    {
        vec2 uv = texCoords + vec2(0, r*g_InvResolution.y); 
        b += BlurFunction(uv, r, center_c, center_d, w_total, depth, ssao);
    }

    float result = b / w_total * 0.8;
    gl_FragColor = vec4(result, result, result, 1);
}


[[FS_BLUR_X]]
#include "shaders/utilityLib/blur.glsl"

varying vec2 texCoords;
uniform sampler2D depth;
uniform sampler2D ssao;
uniform vec2 frameBufSize;

void main()
{
    float b = 0.0;
    float w_total = 0.0;
    float center_c = texture2D(ssao, texCoords).r;
    float center_d = fetch_eye_z(texCoords, depth);

    float g_BlurRadius = 4.0;
    vec2 g_InvResolution = 1.0 / frameBufSize;
    
    for (float r = -g_BlurRadius; r <= g_BlurRadius; ++r)
    {
        vec2 uv = texCoords + vec2(r*g_InvResolution.x, 0); 
        b += BlurFunction(uv, r, center_c, center_d, w_total, depth, ssao);
    }

    float result = b / w_total * 0.8;
    gl_FragColor = vec4(result, result, result, 1);
}

[[VS_SSAO2]]
#version 150
uniform mat4 projMat;
uniform vec2 frameBufSize;
uniform float halfTanFoV;

in vec3 vertPos;

out vec2 texCoords;
noperspective out vec3 viewRay;

void main()
{
  texCoords = vertPos.xy; 

  float aspect = frameBufSize.x / frameBufSize.y;

  viewRay = vec3(
    (1 - 2 * vertPos.x) * halfTanFoV * aspect,
    (1 - 2 * vertPos.y) * halfTanFoV,
    1);
  gl_Position = projMat * vec4( vertPos, 1.0 );
}

[[FS_SSAO2]]
#version 130

#include "shaders/utilityLib/vertCommon.glsl"

uniform sampler2D randomVectorLookup;
uniform sampler2D normals;
uniform sampler2D depth;
uniform vec4 samplerKernel[16];
uniform vec2 frameBufSize;
uniform mat4 camProjMat;
uniform mat4 camViewMat;

in vec2 texCoords;
noperspective in vec3 viewRay;
out vec4 fragColor;

void main()
{
  vec2 noiseScale = frameBufSize / 4.0;
  float radius = 0.35;

  vec3 origin = viewRay * toLinearDepth(texture2D(depth, texCoords).x);
  vec3 rvec = texture2D(randomVectorLookup, texCoords * noiseScale).xyz;
  vec3 normal = (camViewMat * vec4(texture2D(normals, texCoords).xyz, 0)).xyz;

  vec3 tangent = normalize(rvec - (normal * dot(rvec, normal)));
  vec3 bitangent = cross(normal, tangent);
  mat3 tbn = mat3(tangent, bitangent, normal);

  float occlusion = 0.0;
  for (int i = 0; i < 16; i+=2) {
    // get sample position:
    vec3 sample = tbn * samplerKernel[i].xyz;
    sample = (sample * radius) + origin;

    // project sample position:
    vec4 offset = camProjMat * vec4(sample, 1);
    offset.xy /= offset.w;
    offset.xy = (offset.xy * 0.5) + 0.5;

    // get sample location:
    float sampleDepth = toLinearDepth(texture2D(depth, offset.xy).x);

    // range check & accumulate:
    float rangeCheck = abs(origin.z - sampleDepth) < 1 * radius ? 1.0 : 0.0;
    occlusion += (sampleDepth < sample.z ? 1.0 : 0.0) * rangeCheck;
  }

  float visibility = 1.0 - (occlusion / 8.0);
  fragColor = vec4(visibility,visibility,visibility, 1);
}
