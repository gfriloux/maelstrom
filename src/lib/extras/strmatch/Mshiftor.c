#include "util.h"
#include <string.h>
/*多模式串shiftor算法*/
#include"util.h" 
#include <string.h>
#define WORD_SIZE 32
#define ASIZE 256
int preMo(char *Ps[],int ss,int length, unsigned int S[],int maxsingle)
{
int i,k;
unsigned int j,lim; 	
char *temp;
char x[length];
temp=x;
for(k=0;k<maxsingle;k++)
for(i=0;i<ss;i++) {
	if((int)strlen(Ps[i])>k){
		 *temp=*(Ps[i]+k);
		 temp++;
		 }
	else{
	   *temp=0;
	   temp++;
	 }	 
	}
for (i = 0; i < ASIZE; ++i) 
     S[i] = ~0; 
for (lim = i = 0, j = 1; i < length; ++i, j <<= 1) { 
    S[(unsigned char)x[i]] &= ~j; 
    lim |= j; 

   } 
lim = ~(lim>>1); 
return(lim);  	
	
}
void Mshiftor(char *T,char *Ps[],int ss)
{
	unsigned int lim, state,limmax=0,limmin=0,t=1; 
	unsigned int S[ASIZE];
	int i,j,n,length=0,temp,maxsingle=0;
    char* text = T;
    char** patts = Ps;
	n=strlen(T);
	for(i=0;i<ss;i++){
		temp=strlen(Ps[i]);
		if(temp>maxsingle) maxsingle=temp;
		length +=temp;
	}
	if (length > WORD_SIZE) 
		error("MO: Use pattern size <= word size"); 
	lim=preMo(Ps,ss,length,S,maxsingle);


	for(i=(maxsingle-1)*ss;i<maxsingle*ss;i++)
	{
		for(j=i;j>0;j--)
			t=t*2;
		if(i==(maxsingle-1)*ss)  limmin=t;
		limmax+=t;
	}          
	/* Searching */ 
	for (state = ~0, j = 0; j < n; ++j) {
		state = (state<<maxsingle) | S[(unsigned char)T[j]];
		t=~state;
		if(t>=limmin && t<=limmax)    	   
			OUTPUTs(0,j -maxsingle + 1);
	} 

}
