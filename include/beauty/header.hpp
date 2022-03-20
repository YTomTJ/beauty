#pragma once

#include <vector>
#include <string>
#include <functional>
#include <boost/asio.hpp>

#define USING_LOG 1
#define USING_LOGURU

#if defined(USING_LOG) && USING_LOG
#ifdef USING_LOGURU
#include "./loguru/loguru.hpp"
#define _INFO(cond, x)  VLOG_IF_S(loguru::Verbosity_INFO, cond) << x;
#define _ERROR(cond, x) VLOG_IF_S(loguru::Verbosity_ERROR, cond) << x;
#else
#define _INFO(cond, x) (cond ? std::cout << x << std::endl : (void)0;
#define _ERROR(cond, x) (cond ? std::cout << x << std::endl: (void)0;
#endif
#else
#define _INFO(cond, x)  (void)0;
#define _ERROR(cond, x) (void)0;
#endif

#ifndef __FUNCTION_NAME__
#ifdef WIN32 // WINDOWS
#define __FUNCTION_NAME__ __FUNCTION__
#else //*NIX
#define __FUNCTION_NAME__ __func__
#endif
#endif

namespace beauty {

    using buffer_type = std::vector<uint8_t>;
    using endpoint = boost::asio::ip::tcp::endpoint;
    using error_code = boost::system::error_code;

    // --------------------------------------------------------------------------
    // TCP callback interface
    // --------------------------------------------------------------------------

    class callback {
    public:
        std::function<void(endpoint)> on_connected = [](endpoint) {};
        std::function<void(endpoint, error_code)> on_connect_failed = [](endpoint, error_code) {};
        std::function<void(endpoint)> on_accepted = [](endpoint) {};
        std::function<void(endpoint)> on_disconnected = [](endpoint) {};
        std::function<void(const size_t)> on_write = [](const size_t) {};
        std::function<void(const buffer_type &)> on_read = [](const buffer_type &) {};
    };

} // namespace beauty
