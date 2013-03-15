#ifndef UTILL_match
#define UTILL_match
#include "report.h"
typedef struct 
{
	union	
	{
		unsigned long long _u64;
		struct{
			unsigned int low;
			unsigned int high;
			unsigned int low2;
			unsigned int high2;
		};
	};
} _U64;
typedef unsigned char u8;
char * Getsubjectfromfile(char *pfname);
int  Getpatternfromfile(char *pfname,char **patts);
void Mtime(_U64 * readr);
double Mdifftime(_U64 start,_U64 end);
//int output(int);
int output(char* text, int i, char* pat);
int  outputs(int whichp,int addr);
#define STOP 1
#define CONTINUE 0
#define OUTPUT(x) if( report_function((const char*)text, x, (const char*)pat) == STOP ) return (char*)(text + (x));
#define OUTPUT3(text, x , pat) if( report_function((const char*)text, x, (const char*)pat) == STOP ) return (char*) (text +(x)) ;
//#define SRET(i) if( (i)< n) return (text + j) ; else return NULL;
#define SRET(i)  return NULL;
#define OUTPUTs(which, i) if( report_function(text, i, patts[which]) == STOP ) return ;

typedef struct {
	char str[3];
	char type;
}structHeader;

#define error(msg) perror(msg)
#include <stdio.h>
#include <stdlib.h>

#endif
