#include "ya_authorize.h"
#include "ya_utils.h"
#include <stdbool.h>
#include <string.h>

static __thread char *AUTHORIZE_ERROR = NULL;

bool authorize(const YAAuthorizeEventRequest *param)
{
    if (AUTHORIZE_ERROR)
    {
        safe_free((void **)&AUTHORIZE_ERROR);
    }

    if (!param)
    {
        AUTHORIZE_ERROR = strdup("Unknown error");
        return false;
    }

    if (param->version < 1)
    {
        AUTHORIZE_ERROR = strdup("Client version too low, upgrade please.");
        return false;
    }

    // Validate client type
    if (param->type != CLIENT_IOS && param->type != CLIENT_ANDROID)
    {
        AUTHORIZE_ERROR = strdup("Invalid client type");
        return false;
    }

    AUTHORIZE_ERROR = NULL;
    return true;
}

const char *get_authorize_error()
{
    return AUTHORIZE_ERROR;
}
