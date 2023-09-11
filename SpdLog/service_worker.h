#pragma once
#include <atomic>
#include <thread>

#include "log_manager.h"

// log4cxx
// static LoggerPtr _log(Logger::getLogger("agent_messaging"));
// static LoggerPtr _log_msg(Logger::getLogger("messaging.agent_messaging"));

static log_wrap _log = log_manager::create_logger("agent_messaging");
static log_wrap _log_msg = log_manager::create_messaging_logger("agent_messaging");

class service_worker
{
    std::string name_;
    std::atomic_bool done_{};
    std::thread worker_{}; // std::jthread is C++20

    const log_wrap member_log_; //can be const it's just a wrapper

public:

    explicit service_worker(std::string name)
        : name_(std::move(name)),
          member_log_(log_manager::create_logger(name_))
    {
        _log.debug(R"(creating new service_worker({{"{}"}}))", name);
        this->worker_ = std::thread{ &service_worker::run, this };
    }

    service_worker(const service_worker&) = delete;
    service_worker& operator=(const service_worker&) = delete;

    void stop()
    {
        this->done_.store(true);
    }

    void join()
    {
        if (this->worker_.joinable())
            this->worker_.join();
    }

private:

    void run()
    {
        LOG_TRACE(member_log_, "instance trace")

        // wformat_string_t
        const std::wstring wstr(L"8Go de RAM est la quantité que nous recommandons pour une utilisation occasionnelle de votre ordinateur.");
        _log.info(L"🛈 {} 🛈", wstr);
        _log.info(L"Mémoire {}Go", 8);
        _log.warn(L"Speicherkapazität {}kB", 512);
        _log.debug(L"Aktivitätsträger {}", std::this_thread::get_id());

        _log.trace("{} static logger", name_);
        _log.debug("{} static logger", name_);
        _log.info("{} static logger", name_);
        _log.warn("{} static logger", name_);
        _log.error("{} static logger", name_);
        _log.fatal("{} static logger", name_);

        using namespace std::chrono_literals;
        while (!this->done_.load())
        {
            // this produces:

            // 2023-09-06 10:08:38,655 [3212] DEBUG agent_messaging - Bar is logging...
            // 2023-09-06 10:08:38,657 [13516] DEBUG agent_messaging - Foo is logging...

            // in c:\ProgramData\SpdLog\messaging.log like log4cxx nice...
            LOG_DEBUG(_log_msg, "{} is logging...", name_)
            std::this_thread::sleep_for(1ms);
        }
    }
};
