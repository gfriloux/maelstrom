#include "util.h"
#include <string.h>
#define SHIFTSIZE 32768
#define MAXLENGTH 100 
#define Bchar 2
#include "util.h"

long calHash(unsigned char p,unsigned char q)
/* calculate the HASH VALUE of two chars */
{
   long h;
   h=(p<<5)+q;
   return(h);
}

long preShift(char *p[], long rows,long preSHIFT[])
/* calculate the SHIFT TABLE and return the shortest length of the patterns*/
{
   long i,j,h;
   char *y[MAXLENGTH];
   long m,tempm;

   y[0]=p[0];
   m=strlen(y[0]);

   for(i=1;i<rows;i++)
   {
      y[i]=p[i];
      tempm=strlen(y[i]);
      if(tempm<m)
        m=tempm;
   }

   for(i=0;i<SHIFTSIZE;i++)
      preSHIFT[i]=m-Bchar+1;

  for(i=0;i<rows;i++)
  {
     tempm=strlen(y[i]);
     for(j=tempm-m;j<tempm-Bchar+1;j++)
     {
        h=calHash(y[i][j],y[i][j+1]);
        //if(preSHIFT[h]!=0)
        if(preSHIFT[h]>tempm-j-Bchar)//dzh emend at 2004.10.23 
          preSHIFT[h]=tempm-j-Bchar;
     }
  }
  return(m);
}     

void prePoint(char *p[],long rows,char prePOINTER[][MAXLENGTH])
/* calculate the POINTER TABLE,arraged by the hash value of last two chars */
{
    long i,j,k;
    long hpat,hPOINT;
    long LPOINT;
    long m,mP;

    strcpy(prePOINTER[0],p[0]);
  
    LPOINT=1;
 
    for(i=1;i<rows;i++)
    {
       m=strlen(p[i]);
       hpat=calHash(p[i][m-1],p[i][m-2]);

       j=0;
       while(j<LPOINT)
       {
          mP=strlen(prePOINTER[j]);
          hPOINT=calHash(prePOINTER[j][mP-1],prePOINTER[j][mP-2]);

          if(hpat<hPOINT)
          {
             LPOINT++;
             
             for(k=LPOINT;k>j;k--)
                strcpy(prePOINTER[k],prePOINTER[k-1]);
             strcpy(prePOINTER[j],p[i]);
 
             break;
          }
          else
             if(j==LPOINT-1)
             {
                LPOINT++;
                  
                strcpy(prePOINTER[j+1],p[i]);
 
                break;
             }
          j++;
       }
    }
}

void preFind(char prePOINTER[][MAXLENGTH],char *p[],long rows,long  preFIND[])
/* to find the former place of POINTER TABLE  in the PATTERNS TABLE */
{
	long i,j;
	int check;
	
	for (i=0;i<rows;i++)
	{
		j=0;
		check=strcmp(prePOINTER[i],p[j]);
		while(check!=0)
		{
			j++;
			check=strcmp(prePOINTER[i],p[j]);
		}
		preFIND[i]=j;
	}
}

void preHash(char prePOINTER[][MAXLENGTH],long rows,long preHASH[])
/* calculate the HASH TABLE,telling the starting and ending address of a hash value */
{
    long i;
    long m;
    long hPOINT,tempPOINT; 

    for(i=0;i<SHIFTSIZE;i++)
      preHASH[i]=-1;

    m=strlen(prePOINTER[0]);
    hPOINT=calHash(prePOINTER[0][m-2],prePOINTER[0][m-1]);
    preHASH[hPOINT]=0;

    for(i=1;i<rows;i++)
    {
      m=strlen(prePOINTER[i]);
      tempPOINT=calHash(prePOINTER[i][m-2],prePOINTER[i][m-1]);
      if(tempPOINT!=hPOINT)
      {
        preHASH[hPOINT+1]=i;
        preHASH[tempPOINT]=i;
        hPOINT=tempPOINT;
      }
    }
    preHASH[hPOINT+1]=i;
}


void prePrefix(char prePOINTER[][MAXLENGTH],long rows,long m,long preFIX[])
/* calculate the PREFIX TABLE,for text to match its prefix */
{
     long i;
     long mpoint;  

     for(i=0;i<rows;i++)
     {
        mpoint=strlen(prePOINTER[i]);
        preFIX[i]=calHash(prePOINTER[i][mpoint-m],prePOINTER[i][mpoint-m+1]);
     }
}

void Mwm(char *Tt,char *Patts[],int s)
{
    char prePOINTER[5][MAXLENGTH];
    long preSHIFT[SHIFTSIZE],preHASH[SHIFTSIZE],preFIX[s],preFIND[s];
    long i,px,qx;
    long m,textm,htext,shiftext,prefixtext;
    long pstart,pend;
    long lengthP;
    unsigned char * T;//dzh emend at 2004.10.22
    _U64 sta1,end1;
    double ela;
    char* text = Tt;
    char** patts = Patts;
    T=(unsigned char*)Tt;
    m=preShift(Patts,s,preSHIFT);
    prePoint(Patts,s,prePOINTER);
    preHash(prePOINTER,s,preHASH);
    prePrefix(prePOINTER,s,m,preFIX);
    preFind(prePOINTER,Patts,s,preFIND);
     
    textm=strlen((char*)T);
    i=m-1;
    Mtime(&sta1);
    while(i<textm)
    {
       htext=calHash(T[i-1],T[i]);
       shiftext=preSHIFT[htext];
       if(shiftext==0)
       {
         prefixtext=calHash(T[i-m+1],T[i-m+2]);
         pstart=preHASH[htext];
         pend=preHASH[htext+1];

         while(pstart<pend)
         {
           if(prefixtext!=preFIX[pstart])
           {
             pstart++;
             continue;
           }

           lengthP=strlen(prePOINTER[pstart]);
           if(i+1>=lengthP)
           {
              qx=i+1-lengthP;
              px=0;
              while(prePOINTER[pstart][px]==T[qx])
              {
                 px++;
                 qx++;
              }
              if(px==lengthP)
		      OUTPUTs(preFIND[pstart],(qx-lengthP));
           }
           pstart++;
         }
         shiftext=1; 
        }
        i+=shiftext;
     }
    Mtime(&end1);
    ela=Mdifftime(sta1,end1);
    printf("%20.15f,,,,",ela);
}
