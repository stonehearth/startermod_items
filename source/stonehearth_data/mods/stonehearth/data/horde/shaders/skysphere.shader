[[FX]]

float4 skycolor_start;
float4 skycolor_end;

[[VS]]

uniform mat4 worldMat;
uniform mat4 projMat;

uniform vec4 skycolor_start;
uniform vec4 skycolor_end;

attribute vec3 vertPos;
attribute vec2 texCoords0;

varying float colorT;
varying vec4 startCol;
varying vec4 endCol;

void main() {
  // Compress the gradient to occupy the most visible band of the sky.
  colorT = (12.0 * texCoords0.y) - 2.75;
  
  startCol = vec4(skycolor_start.rgb, 0.0);
  endCol = vec4(skycolor_end.rgb, 0.0);
  
  // We rotate the sphere just a bit, in order to minimize the perspective warping
  // of the sphere.
  vec3 worldPos = (worldMat * vec4(vertPos.x, vertPos.y - 0.5, vertPos.z, 0.0)).xyz;
  gl_Position = projMat * vec4(worldPos, 1.0);
}

[[FS]]
varying float colorT;
varying vec4 startCol;
varying vec4 endCol;

void main() {
  float clampedT = clamp(colorT, 0.0, 1.0);
  gl_FragColor = startCol * (1.0 - clampedT) + clampedT * endCol;
}
