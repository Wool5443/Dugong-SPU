//! @file

#ifndef STACK_HPP
#define STACK_HPP

#include <stddef.h>
#include <stdint.h>
#include "Utils.hpp"

#include "Stack.settings"

typedef size_t canary_t;

/**
 * @brief Stack structure with hidden fields.
*/
struct Stack;

/**
 * @brief Struct that @see StackInit returns. If error is not 0, than @see StackOption::stack = NULL.
 * 
 * @var StackOption::stack - pointer to the stack.
 * @var StackOption::error - error message @see ErrorCode.
*/
struct StackOption
{
    Stack* stack;
    ErrorCode error;
};

/**
 * @brief Struct used for pop. If it couldn't pop returns an error and a poison as value.
 * 
 * @var StackOption::value - returned value.
 * @var StackOption::error - error message @see ErrorCode.
*/
struct StackElementOption
{
    StackElement_t value;
    ErrorCode error;
};

/**
 * @brief Initializes a stack.
 * 
 * @return Stack*.
*/
#define StackInit()                                                                      \
({                                                                                       \
    SourceCodePosition _owner = {__FILE__, __LINE__, __func__};                          \
    _stackInit(&_owner);                                                                 \
})

/**
 * @brief dumps a stack to a given file.
*/
#define StackDump(where, stack)                                                          \
do                                                                                       \
{                                                                                        \
    if (stack)                                                                           \
    {                                                                                    \
        SourceCodePosition _caller = {__FILE__, __LINE__, __func__};                     \
        _stackDump(where, stack, &_caller, CheckStackIntegrity(stack));                  \
    }                                                                                    \
} while (0);                        

StackOption _stackInit(SourceCodePosition* owner);

/**
 * @brief Destructor of a stack.
 * 
 * @param [in] stack - the stack to destruct.
 * 
 * @return @see @enum ErrorCode.
*/
ErrorCode StackDestructor(Stack* stack);

/**
 * @brief Check the state of a stack.
 * 
 * @param [in] stack - the stack to check.
 * 
 * @return @see @enum ErrorCode.
*/
ErrorCode CheckStackIntegrity(Stack* stack);

ErrorCode _stackDump(FILE* where, Stack* stack, SourceCodePosition* caller, ErrorCode error);

/**
 * @brief Adds an element on top of stack.
 * 
 * @param [in] stack - the stack to add to.
 * @param [in] value - what to add.
 * 
 * @return error code.
*/
ErrorCode Push(Stack* stack, StackElement_t value);

/**
 * @brief Deletes and returns the top element of the stack.
 * 
 * @param [in] stack - the stack to pop from.
 * 
 * @return Option containing value and error code.
*/
StackElementOption Pop(Stack* stack);

#endif
