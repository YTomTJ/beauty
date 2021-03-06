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

        using _Protocol = tcp;
        using cb_t = callback<_Protocol>;
        using edp_t = endpoint<_Protocol>;
        using sess_t = session<_Protocol>;

    public:
        acceptor(application &app, const edp_t &endpoint, const cb_t &cb, int verbose)
            : _app(app)
            , _endpoint(endpoint)
            , _acceptor(app.ioc())
            , _socket(app.ioc())
            , _callback(cb)
            , _verbose(verbose)
        {
            // NOTE: on_disconnected event will be replaced.
            _on_disconnected = _callback.on_disconnected;
            _callback.on_disconnected = [this](sess_t &sess, edp_t ep) {
                _on_disconnected(sess, ep);
                // Release the session.
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

        /**
         * @brief Write some data.
         * @tparam T Buffer template type. Supported types see @ref session.
         * @param data The writing buffer.
         * @param async If using async writing mode.
         */
        template <typename T>
        void write(const T &data, bool async)
        {
            if (_session)
                _session->write(data, async);
        }

        /**
         * @brief Start a read action.
         * @param async If using async reading mode.
         */
        void read(bool async)
        {
            if (_session)
                _session->read(async);
        }

        void do_accept()
        {
            BEAUTY_INFO(true, "Start acception on " << _endpoint);
            _acceptor.async_accept(_socket, [this](auto ec) { this->on_accept(ec); });
        }

        const edp_t get_endpoint() const { return _endpoint; };

    protected:
        void on_accept(error_code ec)
        {
            error_code ecx;
            auto ep = _acceptor.local_endpoint(ecx);
            auto epr = _socket.remote_endpoint(ecx);

            if (ec == boost::system::errc::operation_canceled) {
                BEAUTY_INFO(_verbose > 0,
                    "Acception on " << ep << " canceled (" << ec.value() << "): " << ec.message());
                return; // Nothing to do anymore
            }

            BEAUTY_INFO(true, "Accepted connection from " << epr);
            _callback.on_accepted(*this, ep, epr);

            if (ec) {
                BEAUTY_ERROR(_verbose > 0,
                    "Acception on " << ep << " faild with error (" << ec.value()
                                    << "): " << ec.message());
                _app.stop();
            } else {

                try {
                    if (!_app.is_started()) {
                        _app.start();
                    }

                    if (!_session) {
                        BEAUTY_INFO(_verbose > 0, "Make session on " << ep << " for " << epr);
                        _session = std::make_shared<sess_t>(
                            _app.ioc(), std::move(_socket), _callback, _verbose);
                    }

                    _session->_is_connnected = true;
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
        tcp::acceptor _acceptor;
        tcp::socket _socket;
        const edp_t _endpoint;
        std::shared_ptr<sess_t> _session;
        cb_t _callback;
        const int _verbose;
        std::function<void(sess_t &, edp_t)> _on_disconnected;
    };

} // namespace beauty
