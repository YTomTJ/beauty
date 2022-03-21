# Readme
This is a simple Http server and client above <a href="https://github.com/boostorg/asio">Boost.Asio</a>. The implement is somewhat a simplified version of <a href="https://github.com/dfleury2/beauty">Beauty</a>, only with basic buffer transfer. This project is design with event-driven high-level interface.

## Features
- Header only
- Synchronous or Asynchronous API
- Event-driven high-level interface

## Examples

- a server

```cpp
#include "beauty/beauty.hpp"

int main()
{
    beauty::tcp_server server;

    beauty::tcp_callback cb;
    cb.on_read = [&server](const beauty::buffer_type &data) {
        return true; // return `true` to continue next read, only when connected.
    };

    server.concurrency(1); // using 1 worker
    server.listen(5580, cb, 2); // listen on local port

    // Async writing data to any connected remote client.
    const std::string str = "Hello world!";
    while (true) {
        server.get_acceptor(5580)->write(str, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return 0;
}

```
- a client

```cpp
#include "beauty/beauty.hpp"

int main()
{
    beauty::tcp_client client;
    beauty::tcp_callback cb;

    std::string str = "HEARTBEAT";
    size_t write_trial = 0;

    cb.on_connected = [&client, str, &write_trial](beauty::tcp_endpoint ep) {
        client.write(str, true); // start write
        client.read(true); // start read
        write_trial = 0;
    };
    cb.on_connect_failed = [&client](auto, auto) {
        return true; // return `true` to connect same endpoint angain on failed.
    };
    cb.on_disconnected = [&client](beauty::tcp_endpoint) {
        client.connect(5580, "127.0.0.1"); // try connect again.
    };
    cb.on_read = [&client](const beauty::buffer_type &data) { //
        return true; // return `true` to continue next read,  only when connected.
    };
    cb.on_write = [&client, str](const size_t size) { //
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        client.write(str, true); // continue next write, only when connected.
    };
    cb.on_write_failed = [&client, &write_trial](beauty::error_code) { //
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        write_trial++; // writing trial on failed. but only when 
        return write_trial < 3;
    };

    client.connect(5580, "127.0.0.1", cb, 2);
    client.wait();
    return 0;
}


```
