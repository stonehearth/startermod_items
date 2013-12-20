
uniform mat4 shadowMats[4];
uniform sampler2DShadow shadowMap;
uniform vec4 shadowSplitDists;
uniform float shadowMapSize;

uniform mat4 camViewMat;

varying vec4 projShadowPos[3];
///////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

// Unused, for now.
int _selectShadowCascade(const vec3 worldSpace_fragmentPos, out vec4 cascadeTexCoord)
{
  int cascadeFound = 0;
  int cascadeNum = 3;
  for (int i = 0; i < 4 /*cascade count*/ && cascadeFound == 0; i++) {
    if (min(cascadeTexCoord.x, cascadeTexCoord.y) > 0.0 &&
      (max(cascadeTexCoord.x, cascadeTexCoord.y) < 1.0)) {
        cascadeNum = i;
        cascadeFound = 1;
    }
  }
  
  return cascadeNum;
}

vec4 _shadowCoordsByMap(const vec3 worldSpace_fragmentPos) {
  vec2 cascadeTexCoord;
  vec4 hWorldSpace_fragmentPos = vec4(worldSpace_fragmentPos, 1.0);

  cascadeTexCoord = projShadowPos[0].xy;
  if (max(abs(cascadeTexCoord.x - 0.25), abs(cascadeTexCoord.y - 0.25)) <= 0.249) {
    return shadowMats[0] * hWorldSpace_fragmentPos;
  }

  cascadeTexCoord = projShadowPos[1].xy;
  if (max(abs(cascadeTexCoord.x - 0.75), abs(cascadeTexCoord.y - 0.25)) <= 0.249) {
    return shadowMats[1] * hWorldSpace_fragmentPos;
  }

  cascadeTexCoord = projShadowPos[2].xy;
  if (max(abs(cascadeTexCoord.x - 0.75), abs(cascadeTexCoord.y - 0.75)) <= 0.249) {
    return shadowMats[2] * hWorldSpace_fragmentPos;
  }

  return shadowMats[3] * hWorldSpace_fragmentPos;
}

vec4 _shadowCoordsByDistance(const vec3 worldSpace_fragmentPos, out int cascadeNum) {
  float viewDist = -(camViewMat * vec4(worldSpace_fragmentPos, 1)).z;
  vec4 projShadow;

  if(viewDist < shadowSplitDists.x) {
    cascadeNum = 0;
    projShadow = shadowMats[0] * vec4(worldSpace_fragmentPos, 1.0);
  } else if(viewDist < shadowSplitDists.y) {
    cascadeNum = 1;
    projShadow = shadowMats[1] * vec4(worldSpace_fragmentPos, 1.0);
  } else if(viewDist < shadowSplitDists.z ) {
    cascadeNum = 2;
    projShadow = shadowMats[2] * vec4(worldSpace_fragmentPos, 1.0);
  } else {
    cascadeNum = 3;
    projShadow = shadowMats[3] * vec4(worldSpace_fragmentPos, 1.0);
  }
  return projShadow;
}

float _PCF( vec4 projShadow ) {
  // 5-tap PCF with a 30° rotated grid
  
  float offset = 1.0 / shadowMapSize;
  
  float shadowTerm = shadow2DProj(shadowMap, projShadow).r;
  shadowTerm += shadow2DProj(shadowMap, projShadow + vec4(-0.866 * offset, 0.5 * offset, 0.0, 0.0)).r;
  shadowTerm += shadow2DProj(shadowMap, projShadow + vec4(-0.866 * offset, -0.5 * offset, 0.0, 0.0)).r;
  shadowTerm += shadow2DProj(shadowMap, projShadow + vec4(0.866 * offset, -0.5 * offset, 0.0, 0.0)).r;
  shadowTerm += shadow2DProj(shadowMap, projShadow + vec4(0.866 * offset, 0.5 * offset, 0.0, 0.0)).r;
  
  return shadowTerm / 5.0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Public Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

float getShadowValue(const vec3 worldSpace_fragmentPos)
{
  //vec4 projCoords = _shadowCoordsByDistance(worldSpace_fragmentPos, cascadeNum);
  vec4 projCoords = _shadowCoordsByMap(worldSpace_fragmentPos);

  float shadowTerm = shadow2DProj(shadowMap, projCoords).r;

  return shadowTerm;
}

vec3 getCascadeColor(const vec3 worldSpace_fragmentPos) {
  int cascadeNum;
  //_shadowCoordsByDistance(worldSpace_fragmentPos, cascadeNum);
  _shadowCoordsByMap(worldSpace_fragmentPos);

  if (cascadeNum == 0) {
    return vec3(1, 0, 0);
  } else if (cascadeNum == 1) {
    return vec3(0, 1, 0);
  } else if (cascadeNum == 2) {
    return vec3(0, 0, 1);
  }

  return vec3(1, 1, 0);
}
