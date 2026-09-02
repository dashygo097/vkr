struct MMulParams {
  uint M;
  uint K;
  uint N;
};

[[vk::binding(0, 0)]]
StructuredBuffer<float> A;

[[vk::binding(1, 0)]]
StructuredBuffer<float> B;

[[vk::binding(2, 0)]]
RWStructuredBuffer<float> C;

[[vk::binding(3, 0)]]
ConstantBuffer<MMulParams> params;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadId: SV_DispatchThreadID) {
  const uint idx = dispatchThreadId.x;
  const uint idy = dispatchThreadId.y;

  if (idx < params.M && idy < params.N) {
    float sum = 0.0f;
    for (uint k = 0; k < params.K; k++) {
      sum += A[idx * params.K + k] * B[k * params.N + idy];
    }
    C[idx * params.N + idy] = sum;
  }
}
