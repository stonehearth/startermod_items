[[FX]]

context DEBUG_SHAPES
{
	VertexShader = compile GLSL VS_GENERAL;
	PixelShader = compile GLSL FS_AMBIENT;
	ZWriteEnable = false;
   CullMode = None;
	BlendMode = Blend;
}


[[VS_GENERAL]]

uniform mat4 viewProjMat;
uniform mat4 worldMat;
attribute vec3 vertPos;
attribute vec4 inputColor;
varying vec4 theColor;
void main() {
	theColor = inputColor;
	gl_Position = viewProjMat * worldMat * vec4(vertPos, 1.0);
}

[[FS_AMBIENT]]	

varying vec4 theColor;
void main() {
	gl_FragColor = theColor;
   gl_FragColor.a = 0.5;
};

