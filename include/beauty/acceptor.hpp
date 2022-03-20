#pragma once

#include <beauty/header.hpp>
#include <beauty/application.hpp>
#include <beauty/session.hpp>

#include <boost/asio.hpp>

#include <memory>

namespace asio = boost::asio;

namespace beauty {

    //---------------------------------------------------------------------------
    // Accepts incoming connections and launches the sessions
    //---------------------------------------------------------------------------
    class acceptor : public std::enable_shared_from_this<acceptor> {
    public:
        acceptor(application &app, const endpoint &endpoint, const callback &cb, int verbose)
            : _app(app)
            , _acceptor(app.ioc())
            , _socket(app.ioc())
            , _callback(std::move(cb))
            , _verbose(verbose)
        {
            // NOTE: on_disconnected event will be replaced.
            _callback.on_disconnected = [this](beauty::endpoint ep) {
                this->_session.reset();
                // Accept another connection on failed.
                do_accept();
            };

            boost::system::error_code ec;

            // Open the acceptor
            _acceptor.open(endpoint.protocol(), ec);
            assert(!ec);

            // Bind to the server address
            _acceptor.bind(endpoint, ec);
            assert(!ec);

            // Start listening for connections
            _acceptor.listen(asio::socket_base::max_listen_connections, ec);
            assert(!ec);

            this->run();
        }

        ~acceptor() { stop(); }

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

        template <typename T>
        void write(const T &data, bool async)
        {
            if (_session)
                _session->write(data, async);
        }

        void read(bool async)
        {
            if (_session)
                _session->read(async);
        }

        void do_accept()
        {
            auto ep = _acceptor.local_endpoint();
            _INFO(_verbose > 1, "Start acception on " << ep);
            _acceptor.async_accept(_socket, [this](auto ec) { this->on_accept(ec); });
        }

    protected:
        void on_accept(error_code ec)
        {
            error_code ecx;
            auto ep = _acceptor.local_endpoint();
            auto epr = _socket.remote_endpoint(ecx);

            if (ec == boost::system::errc::operation_canceled) {
                _INFO(_verbose > 1,
                    "Acception on " << ep << " canceled (" << ec.value() << "): " << ec.message());
                return; // Nothing to do anymore
            }

            _INFO(_verbose > 1, "Acception connection from " << epr);
            _callback.on_accepted(ep);

            if (ec) {
                _ERROR(_verbose > 0,
                    "Acception on " << ep << " faild with error (" << ec.value()
                                    << "): " << ec.message());
                _app.stop();
            } else {

                try {
                    if (!_app.is_started()) {
                        _app.start();
                    }

                    if (!_session) {
                        _INFO(_verbose > 1, "Make session on " << ep << " for " << epr);
                        _session = std::make_shared<session>(
                            _app.ioc(), std::move(_socket), _callback, _verbose);
                    }

                    _session->read(true);
                    // Return on connection succeeded.
                    return;

                } catch (const boost::system::system_error &) {
                    _session.reset();

                } catch (const std::exception &) {

                    _session.reset();
                }
            }

            // Accept another connection on failed.
            if (_session) {
                do_accept();
            }
        }

    private:
        application &_app;
        asio::ip::tcp::acceptor _acceptor;
        asio::ip::tcp::socket _socket;
        std::shared_ptr<session> _session;
        callback _callback;
        const int _verbose;
    };

} // namespace beauty
