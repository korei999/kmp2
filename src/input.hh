#pragma once
#include "app.hh"

#include <optional>

namespace input
{

void read(app::PipeWirePlayer* p);
void readWString(std::wstring_view prefix, wint_t* pBuff, int buffSize);
std::optional<u64> parseTimeString(std::wstring_view ts, app::PipeWirePlayer* p);

} /* namespace input */
