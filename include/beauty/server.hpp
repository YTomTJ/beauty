#pragma once

#include <beauty/header.hpp>
#include <beauty/application.hpp>
#include <beauty/acceptor.hpp>

#include <string>

namespace beauty {

    // --------------------------------------------------------------------------
    class server {
    public:
        server(std::string name = "server")
            : _app(name)
        {
        }
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

        const std::shared_ptr<acceptor> &listen(
            int port, std::string addr, const callback &cb, int verbose = 0)
        {
            return listen(port, cb, address_v4::from_string(addr), verbose);
        }

        const std::shared_ptr<acceptor> &listen(
            int port, const callback &cb, address_v4 addr = {}, int verbose = 0)
        {
            return listen(endpoint(addr, port), cb, verbose);
        }

        const std::shared_ptr<acceptor> &listen(endpoint ep, const callback &cb, int verbose = 0)
        {
            if (!_app.is_started()) {
                _app.start(_concurrency);
            }
            _acceptors.emplace(ep, std::make_shared<acceptor>(_app, ep, cb, verbose));
            return _acceptors.at(ep);
        }

        void stop()
        {
            for (auto &actp : _acceptors) {
                if (actp.second) {
                    actp.second->stop();
                }
            }
            _app.stop();
        }

        void run()
        {
            for (auto &actp : _acceptors) {
                if (actp.second) {
                    actp.second->run();
                }
            }
            _app.run();
        }

        void wait() { _app.wait(); }

        const std::shared_ptr<acceptor> &get_acceptor(int port, std::string addr) const
        {
            return get_acceptor(port, address_v4::from_string(addr));
        }

        const std::shared_ptr<acceptor> &get_acceptor(int port, address_v4 addr = {}) const
        {
            return get_acceptor(endpoint(addr, port));
        }

        const std::shared_ptr<acceptor> &get_acceptor(endpoint ep) const
        {
            assert(_acceptors.find(ep) != _acceptors.end());
            return _acceptors.at(ep);
        }

        const std::vector<endpoint> &get_endpoints() const
        {
            std::vector<endpoint> keys;
            std::transform(_acceptors.begin(), _acceptors.end(), std::back_inserter(keys),
                [](auto &pair) { return pair.first; });
            return std::move(keys);
        }

    private:
        application _app;
        int _concurrency = 1;
        callback _callback;

        std::map<endpoint, std::shared_ptr<acceptor>> _acceptors;
    };

} // namespace beauty
