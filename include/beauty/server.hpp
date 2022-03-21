#pragma once

#include <beauty/header.hpp>
#include <beauty/application.hpp>
#include <beauty/acceptor.hpp>

#include <string>

namespace beauty {

    // --------------------------------------------------------------------------
    class tcp_server {

        using _Protocol = tcp;
        using cb_t = callback<_Protocol>;
        using edp_t = endpoint<_Protocol>;
        using sess_t = session<_Protocol>;
        using accep_t = acceptor;

    public:
        tcp_server(std::string name = "tcp_server")
            : _app(name)
        {
        }
        ~tcp_server() { stop(); }

        tcp_server(const tcp_server &) = delete;
        tcp_server &operator=(const tcp_server &) = delete;

        tcp_server(tcp_server &&) = default;
        tcp_server &operator=(tcp_server &&) = default;

        tcp_server &concurrency(int concurrency)
        {
            _concurrency = concurrency;
            return *this;
        }

        /**
         * @brief Litsen on target local port.
         * @param port Local listening endpoint's port.
         * @param cb Callback on the connection.
         * @param verbose Verbose for the session of the connection.
         * @return client&
         */
        const std::shared_ptr<accep_t> &listen(int port, const cb_t &cb, int verbose = 0)
        {
            if (!_app.is_started()) {
                _app.start(_concurrency);
            }
            auto ep = edp_t(address_v4(), port);
            _acceptors.emplace(port, std::make_shared<accep_t>(_app, ep, cb, verbose));
            return _acceptors.at(port);
        }

        /**
         * @brief Run all acceptions and the applition's IO service (blocking).
         */
        void run()
        {
            for (auto &actp : _acceptors) {
                if (actp.second) {
                    actp.second->run();
                }
            }
            _app.run();
        }

        /**
         * @brief Stop all acceptions and the applition's IO service.
         */
        void stop()
        {
            for (auto &actp : _acceptors) {
                if (actp.second) {
                    actp.second->stop();
                }
            }
            _app.stop();
        }

        /**
         * @brief Wait for the applition's IO service (blocking).
         */
        void wait() { _app.wait(); }

        /**
         * @brief Access the acceptor on target port.
         * @param port Local listening endpoint's port.
         * @return const std::shared_ptr<acceptor>&
         */
        const std::shared_ptr<accep_t> &get_acceptor(int port) const
        {
            assert(_acceptors.find(port) != _acceptors.end());
            return _acceptors.at(port);
        }

        const std::vector<int> &get_ports() const
        {
            std::vector<int> keys;
            std::transform(_acceptors.begin(), _acceptors.end(), std::back_inserter(keys),
                [](auto &pair) { return pair.first; });
            return std::move(keys);
        }

    private:
        application _app;
        int _concurrency = 1;
        cb_t _callback;
        std::map<int, std::shared_ptr<accep_t>> _acceptors;
    };

} // namespace beauty
