#include "util.h"
#include <string.h>
#define ASIZE 256
#define MAX(a,b) (a>b)?a:b
void preBmBc(char *pat, int m, int bmBc[]); 
void preQsBc(char *pat,int m,int qsBc[]);

char*  Ssmith2(char * textt,char *patt,int n, int m)//smith Ëã·¨ 
{
	int j,bmBc[ASIZE],qsBc[ASIZE];
	unsigned char * text,*pat;
	text = (unsigned char*)textt;
	pat = (unsigned char*)patt;


	/* preprocessing */
	preBmBc((char*)pat,m,bmBc);
	preQsBc((char*)pat,m,qsBc);

	/* searching */
	j=0;
	while (j<=n-m)
	{
		if (memcmp(pat,text+j,m)==0)
			OUTPUT(j);
		j+=MAX(bmBc[text[j+m-1]],qsBc[text[j+m]]);
	}
	SRET(j);
}
char*  Ssmith(char * textt,char *patt)//smith Ëã·¨ 
{
	int m,n;
	m=strlen(patt);
	n=strlen(textt);
	return 	Ssmith2(textt, patt, n ,m);
}
