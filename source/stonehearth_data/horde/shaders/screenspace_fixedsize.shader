// This is a screen-space shader that renders hud elements in the world, but with a fixed screen-space
// size.  That is, the hud element will not grow/shrink as the user changes their position relative to 
// the object to which the element is attached.
[[FX]]

context TRANSLUCENT
{
	VertexShader = compile GLSL VS_OVERLAY;
	PixelShader = compile GLSL FS_OVERLAY;
	
	BlendMode = Blend;
	ZWriteEnable = false;
	CullMode = None;
}

[[VS_OVERLAY]]
#version 130

in vec4 vertPos;

void main() {
	gl_Position = vertPos;
}


[[FS_OVERLAY]]
#version 130

out vec4 fragColor;

void main() {
   fragColor = vec4(1, 1, 1, 1);
};
