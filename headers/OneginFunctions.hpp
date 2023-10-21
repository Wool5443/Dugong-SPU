//! @file

#ifndef ONEGIN_FUNCTIONS_HPP
#define ONEGIN_FUNCTIONS_HPP

#include <stddef.h>
#include "StringFunctions.hpp"

/**
 * @brief Characters to ignore in texts.
*/
static const char* const IGNORED_SYMBOLS = " ,.;:'\"-!?`~()[]{}";

/** @struct Text
 * @brief Text struct which contains text itself and lines of the text.
 * 
 * @var Text::rawText - text.
 * @var Text::lines - const pointers to lines in rawText.
 * @var Text::size - number of bytes in text.
 * @var Text::numberOfLines - the length of lines.
*/
struct Text
{
    const char* rawText;
    const String* lines;
    size_t size;
    size_t numberOfLines;
};

/**
 * @brief Creates a Text member and reads its contents from file.
 * 
 * @param [in] path - the path to a file.
 * 
 * @return Text.
*/
Text CreateText(const char* path, char terminator);

/**
 * @brief Frees all text's memory.
 * 
 * @param [in] text - pointer to the text to destroy.
*/
void DestroyText(Text* text);

/**
 * @brief Sorts text's lines. Uses quick sort.
 * 
 * @param [in] text - the text lines of which are to be sorted.
 * @param [in] sortType - the way to compare lines.
*/
void SortTextLines(Text* text, StringCompareMethod sortType);

/**
 * @brief Prints raw text form text to a file.
 * 
 * @param [in] text - what to write.
 * @param [in, out] file - where to write.
*/
void PrintRawText(const Text* text, FILE* file);

/**
 * @brief Prints text's line to file, ignoring \\n's.
 * 
 * @param[in] text - what to write.
 * @param [in, out] file - where to write.
*/
void PrintTextLines(const Text* text, FILE* file);

#endif
