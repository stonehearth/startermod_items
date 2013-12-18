float calcFogFac(const float depth) {
  return clamp(exp(-depth / 1000.0) - 1.7, 0.0, 1.0);
}