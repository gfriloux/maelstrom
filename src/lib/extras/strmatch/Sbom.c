#include "util.h"
#include <string.h>
#include "util.h"
#include "automaton.h"

#define FALSE      0
#define TRUE       1
#define UNDEFINED 123

int getTransition(char *x, int p, List L[], char c) {
   List cell;

   if (p > 0 && x[p - 1] == c)
      return(p - 1);
   else {
      cell = L[p];
      while (cell != NULL)
         if (x[cell->element] == c)
            return(cell->element);
         else
            cell = cell->next;
      return(UNDEFINED);
   }
}


void setTransition(int p, int q, List L[]) {
   List cell;

   cell = (List)malloc(sizeof(struct _cell));
   if (cell == NULL){
      //error("BOM/setTransition");
      return; 
   }
   cell->element = q;
   cell->next = L[p];
   L[p] = cell;
}


void oracle(char *x, int m, char T[], List L[]) {
   int i, p, q = 0;
   int S[m + 1];
   char c;

   S[m] = m + 1;
   for (i = m; i > 0; --i) {
      c = x[i - 1];
      p = S[i];
      while (p <= m &&
             (q = getTransition(x, p, L, c)) ==
             UNDEFINED) {
         setTransition(p, i - 1, L);
         p = S[p];
      }
      S[i - 1] = (p == m + 1 ? m : q);
   }
   p = 0;
   while (p <= m) {
      T[p] = TRUE;
      p = S[p];
   }
}


char* Sbom2(char *text,char *pat,int n, int m) 
{
	char T[strlen(pat) + 1];
	List L[strlen(pat) + 1];
	int i, j, p, period = 0, q, shift;
	/* Preprocessing */
	memset(L, 0, (m + 1)*sizeof(List));
	memset(T, FALSE, (m + 1)*sizeof(char));
	oracle(pat, m, T, L);

	/* Searching */
	j = 0;
	while (j <= n - m) {
		i = m - 1;
		p = m;
		shift = m;
		while (i + j >= 0 &&
				(q = getTransition(pat, p, L, text[i + j])) !=
				UNDEFINED) {
			p = q;
			if (T[p] == TRUE) {
				period = shift;
				shift = i;
			}
			--i;
		}
		if (i < 0) {
			OUTPUT(j);
			shift = period;
		}
		j += shift;
	}
	SRET(j);
}

char* Sbom(char *text,char *pat) 
{
	int m,n;
	m=strlen(pat);
	n=strlen(text);
	return Sbom2(text, pat, n, m);
}
