void mainImage(out vec4 color, in vec2 pixelCoord) {
  vec2 uv = pixelCoord / iResolution.xy;
  color = texture(iChannel0, uv);
}
