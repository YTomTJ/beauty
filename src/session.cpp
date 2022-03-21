#include <beauty/session.hpp>

namespace beauty {

    void session<udp>::receive(endpoint<udp> ep, bool async, const size_t buffer_size)
    {
        if (!_socket.is_open()) {
            error_code ec;
            _socket.open(ep.protocol(), ec);
            if (ec) {
                _ERROR(_verbose > 0,
                    "Open UDP socket for " << ep << " faild with error (" << ec.value()
                                           << "): " << ec.message());
                return;
            }
            _socket.bind(ep, ec);
            if (ec) {
                _ERROR(_verbose > 0,
                    "Bind UDP port for " << ep << " faild with error (" << ec.value()
                                         << "): " << ec.message());
                return;
            }
        }
        _INFO(
            _verbose > 1, "Start " << (async ? "an async" : "a sync") << " receiving from " << ep);
        boost::asio::streambuf::mutable_buffers_type mbuf = _buffer.prepare(buffer_size);
        if (async) {
            _socket.async_receive(mbuf, [me = this->shared_from_this(), ep](auto ec, auto tbytes) {
                me->on_read(ep, ec, tbytes);
            });
        } else {
            error_code ec;
            size_t tbytes = _socket.receive_from(mbuf, ep);
            on_read(ep, ec, tbytes);
        }
    }

    void session<tcp>::do_read(const size_t buffer_size, bool async)
    {
        _INFO(_verbose > 1, "Arrise " << (async ? "an async" : "a sync") << " read action.");
        boost::asio::streambuf::mutable_buffers_type mbuf = _buffer.prepare(buffer_size);
        if (async) {
            this->_socket.async_read_some(mbuf,
                asio::bind_executor(_strand, [me = this->shared_from_this()](auto ec, auto tbytes) {
                    me->on_read({}, ec, tbytes);
                }));
        } else {
            error_code ec;
            size_t tbytes = _socket.read_some(mbuf, ec);
            on_read({}, ec, tbytes);
        }
    }

    template <>
    void session<tcp>::on_read(const edp_t & /* ep not used */, error_code ec, std::size_t tbytes)
    {
        if (ec) {
            _ERROR(_verbose > 0, "Read faild with error (" << ec.value() << "): " << ec.message());
            if (_callback.on_read_failed(ec) && _is_connnected) {
                read(true);
            } else {
                do_close();
            }
        } else {
            _INFO(_verbose > 1, "Successfully read " << tbytes << " bytes.");
            _buffer.commit(tbytes);
            if (_callback.on_read(_buffer, tbytes)) {
                read(true);
            }
            _buffer.consume(tbytes);
        }
    }

    template <>
    void session<udp>::on_read(const edp_t &ep, error_code ec, std::size_t tbytes)
    {
        if (ec) {
            _ERROR(_verbose > 0, "Read faild with error (" << ec.value() << "): " << ec.message());
            if (_callback.on_read_failed(ec)) {
                receive(ep, true);
            } else {
                do_close();
            }
        } else {
            _INFO(_verbose > 1, "Successfully read " << tbytes << " bytes.");
            // Copy data from to temporary buffer.
            _buffer.commit(tbytes);
            if (_callback.on_read(_buffer, tbytes)) {
                receive(ep, true);
            }
        }
    }

    void session<tcp>::do_write(const boost::asio::const_buffer &&buffer, bool async)
    {
        _INFO(_verbose > 1, "Arrise " << (async ? "an async" : "a sync") << " write action.");
        boost::asio::const_buffer copy_buffer = buffer;
        if (async) {
            this->_socket.async_write_some(buffer,
                asio::bind_executor(this->_strand,
                    [me = this->shared_from_this(), copy_buffer](
                        auto ec, auto tbytes) { me->on_write(copy_buffer, ec, tbytes); }));
        } else {
            error_code ec;
            size_t tbytes = this->_socket.write_some(buffer, ec);
            on_write(std::move(copy_buffer), ec, tbytes);
        }
    }

    void session<udp>::do_write(const boost::asio::const_buffer &&buffer, bool async)
    {
        _INFO(_verbose > 1, "Arrise " << (async ? "an async" : "a sync") << " write action.");
        boost::asio::const_buffer copy_buffer = buffer;
        if (async) {
            this->_socket.async_send(buffer,
                asio::bind_executor(this->_strand,
                    [me = this->shared_from_this(), copy_buffer](
                        auto ec, auto tbytes) { me->on_write(copy_buffer, ec, tbytes); }));
        } else {
            error_code ec;
            size_t tbytes = this->_socket.send(buffer, 0, ec);
            on_write(std::move(copy_buffer), ec, tbytes);
        }
    }

} // namespace beauty
