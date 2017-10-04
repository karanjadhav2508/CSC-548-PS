#include <sys/time.h>

#ifndef MACRO_BEGIN

#define MACRO_BEGIN     do {

#ifndef lint
#define MACRO_END       } while (0)
#else /* lint */
extern int _NEVER_;
#define MACRO_END       } while (_NEVER_)
#endif /* lint */

#endif /* !MACRO_BEGIN */

/*
 * MINUS_NTIME only works if src1 > src2
 */
#define MINUS_UTIME(dst, src1, src2) \
  MACRO_BEGIN \
    if ((src2).tv_usec > (src1).tv_usec) { \
      (dst).tv_sec = (src1).tv_sec - (src2).tv_sec - 1; \
      (dst).tv_usec = ((src1).tv_usec - (src2).tv_usec) + 1000000000; \
    } \
    else { \
      (dst).tv_sec = (src1).tv_sec - (src2).tv_sec; \
      (dst).tv_usec = (src1).tv_usec - (src2).tv_usec; \
    } \
  MACRO_END
