#include "search.hh"
#include "utils.hh"

#include <unicode/unistr.h>

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
        std::string s(a.size(), 0);
        std::string k(key.size(), 0);

        icu::UnicodeString ss = utils::removePath(a[i].data()).data();
        ss.toUpper();
        icu::UnicodeString ff = key.data();
        ff.toUpper();
        ss.extract(0, ss.length(), s.data());
        ff.extract(0, ff.length(), k.data());

        if (s.find(k) != std::string::npos)
            ret.push_back(i);
    }

    return ret;
}

} /* namespace search */
