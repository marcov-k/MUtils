#include "../include/m_string.h"

#include "../include/m_result.h"
#include "../include/m_contain.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int GrowString(String* s, size_t length)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (s->data != NULL && s->capacity >= length) return M_SUCCESS;

	size_t capacity = max(s->capacity, 1);
	while (capacity < length)
	{
		if (capacity > SIZE_MAX / 2) return M_FAILURE_GEN;
		capacity *= 2;
	}

	if (capacity > SIZE_MAX - 1) return M_FAILURE_GEN;
	char* newBuffer = realloc(s->data, capacity + 1);
	if (newBuffer == NULL) return M_FAILURE_GEN;

	s->data = newBuffer;
	s->capacity = capacity;

	return M_SUCCESS;
}

String CreateString(const char s[])
{
	String string = { NULL, 0, 0 };

	if (s == NULL)
	{
		if (GrowString(&string, 0) != M_SUCCESS) return string;
		
		string.data[0] = '\0';
		return string;
	}

	size_t length = strlen(s);
	if (GrowString(&string, length) != M_SUCCESS) return string;

	memcpy(string.data, s, length);
	string.data[length] = '\0';
	string.length = length;

	return string;
}

int DestroyString(void* s)
{
	if (s == NULL) return M_FAILURE_GEN;

	String* sPtr = (String*)s;

	if (sPtr->data != NULL) // only free memory if allocated
	{
		free(sPtr->data);
		sPtr->data = NULL;
	}
	sPtr->capacity = 0;
	sPtr->length = 0;

	return M_SUCCESS;
}

int ClearString(String* s)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (GrowString(s, 0) != M_SUCCESS) return M_FAILURE_GEN;

	s->data[0] = '\0';
	s->length = 0;

	return M_SUCCESS;
}

int MoveString(String* dest, String* source)
{
	if (dest == NULL || source == NULL) return M_FAILURE_GEN;
	if (dest == source) return M_SUCCESS;

	if (GrowString(dest, source->length) != M_SUCCESS) return M_FAILURE_GEN;

	memcpy(dest->data, source->data, source->length);
	dest->data[source->length] = '\0';
	dest->length = source->length;

	DestroyString(source); // cannot fail (source != NULL)
	return M_SUCCESS;
}

String CopyString(const String* s)
{
	String copy = { NULL, 0, 0 };

	if (s == NULL) return copy;

	if (GrowString(&copy, s->length) != M_SUCCESS) return copy;

	memcpy(copy.data, s->data, s->length);
	copy.data[s->length] = '\0';
	copy.length = s->length;

	return copy;
}

int CopyStringTo(void* dest, const void* source)
{
	if (dest == NULL || source == NULL) return M_FAILURE_GEN;

	if (dest == source) return M_SUCCESS;

	String* destStr = (String*)dest;
	String* sourceStr = (String*)source;

	if (sourceStr->length == 0)
	{
		if (ClearString(destStr) != M_SUCCESS) return M_FAILURE_GEN;
		return M_SUCCESS;
	}

	if (GrowString(destStr, sourceStr->length) != M_SUCCESS) return M_FAILURE_GEN;

	memcpy(destStr->data, sourceStr->data, sourceStr->length);
	destStr->data[sourceStr->length] = '\0';
	destStr->length = sourceStr->length;

	return M_SUCCESS;
}

int CopyCStrToString(String* dest, const char source[])
{
	if (dest == NULL || source == NULL) return M_FAILURE_GEN;

	size_t length = strlen(source);
	if (length == 0)
	{
		if (ClearString(dest) != M_SUCCESS) return M_FAILURE_GEN;
		return M_SUCCESS;
	}

	if (GrowString(dest, length) != M_SUCCESS) return M_FAILURE_GEN;

	memmove(dest->data, source, length);
	dest->data[length] = '\0';
	dest->length = length;

	return M_SUCCESS;
}

