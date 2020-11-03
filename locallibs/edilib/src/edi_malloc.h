#ifndef _EDI_MALLOC_H_
#define _EDI_MALLOC_H_

#define edi_calloc(a,b) calloc((a),(b))
#define edi_malloc(a) malloc((a))
#define edi_free(a) free((a))
#define edi_realloc(a,b) realloc((a),(b))

#endif /*_EDI_MALLOC_H_*/
