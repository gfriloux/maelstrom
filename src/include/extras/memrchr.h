#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef HAVE_MEMRCHR
# ifndef __WIN32__
#  include "string.h"

void *memrchr(const void *s, int c, size_t n);
# endif
#endif
