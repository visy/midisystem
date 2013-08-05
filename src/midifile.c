/*
 * midiFile.c - A general purpose midi file handling library. This code
 *				can read and write MIDI files in formats 0 and 1.
 * Version 1.4
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *          Copyright 1998-2010, Steven Goodwin
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License,or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifndef  __APPLE__
#include <malloc.h>
#endif
#include "midifile.h"
#include "mendian.h"


/*
** Internal Data Structures
*/
typedef struct
{
    Uint8 note, chn;
    Uint8 valid, p2;
    Uint32 end_pos;
} MIDI_LAST_NOTE;

typedef struct
{
    Uint8 *ptr;
    Uint8 *pBase;
    Uint8 *pEnd;

    Uint32 pos;
    Uint32 dt;
    /* For Reading MIDI Files */
    Uint32 sz;                   /* size of whole iTrack */
    /* For Writing MIDI Files */
    Uint32 iBlockSize;           /* max size of track */
    Uint8 iDefaultChannel;       /* use for write only */
    Uint8 last_status;           /* used for running status */

    MIDI_LAST_NOTE LastNote[MAX_TRACK_POLYPHONY];
} MIDI_FILE_TRACK;

typedef struct
{
    Uint32 iHeaderSize;
      /**/ Uint16 iVersion;       /* 0, 1 or 2 */
    Uint16 iNumTracks;            /* number of tracks... (will be 1 for MIDI type 0) */
    Uint16 PPQN;                  /* pulses per quarter note */
} MIDI_HEADER;

typedef struct
{
    FILE *pFile;
    BOOL bOpenForWriting;

    MIDI_HEADER Header;

    Uint8 *ptr, *curr, *end;
    size_t file_size;

    MIDI_FILE_TRACK Track[MAX_MIDI_TRACKS];
} _MIDI_FILE;


/*
** Internal Functions
*/
#define DT_DEF				32      /* assume maximum delta-time + msg is no more than 32 bytes */
#define _VAR_CAST			_MIDI_FILE *pMF = (_MIDI_FILE *)_pMF
#define IsFilePtrValid(pMF)		(pMF)
#define IsTrackValid(_x)		(_midiValidateTrack(pMF, _x))
#define IsChannelValid(_x)		((_x)>=1 && (_x)<=16)
#define IsNoteValid(_x)			((_x)>=0 && (_x)<128)
#define IsMessageValid(_x)		((_x)>=msgNoteOff && (_x)<=msgMetaEvent)


static void midiErrorV(_MIDI_FILE *pMF, const char *fmt, va_list ap)
{
    printf("MIDI: ");
    vprintf(fmt, ap);
}


static void midiError(_MIDI_FILE *pMF, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    midiErrorV(pMF, fmt, ap);
    va_end(ap);
}


static int midiStrNCmp(_MIDI_FILE *pMF, const char *str, const size_t n)
{
    if (pMF->curr + n < pMF->end)
    {
        int res = strncmp((char *) pMF->curr, str, n);
        pMF->curr += n;
        return res;
    }
    else
        return 1;
}


static BOOL midiSkip(_MIDI_FILE *pMF, const size_t n)
{
    pMF->curr += n;
    return (pMF->curr < pMF->end);
}


static BOOL midiGetBE32(_MIDI_FILE *pMF, Uint32 *val)
{
    if (pMF->curr + sizeof(Uint32) < pMF->end)
    {
        Uint32 tmp;
        memcpy(&tmp, pMF->curr, sizeof(Uint32));
        *val = DM_BE32_TO_NATIVE(tmp);

        pMF->curr += sizeof(Uint32);
        return TRUE;
    }
    else
        return FALSE;
}


static BOOL midiGetBE16(_MIDI_FILE *pMF, Uint16 *val)
{
    if (pMF->curr + sizeof(Uint16) < pMF->end)
    {
        Uint16 tmp;
        memcpy(&tmp, pMF->curr, sizeof(Uint16));
        *val = DM_BE16_TO_NATIVE(tmp);

        pMF->curr += sizeof(Uint16);
        return TRUE;
    }
    else
        return FALSE;
}


static BOOL midiGetByte(_MIDI_FILE *pMF, Uint8 *val)
{
    if (pMF->curr + sizeof(Uint8) < pMF->end)
    {
        memcpy(val, pMF->curr, sizeof(Uint8));
        pMF->curr += sizeof(Uint8);
        return TRUE;
    }
    else
        return FALSE;
}


static void midiFree(void *ptr)
{
    if (ptr != NULL)
        free(ptr);
}


static BOOL midiWriteBE32(FILE *fp, const Uint32 val)
{
    Uint32 result = DM_NATIVE_TO_BE32(val);
    return (fwrite(&result, sizeof(result), 1, fp) == 1);
}


static BOOL midiWriteBE16(FILE *fp, const Uint16 val)
{
    Uint16 result = DM_NATIVE_TO_BE16(val);
    return (fwrite(&result, sizeof(result), 1, fp) == 1);
}


static BOOL midiWriteByte(FILE *fp, const Uint8 val)
{
    return fputc(val, fp) == val;
}


static BOOL midiWriteData(FILE *fp, const Uint8 *data, const size_t n)
{
    return fwrite(data, sizeof(Uint8), n, fp) == n;
}


