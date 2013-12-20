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

attribute vec4 vertPos;

void main() {
	gl_Position = vertPos;
}


[[FS_OVERLAY]]

void main() {
  gl_FragColor = vec4(1, 1, 1, 1);
}
