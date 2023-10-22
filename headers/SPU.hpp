#ifndef SPU_HPP
#define SPU_HPP

#include "Utils.hpp"

typedef unsigned char byte;

struct SPU;

struct SPUresult
{
    SPU* value;
    ErrorCode error;
};

SPUresult SPUinit(const byte codeArray[]);

ErrorCode SPUdestructor(SPU* spu);

ErrorCode Run(SPU* SPU);

#endif