StringSplit CreateStringSplit(void)
{
	StringSplit split = { CreateString(NULL), CreateString(NULL) };
	return split;
}

int DestroyStringSplit(StringSplit* split)
{
	if (split == NULL) return M_FAILURE_GEN;

	DestroyString(&split->string1); // StringSplit still successfully destroyed if string1 == NULL
	DestroyString(&split->string2); // StringSplit still successfully destroyed if string2 == NULL

	return M_SUCCESS;
}

char StringGetCharAt(const String* s, size_t index)
{
	if (s == NULL) return '\0';
	if (index >= s->length) return '\0';

	return s->data[index];
}

char* StringGetCharPtrAt(const String* s, size_t index)
{
	if (s == NULL) return NULL;
	if (index >= s->length) return NULL;

	return s->data + index;
}

int StringSetCharAt(String* s, size_t index, char chara)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (index >= s->length) return M_FAILURE_GEN;

	s->data[index] = chara;
	return M_SUCCESS;
}

int PrintString(const String* s)
{
	if (s == NULL) return M_FAILURE_GEN;

	printf("%s", s->data ? s->data : "");

	return M_SUCCESS;
}

int PrintStringLn(const String* s)
{
	if (s == NULL) return M_FAILURE_GEN;

	printf("%s\n", s->data ? s->data : "");

	return M_SUCCESS;
}

static int PointsInto(const char s[], size_t sLen, const char p[], size_t pLen, size_t* offOut)
{
	if (p == NULL) return M_CONT_NONE;

	uintptr_t base = (uintptr_t)s, q = (uintptr_t)p;

	if (q < base || q - base >= sLen) return M_CONT_NONE;

	size_t off = (size_t)(q - base);
	if (pLen > sLen - off) return M_CONT_PART;

	if (offOut != NULL) *offOut = off;
	return M_CONT_FULL;
}

int StringPrependChar(String* s, char chara)
{
	if (s == NULL) return M_FAILURE_GEN;

	if (s->length > SIZE_MAX - 1) return M_FAILURE_GEN;
	size_t length = s->length + 1;
	if (GrowString(s, length) != M_SUCCESS) return M_FAILURE_GEN;

	memmove(s->data, s->data + 1, s->length);
	s->data[0] = chara;
	s->data[length] = '\0';
	s->length++;

	return M_SUCCESS;
}

static int PrependStringBytes(String* s, const char prepend[], size_t prependLen)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (prepend == NULL || prependLen == 0) return M_SUCCESS;

	if (s->length > SIZE_MAX - prependLen) return M_FAILURE_GEN;
	size_t length = s->length + prependLen;

	size_t off;
	int contained = PointsInto(s->data, s->capacity + 1, prepend, prependLen, &off);
	if (contained == M_CONT_PART) return M_FAILURE_GEN;
	if (contained == M_CONT_FULL && off + prependLen > s->length) return M_FAILURE_GEN;

	if (GrowString(s, length) != M_SUCCESS) return M_FAILURE_GEN;
	memmove(s->data + prependLen, s->data, s->length);

	if (contained == M_CONT_FULL) // prepend string is contained in target String
	{
		memmove(s->data, s->data + prependLen + off, prependLen);
	}
	else
	{
		memmove(s->data, prepend, prependLen);
	}
	s->data[length] = '\0';
	s->length = length;

	return M_SUCCESS;
}

int StringPrependString(String* s, const String* prepend)
{
	if (prepend == NULL) return M_FAILURE_GEN;
	
	return PrependStringBytes(s, prepend->data, prepend->length);
}

int StringPrependCStr(String* s, const char prepend[])
{
	if (prepend == NULL) return M_FAILURE_GEN;

	return PrependStringBytes(s, prepend, strlen(prepend));
}

