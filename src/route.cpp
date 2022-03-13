#include <beauty/route.hpp>
#include <beauty/utils.hpp>

namespace beauty
{
// --------------------------------------------------------------------------
route::route(const std::string& path, route_cb&& cb) :
        _path(path),
        _cb(std::move(cb))
{
    if (path.empty() || path[0] !='/') {
        throw std::runtime_error("Route path [" + path + "] must begin with '/'.");
    }

    for(auto&& p : split(path, '/')) {
        _segments.emplace_back(p);
    }

    extract_route_info();
}

// --------------------------------------------------------------------------
route::route(const std::string& path, const beauty::route_info& route_info, route_cb&& cb) :
        route(path, std::move(cb))
{
    update_route_info(route_info);
}

// --------------------------------------------------------------------------
route::route(const std::string& path, ws_handler&& handler) :
        route(path)
{
    _is_websocket = true;
    _ws_handler = std::move(handler);
}

// --------------------------------------------------------------------------
// Try to extract a maximum of information from the route path
// --------------------------------------------------------------------------
void
route::extract_route_info()
{
    for (auto& segment : _segments) {
        if (segment[0] == ':') {
            _route_info.route_parameters.push_back(
                    beauty::route_parameter{
                            /*.name = */segment.data() + 1,
                            /*.in = */"path",
                            /*.description = */"Undefined",
                            /*.type = */"Undefined",
                            /*.format = */"",
                            /*.required = */ true
                        });
        }
    }
}

// --------------------------------------------------------------------------
// Update the route info automatically generated by the one provide by the user
// --------------------------------------------------------------------------
void
route::update_route_info(const beauty::route_info& route_info)
{
    _route_info.description = route_info.description;

    for (const auto& param : route_info.route_parameters) {
        if (auto found = std::find_if(begin(_route_info.route_parameters),
                                  end(_route_info.route_parameters),
                                  [&param](const auto& info) {
            return (param.name == info.name);
        }); found != end(_route_info.route_parameters)) {
            found->description = param.description;
            found->type = param.type;
            found->format = param.format;
        }
        else {
            // parameter not found, supposed to be a query param
            auto tmp_param = param;
            if (tmp_param.in.empty()) tmp_param.in = "query";
            _route_info.route_parameters.emplace_back(std::move(tmp_param));
        }
    }
}

// --------------------------------------------------------------------------
bool
route::match(beauty::request& req, bool is_websocket) const noexcept
{
    if (_is_websocket != is_websocket) {
        return false;
    }

    // Remove attributes and target split
    auto target_split = split(std::string_view{req.target().data(), req.target().size()}, '?');
    auto request_paths = split(target_split[0], '/');
    std::string attrs = (target_split.size() > 1 ? std::string(target_split[1]): "");

    if (_segments.size() != request_paths.size()) {
        return false;
    }

    for(std::size_t i = 0; i < _segments.size(); ++i) {
        auto& segment = _segments[i];
        auto& request_segment = request_paths[i];

        if (segment[0] == ':') {
            attrs += (attrs.empty() ? "":"&")
                    + std::string(&segment[1], segment.size() - 1)
                    + "="
                    + std::string(request_segment);
        } else if (segment != request_segment) {
            return false;
        }
    }

    if (!attrs.empty()) {
        req.get_attributes() = attributes(attrs);
    }
    return true;
}


}