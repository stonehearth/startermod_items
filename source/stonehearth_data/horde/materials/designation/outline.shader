[[FX]]
float4 alpha = {0, 0, 0, 0.5};

context TRANSLUCENT_ON_TOP
{
	VertexShader = compile GLSL VS_GENERAL;
	PixelShader = compile GLSL FS_AMBIENT;
	ZWriteEnable = false;
	BlendMode = Blend;
}


[[VS_GENERAL]]
#version 130

uniform   mat4    viewProjMat;
uniform   mat4    worldMat;
uniform   vec4    alpha;
in        vec3    vertPos;
in        vec3    color;
out       vec4    vs_color;

void main() {
   vs_color = vec4(color, alpha.a);
	gl_Position = viewProjMat * worldMat * vec4(vertPos, 1.0);
}

[[FS_AMBIENT]]	
#version 130

in       vec4    vs_color;
out      vec4    fragment_color;
void main() {
   fragment_color = vs_color;
};

