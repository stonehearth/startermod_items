[[FX]]

context TRANSLUCENT
{
	VertexShader = compile GLSL VS_TRANSLUCENT;
	PixelShader = compile GLSL FS_TRANSLUCENT;
	
	ZWriteEnable = false;
	BlendMode = AddBlended;
	CullMode = None;
}



[[VS_TRANSLUCENT]]
// =================================================================================================

uniform mat4 viewProjMat;
attribute mat4 particleWorldMatrix;
attribute vec3 vertPos;

void main(void)
{
	gl_Position = viewProjMat * particleWorldMatrix * vec4(vertPos, 1);
}


[[FS_TRANSLUCENT]]
// =================================================================================================

void main( void )
{
	
	gl_FragColor.rgba = vec4(1, 1, 1, 1);
}
