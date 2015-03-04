#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "memrchr.h"

#ifndef HAVE_MEMRCHR
void *memrchr(const void *s, int c, size_t n)
{
   const unsigned char *cp;

   if (!n)
     return NULL;

   cp = (unsigned char *)s + n;
   do {
      if (*(--cp) == (unsigned char)c)
        return (void *)cp;
   } while (--n != 0);

   return NULL;
}
#endif
