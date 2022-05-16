#pragma once

#include <beauty/header.hpp>

#include <boost/asio.hpp>
#include <boost/atomic.hpp>

#include <string>
#include <memory>
#include <type_traits>

namespace asio = boost::asio;

namespace beauty {

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

        ~session()
        {
            do_close();
            error_code ec;
            edp_t ep = _socket.local_endpoint(ec);
            BEAUTY_ERROR(_verbose > 0, "Session on " << ep << " destroyed.");
        }

        /**
         * @brief Check connection.
         */
        bool is_connnected() const { return _is_connnected; }

        /**
         * @brief Make connection.
         * @param ep Target remote endpoint.
         */
        void connect(edp_t ep, bool retry = false)
        {
            if (_is_connnected)
                return;
            BEAUTY_INFO(!retry, "Try connect to " << ep);
            _socket.async_connect(ep, [me = this->shared_from_this(), ep](const error_code &ec) {
                me->on_connect(ep, ec);
            });
        }

        /**
         * @brief Start a receive action.
         * @param ep Target remote endpoint.
         * @param async If using async reading mode.
         * @param buffer_size Size of the receiving buffer.
         */
        void receive(endpoint<udp> ep, bool async,
            const size_t buffer_size = 32 * 1024 /* MTU max packet size */);

        /**
         * @brief Start a read action.
         * @param async If using async reading mode.
         * @param buffer_size Size of the receiving buffer.
         */
        void read(bool async, const size_t buffer_size = 32 * 1024)
        {
            do_read(buffer_size, async);
        }

        /**
         * @brief Write some data.
         * @param pack The buffer in type of a packet of bytes.
         * @param async If using async writing mode.
         */
        void write(const std::vector<uint8_t> &pack, bool async)
        {
            do_write(boost::asio::buffer(pack.data(), pack.size()), async);
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
                BEAUTY_ERROR(_verbose > 0,
                    "Connect to " << ep << " faild with error (" << ec.value()
                                  << "): " << ec.message());
                // Only refused connection could be reconnect.
                if (_callback.on_connect_failed(*this, ep, ec) && !_is_connnected
                    && ec == boost::system::errc::connection_refused) {
                    connect(ep, true);
                }
                return;
            } else {
                BEAUTY_INFO(true, "Succeed connecting to " << ep);
                error_code ecx;
                auto ep = _socket.local_endpoint(ecx);
                auto epr = _socket.remote_endpoint(ecx);
                _is_connnected = true;

                _callback.on_connected(*this, ep, epr);
            }
        }

        // TCP only.
        void do_read(const size_t buffer_size, bool async);

        void on_read(const edp_t &ep, error_code ec, std::size_t tbytes);

        void do_write(const boost::asio::const_buffer &&buffer, bool async);

        void on_write(const boost::asio::const_buffer &buffer, error_code ec, std::size_t tbytes)
        {
            if (ec) {
                BEAUTY_ERROR(_verbose > 0,
                    "Write faild with error (" << ec.value() << "): " << ec.message());
                // Will re-write only when connected.
                if (_callback.on_write_failed(*this, ec) && _is_connnected) {
                    do_write(std::move(buffer), true);
                } else {
                    do_close();
                }
            } else {
                BEAUTY_INFO(_verbose > 1, "Successfully write " << tbytes << " bytes.");
                _callback.on_write(*this, tbytes);
            }
        }

        void do_close()
        {
            if (!_is_connnected)
                return;

            error_code ec;
            edp_t epx = _socket.remote_endpoint(ec);
            BEAUTY_ERROR(true, "Close connection on " << epx);
            // Send a TCP shutdown
            _socket.shutdown(socket_t::shutdown_send, ec);
            _socket.close();
            _is_connnected = false;
            _callback.on_disconnected(*this, epx);
        }

    protected:
        friend class acceptor;
        boost::atomic<bool> _is_connnected = false;

    private:
        socket_t _socket;
        asio::strand<asio::io_context::executor_type> _strand;
        boost::asio::streambuf _buffer;
        const cb_t &_callback;
        const int _verbose;
    };

} // namespace beauty
