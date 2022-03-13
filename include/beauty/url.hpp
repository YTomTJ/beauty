#pragma once

#include <string>
#include <boost/utility/string_view.hpp>

namespace beauty
{
// --------------------------------------------------------------------------
// Almost like this https://en.wikipedia.org/wiki/URL
// <scheme>://[[login][:password]@]<host>[:port][/path][?query]
// Examples:
//    http://localhost.com
//    http://localhost.com/path
//    http://localhost.com/path?query=yes
//    http://localhost.com:8085/path?query=yes
//    http://login@localhost.com/path
//    http://login:pwd@localhost.com/path
// --------------------------------------------------------------------------
class url
{
public:
    url() = default;
    explicit url(std::string u);

    boost::string_view scheme_view() const { return _scheme; }
    std::string scheme() const { return std::string(_scheme); }
    bool is_http() const { return _scheme == "http"; }
    bool is_https() const { return _scheme == "https"; }

    boost::string_view login_view() const { return _login; }
    std::string login() const { return std::string(_login); }
    boost::string_view password_view() const { return _password; }
    std::string password() const { return std::string(_password); }

    boost::string_view host_view() const { return _host; }
    std::string host() const { return std::string(_host); }
    boost::string_view port_view() const { return _port_view; }
    unsigned short   port() const { return _port; }

    boost::string_view path_view() const { return (_path.size() ? _path : "/"); }
    std::string path() const { return std::string(_path.size() ? _path : "/"); }
    boost::string_view query_view() const { return _query; }
    std::string query() const { return std::string(_query); }

    std::string strip_login_password() const;

private:
    // Full input url for the string_view
    std::string _url;

    // Http or https
    boost::string_view    _scheme;
    // User Info
    boost::string_view    _login;
    boost::string_view    _password;

    boost::string_view    _host;
    boost::string_view    _port_view;
    unsigned short      _port{0};

    boost::string_view    _path;
    boost::string_view    _query;
};

}
