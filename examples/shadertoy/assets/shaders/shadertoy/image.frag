void mainImage(out vec4 color, in vec2 pixelCoord) {
  vec2 uv = pixelCoord / iResolution.xy;
  color = vec4(uv, 0.5 + 0.5 * sin(iTime), 1.0);
}