int StringAppendChar(String* s, char chara)
{
	if (s == NULL) return M_FAILURE_GEN;

	if (s->length > SIZE_MAX - 1) return M_FAILURE_GEN;
	size_t length = s->length + 1;
	if (GrowString(s, length) != M_SUCCESS) return M_FAILURE_GEN;

	s->data[s->length] = chara;
	s->data[length] = '\0';
	s->length++;

	return M_SUCCESS;
}

static int AppendStringBytes(String* s, const char append[], size_t appendLen)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (append == NULL || appendLen == 0) return M_SUCCESS;

	if (s->length > SIZE_MAX - appendLen) return M_FAILURE_GEN;
	size_t length = s->length + appendLen;

	size_t off;
	int contained = PointsInto(s->data, s->capacity + 1, append, appendLen, &off);
	if (contained == M_CONT_PART) return M_FAILURE_GEN;

	if (GrowString(s, length) != M_SUCCESS) return M_FAILURE_GEN;

	if (contained == M_CONT_FULL) // append string is contained in target String
	{
		memmove(s->data + s->length, s->data + off, appendLen);
	}
	else
	{
		memmove(s->data + s->length, append, appendLen);
	}
	s->data[length] = '\0';
	s->length = length;

	return M_SUCCESS;
}

int StringAppendString(String* s, const String* append)
{
	if (append == NULL) return M_FAILURE_GEN;

	return AppendStringBytes(s, append->data, append->length);
}

int StringAppendCStr(String* s, const char append[])
{
	if (append == NULL) return M_FAILURE_GEN;

	return AppendStringBytes(s, append, strlen(append));
}

int StringInsertCharAt(String* s, size_t index, char chara)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (index > s->length) return M_FAILURE_GEN;

	if (s->length > SIZE_MAX - 1) return M_FAILURE_GEN;
	size_t length = s->length + 1;
	if (GrowString(s, length) != M_SUCCESS) return M_FAILURE_GEN;

	size_t moveLength = length - index;
	memmove(s->data + index + 1, s->data + index, moveLength);
	s->data[index] = chara;
	s->data[length] = '\0';
	s->length++;

	return M_SUCCESS;
}

static int InsertStringBytes(String* s, size_t index, const char insert[], size_t insertLen)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (index > s->length) return M_FAILURE_GEN;
	if (insert == NULL || insertLen == 0) return M_SUCCESS;

	size_t insertOff = 0;
	int contained = PointsInto(s->data, s->length, insert, insertLen, &insertOff);
	if (contained == M_CONT_PART) return M_FAILURE_GEN;

	if (s->length > SIZE_MAX - insertLen) return M_FAILURE_GEN;
	size_t length = s->length + insertLen;

	bool needTemp = contained == M_CONT_FULL && index > insertOff && index < insertOff + insertLen;
	char* temp = NULL;
	if (needTemp)
	{
		temp = malloc(insertLen);
		if (temp == NULL) return M_FAILURE_GEN;
		memcpy(temp, insert, insertLen);
	}

	size_t moveLength = s->length - index;

	if (GrowString(s, length) != M_SUCCESS) goto FAILURE;

	if (contained == M_CONT_FULL)
	{
		memmove(s->data + index + insertLen, s->data + index, moveLength);
		if (needTemp)
		{
			memcpy(s->data + index, temp, insertLen);
			free(temp);
			temp = NULL;
		}
		else
		{
			if (index <= insertOff) insertOff += insertLen;
			memmove(s->data + index, s->data + insertOff, insertLen);
		}
	}
	else
	{
		memmove(s->data + index + insertLen, s->data + index, moveLength);
		memcpy(s->data + index, insert, insertLen);
	}
	s->data[length] = '\0';
	s->length = length;

	return M_SUCCESS;

FAILURE:
	if (temp != NULL) free(temp);
	return M_FAILURE_GEN;
}

int StringInsertStringAt(String* s, size_t index, const String* insert)
{
	if (s == NULL || insert == NULL) return M_FAILURE_GEN;
	
	return InsertStringBytes(s, index, insert->data, insert->length);
}

