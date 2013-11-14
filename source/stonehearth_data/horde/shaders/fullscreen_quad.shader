[[FX]]

// Samplers

sampler2D buf0 = sampler_state
{
  Address = Clamp;
  Filter = None;
};

context FSQUAD_COPY
{
	VertexShader = compile GLSL VS_FSQUAD;
	PixelShader = compile GLSL FS_FSQUAD_COPY;
	
	ZWriteEnable = false;
}

[[VS_FSQUAD]]

uniform mat4 projMat;
attribute vec3 vertPos;
varying vec2 texCoords;
				
void main( void )
{
	texCoords = vertPos.xy; 
	gl_Position = projMat * vec4( vertPos, 1.0 );
}


[[FS_FSQUAD_COPY]]

uniform sampler2D buf0;
varying vec2 texCoords;

void main( void )
{
   gl_FragColor = vec4(texture2D(buf0, texCoords).xyz, 1.0);
}
