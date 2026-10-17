#include "find_dir.h"

#include "../include/m_string.h"
#include "../include/m_vector.h"

#include <direct.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

typedef struct
{
	String name;
	String path;
} Directory;

static Directory CreateDirectoryWrapper(String* name, String* path)
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
	Directory* sourcePtr = (Directory*)source;

	if (CopyStringTo(&destPtr->name, &sourcePtr->name) != M_SUCCESS) return M_FAILURE_GEN;
	if (CopyStringTo(&destPtr->path, &sourcePtr->path) != M_SUCCESS) return M_FAILURE_GEN;

	return M_SUCCESS;
}

static void FilterDirectories(Vector* directories, const String* query)
{
	for (size_t i = 0; i < directories->length; ++i)
	{
		const Directory* dir = VectorGetElementAt(directories, i);

		if (!StringContainsSubstring(&dir->name, query))
		{
			VectorRemoveAt(directories, i);
			i--;
		}
	}
}

static void PrintDirectories(const Vector* directories)
{
	if (directories->length == 0)
	{
		printf("No directories found.\n");
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
		PrintStringLn(&dir->path);
	}
}

static Vector* FindAllDirectories(void)
{
	char* workingDir = _getcwd(NULL, 0);
	String startPath = CreateString(workingDir);
	free(workingDir);

	Directory startDirectory = CreateDirectoryWrapper(&startPath, &startPath);

	Vector* directories = malloc(sizeof(Vector));
	*directories = CreateVector(NULL, 0, sizeof(Directory), DestroyDirectory, NULL);

	Vector searchDirs = CreateVector(NULL, 0, sizeof(Directory), DestroyDirectory, NULL);
	VectorPushBack(&searchDirs, &startDirectory, ZeroDirectory, CopyDirectoryTo);

	do
	{
		Directory* search = VectorGetElementLast(&searchDirs);
		const String* searchPath = &search->path;

		Vector* foundDirs = GetAllDirectoriesInDir(searchPath);
		VectorPopBack(&searchDirs);

		if (foundDirs->length > 0)
		{
			VectorInsertRangeAt(&searchDirs, searchDirs.length, foundDirs, ZeroDirectory, CopyDirectoryTo);
			VectorInsertRangeAt(directories, directories->length, foundDirs, ZeroDirectory, CopyDirectoryTo);
		}

		DestroyVector(foundDirs);
		free(foundDirs);
	} while (searchDirs.length > 0);

	DestroyString(&startPath);
	DestroyDirectory(&startDirectory);
	DestroyVector(&searchDirs);
	return directories;
}

void FindAllDirectoriesByQuery(const String* query)
{
	struct timespec start, end;
	timespec_get(&start, TIME_UTC);

	printf("Finding all directories with names containing \"");
	PrintString(query);
	printf("\" in the current working directory...\n\n");

	Vector* directories = FindAllDirectories();
	FilterDirectories(directories, query);

	PrintDirectories(directories);

	DestroyVector(directories);
	free(directories);

	timespec_get(&end, TIME_UTC);

	double timeSpent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	printf("\nSearch took %f seconds\n", timeSpent);
}

Vector* GetAllDirectoriesInDir(const String* path)
{
	// Prepare search path to match all contents
	String searchPath = CopyString(path);
	StringAppendCStr(&searchPath, "\\*");

	// Prepare parent directory path to prepend to every found directory name
	String parentPath = CopyString(path);
	StringAppendCStr(&parentPath, "\\");

	// Initialize vector to store paths of all directories found
	Vector* directories = malloc(sizeof(Vector));
	if (directories == NULL) goto CLEANUP;
	*directories = CreateVector(NULL, 0, sizeof(Directory), DestroyDirectory, NULL);

	// Find first item in the directory
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(searchPath.data, &findData);
	if (hFind == INVALID_HANDLE_VALUE) goto CLEANUP;

	// Iterate through all items in the directory and store all found directories
	do
	{
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) continue;

			// Create strings storing directory name and path
			String directoryName = CreateString(findData.cFileName);
			String directoryPath = CopyString(&parentPath);
			StringAppendCStr(&directoryPath, findData.cFileName);

			// Add directory to found directories
			Directory* directory = malloc(sizeof(Directory));
			*directory = CreateDirectoryWrapper(&directoryName, &directoryPath);
			VectorPushBack(directories, directory, NULL, NULL);

			DestroyString(&directoryName);
			DestroyString(&directoryPath);
		}
	} while (FindNextFile(hFind, &findData));

	// Free all temporary allocated memory and return found directories
CLEANUP:
	DestroyString(&searchPath);
	DestroyString(&parentPath);
	return directories;
}