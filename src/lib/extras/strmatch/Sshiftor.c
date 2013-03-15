#include "util.h"
#include <string.h>
#include "match.h"
Uword preSo(char *x, int m, Uword S[]) { 
    Uword j, lim; 
    int i; 
	for (i = 0; i < ASIZE; ++i) 
		S[i] = ~0; 
	for (lim = i = 0, j = 1; i < m; ++i, j <<= 1) { 
		S[(unsigned char)x[i]] &= ~j; 
	} 
    lim = (((Uword)1)<<(m-1)) -1;
	lim = ~(lim); 
	return(lim); 
} 

char* Sshiftor2(char *text0, char *pat, int n, int m) { 
	Uword lim, state; 
	Uword S[ASIZE];
	int j;
    unsigned char* text = (unsigned char*)text0;
	if (m > WORD_SIZE) 
		error("SO: Use pattern size <= word size"); 

	/* Preprocessing */ 
	lim = preSo(pat, m, S); 

	/* Searching */ 
	for (state = ~(Uword)0, j = 0; j < n; ++j) { 
		state = (state<<1) | S[text[j]]; 
		if (state < lim) 
			OUTPUT(j - m + 1); 
	} 
	SRET(j-m+1);
} 

char* Sshiftor(char *text,char *pat) { 
	int m,n;
	m=strlen(pat);
	n=strlen(text); 
	return Sshiftor2(text , pat ,n ,m);
}

typedef struct{
    structHeader header;
    Uword limit;
    Uword S[256];
}structSshiftor;

void* preSshiftor(char* pat, int m)
{
    if(m == 0) m = strlen(pat);
    structSshiftor* s= (structSshiftor*)malloc(sizeof(structSshiftor));;
    s->limit=preSo(pat, m, s->S);
    return (void*) s;
}
