#ifndef MPLATFORM_H
#define MPLATFORM_H 1

#include <stdlib.h>
#ifndef  __APPLE__
#include <malloc.h>
#endif


/* Some platform specifics
 */
#if defined(_WIN32) || defined(_WIN64)
#  define wcpncpy wcsncpy
#  define wcpcpy  wcscpy
#endif


/* Sized integer types
 */
#ifdef _WIN32
#include <wtypes.h>
typedef unsigned __int8 Uint8;
typedef unsigned __int16 Uint16;
typedef unsigned __int32 Uint32;
typedef unsigned __int64 Uint64;

typedef unsigned __int8 int8_t;
typedef unsigned __int16 int16_t;
typedef unsigned __int32 int32_t;
typedef unsigned __int64 int64_t;
#else
#include <stdint.h>
typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;
#endif


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


/* Endianess swapping macros
 */
#define DM_SWAP_16_LE_BE(value)    ((Uint16) (   \
    (Uint16) ((Uint16) (value) >> 8) |      \
    (Uint16) ((Uint16) (value) << 8)) )


#define DM_SWAP_32_LE_BE(value) ((Uint32) (               \
    (((Uint32) (value) & (Uint32) 0x000000ffU) << 24) | \
    (((Uint32) (value) & (Uint32) 0x0000ff00U) <<  8) | \
    (((Uint32) (value) & (Uint32) 0x00ff0000U) >>  8) | \
    (((Uint32) (value) & (Uint32) 0xff000000U) >> 24)))


/* Macros that swap only when needed ...
 */
#if (BYTEORDER == BIG_ENDIAN)
#  define DM_LE16_TO_NATIVE(value) DM_SWAP_16_LE_BE(value)
#  define DM_LE32_TO_NATIVE(value) DM_SWAP_32_LE_BE(value)
#  define DM_NATIVE_TO_LE16(value) DM_SWAP_16_LE_BE(value)
#  define DM_NATIVE_TO_LE32(value) DM_SWAP_32_LE_BE(value)

#  define DM_BE16_TO_NATIVE(value) ((Uint16) (value))
#  define DM_BE32_TO_NATIVE(value) ((Uint32) (value))
#  define DM_NATIVE_TO_BE16(value) ((Uint16) (value))
#  define DM_NATIVE_TO_BE32(value) ((Uint32) (value))

#elif (BYTEORDER == LIL_ENDIAN)

#  define DM_LE16_TO_NATIVE(value) ((Uint16) (value))
#  define DM_LE32_TO_NATIVE(value) ((Uint32) (value))
#  define DM_NATIVE_TO_LE16(value) ((Uint16) (value))
#  define DM_NATIVE_TO_LE32(value) ((Uint32) (value))

#  define DM_BE16_TO_NATIVE(value) DM_SWAP_16_LE_BE(value)
#  define DM_BE32_TO_NATIVE(value) DM_SWAP_32_LE_BE(value)
#  define DM_NATIVE_TO_BE16(value) DM_SWAP_16_LE_BE(value)
#  define DM_NATIVE_TO_BE32(value) DM_SWAP_32_LE_BE(value)

#endif


#ifdef __cplusplus
extern "C" {
#endif

#if defined(__APPLE__) || defined(_WIN32) || defined(_WIN64) 
char * strndup(const char *s, size_t n);
#    pragma warning (disable: 4244)
#endif

#if defined(_WIN32) || defined(_WIN64) 
double round(float v);
#endif

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // MPLATFORM_H
