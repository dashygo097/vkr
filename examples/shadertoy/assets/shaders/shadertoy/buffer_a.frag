void mainImage(out vec4 color, in vec2 pixelCoord) {
  vec2 uv = pixelCoord / iResolution.xy;
  vec3 previous = texture(iChannel0, uv).rgb;
  vec3 pulse = 0.5 + 0.5 * cos(iTime + uv.xyx * 6.28318 + vec3(0.0, 2.0, 4.0));
  color = vec4(mix(previous, pulse, 0.08), 1.0);
}
