[[FX]]

// Samplers
sampler2D cloudMap = sampler_state
{
  Texture = "textures/environment/cloudmap.png";
  Address = Wrap;
   Filter = None;
};

sampler2D lightingBuffer = sampler_state
{
  Address = Clamp;
  Filter = Bilinear;
};

sampler2D ssaoBuffer = sampler_state
{
  Address = Clamp;
  Filter = Bilinear;
};

sampler2D outlineSampler = sampler_state
{
  Address = Clamp;
  Filter = Trilinear;
};

// Contexts
context AMBIENT
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_FORWARD_AMBIENT;
  
  ZWriteEnable = true;
}

context DEPTH_AND_LIGHT_ATTRIBUTES
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_DEFERRED_DEPTH_AND_LIGHT;
}

context MATERIAL
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_DEFERRED_MATERIAL;  
}

context SELECTED_SCREENSPACE
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_SELECTED_SCREENSPACE;
}

context SELECTED_SCREENSPACE_OUTLINER
{
  VertexShader = compile GLSL VS_SELECTED_SCREENSPACE_OUTLINER;
  PixelShader = compile GLSL FS_SELECTED_SCREENSPACE_OUTLINER;
  BlendMode = Blend;
  ZWriteEnable = false;
}

context SHADOWMAP
{
  VertexShader = compile GLSL VS_SHADOWMAP;
  PixelShader = compile GLSL FS_SHADOWMAP;
   CullMode = Back;
}

context DIRECTIONAL_SHADOWMAP
{
  VertexShader = compile GLSL VS_DIRECTIONAL_SHADOWMAP;
  PixelShader = compile GLSL FS_DIRECTIONAL_SHADOWMAP;
   CullMode = Back;
}

context OMNI_LIGHTING
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_OMNI_LIGHTING;
  
  ZWriteEnable = false;
  BlendMode = Add;
  CullMode = Back;
}

context DIRECTIONAL_LIGHTING
{
  VertexShader = compile GLSL VS_GENERAL;
  PixelShader = compile GLSL FS_DIRECTIONAL_LIGHTING;
  
  ZWriteEnable = false;
  BlendMode = Add;
    CullMode = Back;
}


context CLOUDS
{
   VertexShader = compile GLSL VS_GENERAL;
   PixelShader = compile GLSL FS_CLOUDS;

   ZWriteEnable = false;
   BlendMode = Mult;
   CullMode = Back;
}

[[VS_GENERAL]]
#version 150
#include "shaders/utilityLib/vertCommon.glsl"

uniform mat4 viewProjMat;
uniform vec3 viewerPos;
in vec3 vertPos;
in vec3 normal;
in vec3 color;
out vec4 pos;
out vec4 vsPos;
out vec3 tsbNormal;
out vec3 albedo;

void main( void )
{
  pos = calcWorldPos( vec4( vertPos, 1.0 ) );
  vsPos = calcViewPos( pos );
  tsbNormal = calcWorldVec(normal);
  albedo = color;

  gl_Position = viewProjMat * pos;;
}


[[FS_DEFERRED_DEPTH_AND_LIGHT]] 
#version 150
uniform vec3 viewerPos;
in vec4 pos;
in vec3 tsbNormal;
out vec4 fragNormal;
out vec4 fragViewPos;

void main( void )
{
  fragNormal.xyz = normalize(tsbNormal).xyz;
  fragViewPos.xyz = pos.xyz - viewerPos;
}


[[FS_FORWARD_AMBIENT]]  
#version 150

out vec4 fragColor;

void main( void )
{
  fragColor = vec4(0, 0, 0, 1);
}

[[VS_SHADOWMAP]]
// =================================================================================================
  
#include "shaders/utilityLib/vertCommon.glsl"
#include "shaders/utilityLib/vertSkinning.glsl"

uniform mat4 viewProjMat;
uniform vec4 lightPos;
in vec3 vertPos;
varying vec3 lightVec;

void main( void )
{
  vec4 pos = calcWorldPos( vec4( vertPos, 1.0 ) );
  lightVec = lightPos.xyz - pos.xyz;
  gl_Position = viewProjMat * pos;
}
  
  
[[FS_SHADOWMAP]]
// =================================================================================================

