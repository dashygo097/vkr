#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>
#include <vkr.hh>

using namespace vkr::util;
using namespace vkr::core;
using namespace vkr::exec;
using namespace vkr::res;

auto main() -> int {
  vkr::Logger::init();

  // instance
  InstanceDesc instDesc{};
  instDesc.name = "softmax";
  instDesc.version = VK_MAKE_VERSION(1, 0, 0);
  instDesc.surfaceIntegration = SurfaceIntegration::None;

  Instance instance{instDesc};

  // device
  DeviceDesc deviceDesc{};

  Device device{instance, deviceDesc};

  // command pool
  CommandPoolDesc commandPoolDesc{};
  commandPoolDesc.queueRole = CommandQueueRole::Compute;

  CommandPool commandPool{device, commandPoolDesc};

  // buffers
  constexpr uint32_t ElementCount = 4096;

  std::vector<float> in{};
  std::vector<float> out{};
  auto inBuffer = StorageBuffer<float>{device};
  auto outBuffer = StorageBuffer<float>{device};

  auto inDesc = StorageBufferDesc{};
  inDesc.reserve(ElementCount)
      .readonly()
      .storage()
      .transfer()
      .hostVisible()
      .mapped();

  auto outDesc = StorageBufferDesc{};
  outDesc.reserve(ElementCount)
      .writeonly()
      .storage()
      .transfer()
      .hostVisible()
      .mapped();

  in.resize(ElementCount);
  out.assign(ElementCount, 0.0f);

  for (uint32_t i = 0; i < ElementCount; ++i) {
    in[i] = static_cast<float>(i);
  }

  inBuffer.update(inDesc);
  outBuffer.update(outDesc);

  inBuffer.write(in);

  // executor
  ComputeExecutor executor{device, commandPool};

  // pass
  const auto shaders = executableDir() / "assets" / "shaders";
  ComputePassDesc passDesc{};
  passDesc.storage(0, 0)
      .storage(0, 1)
      .shader("softmax",
#ifdef VKR_HAS_SLANG
              ShaderModuleDesc::computeSlangFile(shaders / "softmax.slang"))
#else
              ShaderModuleDesc::computeGlslFile(shaders / "softmax.comp"))
#endif
      .dispatchX(1);

  ComputePass pass{executor, device};
  pass.name("softmax");
  pass.update(passDesc);
  pass.storage(0, 0, inBuffer).storage(0, 1, outBuffer);
  pass.create();

  // execution
  executor.begin();
  pass.record();
  executor.hostRead(outBuffer);
  executor.submitAndWait();
  executor.end();

  // after execution
  outBuffer.read(out);

  const double maximum = *std::max_element(in.begin(), in.end());
  std::vector<double> reference(ElementCount);
  for (uint32_t i = 0; i < ElementCount; ++i) {
    reference[i] = std::exp(static_cast<double>(in[i]) - maximum);
  }
  const double denominator =
      std::accumulate(reference.begin(), reference.end(), 0.0);

  double maxError = 0.0;
  for (uint32_t i = 0; i < ElementCount; ++i) {
    reference[i] /= denominator;
    const double error = std::abs(out[i] - reference[i]);
    if (!std::isfinite(out[i]) || out[i] < 0.0f || out[i] > 1.0f ||
        error > 1.0e-6 + 1.0e-4 * reference[i]) {
      std::cerr << "softmax validation failed at index " << i << ": expected "
                << reference[i] << ", got " << out[i] << '\n';
      return 1;
    }
    maxError = std::max(maxError, error);
  }

  const double sum = std::accumulate(out.begin(), out.end(), 0.0);
  if (std::abs(sum - 1.0) > 1.0e-5) {
    std::cerr << "softmax validation failed: output sum = " << sum << '\n';
    return 1;
  }

  std::cout << std::setprecision(7) << "softmax passed: " << ElementCount
            << " elements, sum = " << sum << ", max error = " << maxError
            << '\n';
  for (uint32_t i = ElementCount - 16; i < ElementCount; ++i) {
    std::cout << "softmax[" << i << "] = " << out[i] << '\n';
  }

  return 0;
}
