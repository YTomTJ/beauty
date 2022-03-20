#pragma once

#include <beauty/header.hpp>
#include <beauty/application.hpp>
#include <beauty/session.hpp>

#include <boost/asio.hpp>

#include <memory>

namespace asio = boost::asio;

namespace beauty {

    using endpoint = endpoint;

    //---------------------------------------------------------------------------
    // Accepts incoming connections and launches the sessions
    //---------------------------------------------------------------------------
    class acceptor : public std::enable_shared_from_this<acceptor> {
    public:
        acceptor(application &app, endpoint &endpoint, const callback &cb)
            : _app(app)
            , _acceptor(app.ioc())
            , _socket(app.ioc())
            , _callback(cb)
        {
            boost::system::error_code ec;

            // Open the acceptor
            _acceptor.open(endpoint.protocol(), ec);
            if (ec) {
                _app.stop();
                return;
            }

            // Allow address reuse
            _acceptor.set_option(asio::socket_base::reuse_address(true));
            if (ec) {
                _app.stop();
                return;
            }

            // Bind to the server address
            _acceptor.bind(endpoint, ec);
            if (ec) {
                _app.stop();
                return;
            }

            // Update server endpoint in case of dynamic port allocation
            endpoint.port(_acceptor.local_endpoint().port());

            // Start listening for connections
            _acceptor.listen(asio::socket_base::max_listen_connections, ec);
            if (ec) {
                _app.stop();
                return;
            }
        }

        ~acceptor() { stop(); }

        template <typename T>
        void write(T data, bool async)
        {
            if (_session)
                _session->write(data, async);
        }

        void read(bool async)
        {
            if (_session)
                _session->read(async);
        }

        void run()
        {
            if (!_acceptor.is_open()) {
                return;
            }
            do_accept();
        }

        void stop()
        {
            if (_acceptor.is_open()) {
                _acceptor.close();
            }
        }

        void do_accept()
        {
            _acceptor.async_accept(
                _socket, [me = shared_from_this()](auto ec) { me->on_accept(ec); });
        }

    protected:
        void on_accept(error_code ec)
        {
            if (ec == boost::system::errc::operation_canceled) {
                return; // Nothing to do anymore
            }

            error_code ecx;
            endpoint epx = _socket.remote_endpoint(ecx);
            _callback.on_accepted(epx);

            if (ec) {
                _app.stop();
            } else {

                try {
                    if (!_app.is_started()) {
                        _app.start();
                    }

                    if (!_session) {
                        // Create the session on first call...
                        _session
                            = std::make_shared<session>(_app.ioc(), std::move(_socket), _callback);
                    }

                    _session->read(true);

                } catch (const boost::system::system_error &) {
                    _session.reset();

                } catch (const std::exception &) {

                    _session.reset();
                }
            }

            // Accept another connection
            do_accept();
        }

    private:
        application &_app;
        asio::ip::tcp::acceptor _acceptor;
        asio::ip::tcp::socket _socket;
        std::shared_ptr<session> _session;
        const callback &_callback;
    };

} // namespace beauty
