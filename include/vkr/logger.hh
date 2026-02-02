#pragma once

#include <memory>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace vkr {

class Formatter : public spdlog::custom_flag_formatter {
public:
  void format(const spdlog::details::log_msg &msg, const std::tm &,
              spdlog::memory_buf_t &dest) override {
    std::string_view level_name;
    switch (msg.level) {
    case spdlog::level::trace:
      level_name = " TRACE";
      break;
    case spdlog::level::debug:
      level_name = " DEBUG";
      break;
    case spdlog::level::info:
      level_name = " INFO ";
      break;
    case spdlog::level::warn:
      level_name = " WARN ";
      break;
    case spdlog::level::err:
      level_name = " ERROR";
      break;
    case spdlog::level::critical:
      level_name = " CRIT ";
      break;
    default:
      level_name = " UNKN ";
      break;
    }
    dest.append(level_name.data(), level_name.data() + level_name.size());
  }

  std::unique_ptr<custom_flag_formatter> clone() const override {
    return std::make_unique<Formatter>();
  }
};

class Logger {
public:
  static void init();

  static std::shared_ptr<spdlog::logger> &getCoreLogger() {
    return core_logger_;
  }
  static std::shared_ptr<spdlog::logger> &getResourceLogger() {
    return resource_logger_;
  }
  static std::shared_ptr<spdlog::logger> &getPipelineLogger() {
    return pipeline_logger_;
  }
  static std::shared_ptr<spdlog::logger> &getRenderLogger() {
    return render_logger_;
  }
  static std::shared_ptr<spdlog::logger> &getSceneLogger() {
    return scene_logger_;
  }
  static std::shared_ptr<spdlog::logger> &getUiLogger() { return ui_logger_; }

private:
  static std::shared_ptr<spdlog::logger> core_logger_;
  static std::shared_ptr<spdlog::logger> resource_logger_;
  static std::shared_ptr<spdlog::logger> pipeline_logger_;
  static std::shared_ptr<spdlog::logger> render_logger_;
  static std::shared_ptr<spdlog::logger> scene_logger_;
  static std::shared_ptr<spdlog::logger> ui_logger_;
};
} // namespace vkr

// Logging Macros
#define VKR_CORE_TRACE(...) ::vkr::Logger::getCoreLogger()->trace(__VA_ARGS__);
#define VKR_CORE_DEBUG(...) ::vkr::Logger::getCoreLogger()->debug(__VA_ARGS__);
#define VKR_CORE_INFO(...) ::vkr::Logger::getCoreLogger()->info(__VA_ARGS__);
#define VKR_CORE_WARN(...) ::vkr::Logger::getCoreLogger()->warn(__VA_ARGS__);
#define VKR_CORE_CRIT(...)                                                     \
  ::vkr::Logger::getCoreLogger()->critical(__VA_ARGS__);
#define VKR_CORE_ERROR(...)                                                    \
  do {                                                                         \
    ::vkr::Logger::getCoreLogger()->error(__VA_ARGS__);                        \
    std::abort();                                                              \
  } while (0);

#define VKR_RES_TRACE(...)                                                     \
  ::vkr::Logger::getResourceLogger()->trace(__VA_ARGS__);
#define VKR_RES_DEBUG(...)                                                     \
  ::vkr::Logger::getResourceLogger()->debug(__VA_ARGS__);
#define VKR_RES_INFO(...) ::vkr::Logger::getResourceLogger()->info(__VA_ARGS__);
#define VKR_RES_WARN(...) ::vkr::Logger::getResourceLogger()->warn(__VA_ARGS__);
#define VKR_RES_CRIT(...)                                                      \
  ::vkr::Logger::getResourceLogger()->critical(__VA_ARGS__);
#define VKR_RES_ERROR(...)                                                     \
  do {                                                                         \
    ::vkr::Logger::getResourceLogger()->error(__VA_ARGS__);                    \
    std::abort();                                                              \
  } while (0);

#define VKR_PIPE_TRACE(...)                                                    \
  ::vkr::Logger::getPipelineLogger()->trace(__VA_ARGS__);
#define VKR_PIPE_DEBUG(...)                                                    \
  ::vkr::Logger::getPipelineLogger()->debug(__VA_ARGS__);
#define VKR_PIPE_INFO(...)                                                     \
  ::vkr::Logger::getPipelineLogger()->info(__VA_ARGS__);
#define VKR_PIPE_WARN(...)                                                     \
  ::vkr::Logger::getPipelineLogger()->warn(__VA_ARGS__);
#define VKR_PIPE_CRIT(...)                                                     \
  ::vkr::Logger::getPipelineLogger()->critical(__VA_ARGS__);
#define VKR_PIPE_ERROR(...)                                                    \
  do {                                                                         \
    ::vkr::Logger::getPipelineLogger()->error(__VA_ARGS__);                    \
    std::abort();                                                              \
  } while (0);

#define VKR_RENDER_TRACE(...)                                                  \
  ::vkr::Logger::getRenderLogger()->trace(__VA_ARGS__);
#define VKR_RENDER_DEBUG(...)                                                  \
  ::vkr::Logger::getRenderLogger()->debug(__VA_ARGS__);
#define VKR_RENDER_INFO(...)                                                   \
  ::vkr::Logger::getRenderLogger()->info(__VA_ARGS__);
#define VKR_RENDER_WARN(...)                                                   \
  ::vkr::Logger::getRenderLogger()->warn(__VA_ARGS__);
#define VKR_RENDER_CRIT(...)                                                   \
  ::vkr::Logger::getRenderLogger()->critical(__VA_ARGS__);
#define VKR_RENDER_ERROR(...)                                                  \
  do {                                                                         \
    ::vkr::Logger::getRenderLogger()->error(__VA_ARGS__);                      \
    std::abort();                                                              \
  } while (0);

#define VKR_SCENE_TRACE(...)                                                   \
  ::vkr::Logger::getSceneLogger()->trace(__VA_ARGS__);
#define VKR_SCENE_DEBUG(...)                                                   \
  ::vkr::Logger::getSceneLogger()->debug(__VA_ARGS__);
#define VKR_SCENE_INFO(...) ::vkr::Logger::getSceneLogger()->info(__VA_ARGS__);
#define VKR_SCENE_WARN(...) ::vkr::Logger::getSceneLogger()->warn(__VA_ARGS__);
#define VKR_SCENE_CRIT(...)                                                    \
  ::vkr::Logger::getSceneLogger()->critical(__VA_ARGS__);
#define VKR_SCENE_ERROR(...)                                                   \
  do {                                                                         \
    ::vkr::Logger::getSceneLogger()->error(__VA_ARGS__);                       \
    std::abort();                                                              \
  } while (0);

#define VKR_UI_TRACE(...) ::vkr::Logger::getUiLogger()->trace(__VA_ARGS__);
#define VKR_UI_DEBUG(...) ::vkr::Logger::getUiLogger()->debug(__VA_ARGS__);
#define VKR_UI_INFO(...) ::vkr::Logger::getUiLogger()->info(__VA_ARGS__);
#define VKR_UI_WARN(...) ::vkr::Logger::getUiLogger()->warn(__VA_ARGS__);
#define VKR_UI_CRIT(...) ::vkr::Logger::getUiLogger()->critical(__VA_ARGS__);
#define VKR_UI_ERROR(...)                                                      \
  do {                                                                         \
    ::vkr::Logger::getUiLogger()->error(__VA_ARGS__);                          \
    std::abort();                                                              \
  } while (0);