static BOOL midiWriteStr(FILE *fp, const char *str)
{
    size_t len = strlen(str);
    return fwrite(str, sizeof(char), strlen(str), fp) == len;
}


static BOOL _midiValidateTrack(const _MIDI_FILE *pMF, int iTrack)
{
    if (!IsFilePtrValid(pMF))
        return FALSE;

    if (pMF->bOpenForWriting)
    {
        if (iTrack < 0 || iTrack >= MAX_MIDI_TRACKS)
            return FALSE;
    }
    else                        /* open for reading */
    {
        if (!pMF->ptr)
            return FALSE;

        if (iTrack < 0 || iTrack >= pMF->Header.iNumTracks)
            return FALSE;
    }

    return TRUE;
}

static Uint8 *_midiWriteVarLen(Uint8 * ptr, int n)
{
    register long buffer;
    register long value = n;

    buffer = value & 0x7f;
    while ((value >>= 7) > 0)
    {
        buffer <<= 8;
        buffer |= 0x80;
        buffer += (value & 0x7f);
    }

    while (TRUE)
    {
        *ptr++ = (Uint8) buffer;
        if (buffer & 0x80)
            buffer >>= 8;
        else
            break;
    }

    return (ptr);
}

/* Return a ptr to valid block of memory to store a message
** of up to sz_reqd bytes
*/
static Uint8 *_midiGetPtr(_MIDI_FILE *pMF, int iTrack, int sz_reqd)
{
    const Uint32 mem_sz_inc = 8092;      /* arbitary */
    Uint8 *ptr;
    int curr_offset;
    MIDI_FILE_TRACK *pTrack = &pMF->Track[iTrack];

    ptr = pTrack->ptr;
    if (ptr == NULL || ptr + sz_reqd > pTrack->pEnd)    /* need more RAM! */
    {
        curr_offset = ptr - pTrack->pBase;
        if ((ptr =
             (Uint8 *) realloc(pTrack->pBase,
                              mem_sz_inc + pTrack->iBlockSize)))
        {
            pTrack->pBase = ptr;
            pTrack->iBlockSize += mem_sz_inc;
            pTrack->pEnd = ptr + pTrack->iBlockSize;
            /* Move new ptr to continue data entry: */
            pTrack->ptr = ptr + curr_offset;
            ptr += curr_offset;
        }
        else
        {
            /* NO MEMORY LEFT */
            return NULL;
        }
    }

    return ptr;
}


static int _midiGetLength(int ppqn, int iNoteLen, BOOL bOverride)
{
    int length = ppqn;

    if (bOverride)
    {
        length = iNoteLen;
    }
    else
    {
        switch (iNoteLen)
        {
        case MIDI_NOTE_DOTTED_MINIM:
            length *= 3;
            break;

        case MIDI_NOTE_DOTTED_CROCHET:
            length *= 3;
            length /= 2;
            break;

        case MIDI_NOTE_DOTTED_QUAVER:
            length *= 3;
            length /= 4;
            break;

        case MIDI_NOTE_DOTTED_SEMIQUAVER:
            length *= 3;
            length /= 8;
            break;

        case MIDI_NOTE_DOTTED_SEMIDEMIQUAVER:
            length *= 3;
            length /= 16;
            break;

        case MIDI_NOTE_BREVE:
            length *= 4;
            break;

        case MIDI_NOTE_MINIM:
            length *= 2;
            break;

        case MIDI_NOTE_QUAVER:
            length /= 2;
            break;

        case MIDI_NOTE_SEMIQUAVER:
            length /= 4;
            break;

        case MIDI_NOTE_SEMIDEMIQUAVER:
            length /= 8;
            break;

        case MIDI_NOTE_TRIPLE_CROCHET:
            length *= 2;
            length /= 3;
            break;
        }
    }

    return length;
}

/*
** midiFile* Functions
*/
MIDI_FILE *midiFileCreate(const char *pFilename, BOOL bOverwriteIfExists)
{
    _MIDI_FILE *pMF = (_MIDI_FILE *) malloc(sizeof(_MIDI_FILE));
    int i;

    if (pMF == NULL)
        return NULL;

    if (!bOverwriteIfExists && (pMF->pFile = fopen(pFilename, "r")) != NULL)
    {
        fclose(pMF->pFile);
        midiFree(pMF);
        return NULL;
    }

    if ((pMF->pFile = fopen(pFilename, "wb+")) == NULL)
    {
        midiFree(pMF);
        return NULL;
    }

    pMF->bOpenForWriting = TRUE;
    pMF->Header.PPQN = MIDI_PPQN_DEFAULT;
    pMF->Header.iVersion = MIDI_VERSION_DEFAULT;

    for (i = 0; i < MAX_MIDI_TRACKS; ++i)
    {
        pMF->Track[i].pos = 0;
        pMF->Track[i].ptr = NULL;
        pMF->Track[i].pBase = NULL;
        pMF->Track[i].pEnd = NULL;
        pMF->Track[i].iBlockSize = 0;
        pMF->Track[i].dt = 0;
        pMF->Track[i].iDefaultChannel = (Uint8) (i & 0xf);

        memset(pMF->Track[i].LastNote, 0, sizeof(pMF->Track[i].LastNote));
    }

    return (MIDI_FILE *) pMF;
}

