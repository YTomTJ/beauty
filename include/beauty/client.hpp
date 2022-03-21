#pragma once

#include <beauty/header.hpp>
#include <beauty/session.hpp>

#include <boost/asio.hpp>

#include <iostream>
#include <functional>

namespace asio = boost::asio;

namespace beauty {

    // --------------------------------------------------------------------------
    template<typename _Protocol> 
    class client {

        using cb_t = callback<_Protocol>;
        using edp_t = endpoint<_Protocol>;
        using sess_t = session<_Protocol>;

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

        /**
         * @brief Start a connection.
         * @param port Remote endpoint's port.
         * @param addr Remote endpoint's address.
         * @param cb See alse @ref connect.
         * @param verbose See alse @ref connect.
         * @return client&
         */
        client &connect(int port, std::string addr, const cb_t &cb = {}, int verbose = 0)
        {
            return connect(port, address_v4::from_string(addr), cb, verbose);
        }

        /**
         * @brief Start a connection.
         * @param port Remote endpoint's port.
         * @param addr Remote endpoint's address.
         * @param cb See alse @ref connect.
         * @param verbose See alse @ref connect.
         * @return client&
         */
        client &connect(int port, address_v4 addr = {}, const cb_t &cb = {}, int verbose = 0)
        {
            return connect(edp_t(addr, port), cb, verbose);
        }

        /**
         * @brief Start a connection.
         * @param ep Target remote endpoint.
         * @param cb Callback on the connection.
         * @param verbose Verbose for the session of the connection.
         * @return client&
         */
        client &connect(edp_t ep, const cb_t &cb = {}, int verbose = 0)
        {
            try {
                if (!_app.is_started()) {
                    _app.start();
                }
                if (!_session) {
                    _session = std::make_shared<sess_t>( //
                        _app.ioc(), std::move(_socket), cb, verbose);
                }
                _session->connect(ep);

            } catch (const boost::system::system_error &) {
                _session.reset();
            } catch (const std::exception &) {
                _session.reset();
            }
            return *this;
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

        /**
         * @brief Run the application's IO service (blocking).
         */
        void run() { _app.run(); }

        /**
         * @brief Stop the application's IO service.
         */
        void stop() { _app.stop(); }

        /**
         * @brief Wait for the application's IO service (blocking).
         */
        void wait() { _app.wait(); }

        const application &app() const { return _app; }

    private:
        application _app;
        std::shared_ptr<sess_t> _session;
        typename _Protocol::socket _socket;
    };

} // namespace beauty
