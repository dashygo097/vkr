#pragma once

#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace vkr {
class Logger {
public:
  static void init();

  static std::shared_ptr<spdlog::logger> &getCoreLogger() {
    return core_logger_;
  }

private:
  static std::shared_ptr<spdlog::logger> core_logger_;
};
} // namespace vkr

// Logging Macros
#define VKR_CORE_TRACE(...) ::vkr::Logger::getCoreLogger()->trace(__VA_ARGS__)
#define VKR_CORE_INFO(...) ::vkr::Logger::getCoreLogger()->info(__VA_ARGS__)
#define VKR_CORE_WARN(...) ::vkr::Logger::getCoreLogger()->warn(__VA_ARGS__)
#define VKR_CORE_CRIT(...) ::vkr::Logger::getCoreLogger()->critical(__VA_ARGS__)
#define VKR_CORE_ERROR(...) ::vkr::Logger::getCoreLogger()->error(__VA_ARGS__)
