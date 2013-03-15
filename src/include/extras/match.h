/**
 * =====================================================================================
 *       @file    match.h
 *
 *       @brief  search pat from text, return the first hit or report all the hits.
 *
 *       @author  DAI ZHENGHUA (), djx.zhenghua@gmail.com
 *       @version 1.0
 *       @date    
 *
 *       Revision none
 *       Compiler gcc
 *       Company  
 * =====================================================================================
 */
#ifndef STRINGMATCH_HEADER__
#define STRINGMATCH_HEADER__
#define MAXPAT 256 //!the maximal length of Pattern

#define CHARACTERS 256 //!the length of   set of character  
#define ASIZE 256
#if __WORDSIZE == 32 
#define WORD_SIZE 32
#else
#define WORD_SIZE 64
#endif
typedef unsigned int Uword;
typedef unsigned int u32;
typedef unsigned long long u64;
# define strstrsse42 __strstr_sse42

#define MAXINT2 2147483648 
#include "stdlib.h"
#define __const1
#define __const2
#ifdef __cplusplus 
extern "C"{
#endif
typedef void* (* preSearchFunc)(__const1 char* pat, int m);
typedef void* (* search1Func)(__const1 char* text, void* pat);
typedef void* (* search2Func)(__const1 char* text, __const2 char* pat, int n, int m);
typedef void* (* search3Func)(__const1 char* text, void* pat, int n, int m);

/**
 * @name single-pattern
 * @brief the string ends with \0.
 * @param text 
 * @param pat
 * @return depends on the report function 
 * @{ */
char* Sbf        (__const1 char* text, __const2 char* pat);//!brute force  
char* bfstr      (__const1 char* text, __const2 char* pat);
char* Smp        (__const1 char* text, __const2 char* pat);//!MP 
char* Skmp       (__const1 char* text, __const2 char* pat);//!KMP
                                    
char* Sbm        (__const1 char* text, __const2 char* pat);//!bm 
char* Sbmh       (__const1 char* text, __const2 char* pat);//! Horspool 
char* Sbmhs      (__const1 char* text, __const2 char* pat);//!SUNDA quich search 
char* Ssmith     (__const1 char* text, __const2 char* pat);//!smith  
                                    
char* Sbom       (__const1 char* text, __const2 char* pat);//!BOM 
char* Sshiftor   (__const1 char* text, __const2 char* pat);//!shift-or  
char* Sshiftand  (__const1 char* text, __const2 char* pat);//!shift-and  
char* SshiftorW  (__const1 char* text, __const2 char* pat);//!shift-or  
                                    
char* Skr        (__const1 char* text, __const2 char* pat);//!KR 
char* Sbyh       (__const1 char* text, __const2 char* pat);//!
char* Sskip      (__const1 char* text, __const2 char* pat);//!
char* Skmpskip   (__const1 char* text, __const2 char* pat);//! kmp
char* bfstr      (__const1 char* text, __const2 char* pat);//! gcc 
char* lstrstr    (__const1 char* text, __const2 char* pat);//! gcc 
char* strstrsse  (__const1 char* text, __const2 char* pat);//! gcc 
char* strstrsse42(__const1 char* text, __const2 char* pat);//! gcc 
char* strstrsse42a(__const1 char* text, __const2 char* pat);//! gcc 
/**  @} */

/**
 * @name single-patten with length
 * @brief the string has length.
 * @param text 
 * @param pat
 * @param n the length of text
 * @param m the length of pat
 * @return depends on the report function 
 * @{ */
char* Sbf2      (__const1 char* text, __const2 char* pat, int n, int m);//!brute force  
char* bfstr2    (__const1 char* text, __const2 char* pat, int n, int m);
char* Smp2      (__const1 char* text, __const2 char* pat, int n, int m);//!MP 
char* Skmp2     (__const1 char* text, __const2 char* pat, int n, int m);//!KMP
                                   
char* Sbm2      (__const1 void* text, __const2 void* pat, int n, int m);//!bm 
char* Sbmh2     (__const1 char* text, __const2 char* pat, int n, int m);//! Horspool 
char* Sbmhs2    (__const1 char* text, __const2 char* pat, int n, int m);//!SUNDAY£¨”÷≥∆ quich search 
char* Ssmith2   (__const1 char* text, __const2 char* pat, int n, int m);//!smith  
                                   
char* Sbom2     (__const1 char* text, __const2 char* pat, int n, int m);//!BOM 
char* Sshiftor2 (__const1 char* text, __const2 char* pat, int n, int m);//!shift-or  
char* Sshiftand2(__const1 char* text, __const2 char* pat, int n, int m);//!shift-and  
char* SshiftorW2(__const1 char* text, __const2 char* pat, int n, int m);//!shift-or  
                                   
char* Skr2      (__const1 char* text, __const2 char* pat, int n, int m);//!KR 
char* Sbyh2     (__const1 char* text, __const2 char* pat, int n, int m);//!
char* Sskip2    (__const1 char* text, __const2 char* pat, int n, int m);//!
char* Skmpskip2 (__const1 char* text, __const2 char* pat, int n, int m);//! kmpÃ¯‘æ 
char* bfstr2    (__const1 char* text, __const2 char* pat, int n, int m);//! gcc 
/**  @} */

/**
 * @name single-pattern with pre-processed structure 
 * @brief pre process the pattern and return a structure representing the pattern.
 * @{ */
void* preSbf      (__const1 char* pat, int m);//!brute force
void* prebfstr    (__const1 char* pat, int m);
void* preSmp      (__const1 char* pat, int m);//!MP
void* preSkmp     (__const1 char* pat, int m);//!KMP
                                   
void* preSbm      (__const1 char* pat, int m);//!bm 
void* preSbmh     (__const1 char* pat, int m);//! Horspool 
void* preSbmhs    (__const1 char* pat, int m);//!SUNDAY quich search 
void* preSsmith   (__const1 char* pat, int m);//!smith  
                                   
void* preSdfa     (__const1 char* pat, int m);//!
void* preSbom     (__const1 char* pat, int m);//!BOM 
void* preSshiftor (__const1 char* pat, int m);//!shift-or  
void* preSshiftand(__const1 char* pat, int m);//!shift-and  
void* preSshiftorW(__const1 char* pat, int m);//!shift-or  
                                   
void* preSkr      (__const1 char* pat, int m);//!KR 
void* preSbyh     (__const1 char* pat, int m);//! 
void* preSskip    (__const1 char* pat, int m);//! 
void* preSkmpskip (__const1 char* pat, int m);//! kmp 
void* prebfstr    (__const1 char* pat, int m);//! gcc 


/**
 * @brief search using the pre-processed pattern. 
 */
char* Sbf3      (__const1 char* text, void* pat, int n, int m);//!brute force  
char* bfstr3    (__const1 char* text, void* pat, int n, int m);
char* Smp3      (__const1 char* text, void* pat, int n, int m);//!MP 
char* Skmp3     (__const1 char* text, void* pat, int n, int m);//!KMP
                                   
char* Sbm3      (__const1 char* text, void* pat, int n, int m);//!bm 
char* Sbmh3     (__const1 char* text, void* pat, int n, int m);//! Horspool 
char* Sbmhs3    (__const1 char* text, void* pat, int n, int m);//!SUNDAY£¨”÷≥∆ quich search 
char* Ssmith3   (__const1 char* text, void* pat, int n, int m);//!smith  
                                   
char* Sbom3     (__const1 char* text, void* pat, int n, int m);//!BOM 
char* Sshiftor3 (__const1 char* text, void* pat, int n, int m);//!shift-or  
char* Sshiftand3(__const1 char* text, void* pat, int n, int m);//!shift-and  
char* SshiftorW3(__const1 char* text, void* pat, int n, int m);//!shift-or  
                                   
char* Skr3      (__const1 char* text, void* pat, int n, int m);//!KR 
char* Sbyh3     (__const1 char* text, void* pat, int n, int m);//!
char* Sskip3    (__const1 char* text, void* pat, int n, int m);//!
char* Skmpskip3 (__const1 char* text, void* pat, int n, int m);//! kmp
char* bfstr3    (__const1 char* text, void* pat, int n, int m);//! gcc 
/**  @} */

/**
 * @multi-pattern
 * @{ */
void Mac        (__const1 char *T,char *Ps[],int s);//!AC
void Mwm        (__const1 char *T,char *Ps[],int s);//!WM
void acsm       (__const1 char *T,char *Ps[],int s);//!WM
void snortwm    (__const1 char *T,char *Ps[],int s);//!WM
void Mshiftor   (__const1 char *T,char *Ps[],int s);//!shift-0r 
/**  @} */


#ifdef __cplusplus 
}
#endif

#endif
