[[FX]]

// Samplers

sampler2D depthBuf = sampler_state
{
	Address = Clamp;
};

sampler2D gbuf0 = sampler_state
{
	Address = Clamp;
};

sampler2D gbuf1 = sampler_state
{
	Address = Clamp;
};

sampler2D gbuf2 = sampler_state
{
	Address = Clamp;
};

sampler2D gbuf3 = sampler_state
{
	Address = Clamp;
};

sampler2D buf0 = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D buf1 = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D buf2 = sampler_state
{
  Address = Clamp;
  Filter = None;
};

sampler2D buf3 = sampler_state
{
  Address = Clamp;
  Filter = Trilinear;
};

float4 color = {0, 0, 0, 1};

// Uniforms
float hdrExposure = 2.0;       // Exposure (higher values make scene brighter)
float hdrBrightThres = 0.6;    // Brightpass threshold (intensity where blooming begins)
float hdrBrightOffset = 0.06;  // Brightpass offset (smaller values produce stronger blooming)

context OUTLINE
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_OUTLINE;

  ZWriteEnable = false;
}

context DRAW_SHADOWMAP
{
	VertexShader = compile GLSL VS_FSQUAD;
	PixelShader = compile GLSL FS_DRAW_SHADOWMAP;
	
	ZWriteEnable = false;
}

context DRAW_COLORBUF
{
	VertexShader = compile GLSL VS_FSQUAD;
	PixelShader = compile GLSL FS_DRAW_COLORBUF;
	
	ZWriteEnable = false;
}

context DRAW_NORMALS
{
	VertexShader = compile GLSL VS_FSQUAD;
	PixelShader = compile GLSL FS_DRAW_NORMALS;
	
	ZWriteEnable = false;
}

context FINALPASS
{
	VertexShader = compile GLSL VS_FSQUAD;
	PixelShader = compile GLSL FS_FINALPASS;
	
	ZWriteEnable = false;
}

context BLUEPRINT_FINALPASS
{
	VertexShader = compile GLSL VS_FSQUAD;
	PixelShader = compile GLSL FS_BLUEPRINT_FINALPASS;
	
	ZWriteEnable = false;
}

context FINALPASS_COPY_DEPTH
{
	VertexShader = compile GLSL VS_FSQUAD;
	PixelShader = compile GLSL FS_FINALPASS_COPY_DEPTH;
	
	ZWriteEnable = false;
}

context FXAA
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_FXAA;
}

context BLUEPRINT_FXAA
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_BLUEPRINT_FXAA;
}

[[FS_OUTLINE]]
uniform sampler2D depthBuf;
uniform vec3 viewerPos;
uniform mat4 viewMat;
uniform vec2 frameBufSize;

varying vec2 texCoords;

#include "shaders/utilityLib/fragDeferredWrite.glsl" 
#include "shaders/utilityLib/fragDeferredRead.glsl" 

float LinearizeDepth(float depth)
{
  float n = 4.0; // camera z near
  float f = 1000.0; // camera z far

  float distance = (2.0 * n) / (f + n - depth * (f - n));
  return distance;
}

vec4 GetNormalDepthSample(vec2 coord, vec2 offset)
{
   vec3 normal = getNormal(coord + offset);

   // 4.1.1 - Make sure to convert the depth to the linear distance
   // from the near plane rather than eye coordinates.

   float depth = texture2D(depthBuf, coord + offset * 2).r;
   depth = LinearizeDepth(depth);

   return vec4(normal, depth);   
}

float SampleDepth(vec2 coord, vec2 offset)
{
   float depth = texture2D(depthBuf, coord + offset).r;
   return LinearizeDepth(depth);
}

