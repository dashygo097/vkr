#pragma once

#include "vkr/exec/graph.hh"

namespace vkr::exec {

class ComputeGraph final : public Graph {
public:
  ComputeGraph() : Graph("Compute") {}
  ~ComputeGraph() override = default;

private:
  void validate() const override {}
};

} // namespace vkr::exec
