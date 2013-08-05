#ifndef MTYPES_H
#define MTYPES_H 1


typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;


/* Define a boolean type
 */
#if !defined(FALSE) && !defined(TRUE) && !defined(BOOL)
typedef enum { FALSE = 0, TRUE = 1 } BOOL;
#endif

#ifndef BOOL
#  ifdef bool
#    define BOOL bool
#  else
#    define BOOL int
#  endif
#endif


#endif
