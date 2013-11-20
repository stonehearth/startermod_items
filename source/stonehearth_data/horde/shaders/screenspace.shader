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
