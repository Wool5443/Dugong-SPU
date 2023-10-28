#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "Utils.hpp"
#include "SPU.hpp"

static size_t _getFileSize(const char* path);

int main(int argc, const char* argv[])
{
    if (argc == 1)
    {
        printf("Input a file to run.\n");
        return ERROR_BAD_FILE;
    }

    size_t width = 0, height = 0, ramVars = 0;

    uint flags = 0;
    if (argc > 2)
    {
        for (int i = 2; i < argc; i++)
        {
            if (strcmp(argv[i], "--print-ram") == 0)
                flags |= PrintRam;
            else
            {
                sscanf(argv[i], "-w=%zu", &width);
                sscanf(argv[i], "-h=%zu", &height);
                sscanf(argv[i], "--vars=%zu", &ramVars);
            }
        }
    }

    size_t codeArraySize = _getFileSize(argv[1]);
    byte* codeArray  = (byte*)calloc(codeArraySize, 1);
    MyAssertSoft(codeArray, ERROR_NO_MEMORY);

    uint64_t RAMsize = width * height + ramVars;
    double* RAM = (double*)calloc(RAMsize, sizeof(*RAM));
    MyAssertSoft(RAM, ERROR_NO_MEMORY);

    FILE* binFile = fopen(argv[1], "rb");
    MyAssertSoft(binFile, ERROR_BAD_FILE);

    fread(codeArray, 1, codeArraySize, binFile);
    fclose(binFile);

    SPUresult spu = SPUinit(codeArray, RAM, RAMsize);

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

    if (flags & PrintRam)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                if (RAM[y * height + x + ramVars])
                    putchar('#');
                else
                    putchar('.');
            }
            putchar('\n');
        }
    }

    free(codeArray);
    free(RAM);
    SPUdestructor(spu.value);
    return 0;
}

static size_t _getFileSize(const char* path)
{
    MyAssertHard(path, ERROR_NULLPTR);

    struct stat result = {};

    stat(path, &result);

    return (size_t)result.st_size;
}
