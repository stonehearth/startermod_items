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
uniform vec2 viewPortSize;
uniform mat4 viewProjMat;
uniform mat4 worldMat;

attribute vec4 vertPos;
attribute vec2 texCoords0;
attribute vec4 color;

varying vec4 oColor;
varying vec2 texCoords;

void main() {
  texCoords = texCoords0;
  oColor = color;

  vec4 origin = viewProjMat * worldMat * vec4(0.0, 0.0, 0.0, 1.0);
  vec2 offset = vertPos.zw;
  vec2 scale = origin.w * vec2(2.0 / viewPortSize.x, 2.0 / viewPortSize.y);
  vec4 screenPos = vec4(vertPos.xy + offset, 0, 0);

  gl_Position = origin + screenPos * scale.xyxy;
}


[[FS]]

uniform sampler2D albedoMap;

varying vec4 oColor;
varying vec2 texCoords;

void main() {
  gl_FragColor = oColor * texture2D(albedoMap, texCoords);
}
