#include "vkr/logger.hh"
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>

namespace vkr {

std::shared_ptr<spdlog::logger> Logger::core_logger_;
std::shared_ptr<spdlog::logger> Logger::resource_logger_;

void Logger::init() {
  std::vector<spdlog::sink_ptr> sinks;
  auto formatter = std::make_unique<spdlog::pattern_formatter>();

  formatter->add_flag<Formatter>('*');
  formatter->set_pattern("%^%*%$ \033[2mlibvkr::%n\033[0m %v");

  sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  sinks.push_back(
      std::make_shared<spdlog::sinks::basic_file_sink_mt>("vkr.log", true));

  for (auto &sink : sinks) {
  }

  sinks[0]->set_formatter(std::move(formatter));
  sinks[1]->set_pattern("[%T] [%l] %n: %v");

  // Initialize individual loggers
  core_logger_ =
      std::make_shared<spdlog::logger>("core", sinks.begin(), sinks.end());
  resource_logger_ =
      std::make_shared<spdlog::logger>("resource", sinks.begin(), sinks.end());

  // Set levels
  core_logger_->set_level(spdlog::level::trace);
  resource_logger_->set_level(spdlog::level::trace);
}
} // namespace vkr
