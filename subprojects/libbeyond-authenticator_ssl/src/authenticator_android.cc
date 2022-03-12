#ifdef ANDROID
#ifndef NDEBUG

#include "authenticator.h"

#include <beyond/platform/beyond_platform.h>
#include <beyond/private/beyond_private.h>

#include <cstdio>
#include <cerrno>

#include <string>

#define PKGNAME_LEN 256
#define DATA_DIR "/data/data/"
#define FILES_DIR "/files/"
#define PROC_CMDLINE "/proc/self/cmdline"

void Authenticator::DumpToFile(const char *filename, const void *buffer, size_t size)
{
    FILE *fp;

    fp = fopen(PROC_CMDLINE, "r");
    if (fp == nullptr) {
        ErrPrintCode(errno, "fopen");
        return;
    }

    char pkgname[PKGNAME_LEN];
    if (fread(pkgname, 1, sizeof(pkgname), fp) !=  1*sizeof(pkgname)) {
        ErrPrintCode(errno, "fread");
        if (fclose(fp) < 0) {
            ErrPrintCode(errno, "fclose");
        }
        return;
    }

    if (fclose(fp) < 0) {
        ErrPrintCode(errno, "fclose");
    }

    std::string abspath(DATA_DIR);
    abspath += std::string(pkgname) + std::string(FILES_DIR);
    if (authenticator == nullptr) {
        abspath += std::string("root.");
    }
    abspath += std::string(filename);
    DbgPrint("abspath: %s", abspath.c_str());

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
