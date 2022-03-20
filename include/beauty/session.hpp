#pragma once

#include <beauty/header.hpp>

#include <boost/asio.hpp>

#include <string>
#include <memory>
#include <type_traits>

namespace asio = boost::asio;

namespace beauty {
    // --------------------------------------------------------------------------
    // Handles an TCP server connection
    //---------------------------------------------------------------------------
    class session : public std::enable_shared_from_this<session> {
    public:
        session(
            asio::io_context &ioc, asio::ip::tcp::socket &&socket, const callback &cb, int verbose)
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

        void do_connect(endpoint ep)
        {
            _INFO(_verbose > 1, "Make connect to " << ep);
            _socket.async_connect(ep, [me = this->shared_from_this(), ep](const error_code &ec) {
                me->on_connect(ep, ec);
            });
        }

        void on_connect(const endpoint &ep, const error_code &ec)
        {
            if (ec) {
                _ERROR(_verbose > 0,
                    "Connect to " << ep << " faild with error (" << ec.value()
                                  << "): " << ec.message());
                _callback.on_connect_failed(ep, ec);
                return;
            } else {
                _INFO(_verbose > 1, "Succeed connecting to " << ep);
                error_code ecx;
                auto epx = _socket.remote_endpoint(ecx);
                _callback.on_connected(epx);
            }
        }

    public:
        void read(bool async) //
        {
            do_read(async);
        }

        void write(const std::vector<uint8_t> &pack, bool async)
        {
            do_write(boost::asio::buffer(pack), async);
        }

        void write(const std::string &info, bool async)
        {
            do_write(boost::asio::buffer(info.c_str(), info.size()), async);
        }

        void write(const boost::asio::streambuf &buf, bool async)
        {
            do_write(boost::asio::buffer(buf.data(), buf.size()), async);
        }

    protected:
        void do_read(bool async)
        {
            _INFO(_verbose > 1, "Arrise " << (async ? "an async" : "a sync") << " read action.");
            boost::asio::streambuf::mutable_buffers_type mbuf
                = _buffer.prepare(1024 * 1024 * 1024); // 1Go..
            if (async) {
                // Read a full request (only if on _stream/_socket)
                boost::asio::async_read(_socket, mbuf,
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
                if (_callback.on_read_failed(ec)) {
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
                _callback.on_read(_temp_buffer);
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
                if (_callback.on_write_failed(ec)) {
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
            endpoint epx = _socket.remote_endpoint(ec);
            _INFO(_verbose > 0, "Close connection on " << epx);
            // Send a TCP shutdown
            _socket.shutdown(asio::ip::tcp::socket::shutdown_send, ec);
            _socket.close();
            _callback.on_disconnected(epx);
        }

    private:
        asio::ip::tcp::socket _socket;
        asio::strand<asio::io_context::executor_type> _strand;
        boost::asio::streambuf _buffer;
        const callback &_callback;
        const int _verbose;
    };
} // namespace beauty
