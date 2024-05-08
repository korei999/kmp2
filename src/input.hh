#pragma once
#include "app.hh"

namespace input
{

void read(app::PipeWirePlayer* app);
/* first char as prefix while echoing current input */
void readWStringEcho(wint_t* wb, char firstChar, int n);

} /* namespace input */
