/*
 * =====================================================================================
 *Copyright(C) by Zhenghua Dai. All rights reserved.
 *
 *
 *
 * ==================================
 * ==================================
 *
 *       Filename:  report.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2009年11月06日 21时40分05秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhenghua Dai (Zhenghua Dai), djx.zhenghua@gmail.com
 *        Company:  dzh
 *
 * =====================================================================================
 */

#ifndef  REPORT_INC
#define  REPORT_INC
/*
 *text: the pointer of text 
 *idx : the offset of position that is matched
 *pat : the pattern
 */
#ifdef __cplusplus 
extern "C"{
#endif
typedef int (*reportFunc)(const char* text, int idx, const char* pat);

void setReportFunc(reportFunc rf);
#define SEARCH_STOP 1
#define SEARCH_CONTINUE 0
#define SEARCH_FIRST ((reportFunc) 1)
#define SEARCH_ALL ((reportFunc) 2)
#define SEARCH_SILENT ((reportFunc) 3)

extern reportFunc report_function ;
#ifdef __cplusplus 
}
#endif
#endif   /* ----- #ifndef REPORT_INC  ----- */
