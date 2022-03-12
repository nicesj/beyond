#ifndef ANDROID
#ifndef NDEBUG

#include "authenticator.h"

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include <cstdio>
#include <cerrno>

#include <string>

#define DATA_DIR "/tmp/"

void Authenticator::DumpToFile(const char *filename, const void *buffer, size_t size)
{
    std::string abspath(DATA_DIR);
    FILE *fp;

    if (authenticator == nullptr) {
        abspath += std::string("root.");
    }

    abspath += std::string(filename);
    fp = fopen(abspath.c_str(), "w+");
    if (fp == nullptr) {
        ErrPrintCode(errno, "fopen");
        return;
    }

    if (fwrite(buffer, 1, size, fp) != size) {
        ErrPrintCode(errno, "fwrite");
        if (fclose(fp) < 0) {
            ErrPrintCode(errno, "fclose");
        }
        return;
    }

    if (fclose(fp) < 0) {
        ErrPrintCode(errno, "fclose");
    }
}

#endif // NDEBUG
#endif // ANDROID
