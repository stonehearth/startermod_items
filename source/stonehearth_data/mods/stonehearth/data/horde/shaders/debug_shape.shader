[[FX]]

[[VS]]

uniform mat4 viewProjMat;
uniform mat4 worldMat;

attribute vec3 vertPos;
attribute vec4 inputColor;

varying vec4 theColor;

void main() {
	theColor = inputColor;
	gl_Position = viewProjMat * worldMat * vec4(vertPos, 1.0);
}

[[FS]]	

varying vec4 theColor;

void main() {
  gl_FragColor = theColor;
  gl_FragColor.a = 0.5;
}

