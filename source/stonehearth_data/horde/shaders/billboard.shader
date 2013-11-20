[[FX]]

// Samplers
sampler2D albedoMap  = sampler_state
{
	Filter = Pixely;
	Address = Clamp;
};


// Contexts
context TRANSLUCENT
{
	VertexShader = compile GLSL VS_OVERLAY;
	PixelShader = compile GLSL FS_OVERLAY;
	
	BlendMode = Blend;
	ZWriteEnable = true;
	CullMode = None;
}

[[VS_OVERLAY]]
#version 130

uniform mat4 viewProjMat;
uniform mat4 worldMat;
uniform mat4 viewMat;
uniform mat4 projMat;

in vec3 vertPos;
in vec2 texCoords0;
in vec4 color;

out vec2 texCoords;
out vec4 oColor;

void main() {
	texCoords = vec2(texCoords0.x, texCoords0.y);
	oColor = color;
	gl_Position = projMat * ((viewMat * worldMat * vec4(0, 0, 0, 1)) + vec4(vertPos.x, vertPos.y, 0, 0));

	//gl_Position = projMat * ((viewMat * worldMat * vec4(0, vertPos.y, 0, 1)) + vec4(vertPos.x, 0, 0, 0));
}


[[FS_OVERLAY]]
#version 130

uniform sampler2D albedoMap;

in vec2 texCoords;
in vec4 oColor;

out vec4 fragColor;

void main() {
   fragColor = texture2D(albedoMap, texCoords) * oColor;
}
