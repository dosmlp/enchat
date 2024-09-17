// Copyright(c) 2016 Alexander Dalshov.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>
#include <iostream>

namespace spdlog {
namespace sinks {

template<typename Mutex>
class apple_unifiedlog_sink : public base_sink<Mutex>
{
public:
    apple_unifiedlog_sink()
    {
    }
    ~apple_unifiedlog_sink()
    {
    }

protected:
    void sink_it_(const details::log_msg &msg) override
    {
        //TODO https://developer.apple.com/documentation/os/os_log_with_type
        memory_buf_t formatted;
        base_sink<Mutex>::formatter_->format(msg, formatted);
        formatted.push_back('\0');
        std::cout<<formatted.data()<<std::flush;
    }

    void flush_() override {}
private:
};

using apple_unifiedlog_sink_mt = apple_unifiedlog_sink<std::mutex>;
using apple_unifiedlog_sink_st = apple_unifiedlog_sink<details::null_mutex>;


} // namespace sinks
} // namespace spdlog

