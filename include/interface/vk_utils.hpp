#pragma once
#include <vector>

std::vector<const char *>
getRequiredExtensions(std::vector<const char *> preExtensions);
bool checkValidationLayerSupport(std::vector<const char *> validationLayers);
