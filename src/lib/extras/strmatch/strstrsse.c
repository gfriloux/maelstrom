#ifdef USE_SSE
/*
 * =====================================================================================
 *
 *       Filename:  strstr.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/22/2008 
 *       Revision:   
 *       Compiler:   
 *
 *         Author:  Zhenghua Dai (Zhenghua Dai), djx.zhenghua@gamil.com
 *        Company:  dzh
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <emmintrin.h>
#include <xmmintrin.h>
#include "strstrsse.h"
#define USE_BTR 
char* strchrsse(const char *str,char c);
char* strstrabsse(const char* text, const char* pattern);
//#define REPORT(i) return i;
#ifndef REPORT
#define REPORT(i) {if( report_function(text, i-text, pattern)== SEARCH_STOP) return (char*)i;};
#endif

static inline unsigned int hasByteC16(__m128i a0, register __m128i sseic)
{
	return ((_mm_movemask_epi8(_mm_cmpeq_epi8(a0,sseic))  )   );	

}

static inline unsigned int hasByteC(__m128i a0, __m128i a1, register __m128i sseic)
{
	return ((_mm_movemask_epi8(_mm_cmpeq_epi8(a0,sseic))  )  | \
			(_mm_movemask_epi8(_mm_cmpeq_epi8(a1,sseic)) << 16) \
		   );	
}
#define haszeroByte hasByteC
inline static int strcmpInline(char* str1,char* str2)
{
	while((*str2) && (*str1 == * str2)) {
		str1++;
		str2++;
	}
	if(*str2 == 0) return 0;
	return 1;
}

#    define bsf(x) __builtin_ctz(x) 
char* strstrsse(const char* text, const char* pattern)
{
	__m128i * sseiPtr = (__m128i *) text;
	unsigned char * chPtrAligned = (unsigned char*)text;
	__m128i sseiWord0 ;//= *sseiPtr ;
	__m128i sseiWord1 ;//= *sseiPtr ;
	__m128i sseiZero = _mm_set1_epi8(0);
	char chara;
	char charb;
	char charc;
	register __m128i byte16a;
	register __m128i byte16b;
	register __m128i byte16c;
	char* bytePtr = (char*)text;
	if(text==NULL) return NULL;
	if(text[0] == 0) {
		return pattern[0]?NULL:(char*)text;
	}
	if(pattern ==NULL) return NULL;
	if(pattern[0] == 0) return (char*)text;
	if(pattern[1] == 0) return strchrsse(text,pattern[0]); 
	if(pattern[2] == 0) return strstrabsse(text,pattern); 
	chara = pattern[0];
	charb = pattern[1];
	charc = pattern[2];
	byte16a = _mm_set1_epi8(chara);
	byte16b = _mm_set1_epi8(charb);
	byte16c = _mm_set1_epi8(charc);
#if 1
	//! the pre-byte that is not aligned.
	{
		int j;
		int preBytes = 16 - (((size_t) text) & 15);
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


#endif
alignStart:
	sseiWord0 = *sseiPtr;
	sseiWord1 = *(sseiPtr+1);
	while( haszeroByte(sseiWord0,sseiWord1,sseiZero) ==0) 
	{
		unsigned int reta ;
//searcha:
		reta = hasByteC(sseiWord0,sseiWord1,  byte16a);
		if(reta!=0 ) {
			unsigned int retb ;
			unsigned int retc ;
//findouta:		
			retb = hasByteC(sseiWord0,sseiWord1,  byte16b);
			retc = hasByteC(sseiWord0,sseiWord1,  byte16c);
findoutb:
			reta = (reta ) & (retb >> 1);
			reta = (reta ) & (retc >> 2);
			if(reta)
			{
				// have ab
				int i=1;
				char * bytePtr0 = (char*) ( sseiPtr );
#ifdef USE_BTR
				while(reta){
					int idx ;
					idx = bsf(reta);
					bytePtr = bytePtr0 + idx ;
					i = 3;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) REPORT(bytePtr);
					//reta = reta & ( ~(1<<idx));
					__asm__ ( "btr %1, %0"
							:"=r"(reta)
							:"r"(idx),"0"(reta)
							);

				}
#else
				//search completely : version no bsf instruction 
				for(j =0;j<8;j++){
					if(reta & 0xff) {
						if(bytePtr0[0] == chara)
							//if(reta & 1)
						{
							i =1;
							bytePtr = bytePtr0 ;
							while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
							if(pattern[i] == 0) REPORT(bytePtr);
						}
						if(bytePtr0[1] == chara)
							//if(reta & 2)
						{
							i =1;
							bytePtr = bytePtr0 + 1;
							while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
							if(pattern[i] == 0) REPORT(bytePtr);
						}
						if(bytePtr0[2] == chara)
							//if(reta & 4)
						{
							i =1;
							bytePtr = bytePtr0 + 2;
							while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
							if(pattern[i] == 0) REPORT(bytePtr);
						}
						if(bytePtr0[3] == chara)
							//if(reta & 8)
						{
							i =1;
							bytePtr = bytePtr0 + 3;
							while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
							if(pattern[i] == 0) REPORT(bytePtr);
						}
					}
					reta = reta >> 4;
					bytePtr0 += 4;
				}
#endif

			}
			// search b
			sseiPtr += 2;
			sseiWord0 = *sseiPtr;
			sseiWord1 = *(sseiPtr+1);

			while( haszeroByte(sseiWord0,sseiWord1,sseiZero) ==0){ 
				retc = hasByteC(sseiWord0,sseiWord1,  byte16c);
				if(retc !=0){
					//! this line will decrease the performance, I am a little amazed. if(retc& (3<<30)) 
					{
						if((*((char*) sseiPtr)) == charb){
							//a|||b000
							char * bytePtr = ((char*) ( sseiPtr )) -1;
							if(bytePtr[0] == chara){
								int i=1;
								while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
								if(pattern[i] == 0) REPORT(bytePtr);
								if(bytePtr[i] == 0) return NULL;
							}
						}
#if 1
						if((*((char*) sseiPtr)) == charc){
							//ab||||c000
							char * bytePtr = ((char*) ( sseiPtr )) -2;
							if(bytePtr[0] == chara){
								int i=1;
								while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
								if(pattern[i] == 0) REPORT(bytePtr);
								if(bytePtr[i] == 0) return NULL;
							}
						}
					}
#endif
					reta = hasByteC(sseiWord0,sseiWord1,  byte16a);
					if(reta !=0) 
					{
						retb = hasByteC(sseiWord0,sseiWord1,  byte16b);
						if(retb !=0) 
							goto findoutb;
						else
						{
							if(*(char*) (sseiPtr +2) == charb ){
								char * bytePtr = ((char*) ( sseiPtr +2  )) -1;
								if(bytePtr[0] == chara){
									int i=1;
									while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
									if(pattern[i] == 0) REPORT(bytePtr);
									if(bytePtr[i] == 0) return NULL;
								}
							}
							goto nextWord;					
						}
					}	
					else{
						goto nextWord;					
					}
				}
				sseiPtr += 2;
				sseiWord0 = *sseiPtr;
				sseiWord1 = *(sseiPtr+1);
			}
			// search  from (char*)sseiPtr
			char * bytePtr = ((char*) ( sseiPtr )) -1;
			if(bytePtr[0] == chara){
				int i=1;
				while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
				if(pattern[i] == 0) REPORT(bytePtr);
			}
			bytePtr --;
			if(bytePtr[0] == chara){
				int i=1;
				while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
				if(pattern[i] == 0) REPORT(bytePtr);
			}

			goto prePareForEnd;
		}
nextWord:
		sseiPtr += 2;
		sseiWord0 = *sseiPtr;
		sseiWord1 = *(sseiPtr+1);
	}
prePareForEnd:
	{
		unsigned int reta;
		unsigned int retb;
		reta =hasByteC(sseiWord0,sseiWord1,  byte16a);
		retb =hasByteC(sseiWord0,sseiWord1,  byte16b);
		if(((reta<<1) & retb)){
			bytePtr = (char*)sseiPtr;
			while(*bytePtr){
				if(*bytePtr == chara) {
					int i=1;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) REPORT(bytePtr);
					if(bytePtr[i] == 0) return NULL;

				}
				bytePtr++;
			}
		}
	}
	return NULL;
}


char* strstrabsse(const char* text, const char* pattern)
{
	__m128i * sseiPtr = (__m128i *) text;
	unsigned char * chPtrAligned = (unsigned char*)text;
	__m128i sseiWord0 ;//= *sseiPtr ;
	__m128i sseiWord1 ;//= *sseiPtr ;
	__m128i sseiZero = _mm_set1_epi8(0);
	char chara = pattern[0];
	char charb = pattern[1];
	register __m128i byte16a;
	register __m128i byte16b;
	char* bytePtr = (char*)text;
	if(pattern ==NULL) return NULL;
	if(pattern[0] == 0) return NULL;
	if(pattern[1] == 0) return strchrsse(text,pattern[0]); 
	byte16a = _mm_set1_epi8(chara);
	byte16b = _mm_set1_epi8(charb);
#if 1
	//! the pre-byte that is not aligned.
	{
		int j;
		int preBytes = 16 - (((size_t) text) & 15);
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


#endif
alignStart:
	sseiWord0 = *sseiPtr;
	sseiWord1 = *(sseiPtr+1);
	while( haszeroByte(sseiWord0,sseiWord1,sseiZero) ==0) 
	{
		unsigned int reta ;
//searcha:
		reta = hasByteC(sseiWord0,sseiWord1,  byte16a);
		if(reta!=0 ) {
			unsigned int retb ;
//findouta:		
			retb = hasByteC(sseiWord0,sseiWord1,  byte16b);
findoutb:
			reta = (reta ) & (retb >> 1);
			if(reta)
			{
				// have ab
				char * bytePtr0 = (char*) ( sseiPtr );
				int j;
				//printf("test::%0x,%d\n",reta ,bytePtr0 -text);
				bytePtr = (char*) ( sseiPtr );
				for(j =0;j<8;j++){
					if(reta & 0xff) {
						//if(bytePtr0[0] == chara)
						if(reta & 1)
						{
							bytePtr = bytePtr0 ;
							REPORT(bytePtr);
						}
						//if(bytePtr0[1] == chara)
						if(reta & 2)
						{
							bytePtr = bytePtr0 + 1;
							REPORT(bytePtr);
						}
						//if(bytePtr0[2] == chara)
						if(reta & 4)
						{
							bytePtr = bytePtr0 + 2;
							REPORT(bytePtr);
						}
						//if(bytePtr0[3] == chara)
						if(reta & 8)
						{
							bytePtr = bytePtr0 + 3;
							REPORT(bytePtr);
						}
					}
					reta = reta >> 4;
					bytePtr0 += 4;
				}
			}
			// search b
			sseiPtr += 2;
			sseiWord0 = *sseiPtr;
			sseiWord1 = *(sseiPtr+1);

			while( haszeroByte(sseiWord0,sseiWord1,sseiZero) ==0){ 
				retb = hasByteC(sseiWord0,sseiWord1,  byte16b);
				if(retb !=0){
					// findout b
					if((*((char*) sseiPtr)) == charb){
						//b000
						char * bytePtr = ((char*) ( sseiPtr )) -1;
						if(bytePtr[0] == chara){
							int i=1;
							while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
							if(pattern[i] == 0) REPORT(bytePtr);
							if(bytePtr[i] == 0) return NULL;
						}

					}
					reta = hasByteC(sseiWord0,sseiWord1,  byte16a);
					if(reta !=0) 
						goto findoutb;
					else{
						goto nextWord;					
					}
				}
				sseiPtr += 2;
				sseiWord0 = *sseiPtr;
				sseiWord1 = *(sseiPtr+1);
			}
			// search  from (char*)sseiPtr
			char * bytePtr = ((char*) ( sseiPtr )) -1;
			if(bytePtr[0] == chara){
				int i=1;
				while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
				if(pattern[i] == 0) REPORT(bytePtr);
			}

			goto prePareForEnd;
		}
nextWord:
		sseiPtr += 2;
		sseiWord0 = *sseiPtr;
		sseiWord1 = *(sseiPtr+1);
	}
prePareForEnd:
	{
		unsigned int reta;
		unsigned int retb;
		reta =hasByteC(sseiWord0,sseiWord1,  byte16a);
		retb =hasByteC(sseiWord0,sseiWord1,  byte16b);
		if(((reta<<1) & retb)){
			bytePtr = (char*)sseiPtr;
			while(*bytePtr){
				if(*bytePtr == chara) {
					int i=1;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) REPORT(bytePtr);
					if(bytePtr[i] == 0) return NULL;

				}
				bytePtr++;
			}
		}
	}
	return NULL;
}

char* strchrsse(const char *text,char c) 
{
	const char *char_ptr=text;
	const __m128i* m128_ptr;
	char pattern[2] ={c,0};
	__m128i byte16c= _mm_set1_epi8(c);
	__m128i sseiZero = _mm_set1_epi8(0);
	__m128i m128word;
	for (char_ptr = text; ((size_t)char_ptr 
				& (sizeof(__m128) - 1)) != 0;
			++char_ptr) {
		if (*char_ptr == '\0')return NULL;
		if (*char_ptr == c)
			REPORT(char_ptr);
	}

	m128_ptr = (const __m128i*)char_ptr;


	m128word= *m128_ptr;
	while ((hasByteC16(m128word, sseiZero))==0) {
		if (hasByteC16(m128word, byte16c)  != 0) {
			int i;
			const char *cp = (const char*)(m128_ptr);
			//const char* endcp = cp + 16;
			for(i=0;i<16;i++){
				if(*cp == c) REPORT(cp);
				if(*cp==0) return NULL;
					cp++;
			}
		}
		m128_ptr++;
		m128word= *m128_ptr;
	}
	{
		const char* cp = (const char*) m128_ptr;
		while(*cp){
				if(*cp == c) REPORT(cp);
				cp++;
		}
	}
	return NULL;
}

char* strstrabxsse(char* text, char* pattern)
{
	__m128i * sseiPtr = (__m128i *) text;
	unsigned char * chPtrAligned = (unsigned char*)text;
	__m128i sseiWord0 ;//= *sseiPtr ;
	__m128i sseiWord1 ;//= *sseiPtr ;
	__m128i sseiZero = _mm_set1_epi8(0);
	char chara = pattern[0];
	char charb = pattern[1];
	register __m128i byte16a;
	register __m128i byte16b;
	char* bytePtr =text;
	if(pattern ==NULL) return NULL;
	if(pattern[0] == 0) return NULL;
	if(pattern[1] == 0) return strchrsse(text,pattern[0]); 
	byte16a = _mm_set1_epi8(chara);
	byte16b = _mm_set1_epi8(charb);
#if 1
	//! the pre-byte that is not aligned.
	{
		int j;
		int preBytes = 16 - (((size_t) text) & 15);
		preBytes &= 15;
		if (preBytes == 0) goto alignStart;
		chPtrAligned = (unsigned char*)text + preBytes;
		for(j =0;j< preBytes; j++){
			if(text[j] ==0) return NULL;
			if(text[j] == chara){
				if(text[j+1] == charb){
					int i=1;
					bytePtr = & text[j];
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) REPORT(bytePtr);
					if(bytePtr[i] == 0) return NULL;
				}
			}
		}
		sseiPtr = (__m128i *) chPtrAligned;
	}


#endif
alignStart:
	sseiWord0 = *sseiPtr;
	sseiWord1 = *(sseiPtr+1);
	while( haszeroByte(sseiWord0,sseiWord1,sseiZero) ==0) 
	{
		unsigned int reta ;
//searcha:
		reta = hasByteC(sseiWord0,sseiWord1,  byte16a);
		if(reta!=0 ) {
			unsigned int retb ;
//findouta:		
			retb = hasByteC(sseiWord0,sseiWord1,  byte16b);
findoutb:
			reta = (reta ) & (retb >> 1);
			if(reta)
			{
				// have ab
				int i=1;
				char * bytePtr0 = (char*) ( sseiPtr );
				int j;
				//printf("test::%0x,%d\n",reta ,bytePtr0 -text);
				bytePtr = (char*) ( sseiPtr );
				for(j =0;j<8;j++){
					if(reta & 0xff) {
						if(bytePtr0[0] == chara)
							//if(reta & 1)
						{
							i =1;
							bytePtr = bytePtr0 ;
							while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
							if(pattern[i] == 0) REPORT(bytePtr);
						}
						if(bytePtr0[1] == chara)
							//if(reta & 2)
						{
							i =1;
							bytePtr = bytePtr0 + 1;
							while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
							if(pattern[i] == 0) REPORT(bytePtr);
						}
						if(bytePtr0[2] == chara)
							//if(reta & 4)
						{
							i =1;
							bytePtr = bytePtr0 + 2;
							while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
							if(pattern[i] == 0) REPORT(bytePtr);
						}
						if(bytePtr0[3] == chara)
							//if(reta & 8)
						{
							i =1;
							bytePtr = bytePtr0 + 3;
							while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
							if(pattern[i] == 0) REPORT(bytePtr);
						}
					}
					reta = reta >> 4;
					bytePtr0 += 4;
				}
			}
			// search b
			sseiPtr += 2;
			sseiWord0 = *sseiPtr;
			sseiWord1 = *(sseiPtr+1);

			while( haszeroByte(sseiWord0,sseiWord1,sseiZero) ==0){ 
				retb = hasByteC(sseiWord0,sseiWord1,  byte16b);
				if(retb !=0){
					// findout b
					if((*((char*) sseiPtr)) == charb){
						//b000
						char * bytePtr = ((char*) ( sseiPtr )) -1;
						if(bytePtr[0] == chara){
							int i=1;
							while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
							if(pattern[i] == 0) REPORT(bytePtr);
							if(bytePtr[i] == 0) return NULL;
						}

					}
					reta = hasByteC(sseiWord0,sseiWord1,  byte16a);
					if(reta !=0) 
						goto findoutb;
					else{
						goto nextWord;					
					}
				}
				sseiPtr += 2;
				sseiWord0 = *sseiPtr;
				sseiWord1 = *(sseiPtr+1);
			}
			// search  from (char*)sseiPtr
			char * bytePtr = ((char*) ( sseiPtr )) -1;
			if(bytePtr[0] == chara){
				int i=1;
				while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
				if(pattern[i] == 0) REPORT(bytePtr);
			}

			goto prePareForEnd;
		}
nextWord:
		sseiPtr += 2;
		sseiWord0 = *sseiPtr;
		sseiWord1 = *(sseiPtr+1);
	}
prePareForEnd:
	{
		unsigned int reta;
		unsigned int retb;
		reta =hasByteC(sseiWord0,sseiWord1,  byte16a);
		retb =hasByteC(sseiWord0,sseiWord1,  byte16b);
		if(((reta<<1) & retb)){
			bytePtr = (char*)sseiPtr;
			while(*bytePtr){
				if(*bytePtr == chara) {
					int i=1;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) REPORT(bytePtr);
					if(bytePtr[i] == 0) return NULL;

				}
				bytePtr++;
			}
		}
	}
	return NULL;
}

#else
#include <stdio.h>
#include <string.h>
char *strstrsse(const char *a, const char *b)
{
return strstr(a, b);
}
#endif
