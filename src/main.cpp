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

    FILE* binFile = fopen(argv[1], "rb");
    fread(codeArray, 1, codeArraySize, binFile);
    fclose(binFile);


    SPUresult spu = SPUinit(codeArray);

    RETURN_ERROR(spu.error);

    RETURN_ERROR(Run(spu.value));

    free(codeArray);
    return 0;
}

size_t _getFileSize(const char* path)
{
    MyAssertHard(path, ERROR_NULLPTR);

    struct stat result = {};

    stat(path, &result);

    return (size_t)result.st_size;
}