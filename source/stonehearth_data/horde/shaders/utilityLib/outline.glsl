uniform vec2 frameBufSize;

/**
 * Computes the outline color for the given texture coordinates and texture.  The texture should
 * be non-zero red in the places where the mesh exists.
 */
vec4 compute_outline_color(sampler2D outlineSampler, const vec2 texCoords) {
	vec2 offset = 1.0 / frameBufSize;

  vec3 centerColor = texture2D(outlineSampler, texCoords).xyz;

  vec3 color1 = texture2D(outlineSampler, texCoords + vec2(offset.x * 2, offset.y * 2)).xyz;
  vec3 color2 = texture2D(outlineSampler, texCoords + vec2(-offset.x * 2, offset.y * 2)).xyz;
  vec3 color3 = texture2D(outlineSampler, texCoords + vec2(offset.x * 2, -offset.y * 2)).xyz;
  vec3 color4 = texture2D(outlineSampler, texCoords + vec2(-offset.x * 2, -offset.y * 2)).xyz;

  vec3 finalColor = color1 + color2 + color3 + color4;

  float finalAlpha = finalColor.x > 0 ? 1.0 : 0.0;
  return vec4(finalColor, finalAlpha);
}