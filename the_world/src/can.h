#pragma once

#include "common.h"

void can_init();
void can_update();
bool send_can_message(uint32_t can_id, uint8_t* data, size_t len);
