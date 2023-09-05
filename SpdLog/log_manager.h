#pragma once
#include <filesystem>
#include <string>
#include <iostream>
#include <optional>

#define SPDLOG_LEVEL_NAMES \
{ \
  std::string_view{"TRACE"}, \
  std::string_view{"DEBUG"}, \
  std::string_view{"INFO"}, \
  std::string_view{"WARN"}, \
  std::string_view{"ERROR"}, \
  std::string_view{"FATAL"}, \
  std::string_view{"OFF"} \
};

// SPDLOG_NO_EXCEPTIONS calls abort() not sure whether this is good idea...

#define SPDLOG_NO_EXCEPTIONS

#pragma warning(push)
#pragma warning ( disable: 4307 ) // spdlog warning C4307: '*': integral constant overflow
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#pragma warning(pop)

class log_ptr;

class log_manager
{
    static const std::string default_sink_name;
    static const std::string messaging_sink_name;

    static const std::string log_pattern;

    log_manager();
    std::unordered_map<std::string, spdlog::sink_ptr> sinks_;

    std::shared_ptr<spdlog::logger> get_internal_logger(const std::string& sink_name, const std::string& logger_name);

public:
    friend log_ptr;

    static log_ptr get_logger(std::string logger_name);
    static log_ptr get_messaging_logger(std::string logger_name);

    void initialize(const std::filesystem::path& base_path)
    {
        static constexpr std::size_t max_size = 1024;
        static constexpr std::size_t max_files = 3;
        static spdlog::level::level_enum global_level = spdlog::level::debug;

        spdlog::drop_all(); // remove implicit default logger
        spdlog::set_level(global_level);

        spdlog::file_event_handlers handlers;
        handlers.after_open = [](const spdlog::filename_t&, std::FILE *file_stream) { fputs("*** SpdLog Service v2023.4.0.1070 ***\n", file_stream); };

        const auto default_log_path = base_path / "default.log";
        auto default_log_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(default_log_path.generic_string(), max_size, max_files, false, handlers);
        const auto default_logger = std::make_shared<spdlog::logger>("SpdLog", default_log_sink);
        
        spdlog::set_default_logger(default_logger);
        spdlog::set_pattern(log_pattern); // must be set after set_default_logger

        const auto messaging_log_path = base_path / "messaging.log";
        const auto messaging_log_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(messaging_log_path.generic_string(), max_size, max_files, false, handlers);

        sinks_[default_sink_name] = default_log_sink;
        sinks_[messaging_sink_name] = messaging_log_sink;

        spdlog::flush_every(std::chrono::seconds(1));
    }

    static void shutdown()
    {
        spdlog::shutdown();
    }

    static log_manager &instance();
};

class log_ptr
{
    std::string sink_name_;
    std::string name_;
    std::optional<std::shared_ptr<spdlog::logger>> logger_;

    explicit log_ptr(std::string sink_name, std::string name)
        : sink_name_(std::move(sink_name)), name_(std::move(name)), logger_(std::nullopt)
    {
        std::clog << "ctor " << sink_name_ << "." << name_ << std::endl;
    }

public:
    friend log_manager;

    log_ptr(const log_ptr&) = delete;
    log_ptr& operator=(const log_ptr&) = delete;

    log_ptr(log_ptr&& rhs) noexcept // cannot compile without this, but it is not called at runtime!?
    {
        sink_name_ = std::move(rhs.sink_name_);
        name_ = std::move(rhs.name_);
        logger_ = std::move(rhs.logger_);
        std::clog << "mv ctor " << sink_name_ << "." << name_ << std::endl;
    }

    void initialize();

    spdlog::logger* get()
    {
        initialize();
        return logger_.value().get();
    }

    spdlog::logger* operator->()
    {
        return get();
    }
};

inline void log_ptr::initialize()
{
    if (logger_.has_value())
        return;
    {
        static std::mutex mutex;
        std::lock_guard lk{ mutex };
        if (logger_.has_value())
            return;
        logger_ = log_manager::instance().get_internal_logger(sink_name_, name_);
    }
}

const std::string log_manager::default_sink_name("default");
const std::string log_manager::messaging_sink_name("messaging");
const std::string log_manager::log_pattern("%Y-%m-%d %H:%M:%S,%e [%t] %l %n - %v");

inline log_manager::log_manager() = default;

inline std::shared_ptr<spdlog::logger> log_manager::get_internal_logger(const std::string& sink_name, const std::string& logger_name)
{
    // logger name must be unique so we need to combine it with sink_name for nondefault
    const auto spd_logger_name = sink_name == default_sink_name ? logger_name : sink_name + "." + logger_name;
    auto logger = spdlog::get(spd_logger_name);
    if (logger)
    {
        return logger;
    }

    logger = std::make_shared<spdlog::logger>(spd_logger_name, sinks_[sink_name]);
    logger->set_level(spdlog::level::debug);
    spdlog::initialize_logger(logger);
    return logger;
}

inline log_ptr log_manager::get_logger(std::string logger_name)
{
    return log_ptr(default_sink_name, std::move(logger_name));
}

inline log_ptr log_manager::get_messaging_logger(std::string logger_name)
{
    return log_ptr(messaging_sink_name, std::move(logger_name));
}


inline log_manager &log_manager::instance()
{
    static log_manager static_instance;
    return static_instance;
}
