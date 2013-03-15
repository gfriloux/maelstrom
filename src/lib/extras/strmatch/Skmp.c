#include "util.h"
#include <string.h>
#define XSIZE 100


void preKmp(char *pat,int m,int kmpNext[])
{
   int i,j;
   i=0;
   j=kmpNext[0]=-1;
   while(i<m)
   {
      while (j>-1 && pat[i]!=pat[j])
        j=kmpNext[j];
      i++;  j++;
      if (pat[i]==pat[j])
        kmpNext[i]=kmpNext[j];
      else
        kmpNext[i]=j;
   }
}

char* Skmp2(char *text,char *pat, int n, int m) //KMPÀ„∑®
{
   int i,j,kmpNext[XSIZE];
   
   
   /* preprocessing */
   preKmp(pat,m,kmpNext);

   /* searching */
   i=j=0;
   while (j<n)
   {
     while (i>-1 && pat[i]!=text[j])
        i=kmpNext[i];
     i++; j++;
     if (i>=m)
     {
        OUTPUT(j-i);
        i=kmpNext[i];
     }
   }
   return NULL;
}

char* Skmp(char *text,char *pat) //KMPÀ„∑®
{

   int m,n;
   m=strlen(pat);
   n=strlen(text);
   return Skmp2(text, pat, n, m);
}

typedef struct{
    structHeader header;
    int limit;
    int kmpNext[XSIZE];
}structSkmp;

void* preSkmp(char* pat, int m)
{
    if(m == 0) m = strlen(pat);
    structSkmp* s= (structSkmp*)malloc(sizeof(structSkmp));;
    preKmp(pat,m,s->kmpNext);
    return (void*) s;
}
