#include <beauty/attributes.hpp>
#include <beauty/utils.hpp>

namespace beauty
{

const attribute attributes::EMPTY;

// --------------------------------------------------------------------------
attributes::attributes(const std::string& str, char sep)
{
    for(auto&& a : split(str, sep)) {
        auto kv = split(a, '=');
        if (kv.size() == 2) {
            insert(std::string(kv[0]), std::string(kv[1]));
        }
    }
}

// --------------------------------------------------------------------------
void
attributes::insert(std::string key, std::string value)
{
    _attributes.emplace(std::move(key), beauty::unescape(value));
}

// --------------------------------------------------------------------------
const attribute&
attributes::operator[](const std::string& key) const
{
    auto found_key = _attributes.find(key);
    if (found_key != _attributes.end()) {
        return found_key->second;
    }

    return EMPTY;
}

}
