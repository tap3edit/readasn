/****************************************************************************
|*
|* tap3edit Tools (http://www.tap3edit.com)
|*
|* $Id: readasn.h 46 2014-08-28 21:46:27Z mrjones $
|*
|* Copyright (c) 2005-2014, Javier Gutierrez <jgutierrez@tap3edit.com>
|* 
|* Permission to use, copy, modify, and/or distribute this software for any
|* purpose with or without fee is hereby granted, provided that the above
|* copyright notice and this permission notice appear in all copies.
|* 
|* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
|* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
|* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
|* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
|* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
|* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
|* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
|*
|*
|* Module: readasn.h
|*
|* Author: Javier Gutierrez (JG)
|*
|* Modifications:
|*
|* When         Who     Pos     What
|* 20120226     JG              Initial Version
|*
****************************************************************************/

#ifndef _READASN_H_
#define _READASN_H_

/* 2. Defines */

#ifndef TRUE
    #define FALSE 0
    #define TRUE (!FALSE)
#endif


#ifndef MAXLEN
    #define MAXLEN 50
#endif

#ifndef REALLOC_INCR_FACTOR
    #define REALLOC_INCR_FACTOR 10
#endif

#ifndef MAXTAGS
    #define MAXTAGS 560
#endif

/* File type */
#define FT_UNK 0x01     /* Unknown type of file */
#define FT_TAP 0x02     /* Tap file */
#define FT_NOT 0x03     /* Notification file */
#define FT_NRT 0x04     /* NRTRDE file */
#define FT_RAP 0x05     /* RAP file */
#define FT_ACK 0x06     /* Acknowledge file */


/* 3. Typedefs and structures */

typedef unsigned char uchar;
typedef struct _asn1item
{
    unsigned    class: 2;       /* Class */
    unsigned    pc: 1;          /* Primitive/Constructed */
    int         tag;            /* Tag: decimal format */
    uchar       tag_x[4];       /* Tag: bcd format */
    char        tag_h[9];       /* Tag: hexadecimal string format */
    int         tag_l;          /* Tag: number of bytes in file */
    long        size;           /* Size: decimal format */
    uchar       size_x[8];      /* Size: bcd format */
    char        size_h[17];     /* Size: hexadecimal format */
    int         size_l;         /* Size: number of bytes in file */
} asn1item;

typedef struct _gsmainfo_t
{
    int         ver;            /* File version */
    int         rel;            /* File release */
    int         rap_ver;        /* RAP File version */
    int         rap_rel;        /* RAP File release */
} gsmainfo_t;


void            tagid_init      (void);
int             merge_tap_rapids(char tap_tagname_map[MAXTAGS][MAXLEN], char rap_tagname_map[MAXTAGS][MAXLEN]);

#endif

/* EOF */
