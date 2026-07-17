void mainImage(out vec4 color, in vec2 pixelCoord) {
  vec2 uv = pixelCoord / iResolution.xy;
  vec3 a = texture(iChannel0, uv).rgb;
  vec3 b = texture(iChannel1, uv).rgb;
  vec3 c = texture(iChannel2, uv).rgb;
  vec3 previous = texture(iChannel3, uv).rgb;
  color = vec4(mix(previous, vec3(a.r, b.g, c.b), 0.1), 1.0);
}
