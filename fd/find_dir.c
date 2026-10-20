#include "find_dir.h"

#include "../include/m_result.h"
#include "../include/m_string.h"
#include "../include/m_vector.h"
#include "../utils/wide_utf8.h"

#include <direct.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

typedef struct
{
	String name;
	String path;
} Directory;

static Directory CreateDirectoryWrapper(const String* name, const String* path)
{
	Directory directory = { CopyString(name), CopyString(path) };
	return directory;
}

static int DestroyDirectory(void* d)
{
	if (d == NULL) return M_FAILURE_GEN;

	Directory* dPtr = (Directory*)d;

	if (DestroyString(&dPtr->name) != M_SUCCESS) return M_FAILURE_GEN;
	if (DestroyString(&dPtr->path) != M_SUCCESS) return M_FAILURE_GEN;

	return M_SUCCESS;
}

static int ZeroDirectory(void* d)
{
	if (d == NULL) return M_FAILURE_GEN;

	Directory* dPtr = (Directory*)d;

	if (ZeroString(&dPtr->name) != M_SUCCESS) return M_FAILURE_GEN;
	if (ZeroString(&dPtr->path) != M_SUCCESS) return M_FAILURE_GEN;

	return M_SUCCESS;
}

static int CopyDirectoryTo(void* dest, const void* source)
{
	if (dest == NULL || source == NULL) return M_FAILURE_GEN;

	Directory* destPtr = (Directory*)dest;
	const Directory* sourcePtr = (Directory*)source;

	if (CopyStringTo(&destPtr->name, &sourcePtr->name) != M_SUCCESS) return M_FAILURE_GEN;
	if (CopyStringTo(&destPtr->path, &sourcePtr->path) != M_SUCCESS) return M_FAILURE_GEN;

	return M_SUCCESS;
}

static void PrintDirectories(const Vector* directories)
{
	if (directories->length == 0)
	{
		printf("No directories found matching the query.\n");
		return;
	}

	if (directories->length == 1)
	{
		printf("Found 1 directory");
	}
	else
	{
		printf("Found %zu directories", directories->length);
	}
	printf(" matching the query:\n");

	for (size_t i = 0; i < directories->length; ++i)
	{
		const Directory* dir = VectorGetElementAt(directories, i);
		if (dir != NULL) PrintStringLn(&dir->path);
	}
}

