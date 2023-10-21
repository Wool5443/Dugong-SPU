#include <time.h>
#include "Stack.hpp"
#include "MinMax.hpp"

typedef unsigned int hash_t;

static const hash_t HASH_SEED = 0xBEBDA;

static FILE* _getLogFile()
{
    FILE* _logFile = fopen(logFilePath, "w");
    
    if (!_logFile)
    {
        SetConsoleColor(stderr, COLOR_RED);
        fprintf(stderr, "ERROR!!! COULDN'T OPEN LOG FILE!!!!\n");
        SetConsoleColor(stderr, COLOR_WHITE);

        return NULL;
    }

    setvbuf(_logFile, NULL, _IONBF, 0);

    return _logFile;
}

static FILE* LOG_FILE = _getLogFile();

#define _STACK_DUMP_ERROR_DEBUG(stack, error)                                            \
do                                                                                       \
{                                                                                        \
    if (error && stack && LOG_FILE)                                                      \
    {                                                                                    \
        SourceCodePosition _caller = {__FILE__, __LINE__, __func__};                     \                  
        _stackDump(LOG_FILE, stack, &_caller, error);                                    \
    }                                                                                    \
} while (0);

#ifdef CANARY_PROTECTION
    static canary_t _getRandomCanary()
    {
        srand((unsigned int)time(NULL));

        union _canaryTemp
        {
            canary_t canary;
            uint randNumbers[2];
        } _tCnry = {};
        _tCnry.randNumbers[0] = (uint)rand();
        _tCnry.randNumbers[1] = (uint)rand();

        return _tCnry.canary;
    }

    static const size_t _CANARY = _getRandomCanary();
#endif

struct Stack
{
    #ifdef CANARY_PROTECTION
    canary_t leftCanary;
    #endif

    size_t realDataSize;
    StackElement_t* data;

    SourceCodePosition origin;
    size_t size;
    size_t capacity;

    #ifdef HASH_PROTECTION
    hash_t hashData;
    hash_t hashStack;
    #endif

    #ifdef CANARY_PROTECTION
    canary_t rightCanary;
    #endif
};

#ifdef CANARY_PROTECTION
static ErrorCode _checkCanary(const Stack* stack);

static canary_t* _getLeftDataCanaryPtr(const StackElement_t* data);

static canary_t* _getRightDataCanaryPtr(const StackElement_t* data, size_t realDataSize);
#endif

#ifdef HASH_PROTECTION
static ErrorCode _checkHash(Stack* stack);

static ErrorCode _reHashify(Stack* stack);

static hash_t _calculateDataHash(const Stack* stack);

static hash_t _calculateStackHash(Stack* stack);
#endif

static ErrorCode _stackRealloc(Stack* stack);

StackOption _stackInit(SourceCodePosition* origin)
{
    Stack* stack = (Stack*)calloc(1, sizeof(Stack));

    if (!stack)
        return {NULL, ERROR_NO_MEMORY};

    *stack = 
    {
        #ifdef CANARY_PROTECTION
            .leftCanary = _CANARY,
        #endif
        .origin = *origin,
        .size = 0,
        .capacity = DEFAULT_CAPACITY
        #ifdef CANARY_PROTECTION
            ,.rightCanary = _CANARY
        #endif
    };

    size_t realDataSize = DEFAULT_CAPACITY * sizeof(StackElement_t);

    #ifdef CANARY_PROTECTION
    realDataSize += 2 * sizeof(canary_t);
    #endif

    StackElement_t* data = (StackElement_t*)calloc(realDataSize, 1);

    ErrorCode error = EVERYTHING_FINE;
    if (!data)
    {
        error = ERROR_NO_MEMORY;
        stack->capacity = 0;
        realDataSize = 0;
    }

    #ifdef CANARY_PROTECTION
    if (data)
        data = (StackElement_t*)((void*)data + sizeof(canary_t));
    #endif

    #ifdef CANARY_PROTECTION
    if (data)
    {
        *_getLeftDataCanaryPtr(data) = _CANARY;
        *_getRightDataCanaryPtr(data, realDataSize) = _CANARY;
    }
    #endif

    for (size_t i = 0; i < stack->capacity; i++)
        data[i] = POISON;
    
    stack->data = data;
    stack->realDataSize = realDataSize;

    #ifdef HASH_PROTECTION
    _reHashify(stack);
    #endif

    return {stack, error};
}

