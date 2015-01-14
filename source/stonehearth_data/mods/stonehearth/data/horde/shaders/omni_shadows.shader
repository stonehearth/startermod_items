
uniform samplerCube shadowMap;

float getOmniShadowValue(const vec3 lightPos, const vec3 worldSpace_fragmentPos)
{
  float shadowTerm;

#ifdef DISABLE_SHADOWS
  shadowTerm = 1.0;
#else
  vec3 lightDir = worldSpace_fragmentPos - lightPos;
  float lightDist = length(lightDir);
  
  // This really seems to suggest something very slightly, subtly, horribly wrong....
  lightDir.xy *= -1.0;

  float dist = textureCube(shadowMap, lightDir).r;
  // 0.3 is a nice term to simulate some light bouncing....
  shadowTerm = (dist >= lightDist - 0.1 ) ? 1.0 : 0.3;
#endif

  return shadowTerm;
}

