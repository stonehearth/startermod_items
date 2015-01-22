[[FX]]

sampler2D albedoMap = sampler_state
{
	Filter = Bilinear;
};

[[VS]]

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
   mat4 worldView = viewMat * worldMat;
   gl_Position = projMat * (vec4(vertPos, 1.0) + vec4(worldView[3].xyz, 0));
}

[[FS_AMBIENT]]	

uniform sampler2D albedoMap;
varying vec2 texCoords;

void main() {
   vec4 color = texture2D(albedoMap, texCoords);
   gl_FragColor = vec4(color.rgb, color.a * .75);
}
