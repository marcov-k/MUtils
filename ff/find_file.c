#include "find_file.h"

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
} File;

static File CreateFileWrapper(const String* name, const String* path)
{
	File file = { CopyString(name), CopyString(path) };
	return file;
}

static int DestroyFile(void* f)
{
	if (f == NULL) return M_FAILURE_GEN;

	File* fPtr = (File*)f;

	if (DestroyString(&fPtr->name) != M_SUCCESS) return M_FAILURE_GEN;
	if (DestroyString(&fPtr->path) != M_SUCCESS) return M_FAILURE_GEN;

	return M_SUCCESS;
}

static int ZeroFile(void* f)
{
	if (f == NULL) return M_FAILURE_GEN;

	File* fPtr = (File*)f;

	if (ZeroString(&fPtr->name) != M_SUCCESS) return M_FAILURE_GEN;
	if (ZeroString(&fPtr->path) != M_SUCCESS) return M_FAILURE_GEN;

	return M_SUCCESS;
}

static int CopyFileTo(void* dest, const void* source)
{
	if (dest == NULL || source == NULL) return M_FAILURE_GEN;

	File* destPtr = (File*)dest;
	const File* sourcePtr = (File*)source;

	if (CopyStringTo(&destPtr->name, &sourcePtr->name) != M_SUCCESS) return M_FAILURE_GEN;
	if (CopyStringTo(&destPtr->path, &sourcePtr->path) != M_SUCCESS) return M_FAILURE_GEN;

	return M_SUCCESS;
}

static void PrintFiles(const Vector* files)
{
	if (files->length == 0)
	{
		printf("No files found matching the query.\n");
		return;
	}

	if (files->length == 1)
	{
		printf("Found 1 file");
	}
	else
	{
		printf("Found %zu files", files->length);
	}
	printf(" matching the query:\n");

	for (size_t i = 0; i < files->length; ++i)
	{
		const File* file = VectorGetElementAt(files, i);
		if (file != NULL) PrintStringLn(&file->path);
	}
}

static int ScanDirectory(const String* path, Vector* files, Vector* directories)
{
	if (path == NULL || files == NULL || directories == NULL) return M_FAILURE_GEN;

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
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;

		int loopStatus = M_SUCCESS;

		char nameUtf8[MAX_PATH * 4];
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;

			if (WideToUTF8(findData.cFileName, nameUtf8, sizeof(nameUtf8)) != M_SUCCESS) continue;

			String* directoryPath = malloc(sizeof(String));
			if (directoryPath == NULL) goto FAILURE;
			
			*directoryPath = CopyString(&parentPath);
			if (directoryPath->data == NULL) goto DIR_PATH_FAILURE;
			if (StringAppendCStr(directoryPath, nameUtf8) != M_SUCCESS) goto DIR_FAILURE;

			if (VectorPushBack(directories, directoryPath, NULL, NULL) != M_SUCCESS) goto DIR_FAILURE;

			goto DIR_LOOP_END;

		DIR_FAILURE:
			DestroyString(directoryPath);

		DIR_PATH_FAILURE:
			loopStatus = M_FAILURE_GEN;

		DIR_LOOP_END:
			free(directoryPath);
			if (loopStatus != M_SUCCESS) goto FAILURE;
		}
		else
		{
			if (WideToUTF8(findData.cFileName, nameUtf8, sizeof(nameUtf8)) != M_SUCCESS) continue;

			String fileName = CreateString(nameUtf8);
			String filePath = CopyString(&parentPath);
			if (fileName.data == NULL || filePath.data == NULL) goto FILE_FAILURE;
			if (StringAppendCStr(&filePath, nameUtf8) != M_SUCCESS) goto FILE_FAILURE;

			File* file = malloc(sizeof(File));
			if (file == NULL) goto FILE_FAILURE;

			*file = CreateFileWrapper(&fileName, &filePath);
			if (VectorPushBack(files, file, NULL, NULL) != M_SUCCESS)
			{
				DestroyFile(file);
				free(file);
				goto FILE_FAILURE;
			}

			goto FILE_LOOP_END;

		FILE_FAILURE:
			loopStatus = M_FAILURE_GEN;

		FILE_LOOP_END:
			DestroyString(&fileName);
			DestroyString(&filePath);

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

static int FindAllFiles(Vector* files, const String* query)
{
	if (files == NULL || query == NULL) return M_FAILURE_GEN;

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

		Vector foundFiles = CreateVector(NULL, 0, sizeof(File), DestroyFile, NULL);
		Vector foundDirs = CreateVector(NULL, 0, sizeof(String), DestroyString, NULL);
		if (foundFiles.itemSize == 0 || foundDirs.itemSize == 0) goto BREAK_SEARCH;

		if (ScanDirectory(search, &foundFiles, &foundDirs) != M_SUCCESS) goto BREAK_SEARCH;
		if (VectorPopBack(&searchDirs) != M_SUCCESS) goto BREAK_SEARCH;

		if (foundFiles.length > 0)
		{
			if (query->length == 0)
			{
				if (VectorInsertRangeAt(files, files->length, &foundFiles, ZeroFile, CopyFileTo) != M_SUCCESS) goto BREAK_SEARCH;
			}
			else
			{
				for (size_t i = 0; i < foundFiles.length; ++i)
				{
					const File* file = VectorGetElementAt(&foundFiles, i);
					if (file == NULL) continue;

					if (StringContainsSubstring(&file->name, query))
					{
						if (VectorPushBack(files, file, ZeroFile, CopyFileTo) != M_SUCCESS) goto BREAK_SEARCH;
					}
				}
			}
		}

		if (foundDirs.length > 0)
		{
			if (VectorInsertRangeAt(&searchDirs, searchDirs.length, &foundDirs, ZeroString, CopyStringTo) != M_SUCCESS) goto BREAK_SEARCH;
		}
		
		DestroyVector(&foundFiles);
		DestroyVector(&foundDirs);
		continue;

	BREAK_SEARCH:
		if (foundFiles.itemSize == 0) foundFiles.itemSize = sizeof(File);
		if (foundDirs.itemSize == 0) foundDirs.itemSize = sizeof(String);
		DestroyVector(&foundFiles);
		DestroyVector(&foundDirs);
		goto FAILURE;
	} while (searchDirs.length > 0);

	goto CLEANUP;

FAILURE:
	status = M_FAILURE_GEN;
	if (searchDirs.itemSize == 0) searchDirs.itemSize = sizeof(String);

CLEANUP:
	DestroyString(&startPath);
	DestroyVector(&searchDirs);

	return status;
}

int FindAllFilesByQuery(const String* query)
{
	if (query == NULL) return M_FAILURE_GEN;

	struct timespec start, end;
	if (timespec_get(&start, TIME_UTC) == 0) return M_FAILURE_GEN;

	printf("Finding all files with names containing \"");
	PrintString(query);
	printf("\" in the current working directory...\n");

	Vector files = CreateVector(NULL, 0, sizeof(File), DestroyFile, NULL);
	if (files.itemSize == 0) goto FAILURE;

	if (FindAllFiles(&files, query) != M_SUCCESS) goto FAILURE;

	PrintFiles(&files);

	DestroyVector(&files);

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

FAILURE:
	if (files.itemSize == 0) files.itemSize = sizeof(File);
	DestroyVector(&files);

	return M_FAILURE_GEN;
}