void main( void )
{
   if( getMatID( texCoords ) == 2.0 ) {
      // sky
      gl_FragColor = vec4(0.9, 0.9, 0.9, 1.0);
   } else {
      vec2 invScreenSize = vec2(1.0 / 1600.0, 1.0 / 1200.0);
      vec2 offset = vec2(1, 1) / (frameBufSize * 2);

      // 4.1.2 - Calcuate crease edges by comparing the angles between
      // normals of diagonal texels...

      vec3 A = getNormal(texCoords + vec2( offset.x,  offset.y));
      vec3 C = getNormal(texCoords + vec2(-offset.x,  offset.y));
      vec3 F = getNormal(texCoords + vec2( offset.x, -offset.y));
      vec3 H = getNormal(texCoords + vec2(-offset.x, -offset.y));
      float In = (dot(A, H) + dot(C, F)) / 2;

      // 4.1.2 - Calcuate siloette edges by comparing the discontinuties
      float scale = 2.0;
      float Iz = 8 * SampleDepth(texCoords, vec2(0, 0));
   
      Iz -= SampleDepth(texCoords, vec2( offset.x, -offset.y));
      Iz -= SampleDepth(texCoords, vec2(      0.0, -offset.y));
      Iz -= SampleDepth(texCoords, vec2(-offset.x, -offset.y));
   
      Iz -= SampleDepth(texCoords, vec2(-offset.x, 0.0));
      Iz -= SampleDepth(texCoords, vec2( offset.x, 0.0));

      Iz -= SampleDepth(texCoords, vec2(-offset.x,  offset.y));
      Iz -= SampleDepth(texCoords, vec2(      0.0,  offset.y));
      Iz -= SampleDepth(texCoords, vec2( offset.x,  offset.y));  

      Iz = 1 - clamp(-Iz * 400, 0, 1);

      float c = In * pow(Iz, 2);
      //Iz = SampleDepth(texCoords, vec2(0, 0)) * scale;
      //Iz = texture2D(depthBuf, texCoords).r;
      //gl_FragColor = vec4(Iz, Iz, 1, 1);

      gl_FragColor = vec4(c, c, c, 1);
   }
}

[[VS_FSQUAD]]

uniform mat4 projMat;
attribute vec3 vertPos;
varying vec2 texCoords;
				
void main( void )
{
	texCoords = vertPos.xy; 
	gl_Position = projMat * vec4( vertPos, 1 );
}


[[FS_FINALPASS]]
uniform sampler2D buf0, buf1, buf2, buf3;
uniform vec2 frameBufSize;
uniform float hdrExposure;
varying vec2 texCoords;
uniform sampler2D depthBuf;

float LinearizeDepth(float depth)
{
  float n = 4.0; // camera z near
  float f = 1000.0; // camera z far

  float distance = (2.0 * n) / (f + n - depth * (f - n));
  return distance;
}


void main( void )
{
	vec4 hdrColor = texture2D( buf0, texCoords );	// HDR color
	vec4 bloomColor = texture2D( buf1, texCoords );	// Bloom
   vec4 outlineColor = texture2D( buf2, texCoords ); // Outline
  
   vec4 ssaoColor = texture2D( buf3, texCoords); //SSAO
   // col = 1.0 - exp2( -hdrExposure * hdrColor );

   vec4 finalColor = ssaoColor*(hdrColor+bloomColor);
   // finalColor = hdrColor+bloomColor;

   // Thow in some depth fog for fun...
   // float depth = texture2D(depthBuf, texCoords).r;
   // depth = LinearizeDepth(depth);
   // depth = pow(depth, 3);
   // finalColor = mix(finalColor, vec4(1, 1, 1, 1), depth);

   gl_FragColor = finalColor;
	//gl_FragColor = ssaoColor*outlineColor*(hdrColor+bloomColor);
}

[[FS_BLUEPRINT_FINALPASS]]
uniform sampler2D buf0, buf1, buf2, buf3;
uniform vec2 frameBufSize;
uniform float hdrExposure;
varying vec2 texCoords;

void main( void )
{
	vec4 col0 = texture2D( buf0, texCoords );	// HDR color
   vec4 col2 = texture2D( buf2, texCoords ); // Outline
  
	gl_FragColor = col0 * col2;
}

[[FS_FINALPASS_COPY_DEPTH]]

uniform sampler2D depthBuf;
varying vec2 texCoords;

