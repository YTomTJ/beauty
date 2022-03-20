#pragma once

#include <beauty/header.hpp>

#include <boost/asio.hpp>
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
        application(std::string name)
            : _name(name)
            , _work(asio::make_work_guard(_ioc))
            , _state(State::waiting)
        {
        }
        ~application() { stop(); }

        application(const application &) = delete;
        application &operator=(const application &) = delete;
        application(application &&) = delete;
        application &operator=(application &&) = delete;

        // Start the thread pool, running the event loop, not blocking
        void start(int concurrency = 1)
        {
            // Prevent to run twice
            if (is_started()) {
                return;
            }

            if (is_stopped()) {
                // The application was started before, we need
                // to restart the ioc cleanly
                _ioc.restart();
            }
            _state = State::started;

            // Run the I/O service on the requested number of threads
            _threads.resize(std::max(1, concurrency));
            _active_threads = 0;
            for (auto &t : _threads) {
                ++_active_threads;
                t = std::thread([this] {
#ifdef USING_LOGURU
                    loguru::set_thread_name(_name.c_str());
#endif
                    for (;;) {
                        try {
                            _ioc.run();
                            break;
                        } catch (const std::exception &ex) {
                            _ERROR(true, "worker error: " << ex.what());
                        }
                    }
                    --_active_threads;
                });
                t.detach();
                // Threads are detached, it's easier to stop inside an handler
            }
        }

        // Stop the event loop.
        void stop()
        {
            if (is_stopped()) {
                return;
            }
            _state = State::stopped;

            _ioc.stop();

            while (_active_threads != 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        // Run the event loop in the current thread, blocking
        void run()
        {
            if (is_stopped()) {
                _ioc.restart();
            }
            _state = State::started;

            // Run
            _ioc.run();
        }

        // Wait for the application to be stopped, blocking
        void wait()
        {
            while (!is_stopped()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
        }

        void post(std::function<void()> fct)
        {
            boost::asio::post(_ioc.get_executor(), std::move(fct));
        }

        bool is_started() const { return _state == State::started; }
        bool is_stopped() const { return _state == State::stopped; }
        size_t active_threads() const { return _active_threads; }
        asio::io_context &ioc() { return _ioc; }

    private:
        const std::string _name;

        asio::io_context _ioc;
        asio::executor_work_guard<asio::io_context::executor_type> _work;

        std::vector<std::thread> _threads;

        enum class State { waiting, started, stopped };
        std::atomic<State> _state{ State::waiting }; // Three State allows a good ioc.restart
        std::atomic<int> _active_threads{ 0 }; // std::barrier in C++20
    };

} // namespace beauty
