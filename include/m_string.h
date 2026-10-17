#ifndef M_STRING_H
#define M_STRING_H

#include "m_result.h"

#include <stdlib.h>
#include <stdbool.h>

typedef struct [[nodiscard("String owns heap memory; the result must be stored and destroyed")]]
{
	char* data;
	size_t length;
	size_t capacity;
} String;

typedef struct [[nodiscard("StringSplit owns heap memory; the result must be stored and destroyed")]]
{
	String string1;
	String string2;
} StringSplit;

/*
	Empty vs. NULL:
	An empty String is defined as having a length of 0, regardless of data buffer allocation state.
	Functions will treat empty Strings with NULL and non-NULL data buffers as equivalent, except when
	returning a new String under failure conditions wherein a NULL data buffer will be returned.


	Copies:
	Shallow copies are not supported under any conditions.


	Valid Strings:
	Every String must be initialized (zeroed or created) before use.
	Valid Strings obey the following invariants:
		- NULL data implies 0 length and 0 capacity
		- Length never exceeds capacity
		- A non-NULL buffer is always terminated at length


	Return Values and Failure Guarantees:
	A return value of M_SUCCESS (0) indicates success; M_FAILURE_GEN (1) indicates failure.
	For functions returning an index, the index of the null terminator (length + 1) will be returned
	in case of a failure of any kind (eg. a substring not being present, etc.).

	A failure is defined as any function and/or operation not succeeding completely.

	All functions returning new structs, eg. a new String, will return an empty struct with a NULL data buffer in case of failure;
	if the function succeeds but results in an empty struct, the struct will have a non-NULL data buffer.
	Functions mutating parameters will leave output parameters unmodified in case of failure.


	NULL and Empty Arguments:
	NULL pointer arguments to functions returning Strings will return an empty String with a NULL data buffer.
	Any NULL pointer argument to a function mutating output parameters will result in a failure.
	NULL pointer arguments to comparison functions will always cause the function to return false.

	Functions receiving empty Strings as parameters will execute normally.


	Aliasing:
	Argument aliasing is supported for the following operations:
		- CopyTo self
		- Prepend self
		- Append self
		- Concat with destination equal to an operand
		- Extract substring into the same String
		- SplitString aliasing a member String of the given StringSplit

	Other aliasing cases should be assumed unsupported.


	Bounds Checking:
	Trimming and extracting functions will clamp indices within the length of the String.
	Removal functions will return M_FAILURE_GEN (1) in case of out-of-range indices.
	Data access functions returning by value, eg. GetCharAt(), will return a null terminator (\0) character if the return is a single char
	or an empty String with a NULL data buffer in case of an out-of-range index or other failure.
	Data access functions returning pointers, eg. GetCharPtrAt(), will return a NULL pointer in case of an out-of-range index or other failure.
	Out-of-order index arguments, eg. start > end, will return an error code of M_FAILURE_GEN (1).
*/

// Create a new String struct from a C-style string
// Null pointer argument will return an empty String
// Returned String is owned by and must be destroyed by caller
String CreateString(const char s[]);
// Free all heap memory used by a String
int DestroyString(void* s);
// Clear the data of a String
int ClearString(String* s);
// Move data from one String to another
// Destroys the source String when successful
// No-op if destination and source are the same String
int MoveString(String* dest, String* source);
// Create a new String struct from an existing String
// Returned String is owned by and must be destroyed by caller
String CopyString(const String* s);
// Copy data from one String to another
int CopyStringTo(void* dest, const void* source);
// Copy data from C-style string to a String
// Source C-strings pointing into the dest String are unsupported
int CopyCStrToString(String* dest, const char source[]);

// Create a new zero-initialized StringSplit struct
// Returned StringStruct is owned by an must be destroyed by caller
StringSplit CreateStringSplit(void);
// Free all memory used by a StringSplit
int DestroyStringSplit(StringSplit* split);

// Get a copy of the character at a given index in a String
char StringGetCharAt(const String* s, size_t index);
// Get a pointer to the character at a given index in a String
char* StringGetCharPtrAt(const String* s, size_t index);
// Set a character at a given index in a String
int StringSetCharAt(String* s, size_t index, char chara);

// Print data of a String
int PrintString(const String* s);
// Print data of a String and append a line break
int PrintStringLn(const String* s);

// Prepend a character to a String
int StringPrependChar(String* s, char chara);
// Prepend a String to another String
int StringPrependString(String* s, const String* prepend);
// Prepend a C-string to a String
int StringPrependCStr(String* s, const char prepend[]);
// Append a character to a String
int StringAppendChar(String* s, char chara);
// Append a String to another String
int StringAppendString(String* s, const String* append);
// Append a C-string to a String
int StringAppendCStr(String* s, const char append[]);
// Insert a character at a given index in a String
int StringInsertCharAt(String* s, size_t index, char chara);
// Insert a String at a given index in another String
int StringInsertStringAt(String* s, size_t index, const String* insert);
// Insert a C-string at a given index in a String
int StringInsertCStrAt(String* s, size_t index, const char insert[]);

// Concatenate two Strings
int ConcatStrings(String* dest, const String* s1, const String* s2);
// Concatenate a String and a C-style string
int ConcatStringCStr(String* dest, const String* s1, const char s2[]);
// Concatenate a C-style string and a String
int ConcatCStrString(String* dest, const char s1[], const String* s2);

// Reverse characters of a String in place
int ReverseString(String* s);

// Split a String at an index
// The provided StringSplit must have valid initialization - either from CreateStringSplit() or a previous SplitString() call
int SplitString(StringSplit* split, const String* s, size_t index);
// Trim characters from start of a String
int StringTrimStart(String* s, size_t count);
// Trim characters from end of a String
int StringTrimEnd(String* s, size_t count);
// Trim characters from start and end of a String (start index inclusive, end index exclusive)
int StringTrim(String* s, size_t start, size_t end);
// Extract substring from a String between a start and end index (start inclusive, end exclusive)
int StringExtractSubstring(String* dest, const String* s, size_t start, size_t end);

// Check whether two Strings are exactly equal
bool StringsEqual(const void* s1, const void* s2);
// Check whether a String and a C-string are exactly equal
bool StringsEqualCStr(const String* s1, const char s2[]);
// Check whether a String contains another String
bool StringContainsSubstring(const String* s, const String* sub);
// Check whether a String contains a C-string
bool StringContainsCStr(const String* s, const char sub[]);

// Remove character from a String at a given index
int StringRemoveCharAt(String* s, size_t index);
// Remove a range from a String
int StringRemoveRange(String* s, size_t start, size_t length);
// Remove the first instance of a character from a String
int StringRemoveCharFirst(String* s, char chara);
// Remove all instances of a character from a String
int StringRemoveCharAll(String* s, char chara);
// Remove the first instance of a substring from a String
int StringRemoveSubstringFirst(String* s, const String* substring);
// Remove all instances of a substring from a String.
int StringRemoveSubstringAll(String* s, const String* substring);
// Remove the first instance of a C-string from a String
int StringRemoveCStrFirst(String* s, const char substring[]);
// Remove all instances of a C-string from a String
int StringRemoveCStrAll(String* s, const char substring[]);

#endif