void main( void )
{
	// gl_FragDepth = texture2D( depthBuf, texCoords ).r;
   gl_FragColor.r = texture2D( depthBuf, texCoords ).r;
   gl_FragColor.g = texture2D( depthBuf, texCoords ).b;
   gl_FragColor.b = 1;
}


[[FS_BLUEPRINT_FXAA]]
uniform sampler2D buf0;
varying vec2 texCoords;

void main( void )
{
   vec3 rgb = texture2D(buf0, texCoords);
   gl_FragColor = vec4(rgb, 1);
}

[[FS_FXAA]]
//Thanks http://horde3d.org/wiki/index.php5?title=Shading_Technique_-_FXAA
uniform sampler2D buf0, buf1;
uniform vec2 frameBufSize;
uniform float hdrExposure;
varying vec2 texCoords;

void main( void )
{
	float FXAA_SPAN_MAX = 8.0;
	float FXAA_REDUCE_MUL = 1.0/8.0;
	float FXAA_REDUCE_MIN = 1.0/128.0;

	vec3 rgbNW=texture2D(buf0,texCoords+(vec2(-1.0,-1.0)/frameBufSize)).xyz;
	vec3 rgbNE=texture2D(buf0,texCoords+(vec2(1.0,-1.0)/frameBufSize)).xyz;
	vec3 rgbSW=texture2D(buf0,texCoords+(vec2(-1.0,1.0)/frameBufSize)).xyz;
	vec3 rgbSE=texture2D(buf0,texCoords+(vec2(1.0,1.0)/frameBufSize)).xyz;
	vec3 rgbM=texture2D(buf0,texCoords).xyz;
	
	vec3 luma=vec3(0.299, 0.587, 0.114);
	float lumaNW = dot(rgbNW, luma);
	float lumaNE = dot(rgbNE, luma);
	float lumaSW = dot(rgbSW, luma);
	float lumaSE = dot(rgbSE, luma);
	float lumaM  = dot(rgbM,  luma);
	
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	
	vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
	
	float dirReduce = max(
		(lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
		FXAA_REDUCE_MIN);
	  
	float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
	
	dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
		  max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
		  dir * rcpDirMin)) / frameBufSize;
		
	vec3 rgbA = (1.0/2.0) * (
		texture2D(buf0, texCoords.xy + dir * (1.0/3.0 - 0.5)).xyz +
		texture2D(buf0, texCoords.xy + dir * (2.0/3.0 - 0.5)).xyz);
	vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
		texture2D(buf0, texCoords.xy + dir * (0.0/3.0 - 0.5)).xyz +
		texture2D(buf0, texCoords.xy + dir * (3.0/3.0 - 0.5)).xyz);
	float lumaB = dot(rgbB, luma);
  
  vec4 outputCol;
	if((lumaB < lumaMin) || (lumaB > lumaMax)){
		outputCol.xyz=rgbA;
	}else{
		outputCol.xyz=rgbB;
	}
  //Gamma correct
  gl_FragColor = pow(outputCol, 1.0/1.2);
}

[[FS_DRAW_SHADOWMAP]]

uniform sampler2D buf0; // is actually the shadow map...
varying vec2 texCoords;

float LinearizeDepth(float depth)
{
  float n = 4.0; // camera z near
  float f = 1000.0; // camera z far

  float distance = (2.0 * n) / (f + n - depth * (f - n));
  return distance;
}

void main( void )
{
   float depth = LinearizeDepth(texture2D(buf0, texCoords).r);
   gl_FragColor.rgb = vec3(depth, depth, depth);
}

[[FS_DRAW_COLORBUF]]

uniform sampler2D buf0;
varying vec2 texCoords;

void main( void )
{
   gl_FragColor.rgb = texture2D(buf0, texCoords);
}

[[FS_DRAW_NORMALS]]

#include "shaders/utilityLib/fragDeferredRead.glsl"

uniform sampler2D buf0;
varying vec2 texCoords;

void main( void )
{
   vec3 normal = getNormal(texCoords);
   gl_FragColor.rgb = normal;
}
