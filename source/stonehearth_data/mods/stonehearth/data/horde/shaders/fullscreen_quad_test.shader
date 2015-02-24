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
  vec4 col = texture2D(buf0, texCoords);

  if (col.a == 0.0) {
  	discard;
  }
  gl_FragColor = vec4(col.rgb, 1.0);
}
