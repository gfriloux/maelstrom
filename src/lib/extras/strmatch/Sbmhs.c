#include "util.h"
#include <string.h>
#define ASIZE 256


void preQsBc(char *pat,int m,int qsBc[])
{
   int i;
   for (i=0;i<ASIZE;++i)
      qsBc[i]=m+1;
   for (i=0;i<m;++i)
      qsBc[(unsigned char)pat[i]]=m-i;
}

char* Sbmhs2(char * textt,char *patt,int n, int m)//SUNDAY算法，又称 quich search 
{
	int j,qsBc[ASIZE];
	unsigned char *text,*pat;
	text = (unsigned char*)textt;
	pat = (unsigned char*)patt;

	/* preprocessing */
	preQsBc((char*)pat,m,qsBc);

	/* Searching */
	j=0;
	while (j<=n-m)
	{
		if (memcmp(pat,text+j,m)==0)
			OUTPUT(j);
		j+=qsBc[text[j+m]]; //shift
	}
	SRET(j);
}
char* Sbmhs(char * textt,char *patt)//SUNDAY算法，又称 quich search 
{
	int m,n;
	m=strlen(patt);
	n=strlen(textt);
	return Sbmhs2(textt,patt,n,m);
}
