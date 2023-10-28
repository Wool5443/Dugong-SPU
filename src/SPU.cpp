#include <string.h>
#include <math.h>
#include <time.h>
#include "Stack.hpp"
#include "SPU.hpp"
#include "SPUsettings.ini"

static const size_t HIDDEN_REGISTERS_NUMBER = 1;

struct SPU
{
    Stack* stack;
    Stack* callStack;
    double* RAM;
    uint64_t RAMsize;
    double regs[regNum + HIDDEN_REGISTERS_NUMBER];
    const byte* codeArray;
    uint64_t ip;
};

struct ArgResult
{
    double* value;
    ErrorCode error;
};

ArgResult _getArg(SPU* spu, byte command);

ErrorCode Run(SPU* spu)
{
    MyAssertSoft(spu, ERROR_NULLPTR);

    spu->ip = 0;

    byte command = spu->codeArray[spu->ip++];
    byte commandType = command & ~((byte)~0 << BITS_FOR_COMMAND);

    while (true)
    {
        switch (commandType)
        {
            #define DEF_COMMAND(name, num, hasArg, code, ...)                 \
            case num:                                                         \
            {                                                                 \
                ArgResult argResult = {};                                     \
                if (hasArg)                                                   \
                {                                                             \
                    argResult = _getArg(spu, command);                        \
                    RETURN_ERROR(argResult.error);                            \
                }                                                             \
                code                                                          \
                break;                                                        \
            }                                                                 \

            #include "Commands.gen"

            #undef DEF_COMMAND

            default:
                return ERROR_SYNTAX;
        }

        command = spu->codeArray[spu->ip++];
        commandType = (byte)(command & ~((byte)~0 << BITS_FOR_COMMAND));
    }

    return EVERYTHING_FINE;
}

SPUresult SPUinit(const byte codeArray[], double* RAM, uint64_t RAMsize)
{
    MyAssertSoftResult(codeArray, NULL, ERROR_NULLPTR);

    SPU* spu = (SPU*)calloc(1, sizeof(*spu));
    MyAssertSoftResult(spu, NULL, ERROR_NO_MEMORY);

    StackResult stack = StackInit();
    RETURN_ERROR_RESULT(stack, NULL);

    StackResult callStack = StackInit();
    RETURN_ERROR_RESULT(callStack, NULL);

    spu->RAM = RAM;
    spu->RAMsize = RAMsize;

    spu->stack = stack.value;
    spu->callStack = callStack.value;
    spu->codeArray = codeArray;
    spu->ip = 0;

    return {spu, EVERYTHING_FINE};
}

ErrorCode SPUdestructor(SPU* spu)
{
    MyAssertSoft(spu, ERROR_NULLPTR);

    memset(spu->regs, 0, (regNum + HIDDEN_REGISTERS_NUMBER) * sizeof(spu->regs[0]));

    RETURN_ERROR(StackDestructor(spu->stack));
    RETURN_ERROR(StackDestructor(spu->callStack));

    free(spu);

    return EVERYTHING_FINE;
}

ArgResult _getArg(SPU* spu, byte command)
{
    MyAssertSoftResult(spu, NULL, ERROR_NULLPTR);

    ArgResult result = {};

    spu->regs[rtx] = 0;

    if (command & (ImmediateNumberArg << BITS_FOR_COMMAND))
    {
        memcpy(&spu->regs[rtx], spu->codeArray + spu->ip, sizeof(double));
        spu->ip += sizeof(double);
    }

    if (command & (RegisterArg << BITS_FOR_COMMAND))
    {
        spu->regs[rtx] += spu->regs[spu->codeArray[spu->ip]];
        result.value = &spu->regs[spu->codeArray[spu->ip]]; 
        spu->ip++;
    }

    if (command & (RAMArg << BITS_FOR_COMMAND))
    {
        uint64_t index = (uint64_t)spu->regs[rtx];
        if (index >= spu->RAMsize)
        {
            return {NULL, ERROR_INDEX_OUT_OF_BOUNDS};
        }
        result.value = &spu->RAM[index];
    }
    else if (command & (ImmediateNumberArg << BITS_FOR_COMMAND))
        result.value = &spu->regs[rtx];

    result.error = EVERYTHING_FINE;

    return result;
}
