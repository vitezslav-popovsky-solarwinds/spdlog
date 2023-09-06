#pragma once
#include <filesystem>
#include <iostream>
#include <string>

#define SPDLOG_LEVEL_NAMES \
    { spdlog::string_view_t("TRACE", 5) \
    , spdlog::string_view_t("DEBUG", 5) \
    , spdlog::string_view_t("INFO", 4)  \
    , spdlog::string_view_t("WARN", 4)  \
    , spdlog::string_view_t("ERROR", 5) \
    , spdlog::string_view_t("FATAL", 5) \
    , spdlog::string_view_t("OFF", 3) }

#pragma warning(push)
#pragma warning ( disable: 4307 ) // spdlog warning C4307: '*': integral constant overflow
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#pragma warning(pop)

class log_wrap;

class log_manager
{
    static const std::string default_logger_name;
    static const std::string messaging_logger_name;

    static const std::string log_pattern;
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers_;

    log_manager();

    std::shared_ptr<spdlog::logger> get_internal_logger(const std::string &logger_name);

public:
    friend log_wrap;

    static log_wrap create_logger(std::string category);
    static log_wrap create_messaging_logger(std::string category);

    bool initialize(const std::filesystem::path& base_path)
    {
        static constexpr std::size_t max_size = 1024;
        static constexpr std::size_t max_files = 3;
        static spdlog::level::level_enum global_level = spdlog::level::trace;

        spdlog::file_event_handlers handlers;
        handlers.after_open = [](const spdlog::filename_t&, std::FILE *file_stream) { fputs("*** SpdLog Service v2023.4.0.1070 ***\n", file_stream); };
        try
        {
            spdlog::drop_all(); // remove implicit default logger

            const auto default_log_path = base_path / "default.log";
            auto default_log_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(default_log_path.generic_string(), max_size, max_files, false, handlers);
            const auto default_logger = std::make_shared<spdlog::logger>(default_logger_name, default_log_sink);

            spdlog::set_default_logger(default_logger);
            spdlog::set_pattern(log_pattern); // must be set after set_default_logger
            spdlog::set_level(global_level);

            const auto messaging_log_path = base_path / "messaging.log";
            const auto messaging_log_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(messaging_log_path.generic_string(), max_size, max_files, false, handlers);
            const auto messaging_logger = std::make_shared<spdlog::logger>(messaging_logger_name, messaging_log_sink);
            spdlog::initialize_logger(messaging_logger);

            loggers_[default_logger_name] = default_logger;
            loggers_[messaging_logger_name] = messaging_logger;

            spdlog::flush_every(std::chrono::seconds(1));
            return true;
        }
        catch (const spdlog::spdlog_ex& e)
        {
            std::cerr << "Unable to initialize logging: " << e.what() << std::endl;
        }

        return false;
    }

    static void shutdown()
    {
        spdlog::shutdown();
    }

    static log_manager &instance();
};

class log_wrap
{
    std::string logger_name_;
    std::string category_;

    explicit log_wrap(std::string logger_name, std::string category)
        : logger_name_(std::move(logger_name)), category_(std::move(category))
    {
        std::clog << "ctor " << logger_name_ << "." << category_ << std::endl;
    }

public:
    friend log_manager;

    log_wrap(const log_wrap&) = delete;
    log_wrap& operator=(const log_wrap&) = delete;

    log_wrap(log_wrap&& rhs) noexcept
    {
        logger_name_ = std::move(rhs.logger_name_);
        category_ = std::move(rhs.category_);
        std::clog << "mv ctor " << logger_name_ << "." << category_ << std::endl;
    }


    template<typename... Args>
    void trace(spdlog::format_string_t<Args...> fmt, Args &&...args) const
    {
        log(spdlog::level::trace, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(spdlog::format_string_t<Args...> fmt, Args &&...args) const
    {
        log(spdlog::level::debug, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(spdlog::format_string_t<Args...> fmt, Args &&...args) const
    {
        log(spdlog::level::info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(spdlog::format_string_t<Args...> fmt, Args &&...args) const
    {
        log(spdlog::level::warn, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(spdlog::format_string_t<Args...> fmt, Args &&...args) const
    {
        log(spdlog::level::err, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void fatal(spdlog::format_string_t<Args...> fmt, Args &&...args) const
    {
        log(spdlog::level::critical, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log(spdlog::level::level_enum lvl, spdlog::format_string_t<Args...> fmt, Args &&...args) const
    {
        if (const auto logger = log_manager::instance().get_internal_logger(logger_name_))
        {
            logger->log(spdlog::source_loc{ "", 1, category_.c_str() }, lvl, fmt, std::forward<Args>(args)...);
        }
    }

};

const std::string log_manager::default_logger_name("default");
const std::string log_manager::messaging_logger_name("messaging");

// hack %! use source_loc.funcname as category name
const std::string log_manager::log_pattern("%Y-%m-%d %H:%M:%S,%e [%t] %l %! - %v");

inline log_manager::log_manager() = default;

inline log_wrap log_manager::create_logger(std::string category)
{
    return log_wrap(default_logger_name, std::move(category));
}

inline log_wrap log_manager::create_messaging_logger(std::string category)
{
    return log_wrap(messaging_logger_name, std::move(category));
}

inline std::shared_ptr<spdlog::logger> log_manager::get_internal_logger(const std::string& logger_name)
{
    return loggers_[logger_name];
}

inline log_manager &log_manager::instance()
{
    static log_manager static_instance;
    return static_instance;
}
