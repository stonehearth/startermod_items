[[FX]]

float4 axis;
float4[16] samplerKernel;

// Samplers
sampler2D cloudMap = sampler_state
{
  Texture = "textures/environment/cloudmap.png";
  Address = Wrap;
  Filter = Pixely;
};

sampler2D depthBuffer = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D normalBuffer = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D ssaoImage = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D randomVectorLookup = sampler_state
{
  Address = Wrap;
  Filter = None;
};

context OMNI_LIGHTING_FORWARD_POSTPROCESS
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_OMNI_LIGHTING;
  
  ZWriteEnable = false;
  BlendMode = Add;
  CullMode = Back;
}

context DIRECTIONAL_LIGHTING_FORWARD_POSTPROCESS
{
  VertexShader = compile GLSL VS_GENERAL_SHADOWS;
  PixelShader = compile GLSL FS_DIRECTIONAL_LIGHTING;
  
  ZWriteEnable = false;
  BlendMode = Add;
  CullMode = Back;
}

context SSAO
{
  VertexShader = compile GLSL VS_SSAO;
  PixelShader = compile GLSL FS_SSAO;
}

context SSAO2
{
  VertexShader = compile GLSL VS_SSAO;
  PixelShader = compile GLSL FS_SSAO2;
}

context DEPTH_LINEAR
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_DEPTH_LINEAR;
}

context BLUR
{
  VertexShader = compile GLSL VS_BLUR;
  PixelShader = compile GLSL FS_BLUR;
}

context BLUR2
{
  VertexShader = compile GLSL VS_BLUR;
  PixelShader = compile GLSL FS_BLUR_2;
}

context BLUR3
{
  VertexShader = compile GLSL VS_BLUR;
  PixelShader = compile GLSL FS_BLUR_3;
}

context BLUR_BLEND
{
  VertexShader = compile GLSL VS_BLUR;
  PixelShader = compile GLSL FS_BLUR;
}

[[VS_GENERAL]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec3 color;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 tsbNormal;
varying vec3 albedo;

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  vsPos = calcViewPos(pos);
  tsbNormal = calcWorldVec(normal);
  albedo = color;

  gl_Position = viewProjMat * pos;
}


[[VS_GENERAL_SHADOWS]]
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;
uniform mat4 shadowMats[4];

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec3 color;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 tsbNormal;
varying vec3 albedo;
varying vec4 projShadowPos[3];

void main( void )
{
  pos = calcWorldPos(vec4(vertPos, 1.0));
  vsPos = calcViewPos(pos);
  tsbNormal = calcWorldVec(normal);
  albedo = color;

  projShadowPos[0] = shadowMats[0] * pos;
  projShadowPos[1] = shadowMats[1] * pos;
  projShadowPos[2] = shadowMats[2] * pos;

  gl_Position = viewProjMat * pos;
}


