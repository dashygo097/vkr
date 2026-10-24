static const uint LocalSize = 64;

[[vk::binding(0, 0)]]
StructuredBuffer<float> inputValues;

[[vk::binding(1, 0)]]
RWStructuredBuffer<float> outputValues;

groupshared float partial[LocalSize];

// A single workgroup computes softmax over the entire input vector.
[shader("compute")]
[numthreads(LocalSize, 1, 1)]
void main(uint3 groupThreadId: SV_GroupThreadID) {
  const uint lane = groupThreadId.x;
  uint elementCount;
  uint elementStride;
  inputValues.GetDimensions(elementCount, elementStride);

  float localMax = -3.402823466e+38;
  for (uint i = lane; i < elementCount; i += LocalSize) {
    localMax = max(localMax, inputValues[i]);
  }
  partial[lane] = localMax;
  GroupMemoryBarrierWithGroupSync();

  for (uint stride = LocalSize / 2; stride > 0; stride >>= 1) {
    if (lane < stride) {
      partial[lane] = max(partial[lane], partial[lane + stride]);
    }
    GroupMemoryBarrierWithGroupSync();
  }

  const float maximum = partial[0];
  // Every lane must read the maximum before the shared scratch is reused.
  GroupMemoryBarrierWithGroupSync();

  float localSum = 0.0;
  for (uint i = lane; i < elementCount; i += LocalSize) {
    localSum += exp(inputValues[i] - maximum);
  }
  partial[lane] = localSum;
  GroupMemoryBarrierWithGroupSync();

  for (uint stride = LocalSize / 2; stride > 0; stride >>= 1) {
    if (lane < stride) {
      partial[lane] += partial[lane + stride];
    }
    GroupMemoryBarrierWithGroupSync();
  }

  const float denominator = partial[0];
  for (uint i = lane; i < elementCount; i += LocalSize) {
    outputValues[i] = exp(inputValues[i] - maximum) / denominator;
  }
}
