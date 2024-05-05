#include "search.hh"
#include "utils.hh"

namespace search
{

std::vector<int>
getIndexList(const std::vector<std::string>& a, std::string_view key, enum dir direction)
{
    std::vector<int> ret {};

    int inc = direction == dir::forward ? 1 : -1;
    int start = direction == dir::forward ? 0 : a.size() - 1;
    int cond = direction == dir::forward ? a.size() : -1;

    for (int i = start; i != cond; i += inc)
    {
        auto s = utils::removePath(a[i]);

        if (s.find(key) != std::string::npos)
            ret.push_back(i);
    }

    return ret;
}

} /* namespace search */
