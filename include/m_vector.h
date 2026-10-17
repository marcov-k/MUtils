#ifndef M_VECTOR_H
#define M_VECTOR_H

#include "m_deref.h"

#include <stdlib.h>
#include <stdbool.h>

typedef int (*Destructor)(void*);
typedef int (*Copier)(void*, const void*);
typedef bool (*Comparer)(const void*, const void*);

typedef struct [[nodiscard("Vector owns heap memory; the result must be stored and destroyed")]]
{
	void* data;
	Destructor destructor;
	size_t itemSize;
	size_t length;
	size_t capacity;
} Vector;

/*
	Empty vs. NULL:
	An empty vector is defined as having a length of 0, regardless of data buffer allocation state.
	Functions will treat empty Vectors with NULL and non-NULL data buffers as equivalent.

	
	Length:
	All length parameters and fields represent the number of items in a Vector or void* pointer.


	Valid Vectors:
	Every vector must be initialized (zeroed or created) before use.
	Valid Vectors obey the following invariants:
		- Item size is always > 0
		- NULL data implies 0 length and 0 capacity
		- Length never exceeds capacity
		- Data buffer does not overlap with any other Vector's data buffer


	Return Values and Failure Guarantees:
	A return value of M_SUCCESS (0) indicates success; M_FAILURE_GEN (1) indicates failure.
	For functions returning an index, the index 1 past the final element (length + 1) will be returned
	in case of a failure of any kind.

	A failure is defined as any function and/or operation not succeeding completely.
	Any function receiving a Vector with an item size of 0 will fail.

	All functions returning a new struct, eg. a new Vector, will return an empty struct with an item size of 0 in case of failure;
	if the function succeeds but results in an empty struct, the struct will have a non-zero item size.
	Functions mutating parameters will leave all output parameters unmodified in the case of any argument being a NULL Vector pointer,
	otherwise the output parameter will be invalidated (item size = 0) in case of failure.

	All functions returning a void* pointer will return a NULL pointer in case of failure.
	Data writing functions will return M_FAILURE_GEN (1) in case of failure and may leave the Vector in a partially modified state.


	NULL and Empty Arguments:
	NULL void* pointer arguments to creation and copy functions will return a valid empty Vector.
	NULL void* pointer arguments to data writing functions will result in a failure.
	Any NULL Vector argument to a function mutating output parameters will result in a failure.
	NULL pointer arguments to comparison functions will always cause the function to return false.


	Aliasing:
	The data buffers of Vectors can never overlap.
	A Vector cannot contain multiple instances of the same element or struct.


	Destructors:
	Functions removing elements from Vectors, eg. DestroyVector() and ClearVector(), will use the Vector's destructor pointer
	if it is non-NULL.
	Heap memory used by elements of Vectors with NULL destructor pointers will not be freed.


	Copiers:
	Functions receiving Copier function pointers, eg. CopyVector(), will perform a deep copy if receiving a copier pointer
	or a shallow copy if the copier pointer is NULL.


	Comparers:
	Functions receiving Comparer function pointers, eg. VectorsEqual(), will use the given comparer to determine whether their elements are
	equal if receiving a comparer pointer or compare the raw bytes of their elements without accounting for any nested heap memory if the
	comparer pointer is NULL.


	Bounds Checking:
	Data access functions will return a NULL pointer in case of an out-of-range index.
	Data writing functions will return M_FAILURE_GEN (1) in case of an out-of-range index.
*/

// Create a new Vector struct
// Null pointer argument will return an empty Vector with capacity = length
// Returned Vector is owned by and must be destroyed by caller
Vector CreateVector(const void* data, size_t length, size_t itemSize, Destructor destructor, Copier copier);
// Free all heap memory used by a Vector and its elements
int DestroyVector(void* v);
// Clear the data of a Vector
int ClearVector(Vector* v);
// Move data from one Vector to another
// Always performs a shallow copy of the data
// Destroys the source Vector when successful
// No-op if destination and source are the same Vector
int MoveVector(Vector* dest, Vector* source);
// Create a new Vector struct from an existing Vector
// Return Vector is owned by and must be destroyed by caller
Vector CopyVector(const Vector* v, Copier copier);
// Copy data from one Vector to another
// Will mutate the dest Vector to accept the type of data stored in the source Vector
int CopyVectorTo(void* dest, const void* source, Copier copier);
// Copy data into a Vector
// Source data pointers pointing into the dest Vector are unsupported
// Dest Vector must be configured for the type of data being copied.
int CopyDataToVector(Vector* dest, const void* source, size_t sourceLen, Copier copier);

// Get a pointer to the element at a given index in a Vector
void* VectorGetElementAt(const Vector* v, size_t index);
// Get a pointer to the first element in a Vector
void* VectorGetElementFirst(const Vector* v);
// Get a pointer to the last element in a Vector
void* VectorGetElementLast(const Vector* v);
// Set an element at a given index in a Vector
int VectorSetElementAt(Vector* v, size_t index, const void* item, Copier copier);
// Set the first element in a Vector
int VectorSetElementFirst(Vector* v, const void* item, Copier copier);
// Set the last element in a Vector
int VectorSetElementLast(Vector* v, const void* item, Copier copier);

// Prepend an element to the start of a Vector
int VectorPushFront(Vector* v, const void* item, Copier copier);
// Append an element to the end of a Vector
int VectorPushBack(Vector* v, const void* item, Copier copier);
// Insert an element at a given index in a Vector
int VectorInsertAt(Vector* v, size_t index, const void* item, Copier copier);
// Insert a Vector at a given index in another Vector
int VectorInsertRangeAt(Vector* v, size_t index, const Vector* insert, Copier copier);
// Insert a C-array at a given index in a Vector
int VectorInsertArrayAt(Vector* v, size_t index, const void* insert, size_t insertLen, Copier copier);

// Check whether two Vectors are equal
bool VectorsEqual(const void* v1, const void* v2, Comparer comparer);

// Remove the first element in a Vector
int VectorPopFront(Vector* v);
// Remove the last element in a Vector
int VectorPopBack(Vector* v);
// Remove an element at a given index from a Vector
int VectorRemoveAt(Vector* v, size_t index);
// Remove a range from a Vector
int VectorRemoveRange(Vector* v, size_t start, size_t length);

#endif