int StringInsertCStrAt(String* s, size_t index, const char insert[])
{
	if (s == NULL || insert == NULL) return M_FAILURE_GEN;

	return InsertStringBytes(s, index, insert, strlen(insert));
}

static int ConcatCStrings(String* dest, const char s1[], size_t s1Len, const char s2[], size_t s2Len) // Length parameters must be 0 for NULL pointers
{
	if (dest == NULL) return M_FAILURE_GEN;

	size_t s1Off = 0;
	size_t s2Off = 0;
	int s1Contained = PointsInto(dest->data, dest->capacity + 1, s1, s1Len, &s1Off);
	int s2Contained = PointsInto(dest->data, dest->capacity + 1, s2, s2Len, &s2Off);

	if (s1Contained == M_CONT_PART || s2Contained == M_CONT_PART) return M_FAILURE_GEN;
	if (s1Len == 0) s1Contained = M_CONT_NONE;
	if (s2Len == 0) s2Contained = M_CONT_NONE;

	bool bothContained = s1Contained == M_CONT_FULL && s1Len > 0 && s2Contained == M_CONT_FULL && s2Len > 0;
	bool swapRequired = bothContained && s2Off < s1Len && s1Off != 0 && s1Off < s1Len + s2Len;

	if (s1Len > SIZE_MAX - s2Len) return M_FAILURE_GEN;
	size_t length = s1Len + s2Len;

	bool s1Stored = false;
	char* temp = NULL;
	if (swapRequired)
	{
		if (s1Len <= s2Len)
		{
			temp = malloc(s1Len);
			if (temp == NULL) return M_FAILURE_GEN;

			memcpy(temp, s1, s1Len);
			s1Stored = true;
		}
		else
		{
			temp = malloc(s2Len);
			if (temp == NULL) return M_FAILURE_GEN;

			memcpy(temp, s2, s2Len);
			s1Stored = false;
		}
	}

	if (GrowString(dest, length) != M_SUCCESS) goto STRING_FAILURE;

	if (bothContained)
	{
		if (swapRequired)
		{
			if (s1Stored)
			{
				memmove(dest->data + s1Len, dest->data + s2Off, s2Len);
				memcpy(dest->data, temp, s1Len);
			}
			else
			{
				memmove(dest->data, dest->data + s1Off, s1Len);
				memcpy(dest->data + s1Len, temp, s2Len);
			}
			free(temp);
		}
		else
		{
			if (s2Off >= s1Len)
			{
				memmove(dest->data, dest->data + s1Off, s1Len);
				memmove(dest->data + s1Len, dest->data + s2Off, s2Len);
			}
			else
			{
				memmove(dest->data + s1Len, dest->data + s2Off, s2Len);
				memmove(dest->data, dest->data + s1Off, s1Len);
			}
		}
		dest->data[length] = '\0';
		dest->length = length;
		return M_SUCCESS;
	}

	if (s1Contained == M_CONT_FULL)
	{
		memmove(dest->data, dest->data + s1Off, s1Len);
		if (s2Len > 0) memcpy(dest->data + s1Len, s2, s2Len);
	}
	else if (s2Contained == M_CONT_FULL)
	{
		memmove(dest->data + s1Len, dest->data + s2Off, s2Len);
		if (s1Len > 0) memcpy(dest->data, s1, s1Len);
	}
	else
	{
		if (s1Len > 0) memcpy(dest->data, s1, s1Len);
		if (s2Len > 0) memcpy(dest->data + s1Len, s2, s2Len);
	}

	dest->data[length] = '\0';
	dest->length = length;
	return M_SUCCESS;

STRING_FAILURE:
	if (temp != NULL) free(temp);
	return M_FAILURE_GEN;
}

