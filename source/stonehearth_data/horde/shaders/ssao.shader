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

#define LOG_MAX_OFFSET 3
#define MAX_MIP_LEVEL 5
#define NUM_SAMPLES 32

const vec3 samplerKernel[] = vec3[32] (
vec3( -9.6874980394e-05 , 0.000276812543097 , 0.000272011128739 ),
vec3( -0.0010955683475 , -0.00228584034286 , 0.000688109358538 ),
vec3( -0.000952953214339 , -0.00327343416717 , 0.00589080198165 ),
vec3( -0.00473005826816 , -0.0061056044627 , 0.0103811117566 ),
vec3( -0.00736544782026 , -0.0134670216626 , 0.0143680246014 ),
vec3( -0.0129832041189 , -0.0186575713004 , 0.0211732714284 ),
vec3( -0.018627517779 , 0.0382336242859 , 0.00671164804028 ),
vec3( -0.04611629206 , 0.0157685961902 , 0.0295603976339 ),
vec3( -0.0278360157415 , -0.045752661868 , 0.0494596821626 ),
vec3( 0.0527046833779 , 0.020908989173 , 0.0708581443307 ),
vec3( 0.0567340119008 , -0.0653771776322 , 0.0687732575392 ),
vec3( 0.00596325547706 , -0.0988856047872 , 0.087710249602 ),
vec3( 0.114283897281 , 0.0972098550869 , 0.0428162997159 ),
vec3( 0.116657199955 , 0.123654924911 , 0.0641207663388 ),
vec3( 0.0102148158118 , 0.0116407716567 , 0.208732499296 ),
vec3( 0.199152030445 , -0.0369309925126 , 0.126635629558 ),
vec3( 0.204612550215 , 0.0374517043116 , 0.172763520857 ),
vec3( -0.0428690759454 , 0.213688154636 , 0.21175594474 ),
vec3( 0.173519176474 , -0.156058031744 , 0.246304115704 ),
vec3( 0.216569386783 , -0.303077100506 , 0.0560055503 ),
vec3( -0.32151593625 , -0.0829267374066 , 0.250654063573 ),
vec3( -0.0307920629761 , -0.200659155955 , 0.409785192203 ),
vec3( -0.331978063549 , -0.30977464232 , 0.210681740338 ),
vec3( -0.188784751564 , -0.398879389852 , 0.321092382189 ),
vec3( -0.055234148103 , 0.0909524961582 , 0.583272871241 ),
vec3( 0.16011641721 , 0.107895912201 , 0.612280337201 ),
vec3( -0.140946085766 , 0.503087181758 , 0.455372873722 ),
vec3( 0.538430130553 , 0.50447828636 , 0.110481846734 ),
vec3( -0.168147963702 , 0.613630454174 , 0.486646667139 ),
vec3( -0.062770090465 , 0.230451716889 , 0.824021454144 ),
vec3( -0.337905573528 , -0.754176119899 , 0.396953276333 ),
vec3( -0.224062596748 , 0.731619243079 , 0.60850877693 )
);

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

  int mipLevel = clamp(int(floor(log2(screenSpaceDistance))) - LOG_MAX_OFFSET, 0, MAX_MIP_LEVEL);

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
  float radius = 1.0;
  const float intensity = 0.6;

  vec4 attribs = texture2D(depthBuffer, texCoords);
  vec3 origin = toCameraSpace(texCoords, attribs.r);
  radius *= attribs.g;
  vec3 rvec = getRandomVec(texCoords);
  vec3 normal = -texture2D(normalBuffer, texCoords).xyz;

  vec3 tangent = normalize(rvec - (normal * dot(rvec, normal)));
  vec3 bitangent = cross(normal, tangent);
  mat3 tbn = mat3(tangent, bitangent, normal);

  float occlusion = 0.0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    // get sample position:
    vec3 cameraSpaceSample = tbn * samplerKernel[i].xyz;
    vec3 ssaoSample = (cameraSpaceSample * radius) + origin;

    // project sample position:
    vec4 offset = camProjMat * vec4(ssaoSample, 1.0);
    offset.xy /= offset.w;
    offset.xy = (offset.xy * 0.5) + 0.5;

    // get sample location:
    vec2 sampleDepth = getSampleDepth(offset.xy, length((offset.xy - texCoords) * frameBufSize));

    float sampleOcclusion;
    // range check & accumulate:
    
    // Old and busted:
    /*
    float rangeCheck = abs(origin.z - sampleDepth.r) <  radius ? 1.0 : 0.0;
    sampleOcclusion = sampleDepth.r < ssaoSample.z ? (1.0 * rangeCheck) : 0.0;
    //*/

    // New hotness:
    sampleOcclusion = ((sampleDepth.r < ssaoSample.z) && (sampleDepth.g > ssaoSample.z)) ? 1.0 : 0.0;


    occlusion += sampleOcclusion;
  }

  occlusion /= NUM_SAMPLES;
  occlusion *= intensity;

  fragColor = vec4(vec3(occlusion), 1.0);
}
