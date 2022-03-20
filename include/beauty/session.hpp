#pragma once

#include <beauty/header.hpp>

#include <boost/asio.hpp>

#include <string>
#include <memory>
#include <type_traits>

namespace asio = boost::asio;
using boost::asio::ip::address_v4;

namespace beauty {
    // --------------------------------------------------------------------------
    // Handles an TCP server connection
    //---------------------------------------------------------------------------
    class session : public std::enable_shared_from_this<session> {
    public:
        using stream_type = void*;

    public:
        session(asio::io_context& ioc, asio::ip::tcp::socket&& socket, const callback& cb)
            : _socket(std::move(socket))
            , _identity_as_server(true)
            , _callback(cb)
#if (BOOST_VERSION < 107000)
            , _strand(_socket.get_executor())
#else
            , _strand(asio::make_strand(ioc))
#endif
        {
        }

        void do_connect(endpoint ep)
        {
            // Make the connection on the IP address we get from a lookup
            _INFO(true, "session_client:" << __LINE__ << " : Try a connection");
            _socket.async_connect(
                ep, [me = this->shared_from_this(), ep](const error_code& ec) {
                me->on_connect(ep, ec);
            });
        }

        void on_connect(const endpoint& ep, const error_code& ec)
        {
            // std::cout << "session_client:" << __LINE__ << " : on_connect" << std::endl;
            if (ec) {
                _callback.on_connect_failed(ep, ec);
                return;
            }
            else {
                error_code ecx;
                auto epx = _socket.remote_endpoint(ecx);
                _callback.on_connected(epx);
            }
        }

        void read(bool async)
        {
            // std::cout << "session: do read" << std::endl;
            // Make a new request_parser before reading
            boost::asio::streambuf::mutable_buffers_type mbuf
                = _buffer.prepare(1024 * 1024 * 1024); // 1Go..
            if (async) {
                // Read a full request (only if on _stream/_socket)
                boost::asio::async_read(_socket, mbuf,
                    asio::bind_executor(
                        _strand, [me = this->shared_from_this()](auto ec, auto bytes_transferred) {
                    me->on_read(ec, bytes_transferred);
                }));
            }
            else {
                error_code ec;
                size_t bytes_transferred = asio::read(_socket, mbuf, ec);
                on_read(ec, bytes_transferred);
            }
        }

        void on_read(error_code ec, std::size_t bytes_transferred)
        {
            // This means they closed the connection
            if (/*ec == asio::error::basic_errors::network_reset || */ ec) {
                return do_close();
            }

            // Copy data from to temporary buffer.
            std::vector<uint8_t> _temp_buffer;
            _buffer.commit(bytes_transferred);
            buffer_copy(boost::asio::buffer(_temp_buffer), _buffer.data());
            _buffer.consume(bytes_transferred);

            _callback.on_read(_temp_buffer);
        }

        void write(boost::asio::const_buffer&& buffer)
        {
            error_code ec;
            size_t bytes_transferred = asio::write(this->_socket, buffer, ec);
            on_write(ec, bytes_transferred);
        }

        void do_write(boost::asio::const_buffer&& buffer)
        {
            asio::async_write(this->_socket, buffer,
                asio::bind_executor(this->_strand,
                    [me = this->shared_from_this()](
                        auto ec, auto bytes_transferred) { me->on_write(ec, bytes_transferred); }));
        }

        void write(const std::vector<uint8_t>& pack, bool async) //
        {
            async ? do_write(boost::asio::buffer(pack)) : write(boost::asio::buffer(pack));
        }

        void write(const std::string& info, bool async)
        {
            async ? do_write(boost::asio::buffer(info.c_str(), info.size()))
                : write(boost::asio::buffer(info.c_str(), info.size()));
        }

        void write(const boost::asio::streambuf& buf, bool async)
        {
            async ? do_write(boost::asio::buffer(buf.data(), buf.size()))
                : write(boost::asio::buffer(buf.data(), buf.size()));
        }

        void on_write(error_code ec, std::size_t bytes_transferred)
        {
            // std::cout << "session: do write" << std::endl;
            if (ec) {
                return do_close();
            }

            _callback.on_write(bytes_transferred);
        }

        void do_close()
        {
            // Send a TCP shutdown
            error_code ec;
            _socket.shutdown(asio::ip::tcp::socket::shutdown_send, ec);
            _socket.close();

            endpoint epx = _socket.remote_endpoint(ec);
            _callback.on_disconnected(epx);
        }

    private:
        asio::ip::tcp::socket _socket;
        stream_type _stream = {};
        asio::strand<asio::io_context::executor_type> _strand;
        boost::asio::streambuf _buffer;
        const callback& _callback;
        const bool _identity_as_server;
    };
} // namespace beauty