int ConcatStrings(String* dest, const String* s1, const String* s2)
{
	if (dest == NULL || s1 == NULL || s2 == NULL) return M_FAILURE_GEN;

	return ConcatCStrings(dest, s1->data, s1->length, s2->data, s2->length);
}

int ConcatStringCStr(String* dest, const String* s1, const char s2[])
{
	if (dest == NULL || s1 == NULL || s2 == NULL) return M_FAILURE_GEN;

	return ConcatCStrings(dest, s1->data, s1->length, s2, strlen(s2));
}

int ConcatCStrString(String* dest, const char s1[], const String* s2)
{
	if (dest == NULL || s1 == NULL || s2 == NULL) return M_FAILURE_GEN;

	return ConcatCStrings(dest, s1, strlen(s1), s2->data, s2->length);
}

int ReverseString(String* s)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (s->length < 2) return M_SUCCESS;

	size_t halfLen = s->length / 2;
	for (size_t i = 0; i < halfLen; ++i)
	{
		size_t otherIndex = s->length - i - 1;
		char temp = s->data[i];
		s->data[i] = s->data[otherIndex];
		s->data[otherIndex] = temp;
	}
	s->data[s->length] = '\0';

	return M_SUCCESS;
}

int SplitString(StringSplit* split, const String* s, size_t index)
{
	if (split == NULL || s == NULL) return M_FAILURE_GEN;

	size_t length = s->length;
	size_t len1 = index < length ? index : length;
	size_t len2 = length - len1;

	if (GrowString(&split->string1, len1) != M_SUCCESS) return M_FAILURE_GEN;
	if (GrowString(&split->string2, len2) != M_SUCCESS) return M_FAILURE_GEN;

	if (len1 > 0) memmove(split->string1.data, s->data, len1);
	if (len2 > 0) memmove(split->string2.data, s->data + len1, len2);

	split->string1.data[len1] = '\0';
	split->string1.length = len1;
	split->string2.data[len2] = '\0';
	split->string2.length = len2;

	return M_SUCCESS;
}

int StringTrimStart(String* s, size_t count)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (s->data == NULL || count == 0) return M_SUCCESS;

	if (count >= s->length)
	{
		if (ClearString(s) != M_SUCCESS) return M_FAILURE_GEN;
		return M_SUCCESS;
	}

	memmove(s->data, s->data + count, s->length - count);
	s->data[s->length - count] = '\0';
	s->length -= count;

	return M_SUCCESS;
}

int StringTrimEnd(String* s, size_t count)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (s->data == NULL || count == 0) return M_SUCCESS;

	if (count >= s->length)
	{
		if (ClearString(s) != M_SUCCESS) return M_FAILURE_GEN;
		return M_SUCCESS;
	}

	s->data[s->length - count] = '\0';
	s->length -= count;

	return M_SUCCESS;
}

int StringTrim(String* s, size_t start, size_t end)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (start > end) return M_FAILURE_GEN;

	if (s->data == NULL) return M_SUCCESS;
	if (start == 0 && end >= s->length) return M_SUCCESS;

	if (start >= s->length)
	{
		if (ClearString(s) != M_SUCCESS) return M_FAILURE_GEN;
		return M_SUCCESS;
	}
	end = min(end, s->length);

	size_t length = end - start;
	memmove(s->data, s->data + start, length);
	s->data[length] = '\0';
	s->length = length;

	return M_SUCCESS;
}

int StringExtractSubstring(String* dest, const String* s, size_t start, size_t end)
{
	if (dest == NULL || s == NULL) return M_FAILURE_GEN;

	if (start > end) return M_FAILURE_GEN;

	if (s->data == NULL)
	{
		if (ClearString(dest) != M_SUCCESS) return M_FAILURE_GEN;
		return M_SUCCESS;
	}
	if (start == 0 && end >= s->length)
	{
		return CopyStringTo(dest, s);
	}

	if (start >= s->length)
	{
		if (ClearString(dest) != M_SUCCESS) return M_FAILURE_GEN;
		return M_SUCCESS;
	}

	end = min(end, s->length);

	size_t length = end - start;
	if (GrowString(dest, length) != M_SUCCESS) return M_FAILURE_GEN;

	memmove(dest->data, s->data + start, length);
	dest->data[length] = '\0';
	dest->length = length;

	return M_SUCCESS;
}

