#include "util.h"
#include <string.h>
#include"match.h"
/* //改函数为前面MP算法中函数，为调试复制到次
void preMp(char *x, int m, int mpNext[]) {
   int i, j;

   i = 0;
   j = mpNext[0] = -1;
   while (i < m) {
      while (j > -1 && x[i] != x[j])
         j = mpNext[j];
      mpNext[++i] = ++j;
   }
}
//该函数为前面KMP算法中子函数，为调试复制到次
void preKmp(char *x, int m, int kmpNext[]) {
   int i, j;

   i = 0;
   j = kmpNext[0] = -1;
   while (i < m) {
      while (j > -1 && x[i] != x[j])
         j = kmpNext[j];
      i++;
      j++;
      if (x[i] == x[j])
         kmpNext[i] = kmpNext[j];
      else
         kmpNext[i] = j;
   }
}
*/
void preKmp(char *pat,int m,int kmpNext[]);
void  preMp(char *pat,int m,int mpNext[]);
//KMP Skip Search algorithm
int attempt(char *y, char *x, int m, int start, int wall) {
   int k;

   k = wall - start;
   while (k < m && x[k] == y[k + start])
      ++k;
   return(k);
}


char* Skmpskip2(char *textt,char *patt,int n, int m) {
	int i, j, k, kmpStart, per, start, wall;
	unsigned char * text,*pat;
	text = (unsigned char *) textt;
	pat = (unsigned char *)patt;
	int kmpNext[m], list[m], mpNext[m],
		z[ASIZE];

	/* Preprocessing */
	preMp((char*)pat, m, mpNext);
	preKmp((char*)pat, m, kmpNext);
	memset(z, -1, ASIZE*sizeof(int));
	memset(list, -1, m*sizeof(int));
	z[pat[0]] = 0;
	for (i = 1; i < m; ++i) {
		list[i] = z[pat[i]];
		z[pat[i]] = i;
	}

	/* Searching */
	wall = 0;
	per = m - kmpNext[m];
	i = j = -1;
	do {
		j += m;
	} while (j < n && z[text[j]] < 0);
	if (j >= n)
		return NULL;
	i = z[text[j]];
	start = j - i;
	while (start <= n - m) {
		if (start > wall)
			wall = start;
		k = attempt((char*)text, (char*)pat, m, start, wall);
		wall = start + k;
		if (k == m) {
			OUTPUT(start);
			i -= per;
		}
		else
			i = list[i];
		if (i < 0) {
			do {
				j += m;
			} while (j < n && z[text[j]] < 0);
			if (j >= n)
				return NULL;
			i = z[text[j]];
		}
		kmpStart = start + k - kmpNext[k];
		k = kmpNext[k];
		start = j - i;
		while (start < kmpStart ||
				(kmpStart < start && start < wall)) {
			if (start < kmpStart) {
				i = list[i];
				if (i < 0) {
					do {
						j += m;
					} while (j < n && z[text[j]] < 0);
					if (j >= n)
						return NULL;
					i = z[text[j]];
				}
				start = j - i;
			}
			else {
				kmpStart += (k - mpNext[k]);
				k = mpNext[k];
			}
		}
	}
	SRET(start);
}
char* Skmpskip(char *textt,char *patt) {
	int m, n;
	m=strlen(patt);
	n=strlen(textt);
	return Skmpskip2(textt, patt, n, m);
}
