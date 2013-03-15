#include "util.h"
#include <string.h>
//¼ÆÊıËã·¨
#include"match.h"
char* SbyhSearch(unsigned char *text,int m,int n,int  ch[]); 
char* Sbyh2(char *textt,char *patt,int n ,int m)
{
	int i;
	unsigned char *text,*pat;
	text = (unsigned char*) textt;
	pat = (unsigned char*)patt;
	int ch[ASIZE];
	for(i=0;i<ASIZE;i++)
		ch[i]=-1;
	for(i=0;i<m;i++)
	{
		ch[pat[i]]=i;
	}
	/*search*/
	return SbyhSearch(text,m,n,ch);
}

char* Sbyh(char *textt,char *patt)
{
	int m, n;
	m=strlen(patt);
	n=strlen(textt);
	return Sbyh2(textt, patt, n, m);
}

char* SbyhSearch(unsigned char *text,int m,int n,int ch[]){
	int j;
    const int yu = 64;//(2*m>32?(2*m>64?128:64):32);
	int  count[yu];
	//int yu=2*m; 
	//int  count[2*m],yu;

	for(j=0;j<2*m;j++) {
		count[j]=0;
	}
	for(j=0;j<m;j++)
		if((ch[text[j]]!=-1)&&(j-ch[text[j]]>=0) )
			count[j-ch[text[j]]]++;
	for(j=m;j<n;j++){
		if(ch[text[j]]!=-1)	
			count[(j-ch[text[j]])%yu]++;

		if(count[(j-m)%yu]==(m))
		{
			//OUTPUT(j-m);
			OUTPUT3(text, j-m, ch);
		}
		count[(j-m)%yu]=0;
	}	
	SRET(j-m);
}




