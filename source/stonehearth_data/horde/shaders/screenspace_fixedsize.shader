// This is a screen-space shader that renders hud elements in the world, but with a fixed screen-space
// size.  That is, the hud element will not grow/shrink as the user changes their position relative to 
// the object to which the element is attached.
[[FX]]

sampler2D albedoMap = sampler_state
{
  Address = Clamp;
  Filter = Pixely;
};

context TRANSLUCENT
{
	VertexShader = compile GLSL VS_OVERLAY;
	PixelShader = compile GLSL FS_OVERLAY;
	
	BlendMode = Blend;
	ZWriteEnable = false;
	CullMode = None;
}

[[VS_OVERLAY]]

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


[[FS_OVERLAY]]

uniform sampler2D albedoMap;

attribute vec4 oColor;
attribute vec2 texCoords;

void main() {
   gl_FragColor = oColor * texture2D(albedoMap, texCoords);
}
