[[FX]]

// Samplers
sampler2D cloudMap = sampler_state
{
  Texture = "textures/environment/cloudmap.png";
  Address = Wrap;
   Filter = None;
};

sampler2D depthBuf = sampler_state
{
  Address = Clamp;
};

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

samplerCube ambientMap = sampler_state
{
  Address = Clamp;
  Filter = Bilinear;
  MaxAnisotropy = 1;
};

// Contexts
context AMBIENT
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_AMBIENT;
  
  ZWriteEnable = false;
  BlendMode = Replace;
}

context BLUEPRINT_AMBIENT
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_BLUEPRINT_AMBIENT;
  
  ZWriteEnable = false;
  BlendMode = Replace;
}

context LIGHTING
{
  VertexShader = compile GLSL VS_VOLUME;
  PixelShader = compile GLSL FS_LIGHTING;
  
  ZWriteEnable = false;
  BlendMode = Add;
}

context DIRECTIONAL_LIGHTING
{
  VertexShader = compile GLSL VS_VOLUME_DIRECTIONAL;
  PixelShader = compile GLSL FS_LIGHTING_DIRECTIONAL;
  
  ZWriteEnable = false;
  BlendMode = Add;
}

context COPY_DEPTH
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_COPY_DEPTH;
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

[[VS_VOLUME]]

uniform mat4 viewProjMat;
uniform mat4 worldMat;
attribute vec3 vertPos;
varying vec4 vpos;
        
void main( void )
{
  vpos = viewProjMat * worldMat * vec4( vertPos, 1 );
  gl_Position = vpos;
}


[[FS_AMBIENT]]

#include "shaders/utilityLib/fragDeferredRead.glsl"

varying vec2 texCoords;
void main( void )
{
  if( getMatID( texCoords ) == 2.0 )  // Sky
  {
    gl_FragColor.rgb = getAlbedo( texCoords );
  }
  else
  {
    gl_FragColor = vec3(0,0,0);
  }
}



[[FS_BLUEPRINT_AMBIENT]]

#include "shaders/utilityLib/fragDeferredRead.glsl"

uniform samplerCube ambientMap;
varying vec2 texCoords;

void main( void )
{
  if( getMatID( texCoords ) == 0.0 )  // Background
  {
    gl_FragColor.rgb = vec3(255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0);
  }
  else if( getMatID( texCoords ) == 2.0 ) // Sky
  {
    gl_FragColor.rgb = getAlbedo( texCoords );
  }
  else
  {
    gl_FragColor.rgb = getAlbedo( texCoords ) * textureCube( ambientMap, getNormal( texCoords ) ).rgb;
    gl_FragColor.rgb = getAlbedo( texCoords );
    gl_FragColor.rgb = getAlbedo( texCoords ) * vec3(0.5, 0.5, 0.5);
  }
}


[[FS_LIGHTING]]

#include "shaders/utilityLib/fragLighting.glsl"
#include "shaders/utilityLib/fragDeferredRead.glsl"

uniform mat4 viewMat;
varying vec4 vpos;

void main( void )
{
  vec2 fragCoord = (vpos.xy / vpos.w) * 0.5 + 0.5;
  
  if( getMatID( fragCoord ) == 1.0 )  // Standard phong material
  {
    vec3 pos = getPos( fragCoord ) + viewerPos;
    float vsPos = (viewMat * vec4( pos, 1.0 )).z;
    vec4 specParams = getSpecParams( fragCoord );
    
    gl_FragColor.rgb =
      calcPhongSpotLight( pos, getNormal( fragCoord ),
                getAlbedo( fragCoord ), specParams.rgb, specParams.a, -vsPos, 0.3 );
  }
  else discard;
}


[[VS_VOLUME_DIRECTIONAL]]

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
#include "shaders/utilityLib/fragDeferredRead.glsl"

uniform mat4 viewMat;
varying vec4 vpos;
uniform sampler2D cloudMap;
uniform float currentTime;
uniform vec3 lightAmbientColor;

void main( void )
{
  vec2 fragCoord = (vpos.xy / vpos.w) * 0.5 + 0.5;
   //vec2 fragCoord = vpos.xy;
  
  if( getMatID( fragCoord ) == 1.0 )  // Standard phong material
  {
    vec3 pos = getPos( fragCoord ) + viewerPos;
    float vsPos = (viewMat * vec4( pos, 1.0 )).z;
    vec4 specParams = getSpecParams( fragCoord );
    
    float cloudSpeed = currentTime / 80.0;
    vec4 cloudColor = texture2D(cloudMap, pos.xz / 128.0 + cloudSpeed);
    cloudColor *= texture2D(cloudMap, pos.zx / 192.0 + (cloudSpeed / 10.0));
    vec3 albedo = getAlbedo( fragCoord );

    gl_FragColor.rgb =
      calcPhongDirectionalLight( pos, getNormal( fragCoord ),
                     albedo, specParams.rgb, specParams.a, -vsPos, 0.3 ) + (albedo * lightAmbientColor);

    gl_FragColor *= cloudColor;
  }
  else discard;
}

[[FS_COPY_DEPTH]]

uniform sampler2D depthBuf;
varying vec2 texCoords;

void main( void )
{
  gl_FragDepth = texture2D( depthBuf, texCoords ).r;
}

[[FS_WRITE_GREEN]]

void main( void )
{
   gl_FragColor.rgb = vec3(0, 1, 0);
}

[[FS_WRITE_VARYING_VPOS]]

varying vec4 vpos;
void main( void )
{
   vec2 fragCoord = (vpos.xy / vpos.w) * 0.5 + 0.5;
   gl_FragColor.rgb = vec3(fragCoord.xy, 1);
}

[[FS_WRITE_GBUF_MATID]]

#include "shaders/utilityLib/fragDeferredRead.glsl"
varying vec4 vpos;

void main( void )
{
   vec2 fragCoord = (vpos.xy / vpos.w) * 0.5 + 0.5;
   gl_FragColor.rgb = vec3(getMatID(fragCoord), 0, 0);
}

[[FS_WRITE_GBUF_POS]]

#include "shaders/utilityLib/fragDeferredRead.glsl"
varying vec4 vpos;

void main( void )
{
   // this is buggy...
   vec2 fragCoord = (vpos.xy / vpos.w) * 0.5 + 0.5;
   gl_FragColor.rgb = getPos(fragCoord);
}

[[FS_WRITE_GBUF_NORMAL]]

#include "shaders/utilityLib/fragDeferredRead.glsl"
varying vec4 vpos;

void main( void )
{
   vec2 fragCoord = (vpos.xy / vpos.w) * 0.5 + 0.5;
   vec3 normal = getNormal(fragCoord);
   gl_FragColor.rgb = normal * 0.5 + 0.5;
}

[[FS_WRITE_GBUF_ALBEDO]]

#include "shaders/utilityLib/fragDeferredRead.glsl"
varying vec4 vpos;

void main( void )
{
   vec2 fragCoord = (vpos.xy / vpos.w) * 0.5 + 0.5;
   gl_FragColor.rgb = getAlbedo(fragCoord);
}

[[FS_WRITE_GBUF_SPEC_PARAMS]]

#include "shaders/utilityLib/fragDeferredRead.glsl"
varying vec4 vpos;

void main( void )
{
   vec2 fragCoord = (vpos.xy / vpos.w) * 0.5 + 0.5;
   gl_FragColor = getSpecParams(fragCoord);
}
