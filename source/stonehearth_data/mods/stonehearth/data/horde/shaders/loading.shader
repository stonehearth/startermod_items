[[FX]]

// Samplers
sampler2D loadingMap  = sampler_state
{
   Filter = Trilinear;
};

[[VS]]

uniform mat4 projMat;
attribute vec2 vertPos;
varying vec2 texCoords;

void main( void )
{
   texCoords = vec2( vertPos.x, -vertPos.y ); 
   gl_Position = projMat * vec4( vertPos.x, vertPos.y, 1, 1 );
}


[[FS]]

uniform sampler2D loadingMap;
varying vec2 texCoords;

void main( void )
{
   vec4 albedo = texture2D( loadingMap, texCoords );
   
   gl_FragColor = albedo;
}
