// *************************************************************************************************
// Common functions for cubemitters; this hides the differing APIs needed for batching/instancing.
// *************************************************************************************************
in vec3 vertPos;

#ifdef _F01_INSTANCE_SUPPORT
  in vec4 cubeColor;
  in mat4 particleWorldMatrix;
#else
  in float parIdx;
  uniform mat4 cubeBatchTransformArray[32];
  uniform vec4 cubeBatchColorArray[32];
#endif

vec4 cubemitter_getWorldspacePos()
{

#ifdef _F01_INSTANCE_SUPPORT
  return particleWorldMatrix * vec4(vertPos, 1);
#else
  return cubeBatchTransformArray[int(parIdx)] * vec4(vertPos, 1);
#endif
}

vec4 cubemitter_getColor()
{
#ifdef _F01_INSTANCE_SUPPORT
  return cubeColor;
#else
  return cubeBatchColorArray[int(parIdx)];
#endif
}