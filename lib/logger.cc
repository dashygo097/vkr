#include "vkr/logger.hh"
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>

namespace vkr {

std::shared_ptr<UiLogSink> Logger::ui_sink_;
std::shared_ptr<spdlog::logger> Logger::core_logger_;
std::shared_ptr<spdlog::logger> Logger::resource_logger_;
std::shared_ptr<spdlog::logger> Logger::pipeline_logger_;
std::shared_ptr<spdlog::logger> Logger::render_logger_;
std::shared_ptr<spdlog::logger> Logger::scene_logger_;
std::shared_ptr<spdlog::logger> Logger::ui_logger_;

void Logger::init() {
  std::vector<spdlog::sink_ptr> sinks;

  // Console formatter
  auto console_formatter = std::make_unique<spdlog::pattern_formatter>();
  console_formatter->add_flag<Formatter>('*');
  console_formatter->set_pattern("%^%*%$ \033[2mlibvkr::%n\033[0m %v");

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_formatter(std::move(console_formatter));
  sinks.push_back(console_sink);

  // File sink
  auto file_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>("vkr.log", true);
  file_sink->set_pattern("[%T] [%l] %n: %v");
  sinks.push_back(file_sink);

  // UI Sink (Memory buffer)
  ui_sink_ = std::make_shared<UiLogSink>(1000);
  ui_sink_->set_pattern(" %-5l %-16n %v");
  sinks.push_back(ui_sink_);

  // Initialize individual loggers with all three sinks
  core_logger_ =
      std::make_shared<spdlog::logger>("core    ", sinks.begin(), sinks.end());
  resource_logger_ =
      std::make_shared<spdlog::logger>("resource", sinks.begin(), sinks.end());
  pipeline_logger_ =
      std::make_shared<spdlog::logger>("pipeline", sinks.begin(), sinks.end());
  render_logger_ =
      std::make_shared<spdlog::logger>("render  ", sinks.begin(), sinks.end());
  scene_logger_ =
      std::make_shared<spdlog::logger>("scene   ", sinks.begin(), sinks.end());
  ui_logger_ =
      std::make_shared<spdlog::logger>("ui      ", sinks.begin(), sinks.end());

  // Set levels
  core_logger_->set_level(spdlog::level::trace);
  resource_logger_->set_level(spdlog::level::trace);
  pipeline_logger_->set_level(spdlog::level::trace);
  render_logger_->set_level(spdlog::level::trace);
  scene_logger_->set_level(spdlog::level::trace);
  ui_logger_->set_level(spdlog::level::trace);
}
} // namespace vkr
