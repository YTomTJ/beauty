#pragma once

#include <beauty/header.hpp>

#include <boost/asio.hpp>
#include <boost/atomic.hpp>

#include <string>
#include <memory>
#include <type_traits>

namespace asio = boost::asio;

namespace beauty {
    // --------------------------------------------------------------------------
    // Handles an TCP server connection
    //---------------------------------------------------------------------------
    template <typename _Protocol>
    class session : public std::enable_shared_from_this<session<_Protocol>> {

        using cb_t = callback<_Protocol>;
        using edp_t = endpoint<_Protocol>;
        using socket_t = typename _Protocol::socket;

    public:
        session(asio::io_context &ioc, const cb_t &cb, int verbose)
            : _callback(cb)
            , _verbose(verbose)
            , _socket(ioc)
#if (BOOST_VERSION < 107000)
            , _strand(_socket.get_executor())
#else
            , _strand(asio::make_strand(ioc))
#endif
        {
        }

        session(asio::io_context &ioc, socket_t &&soc, const cb_t &cb, int verbose)
            : _callback(cb)
            , _verbose(verbose)
            , _socket(std::move(soc))
#if (BOOST_VERSION < 107000)
            , _strand(_socket.get_executor())
#else
            , _strand(asio::make_strand(ioc))
#endif
        {
        }

        /**
         * @brief Make connection.
         * @param ep Target remote endpoint.
         */
        void connect(edp_t ep)
        {
            if (_is_connnected)
                return;
            _INFO(_verbose > 0, "Make connect to " << ep);
            _socket.async_connect(ep, [me = this->shared_from_this(), ep](const error_code &ec) {
                me->on_connect(ep, ec);
            });
        }

        /**
         * @brief Start a receive action.
         * @param ep Target remote endpoint.
         * @param async If using async reading mode.
         */
        void receive(endpoint<udp> ep, bool async);

        /**
         * @brief Start a read action.
         * @param async If using async reading mode.
         */
        void read(bool async) //
        {
            do_read(async);
        }

        /**
         * @brief Write some data.
         * @param pack The buffer in type of a packet of bytes.
         * @param async If using async writing mode.
         */
        void write(const std::vector<uint8_t> &pack, bool async)
        {
            do_write(boost::asio::buffer(pack), async);
        }

        /**
         * @brief Write some data.
         * @param pack The string type buffer.
         * @param async If using async writing mode.
         */
        void write(const std::string &info, bool async)
        {
            do_write(boost::asio::buffer(info.c_str(), info.size()), async);
        }

        /**
         * @brief Write some data.
         * @param pack The stream type buffer.
         * @param async If using async writing mode.
         */
        void write(const boost::asio::streambuf &buf, bool async)
        {
            do_write(boost::asio::buffer(buf.data(), buf.size()), async);
        }

    protected:
        void on_connect(const edp_t &ep, const error_code &ec)
        {
            if (ec) {
                _ERROR(_verbose > 0,
                    "Connect to " << ep << " faild with error (" << ec.value()
                                  << "): " << ec.message());
                // Only refused connection could be reconnect.
                if (_callback.on_connect_failed(ep, ec) && !_is_connnected
                    && ec == boost::system::errc::connection_refused) {
                    connect(ep);
                }
                return;
            } else {
                _INFO(_verbose > 0, "Succeed connecting to " << ep);
                error_code ecx;
                auto epx = _socket.remote_endpoint(ecx);
                _is_connnected = true;

                _callback.on_connected(epx);
            }
        }

        // TCP only.
        void do_read(bool async);

        void on_read(const edp_t &ep, error_code ec, std::size_t tbytes);

        void do_write(const boost::asio::const_buffer &&buffer, bool async);

        void on_write(const boost::asio::const_buffer &buffer, error_code ec, std::size_t tbytes)
        {
            if (ec) {
                _ERROR(_verbose > 0,
                    "Write faild with error (" << ec.value() << "): " << ec.message());
                // Will re-write only when connected.
                if (_callback.on_write_failed(ec) && _is_connnected) {
                    do_write(std::move(buffer), true);
                } else {
                    do_close();
                }
            } else {
                _INFO(_verbose > 1, "Successfully write " << tbytes << " bytes.");
                _callback.on_write(tbytes);
            }
        }

        void do_close()
        {
            error_code ec;
            edp_t epx = _socket.remote_endpoint(ec);
            _INFO(_verbose > 0, "Close connection on " << epx);
            // Send a TCP shutdown
            _socket.shutdown(socket_t::shutdown_send, ec);
            _socket.close();
            _is_connnected = false;

            _callback.on_disconnected(epx);
        }

    private:
        boost::atomic<bool> _is_connnected = false;
        socket_t _socket;
        asio::strand<asio::io_context::executor_type> _strand;
        boost::asio::streambuf _buffer;
        const cb_t &_callback;
        const int _verbose;
    };

} // namespace beauty
