[[FX]]

// Samplers
sampler2D loadingMap  = sampler_state
{
   Filter = Trilinear;
};
sampler2D progressMap  = sampler_state
{
   Filter = Trilinear;
};

// Contexts
context LOADING_OVERLAY
{
   VertexShader = compile GLSL VS_OVERLAY;
   PixelShader = compile GLSL FS_OVERLAY;
   
   BlendMode = Blend;
   ZWriteEnable = false;
}

context PROGRESS_OVERLAY
{
   VertexShader = compile GLSL VS_PROGRESS;
   PixelShader = compile GLSL FS_PROGRESS;
   
   ZWriteEnable = false;
   CullMode = None;
}

[[VS_OVERLAY]]

uniform mat4 projMat;
attribute vec2 vertPos;
attribute vec2 texCoord0;
varying vec2 texCoords;

void main( void )
{
   texCoords = vec2( vertPos.x, -vertPos.y ); 
   gl_Position = /*projMat **/ vec4( vertPos.x, vertPos.y, 1, 1 );
}


[[FS_OVERLAY]]

uniform sampler2D loadingMap;
varying vec2 texCoords;

void main( void )
{
   vec4 albedo = texture2D( loadingMap, texCoords );
   
   gl_FragColor = vec4(1,1,1,1);//albedo;
}



[[VS_PROGRESS]]

attribute vec4 vertPos;
//attribute vec2 texCoords0;

varying vec2 texCoords;

void main() {
  texCoords = vec2(vertPos.x/10.0, -vertPos.y/10.0);//texCoords0;
  gl_Position = vertPos;
}


[[FS_PROGRESS]]

uniform sampler2D progressMap;

varying vec2 texCoords;

void main() {
  gl_FragColor = texture2D(progressMap, texCoords);
}
