#ifdef USE_SSE4
/*
 * =====================================================================================
 *
 *       Filename:  strstrsse42a.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/13/2011 07:20:36 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  DAI ZHENGHUA (), djx.zhenghua@gmail.com
 *        Company:  
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <emmintrin.h>
#include <nmmintrin.h>
#include <tmmintrin.h>
#include <pmmintrin.h>
#include "strstrsse.h"
#include <string.h> // for strlen

#include "report.h"
char* strchrsse(const char *str,char c);
static char* __STRSTRSSE42A_gt16(const char* text, const char* pattern);
#define STRCHR strchrsse
#define REPORT(i) {if( report_function((char*)ss1, (char*)i-(char*)ss1, (char*)s2)== SEARCH_STOP) return (i);};
#ifndef  STRSTRSSE42A
#define  STRSTRSSE42A strstrsse42a
#endif   /* ----- #ifndef strstrsse42a_INC  ----- */

//#define SEARCH_PRE_F(s2,s1)   _mm_cmpistrc(s2,s1,_SIDD_UNIT_MASK|_SIDD_CMP_EQUAL_ORDERED) 
#define SEARCH_PRE_F(s2,s1)   _mm_cmpistri(s2,s1,0x0c) 
#define SEARCH_PRE_M(s2,s1)   _mm_cmpistrm(s2,s1,_SIDD_UNIT_MASK|_SIDD_CMP_EQUAL_ORDERED)
#define SIMD_LOAD(p)   _mm_load_si128((__m128i*)(p))   
#define SIMD_LOADU(p)  _mm_loadu_si128((__m128i*)(p))   
#define SS_GET_MASK(V) _mm_movemask_epi8(V)
#define SS_XAND(s1,s2) _mm_and_si128(s1,s2)

#define bit_reverse(x)  _mm_shuffle_epi8(x,shf_indexV)
#    define bsf(x) __builtin_ctz(x) 

typedef unsigned int u32;
static unsigned char IndexVector[][16] = {
    {},// 0
    {},// 1
    /*  
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x01},// 2 
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x02,0x01},// 3
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x03,0x02,0x01},// 4
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x04,0x03,0x02,0x01},// 5
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x05,0x04,0x03,0x02,0x01},// 6
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x06,0x05,0x04,0x03,0x02,0x01},// 7
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x07,0x06,0x05,0x04,0x03,0x02,0x01},// 8
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01},// 9
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01},// a
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01},// b
        {0xFF,0xFF,0xFF,0xFF,0xFF,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01},// c
        {0xFF,0xFF,0xFF,0xFF,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01},// d
        {0xFF,0xFF,0xFF,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01},// e
        {0xFF,0xFF,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01},// f
        {0xFF,0x0F,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01},// 0x10
        */
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0f},// 2 
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0f,0x0e},// 3
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0f,0x0e,0x0d},// 4
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0f,0x0e,0x0d,0x0c},// 5
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0f,0x0e,0x0d,0x0c,0x0b},// 6
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a},// 7
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09},// 8
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08},// 9
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07},// a
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06},// b
        {0xFF,0xFF,0xFF,0xFF,0xFF,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05},// c
        {0xFF,0xFF,0xFF,0xFF,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04},// d
        {0xFF,0xFF,0xFF,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03},// e
        {0xFF,0xFF,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02},// f
        {0xFF,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01},// 0x10
};                                                                      

static inline unsigned int has_byte_null(__m128i a0)
{
    return _mm_cmpistrz (a0, a0, 0x0c);
}

