[[FX]]

context TRANSLUCENT
{
  VertexShader = compile GLSL VS_TRANSLUCENT;
  PixelShader = compile GLSL FS_TRANSLUCENT;
  
  ZWriteEnable = false;
  BlendMode = Blend;
  CullMode = Back;
}



[[VS_TRANSLUCENT]]
// =================================================================================================

uniform mat4 viewProjMat;
varying vec4 color;
attribute vec3 vertPos;

#ifdef _F01_INSTANCE_SUPPORT
  attribute vec4 cubeColor;
  attribute mat4 particleWorldMatrix;
#else
  attribute float parIdx;
  uniform mat4 cubeBatchTransformArray[32];
  uniform vec4 cubeBatchColorArray[32];

  vec4 calcBatchCubemitterPos(const vec3 localCoords)
  {
    int index = int(parIdx);
    vec3 cornerPos = localCoords - vec3(0.5, 0.5, 0.5);
    return cubeBatchTransformArray[index] * vec4(cornerPos, 1);
  }
#endif


void main(void)
{

  #ifdef _F01_INSTANCE_SUPPORT
    color = cubeColor;
    gl_Position = viewProjMat * particleWorldMatrix * vec4(vertPos, 1);
  #else
    color = cubeBatchColorArray[int(parIdx)];
    gl_Position = viewProjMat * vec4(calcBatchCubemitterPos(vertPos).xyz, 1);
  #endif
}


[[FS_TRANSLUCENT]]
// =================================================================================================

varying vec4 color;
void main( void )
{
  gl_FragColor.rgba = color;
}
