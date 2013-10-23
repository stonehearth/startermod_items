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
#version 150

uniform mat4 viewProjMat;
uniform mat4 worldMat;
uniform vec2 frameBufSize;
uniform mat4 viewMat;
uniform mat4 projMat;
uniform float nearPlane;
uniform float farPlane;

// In pixels!  (Ignore the 'z').
in vec3 vertPos;

void main() {
	vec4 clipSpaceOrigin = viewProjMat * worldMat * vec4(0, 0, 0, 1);
	vec2 vScreenPos = vertPos.xy;
	vScreenPos /= frameBufSize;
	vScreenPos *= 2;
	vScreenPos *= clipSpaceOrigin.w;

	vec4 result = clipSpaceOrigin + vec4(vScreenPos, 0, 0);
	gl_Position = result;
}


[[FS_OVERLAY]]
#version 150

out vec4 fragColor;

void main() {
   fragColor = vec4(1, 1, 1, 1);
};
