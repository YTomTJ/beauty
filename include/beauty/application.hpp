#pragma once

#include <beauty/certificate.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/optional.hpp>

#include <vector>
#include <thread>
#include <optional>
#include <atomic>

namespace asio = boost::asio;

namespace beauty {
class timer;

// --------------------------------------------------------------------------
class application {
public:
    application();
    explicit application(certificates&& c);

    ~application();

    application(const application&) = delete;
    application& operator=(const application&) = delete;
    application(application&&) = delete;
    application& operator=(application&&) = delete;

    // Start the thread pool, running the event loop, not blocking
    void start(int concurrency = 1);

    bool is_started() const { return _state == State::started; }
    bool is_stopped() const { return _state == State::stopped; }

    // Stop the event loop, reset wil cancel all current operations (timers)
    void stop(bool reset = true);

    // Run the event loop in the current thread, blocking
    void run();

    // Wait for the application to be stopped, blocking
    void wait();

    void post(std::function<void()>);

    asio::io_context& ioc() { return _ioc; }
    asio::ssl::context& ssl_context() { return _ssl_context; }
    bool is_ssl_activated() const { return (bool)_certificates; }

    static application& Instance();
    static application& Instance(certificates&& c);

    std::vector<std::shared_ptr<timer>> timers;
        // Need for cancellation, to be improved...

private:
    asio::io_context            _ioc;
    asio::executor_work_guard<asio::io_context::executor_type> _work;
    asio::ssl::context          _ssl_context;
    boost::optional<certificates> _certificates;

    std::vector<std::thread>    _threads;

    enum class State { waiting, started, stopped };
    std::atomic<State> _state{State::waiting}; // Three State allows a good ioc.restart
    std::atomic<int>   _active_threads{0}; // std::barrier in C++20

private:
    void load_server_certificates();
};

// --------------------------------------------------------------------------
// Singleton direct access
// --------------------------------------------------------------------------
void start(int concurrency = 1);
bool is_started();
void run();
void wait();
void stop();
void post(std::function<void()>);
}
