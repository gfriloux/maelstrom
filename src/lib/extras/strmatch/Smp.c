#include "util.h"
#include <string.h>
#define XSIZE 100


void  preMp(char *pat,int m,int mpNext[])
{
  int i,j;
  i=0;
  j=mpNext[0]=-1;
  while (i<m)
  {
    while (j>-1 && pat[i]!=pat[j])
      j=mpNext[j];
    mpNext[++i]=++j;
  }
}

char* Smp2(char * text,char *pat, int n, int m)//MPÀ„∑®
{
   int i,j,mpNext[XSIZE];

  
   /* preprocessing */
   preMp(pat,m,mpNext);

   /* searching */
   i=j=0;
   while(j<n)
   {
       while (i>-1 && pat[i]!=text[j])
          i=mpNext[i];
       i++;  j++;
       if (i>=m)
       {
          OUTPUT(j-i);
          i=mpNext[i];
       }
   }
   SRET(j-i);
} 
char* Smp(char * text,char *pat)//MPÀ„∑®
{
   int m,n;
   m=strlen(pat);
   n=strlen(text);
   return Smp2(text, pat, n, m);
}