char* STRSTRSSE42A(const char* text, const char* pattern)
{
    const unsigned char *ss1 = (const unsigned char*)text;
    const unsigned char *s2 = (const unsigned char*)pattern;
    unsigned char * chPtrAligned = (unsigned char*)text;
    unsigned int ret_z;
    unsigned int pref=0;
    unsigned int plen = strlen(pattern);
    __m128i premV, postmV;
    __m128i * sseiPtr = (__m128i *) text;
    __m128i sseiWord0 ;//= *sseiPtr ;
    __m128i sseiPattern; 
    __m128i sseiPattern2;
    __m128i shf_indexV;
    if(text==NULL) return NULL;
    if(text[0] == 0) {
        return pattern[0]?NULL:(char*)text;
    }
    if(pattern ==NULL) return NULL;
    if(pattern[0] == 0) return (char*)text;
	if(pattern[1] == 0) return STRCHR(text,pattern[0]); 
    if(plen > 16) return __STRSTRSSE42A_gt16(text, pattern); 

    {
        int j;
        int preBytes = 16 - (((size_t) text) & 15);
        char	chara = pattern[0];
        char	charb = pattern[1];
//        char	charc = pattern[2];
        char* bytePtr;

        preBytes &= 15;
        if (preBytes == 0) goto alignStart;
        chPtrAligned = (unsigned char*)text + preBytes;
        for(j =0;j< preBytes; j++){
            if(text[j] ==0) return NULL;
            if(text[j] == chara){
                if(text[j+1] == charb){
                    int i=1;
                    bytePtr = (char*) & text[j];
                    while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
                    if(pattern[i] == 0) REPORT(bytePtr);
                    if(bytePtr[i] == 0) return NULL;
                }
            }
        }
        sseiPtr = (__m128i *) chPtrAligned;
    }

alignStart:
    {
        if(plen <= 16){
            shf_indexV = SIMD_LOAD(IndexVector[plen]);
            __attribute__ ((aligned (16))) char pbuf[16];
            unsigned int i=0, j=0;
            for(i =0;i<16-plen ;i++){
                pbuf[i] = 0xff;
            }
            for(;i<16;i++)
                pbuf[i] = pattern[j++];
            sseiPattern2 = SIMD_LOAD(pbuf);
        }
    }

    sseiPattern = SIMD_LOADU(pattern);
    sseiWord0 = SIMD_LOAD(sseiPtr) ;
    pref = SEARCH_PRE_F(sseiPattern, sseiWord0);
    ret_z = has_byte_null(sseiWord0); 
    while(!ret_z){
        //! find out the prefix
        __m128i postmV2;
        __m128i flagV;
        u32 flag16;
        while(( pref==16)&&( ret_z ==0)){
            sseiPtr ++;
            sseiWord0 = SIMD_LOAD(sseiPtr) ;
            ret_z = has_byte_null(sseiWord0); 
            pref = SEARCH_PRE_F(sseiPattern, sseiWord0);
        }
        if(pref <= 16 - plen){
            REPORT((((char*)sseiPtr)+pref));
        }
#if 1
        premV = SEARCH_PRE_M(sseiPattern, sseiWord0);
        sseiPtr ++;
        sseiWord0 = SIMD_LOAD(sseiPtr) ;
        postmV = SEARCH_PRE_M(sseiWord0, sseiPattern2);
        postmV2= bit_reverse(postmV);
        flagV  = SS_XAND(premV, postmV2);
        flag16 = SS_GET_MASK(flagV);

        if(flag16){
            int idx = bsf(flag16);
            REPORT((((char*)sseiPtr)-16+idx));
        }
#else
        {
            unsigned int postf=0;
            sseiPtr ++;
            sseiWord0 = SIMD_LOAD(sseiPtr) ;
            postf = SEARCH_PRE_F(sseiWord0, sseiPattern2);
            if( postf + pref == 32 - plen){
                REPORT(sseiPtr);
            }
        }
#endif
        pref = SEARCH_PRE_F(sseiPattern, sseiWord0);
        ret_z = has_byte_null(sseiWord0); 
    }
    //! now there is less than 16 char left
    if( (pref != 16)&& (pref <= 16 - plen)){
        char* p = ((char*)sseiPtr ) + pref; 
        char* pend = p + plen;
        while((p < pend) && (*p != 0)) p++;
        if(p == pend){
                REPORT(pend - plen);
        }
    }
    return NULL;
}

__inline int simple_strcmp(const char* s1, const char* s2)
{
    while(*s2 && *s1 &&(*s1 == *s2)){
        s1 ++;
        s2 ++;
    }
    return *s2;
}

