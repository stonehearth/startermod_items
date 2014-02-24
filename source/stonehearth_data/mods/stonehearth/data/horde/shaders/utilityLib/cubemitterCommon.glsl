// *************************************************************************************************
// Common functions for cubemitters; this hides the differing APIs needed for batching/instancing.
// *************************************************************************************************
attribute vec3 vertPos;

#ifdef INSTANCE_SUPPORT
  attribute vec4 cubeColor;
  attribute mat4 particleWorldMatrix;
#else
  attribute float parIdx;
  uniform mat4 cubeBatchTransformArray[32];
  uniform vec4 cubeBatchColorArray[32];
#endif

vec4 cubemitter_getWorldspacePos()
{

#ifdef INSTANCE_SUPPORT
  return particleWorldMatrix * vec4(vertPos, 1.0);
#else
  return cubeBatchTransformArray[int(parIdx)] * vec4(vertPos, 1);
#endif
}

vec4 cubemitter_getColor()
{
#ifdef INSTANCE_SUPPORT
  return cubeColor;
#else
  return cubeBatchColorArray[int(parIdx)];
#endif
}