void mainImage(out vec4 color, in vec2 pixelCoord) {
  vec2 uv = pixelCoord / iResolution.xy;
  vec3 a = texture(iChannel0, uv).rgb;
  vec3 b = texture(iChannel1, uv).rgb;
  vec3 previous = texture(iChannel2, uv).rgb;
  color = vec4(mix(previous, 0.5 * (a + b), 0.1), 1.0);
}
