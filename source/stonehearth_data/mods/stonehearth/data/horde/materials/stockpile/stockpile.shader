[[FX]]

sampler2D albedoMap = sampler_state
{
   Address = Wrap;
   Texture = "textures/common/white.tga";
   Filter = None;
};

[[VS]]
uniform   mat4    viewProjMat;
uniform   mat4    worldMat;
attribute vec3    vertPos;
attribute vec2    texCoords0;
varying   vec2    texCoords;

void main() {
	texCoords = texCoords0;
	gl_Position = viewProjMat * worldMat * vec4(vertPos, 1.0);
}


[[FS]]	
uniform sampler2D albedoMap;
varying vec2      texCoords;

void main() {
   gl_FragColor = texture2D(albedoMap, texCoords);
}
