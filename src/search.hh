#pragma once

#include <string>
#include <vector>

namespace search
{

enum class dir : int
{
    forward,
    backwards
};

std::vector<int> getIndexList(const std::vector<std::string>& a, std::string_view key, enum dir direction);

} /* namespace search */
