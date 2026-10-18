#include "count_items.h"

#include "../include/m_result.h"
#include "../include/m_string.h"
#include "../include/m_vector.h"
#include "../utils/wide_utf8.h"

#include <direct.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

static void PrintCounts(size_t total, size_t files, size_t dirs)
{
	printf("\nThe current working directory contains:\n");
	
	printf("  %zu total item", total);
	if (total != 1) printf("s");
	printf("\n");

	printf("  %zu file", files);
	if (files != 1) printf("s");
	printf("\n");

	printf("  %zu director", dirs);
	if (dirs == 1) printf("y");
	else printf("ies");
	printf("\n");
}

static int ScanDirectory(const String* path, Vector* directories, size_t* totalCount, size_t* fileCount, size_t* dirCount)
{
	if (path == NULL || directories == NULL || totalCount == NULL || fileCount == NULL || dirCount == NULL) return M_FAILURE_GEN;

	int status = M_SUCCESS;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	String searchPath = CopyString(path);
	if (searchPath.data == NULL) return M_FAILURE_GEN;
	if (StringAppendCStr(&searchPath, "\\*") != M_SUCCESS)
	{
		DestroyString(&searchPath);
		return M_FAILURE_GEN;
	}

	String parentPath = CopyString(path);
	if (parentPath.data == NULL)
	{
		DestroyString(&searchPath);
		return M_FAILURE_GEN;
	}
	if (StringAppendCStr(&parentPath, "\\") != M_SUCCESS) goto FAILURE;

	WIN32_FIND_DATAW findData;

	wchar_t* wideSearchPath = UTF8ToWide(searchPath.data);
	if (wideSearchPath == NULL) goto FAILURE;

	hFind = FindFirstFileW(wideSearchPath, &findData);
	free(wideSearchPath);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		if (error == ERROR_ACCESS_DENIED || error == ERROR_SHARING_VIOLATION)
		{
			// not a fatal error - simply skip current directory
			DestroyString(&searchPath);
			DestroyString(&parentPath);
			return M_SUCCESS;
		}
		goto FAILURE;
	}

	do
	{
		++(*totalCount);

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;

			char nameUtf8[MAX_PATH * 4];
			if (WideToUTF8(findData.cFileName, nameUtf8, sizeof(nameUtf8)) != M_SUCCESS) continue;

			int loopStatus = M_SUCCESS;

			String* directoryPath = malloc(sizeof(String));
			if (directoryPath == NULL) goto FAILURE;

			*directoryPath = CopyString(&parentPath);
			if (directoryPath->data == NULL) goto DIR_PATH_FAILURE;
			if (StringAppendCStr(directoryPath, nameUtf8) != M_SUCCESS) goto DIR_FAILURE;

			if (VectorPushBack(directories, directoryPath, NULL, NULL) != M_SUCCESS) goto DIR_FAILURE;
			++(*dirCount);

			goto DIR_LOOP_END;

		DIR_FAILURE:
			DestroyString(directoryPath);

		DIR_PATH_FAILURE:
			loopStatus = M_FAILURE_GEN;

		DIR_LOOP_END:
			free(directoryPath);
			if (loopStatus != M_SUCCESS) goto FAILURE;
		}
		else ++(*fileCount);
	} while (FindNextFileW(hFind, &findData));

	goto CLEANUP;

FAILURE:
	status = M_FAILURE_GEN;

CLEANUP:
	if (hFind != INVALID_HANDLE_VALUE) FindClose(hFind);
	DestroyString(&searchPath);
	DestroyString(&parentPath);

	return status;
}

static int CountAllItems(size_t* totalCount, size_t* fileCount, size_t* dirCount)
{
	if (totalCount == NULL || fileCount == NULL || dirCount == NULL) return M_FAILURE_GEN;

	int status = M_SUCCESS;

	wchar_t* wideWorkingDir = _wgetcwd(NULL, 0);
	if (wideWorkingDir == NULL) return M_FAILURE_GEN;
	char workingDir[MAX_PATH * 4];
	if (WideToUTF8(wideWorkingDir, workingDir, sizeof(workingDir)) != M_SUCCESS)
	{
		free(wideWorkingDir);
		return M_FAILURE_GEN;
	}
	free(wideWorkingDir);

	String startPath = CreateString(workingDir);
	if (startPath.data == NULL) return M_FAILURE_GEN;

	Vector searchDirs = CreateVector(NULL, 0, sizeof(String), DestroyString, NULL);
	if (searchDirs.itemSize == 0) goto FAILURE;
	if (VectorPushBack(&searchDirs, &startPath, ZeroString, CopyStringTo) != M_SUCCESS) goto FAILURE;

	do
	{
		const String* search = VectorGetElementLast(&searchDirs);
		if (search == NULL)
		{
			Destructor destructorTemp = searchDirs.destructor;
			searchDirs.destructor = NULL;
			VectorPopBack(&searchDirs);
			searchDirs.destructor = destructorTemp;
			continue;
		}

		Vector foundDirs = CreateVector(NULL, 0, sizeof(String), DestroyString, NULL);
		if (foundDirs.itemSize == 0) goto BREAK_SEARCH;

		if (ScanDirectory(search, &foundDirs, totalCount, fileCount, dirCount) != M_SUCCESS) goto BREAK_SEARCH;
		if (VectorPopBack(&searchDirs) != M_SUCCESS) goto BREAK_SEARCH;

		if (foundDirs.length > 0)
		{
			if (VectorInsertRangeAt(&searchDirs, searchDirs.length, &foundDirs, ZeroString, CopyStringTo) != M_SUCCESS) goto BREAK_SEARCH;
		}

		DestroyVector(&foundDirs);
		continue;

	BREAK_SEARCH:
		if (foundDirs.itemSize == 0) foundDirs.itemSize = sizeof(String);
		DestroyVector(&foundDirs);
		goto FAILURE;
	} while (searchDirs.length > 0);

	goto CLEANUP;

FAILURE:
	if (searchDirs.itemSize == 0) searchDirs.itemSize = sizeof(String);
	status = M_FAILURE_GEN;

CLEANUP:
	DestroyString(&startPath);
	DestroyVector(&searchDirs);

	return status;
}

int CountItemsInDirectory(void)
{
	struct timespec start, end;
	if (timespec_get(&start, TIME_UTC) == 0) return M_FAILURE_GEN;

	printf("Counting all items in the current working directory...\n");

	size_t total = 0, files = 0, dirs = 0;
	if (CountAllItems(&total, &files, &dirs) != M_SUCCESS) return M_FAILURE_GEN;

	PrintCounts(total, files, dirs);

	if (timespec_get(&end, TIME_UTC) != 0)
	{
		double timeSpent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
		printf("\nSearch took %f seconds\n", timeSpent);
	}
	else
	{
		printf("\nAn error occurred and the search time could not be determined\n");
	}

	return M_SUCCESS;
}