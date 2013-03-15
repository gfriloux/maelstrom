#include "util.h"
#include <string.h>
#include"match.h"
Uword preSa(char *x, int m, Uword S[]) { 
	Uword j, lim; 
	int i; 
	for (i = 0; i < ASIZE; ++i) 
		S[i] = 0; 
	for (lim = i = 0, j = 1; i < m; ++i, j <<= 1) { 
		S[(unsigned char)x[i]] |= j; 
	} 
    lim = (((Uword)1)<<(m-1)) -1; 
	return(lim); 
} 

char* Sshiftand2(char *text0,char *pat,int n, int m) { 
	Uword lim, state; 
	Uword S[ASIZE];
	int j; 
    unsigned char* text =(unsigned char*) text0;
	if (m > WORD_SIZE) 
		error("SO: Use pattern size <= word size"); 

	/* Preprocessing */ 
	lim = preSa(pat, m, S); 

	/* Searching */ 
	for (state =0, j = 0; j < n; ++j) { 
		state = ((state<<1) |1) & (S[text[j]]); 
		if (state > lim) 
			OUTPUT(j - m + 1); 
	} 
	SRET(j-m+1);
} 

char* Sshiftand(char *text,char *pat) 
{
	int m,n;
	m=strlen(pat);
	n=strlen(text); 
	return Sshiftand2(text, pat, n, m);
}

typedef struct{
    structHeader header;
    Uword limit;
    Uword S[256];
}structSshiftand;

void* preSshiftand(char* pat, int m)
{
    if(m == 0) m = strlen(pat);
    structSshiftand* s= (structSshiftand*)malloc(sizeof(structSshiftand));;
    s->limit=preSa(pat, m, s->S);
    return (void*) s;
}
