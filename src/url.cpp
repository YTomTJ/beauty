#include <beauty/url.hpp>

#include <beauty/utils.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <charconv>

namespace {
// --------------------------------------------------------------------------
std::pair<boost::string_view, boost::string_view>
split_pair(const boost::string_view& view, char sep, bool mandatory_left = true)
{
    std::pair<boost::string_view, boost::string_view> tmp;

    auto pair_split = beauty::split(view, sep);
    if (pair_split.size() == 1) {
        (mandatory_left ? tmp.first : tmp.second) = pair_split[0];
    } else if (pair_split.size() == 2) {
        tmp.first   = pair_split[0];
        tmp.second  = pair_split[1];
    } else {
        throw std::runtime_error("Invalid URL format for [" + std::string(view) + "]");
    }

    return tmp;
}
}

namespace beauty
{
// --------------------------------------------------------------------------
url::url(std::string u) : _url(std::move(u))
{
    //    0      1                 2 = host             3 = path
    // <scheme>://[[login][:password]@]<host>[:port][/path][?query]
    auto url_split = beauty::split(_url, '/');

    if (url_split.size() < 3 ||
            url_split[1].size() ||
            url_split[2].empty()) {
        throw std::runtime_error("Invalid URL format for [" + _url + "]");
    }

    // <scheme>:
    _scheme = url_split[0];
    if (!_scheme.empty() && *_scheme.rbegin() == ':') {
        _scheme.remove_suffix(1);
    }

    // [[login][:password]@]<host>[:port]
    auto _pair = split_pair(url_split[2], '@', false);
    auto user_info = _pair.first;
    auto host = _pair.second;
    if (user_info.size()) {
        std::tie(_login, _password) = split_pair(user_info, ':');
    }
    if (host.size()) {
        std::tie(_host, _port_view) = split_pair(host, ':');
    }
    if (_port_view.size()) {
        //auto _res /*[p, ec]*/= std::from_chars(&_port_view[0],&_port_view[0] + _port_view.size(), _port);
        //if (_res.second != std::errc()) {
        //    throw std::runtime_error("Invalid port number " + std::string(_port_view));
        //}
        try {
            _port = boost::lexical_cast<decltype(_port)>(_port_view);
        } catch (const boost::bad_lexical_cast&) {
            throw std::runtime_error("Invalid port number " + std::string(_port_view));
        }
    }

    // [/path][?query]
    if (url_split.size() >= 3) {
        // Compute the start of the path
        std::size_t pos = url_split[0].size() + 1
                + url_split[1].size() + 1
                + url_split[2].size();
        // Find the start of the query '?'
        auto found_query = _url.find('?', pos);
        if (found_query != std::string::npos) {
            _path = boost::string_view(&_url[pos], found_query - pos);
            _query = boost::string_view(&_url[found_query]);
        } else {
            _path = boost::string_view(&_url[pos]);
        }
    }
}

// --------------------------------------------------------------------------
std::string
url::strip_login_password() const {
    return scheme() + "://" + host()
           + (port() ? ":" + std::to_string(port()) : "")
           + path();
}

}
