#pragma once
#include <atomic>
#include <thread>

#include "log_manager.h"

// log4cxx
// static LoggerPtr _log(Logger::getLogger("agent_messaging"));
// static LoggerPtr _log_msg(Logger::getLogger("messaging.agent_messaging"));

static log_ptr _log = log_manager::get_logger("agent_messaging");
static log_ptr _log_msg = log_manager::get_messaging_logger("agent_messaging");

class service_worker
{
    std::string name_;
    std::atomic_bool done_{};
    std::thread worker_{}; // std::jthread is C++20

    log_ptr member_log_;

public:

    explicit service_worker(std::string name)
        : name_(std::move(name)),
          member_log_(log_manager::get_logger(name_)) // move ctor. needed??
                                                      // otherwise it fails on
                                                      // error C2280: 'log_ptr::log_ptr(const log_ptr &)': attempting to reference a deleted function
                                                      // but it does not seem to call mv ctor in actual runtime ??
    {
        this->worker_ = std::thread{ &service_worker::run, this };
    }

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
        //member_log_ = log_manager::get_logger("agent_messaging"); // -- error, we do not want this
        member_log_->info("instance logger");
        _log->info("Static logger");
        using namespace std::chrono_literals;
        while (!this->done_.load())
        {
            // this produces:

            // 2023-09-05 13:19:08,221 [732] DEBUG messaging.agent_messaging - Foo is logging...

            // "sink_name." is superfluous but we don't care...
            _log_msg->debug("{} is logging...", name_);
            std::this_thread::sleep_for(1ms);
        }
    }
};
