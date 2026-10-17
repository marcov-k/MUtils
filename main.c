#include "include/m_string.h"
#include "include/m_vector.h"

#include "fd/find_dir.h"
#include "ff/find_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define FIND_DIR "fd"
#define FIND_FILE "ff"

int main(int argc, char* argv[])
{
	SetConsoleOutputCP(CP_UTF8);

	if (argc < 2)
	{
		printf("No command provided\nUsage: %s <command>\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], FIND_DIR) == 0) // 'find directory' command
	{
		String query;
		if (argc < 3)
		{
			query = CreateString("");
		}
		else
		{
			query = CreateString(argv[2]);
		}

		if (FindAllDirectoriesByQuery(&query) != M_SUCCESS)
		{
			printf("An error occured while searching directories\n");
		}
		DestroyString(&query);
	}
	else
	{
		printf("Unknown command\n");
		return 1;
	}

	return 0;
}