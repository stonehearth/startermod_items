[[FX]]

sampler2D depthImage = sampler_state
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
uniform sampler2D depthImage;
varying vec2 texCoords;

void main( void )
{
   gl_FragDepth = texture2D(depthImage, texCoords).r;
   gl_FragColor = vec4(0);
}
