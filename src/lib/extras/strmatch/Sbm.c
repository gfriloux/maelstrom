#include "util.h"
#include <string.h>
#include <stdio.h>
#include <stdio.h>
#define XSIZE 100
#define ASIZE 256
#define MAX(a,b) (a>b)?a:b

void preBmBc(char *pat, int m, int bmBc[]) {
   int i;
 
   for (i = 0; i < ASIZE; ++i)
      bmBc[i] = m;
   for (i = 0; i < m - 1; ++i)
      bmBc[(unsigned char)pat[i]] = m - i - 1;
}
 
 
void suffipates(char *pat, int m, int *suff) {
   int f = 0, g, i;
 
   suff[m - 1] = m;
   g = m - 1;
   for (i = m - 2; i >= 0; --i) {
      if (i > g && suff[i + m - 1 - f] < i - g)
         suff[i] = suff[i + m - 1 - f];
      else {
         if (i < g)
            g = i;
         f = i;
         while (g >= 0 && pat[g] == pat[g + m - 1 - f])
            --g;
         suff[i] = f - g;
      }
   }
}
 
void preBmGs(char *pat, int m, int bmGs[]) {
   int i, j, suff[XSIZE];
 
   suffipates(pat, m, suff);
 
   for (i = 0; i < m; ++i)
      bmGs[i] = m;
   j = 0;
   for (i = m - 1; i >= -1; --i)
      if (i == -1 || suff[i] == i + 1)
         for (; j < m - 1 - i; ++j)
            if (bmGs[j] == m)
               bmGs[j] = m - 1 - i;
   for (i = 0; i <= m - 2; ++i)
      bmGs[m - 1 - suff[i]] = m - 1 - i;
}
 
 
char* Sbm2(char *texts, char *pat,int n, int m) {
	int i, j, bmGs[XSIZE], bmBc[ASIZE];
	//   printf("n lenth of bm:%d,m:%d,",n,m);
	unsigned char* text= (unsigned char*) texts;

	/* Preprocessing */
	preBmGs(pat, m, bmGs);
	preBmBc(pat, m, bmBc);
	/* Searching */
	j = 0;
	while (j <= n - m) {
		for (i = m - 1; (i >= 0) && (pat[i] == text[i + j]); --i);
		if (i < 0) {
			OUTPUT(j);
			j += bmGs[0];
		}
		else{
			int skipn = MAX(bmGs[i], bmBc[text[i + j]] - m + 1 + i);
			//if(skipn> m) 
				//printf("err\n");
			j += skipn; 
		}
	}
	SRET(j);
}

char* Sbm(char *text, char *pat) {
	int m,n;
	m=strlen(pat);
	n=strlen(text);
	return Sbm2(text,pat,n,m);
}

typedef struct{
	structHeader header;
	int limit;
	int bmGs[XSIZE];
	int bmBc[ASIZE];
}structSbm;

void* preSbm(char* pat, int m)
{
	if(m == 0) m = strlen(pat);
	structSbm* s= (structSbm*)malloc(sizeof(structSbm));;
	preBmGs(pat, m, s->bmGs);
	preBmBc(pat, m, s->bmBc);
	return (void*) s;
}
