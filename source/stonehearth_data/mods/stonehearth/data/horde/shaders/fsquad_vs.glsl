uniform mat4 projMat;

attribute vec3 vertPos;

varying vec2 texCoords;
        
void main( void )
{
  texCoords = vertPos.xy;
  gl_Position = projMat * vec4(vertPos, 1.0);
}
