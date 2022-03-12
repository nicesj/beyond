#include "beyond_generic_internal.h"
#include <cerrno>
#include <cstring>

void beyond_generic_handle_init(void *handle)
{
    beyond_generic_handle *_handle = static_cast<beyond_generic_handle *>(handle);
    memcpy(_handle->magic, BEYOND_GENERIC_HANDLE_MAGIC, BEYOND_GENERIC_HANDLE_MAGIC_LEN);
}

void beyond_generic_handle_deinit(void *handle)
{
    beyond_generic_handle *_handle = static_cast<beyond_generic_handle *>(handle);
    memcpy(_handle->magic, BEYOND_GENERIC_HANDLE_MAGIC_FREE, BEYOND_GENERIC_HANDLE_MAGIC_LEN);
}

int beyond_generic_handle_is_valid(void *handle)
{
    if (handle == nullptr) {
        return 0;
    }

    beyond_generic_handle *_handle = static_cast<beyond_generic_handle *>(handle);
    return !memcmp(_handle->magic, BEYOND_GENERIC_HANDLE_MAGIC, BEYOND_GENERIC_HANDLE_MAGIC_LEN);
}

int beyond_generic_handle_set_handle(void *handle, void *_beyond_handle)
{
    beyond_generic_handle *_handle = static_cast<beyond_generic_handle *>(handle);
    if (beyond_generic_handle_is_valid(handle) == 0) {
        return -EINVAL;
    }

    _handle->handle = _beyond_handle;
    return 0;
}
