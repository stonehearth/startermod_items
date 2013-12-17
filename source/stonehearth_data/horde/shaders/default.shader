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

sampler2D skySampler = sampler_state
{
  Address = Clamp;
  Filter = Trilinear;
};

sampler2D outlineSampler = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D outlineDepth = sampler_state
{
  Filter = None;
  Address = Clamp;
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
  ZWriteEnable = true;
  CullMode = None;
}

context SELECTED_SCREENSPACE_OUTLINER
{
  VertexShader = compile GLSL VS_SELECTED_SCREENSPACE_OUTLINER;
  PixelShader = compile GLSL FS_SELECTED_SCREENSPACE_OUTLINER;
  BlendMode = Blend;
  ZWriteEnable = false;
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

context DIRECTIONAL_SHADOWMAP
{
  VertexShader = compile GLSL VS_DIRECTIONAL_SHADOWMAP;
  PixelShader = compile GLSL FS_DIRECTIONAL_SHADOWMAP;
  CullMode = Back;
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


[[FS_DEFERRED_DEPTH_AND_LIGHT]] 

uniform vec3 viewerPos;

varying vec4 pos;
varying vec3 tsbNormal;

void main( void )
{
  gl_FragData[0].xyz = normalize(tsbNormal).xyz;
  gl_FragData[1].xyz = pos.xyz - viewerPos;
}


[[FS_FORWARD_AMBIENT]]  

void main( void )
{
  gl_FragColor = vec4(0, 0, 0, 1);
}


[[FS_OMNI_LIGHTING]]
// =================================================================================================

#include "shaders/utilityLib/fragLighting.glsl" 

uniform vec3 viewerPos;
uniform vec4 matDiffuseCol;
uniform vec4 matSpecParams;
uniform sampler2D albedoMap;

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

uniform sampler2D cloudMap;
uniform sampler2D skySampler;
uniform vec3 viewerPos;
uniform vec3 lightAmbientColor;
uniform vec2 frameBufSize;
uniform float currentTime;

varying vec4 pos;
varying vec4 vsPos;
varying vec3 albedo;
varying vec3 tsbNormal;

void main( void )
{
  // Shadows.
  float shadowTerm = getShadowValue(pos.xyz);

  // Light.
  vec3 lightColor = calcSimpleDirectionalLight(viewerPos, pos.xyz, normalize(tsbNormal), -vsPos.z);
  lightColor = (shadowTerm * (lightColor * albedo)) + (lightAmbientColor * albedo);
  
  // Fog.
  float fogFac = clamp(exp(-vsPos.z / 1000.0) - 1.7, 0.0, 1.0);
  vec3 fogColor = texture2D(skySampler, vec2(gl_FragCoord.x/frameBufSize.x, gl_FragCoord.y / frameBufSize.y)).xyz;

  // Clouds.
  float cloudSpeed = currentTime / 80.0;
  vec2 fragCoord = pos.xz * 0.3;
  vec3 cloudColor = texture2D(cloudMap, fragCoord.xy / 128.0 + cloudSpeed).xyz;
  cloudColor = cloudColor * texture2D(cloudMap, fragCoord.yx / 192.0 + (cloudSpeed / 10.0)).xyz;

  // Mix it all together!  We want to fade to a single fog color, so only mix in the cloud
  // color with the calculated light color.
  gl_FragColor.rgb = mix(lightColor * cloudColor, fogColor, fogFac);
  gl_FragColor.a = 1.0;
}

[[VS_DIRECTIONAL_SHADOWMAP]]
#include "shaders/utilityLib/vertCommon.glsl"
#include "shaders/utilityLib/vertSkinning.glsl"

uniform mat4 viewProjMat;

attribute vec3 vertPos;

void main( void )
{
  vec4 pos = calcWorldPos( vec4( vertPos, 1.0 ) );
  gl_Position = viewProjMat * pos;
}
  
  
[[FS_DIRECTIONAL_SHADOWMAP]]
void main( void )
{
  gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
}


[[FS_DEFERRED_MATERIAL]]

uniform sampler2D lightingBuffer;
uniform sampler2D ssaoBuffer;
uniform vec2 frameBufSize;

varying vec3 albedo;

void main(void)
{
  vec2 fragCoord = vec2(gl_FragCoord.xy / frameBufSize);
  vec3 lightColor = texture2D(lightingBuffer, fragCoord).xyz;
  float ssaoIntensity = texture2D(ssaoBuffer, fragCoord).x;
  gl_FragColor = vec4(lightColor * albedo * ssaoIntensity, 1.0);
}


[[FS_SELECTED_SCREENSPACE]]

void main(void)
{
  gl_FragColor = vec4(1, 1, 0, 1);
}


[[VS_SELECTED_SCREENSPACE_OUTLINER]]

#include "shaders/utilityLib/fullscreen_quad.glsl" 

uniform mat4 projMat;

attribute vec3 vertPos;

varying vec2 texCoords;
        
void main( void )
{
  transform_fullscreen(vertPos, projMat, gl_Position, texCoords);
}


[[FS_SELECTED_SCREENSPACE_OUTLINER]]

#include "shaders/utilityLib/outline.glsl"

uniform sampler2D outlineSampler;
uniform sampler2D outlineDepth;

varying vec2 texCoords;

void main(void)
{
  gl_FragColor = compute_outline_color(outlineSampler, texCoords);
  gl_FragDepth = compute_outline_depth(outlineDepth, texCoords);
}
