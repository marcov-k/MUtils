#ifndef M_FIND_DIR
#define M_FIND_DIR

#include "../include/m_string.h"
#include "../include/m_vector.h"

void FindAllDirectoriesByQuery(const String* query);
Vector* GetAllDirectoriesInDir(const String* path);

#endif