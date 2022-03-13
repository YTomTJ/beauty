#pragma once

#include <string>
#include <memory>

namespace beauty {
class websocket_session;

// --------------------------------------------------------------------------
struct ws_context {
    std::string uuid;
    std::weak_ptr<websocket_session> ws_session;

    std::string target;
    beauty::attributes attributes;

    std::string route_path;
};

}
