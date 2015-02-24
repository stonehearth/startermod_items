// This is a screen-space shader that renders hud elements in the world, but with a fixed screen-space
// size.  That is, the hud element will not grow/shrink as the user changes their position relative to 
// the object to which the element is attached.
[[FX]]

sampler2D albedoMap = sampler_state
{
  Address = Clamp;
  Filter = Pixely;
};

[[VS]]

attribute vec4 vertPos;
attribute vec2 texCoords0;
attribute vec4 color;

varying vec4 oColor;
varying vec2 texCoords;

void main() {
  texCoords = texCoords0;
  oColor = color;
  gl_Position = vertPos;
}


[[FS]]

uniform sampler2D albedoMap;

varying vec4 oColor;
varying vec2 texCoords;

void main() {
  gl_FragColor = oColor * texture2D(albedoMap, texCoords);
}
