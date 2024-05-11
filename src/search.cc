#include "search.hh"
#include "utils.hh"

namespace search
{

std::vector<int>
getIndexList(const std::vector<std::string>& a, std::wstring_view key, enum dir direction)
{
    std::vector<int> ret {};

    int inc = direction == dir::forward ? 1 : -1;
    int start = direction == dir::forward ? 0 : a.size() - 1;
    int cond = direction == dir::forward ? a.size() : -1;

    const auto& f = std::use_facet<std::ctype<wchar_t>>(std::locale());

    for (int i = start; i != cond; i += inc)
    {
        std::string s = utils::removePath(a[i]);

        std::wstring sw(a[i].size(), L'\0');
        std::mbstate_t state = std::mbstate_t();
        const char* mbstr = s.data();
        size_t swSize = std::mbsrtowcs(&sw[0], &mbstr, s.size(), &state);
        sw.resize(swSize);

        std::wstring fw(key.begin(), key.end());

        f.toupper(&sw[0], &sw[0] + sw.size());
        f.toupper(&fw[0], &fw[0] + fw.size());

        if (sw.find(fw) != std::string::npos)
            ret.push_back(i);
    }

    return ret;
}

} /* namespace search */