ErrorCode StackDestructor(Stack* stack)
{
    ErrorCode error = CheckStackIntegrity(stack);

    _STACK_DUMP_ERROR_DEBUG(stack, error);
    RETURN_ERROR(error);

    if (stack->data == NULL)
        error = ERROR_NO_MEMORY;
    else
        #ifdef CANARY_PROTECTION
        free((void*)stack->data - sizeof(canary_t));
        #else
        free((void*)stack->data);
        #endif

    stack->size = POISON;
    stack->capacity = POISON;

    stack->data = NULL;

    stack->origin = {};

    #ifdef HASH_PROTECTION
        stack->hashData = POISON;
        stack->hashStack = POISON;
    #endif

    #ifdef CANARY_PROTECTION
        stack->leftCanary = POISON;
        stack->rightCanary = POISON;
    #endif
    
    free((void*)stack);

    return error;
}

ErrorCode CheckStackIntegrity(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    if (stack->capacity < stack->size)
        return ERROR_INDEX_OUT_OF_BOUNDS;
    if (!stack->data)
        return ERROR_NO_MEMORY;
    
    #ifdef CANARY_PROTECTION
        ErrorCode canaryError = _checkCanary(stack);
        _STACK_DUMP_ERROR_DEBUG(stack, canaryError);
        RETURN_ERROR(canaryError);
    #endif

    #ifdef HASH_PROTECTION
        ErrorCode error = _checkHash(stack);
        _STACK_DUMP_ERROR_DEBUG(stack, error);
        RETURN_ERROR(error);
    #endif
    
    return EVERYTHING_FINE;
}

ErrorCode _stackDump(FILE* where, Stack* stack, SourceCodePosition* caller, ErrorCode error)
{
    MyAssertSoft(stack, ERROR_NULLPTR);
    MyAssertSoft(caller, ERROR_NULLPTR);

    MyAssertSoft(where, ERROR_BAD_FILE);

    const size_t maxStackValues = 4096;

    fprintf(where, "Stack[%p] from %s(%zu) %s()\n", stack, stack->origin.fileName, stack->origin.line, stack->origin.name);
    fprintf(where, "called from %s(%zu) %s()\n", caller->fileName, caller->line, caller->name);
    fprintf(where, "Stack condition - %s\n", ERROR_CODE_NAMES[error]);

    #ifdef HASH_PROTECTION
        uint dataHash  = _calculateDataHash(stack);
        uint stackHash = _calculateStackHash(stack);

        fprintf(where, "Data hash = %u", stack->hashData);
        if (stack->hashData != dataHash)
            fprintf(where, " INVALID!!! SHOULD BE %u", dataHash);
        fprintf(where, "\n");

        fprintf(where, "Stack hash = %u", stack->hashStack);
        if (stack->hashStack != stackHash)
            fprintf(where, " INVALID!!! SHOULD BE %u", stackHash);
        fprintf(where, "\n");
    #endif

    #ifdef CANARY_PROTECTION
        fprintf(where, "Left stack canary = %zu", stack->leftCanary);
        if (stack->leftCanary != _CANARY)
            fprintf(where, " INVALID!!! SHOULD BE %zu", _CANARY);
        fprintf(where, "\n");

        fprintf(where, "Right stack canary = %zu", stack->rightCanary);
        if (stack->rightCanary != _CANARY)
            fprintf(where, " INVALID!!! SHOULD BE %zu", _CANARY);
        fprintf(where, "\n");
    #endif

    fprintf(where, "{\n");
    fprintf(where, "    size = %zu\n", stack->size);
    fprintf(where, "    capacity = %zu\n", stack->capacity);
    fprintf(where, "    data[%p]\n", stack->data);

    #ifdef CANARY_PROTECTION
        canary_t leftDataCanary = *_getLeftDataCanaryPtr(stack->data);
        fprintf(where, "    Left data canary = %zu", leftDataCanary);
        if (leftDataCanary != _CANARY)
            fprintf(where, " INVALID!!! SHOULD BE %zu", _CANARY);
        fprintf(where, "\n");
    #endif

    const StackElement_t* data = stack->data;
    size_t numOfElements = min(stack->capacity, maxStackValues);

    for (size_t i = 0; i < numOfElements; i++)
    {
        fprintf(where, "    ");

        if (data[i] != POISON)
            fprintf(where, "*[%zu] = " STACK_EL_SPECIFIER "\n", i, data[i]);
        else
            fprintf(where, " [%zu] = POISON\n", i);
    }

    #ifdef CANARY_PROTECTION
        canary_t rightDataCanary = *_getRightDataCanaryPtr(stack->data, stack->realDataSize);
        fprintf(where, "    Right data canary = %zu", rightDataCanary);
        if (leftDataCanary != _CANARY)
            fprintf(where, " INVALID!!! SHOULD BE %zu", _CANARY);
        fprintf(where, "\n");
    #endif

    fprintf(where, "}\n\n\n");

    return EVERYTHING_FINE;
}

