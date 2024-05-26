#include "search.hh"
#include "utils.hh"

namespace search
{

std::vector<int>
getIndexList(const std::vector<std::string>& a, std::wstring_view key, enum dir direction)
{
    std::vector<int> ret {};
    int inc, start, doneCnd;

    if (direction == dir::forward)
        inc = 1, start = 0, doneCnd = a.size();
    else
        inc = -1, start = a.size() -1, doneCnd = -1;

    const auto& f = std::use_facet<std::ctype<wchar_t>>(std::locale());

    std::wstring fw(key.begin(), key.end());
    f.toupper(&fw[0], &fw[0] + fw.size());

    for (int i = start; i != doneCnd; i += inc)
    {
        std::string s = utils::removePath(a[i]);

        std::wstring sw(a[i].size(), L'\0');
        std::mbstate_t state = std::mbstate_t();
        const char* mbstr = s.data();
        size_t swSize = std::mbsrtowcs(&sw[0], &mbstr, s.size(), &state);
        sw.resize(swSize);

        f.toupper(&sw[0], &sw[0] + sw.size());

        if (sw.find(fw) != std::string::npos)
            ret.push_back(i);
    }

    return ret;
}

} /* namespace search */
