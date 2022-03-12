#include "beyond_tizen_internal.h"
#include <cerrno>
#include <cstring>

void beyond_tizen_handle_init(void *handle)
{
    beyond_tizen_handle *_handle = static_cast<beyond_tizen_handle *>(handle);
    memcpy(_handle->magic, BEYOND_TIZEN_HANDLE_MAGIC, BEYOND_TIZEN_HANDLE_MAGIC_LEN);
}

void beyond_tizen_handle_deinit(void *handle)
{
    beyond_tizen_handle *_handle = static_cast<beyond_tizen_handle *>(handle);
    memcpy(_handle->magic, BEYOND_TIZEN_HANDLE_MAGIC_FREE, BEYOND_TIZEN_HANDLE_MAGIC_LEN);
}

int beyond_tizen_handle_is_valid(void *handle)
{
    if (handle == nullptr) {
        return 0;
    }

    beyond_tizen_handle *_handle = static_cast<beyond_tizen_handle *>(handle);
    return !memcmp(_handle->magic, BEYOND_TIZEN_HANDLE_MAGIC, BEYOND_TIZEN_HANDLE_MAGIC_LEN);
}

int beyond_tizen_handle_set_handle(void *handle, void *_beyond_handle)
{
    beyond_tizen_handle *_handle = static_cast<beyond_tizen_handle *>(handle);
    if (beyond_tizen_handle_is_valid(handle) == 0) {
        return -EINVAL;
    }

    _handle->handle = _beyond_handle;
    return 0;
}
