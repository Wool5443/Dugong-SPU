//! @file

#ifndef SORT_HPP
#define SORT_HPP

/**
 * @brief typedef for compare functions used in universal quicksort.
*/
typedef int CompareFunction_t(const void* a, const void* b);

#include <stddef.h>
/**
 * @brief Finds min of the given array of any type.
 * 
 * @param [in] data - the array to find min in.
 * @param [in] elementCount - length of the array.
 * @param [in] elementSize - size in bytes of the elements.
 * @param [in] compareFunction - @see CompareFunction_t - the comparator.
 * 
 * @return const void* the the smallest element.
*/
const void* MinArray(const void* data, size_t elementCount, size_t elementSize, CompareFunction_t* compareFunction);

/**
 * @brief Finds max of the given array of any type.
 * 
 * @param [in] data - the array to find max in.
 * @param [in] elementCount - length of the array.
 * @param [in] elementSize - size in bytes of the elements.
 * @param [in] compareFunction - @see CompareFunction_t - the comparator.
 * 
 * @return const void* the the largest element.
*/
const void* MaxArray(const void* data, size_t elementCount, size_t elementSize, CompareFunction_t* compareFunction);

/**
 * @brief Swaps the raw bytes of a and b.
 * 
 * @param [in] a - pointer to the 1st object.
 * @param [in] b - pointer to the 2nd object.
 * @param [in] size - size of the objects.
*/
void Swap(void* a, void* b, size_t size);

/**
 * @brief Sorts the given array according to the Compare Function.
 * Uses randomized quick sort.
 * 
 * @param [in] data - the array to sort.
 * @param [in] elementCount - length of the array.
 * @param [in] elementSize - size in bytes of the array elements.
 * @param [in] compareFunction - @see CompareFunction_t
*/
void Sort(void* data, size_t elementCount, size_t elementSize, CompareFunction_t* compareFunction);

#endif