[[FS_OMNI_LIGHTING]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 

uniform vec3 viewerPos;
uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 albedo;
varying vec3 tsbNormal;

void main( void )
{
  vec3 normal = tsbNormal;
  vec3 newPos = pos.xyz;
  gl_FragColor = vec4(calcPhongOmniLight(viewerPos, newPos, normalize(normal)) * albedo, 1.0);
}


[[FS_DIRECTIONAL_LIGHTING]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 
#include "shaders/shadows.shader"

uniform vec3 lightAmbientColor;
uniform sampler2D cloudMap;
uniform float currentTime;
uniform sampler2D ssaoImage;
uniform vec2 frameBufSize;

varying vec4 pos;
varying vec3 albedo;
varying vec3 tsbNormal;

void main( void )
{
  // Shadows.
  float shadowTerm = getShadowValue(pos.xyz);
  float ssao = texture2D(ssaoImage, gl_FragCoord.xy / frameBufSize).r;

  // Light Color.
  vec3 lightColor = calcSimpleDirectionalLight(normalize(tsbNormal));

  float cloudSpeed = currentTime / 80.0;
  vec2 fragCoord = pos.xz * 0.3;
  vec3 cloudColor = texture2D(cloudMap, fragCoord.xy / 128.0 + cloudSpeed).xyz;
  cloudColor = cloudColor * texture2D(cloudMap, fragCoord.yx / 192.0 + (cloudSpeed / 10.0)).xyz;

  // Mix light and shadow and ambient light.
  lightColor = cloudColor *((shadowTerm * (lightColor * albedo)) + (ssao * lightAmbientColor * albedo));
  
  gl_FragColor = vec4(lightColor, 1.0);
}


[[FS_DEPTH_LINEAR]]
#include "shaders/utilityLib/vertCommon.glsl"

varying vec3 tsbNormal;

float toLin2(float d) {
  float num = nearPlane * farPlane;
  float den = (farPlane + d * (nearPlane - farPlane));
  return num/den;
}

void main(void)
{
  gl_FragData[0] = vec4(toLinearDepth(gl_FragCoord.z));
  gl_FragData[1] = viewMat * vec4(normalize(tsbNormal), 0.0);
}


[[VS_SSAO]]
#include "shaders/fsquad_vs.glsl"


[[FS_SSAO]]
#extension GL_EXT_gpu_shader4 : enable

#define NUM_SAMPLES 32
#define NUM_TURNS   11

const float radius = 0.105;
const float intensity = 0.03;


const float radiusSq = radius * radius;
const float eps = 0.01;
const float bias = 0.04; 

uniform sampler2D depthBuffer;
uniform sampler2D normalBuffer;
uniform mat4 projMat;
uniform mat4 camProjMat;
uniform vec2 frameBufSize;
uniform float farPlane;

vec2 packKey(float pixelZ) {
  vec2 result;
  float uniZ = clamp(pixelZ * (1.0 / farPlane), 0.0, 1.0);

  float temp = floor(uniZ * 256.0);

  result.x = temp * (1.0 / 256.0);
  result.y = uniZ * 256.0 - temp;

  return result;
}

vec3 toCameraSpace(const vec2 fragCoord)
{
  vec3 result;
  vec4 projInfo = vec4(
    -2.0 / (camProjMat[0][0]),
    -2.0 / (camProjMat[1][1]),
    (1.0 - camProjMat[0][2]) / camProjMat[0][0],
    (1.0 + camProjMat[1][2]) / camProjMat[1][1]);

  result.z = texture2D(depthBuffer, fragCoord).r;
  result.xy = vec2((fragCoord.xy * projInfo.xy + projInfo.zw) * result.z);

  return result;
}

vec3 makeFragNormal(vec3 pos)
{
  return normalize(cross(dFdy(pos), dFdx(pos)));
}

vec2 makeSampleProbe (const float sampleAlpha, const float rotation) {
  float angle = sampleAlpha * (float(NUM_TURNS) * 6.28) + rotation;
  return vec2(cos(angle), sin(angle));
}

vec3 makeSamplePoint(vec2 texCoords, vec2 unitSampleVector, float sampleRadius) {
  vec2 texLocation = texCoords + (sampleRadius * unitSampleVector);

  return toCameraSpace(texLocation);
}

float sampleAO(const vec2 texCoords, const vec3 fragPoint, const vec3 normal_cam, const float diskRadius, const int sampleIdx, const float rotation) {
  float alpha = (float(sampleIdx) + 0.5) * (1.0 / float(NUM_SAMPLES));
  //alpha *= alpha * alpha;
  vec2 unitSampleVector = makeSampleProbe(alpha, rotation);
  float sampleRadius = alpha * diskRadius;

  vec3 samplePoint = makeSamplePoint(texCoords, unitSampleVector, sampleRadius);

  vec3 sampleDelta = samplePoint - fragPoint;
  float sampleDistSq = dot(sampleDelta, sampleDelta);
  float sampleContribution = dot(sampleDelta, normal_cam);

  float f = max(radiusSq - sampleDistSq, 0.0);
  return f * f * f * max((sampleContribution - bias) / (eps + sampleDistSq), 0.0);
}


varying vec2 texCoords;

void main(void)
{

  ivec2 pixelCoords = ivec2(texCoords * frameBufSize);
  vec3 frag_cam = toCameraSpace(texCoords);

  float randomRotationAngle = float(3 * pixelCoords.x ^ pixelCoords.y + pixelCoords.x * pixelCoords.y) * 10.0;

  vec3 fragNormal = -texture2D(normalBuffer, texCoords).rgb;//makeFragNormal(frag_cam);

  float diskRadius = radius / frag_cam.z;

  float sum = 0.0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += sampleAO(texCoords, frag_cam, fragNormal, diskRadius, i, randomRotationAngle);
  }
  float temp = radiusSq * radius;
  sum /= temp * temp;
  float A = max(0.0, 1.0 - sum * intensity * float(NUM_SAMPLES));

  if (abs(dFdx(frag_cam.z)) < 0.02) { 
    A -= dFdx(A) * (float(pixelCoords.x & 1) - 0.5);
  }
  if (abs(dFdy(frag_cam.z)) < 0.02) {
    A -= dFdy(A) * (float(pixelCoords.y & 1) - 0.5);
  }

  gl_FragColor.r = A;//1.0;
  gl_FragColor.gb = packKey(frag_cam.z);

  gl_FragColor.a = 1.0;
}

[[VS_BLUR]]
#include "shaders/fsquad_vs.glsl"


[[FS_BLUR]]
#version 120
#define R 0
#define EDGE_SHARPNESS 1.0

const float gaussian[] = 
//  float[4] (0.356642, 0.239400, 0.072410, 0.009869);
//  float[5] (0.398943, 0.241971, 0.053991, 0.004432, 0.000134);  // stddev = 1.0
//  float[5] (0.153170, 0.144893, 0.122649, 0.092902, 0.062970);  // stddev = 2.0
  float[7] (0.111220, 0.107798, 0.098151, 0.083953, 0.067458, 0.050920, 0.036108); // stddev = 3.0
//    float[9] (0.511872, 0.467430, 0.251121, 0.197496, 0.073413, 0.060907, 0.0120259, 0.0101563, 0.009123);

uniform sampler2D ssaoImage;
uniform vec4 axis;
uniform vec2 frameBufSize;

varying vec2 texCoords;

float unpackKey(vec2 p)
{
  return p.x * (256.0 / 257.0) + p.y * (1.0 / 257.0);
}

void main()
{
  vec4 sample = texture2D(ssaoImage, texCoords);

  float sum = sample.r;
  vec2 packedZ = sample.gb;
  float pixelZ = unpackKey(sample.gb);

  float BASE = gaussian[0];
  float totalWeight = BASE;
  sum *= totalWeight;

  for (int r = -R; r <= R; ++r) {
    // We already handled the zero case above.  This loop should be unrolled and the branch discarded
    if (r != 0) {
      sample = texture2D(ssaoImage, texCoords + axis.xy * (r / frameBufSize));
      float value  = sample.r;
      float sampleZ = unpackKey(sample.gb);

      int asdf = int(abs(float(r)));
      float weight = gaussian[asdf];

      // range domain (the "bilateral" weight). As depth difference increases, decrease weight.
      weight *= max(0.0, 1.0 - (2000.0 * EDGE_SHARPNESS) * abs(sampleZ - pixelZ));

      sum += value * weight;
      totalWeight += weight;
    }
  }

  const float epsilon = 0.0001;
  gl_FragColor.r = sum / (totalWeight + epsilon);
  gl_FragColor.gb = packedZ;
  gl_FragColor.a = 1.0;
}







[[FS_SSAO2]]

#include "shaders/utilityLib/vertCommon.glsl"

uniform sampler2D randomVectorLookup;
uniform sampler2D normalBuffer;
uniform sampler2D depthBuffer;
uniform vec4 samplerKernel[16];
uniform vec2 frameBufSize;
uniform mat4 camProjMat;
uniform mat4 camViewMat;
uniform mat4 projMat;

varying vec2 texCoords;


vec3 toCameraSpace(const vec2 fragCoord)
{
  vec3 result;
  vec4 projInfo = vec4(
    -2.0 / (camProjMat[0][0]),
    -2.0 / (camProjMat[1][1]),
    (1.0 - camProjMat[0][2]) / camProjMat[0][0],
    (1.0 + camProjMat[1][2]) / camProjMat[1][1]);

  result.z = texture2D(depthBuffer, fragCoord).r;
  result.xy = vec2((fragCoord.xy * projInfo.xy + projInfo.zw) * result.z);

  return result;
}


void main()
{
  vec2 noiseScale = frameBufSize / 4.0;
  float radius = 1.0;
  float intensity = 1.3;

  vec3 origin = toCameraSpace(texCoords);
  vec3 rvec = texture2D(randomVectorLookup, texCoords * noiseScale).xyz;
  vec3 normal = -texture2D(normalBuffer, texCoords).xyz;

  vec3 tangent = normalize(rvec - (normal * dot(rvec, normal)));
  vec3 bitangent = cross(normal, tangent);
  mat3 tbn = mat3(tangent, bitangent, normal);

  float occlusion = 0.0;
  for (int i = 0; i < 16; i++) {
    // get sample position:
    vec3 unitSample = tbn * samplerKernel[i].xyz;
    vec3 sample = (unitSample * radius) + origin;

    // project sample position:
    vec4 offset = camProjMat * vec4(sample, 1.0);
    offset.xy /= offset.w;
    offset.xy = (offset.xy * 0.5) + 0.5;

    // get sample location:
    float sampleDepth = texture2D(depthBuffer, offset.xy).r;

    // range check & accumulate:
    float rangeCheck = abs(origin.z - sampleDepth) <  radius ? 1.0 : 0.0;
    float normCheck = dot(normal, unitSample) <= 0.01 ? 0.0 : 1.0;
    occlusion += (sampleDepth < sample.z ? (1.0 * rangeCheck * normCheck) : -0.0);
  }

  occlusion /= 16.0;
  occlusion *= intensity;

  float visibility = 1.0 - occlusion;
  gl_FragColor.rgb = vec3(visibility);
  gl_FragColor.a = 1.0;
}






[[FS_BLUR_2]]
uniform sampler2D ssaoImage;

uniform vec2 frameBufSize;

varying vec2 texCoords;

void main() {
  vec2 texelSize = 1.0 / frameBufSize;
  float result = 0.0;
  vec2 hlim = vec2(float(-4) * 0.5);
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      vec2 offset = (hlim + vec2(float(i), float(j))) * texelSize;
      result += texture2D(ssaoImage, texCoords + offset).r;
    }
  }

  vec3 r = vec3(result / float(4 * 4));
  gl_FragColor = vec4(r, 1.0);
}


[[FS_BLUR_3]]
#include "shaders/utilityLib/blur.glsl"

varying vec2 texCoords;
uniform sampler2D depthBuffer;
uniform sampler2D ssaoImage;
uniform vec2 frameBufSize;
uniform vec4 axis;

void main()
{
    float b = 0.0;
    float w_total = 0.0;
    float center_c = texture2D(ssaoImage, texCoords).r;
    float center_d = texture2D(depthBuffer, texCoords).r;

    float g_BlurRadius = 4.0;
    vec2 g_InvResolution = 1.0 / frameBufSize;
    
    for (float r = -g_BlurRadius; r <= g_BlurRadius; ++r)
    {
        vec2 uv = texCoords + axis.xy * r * g_InvResolution;
        b += BlurFunction(uv, r, center_c, center_d, w_total, depthBuffer, ssaoImage);
    }

    float result = b / w_total;
    gl_FragColor = vec4(result, result, result, 1);
}