int midiFileSetTracksDefaultChannel(MIDI_FILE *_pMF, int iTrack, int iChannel)
{
    int prev;

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return 0;
    if (!IsTrackValid(iTrack))
        return 0;
    if (!IsChannelValid(iChannel))
        return 0;

    /* For programmer each, iChannel is between 1 & 16 - but MIDI uses
     ** 0-15. Thus, the fudge factor of 1 :)
     */
    prev = pMF->Track[iTrack].iDefaultChannel + 1;
    pMF->Track[iTrack].iDefaultChannel = (Uint8) (iChannel - 1);
    return prev;
}

int midiFileGetTracksDefaultChannel(const MIDI_FILE *_pMF, int iTrack)
{
    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return 0;
    if (!IsTrackValid(iTrack))
        return 0;

    return pMF->Track[iTrack].iDefaultChannel + 1;
}

int midiFileSetPPQN(MIDI_FILE *_pMF, int PPQN)
{
    int prev;

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return MIDI_PPQN_DEFAULT;
    prev = pMF->Header.PPQN;
    pMF->Header.PPQN = (Uint16) PPQN;
    return prev;
}

int midiFileGetPPQN(const MIDI_FILE *_pMF)
{
    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return MIDI_PPQN_DEFAULT;
    return (int) pMF->Header.PPQN;
}

int midiFileSetVersion(MIDI_FILE *_pMF, int iVersion)
{
    int prev;

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return MIDI_VERSION_DEFAULT;
    if (iVersion < 0 || iVersion > 2)
        return MIDI_VERSION_DEFAULT;
    prev = pMF->Header.iVersion;
    pMF->Header.iVersion = (Uint16) iVersion;
    return prev;
}

int midiFileGetVersion(const MIDI_FILE *_pMF)
{
    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return MIDI_VERSION_DEFAULT;
    return pMF->Header.iVersion;
}

MIDI_FILE *midiFileOpen(const char *pFilename)
{
    _MIDI_FILE *pMF = NULL;
    BOOL bValidFile = FALSE;
    FILE *fp;
    int i;

    if ((fp = fopen(pFilename, "rb")) == NULL)
        goto error;

    if ((pMF = (_MIDI_FILE *) malloc(sizeof(_MIDI_FILE))) == NULL)
        goto error;

    fseek(fp, 0L, SEEK_END);
    pMF->file_size = ftell(fp);

    if ((pMF->curr = pMF->ptr = (Uint8 *) malloc(pMF->file_size)) == NULL)
    {
        midiError(pMF, "Could not allocate %d bytes for MIDI file.\n", pMF->file_size);
        goto error;
    }

    fseek(fp, 0L, SEEK_SET);
    if (fread(pMF->ptr, sizeof(Uint8), pMF->file_size, fp) != pMF->file_size)
    {
        midiError(pMF, "Error reading file data.\n");
        goto error;
    }

    /* Is this a valid MIDI file ? */
    if (midiStrNCmp(pMF, "MThd", 4) != 0)
    {
        midiError(pMF, "Not a MIDI file.\n");
        goto error;
    }

    midiSkip(pMF, 4);

    if (!midiGetBE32(pMF, &pMF->Header.iHeaderSize) ||
        !midiGetBE16(pMF, &pMF->Header.iVersion) ||
        !midiGetBE16(pMF, &pMF->Header.iNumTracks) ||
        !midiGetBE16(pMF, &pMF->Header.PPQN))
    {
        midiError(pMF, "Error reading MIDI file header.\n");
        goto error;
    }

    midiSkip(pMF, pMF->Header.iHeaderSize + 8);

    /*
     **      Get all tracks
     */
    for (i = 0; i < MAX_MIDI_TRACKS; ++i)
    {
        pMF->Track[i].pos = 0;
        pMF->Track[i].last_status = 0;
    }

    for (i = 0; i < pMF->Header.iNumTracks; ++i)
    {
        if (midiStrNCmp(pMF, "MTrk", 4) != 0)
        {
            midiError(pMF, "Expected track %d data, not found.\n", i);
            goto error;
        }
        
        pMF->Track[i].pBase = pMF->curr;
        pMF->Track[i].ptr = pMF->curr + 8;
        
        midiSkip(pMF, 4);
        if (!midiGetBE32(pMF, &pMF->Track[i].sz))
        {
            midiError(pMF, "Could not read MTrk size.\n");
            goto error;
        }

        midiSkip(pMF, 4);

        pMF->Track[i].pEnd = pMF->curr + pMF->Track[i].sz;

        midiSkip(pMF, pMF->Track[i].sz);
    }

    pMF->bOpenForWriting = FALSE;
    pMF->pFile = NULL;
    bValidFile = TRUE;


error:
    // Cleanup
    if (fp != NULL)
        fclose(fp);

    if (!bValidFile)
    {
        midiFree(pMF);
        return NULL;
    }

    return (MIDI_FILE *) pMF;
}

typedef struct
{
    int iIdx;
    int iEndPos;
} MIDI_END_POINT;

static int qs_cmp_pEndPoints(const void *e1, const void *e2)
{
    MIDI_END_POINT *p1 = (MIDI_END_POINT *) e1;
    MIDI_END_POINT *p2 = (MIDI_END_POINT *) e2;

    return p1->iEndPos - p2->iEndPos;
}

