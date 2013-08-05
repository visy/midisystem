#ifndef MENDIAN_H
#define MENDIAN_H 1

#include "mtypes.h"

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


#endif
