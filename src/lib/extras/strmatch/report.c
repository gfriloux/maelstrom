/*
 * =====================================================================================
 *Copyright(C) by Zhenghua Dai. All rights reserved.
 *
 *
 *
 * ==================================
 * ==================================
 *
 *       Filename:  report.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2009年11月06日 21时36分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhenghua Dai (Zhenghua Dai), djx.zhenghua@gmail.com
 *        Company:  dzh
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include "report.h"

static int report_all(const char* text, int idx, const char* pat)
{
    (void) text;
    (void) pat;
	printf("<%d>,", idx);
	return SEARCH_CONTINUE;
}

static int report_default(const char* text, int idx, const char* pat)
{
    (void) text;
    (void) pat;
	printf("<%d> ", idx);
	return SEARCH_STOP;
}

static int report_silent(const char* text, int idx, const char* pat)
{
    (void) text;
    (void) pat;
    (void) idx;
	return SEARCH_CONTINUE;
}


reportFunc report_function = report_default;

void setReportFunc(reportFunc rf)
{ 
	if(rf == SEARCH_ALL){
		report_function = report_all;
	}else if(rf==SEARCH_FIRST){
		report_function = report_default;
	}else if(rf==SEARCH_SILENT){
		report_function = report_silent;
	} else {
		report_function = rf;
	}
}


