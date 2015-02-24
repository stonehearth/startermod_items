[[FX]]

sampler2D buf0 = sampler_state
{
  Address = Clamp;
  Filter = None;
};

[[VS]]
uniform mat4 projMat;
attribute vec3 vertPos;
varying vec2 texCoords;
				
void main( void )
{
	texCoords = vertPos.xy; 
	gl_Position = projMat * vec4( vertPos, 1.0 );
}


[[FS]]
uniform sampler2D buf0;
varying vec2 texCoords;

void main( void )
{
   gl_FragColor = texture2D(buf0, texCoords);
}
