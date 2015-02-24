[[FX]]

sampler2D progressMap  = sampler_state
{
   Filter = Trilinear;
};

[[VS]]
uniform mat4 projMat;

attribute vec2 vertPos;
attribute vec2 texCoords0;

varying vec2 texCoords;

void main() {
  texCoords = texCoords0;
  gl_Position = projMat * vec4( vertPos.x, vertPos.y, 1, 1 );
}


[[FS]]

uniform sampler2D progressMap;

varying vec2 texCoords;

void main() {
  gl_FragColor = texture2D(progressMap, texCoords);
}
