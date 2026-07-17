void mainImage(out vec4 color, in vec2 pixelCoord) {
  vec2 uv = pixelCoord / iResolution.xy;
  vec3 a = texture(iChannel0, uv).rgb;
  vec3 previous = texture(iChannel1, uv).rgb;
  color = vec4(mix(previous, a.brg, 0.12), 1.0);
}
