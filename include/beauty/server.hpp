#pragma once

#include <beauty/header.hpp>
#include <beauty/application.hpp>
#include <beauty/acceptor.hpp>

#include <string>

namespace beauty {

    // --------------------------------------------------------------------------
    class server {
    public:
        server() = default;
        ~server() { stop(); }

        server(const server &) = delete;
        server &operator=(const server &) = delete;

        server(server &&) = default;
        server &operator=(server &&) = default;

        server &concurrency(int concurrency)
        {
            _concurrency = concurrency;
            return *this;
        }

        server &listen(int port, std::string addr = "")
        {
            return listen(port, address_v4::from_string(addr));
        }

        server &listen(int port, address_v4 addr = {}) { return listen(endpoint(addr, port)); }

        server &listen(endpoint ep)
        {
            if (!_app.is_started()) {
                _app.start(_concurrency);
            }
            _endpoint = std::move(ep);
            // Create and launch a listening port
            _acceptor = std::make_shared<acceptor>(_app, _endpoint, _callback);
            _acceptor->run();

            return *this;
        }

        template <typename T>
        void write(T data, bool async)
        {
            if (_acceptor)
                _acceptor->write(data, async);
        }

        void read(bool async)
        {
            if (_acceptor)
                _acceptor->read(async);
        }

        void stop()
        {
            if (_acceptor) {
                _acceptor->stop();
            }
            _app.stop();
        }

        void run() { _app.run(); }

        void wait() { _app.wait(); }

        server &set_callback(const callback &cb)
        {
            _callback = std::move(cb);
            return *this;
        }

        const callback &get_callback() const noexcept { return _callback; }

        const endpoint &get_endpoint() const { return _endpoint; }
        const int port() const { return _endpoint.port(); }

    private:
        application _app;
        int _concurrency = 1;
        callback _callback;
        std::shared_ptr<acceptor> _acceptor;
        endpoint _endpoint;
    };

} // namespace beauty
