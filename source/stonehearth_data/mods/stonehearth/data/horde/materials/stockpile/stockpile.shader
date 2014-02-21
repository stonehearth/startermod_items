[[FX]]

sampler2D depthBuf = sampler_state
{
	Address = Clamp;
};

sampler2D albedoMap = sampler_state
{
   Address = Wrap;
	Texture = "textures/common/white.tga";
   Filter = None;
};

context TRANSLUCENT
{
	VertexShader = compile GLSL VS_GENERAL;
	PixelShader = compile GLSL FS_AMBIENT;
	ZWriteEnable = false;
	BlendMode = Blend;
   CullMode = None;
}


[[VS_GENERAL]]

uniform   mat4    viewProjMat;
uniform   mat4    worldMat;
attribute vec3    vertPos;
attribute vec2    texCoords0;
varying   vec2    texCoords;

void main() {
	texCoords = texCoords0;
	gl_Position = viewProjMat * worldMat * vec4(vertPos, 1.0);
}

[[FS_AMBIENT]]	
uniform sampler2D depthBuf;
uniform sampler2D albedoMap;
varying vec2      texCoords;

void main() {
   // this is the good one...
   gl_FragColor = texture2D(albedoMap, texCoords);
};

