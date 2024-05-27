#pragma once

#include <pipewire/pipewire.h>

namespace play
{

void onProcessCB(void* data);
void stateChangedCB(void* data, enum pw_stream_state old, enum pw_stream_state state, const char* error);
void ioChangedCB(void* data, uint32_t id, void* area, uint32_t size);
void paramChangedCB(void* data, uint32_t id, const spa_pod* param);

} /* namespace play */
