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

struct RAM;

struct RAMresult
{
    RAM* value;
    ErrorCode error;
};

SPUresult SPUinit(const byte codeArray[], RAM* ram);

ErrorCode SPUdestructor(SPU* spu);

RAMresult RAMinit(size_t width, size_t height, size_t vars);

ErrorCode RAMdestructor(RAM* ram);

ErrorCode Run(SPU* SPU);

#endif