BOOL midiFileFlushTrack(MIDI_FILE *_pMF, int iTrack, BOOL bFlushToEnd,
                        Uint32 dwEndTimePos)
{
    size_t sz, i;
    Uint8 *ptr;
    MIDI_END_POINT *pEndPoints;
    int num, mx_pts;
    BOOL bNoChanges = TRUE;

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!_midiValidateTrack(pMF, iTrack))
        return FALSE;
    sz = sizeof(pMF->Track[0].LastNote) / sizeof(pMF->Track[0].LastNote[0]);

    /*
     ** Flush all
     */
    pEndPoints = (MIDI_END_POINT *) malloc(sz * sizeof(MIDI_END_POINT));
    mx_pts = 0;
    for (i = 0; i < sz; ++i)
        if (pMF->Track[iTrack].LastNote[i].valid)
        {
            pEndPoints[mx_pts].iIdx = i;
            pEndPoints[mx_pts].iEndPos =
                pMF->Track[iTrack].LastNote[i].end_pos;
            mx_pts++;
        }

    if (bFlushToEnd)
    {
        if (mx_pts)
            dwEndTimePos = pEndPoints[mx_pts - 1].iEndPos;
        else
            dwEndTimePos = pMF->Track[iTrack].pos;
    }

    if (mx_pts)
    {
        /* Sort, smallest first, and add the note off msgs */
        qsort(pEndPoints, mx_pts, sizeof(MIDI_END_POINT), qs_cmp_pEndPoints);

        i = 0;
        while ((dwEndTimePos >= (Uint32) pEndPoints[i].iEndPos || bFlushToEnd)
               && i < mx_pts)
        {
            ptr = _midiGetPtr(pMF, iTrack, DT_DEF);
            if (!ptr)
                return FALSE;

            num = pEndPoints[i].iIdx;   /* get 'LastNote' index */

            ptr =
                _midiWriteVarLen(ptr,
                                 pMF->Track[iTrack].LastNote[num].end_pos -
                                 pMF->Track[iTrack].pos);
            /* msgNoteOn  msgNoteOff */
            *ptr++ =
                (Uint8) (msgNoteOff | pMF->Track[iTrack].LastNote[num].chn);
            *ptr++ = pMF->Track[iTrack].LastNote[num].note;
            *ptr++ = 0;

            pMF->Track[iTrack].LastNote[num].valid = FALSE;
            pMF->Track[iTrack].pos = pMF->Track[iTrack].LastNote[num].end_pos;

            pMF->Track[iTrack].ptr = ptr;

            ++i;
            bNoChanges = FALSE;
        }
    }

    midiFree(pEndPoints);

    /*
     ** Re-calc current position
     */
    pMF->Track[iTrack].dt = dwEndTimePos - pMF->Track[iTrack].pos;

    return TRUE;
}

BOOL midiFileSyncTracks(MIDI_FILE *_pMF, int iTrack1, int iTrack2)
{
    int p1, p2;

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack1))
        return FALSE;
    if (!IsTrackValid(iTrack2))
        return FALSE;

    p1 = pMF->Track[iTrack1].pos + pMF->Track[iTrack1].dt;
    p2 = pMF->Track[iTrack2].pos + pMF->Track[iTrack2].dt;

    if (p1 < p2)
        midiTrackIncTime(pMF, iTrack1, p2 - p1, TRUE);
    else if (p2 < p1)
        midiTrackIncTime(pMF, iTrack2, p1 - p2, TRUE);

    return TRUE;
}


BOOL midiFileClose(MIDI_FILE *_pMF)
{
    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;

    if (pMF->bOpenForWriting)
    {
        Uint16 iNumTracks = 0;
        int i;

        /* Flush our buffers  */
        for (i = 0; i < MAX_MIDI_TRACKS; ++i)
        {
            if (pMF->Track[i].ptr)
            {
                midiSongAddEndSequence(pMF, i);
                midiFileFlushTrack(pMF, i, TRUE, 0);
                iNumTracks++;
            }
        }
        /*
         ** Header
         */
        if (!midiWriteStr(pMF->pFile, "MThd") ||
            !midiWriteBE32(pMF->pFile, 6) ||
            !midiWriteBE16(pMF->pFile, iNumTracks == 1 ? pMF->Header.iVersion : 1) ||
            !midiWriteBE16(pMF->pFile, iNumTracks) ||
            !midiWriteBE16(pMF->pFile, pMF->Header.PPQN))
        {
            midiError(pMF, "Could not write MThd header.\n");
            goto error;
        }

        /*
         ** Track data
         */
        for (i = 0; i < MAX_MIDI_TRACKS; ++i)
        {
            if (pMF->Track[i].ptr)
            {
                size_t sz = pMF->Track[i].ptr - pMF->Track[i].pBase;
                if (!midiWriteStr(pMF->pFile, "MTrk") ||
                    !midiWriteBE32(pMF->pFile, sz) ||
                    !midiWriteData(pMF->pFile, pMF->Track[i].pBase, sz))
                {
                    midiError(pMF, "Could not write track #%d data.\n", i);
                    goto error;
                }

                midiFree(pMF->Track[i].pBase);
            }
        }
    }

error:
    if (pMF->pFile != NULL)
        return fclose(pMF->pFile) ? FALSE : TRUE;

    midiFree(pMF);
    return TRUE;
}


