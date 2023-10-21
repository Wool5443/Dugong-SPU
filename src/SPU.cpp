#include <string.h>
#include <math.h>
#include <time.h>
#include "Stack.hpp"
#include "SPU.hpp"
#include "SPUsettings.ini"

struct SPU
{
    Stack* stack;
    double* RAM;
    double regs[regNum];
    const byte* codeArray;
    size_t ip;
};

struct SPUresult
{
    SPU value;
    ErrorCode error;
};

enum ArgType
{
    ImmediateNumberArg = 1,
    RegisterArg        = 2,
    RAMArg             = 4,
};

struct ArgResult
{
    double value;
    byte argType;
    ErrorCode error;
};

enum Commands
{
    #define DEF_COMMAND(name, num, ...) \
    CMD_ ## name = num,

    #include "Commands.gen"

    #undef DEF_COMMAND
};

SPUresult _SPUinit(const byte codeArray[]);

ErrorCode _SPUdestructor(SPU* spu);

ArgResult _getArg(SPU* spu, byte command);

ErrorCode Run(const byte codeArray[])
{
    MyAssertSoft(codeArray, ERROR_NULLPTR);

    SPUresult spuResult = _SPUinit(codeArray);

    RETURN_ERROR(spuResult.error);

    SPU spu = spuResult.value;

    byte command = spu.codeArray[spu.ip++];

    while (true)
    {
        #define DEF_COMMAND(name, num, hasArg, code, ...)                 \
        if ((command & ~((byte)~0 << BITS_FOR_COMMAND)) == num)           \
        {                                                                 \
                ArgResult argResult = {};                                 \
                if (hasArg)                                               \
                {                                                         \
                    argResult = _getArg(&spu, command);                   \
                    RETURN_ERROR(argResult.error);                        \
                }                                                         \
                code                                                      \
        }                                                                 \
        else

        #include "Commands.gen"

        /*else*/ return ERROR_SYNTAX;

        #undef DEF_COMMAND

        command = spu.codeArray[spu.ip++];
    }

    RETURN_ERROR(_SPUdestructor(&spu));

    return EVERYTHING_FINE;
}

SPUresult _SPUinit(const byte codeArray[])
{
    if (!codeArray)
        return {{}, ERROR_NULLPTR};

    SPU spu = {};

    StackOption stack = StackInit();

    if (stack.error)
        return {{}, stack.error};

    spu.stack = stack.stack;
    spu.codeArray = codeArray;
    spu.ip = 0;
    memset(spu.regs, 0, regNum * sizeof(spu.regs[0]));

    return {spu, EVERYTHING_FINE};
}

ErrorCode _SPUdestructor(SPU* spu)
{
    MyAssertSoft(spu, ERROR_NULLPTR);

    free(spu->RAM);
    memset(spu->regs, 0, regNum * sizeof(spu->regs[0]));

    return StackDestructor(spu->stack);
}

ArgResult _getArg(SPU* spu, byte command)
{
    ArgResult result = {};
    result.argType = (command & ((byte)~0 << BITS_FOR_COMMAND)) >> BITS_FOR_COMMAND;

    if (result.argType & ImmediateNumberArg)
    {
        double tempVal = 0;
        memcpy(&tempVal, spu->codeArray + spu->ip, sizeof(double));
        spu->ip += sizeof(double);

        result.value += tempVal;
    }

    if (result.argType & RegisterArg)
    {
        result.value += spu->regs[spu->codeArray[spu->ip]];
        spu->ip++;
    }

    if (result.argType & RAMArg)
        result.value = spu->RAM[*(uint64_t*)&result.value];

    result.error = EVERYTHING_FINE;

    return result;
}
