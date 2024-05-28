#pragma once
#include "app.hh"

namespace mpris
{

void init(app::PipeWirePlayer* p);
void process(app::PipeWirePlayer* p);
void clean();

} /* namespace mpris */