/*
** midiSong* Functions
*/
BOOL midiSongAddSMPTEOffset(MIDI_FILE *_pMF, int iTrack, int iHours,
                            int iMins, int iSecs, int iFrames, int iFFrames)
{
    static Uint8 tmp[] =
        { msgMetaEvent, metaSMPTEOffset, 0x05, 0, 0, 0, 0, 0 };

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;

    if (iMins < 0 || iMins > 59)
        iMins = 0;
    if (iSecs < 0 || iSecs > 59)
        iSecs = 0;
    if (iFrames < 0 || iFrames > 24)
        iFrames = 0;

    tmp[3] = (Uint8) iHours;
    tmp[4] = (Uint8) iMins;
    tmp[5] = (Uint8) iSecs;
    tmp[6] = (Uint8) iFrames;
    tmp[7] = (Uint8) iFFrames;
    return midiTrackAddRaw(pMF, iTrack, sizeof(tmp), tmp, FALSE, 0);
}


BOOL midiSongAddSimpleTimeSig(MIDI_FILE *_pMF, int iTrack, int iNom,
                              int iDenom)
{
    return midiSongAddTimeSig(_pMF, iTrack, iNom, iDenom, 24, 8);
}

BOOL midiSongAddTimeSig(MIDI_FILE *_pMF, int iTrack, int iNom, int iDenom,
                        int iClockInMetroTick, int iNotated32nds)
{
    static Uint8 tmp[] = { msgMetaEvent, metaTimeSig, 0x04, 0, 0, 0, 0 };

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;

    tmp[3] = (Uint8) iNom;
    tmp[4] = (Uint8) (MIDI_NOTE_MINIM / iDenom);
    tmp[5] = (Uint8) iClockInMetroTick;
    tmp[6] = (Uint8) iNotated32nds;
    return midiTrackAddRaw(pMF, iTrack, sizeof(tmp), tmp, FALSE, 0);
}

BOOL midiSongAddKeySig(MIDI_FILE *_pMF, int iTrack, tMIDI_KEYSIG iKey)
{
    static Uint8 tmp[] = { msgMetaEvent, metaKeySig, 0x02, 0, 0 };

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;

    tmp[3] = (Uint8) ((iKey & keyMaskKey) * ((iKey & keyMaskNeg) ? -1 : 1));
    tmp[4] = (Uint8) ((iKey & keyMaskMin) ? 1 : 0);
    return midiTrackAddRaw(pMF, iTrack, sizeof(tmp), tmp, FALSE, 0);
}

BOOL midiSongAddTempo(MIDI_FILE *_pMF, int iTrack, int iTempo)
{
    static Uint8 tmp[] = { msgMetaEvent, metaSetTempo, 0x03, 0, 0, 0 };
    int us;                     /* micro-seconds per qn */

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;

    us = 60000000L / iTempo;
    tmp[3] = (Uint8) ((us >> 16) & 0xff);
    tmp[4] = (Uint8) ((us >> 8) & 0xff);
    tmp[5] = (Uint8) ((us >> 0) & 0xff);
    return midiTrackAddRaw(pMF, iTrack, sizeof(tmp), tmp, FALSE, 0);
}

BOOL midiSongAddMIDIPort(MIDI_FILE *_pMF, int iTrack, int iPort)
{
    static Uint8 tmp[] = { msgMetaEvent, metaMIDIPort, 1, 0 };

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;
    tmp[3] = (Uint8) iPort;
    return midiTrackAddRaw(pMF, iTrack, sizeof(tmp), tmp, FALSE, 0);
}

BOOL midiSongAddEndSequence(MIDI_FILE *_pMF, int iTrack)
{
    static Uint8 tmp[] = { msgMetaEvent, metaEndSequence, 0 };

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;

    return midiTrackAddRaw(pMF, iTrack, sizeof(tmp), tmp, FALSE, 0);
}


/*
** midiTrack* Functions
*/
BOOL midiTrackAddRaw(MIDI_FILE *_pMF, int iTrack, int data_sz,
                     const Uint8 * pData, BOOL bMovePtr, int dt)
{
    MIDI_FILE_TRACK *pTrk;
    Uint8 *ptr;
    int dtime;

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;

    pTrk = &pMF->Track[iTrack];
    ptr = _midiGetPtr(pMF, iTrack, data_sz + DT_DEF);
    if (!ptr)
        return FALSE;

    dtime = pTrk->dt;
    if (bMovePtr)
        dtime += dt;

    ptr = _midiWriteVarLen(ptr, dtime);
    memcpy(ptr, pData, data_sz);

    pTrk->pos += dtime;
    pTrk->dt = 0;
    pTrk->ptr = ptr + data_sz;

    return TRUE;
}


BOOL midiTrackIncTime(MIDI_FILE *_pMF, int iTrack, int iDeltaTime,
                      BOOL bOverridePPQN)
{
    Uint32 will_end_at;

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;

    will_end_at = _midiGetLength(pMF->Header.PPQN, iDeltaTime, bOverridePPQN);
    will_end_at += pMF->Track[iTrack].pos + pMF->Track[iTrack].dt;

    midiFileFlushTrack(pMF, iTrack, FALSE, will_end_at);

    return TRUE;
}

