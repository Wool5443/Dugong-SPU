#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "Utils.hpp"
#include "SPU.hpp"

static size_t _getFileSize(const char* path);

int main(int argc, const char* const argv[])
{
    if (argc == 1)
    {
        printf("Input a file to run.\n");
        return ERROR_BAD_FILE;
    }

    size_t width = 0, height = 0, ramVars = 0;

    bool flags = false;
    if (argc > 2)
    {
        flags = true;
        for (int i = 2; i < argc; i++)
        {
            sscanf(argv[i], "-w=%zu", &width);
            sscanf(argv[i], "-h=%zu", &height);
            sscanf(argv[i], "-v=%zu", &ramVars);
            sscanf(argv[i], "--vars=%zu", &ramVars);
        }
    }

    size_t codeArraySize = _getFileSize(argv[1]);
    byte* codeArray      = (byte*)calloc(codeArraySize, 1);
    MyAssertSoft(codeArray, ERROR_NO_MEMORY);

    FILE* binFile = fopen(argv[1], "rb");
    MyAssertSoft(binFile, ERROR_BAD_FILE);

    if (fread(codeArray, 1, codeArraySize, binFile) != codeArraySize)
        return ERROR_BAD_FILE;

    fclose(binFile);

    RAMresult ram = {NULL, EVERYTHING_FINE};

    if (flags)
    {
        ram = RAMinit(width, height, ramVars);

        if (ram.error)
        {
            fprintf(stderr, "%s!!!\n", ERROR_CODE_NAMES[ram.error]);
            return ram.error;
        }
    }

    SPUresult spu = SPUinit(codeArray, ram.value);

    if (spu.error)
    {
        fprintf(stderr, "%s!!!\n", ERROR_CODE_NAMES[spu.error]);
        return spu.error;
    }

    ErrorCode runError = Run(spu.value);
    if (runError)
    {
        fprintf(stderr, "%s!!!\n", ERROR_CODE_NAMES[runError]);
        return spu.error;
    }

    free(codeArray);
    SPUdestructor(spu.value);
    RAMdestructor(ram.value);

    return EVERYTHING_FINE;
}

static size_t _getFileSize(const char* path)
{
    MyAssertHard(path, ERROR_NULLPTR);

    struct stat result = {};

    stat(path, &result);

    return (size_t)result.st_size;
}
