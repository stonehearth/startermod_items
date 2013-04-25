[[FX]]

// Samplers
sampler2D cloudMap = sampler_state
{
	Texture = "textures/environment/cloudmap.jpg";
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

context LIGHTING
{
	VertexShader = compile GLSL VS_VOLUME;
	PixelShader = compile GLSL FS_LIGHTING;
	
	ZWriteEnable = false;
	BlendMode = Add;
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

[[FS_LIGHTING]]

#include "shaders/utilityLib/fragDeferredRead.glsl"
#include "shaders/utilityLib/fragLighting.glsl"

uniform mat4 viewMat;
uniform sampler2D cloudMap;
   
uniform 	vec3 viewerPos;

varying vec4 vpos;
uniform float currentTime;

void main( void )
{
	vec2 fragCoord = (vpos.xy / vpos.w) * 0.5 + 0.5;
	
	if( getMatID( fragCoord ) == 1.0 )	// Standard phong material
	{
		vec3 pos = getPos( fragCoord ) + viewerPos;
		float vsPos = (viewMat * vec4( pos, 1.0 )).z;
		vec4 specParams = getSpecParams( fragCoord );
		
      float cloudSpeed = currentTime / 10.0;
      vec4 cloudColor = texture2D(cloudMap, pos.xz / 64.0 + cloudSpeed);

		gl_FragColor.rgb =
			calcPhongSpotLight( pos, getNormal( fragCoord ),
								getAlbedo( fragCoord ), specParams.rgb, specParams.a, -vsPos, 0.3 );
	}
	else discard;
}
