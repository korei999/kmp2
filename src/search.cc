#include "search.hh"
#include "utils.hh"

#include <codecvt>

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
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    for (int i = start; i != cond; i += inc)
    {
        std::string s = utils::removePath(a[i]);

        /*  https://stackoverflow.com/questions/2573834/c-convert-string-or-char-to-wstring-or-wchar-t */
        std::wstring sw(a[i].size(), L' ');
        sw.resize(std::mbstowcs(&sw[0], s.data(), s.size()));

        std::wstring fw(key.begin(), key.end());
        f.toupper(&sw[0], &sw[0] + sw.size());
        f.toupper(&fw[0], &fw[0] + fw.size());

        if (sw.find(fw) != std::string::npos)
            ret.push_back(i);
    }

    return ret;
}

} /* namespace search */
