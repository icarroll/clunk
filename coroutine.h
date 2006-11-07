/* originally by Simon Tatham */

#define ccrContParam     void **ccrParam

#define ccrBeginContext  struct ccrContextTag { int ccrLine
#define ccrEndContext(x) } *x = *ccrParam

#define ccrBegin(x)      if(!x) {x= *ccrParam=malloc(sizeof(*x)); x->ccrLine=0;}\
                         if (x) switch(x->ccrLine) { case 0:;
#define ccrFinish(z)     } free(*ccrParam); *ccrParam=0; return (z)
#define ccrFinishV       } free(*ccrParam); *ccrParam=0; return

#define ccrReturn(z)     \
        do {\
            ((struct ccrContextTag *)*ccrParam)->ccrLine=__LINE__;\
            return (z); case __LINE__:;\
        } while (0)
#define ccrReturnV       \
        do {\
            ((struct ccrContextTag *)*ccrParam)->ccrLine=__LINE__;\
            return; case __LINE__:;\
        } while (0)

#define ccrStop(z)       do{ free(*ccrParam); *ccrParam=0; return (z); }while(0)
#define ccrStopV         do{ free(*ccrParam); *ccrParam=0; return; }while(0)

#define ccrContext       void *
#define ccrAbort(ctx)    do { free (ctx); ctx = 0; } while (0)
