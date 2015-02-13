[[FX]]

sampler2D randomVectorLookup = sampler_state
{
  Address = Wrap;
  Filter = None;
};

sampler2D normalBuffer = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D depthBuffer = sampler_state
{
  Address = Clamp;
  Filter = None;
};

context SSAO
{
  VertexShader = compile GLSL VS_SSAO;
  PixelShader = compile GLSL FS_SSAO;
}

[[VS_SSAO]]
#include "shaders/fsquad_vs.glsl"


[[FS_SSAO]]
#version 400

#include "shaders/utilityLib/vertCommon_400.glsl"
#include "shaders/utilityLib/ssaoSamplerKernel.glsl"

#define LOG_MAX_OFFSET 3
#define MAX_MIP_LEVEL 5
#define NUM_SAMPLES 32

uniform sampler2D randomVectorLookup;
uniform sampler2D normalBuffer;
uniform sampler2D depthBuffer;
uniform vec2 frameBufSize;
uniform mat4 camProjMat;
uniform mat4 camViewMat;

in vec2 texCoords;

out vec4 fragColor;


vec2 getSampleDepth(const vec2 texCoords, const float screenSpaceDistance) {
  ivec2 pixelCoords = ivec2(floor(texCoords * frameBufSize));

  int mipLevel = 0;//clamp(int(floor(log2(screenSpaceDistance) - LOG_MAX_OFFSET)), 0, MAX_MIP_LEVEL);

  return texelFetch(depthBuffer, pixelCoords >> mipLevel, mipLevel).ra;
}


vec3 toCameraSpace(const vec2 fragCoord, float depth)
{
  vec3 result;
  vec4 projInfo = vec4(
    -2.0 / (camProjMat[0][0]),
    -2.0 / (camProjMat[1][1]),
    (1.0 - camProjMat[0][2]) / camProjMat[0][0],
    (1.0 + camProjMat[1][2]) / camProjMat[1][1]);

  result.z = depth;
  result.xy = vec2((fragCoord.xy * projInfo.xy + projInfo.zw) * result.z);

  return result;
}

vec3 getRandomVec(const vec2 texCoords)
{
  vec2 noiseScale = frameBufSize / 4.0;
  return texture2D(randomVectorLookup, texCoords * noiseScale).xyz;
}


void main()
{
  float radius = 0.5;
  const float intensity = 0.5;

  vec4 attribs = texture2D(depthBuffer, texCoords);
  vec3 origin = toCameraSpace(texCoords, attribs.r);
  radius *= attribs.g;
  vec3 rvec = getRandomVec(texCoords);
  vec3 normal = -texture2D(normalBuffer, texCoords).xyz;

  vec3 tangent = normalize(rvec - (normal * dot(rvec, normal)));
  vec3 bitangent = cross(normal, tangent);
  mat3 tbn = mat3(tangent, bitangent, normal);

  float occlusion = 0.0;
  for (int i = 0; i < NUM_SAMPLES; i+=4) {
    // get sample position:
    vec3 cameraSpaceSample = tbn * samplerKernel[i];
    vec3 ssaoSample = (cameraSpaceSample * radius) + origin;

    // project sample position:
    vec4 offset = camProjMat * vec4(ssaoSample, 1.0);
    offset.xy /= offset.w;
    offset.xy = (offset.xy * 0.5) + 0.5;

    // get sample location: (x is front-face depths, y is back-face depths).
    vec2 sampledDepths = getSampleDepth(offset.xy, length((offset.xy - texCoords) * frameBufSize));

    float sampleOcclusion;
    // Range check:
    float rangeCheck = abs(origin.z - sampledDepths.r) <  radius ? 1.0 : 0.0;
    //float normalCheck = abs(dot(normal, normalize(cameraSpaceSample)));

    // Old and busted:
    //sampleOcclusion = sampledDepths.r < ssaoSample.z ? (1.0 * rangeCheck) : 0.0;

    // New hotness:
    sampleOcclusion = ((sampledDepths.x < ssaoSample.z) && (sampledDepths.y >= ssaoSample.z)) ? 1.0 : 0.0;


    occlusion += sampleOcclusion;
  }

  occlusion /= (NUM_SAMPLES / 4);
  occlusion *= intensity;

  fragColor = vec4(vec3(occlusion), 1.0);
}
