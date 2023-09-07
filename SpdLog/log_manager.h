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

#define LOG_DEBUG(logger, ...) { \
        if (logger.should_log(spdlog::level::debug)) {\
           logger.log(spdlog::level::debug, __VA_ARGS__); }}

#define LOG_TRACE(logger, ...) { \
        if (logger.should_log(spdlog::level::trace)) {\
           logger.log(spdlog::level::trace, __VA_ARGS__); }}


class log_wrap;

class log_manager
{
    static const std::string root_logger_name;
    static const std::string messaging_logger_name;

    static const std::string log_pattern;

    log_manager();

    static std::shared_ptr<spdlog::logger> get_root_category();

    static std::shared_ptr<spdlog::logger> get_messaging_category();

public:
    friend log_wrap;

    static log_wrap create_logger(std::string);
    static log_wrap create_messaging_logger(std::string);

    bool initialize(const std::filesystem::path& base_path) const
    {
        static constexpr std::size_t max_size = 1024;
        static constexpr std::size_t max_files = 3;
        static spdlog::level::level_enum global_level = spdlog::level::info;

        spdlog::file_event_handlers handlers;
        handlers.after_open = [](const spdlog::filename_t&, std::FILE *file_stream) { (void)fputs("*** SpdLog Service v2023.4.0.1070 ***\n", file_stream); };
        try
        {
            spdlog::drop_all(); // remove implicit default logger

            const auto default_log_path = base_path / "default.log";
            const auto default_logger = get_root_category();
            default_logger->sinks()
                .emplace_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(default_log_path.generic_string(), max_size, max_files, false, handlers));

            spdlog::set_default_logger(default_logger);
            spdlog::set_pattern(log_pattern); // must be set after set_default_logger
            spdlog::set_level(global_level);

            const auto messaging_log_path = base_path / "messaging.log";
            const auto messaging_logger = get_messaging_category();
            messaging_logger->sinks()
                .emplace_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(messaging_log_path.generic_string(), max_size, max_files, false, handlers));
            spdlog::initialize_logger(messaging_logger);

            using namespace std::chrono_literals;
            spdlog::flush_every(5s);
            return true;
        }
        catch (const spdlog::spdlog_ex& e)
        {
            std::cerr << "Unable to initialize logging: " << e.what() << std::endl;
        }

        return false;
    }

    static void shutdown();

    static log_manager &instance();

};

class log_wrap
{
    std::string name_;
    std::shared_ptr<spdlog::logger> log_;

    explicit log_wrap(std::string name, std::shared_ptr<spdlog::logger> log)
        : name_(std::move(name)), log_(std::move(log))
    {
        std::clog << "ctor " << log_->name() << "." << name_ << std::endl;
    }

public:
    friend log_manager;

    log_wrap(const log_wrap&) = delete;
    log_wrap& operator=(const log_wrap&) = delete;

    log_wrap(log_wrap&& rhs) noexcept
        : name_(std::move(rhs.name_)), log_(std::move(rhs.log_))
    {
        std::clog << "mv ctor " << log_->name() << "." << name_ << std::endl;
    }

    log_wrap& operator=(log_wrap&& rhs) noexcept
    {
        name_ = std::move(rhs.name_);
        log_ = std::move(rhs.log_);
        std::clog << "mv assigned " << log_->name() << "." << name_ << std::endl;
        return *this;
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
        
        std::clog << std::string(">  log ").append(spdlog::level::to_string_view(lvl).data()).append("\n");

        log_->log(spdlog::source_loc{ "", 1, name_.c_str() }, lvl, fmt, std::forward<Args>(args)...);
    }

    bool is_trace_enabled() const
    {
        return should_log(spdlog::level::trace);
    }

    bool is_debug_enabled() const
    {
        return should_log(spdlog::level::debug);
    }

    bool is_info_enabled() const
    {
        return should_log(spdlog::level::info);
    }

    bool is_warn_enabled() const
    {
        return should_log(spdlog::level::warn);
    }

    bool is_error_enabled() const
    {
        return should_log(spdlog::level::err);
    }

    bool is_fatal_enabled() const
    {
        return should_log(spdlog::level::critical);
    }

    bool should_log(const spdlog::level::level_enum msg_level) const
    {
        return log_->should_log(msg_level);
    }
};

const std::string log_manager::root_logger_name("spdlog");
const std::string log_manager::messaging_logger_name("messaging");

// hack %! use source_loc.funcname as category name
const std::string log_manager::log_pattern("%Y-%m-%d %H:%M:%S,%e [%t] %l %! - %v");

inline log_manager::log_manager() = default;

inline log_wrap log_manager::create_logger(std::string name)
{
    return log_wrap(std::move(name), get_root_category());
}

inline log_wrap log_manager::create_messaging_logger(std::string name)
{
    return log_wrap(std::move(name), get_messaging_category());
}

inline void log_manager::shutdown()
{
    get_root_category().reset();
    get_messaging_category().reset();
    spdlog::shutdown();
}

inline log_manager &log_manager::instance()
{
    static log_manager static_instance;
    return static_instance;
}

inline std::shared_ptr<spdlog::logger> log_manager::get_root_category()
{
    static auto static_instance = std::make_shared<spdlog::logger>(root_logger_name);
    return static_instance;
}

inline std::shared_ptr<spdlog::logger> log_manager::get_messaging_category()
{
    static auto static_instance = std::make_shared<spdlog::logger>(messaging_logger_name);
    return static_instance;
}
