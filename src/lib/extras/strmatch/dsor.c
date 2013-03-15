//Shift or algorithm
#include "util.h"
#include "match.h"
#include <string.h>
static int preSo(char *x, int m, unsigned int S[]) { 
	unsigned int j, lim; 
	int i; 
	memset(S,255,ASIZE*ASIZE*4);
	for (i = 0; i < ASIZE; ++i) 
		S[i] = ~0; 
	for (lim = i = 0, j = 1; i < m; ++i, j <<= 1) { 
		S[(unsigned char)x[i]] &= ~j; 
		lim |= j; 

		//                    printf("S[%c]:%u  lim:%d\n",x[i],S[x[i]],lim); 
	}
	//lim=255;
	//lim = ~(lim>>1); 

	return (lim); 
} 

char* SshiftorW2(char *text,char *pat,int n, int m) { 
	unsigned int lim, state; 
	short *ps=(short*)text;
	unsigned int S[ASIZE*ASIZE];
	int j; 
    (void)n;
	if (m > WORD_SIZE) 
		error("SO: Use pattern size <= word size"); 

	/* Preprocessing */ 
	lim = preSo(pat, m, S); 

	/* Searching */ 
	for (state = ~0, j = 0;  *((char*)ps+1)!=0;) { 
		state = (state<<2) | S[*ps]; 
		if (state < lim) 
			OUTPUT(j - m + 1); 
		ps++;
	} 
	SRET(j-m+1);
} 


char* SshiftorW(char *text,char *pat) { 
	int m,n;
	m=strlen(pat);
	n=strlen(text); 
	return SshiftorW2(text, pat, n, m);
}