bool StringsEqual(const void* s1, const void* s2)
{
	if (s1 == NULL || s2 == NULL) return false;

	String* s1Str = (String*)s1;
	String* s2Str = (String*)s2;

	if (s1Str->length == 0 && s2Str->length == 0) return true;

	if (s1Str->length != s2Str->length) return false;

	if (memcmp(s1Str->data, s2Str->data, s1Str->length) == 0) return true;

	return false;
}

bool StringsEqualCStr(const String* s1, const char s2[])
{
	if (s1 == NULL || s2 == NULL) return false;

	size_t s2Len = strlen(s2);
	if ((s1->data == NULL || s1->length == 0) && s2Len == 0) return true;

	if (s1->length != s2Len) return false;

	if (memcmp(s1->data, s2, s1->length) == 0) return true;

	return false;
}

static bool StringContainsBytes(const String* s, const char sub[], size_t subLen)
{
	if (s->length < subLen) return false;

	for (size_t i = 0; i <= s->length - subLen; ++i)
	{
		if (memcmp(s->data + i, sub, subLen) == 0) return true;
	}
	return false;
}

bool StringContainsSubstring(const String* s, const String* sub)
{
	if (s == NULL || sub == NULL) return false;

	if (sub->data == NULL || sub->length == 0) return true;
	if (s->data == NULL || s->length == 0) return false;

	if (sub->length > s->length) return false;

	return StringContainsBytes(s, sub->data, sub->length);
}

bool StringContainsCStr(const String* s, const char sub[])
{
	if (s == NULL || sub == NULL) return false;

	size_t subLen = strlen(sub);
	if (subLen == 0) return true;
	if (s->data == NULL || s->length == 0) return false;

	if (subLen > s->length) return false;

	return StringContainsBytes(s, sub, subLen);
}

int StringRemoveCharAt(String* s, size_t index)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (index >= s->length) return M_FAILURE_GEN;

	size_t moveLength = s->length - index - 1;
	memmove(s->data + index, s->data + index + 1, moveLength);
	s->data[s->length - 1] = '\0';
	s->length--;

	return M_SUCCESS;
}

int StringRemoveRange(String* s, size_t start, size_t length)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (start > SIZE_MAX - length) return M_FAILURE_GEN;
	if (start + length >= s->length) return M_FAILURE_GEN;
	if (length == 0) return M_SUCCESS;

	size_t moveLength = s->length - start - length;
	memmove(s->data + start, s->data + start + length, moveLength);
	s->data[s->length - length] = '\0';
	s->length -= length;

	return M_SUCCESS;
}

int StringRemoveCharFirst(String* s, char chara)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (s->length == 0) return M_SUCCESS;

	for (size_t i = 0; i < s->length; ++i)
	{
		if (s->data[i] == chara)
		{
			size_t moveLength = s->length - i - 1;
			memmove(s->data + i, s->data + i + 1, moveLength);
			s->data[s->length - 1] = '\0';
			s->length--;
			break;
		}
	}

	return M_SUCCESS;
}

int StringRemoveCharAll(String* s, char chara)
{
	if (s == NULL) return M_FAILURE_GEN;
	if (s->length == 0) return M_SUCCESS;

	size_t sLen = s->length, removed = 0;
	for (size_t i = 0; i < sLen; ++i)
	{
		size_t dataIndex = i - removed;
		if (s->data[dataIndex] == chara)
		{
			size_t moveLength = s->length - dataIndex - 1;
			memmove(s->data + dataIndex, s->data + dataIndex + 1, moveLength);
			s->data[s->length - 1] = '\0';
			s->length--;
			removed++;
		}
	}

	return M_SUCCESS;
}

