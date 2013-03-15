#include "util.h"
#include <string.h>
#define ASIZE 256
#define MAXSTATE 100 
#define MAXLENGTH 10
#define MFail -1

int prestate(char **pat,int rows,int preG[][ASIZE],int preO[])
/* preprocessing of GOTO FUNCTION */
{
    char *y[MAXLENGTH];
    int i,j,m,p;
    int newstate,state;

    newstate=0;  state=0;
 
    for(i=0;i<MAXSTATE;i++)
      for(j=0;j<ASIZE;j++)
         preG[i][j]=MFail;

   for(i=0;i<MAXSTATE;i++)
      preO[MAXSTATE]=0;
    for(i=1;i<=rows;i++)
    {
       y[i]=pat[i];
       m=strlen(y[i]);
       j=0;
       state=0;
       while (preG[state][(unsigned char)y[i][j]]!=MFail) 
       {
          state=preG[state][(unsigned char)y[i][j]];
          j++;
       }

       for(p=j;p<m;p++)
       {
          newstate++;
          preG[state][(unsigned char)y[i][p]]=newstate;
          state=newstate;
       }
       preO[state]=i;
     }

     for(j=0;j<ASIZE;j++)
        if(preG[0][j]==MFail)
           preG[0][j]=0;

     return(state);
}


void preFail(int preG[][ASIZE],int allstate,int preF[])
/* preprocessing of FAILURE FUNCTION */
{
    int i,j;
    int s,remainS,r,preS;
    int preQue[MAXSTATE];
    int *p;

    remainS=allstate;

    j=0;
    for(i=0;i<ASIZE;i++)
    {
       s=preG[0][i];
       if(s!=0)
       {
           preQue[j]=s;
           remainS--;
           preF[s]=0;
           j++;
       }
    }

    p=preQue;

    while (remainS!=0)
    {
        r=*p;  p++;
        for(i=0;i<ASIZE;i++)
        {
           s=preG[r][i];
           if(s!=MFail)
           {
              preQue[j]=s;
              remainS--;
              j++;
           }
           preS=preF[r];
           
           while(preG[preS][i]==MFail)
              preS=preF[preS];
        
           preF[s]=preG[preS][i];
         }
     }
}
          

void Mac(char *text,char  **patts,int rows)
{
   int state,matchS,tempS,oS;
   int preG[MAXSTATE][ASIZE],preF[MAXSTATE],preO[MAXSTATE];
   int i,j;
   int n;
   char *temPat[MAXLENGTH];
   
   temPat[0]="";
   for(i=0;i<rows;i++)
	   temPat[i+1]=patts[i];
   for(i=0;i<MAXSTATE;i++)
      preO[i]=0;

   state=prestate(temPat,rows,preG,preO);
   preFail(preG,state,preF);
   
   n=strlen(text);
   matchS=0;
   j=0;
   while(j<n)
   {
      tempS=preG[matchS][(unsigned char)text[j]];
      if(tempS!=MFail)
      {
         matchS=tempS;
         j++;
         oS=preO[matchS];
         if(oS!=0)
           OUTPUTs(oS-1,(j-strlen(temPat[oS])));		 
      }
      else
      {
        matchS=preF[matchS];
        oS=preO[matchS];
        if(oS!=0)
          OUTPUTs(oS-1,(j-strlen(temPat[oS])));
      }
   }
}
