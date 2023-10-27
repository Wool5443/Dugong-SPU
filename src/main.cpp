#include <stdio.h>
#include <sys/stat.h>
#include "Utils.hpp"
#include "SPU.hpp"

size_t _getFileSize(const char* path);

int main(int argc, const char* argv[])
{
    if (argc == 1)
    {
        printf("Input a file to run.\n");
        return ERROR_BAD_FILE;
    }

    size_t codeArraySize = _getFileSize(argv[1]);
    byte* codeArray  = (byte*)calloc(codeArraySize, 1);
    MyAssertSoft(codeArray, ERROR_NO_MEMORY);

    int width = 80, height = 80;

    uint64_t RAMsize = width * height + 2;
    double* RAM = (double*)calloc(RAMsize, sizeof(*RAM));
    MyAssertSoft(RAM, ERROR_NO_MEMORY);

    FILE* binFile = fopen(argv[1], "rb");
    fread(codeArray, 1, codeArraySize, binFile);
    fclose(binFile);

    SPUresult spu = SPUinit(codeArray, RAM, RAMsize);

    RETURN_ERROR(spu.error);

    RETURN_ERROR(Run(spu.value));
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (RAM[y * height + x])
                putchar('#');
            else
                putchar('.');
        }
        putchar('\n');
    }

    free(codeArray);
    free(RAM);
    SPUdestructor(spu.value);
    return 0;
}

size_t _getFileSize(const char* path)
{
    MyAssertHard(path, ERROR_NULLPTR);

    struct stat result = {};

    stat(path, &result);

    return (size_t)result.st_size;
}