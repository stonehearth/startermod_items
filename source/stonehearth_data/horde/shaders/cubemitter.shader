[[FX]]

context TRANSLUCENT
{
	VertexShader = compile GLSL VS_TRANSLUCENT;
	PixelShader = compile GLSL FS_TRANSLUCENT;
	
	ZWriteEnable = false;
	BlendMode = Blend;
	CullMode = None;
}



[[VS_TRANSLUCENT]]
// =================================================================================================

uniform mat4 viewProjMat;
attribute mat4 particleWorldMatrix;
attribute vec3 vertPos;
attribute vec4 cubeColor;
varying vec4 c1;
void main(void)
{
	c1 = cubeColor;
	gl_Position = viewProjMat * particleWorldMatrix * vec4(vertPos, 1);
}


[[FS_TRANSLUCENT]]
// =================================================================================================

varying vec4 c1;
void main( void )
{
	
	gl_FragColor.rgba = c1;
}
