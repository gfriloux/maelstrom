#include "util.h"
#include <string.h>
#include <string.h>
#include "automaton.h"
#include <stdlib.h>
#include "match.h"

/* struct _cell{
    int element; 
    struct _cell *next;
  };
typedef struct _cell *List;*/
char* Sskip2(char *textt,char *patt, int n, int m)//ÌøÔ¾Ëã·¨ 
{
	int i, j;
	unsigned char * text,*pat;
	text = (unsigned char *) textt;
	pat = (unsigned char *)patt;
	List ptr, z[ASIZE];
	/* Preprocessing */
	memset(z, 0, ASIZE*sizeof(List));
	for (i = 0; i < m; ++i) {
		ptr = (List)malloc(sizeof(struct _cell));
		if (ptr == NULL)
			error("SKIP");
		ptr->element = i;
		ptr->next = z[pat[i]];
		z[pat[i]] = ptr;
	}
	/* Searching */
	for (j = m - 1; j < n; j += m)
	{
		for (ptr = z[text[j]]; ptr != NULL; ptr = ptr->next)
		{
			if (memcmp(pat, text + j - ptr->element, m) == 0)
			{
				if (j-ptr->element <= n - m)
					OUTPUT(j - ptr->element);
			}
			else
				continue;
		}
	}
	SRET(j - ptr->element);
}
char* Sskip(char *textt,char *patt)//ÌøÔ¾Ëã·¨ 
{
	int m, n;
	m=strlen(patt);
	n=strlen(textt);
	return Sskip2(textt, patt, n, m);
}
