#pragma once

#include <vector>
#include <string>
#include <functional>
#include <boost/asio.hpp>

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

    // --------------------------------------------------------------------------

    //// This `enable_if` avoids `std::enable_if`'s failing when false.

    // using _sel_tcp = int;
    // using _sel_udp = bool;

    // template <bool _Test, class _T1 = _sel_tcp, class _T2 = _sel_udp>
    // struct enable_if {
    // };

    // template <class _T1, class _T2>
    // struct enable_if<true, _T1, _T2> {
    //     using type = _T1;
    // };

    // template <class _T1, class _T2>
    // struct enable_if<false, _T1, _T2> {
    //     using type = _T2;
    // }; // default member "type" = bool when !_Test

    // --------------------------------------------------------------------------

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

    template <typename _Protocol>
    class callback {

        using edp_t = endpoint<_Protocol>;

    public:
        /**
         * @brief Callback on server acception succeeded.
         * @note Server ONLY.
         */
        std::function<void(edp_t)> on_accepted = [](edp_t) {};

        /**
         * @brief Callback on client connection succeeded.
         * @note Client ONLY.
         */
        std::function<void(edp_t)> on_connected = [](edp_t) {};

        /**
         * @brief Callback on client connection failed.
         * @note Client ONLY.
         *      return `true` to try connect again.
         *      return `false` to close the session. [Default]
         */
        std::function<bool(edp_t, error_code)> on_connect_failed
            = [](edp_t, error_code) { return false; };

        /**
         * @brief Callback on connection is closed.
         * @note For a @ref client, the param `edp_t` make no sense.
         *       For a @ref server, its @ref acceptor will REPLACE this method to have better
         * response when a connection is failed.
         */
        std::function<void(edp_t)> on_disconnected = [](edp_t) {};

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
        std::function<bool(boost::asio::streambuf &, size_t)> on_read
            = [](boost::asio::streambuf &, size_t) { return true; };
        /**
         * @brief Callback on read failed.
         *      return `true` to try write (async) again.
         *      return `false` to close the session. [Default]
         */
        std::function<bool(error_code)> on_read_failed = [](error_code) { return false; };
    };

} // namespace beauty
