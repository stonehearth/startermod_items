[[FX]]

context DEBUG_SHAPES
{
	VertexShader = compile GLSL VS_GENERAL;
	PixelShader = compile GLSL FS_AMBIENT;
	ZWriteEnable = false;
	BlendMode = Blend;
   CullMode = None;
}

sampler2D characterMap = sampler_state
{
	Address = Clamp;
	Filter = Bilinear;
};

[[VS_GENERAL]]

uniform   mat4    viewProjMat;
uniform   mat4    projMat;
uniform   mat4    viewMat;
uniform   mat4    worldMat;
attribute vec3    vertPos;
attribute vec2    texCoords0;
varying   vec2    texCoords;


// shader resources!!
// http://o3d.googlecode.com/svn/!svn/bc/219/trunk/samples_webgl/shaders/billboard-glsl.shader

void main() {
	texCoords = texCoords0;
	//gl_Position = (viewProjMat * worldMat * vec4(0.0, 0.0, 0.0, 1.0)) + vec4(vertPos, 1.0);
   mat4 worldView = viewMat * worldMat;
   gl_Position = projMat * (vec4(vertPos, 1.0) + vec4(worldView[3].xyz, 0));
}

[[FS_AMBIENT]]	

uniform sampler2D characterMap;
varying vec2 texCoords;

void main() {
   vec4 color = texture(characterMap, texCoords);
   gl_FragColor = color;
}