uniform vec4 lightPos;
uniform float shadowBias;
in vec3 lightVec;

void main( void )
{
  float dist = length( lightVec ) / lightPos.w;
  gl_FragDepth = dist;// + shadowBias;
  
  // Clearly better bias but requires SM 3.0
  // gl_FragDepth = dist + abs( dFdx( dist ) ) + abs( dFdy( dist ) ) + shadowBias;
}

[[VS_DIRECTIONAL_SHADOWMAP]]
// =================================================================================================
  
#include "shaders/utilityLib/vertCommon.glsl"
#include "shaders/utilityLib/vertSkinning.glsl"

uniform mat4 viewProjMat;
in vec3 vertPos;

void main( void )
{
  vec4 pos = calcWorldPos( vec4( vertPos, 1.0 ) );
  gl_Position = viewProjMat * pos;
}
  
  
[[FS_DIRECTIONAL_SHADOWMAP]]
// =================================================================================================

void main( void )
{
}

[[FS_OMNI_LIGHTING]]
// =================================================================================================
#include "shaders/utilityLib/fragLighting.glsl" 

uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;
uniform sampler2D albedoMap;

in vec4 pos;
in vec4 vsPos;
in vec3 albedo;
in vec3 tsbNormal;
out vec4 outLightColor;

void main( void )
{
  vec3 normal = tsbNormal;
  vec3 newPos = pos.xyz;
  outLightColor.rgb = calcPhongOmniLight(newPos, normalize(normal)) * albedo;
}

[[FS_DIRECTIONAL_LIGHTING]]
// =================================================================================================
#include "shaders/utilityLib/fragLighting.glsl" 

uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;
uniform sampler2D albedoMap;
uniform vec3 lightAmbientColor;

in vec4 pos;
in vec4 vsPos;
in vec3 albedo;
in vec3 tsbNormal;
out vec4 outLightColor;

void main( void )
{
  vec3 lightColor = 
    calcPhongDirectionalLight( pos.xyz, normalize( tsbNormal ), albedo, vec3(0,0,0),
                        0.0, -vsPos.z, 0.3 ) + (lightAmbientColor * albedo);
  outLightColor = vec4(lightColor, 1.0);
}

[[FS_CLOUDS]]
#version 150
in vec4 pos;
uniform sampler2D cloudMap;
uniform float currentTime;
out vec4 cloudColor;

void main( void )
{
  vec2 fragCoord = pos.xz * 0.3;
  float cloudSpeed = currentTime / 80.0;
  cloudColor = texture2D(cloudMap, fragCoord.xy / 128.0 + cloudSpeed);
  cloudColor *= texture2D(cloudMap, fragCoord.yx / 192.0 + (cloudSpeed / 10.0));
}


[[FS_DEFERRED_MATERIAL]]
#version 150
uniform sampler2D lightingBuffer;
uniform sampler2D ssaoBuffer;
uniform vec2 frameBufSize;
in vec3 albedo;
out vec4 fragColor;

void main(void)
{
  vec2 fragCoord = vec2(gl_FragCoord.xy / frameBufSize);
  vec3 lightColor = texture2D(lightingBuffer, fragCoord).xyz;
  float ssaoIntensity = texture2D(ssaoBuffer, fragCoord).x;
  fragColor = vec4(lightColor * albedo * ssaoIntensity, 1.0);
}


[[FS_SELECTED_SCREENSPACE]]
#version 150
out vec4 fragColor;

void main(void)
{
  fragColor = vec4(1, 1, 0, 1);
}


[[VS_SELECTED_SCREENSPACE_OUTLINER]]
#version 150

#include "shaders/utilityLib/fullscreen_quad.glsl" 

uniform mat4 projMat;
in vec3 vertPos;
out vec2 texCoords;
        
void main( void )
{
  transform_fullscreen(vertPos, projMat, gl_Position, texCoords);
}


[[FS_SELECTED_SCREENSPACE_OUTLINER]]
#version 150

#include "shaders/utilityLib/outline.glsl"

uniform sampler2D outlineSampler;

in vec2 texCoords;
out vec4 fragColor;

void main(void)
{
  fragColor = compute_outline_color(outlineSampler, texCoords);
}
