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
    template<typename _Protocol> 
    class session : public std::enable_shared_from_this<session<_Protocol>> {

        using cb_t = callback<_Protocol>;
        using edp_t = endpoint<_Protocol>;
        using socket_t = typename _Protocol::socket;

    public:
        session(
            asio::io_context &ioc, socket_t &&socket, const cb_t &cb, int verbose)
            : _socket(std::move(socket))
            , _callback(cb)
            , _verbose(verbose)
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
            _INFO(_verbose > 1, "Make connect to " << ep);
            _socket.async_connect(ep, [me = this->shared_from_this(), ep](const error_code &ec) {
                me->on_connect(ep, ec);
            });
        }

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
                _INFO(_verbose > 1, "Succeed connecting to " << ep);
                error_code ecx;
                auto epx = _socket.remote_endpoint(ecx);
                _callback.on_connected(epx);
                _is_connnected = true;
            }
        }

        void do_read(bool async)
        {
            _INFO(_verbose > 1, "Arrise " << (async ? "an async" : "a sync") << " read action.");
            boost::asio::streambuf::mutable_buffers_type mbuf
                = _buffer.prepare(1024 * 1024 * 1024); // 1Go..
            if (async) {
                _socket.async_read_some(mbuf,
                    asio::bind_executor(
                        _strand, [me = this->shared_from_this()](auto ec, auto tbytes) {
                            me->on_read(ec, tbytes);
                        }));
            } else {
                error_code ec;
                size_t tbytes = asio::read(_socket, mbuf, ec);
                on_read(ec, tbytes);
            }
        }

        void on_read(error_code ec, std::size_t tbytes)
        {
            if (ec) {
                _ERROR(
                    _verbose > 0, "Read faild with error (" << ec.value() << "): " << ec.message());
                if (_callback.on_read_failed(ec) && _is_connnected) {
                    read(true);
                } else {
                    do_close();
                }
            } else {
                _INFO(_verbose > 1, "Successfully read " << tbytes << " bytes.");
                // Copy data from to temporary buffer.
                std::vector<uint8_t> _temp_buffer;
                _buffer.commit(tbytes);
                buffer_copy(boost::asio::buffer(_temp_buffer), _buffer.data());
                _buffer.consume(tbytes);
                if (_callback.on_read(_temp_buffer)) {
                    read(true);
                }
            }
        }

        void do_write(const boost::asio::const_buffer &&buffer, bool async)
        {
            _INFO(_verbose > 1, "Arrise " << (async ? "an async" : "a sync") << " write action.");
            boost::asio::const_buffer copy_buffer = buffer;
            if (async) {
                asio::async_write(this->_socket, buffer,
                    asio::bind_executor(this->_strand,
                        [me = this->shared_from_this(), copy_buffer](
                            auto ec, auto tbytes) { me->on_write(copy_buffer, ec, tbytes); }));
            } else {
                error_code ec;
                size_t tbytes = asio::write(this->_socket, buffer, ec);
                on_write(std::move(copy_buffer), ec, tbytes);
            }
        }

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
            _callback.on_disconnected(epx);
            _is_connnected = false;
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
