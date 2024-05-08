#pragma once
#include "app.hh"

namespace input
{

void read(app::PipeWirePlayer* app);
void readWString(std::wstring_view prefix, wint_t* pBuff, int buffSize);

} /* namespace input */
