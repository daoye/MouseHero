#pragma once

#include "ya_event.h"

/**
 * Auth client
 *
 * */
bool authorize(const YAAuthorizeEventRequest *param);

const char *get_authorize_error();