ErrorCode Push(Stack* stack, StackElement_t value)
{
    ErrorCode error = CheckStackIntegrity(stack);

    _STACK_DUMP_ERROR_DEBUG(stack, error);
    RETURN_ERROR(error);

    error = _stackRealloc(stack);

    _STACK_DUMP_ERROR_DEBUG(stack, error);
    RETURN_ERROR(error);

    stack->data[stack->size++] = value;

    #ifdef CANARY_PROTECTION
        ErrorCode canaryError = _checkCanary(stack);
        _STACK_DUMP_ERROR_DEBUG(stack, canaryError);
        RETURN_ERROR(canaryError);
    #endif

    #ifdef HASH_PROTECTION
    _reHashify(stack);
    #endif

    return EVERYTHING_FINE;
}

StackElementOption Pop(Stack* stack)
{
    ErrorCode error = CheckStackIntegrity(stack);

    _STACK_DUMP_ERROR_DEBUG(stack, error);

    if (error)
        return {POISON, error};

    if (stack->size == 0)
        return {POISON, ERROR_INDEX_OUT_OF_BOUNDS};

    stack->size--;

    StackElement_t value = stack->data[stack->size];
    
    stack->data[stack->size] = POISON;

    error = _stackRealloc(stack);

    _STACK_DUMP_ERROR_DEBUG(stack, error);
    if (error)
        return {value, error};

    #ifdef CANARY_PROTECTION
        ErrorCode canaryError = _checkCanary(stack);
        _STACK_DUMP_ERROR_DEBUG(stack, canaryError);
        if (canaryError)
            return {POISON, canaryError};
    #endif

    #ifdef HASH_PROTECTION
    error = _reHashify(stack);
    #endif

    return {value, error};
}

