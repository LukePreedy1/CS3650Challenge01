// Much of this code is from Nat's lecture notes 07-io-and-shell/reverse/svec.c

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "svec.h"

/*
 * typedef struct svec {
 * 	int size;
 * 	int cap;
 * 	char** data;
 * } svec;
*/

void
print_svec(svec* sv) {
	printf("Size = %d\n", sv->size);
	for (int ii = 0; ii < sv->size; ++ii) {
		printf("%s\n", svec_get(sv, ii));
	}
}

// appends the second svec onto the end of the first svec
// then frees the second, since it should no longer be needed
svec*
append_svec(svec* first, svec* second) {
	for (int ii = 0; ii < second->size; ii++) {
		svec_push_back(first, svec_get(second, ii));
	}
	free_svec(second);
	return first;
}

// makes an svec with an initial size of 0, capacity of 2, and an empty array of strings for data
svec*
make_svec()
{
	svec* sv = malloc(sizeof(svec));
	sv->size = 0;
	sv->capacity = 2;
	sv->data = malloc(2 * sizeof(char*));
	memset(sv->data, 0, 2 * sizeof(char*));
	return sv;
}

// frees memory for the svec
void
free_svec(svec* sv)
{
	for (int ii = 0; ii < sv->size; ++ii) {
		if (sv->data[ii] != 0) free(sv->data[ii]);
	}
	free(sv->data);
	free(sv);
}

// removes the item at the given index from the given svec
svec*
remove_svec(svec* sv, int index) {
	svec* new_svec = make_svec();
	for (int ii = 0; ii < sv->size; ++ii) {
		if (ii == index) {
			continue;
		}
		else {
			svec_push_back(new_svec, svec_get(sv, ii));
		}
	}
	return new_svec;
}

// gets the piece of data in position ii from the svec, provided that ii is a valid position
char*
svec_get(svec* sv, int ii)
{
	//printf("in svec, the given sv->size = %d\nii = %d\n", sv->size, ii);
	assert(ii >= 0 && ii < sv->size);
	return sv->data[ii];
}

// puts a new string in the svec at position ii, provided that ii is a valid position
void
svec_put(svec* sv, int ii, char* str)
{
	assert(ii >= 0 && ii < sv->size);
	char* str_2 = strdup(str);
	sv->data[ii] = str_2;
}

// adds the given string to the end of the svec.  expands the capacity if necessary
void
svec_push_back(svec* sv, char* str)
{
	//print_svec(sv);
	//printf("pushing the string: %s\n", str);

	int ii = sv->size;

	if (ii >= sv->capacity) {
		sv->capacity *= 2;
		sv->data = (char**) realloc(sv->data, sv->capacity * sizeof(char*));
	}

	sv->size = ii + 1;
	svec_put(sv, ii, str);
}
