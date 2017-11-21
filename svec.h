// Much of this code is from Nat's lecture notes 07-io-and-shell/reverse/svec.h

#ifndef SVEC_H
#define SVEC_H

typedef struct svec {
	int size;
	int capacity;
	char** data;
} svec;

void print_svec(svec* sv);

svec* make_svec();

void free_svec(svec* sv);

char* svec_get(svec* sv, int ii);

void svec_push(svec* sv, int ii, char* str);

void svec_push_back(svec* sv, char* str);

svec* remove_svec(svec* sv, int index);

svec* append_svec(svec* first, svec* second);

#endif
