#pragma once

#include <vector>
#include <string>
#include <functional>
#include <boost/asio.hpp>

#if defined(USING_LOG) && USING_LOG
#ifdef USING_LOGURU
#include "loguru/loguru.hpp"
#define BEAUTY_INFO(cond, x)  VLOG_IF_S(loguru::Verbosity_INFO, cond) << x;
#define BEAUTY_ERROR(cond, x) VLOG_IF_S(loguru::Verbosity_ERROR, cond) << x;
#else
#include <iostream>
#define BEAUTY_INFO(cond, x)  ((cond) ? (std::cout << x << std::endl) : (std::cout));
#define BEAUTY_ERROR(cond, x) ((cond) ? (std::cout << x << std::endl) : (std::cout));
#endif
#else
#define BEAUTY_INFO(cond, x)  (void)0;
#define BEAUTY_ERROR(cond, x) (void)0;
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
    using address_v4 = boost::asio::ip::address_v4;
    using error_code = boost::system::error_code;
    using tcp = boost::asio::ip::tcp;
    using udp = boost::asio::ip::udp;

    template <typename _Protocol>
    using endpoint = typename _Protocol::endpoint;

    // --------------------------------------------------------------------------
    // Callback interface
    // --------------------------------------------------------------------------

    class acceptor;

    template <typename _Protocol>
    class session;

    template <typename _Protocol>
    class callback {

        using edp_t = endpoint<_Protocol>;
        using sess_t = session<_Protocol>;

    public:
        /**
         * @brief Callback on server acception succeeded.
         * @param acceptor Server endpoint.
         * @param edp_t Local endpoint.
         * @param edp_t Remote endpoint.
         * @note Server ONLY.
         */
        std::function<void(acceptor &, edp_t, edp_t)> on_accepted = [](acceptor &, edp_t, edp_t) {};

        /**
         * @brief Callback on client connection succeeded.
         * @param sess_t Current session.
         * @param Local endpoint, remote endpoint.
         * @note Client ONLY.
         */
        std::function<void(sess_t &, edp_t, edp_t)> on_connected = [](sess_t &, edp_t, edp_t) {};

        /**
         * @brief Callback on client connection failed.
         * @param sess_t Current session.
         * @note Client ONLY.
         *      return `true` to try connect again.
         *      return `false` to close the session. [Default]
         */
        std::function<bool(sess_t &, edp_t, error_code)> on_connect_failed
            = [](sess_t &, edp_t, error_code) { return false; };

        /**
         * @brief Callback on connection is closed.
         * @param sess_t Current session.
         * @note For a @ref client, the param `edp_t` make no sense.
         *       For a @ref server, its @ref acceptor will REPLACE this method to have better
         * response when a connection is failed.
         */
        std::function<void(sess_t &, edp_t)> on_disconnected = [](sess_t &, edp_t) {};

        /**
         * @brief Callback on successflly wrote some data.
         * @param sess_t Current session.
         */
        std::function<void(sess_t &, const size_t)> on_write = [](sess_t &, const size_t) {};

        /**
         * @brief Callback on write failed.
         *      return `true` to try write (async) again.
         *      return `false` to close the session. [Default]
         * @param sess_t Current session.
         */
        std::function<bool(sess_t &, error_code)> on_write_failed
            = [](sess_t &, error_code) { return false; };

        /**
         * @brief Callback on read some data.
         *      return `true` to try read (async) again. [Default]
         *      return `false` to close the session.
         * @param sess_t Current session.
         */
        std::function<bool(sess_t &, boost::asio::streambuf &, size_t)> on_read
            = [](sess_t &, boost::asio::streambuf &, size_t) { return true; };
        /**
         * @brief Callback on read failed.
         *      return `true` to try write (async) again.
         *      return `false` to close the session. [Default]
         * @param sess_t Current session.
         */
        std::function<bool(sess_t &, error_code)> on_read_failed
            = [](sess_t &, error_code) { return false; };
    };

} // namespace beauty
