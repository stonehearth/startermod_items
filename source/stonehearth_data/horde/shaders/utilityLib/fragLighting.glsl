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
uniform vec4 lightPos;
uniform vec4 lightDir;
uniform vec3 lightColor;


vec3 calcPhongSpotLight( const vec3 viewerPos, const vec3 pos, const vec3 normal, const vec3 albedo, 
    const vec3 specColor, const float gloss, const float viewDist, const float ambientIntensity )
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
  
  return (albedo + specular) * lightColor * atten;// * shadowTerm;
}


vec3 calcPhongOmniLight(const vec3 viewerPos, const vec3 pos, const vec3 normal)
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

vec3 calcPhongDirectionalLight( const vec3 viewerPos, const vec3 pos, const vec3 normal, const vec3 albedo, 
    const vec3 specColor, const float gloss, const float viewDist, const float ambientIntensity )
{     
  // Lambert diffuse
  float atten = max( dot( normal, -lightDir.xyz ), 0.0 );

  // Blinn-Phong specular with energy conservation
  vec3 view = normalize( viewerPos - pos );
  vec3 halfVec = normalize( lightDir.xyz + view );
  float specExp = exp2( 10.0 * gloss + 1.0 );
  vec3 specular = specColor * pow( max( dot( halfVec, normal ), 0.0 ), specExp );
  specular *= (specExp * 0.125 + 0.25);  // Normalization factor (n+2)/8
  
  return (albedo + specular) * atten * lightColor;
}


vec3 calcSimpleDirectionalLight(const vec3 viewerPos, const vec3 pos, const vec3 normal, 
    const float viewDist) {
  // Lambert diffuse
  float atten = max( dot( normal, -lightDir.xyz ), 0.0 );

  // Blinn-Phong specular with energy conservation
  vec3 view = normalize( viewerPos - pos );
  vec3 halfVec = normalize( lightDir.xyz + view );

  return atten * lightColor;
}
