#ifndef M_WIDE_TO_UTF8
#define M_WIDE_TO_UTF8

#include <wchar.h>

int WideToUTF8(const wchar_t* wide, char* out, int outSize);
wchar_t* UTF8ToWide(const char* utf8);

#endif