BOOL midiTrackAddText(MIDI_FILE *_pMF, int iTrack, tMIDI_TEXT iType,
                      const char *pTxt)
{
    Uint8 *ptr;
    int sz;

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;

    sz = strlen(pTxt);
    if ((ptr = _midiGetPtr(pMF, iTrack, sz + DT_DEF)))
    {
        *ptr++ = 0;             /* delta-time=0 */
        *ptr++ = msgMetaEvent;
        *ptr++ = (Uint8) iType;
        ptr = _midiWriteVarLen((Uint8 *) ptr, sz);
        strcpy((char *) ptr, pTxt);
        pMF->Track[iTrack].ptr = ptr + sz;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL midiTrackSetKeyPressure(MIDI_FILE *pMF, int iTrack, int iNote,
                             int iAftertouch)
{
    return midiTrackAddMsg(pMF, iTrack, msgNoteKeyPressure, iNote,
                           iAftertouch);
}

BOOL midiTrackAddControlChange(MIDI_FILE *pMF, int iTrack, tMIDI_CC iCCType,
                               int iParam)
{
    return midiTrackAddMsg(pMF, iTrack, msgControlChange, iCCType, iParam);
}

BOOL midiTrackAddProgramChange(MIDI_FILE *pMF, int iTrack, int iInstrPatch)
{
    return midiTrackAddMsg(pMF, iTrack, msgSetProgram, iInstrPatch, 0);
}

BOOL midiTrackChangeKeyPressure(MIDI_FILE *pMF, int iTrack,
                                int iDeltaPressure)
{
    return midiTrackAddMsg(pMF, iTrack, msgChangePressure,
                           iDeltaPressure & 0x7f, 0);
}

BOOL midiTrackSetPitchWheel(MIDI_FILE *pMF, int iTrack, int iWheelPos)
{
    Uint16 wheel = (Uint16) iWheelPos;

    /* bitshift 7 instead of eight because we're dealing with 7 bit numbers */
    wheel += MIDI_WHEEL_CENTRE;
    return midiTrackAddMsg(pMF, iTrack, msgSetPitchWheel, wheel & 0x7f,
                           (wheel >> 7) & 0x7f);
}

BOOL midiTrackAddMsg(MIDI_FILE *_pMF, int iTrack, tMIDI_MSG iMsg,
                     int iParam1, int iParam2)
{
    Uint8 *ptr;
    Uint8 data[3];
    int sz;

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;
    if (!IsMessageValid(iMsg))
        return FALSE;

    ptr = _midiGetPtr(pMF, iTrack, DT_DEF);
    if (!ptr)
        return FALSE;

    data[0] = (Uint8) (iMsg | pMF->Track[iTrack].iDefaultChannel);
    data[1] = (Uint8) (iParam1 & 0x7f);
    data[2] = (Uint8) (iParam2 & 0x7f);
    /*
     ** Is this msg a single, or double Uint8, prm?
     */
    switch (iMsg)
    {
    case msgSetProgram:        /* only one byte required for these msgs */
    case msgChangePressure:
        sz = 2;
        break;

    default:                   /* double byte messages */
        sz = 3;
        break;
    }

    return midiTrackAddRaw(pMF, iTrack, sz, data, FALSE, 0);

}

BOOL midiTrackAddNote(MIDI_FILE *_pMF, int iTrack, int iNote, int iLength,
                      int iVol, BOOL bAutoInc, BOOL bOverrideLength)
{
    MIDI_FILE_TRACK *pTrk;
    Uint8 *ptr;
    BOOL bSuccess = FALSE;
    int i, chn;

    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;
    if (!IsNoteValid(iNote))
        return FALSE;

    pTrk = &pMF->Track[iTrack];
    ptr = _midiGetPtr(pMF, iTrack, DT_DEF);
    if (!ptr)
        return FALSE;

    chn = pTrk->iDefaultChannel;
    iLength = _midiGetLength(pMF->Header.PPQN, iLength, bOverrideLength);

    for (i = 0; i < sizeof(pTrk->LastNote) / sizeof(pTrk->LastNote[0]); ++i)
        if (pTrk->LastNote[i].valid == FALSE)
        {
            pTrk->LastNote[i].note = (Uint8) iNote;
            pTrk->LastNote[i].chn = (Uint8) chn;
            pTrk->LastNote[i].end_pos = pTrk->pos + pTrk->dt + iLength;
            pTrk->LastNote[i].valid = TRUE;
            bSuccess = TRUE;

            ptr = _midiWriteVarLen(ptr, pTrk->dt);      /* delta-time */
            *ptr++ = (Uint8) (msgNoteOn | chn);
            *ptr++ = (Uint8) iNote;
            *ptr++ = (Uint8) iVol;
            break;
        }

    if (!bSuccess)
        return FALSE;

    pTrk->ptr = ptr;

    pTrk->pos += pTrk->dt;
    pTrk->dt = 0;

    if (bAutoInc)
        return midiTrackIncTime(pMF, iTrack, iLength, bOverrideLength);

    return TRUE;
}

BOOL midiTrackAddRest(MIDI_FILE *_pMF, int iTrack, int iLength,
                      BOOL bOverridePPQN)
{
    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;

    iLength = _midiGetLength(pMF->Header.PPQN, iLength, bOverridePPQN);
    return midiTrackIncTime(pMF, iTrack, iLength, bOverridePPQN);
}

int midiTrackGetEndPos(MIDI_FILE *_pMF, int iTrack)
{
    _VAR_CAST;
    if (!IsFilePtrValid(pMF))
        return FALSE;
    if (!IsTrackValid(iTrack))
        return FALSE;

    return pMF->Track[iTrack].pos;
}

/*
** midiRead* Functions
*/
static Uint8 *_midiReadVarLen(Uint8 * ptr, Uint32 * num)
{
    register Uint32 value;
    register Uint8 c;

    if ((value = *ptr++) & 0x80)
    {
        value &= 0x7f;
        do
        {
            value = (value << 7) + ((c = *ptr++) & 0x7f);
        }
        while (c & 0x80);
    }
    *num = value;
    return (ptr);
}


static BOOL _midiReadTrackCopyData(MIDI_MSG * pMsg, Uint8 * ptr, Uint32 sz,
                                   BOOL bCopyPtrData)
{
    if (sz > pMsg->data_sz)
    {
        pMsg->data = (Uint8 *) realloc(pMsg->data, sz);
        pMsg->data_sz = sz;
    }

    if (!pMsg->data)
        return FALSE;

    if (bCopyPtrData && ptr)
        memcpy(pMsg->data, ptr, sz);

    return TRUE;
}

int midiReadGetNumTracks(const MIDI_FILE *_pMF)
{
    _VAR_CAST;
    return pMF->Header.iNumTracks;
}

BOOL midiReadGetNextMessage(const MIDI_FILE *_pMF, int iTrack,
                            MIDI_MSG * pMsg)
{
    MIDI_FILE_TRACK *pTrack;
    Uint8 *bptr, *pMsgDataPtr;
    int sz;

    _VAR_CAST;
    if (!IsTrackValid(iTrack))
        return FALSE;

    pTrack = &pMF->Track[iTrack];
    /* FIXME: Check if there is data on this track first!!! */
    if (pTrack->ptr >= pTrack->pEnd)
        return FALSE;

    pTrack->ptr = _midiReadVarLen(pTrack->ptr, &pMsg->dt);
    pTrack->pos += pMsg->dt;

    pMsg->dwAbsPos = pTrack->pos;

    if (*pTrack->ptr & 0x80)    /* Is this is sys message */
    {
        pMsg->iType = (tMIDI_MSG) ((*pTrack->ptr) & 0xf0);
        pMsgDataPtr = pTrack->ptr + 1;

        /* SysEx & Meta events don't carry channel info, but something
         ** important in their lower bits that we must keep */
        if (pMsg->iType == 0xf0)
            pMsg->iType = (tMIDI_MSG) (*pTrack->ptr);
    }
    else                        /* just data - so use the last msg type */
    {
        pMsg->iType = pMsg->iLastMsgType;
        pMsgDataPtr = pTrack->ptr;
    }

    pMsg->iLastMsgType = (tMIDI_MSG) pMsg->iType;
    pMsg->iLastMsgChnl = (Uint8) ((*pTrack->ptr) & 0x0f) + 1;

    switch (pMsg->iType)
    {
    case msgNoteOn:
        pMsg->MsgData.NoteOn.iChannel = pMsg->iLastMsgChnl;
        pMsg->MsgData.NoteOn.iNote = *(pMsgDataPtr);
        pMsg->MsgData.NoteOn.iVolume = *(pMsgDataPtr + 1);
        pMsg->iMsgSize = 3;
        break;

    case msgNoteOff:
        pMsg->MsgData.NoteOff.iChannel = pMsg->iLastMsgChnl;
        pMsg->MsgData.NoteOff.iNote = *(pMsgDataPtr);
        pMsg->iMsgSize = 3;
        break;

    case msgNoteKeyPressure:
        pMsg->MsgData.NoteKeyPressure.iChannel = pMsg->iLastMsgChnl;
        pMsg->MsgData.NoteKeyPressure.iNote = *(pMsgDataPtr);
        pMsg->MsgData.NoteKeyPressure.iPressure = *(pMsgDataPtr + 1);
        pMsg->iMsgSize = 3;
        break;

    case msgSetParameter:
        pMsg->MsgData.NoteParameter.iChannel = pMsg->iLastMsgChnl;
        pMsg->MsgData.NoteParameter.iControl = (tMIDI_CC) * (pMsgDataPtr);
        pMsg->MsgData.NoteParameter.iParam = *(pMsgDataPtr + 1);
        pMsg->iMsgSize = 3;
        break;

    case msgSetProgram:
        pMsg->MsgData.ChangeProgram.iChannel = pMsg->iLastMsgChnl;
        pMsg->MsgData.ChangeProgram.iProgram = *(pMsgDataPtr);
        pMsg->iMsgSize = 2;
        break;

    case msgChangePressure:
        pMsg->MsgData.ChangePressure.iChannel = pMsg->iLastMsgChnl;
        pMsg->MsgData.ChangePressure.iPressure = *(pMsgDataPtr);
        pMsg->iMsgSize = 2;
        break;

    case msgSetPitchWheel:
        pMsg->MsgData.PitchWheel.iChannel = pMsg->iLastMsgChnl;
        pMsg->MsgData.PitchWheel.iPitch =
            *(pMsgDataPtr) | (*(pMsgDataPtr + 1) << 7);
        pMsg->MsgData.PitchWheel.iPitch -= MIDI_WHEEL_CENTRE;
        pMsg->iMsgSize = 3;
        break;

    case msgMetaEvent:
        /* We can use 'pTrack->ptr' from now on, since meta events
         ** always have bit 7 set */
        bptr = pTrack->ptr;
        pMsg->MsgData.MetaEvent.iType = (tMIDI_META) * (pTrack->ptr + 1);
        pTrack->ptr = _midiReadVarLen(pTrack->ptr + 2, &pMsg->iMsgSize);
        sz = (pTrack->ptr - bptr) + pMsg->iMsgSize;

        if (_midiReadTrackCopyData(pMsg, pTrack->ptr, sz, FALSE) == FALSE)
            return FALSE;

        /* Now copy the data... */
        memcpy(pMsg->data, bptr, sz);

        /* Now place it in a neat structure */
        switch (pMsg->MsgData.MetaEvent.iType)
        {
        case metaMIDIPort:
            pMsg->MsgData.MetaEvent.Data.iMIDIPort = *(pTrack->ptr + 0);
            break;
        case metaSequenceNumber:
            pMsg->MsgData.MetaEvent.Data.iSequenceNumber = *(pTrack->ptr + 0);
            break;
        case metaTextEvent:
        case metaCopyright:
        case metaTrackName:
        case metaInstrument:
        case metaLyric:
        case metaMarker:
        case metaCuePoint:
            /* TODO - Add NULL terminator ??? */
            pMsg->MsgData.MetaEvent.Data.Text.pData = pTrack->ptr;
            break;
        case metaEndSequence:
            /* NO DATA */
            break;
        case metaSetTempo:
            {
                Uint32 us =
                    ((*(pTrack->ptr + 0)) << 16) | ((*(pTrack->ptr + 1)) << 8)
                    | (*(pTrack->ptr + 2));
                pMsg->MsgData.MetaEvent.Data.Tempo.iBPM = 60000000L / us;
            }
            break;
        case metaSMPTEOffset:
            pMsg->MsgData.MetaEvent.Data.SMPTE.iHours = *(pTrack->ptr + 0);
            pMsg->MsgData.MetaEvent.Data.SMPTE.iMins = *(pTrack->ptr + 1);
            pMsg->MsgData.MetaEvent.Data.SMPTE.iSecs = *(pTrack->ptr + 2);
            pMsg->MsgData.MetaEvent.Data.SMPTE.iFrames = *(pTrack->ptr + 3);
            pMsg->MsgData.MetaEvent.Data.SMPTE.iFF = *(pTrack->ptr + 4);
            break;
        case metaTimeSig:
            pMsg->MsgData.MetaEvent.Data.TimeSig.iNom = *(pTrack->ptr + 0);
            pMsg->MsgData.MetaEvent.Data.TimeSig.iDenom =
                *(pTrack->ptr + 1) * MIDI_NOTE_MINIM;
            /* TODO: Variations without 24 & 8 */
            break;
        case metaKeySig:
            if (*pTrack->ptr & 0x80)
            {
                /* Do some trendy sign extending in reverse :) */
                pMsg->MsgData.MetaEvent.Data.KeySig.iKey =
                    ((256 - *pTrack->ptr) & keyMaskKey);
                pMsg->MsgData.MetaEvent.Data.KeySig.iKey |= keyMaskNeg;
            }
            else
            {
                pMsg->MsgData.MetaEvent.Data.KeySig.iKey =
                    (tMIDI_KEYSIG) (*pTrack->ptr & keyMaskKey);
            }
            if (*(pTrack->ptr + 1))
                pMsg->MsgData.MetaEvent.Data.KeySig.iKey |= keyMaskMin;
            break;
        case metaSequencerSpecific:
            pMsg->MsgData.MetaEvent.Data.Sequencer.iSize = pMsg->iMsgSize;
            pMsg->MsgData.MetaEvent.Data.Sequencer.pData = pTrack->ptr;
            break;
        }

        pTrack->ptr += pMsg->iMsgSize;
        pMsg->iMsgSize = sz;
        break;

    case msgSysEx1:
    case msgSysEx2:
        bptr = pTrack->ptr;
        pTrack->ptr = _midiReadVarLen(pTrack->ptr + 1, &pMsg->iMsgSize);
        sz = (pTrack->ptr - bptr) + pMsg->iMsgSize;

        if (_midiReadTrackCopyData(pMsg, pTrack->ptr, sz, FALSE) == FALSE)
            return FALSE;

        /* Now copy the data... */
        memcpy(pMsg->data, bptr, sz);
        pTrack->ptr += pMsg->iMsgSize;
        pMsg->iMsgSize = sz;
        pMsg->MsgData.SysEx.pData = pMsg->data;
        pMsg->MsgData.SysEx.iSize = sz;
        break;
    }
    /*
     ** Standard MIDI messages use a common copy routine
     */
    pMsg->bImpliedMsg = FALSE;
    if ((pMsg->iType & 0xf0) != 0xf0)
    {
        if (*pTrack->ptr & 0x80)
        {
        }
        else
        {
            pMsg->bImpliedMsg = TRUE;
            pMsg->iImpliedMsg = pMsg->iLastMsgType;
            pMsg->iMsgSize--;
        }
        _midiReadTrackCopyData(pMsg, pTrack->ptr, pMsg->iMsgSize, TRUE);
        pTrack->ptr += pMsg->iMsgSize;
    }
    return TRUE;
}

void midiReadInitMessage(MIDI_MSG * pMsg)
{
    pMsg->data = NULL;
    pMsg->data_sz = 0;
    pMsg->bImpliedMsg = FALSE;
}

void midiReadFreeMessage(MIDI_MSG * pMsg)
{
    midiFree(pMsg->data);
    pMsg->data = NULL;
}
