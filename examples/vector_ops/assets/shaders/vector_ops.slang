struct VectorOpsParams {
  uint elementCount;
  uint iterations;
};

[[vk::binding(0, 0)]]
StructuredBuffer<float> inputA;

[[vk::binding(1, 0)]]
StructuredBuffer<float> inputB;

[[vk::binding(2, 0)]]
RWStructuredBuffer<float> outputC;

[[vk::binding(3, 0)]]
ConstantBuffer<VectorOpsParams> params;

float nonlinearOp(float a, float b) {
  float x = a * 0.000123 + b * 0.000071;
  float y = b * 0.000097 + 0.25;

  for (uint i = 0; i < params.iterations; ++i) {
    x = sin(x) * 0.73 + cos(y) * 0.19 + sqrt(abs(x * y) + 0.001);
    y = sin(y + x * 0.13) * 0.61 + cos(x - y * 0.07) * 0.29;
    x = clamp(x, -8.0, 8.0);
    y = clamp(y, -8.0, 8.0);
  }

  return x + y * 0.5;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId: SV_DispatchThreadID) {
  const uint index = dispatchThreadId.x;
  if (index >= params.elementCount) {
    return;
  }

  outputC[index] = nonlinearOp(inputA[index], inputB[index]);
}