/**
 * @brief Performs stack reallocation if needed.
 * 
 * It increases stack's size if @see STACK_GROW_FACTOR if stack.size == stack.capacity.
 * It shrinks the stack if stack.size <= stack.capacity in @see STACK_GROW_FACTOR ** 2.
 * Otherwise it does nothing.
 * 
 * @param [in] stack - to resize.
 * 
 * @return @see ErrorCode.
*/
static ErrorCode _stackRealloc(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    size_t newCapacity = 0;

    if (stack->size == stack->capacity)
        newCapacity = stack->capacity * STACK_GROW_FACTOR;
    else if (DEFAULT_CAPACITY < stack->capacity && stack->size <= stack->capacity / (STACK_GROW_FACTOR * STACK_GROW_FACTOR))
        newCapacity = stack->capacity / (STACK_GROW_FACTOR * STACK_GROW_FACTOR);

    if (newCapacity != 0)
    {
        StackElement_t* oldData = stack->data;
        size_t newDataSize = newCapacity * sizeof(StackElement_t);

        #ifdef CANARY_PROTECTION
            oldData = (StackElement_t*)((void*)oldData - sizeof(canary_t));
            newDataSize += 2 * sizeof(canary_t);

            canary_t* oldRightCanaryPtr = _getRightDataCanaryPtr(stack->data, stack->realDataSize);
            canary_t  oldRightCanary    = *oldRightCanaryPtr;

            *oldRightCanaryPtr = 0;
        #endif

        StackElement_t* newData = (StackElement_t*)realloc((void*)oldData, newDataSize);

        if (newData == NULL)
        {
            _STACK_DUMP_ERROR_DEBUG(stack, ERROR_NO_MEMORY);
            
            #ifdef CANARY_PROTECTION
            *oldRightCanaryPtr = oldRightCanary;
            #endif

            return ERROR_NO_MEMORY;
        }

        #ifdef CANARY_PROTECTION
        newData = (StackElement_t*)((void*)newData + sizeof(canary_t));
        #endif

        #ifdef CANARY_PROTECTION
        *_getRightDataCanaryPtr(newData, newDataSize) = oldRightCanary;
        #endif

        stack->data = newData;
        stack->realDataSize = newDataSize;
        stack->capacity = newCapacity;

        for (size_t i = stack->size; i < stack->capacity; i++)
            newData[i] = POISON;
    }

    #ifdef CANARY_PROTECTION
        ErrorCode canaryError = _checkCanary(stack);
        _STACK_DUMP_ERROR_DEBUG(stack, canaryError);
        RETURN_ERROR(canaryError);
    #endif

    return EVERYTHING_FINE;
}

#ifdef CANARY_PROTECTION
static ErrorCode _checkCanary(const Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    if (stack->leftCanary != _CANARY ||
        stack->rightCanary != _CANARY ||       
        *_getLeftDataCanaryPtr(stack->data) != _CANARY ||
        *_getRightDataCanaryPtr(stack->data, stack->realDataSize) != _CANARY)
    {
        return ERROR_DEAD_CANARY;
    }
    
    return EVERYTHING_FINE;
}

static canary_t* _getLeftDataCanaryPtr(const StackElement_t* data)
{
    return (canary_t*)((void*)data - sizeof(canary_t));
}

static canary_t* _getRightDataCanaryPtr(const StackElement_t* data, size_t realDataSize)
{
    return (canary_t*)((void*)data + realDataSize - 2 * sizeof(canary_t));
}
#endif

#ifdef HASH_PROTECTION
static ErrorCode _reHashify(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    hash_t hashData = _calculateDataHash(stack);
    hash_t hashStack = _calculateStackHash(stack);

    stack->hashData = hashData;
    stack->hashStack = hashStack;

    return EVERYTHING_FINE;
}

static hash_t _calculateDataHash(const Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    const void* data = (const void*)stack->data;

    #ifdef CANARY_PROTECTION
    data -= sizeof(canary_t);
    #endif

    return CalculateHash(data, stack->realDataSize, HASH_SEED);
}

static hash_t _calculateStackHash(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    hash_t oldHashData = stack->hashData;
    hash_t oldHashStack = stack->hashStack;

    stack->hashData = 0;
    stack->hashStack = 0;

    hash_t stackHash = CalculateHash((const void*)stack, sizeof(*stack), HASH_SEED);

    stack->hashData = oldHashData;
    stack->hashStack = oldHashStack;

    return stackHash;
}

static ErrorCode _checkHash(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    if (stack->hashData != _calculateDataHash(stack) || stack->hashStack != _calculateStackHash(stack))
        return ERROR_BAD_HASH;
    
    return EVERYTHING_FINE;
}
#endif
