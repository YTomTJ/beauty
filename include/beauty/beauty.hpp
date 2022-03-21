#pragma once

#include <beauty/acceptor.hpp>
#include <beauty/application.hpp>
#include <beauty/client.hpp>
#include <beauty/header.hpp>
#include <beauty/server.hpp>
#include <beauty/session.hpp>

namespace beauty {

    // TCP interface
    using tcp_endpoint = endpoint<tcp>;
    using tcp_callback = callback<tcp>;
    using tcp_client = client<tcp>;

    // UDP interface
    using udp_endpoint = endpoint<udp>;
    using udp_callback = callback<udp>;
    using udp_client = client<udp>;

} // namespace beauty
