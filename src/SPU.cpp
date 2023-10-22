#include <string.h>
#include <math.h>
#include <time.h>
#include "Stack.hpp"
#include "SPU.hpp"
#include "SPUsettings.ini"

struct SPU
{
    Stack* stack;
    Stack* callStack;
    double* RAM;
    double regs[regNum];
    const byte* codeArray;
    uint64_t ip;
};

enum Register
{
    rax = 1,
    rbx = 2,
    rcx = 3,
    rdx = 4,
    rtx = 0,
};

enum ArgType
{
    ImmediateNumberArg = 1,
    RegisterArg        = 2,
    RAMArg             = 4,
};

struct ArgResult
{
    double* value;
    ErrorCode error;
};

enum Commands
{
    #define DEF_COMMAND(name, num, ...) \
    CMD_ ## name = num,

    #include "Commands.gen"

    #undef DEF_COMMAND
};

ArgResult _getArg(SPU* spu, byte command);

ErrorCode Run(SPU* spu)
{
    MyAssertSoft(spu, ERROR_NULLPTR);

    byte command = spu->codeArray[spu->ip++];

    while (true)
    {
        #define DEF_COMMAND(name, num, hasArg, code, ...)                 \
        if ((command & ~((byte)~0 << BITS_FOR_COMMAND)) == num)           \
        {                                                                 \
                ArgResult argResult = {};                                 \
                if (hasArg)                                               \
                {                                                         \
                    argResult = _getArg(spu, command);                    \
                    RETURN_ERROR(argResult.error);                        \
                }                                                         \
                code                                                      \
        }                                                                 \
        else

        #include "Commands.gen"

        /*else*/ return ERROR_SYNTAX;

        #undef DEF_COMMAND

        command = spu->codeArray[spu->ip++];
    }

    return EVERYTHING_FINE;
}

SPUresult SPUinit(const byte codeArray[])
{
    if (!codeArray)
        return {NULL, ERROR_NULLPTR};

    SPU* spu = (SPU*)calloc(1, sizeof(*spu));

    StackResult stack = StackInit();

    if (stack.error)
        return {NULL, stack.error};

    StackResult callStack = StackInit();

    if (callStack.error)
        return {NULL, callStack.error};

    spu->RAM = (double*)calloc(RAMsize, sizeof(double));

    if (!spu->RAM)
        return {NULL, ERROR_NO_MEMORY};

    spu->stack = stack.value;
    spu->callStack = callStack.value;
    spu->codeArray = codeArray;
    spu->ip = 0;
    memset(spu->regs, 0, regNum * sizeof(spu->regs[0]));

    return {spu, EVERYTHING_FINE};
}

ErrorCode SPUdestructor(SPU* spu)
{
    MyAssertSoft(spu, ERROR_NULLPTR);

    free(spu->RAM);
    memset(spu->regs, 0, regNum * sizeof(spu->regs[0]));

    RETURN_ERROR(StackDestructor(spu->stack));

    free(spu);
}

ArgResult _getArg(SPU* spu, byte command)
{
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
        result.value = &spu->RAM[(uint64_t)spu->regs[rtx]];
    else if (command & (ImmediateNumberArg << BITS_FOR_COMMAND))
        result.value = &spu->regs[rtx];

    result.error = EVERYTHING_FINE;

    return result;
}
