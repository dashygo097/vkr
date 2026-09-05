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

static const uint LOCAL_SIZE = 16;

groupshared float tileA[LOCAL_SIZE][LOCAL_SIZE];
groupshared float tileB[LOCAL_SIZE][LOCAL_SIZE];

[shader("compute")]
[numthreads(LOCAL_SIZE, LOCAL_SIZE, 1)]
void main(uint3 dispatchThreadId: SV_DispatchThreadID,
          uint3 groupThreadId: SV_GroupThreadID) {
  const uint row = dispatchThreadId.x;
  const uint col = dispatchThreadId.y;
  const uint localRow = groupThreadId.x;
  const uint localCol = groupThreadId.y;

  float sum = 0.0f;
  const uint tileCount = (params.K + LOCAL_SIZE - 1) / LOCAL_SIZE;

  for (uint tile = 0; tile < tileCount; ++tile) {
    const uint aCol = tile * LOCAL_SIZE + localCol;
    const uint bRow = tile * LOCAL_SIZE + localRow;

    tileA[localRow][localCol] =
        (row < params.M && aCol < params.K) ? A[row * params.K + aCol] : 0.0f;
    tileB[localRow][localCol] =
        (bRow < params.K && col < params.N) ? B[bRow * params.N + col] : 0.0f;

    GroupMemoryBarrierWithGroupSync();

    for (uint k = 0; k < LOCAL_SIZE; ++k) {
      sum += tileA[localRow][k] * tileB[k][localCol];
    }

    GroupMemoryBarrierWithGroupSync();
  }

  if (row < params.M && col < params.N) {
    C[row * params.N + col] = sum;
  }
}
