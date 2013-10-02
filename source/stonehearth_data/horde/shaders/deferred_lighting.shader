[[FX]]

sampler2D normals = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D depth = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D positions = sampler_state
{
  Address = Clamp;
  Filter = None;
};

context DIRECTIONAL_LIGHTING
{
  VertexShader = compile GLSL VS_LIGHTING_VERTEX_SHADER;
  PixelShader = compile GLSL FS_LIGHTING_DIRECTIONAL;
  
  ZWriteEnable = false;
  BlendMode = Add;
}

context OMNI_LIGHTING
{
  VertexShader = compile GLSL VS_OMNI_LIGHTING_VERTEX_SHADER;
  PixelShader = compile GLSL FS_OMNI_LIGHTING;
  
  ZWriteEnable = false;
  BlendMode = Add;
}


[[VS_LIGHTING_VERTEX_SHADER]]

uniform mat4 projMat;
attribute vec3 vertPos;
        
void main( void )
{
  gl_Position = projMat * vec4( vertPos, 1 );;
}

[[FS_LIGHTING_DIRECTIONAL]]
#include "shaders/utilityLib/fragLighting.glsl" 

// uniform vec3 viewerPos; -- already declared.  harumph.
uniform mat4 viewMat;
uniform sampler2D positions;
uniform sampler2D normals;
uniform vec3 lightAmbientColor;
uniform vec2 frameBufSize;

out vec4 outLightColor;

void main( void )
{
  vec2 fragCoord = vec2(gl_FragCoord.xy / frameBufSize);

  vec3 pos = texture2D(positions, fragCoord).xyz + viewerPos;
  float vsPos = (viewMat * vec4( pos, 1.0 )).z;

  vec3 normal = texture2D(normals, fragCoord).xyz;
  vec3 intensity = calcSimpleDirectionalLight(pos, normal, -vsPos) + lightAmbientColor;
  outLightColor = vec4(intensity, 1.0);
}



// =================================================================================================
[[VS_OMNI_LIGHTING_VERTEX_SHADER]]

uniform mat4 viewProjMat;
uniform mat4 viewMat;
uniform mat4 worldMat;
uniform mat4 camViewProjMatInv;
attribute vec3 vertPos;
        
void main( void )
{
  gl_Position = viewProjMat * worldMat * vec4(vertPos, 1);
}

[[FS_OMNI_LIGHTING]]
#include "shaders/utilityLib/fragLighting.glsl" 

// uniform vec3 viewerPos; -- already declared.  harumph.
uniform mat4 viewMat;
uniform sampler2D positions;
uniform sampler2D normals;
uniform vec2 frameBufSize;

out vec4 outLightColor;

void main( void )
{
  vec2 fragCoord = vec2(gl_FragCoord.xy / frameBufSize);

  vec3 pos = texture2D(positions, fragCoord).xyz + viewerPos;

  vec3 normal = texture2D(normals, fragCoord).xyz;
  vec3 intensity = calcPhongOmniLight(pos, normalize(normal));

  outLightColor = vec4(intensity, 1.0);
}