static int GetAllDirectoriesInDir(const String* path, Vector* directories)
{
	if (path == NULL || directories == NULL) return M_FAILURE_GEN;

	int status = M_SUCCESS;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	// Prepare search path to match all contents
	String searchPath = CopyString(path);
	if (searchPath.data == NULL) return M_FAILURE_GEN;
	if (StringAppendCStr(&searchPath, "\\*") != M_SUCCESS)
	{
		DestroyString(&searchPath);
		return M_FAILURE_GEN;
	}

	// Prepare parent directory path to prepend to every found directory name
	String parentPath = CopyString(path);
	if (parentPath.data == NULL)
	{
		DestroyString(&searchPath);
		return M_FAILURE_GEN;
	}
	if (StringAppendCStr(&parentPath, "\\") != M_SUCCESS) goto FAILURE;

	// Find first item in the directory
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

	// Iterate through all items in the directory and store all found directories
	do
	{
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
			if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;

			int loopStatus = M_SUCCESS;

			char nameUtf8[MAX_PATH * 4];
			if (WideToUTF8(findData.cFileName, nameUtf8, sizeof(nameUtf8)) != M_SUCCESS) continue;

			// Create strings storing directory name and path
			String directoryName = CreateString(nameUtf8);
			String directoryPath = CopyString(&parentPath);
			if (directoryName.data == NULL || directoryPath.data == NULL) goto BREAK_SEARCH_LOOP;
			if (StringAppendCStr(&directoryPath, nameUtf8) != M_SUCCESS) goto BREAK_SEARCH_LOOP;

			Directory directory = CreateDirectoryWrapper(&directoryName, &directoryPath);
			if (VectorPushBack(directories, &directory, NULL, NULL) != M_SUCCESS)
			{
				DestroyDirectory(&directory);
				goto BREAK_SEARCH_LOOP;
			}

			goto SEARCH_LOOP_END;

		BREAK_SEARCH_LOOP:
			loopStatus = M_FAILURE_GEN;

		SEARCH_LOOP_END:
			DestroyString(&directoryName);
			DestroyString(&directoryPath);

			if (loopStatus != M_SUCCESS) goto FAILURE;
		}
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

static int FindAllDirectories(Vector* directories, const String* query)
{
	if (directories == NULL || query == NULL) return M_FAILURE_GEN;

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

	Directory startDirectory = CreateDirectoryWrapper(&startPath, &startPath);

	Vector searchDirs = CreateVector(NULL, 0, sizeof(Directory), DestroyDirectory, NULL);
	if (searchDirs.itemSize == 0) goto FAILURE;
	if (VectorPushBack(&searchDirs, &startDirectory, ZeroDirectory, CopyDirectoryTo) != M_SUCCESS) goto FAILURE;

	do
	{
		const Directory* search = VectorGetElementLast(&searchDirs);
		if (search == NULL)
		{
			Destructor destructorTemp = searchDirs.destructor;
			searchDirs.destructor = NULL;
			VectorPopBack(&searchDirs);
			searchDirs.destructor = destructorTemp;
			continue;
		}
		const String* searchPath = &search->path;

		Vector foundDirs = CreateVector(NULL, 0, sizeof(Directory), DestroyDirectory, NULL);
		if (foundDirs.itemSize == 0) goto BREAK_SEARCH;

		if (GetAllDirectoriesInDir(searchPath, &foundDirs) != M_SUCCESS) goto BREAK_SEARCH;
		if (VectorPopBack(&searchDirs) != M_SUCCESS) goto BREAK_SEARCH;

		if (foundDirs.length > 0)
		{
			if (VectorInsertRangeAt(&searchDirs, searchDirs.length, &foundDirs, ZeroDirectory, CopyDirectoryTo) != M_SUCCESS) goto BREAK_SEARCH;

			if (query->length == 0)
			{
				if (VectorInsertRangeAt(directories, directories->length, &foundDirs, ZeroDirectory, CopyDirectoryTo) != M_SUCCESS) goto BREAK_SEARCH;
			}
			else
			{
				for (size_t i = 0; i < foundDirs.length; ++i)
				{
					const Directory* dir = VectorGetElementAt(&foundDirs, i);
					if (dir == NULL) continue;

					if (StringContainsSubstring(&dir->name, query))
					{
						if (VectorPushBack(directories, dir, ZeroDirectory, CopyDirectoryTo) != M_SUCCESS) goto BREAK_SEARCH;
					}
				}
			}
		}

		DestroyVector(&foundDirs);
		continue;

	BREAK_SEARCH:
		if (foundDirs.itemSize == 0) foundDirs.itemSize = sizeof(Directory);
		DestroyVector(&foundDirs);
		goto FAILURE;
	} while (searchDirs.length > 0);

	goto CLEANUP;

FAILURE:
	status = M_FAILURE_GEN;
	if (searchDirs.itemSize == 0) searchDirs.itemSize = sizeof(Directory);

CLEANUP:
	DestroyString(&startPath);
	DestroyDirectory(&startDirectory);
	DestroyVector(&searchDirs);

	return status;
}

int FindAllDirectoriesByQuery(const String* query)
{
	if (query == NULL) return M_FAILURE_GEN;

	struct timespec start, end;
	if (timespec_get(&start, TIME_UTC) == 0) return M_FAILURE_GEN;

	printf("Finding all directories with names containing \"");
	PrintString(query);
	printf("\" in the current working directory...\n\n");

	Vector directories = CreateVector(NULL, 0, sizeof(Directory), DestroyDirectory, NULL);
	if (directories.itemSize == 0) goto FAILURE;

	if (FindAllDirectories(&directories, query) != M_SUCCESS) goto FAILURE;

	PrintDirectories(&directories);

	DestroyVector(&directories);

	if (timespec_get(&end, TIME_UTC) != 0)
	{
		double timeSpent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
		printf("\nSearch took %f seconds\n", timeSpent);
	}
	else
	{
		printf("\nAn error occured and the search time could not be determined\n");
	}

	return M_SUCCESS;

FAILURE:
	if (directories.itemSize == 0) directories.itemSize = sizeof(Directory);
	DestroyVector(&directories);

	return M_FAILURE_GEN;
}