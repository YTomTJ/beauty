#pragma once

#include <beauty/header.hpp>
#include <beauty/session.hpp>

#include <boost/asio.hpp>

#include <iostream>
#include <functional>

namespace asio = boost::asio;

namespace beauty {

    // --------------------------------------------------------------------------
    class client {
    public:
        client(std::string name = "client")
            : _app(name)
            , _socket(_app.ioc())
        {
        }
        ~client() { stop(); }

        client(const client &) = delete;
        client &operator=(const client &) = delete;

        client(client &&) = default;
        client &operator=(client &&) = default;

        client &connect(int port, std::string addr, const callback &cb = {}, int verbose = 0)
        {
            return connect(port, address_v4::from_string(addr), cb, verbose);
        }

        client &connect(int port, address_v4 addr = {}, const callback &cb = {}, int verbose = 0)
        {
            return connect(endpoint(addr, port), cb, verbose);
        }

        client &connect(endpoint ep, const callback &cb = {}, int verbose = 0)
        {
            try {
                if (!_app.is_started()) {
                    _app.start();
                }

                if (!_session) {
                    // Create the session on first call...
                    _session
                        = std::make_shared<session>(_app.ioc(), std::move(_socket), cb, verbose);
                }

                _session->do_connect(ep);

            } catch (const boost::system::system_error &) {
                _session.reset();

            } catch (const std::exception &) {

                _session.reset();
            }

            return *this;
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

        void stop() { _app.stop(); }
        void run() { _app.run(); }
        void wait() { _app.wait(); }

        const application &app() const { return _app; }

    private:
        application _app;
        std::shared_ptr<session> _session;
        asio::ip::tcp::socket _socket;
    };

} // namespace beauty