/*Don't call this function directly. It must only be called in STRSTRSSE42A  */
static char* __STRSTRSSE42A_gt16(const char* text, const char* pattern)
{
    const unsigned char *ss1 = (const unsigned char*)text;
    const unsigned char *s2 = (const unsigned char*)pattern;
    unsigned char * chPtrAligned = (unsigned char*)text;
    unsigned int ret_z;
    unsigned int pref=0;
    int plen = 16;
    __m128i premV, postmV;
    __m128i *sseiPtr = (__m128i *) text;
    __m128i sseiWord0 ;//= *sseiPtr ;
    __m128i sseiPattern; 
    __m128i shf_indexV;
    /*  
        if(text==NULL) return NULL;
        if(text[0] == 0) {
        return pattern[0]?NULL:text;
        }
        if(pattern ==NULL) return NULL;
        if(pattern[0] == 0) return text;
        if(pattern[1] == 0) return STRCHR(text,pattern[0]); 
        */
    plen = 16;
    {
        int j;
        int preBytes = 16 - (((size_t) text) & 15);
        char	chara = pattern[0];
        char	charb = pattern[1];
//        char	charc = pattern[2];
        char* bytePtr;

        preBytes &= 15;
        if (preBytes == 0) goto alignStart;
        chPtrAligned = (unsigned char*)text + preBytes;
        for(j =0;j< preBytes; j++){
            if(text[j] ==0) return NULL;
            if(text[j] == chara){
                if(text[j+1] == charb){
                    int i=1;
                    bytePtr = (char*)& text[j];
                    while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
                    if(pattern[i] == 0) REPORT(bytePtr);
                    if(bytePtr[i] == 0) return NULL;
                }
            }
        }
        sseiPtr = (__m128i *) chPtrAligned;
    }

alignStart:
    shf_indexV = SIMD_LOAD(IndexVector[16]);
    sseiPattern = SIMD_LOADU(pattern);
    sseiWord0 = SIMD_LOAD(sseiPtr) ;
    pref = SEARCH_PRE_F(sseiPattern, sseiWord0);
    ret_z = has_byte_null(sseiWord0); 
    while(!ret_z){
        //! find out the prefix
        __m128i postmV2;
        __m128i flagV;
        u32 flag16;
        while(( pref==16)&&( ret_z ==0)){
            sseiPtr ++;
            sseiWord0 = SIMD_LOAD(sseiPtr) ;
            ret_z = has_byte_null(sseiWord0); 
            pref = SEARCH_PRE_F(sseiPattern, sseiWord0);
        }
        if(pref == 0){
            //! find out the first 16 bytes;
            if(simple_strcmp((((char*)sseiPtr)+pref+16), pattern + 16)==0)
            REPORT((((char*)sseiPtr)+pref));
        }
#if 1
        premV = SEARCH_PRE_M(sseiPattern, sseiWord0);
        sseiPtr ++;
        sseiWord0 = SIMD_LOAD(sseiPtr) ;
        postmV = SEARCH_PRE_M(sseiWord0, sseiPattern);
        postmV2= bit_reverse(postmV);
        flagV  = SS_XAND(premV, postmV2);
        flag16 = SS_GET_MASK(flagV);

        if(flag16){
            int idx = bsf(flag16);
            if(simple_strcmp((((char*)sseiPtr)+idx), pattern + 16)==0)
            REPORT((((char*)sseiPtr)-16+idx));
        }
#else
        {
            unsigned int postf=0;
            sseiPtr ++;
            sseiWord0 = SIMD_LOAD(sseiPtr) ;
            postf = SEARCH_PRE_F(sseiWord0, sseiPattern);
            if( postf + pref == 32 - plen){
                REPORT(sseiPtr);
            }
        }
#endif
        pref = SEARCH_PRE_F(sseiPattern, sseiWord0);
        ret_z = has_byte_null(sseiWord0); 
    }
    return NULL;
}
#else
#include <stdlib.h>
#include <string.h>
char* strstrsse42a(const char* text, const char* pattern)
{
return strstr(text, pattern);
}
#endif