static int RemoveCStrFromStringFirst(String* s, const char sub[], size_t subLen)
{
	if (s == NULL || sub == NULL) return M_FAILURE_GEN;
	if (s->length == 0 || subLen == 0) return M_SUCCESS;
	if (subLen > s->length) return M_SUCCESS;

	for (size_t i = 0; i <= s->length - subLen; ++i)
	{
		if (memcmp(s->data + i, sub, subLen) == 0)
		{
			size_t moveLength = s->length - i - subLen;
			memmove(s->data + i, s->data + i + subLen, moveLength);
			s->data[s->length - subLen] = '\0';
			s->length -= subLen;
			break;
		}
	}

	return M_SUCCESS;
}

static int RemoveCStrFromStringAll(String* s, const char sub[], size_t subLen)
{
	if (s == NULL || sub == NULL) return M_FAILURE_GEN;
	if (s->length == 0 || subLen == 0) return M_SUCCESS;
	if (subLen > s->length) return M_SUCCESS;

	int contained = PointsInto(s->data, s->length, sub, subLen, NULL);
	if (contained == M_CONT_PART) return M_FAILURE_GEN;

	const char* compare = sub;

	char* subTemp = NULL;
	if (contained == M_CONT_FULL)
	{
		subTemp = malloc(subLen);
		if (subTemp == NULL) return M_FAILURE_GEN;
		
		memcpy(subTemp, sub, subLen);
		compare = subTemp;
	}

	for (size_t i = 0; i <= s->length - subLen; ++i)
	{
		if (memcmp(s->data + i, compare, subLen) == 0)
		{
			size_t moveLength = s->length - i - subLen;
			memmove(s->data + i, s->data + i + subLen, moveLength);
			s->data[s->length - subLen] = '\0';
			s->length -= subLen;
			i--;
		}
	}

	if (subTemp != NULL) free(subTemp);
	return M_SUCCESS;
}

int StringRemoveSubstringFirst(String* s, const String* substring)
{
	if (s == NULL || substring == NULL) return M_FAILURE_GEN;
	if (s->length == 0 || substring->length == 0) return M_SUCCESS;
	if (substring->length > s->length) return M_SUCCESS;

	if (s == substring)
	{
		if (ClearString(s) != M_SUCCESS) return M_FAILURE_GEN;
		return M_SUCCESS;
	}

	return RemoveCStrFromStringFirst(s, substring->data, substring->length);
}

int StringRemoveSubstringAll(String* s, const String* substring)
{
	if (s == NULL || substring == NULL) return M_FAILURE_GEN;
	if (s->length == 0 || substring->length == 0) return M_SUCCESS;
	if (substring->length > s->length) return M_SUCCESS;

	if (s == substring)
	{
		if (ClearString(s) != M_SUCCESS) return M_FAILURE_GEN;
		return M_SUCCESS;
	}

	return RemoveCStrFromStringAll(s, substring->data, substring->length);
}

int StringRemoveCStrFirst(String* s, const char substring[])
{
	if (s == NULL || substring == NULL) return M_FAILURE_GEN;
	
	size_t substringLen = strlen(substring);
	if (s->length == 0 || substringLen == 0) return M_SUCCESS;
	if (substringLen > s->length) return M_SUCCESS;

	return RemoveCStrFromStringFirst(s, substring, substringLen);
}

int StringRemoveCStrAll(String* s, const char substring[])
{
	if (s == NULL || substring == NULL) return M_FAILURE_GEN;

	size_t substringLen = strlen(substring);
	if (s->length == 0 || substringLen == 0) return M_SUCCESS;
	if (substringLen > s->length) return M_SUCCESS;

	return RemoveCStrFromStringAll(s, substring, substringLen);
}