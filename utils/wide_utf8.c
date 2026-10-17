#include "wide_utf8.h"
#include "../include/m_result.h"

#include <windows.h>

int WideToUTF8(const wchar_t* wide, char* out, int outSize)
{
	return WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, outSize, NULL, NULL) > 0 ? M_SUCCESS : M_FAILURE_GEN;
}

wchar_t* UTF8ToWide(const char* utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (len <= 0) return NULL;

	wchar_t* wide = malloc(len * sizeof(wchar_t));
	if (wide == NULL) return NULL;

	if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, len) == 0)
	{
		free(wide);
		return NULL;
	}

	return wide;
}