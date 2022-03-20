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
    using address_v4 = boost::asio::ip::address_v4;
    using error_code = boost::system::error_code;

    // --------------------------------------------------------------------------
    // TCP callback interface
    // --------------------------------------------------------------------------

    class callback {
    public:
        /**
         * @brief Callback on server acception succeeded.
         * @note Server ONLY.
         */
        std::function<void(endpoint)> on_accepted = [](endpoint) {};

        /**
         * @brief Callback on client connection succeeded.
         * @note Client ONLY.
         */
        std::function<void(endpoint)> on_connected = [](endpoint) {};

        /**
         * @brief Callback on client connection failed.
         * @note Client ONLY.
         *      return `true` to try connect again.
         *      return `false` to close the session. [Default]
         */
        std::function<bool(endpoint, error_code)> on_connect_failed
            = [](endpoint, error_code) { return false; };

        /**
         * @brief Callback on connection is closed.
         * @note For a @ref client, the param `endpoint` make no sense.
         *       For a @ref server, its @ref acceptor will REPLACE this method to have better
         * response when a connection is failed.
         */
        std::function<void(endpoint)> on_disconnected = [](endpoint) {};

        /**
         * @brief Callback on successflly wrote some data.
         */
        std::function<void(const size_t)> on_write = [](const size_t) {};
        /**
         * @brief Callback on write failed.
         *      return `true` to try write (async) again.
         *      return `false` to close the session. [Default]
         */
        std::function<bool(error_code)> on_write_failed = [](error_code) { return false; };

        /**
         * @brief Callback on read some data.
         *      return `true` to try read (async) again. [Default]
         *      return `false` to close the session.
         */
        std::function<bool(const buffer_type &)> on_read = [](const buffer_type &) { return true; };
        /**
         * @brief Callback on read failed.
         *      return `true` to try write (async) again.
         *      return `false` to close the session. [Default]
         */
        std::function<bool(error_code)> on_read_failed = [](error_code) { return false; };
    };

} // namespace beauty
