vec3 toCameraSpace(const mat4 camProjMat, const vec2 fragCoord, float depth)
{
  vec3 result;
  vec4 projInfo = vec4(
    -2.0 / (camProjMat[0][0]),
    -2.0 / (camProjMat[1][1]),
    (1.0 - camProjMat[0][2]) / camProjMat[0][0],
    (1.0 + camProjMat[1][2]) / camProjMat[1][1]);

  result.z = depth;
  result.xy = vec2((fragCoord.xy * projInfo.xy + projInfo.zw) * result.z);

  return result;
}


vec3 toWorldSpace(const mat4 camProjMat, const mat4 camRotInv, const vec2 fragCoord, float linear_depth)
{
  vec3 viewPos = toCameraSpace(camProjMat, fragCoord, linear_depth);  
  return -(camRotInv * vec4(viewPos, -1.0)).xyz;
}