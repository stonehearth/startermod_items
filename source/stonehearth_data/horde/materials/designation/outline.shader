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
uniform    mat4    viewProjMat;
uniform    mat4    worldMat;
uniform    vec4    alpha;
attribute  vec3    vertPos;
attribute  vec3    color;
varying    vec4    vs_color;

void main() {
   vs_color = vec4(color, alpha.a);
	gl_Position = viewProjMat * worldMat * vec4(vertPos, 1.0);
}

[[FS_AMBIENT]]	

varying vec4 vs_color;
void main() {
  gl_FragColor = vs_color;
}
