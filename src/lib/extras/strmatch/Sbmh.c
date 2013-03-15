#include "util.h"
#include <string.h>
#define ASIZE 256
void preBmBc(char *pat, int m, int bmBc[]); 

char*  Sbmh2(char * textt,char *patt,int n, int m) // HorspoolÀ„∑® 
{
	int j,bmBc[ASIZE];
	unsigned char c;
	unsigned char * text,*pat;
	text = (unsigned char*)textt;
	pat = (unsigned char*)patt;

	/* preprocessing */
	preBmBc((char*)pat,m,bmBc);
	/* searching */
	j=0;
	while (j<=n-m)
	{
		c=text[j+m-1];
		if (pat[m-1]==c && memcmp(pat,text+j,m-1)==0)
			OUTPUT(j);
		j+=bmBc[c];
	}
	SRET(j);
}
char*  Sbmh(char * textt,char *patt) // HorspoolÀ„∑® 
{
	int m,n;
	m=strlen(patt);
	n=strlen(textt);
	return	Sbmh2(textt, patt, n, m);
}
