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

const vec3 samplerKernel[] = vec3[NUM_SAMPLES] (
  vec3( -0.000193264809954 , 0.000187472221306 , 0.000295808856311 ),
  vec3( 0.00178519724276 , -0.00132094009995 , 0.00140250441112 ),
  vec3( -0.00620084517084 , -0.00183091079845 , 0.00212657562242 ),
  vec3( -0.00200009545639 , -0.000574745668306 , 0.0127706156453 ),
  vec3( 0.00736218701833 , -0.0174241817609 , 0.00917860105194 ),
  vec3( -0.00662428164914 , -0.0179250715625 , 0.0244905426873 ),
  vec3( -0.0195099779604 , 0.0375272344263 , 0.00805655635895 ),
  vec3( 0.0177096727543 , -0.0479539735416 , 0.0252182878077 ),
  vec3( 0.00266460483425 , 0.0535201359957 , 0.0494257516289 ),
  vec3( -0.0563280540478 , -0.0305867819853 , 0.0642451958606 ),
  vec3( -0.0694671811351 , 0.0278792490382 , 0.0813618008175 ),
  vec3( 0.0901093644497 , 0.0845677718954 , 0.0472821904143 ),
  vec3( 0.116589957503 , -0.0809648238493 , 0.0647709790986 ),
  vec3( -0.0157513773699 , 0.0889314415084 , 0.157651540602 ),
  vec3( 0.0869700682758 , -0.144491797723 , 0.123965454479 ),
  vec3( 0.229581657516 , -0.00679414774471 , 0.0656362262801 ),
  vec3( 0.119938232541 , -0.190813270339 , 0.149403066359 ),
  vec3( -0.0939791736781 , 0.2824946328 , 0.0608741537195 ),
  vec3( -0.191465932906 , -0.158646164408 , 0.230869925161 ),
  vec3( 0.0687983389929 , -0.10923991465 , 0.353875796581 ),
  vec3( 0.255590438485 , 0.233599625235 , 0.230611238819 ),
  vec3( -0.0484980836747 , 0.317077072869 , 0.32595431812 ),
  vec3( -0.429063616216 , -0.0561673651027 , 0.251607233105 ),
  vec3( -0.419129942013 , -0.258454577986 , 0.235321250203 ),
  vec3( -0.405219598876 , 0.383166022024 , 0.201274156942 ),
  vec3( 0.404320684165 , -0.342966708603 , 0.362028489761 ),
  vec3( -0.043971354957 , 0.588394149613 , 0.363573665588 ),
  vec3( 0.498413452009 , 0.40997664793 , 0.374319067591 ),
  vec3( -0.460452275705 , 0.174223415131 , 0.631878907738 ),
  vec3( 0.350842758019 , 0.22605843245 , 0.749577467129 ),
  vec3( -0.511860811951 , 0.279488842215 , 0.707402428824 ),
  vec3( -0.184745160954 , 0.811536684444 , 0.512865607163 )
);

uniform sampler2D randomVectorLookup;
uniform sampler2D normalBuffer;
uniform sampler2D depthBuffer;
uniform vec2 frameBufSize;
uniform mat4 camProjMat;
uniform mat4 camViewMat;

in vec2 texCoords;

out vec4 fragColor;


float getSampleDepth(const vec2 texCoords, const float screenSpaceDistance) {
  ivec2 pixelCoords = ivec2(floor(texCoords * frameBufSize));

  int mipLevel = clamp(int(floor(log2(screenSpaceDistance))) - LOG_MAX_OFFSET, 0, MAX_MIP_LEVEL);

  return texelFetch(depthBuffer, pixelCoords >> mipLevel, mipLevel);
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


void main()
{
  vec2 noiseScale = frameBufSize / 4.0;
  float radius = 1.0;
  const float intensity = 1.0;

  vec4 attribs = texture2D(depthBuffer, texCoords);
  vec3 origin = toCameraSpace(texCoords, attribs.r);
  radius *= attribs.g;
  vec3 rvec = texture2D(randomVectorLookup, texCoords * noiseScale).xyz;
  vec3 normal = -texture2D(normalBuffer, texCoords).xyz;

  vec3 tangent = normalize(rvec - (normal * dot(rvec, normal)));
  vec3 bitangent = cross(normal, tangent);
  mat3 tbn = mat3(tangent, bitangent, normal);

  float occlusion = 0.0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    // get sample position:
    vec3 unitSample = tbn * samplerKernel[i].xyz;
    vec3 ssaoSample = (unitSample * radius) + origin;

    // project sample position:
    vec4 offset = camProjMat * vec4(ssaoSample, 1.0);
    offset.xy /= offset.w;
    offset.xy = (offset.xy * 0.5) + 0.5;

    // get sample location:
    float sampleDepth = getSampleDepth(offset.xy, length((offset.xy - texCoords) * frameBufSize));

    // range check & accumulate:
    float rangeCheck = abs(origin.z - sampleDepth) <  radius ? 1.0 : 0.0;
    float normCheck = dot(normal, unitSample) <= 0.0 ? 0.0 : 1.0;
    occlusion += (sampleDepth < ssaoSample.z ? (1.0 * rangeCheck * normCheck) : 0.0);
  }

  occlusion /= NUM_SAMPLES;
  occlusion *= intensity;

  float visibility = 1.0 - occlusion;
  fragColor.rgb = vec3(visibility);
  fragColor.a = 1.0;
}
