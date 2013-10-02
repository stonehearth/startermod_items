#version 150
// *************************************************************************************************
// Horde3D Shader Utility Library
// --------------------------------------
//    - Lighting functions -
//
// Copyright (C) 2006-2011 Nicolas Schulz
//
// You may use the following code in projects based on the Horde3D graphics engine.
//
// *************************************************************************************************
uniform   vec3 viewerPos;
uniform   vec4 lightPos;
uniform   vec4 lightDir;
uniform   vec3 lightColor;
uniform   sampler2DShadow shadowMap;
uniform   vec4 shadowSplitDists;
uniform   mat4 shadowMats[4];
uniform   float shadowMapSize;


float PCF( vec4 projShadow ) {
  // 5-tap PCF with a 30� rotated grid
  
  float offset = 1.0 / shadowMapSize;
  
  float shadowTerm = textureProj(shadowMap, projShadow);
  shadowTerm += textureProj(shadowMap, projShadow + vec4(-0.866 * offset, 0.5 * offset, 0.0, 0.0));
  shadowTerm += textureProj(shadowMap, projShadow + vec4(-0.866 * offset, -0.5 * offset, 0.0, 0.0));
  shadowTerm += textureProj(shadowMap, projShadow + vec4(0.866 * offset, -0.5 * offset, 0.0, 0.0));
  shadowTerm += textureProj(shadowMap, projShadow + vec4(0.866 * offset, 0.5 * offset, 0.0, 0.0));
  
  return shadowTerm / 5.0;
}

float simpleShadow(vec3 pos, float atten, float viewDist) {
  float shadowTerm = 1.0;
  
  // Skip shadow mapping if default shadow map (size==4) is bound
  if( atten * (shadowMapSize - 4.0) > 0.0) {
    vec4 projShadow;
    if(viewDist < shadowSplitDists.x) {
     projShadow = shadowMats[0] * vec4( pos, 1.0);
    } else if(viewDist < shadowSplitDists.y) {
      projShadow = shadowMats[1] * vec4(pos, 1.0);
    } else if(viewDist < shadowSplitDists.z ) {
      projShadow = shadowMats[2] * vec4(pos, 1.0);
    } else {
      projShadow = shadowMats[3] * vec4(pos, 1.0);
    }
    shadowTerm = textureProj(shadowMap, projShadow);
  }
  return shadowTerm;
}


vec3 calcPhongSpotLight( const vec3 pos, const vec3 normal, const vec3 albedo, const vec3 specColor,
                   const float gloss, const float viewDist, const float ambientIntensity )
{
  vec3 light = lightPos.xyz - pos;
  float lightLen = length( light );
  light /= lightLen;
  
  // Distance attenuation
  float lightDepth = lightLen / lightPos.w;
  float atten = max( 1.0 - lightDepth * lightDepth, 0.0 );
  
  // Spotlight falloff
  float angle = dot( lightDir.xyz, -light );
  atten *= clamp( (angle - lightDir.w) / 0.2, 0.0, 1.0 );
    
  // Lambert diffuse
  float NdotL = max( dot( normal, light ), 0.0 );
  atten *= NdotL;
    
  // Blinn-Phong specular with energy conservation
  vec3 view = normalize( viewerPos - pos );
  vec3 halfVec = normalize( light + view );
  float specExp = exp2( 10.0 * gloss + 1.0 );
  vec3 specular = specColor * pow( max( dot( halfVec, normal ), 0.0 ), specExp );
  specular *= (specExp * 0.125 + 0.25);  // Normalization factor (n+2)/8
  
  float shadowTerm = simpleShadow(pos, atten, viewDist);
  
  return (albedo + specular) * lightColor * atten * shadowTerm;
}


vec3 calcPhongOmniLight(const vec3 pos, const vec3 normal)
{
	vec3 light = lightPos.xyz - pos;
	float lightLen = length( light );
	light /= lightLen;
	
	// Distance attenuation
	float lightDepth = lightLen / lightPos.w;
	float atten = max( 1.0 - lightDepth * lightDepth, 0.0 );
	
	// Lambert diffuse
	float NdotL = max( dot( normal, light ), 0.0 );
	atten *= NdotL;
		
	// Blinn-Phong specular with energy conservation
	vec3 view = normalize( viewerPos - pos );
	vec3 halfVec = normalize( light + view );
	
	return lightColor * atten;
}

vec3 calcPhongDirectionalLight( const vec3 pos, const vec3 normal, const vec3 albedo, const vec3 specColor,
                          const float gloss, const float viewDist, const float ambientIntensity )
{     
  // Lambert diffuse
  float atten = max( dot( normal, -lightDir.xyz ), 0.0 );

  // Blinn-Phong specular with energy conservation
  vec3 view = normalize( viewerPos - pos );
  vec3 halfVec = normalize( lightDir.xyz + view );
  float specExp = exp2( 10.0 * gloss + 1.0 );
  vec3 specular = specColor * pow( max( dot( halfVec, normal ), 0.0 ), specExp );
  specular *= (specExp * 0.125 + 0.25);  // Normalization factor (n+2)/8
  
  float shadowTerm = simpleShadow(pos, atten, viewDist);

  return (albedo + specular) * atten * lightColor * shadowTerm;
}


vec3 calcSimpleDirectionalLight(const vec3 pos, const vec3 normal, const float viewDist) {
  // Lambert diffuse
  float atten = max( dot( normal, -lightDir.xyz ), 0.0 );

  // Blinn-Phong specular with energy conservation
  vec3 view = normalize( viewerPos - pos );
  vec3 halfVec = normalize( lightDir.xyz + view );

  // Shadows
  float shadowTerm = simpleShadow(pos, atten, viewDist);

  return atten * lightColor * shadowTerm;
}
