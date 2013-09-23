[[FX]]

sampler2D normals = sampler_state
{
  Address = Clamp;
  Filter = Bilinear;
};

sampler2D depth = sampler_state
{
  Address = Clamp;
  Filter = Bilinear;
};

sampler2D positions = sampler_state
{
  Address = Clamp;
  Filter = Bilinear;
};

context DIRECTIONAL_LIGHTING
{
  VertexShader = compile GLSL VS_LIGHTING_DIRECTIONAL;
  PixelShader = compile GLSL FS_LIGHTING_DIRECTIONAL;
  
  ZWriteEnable = false;
  BlendMode = Add;
}


[[VS_LIGHTING_DIRECTIONAL]]

uniform mat4 projMat;
attribute vec3 vertPos;
varying vec4 vpos;
        
void main( void )
{
  vpos = projMat * vec4( vertPos, 1 );
  gl_Position = vpos;
}

[[FS_LIGHTING_DIRECTIONAL]]

#include "shaders/utilityLib/fragLighting.glsl" 

// uniform vec3 viewerPos; -- already declared.  harumph.
uniform mat4 viewMat;
varying vec4 vpos;
uniform sampler2D depth;
uniform sampler2D positions;
uniform sampler2D normals;
uniform vec3 lightAmbientColor;
uniform vec2 frameBufSize;

void main( void )
{
  vec2 fragCoord = vec2(gl_FragCoord.xy / frameBufSize);

  vec3 pos = texture2D(positions, fragCoord).xyz + viewerPos;
  float vsPos = (viewMat * vec4( pos, 1.0 )).z;

  vec3 normal = texture2D(normals, fragCoord).xyz;
  vec3 intensity = calcSimpleDirectionalLight( pos, normal, -vsPos, 0.3 ) + lightAmbientColor;
  gl_FragColor = vec4(intensity, 1.0);
}
