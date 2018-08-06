/****************************************************************************
|*
|* tap3edit Tools (http://www.tap3edit.com)
|*
|* Copyright (c) 2005-2018, Javier Gutierrez <https://github.com/tap3edit/readasn>
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
|* Module: readtap.c
|*
|* Description: Universal ASN.1 decoder. Dumps the content of the file to
|*              the screen. For TAP, NRT and RAP files the tag names can
|*              be also shown together with the tag ids.
|*
|* Return:
|*      0: successful
|*      1: error
|*
|*
|*
|* Author: Javier Gutierrez (JG)
|*
|* Modifications:
|*
|* When         Who     Pos     What
|* 20120226     JG              Initial version (redesign of readtap). 
|*
****************************************************************************/

/* 1. Includes */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>


#include "readasn.h"


/* 2. Global Variables */

static long    pos = 0;                         /* Current position in file */
static char   (*tagname)[MAXLEN];               /* Array with the tag definition */
static int     use_tagnames = TRUE;             /* Flag to use tagnames. Default->TRUE */

char    nrt0201_tagname_map[MAXTAGS][MAXLEN];
char    rap01XX_tagname_map[MAXTAGS][MAXLEN];
char    tap03le09_tagname_map[MAXTAGS][MAXLEN];
char    tap03ge10_tagname_map[MAXTAGS][MAXLEN];

uchar   *buffin_str = NULL, *buffin_str_tmp = NULL;
long    buffin_str_len = 0;

/* 3. Prototypes */

static int      decode_asn      (FILE *file, long size, int is_indef, int is_root, int recno, int is_tap, int depth);
static int      decode_size     (FILE *file, asn1item *a_item);
static int      decode_tag      (FILE *file, asn1item *a_item);
static void     bcd_2_hexa      (char *str2, const uchar *str1, const int len);

//static int      read_def_file   (void);
static int      is_printable    (uchar *str, long len);
static void     help            (char* program_name);
static int      get_file_type   (FILE* file, int *file_type, gsmainfo_t *gsminfo);


/****************************************************************************
|* 
|* Function: printout
|* 
|* Description; 
|* 
|*     Print out according to format and parameters
|* 
|* Return:
|*      void
|* 
|* 
|* Author: Javier Gutierrez (JG)
|* 
|* Modifications:
|* 20050719    JG    Initial version
|* 
****************************************************************************/
static void printout(int depth, long pos, int recno, const char *format, ...)
{
    
   va_list args;

   va_start(args, format);

    printf("%08ld:%04d ", pos, recno);
    for (; depth != 0; depth--)
        printf("    ");
    vfprintf(stdout, format, args);

   va_end(args);
}


int main(int argc, char **argv)
{
    FILE*           file = NULL;
    char*           filename = "";
    long            size = 0;
    int             file_type = FT_UNK;
    char*           program_name = argv[0];
    gsmainfo_t      gsmainfo;

    memset(&gsmainfo, 0x00, sizeof(gsmainfo));


    /* 1. Checking parameters */

    filename = argv[1];

    if (argc != 2 && argc != 3)
        help(program_name);

    if ( argc == 3 )
    {
        /* 1.1. -n : Use tags */

        if ( argv[1][0] == '-' && argv[1][1] == 'n' )
            use_tagnames = FALSE;
        else
            help(program_name);

        filename = argv[2];
    }


    /* 2. Open Input File */
    
    if ( ( file = fopen(filename, "rb") ) == NULL )
    {
        fprintf(stderr, "Cannot open file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* 3. Get File Type */
    if ( get_file_type(file, &file_type, &gsmainfo) != 0)
    {
        fprintf(stderr, "Error getting the type of file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    printf("File type: %s ver: %d, rel: %d, rap_ver: %d, rap_rel: %d\n", 
            (file_type == FT_TAP ? "TAP" : (file_type == FT_NOT ? "NOT" : (file_type == FT_RAP ? "RAP" : (file_type == FT_NRT ? "NRT" : "UNK")))),
            gsmainfo.ver, gsmainfo.rel, gsmainfo.rap_ver, gsmainfo.rap_rel);

    if (use_tagnames && file_type != FT_UNK)
    {
        tagid_init();
        if (
                ((file_type == FT_TAP || file_type == FT_NOT || file_type == FT_RAP) && gsmainfo.ver == 3) ||
                (file_type == FT_ACK && gsmainfo.ver == 0)
           )
        {
            if (gsmainfo.rel <= 9)
            {
                tagname = tap03le09_tagname_map;
            }
            else
            {
                tagname = tap03ge10_tagname_map;
            }

            if (((file_type == FT_RAP || file_type == FT_ACK) && gsmainfo.rap_ver == 1))
            {
                if (merge_tap_rapids(tagname, rap01XX_tagname_map) != 0)
                {
                    exit(EXIT_FAILURE);
                }
            }

        }
        else if (file_type == FT_NRT)
        {
            tagname = nrt0201_tagname_map;
        } 
        else
        {
            use_tagnames = FALSE;
        }
    }
    else
    {
        use_tagnames = FALSE;
    }

    /* 4. Get file size */

    if (fseek(file, 0, SEEK_END) != 0) // seek to end of file
    {
        fprintf(stderr, "Error moving to the end of the file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    size = ftell(file); // get current file pointer
    if (fseek(file, 0, SEEK_SET) != 0) // seek back to beginning of file
    {
        fprintf(stderr, "Error moving to the beginning of the file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }


    /* 5. Decode and prints file */

    if ( (decode_asn(
                    file,                                   /* file */
                    size,                                   /* size */
                    FALSE,                                  /* is_indef */
                    (file_type == FT_UNK ? TRUE : FALSE),   /* is_root */
                    (file_type == FT_UNK ? 1 : 0),          /* recno */
                    file_type,                              /* file_type */
                    0                                       /* depth */
                    )
         ) == -1 )
    {
        //fprintf(stderr, "Error decoding file\n");
        exit(EXIT_FAILURE);
    }


    /* 6. Closing and End. */

    (void)fclose(file);

    if (buffin_str) 
    {
        free(buffin_str);
    }

    return(EXIT_SUCCESS);
}

/****************************************************************************
|* 
|* Function: decode_asn
|* 
|* Description; 
|* 
|*     recursive function to decode a tag item.
|* 
|* Return:
|*      0: Successful
|*     -1: Error decoding
|* 
|* 
|* Author: Javier Gutierrez (JG)
|* 
|* Modifications:
|* 20050707    JG    Initial version
|* 
****************************************************************************/
static int decode_asn(
    FILE*               file,           /* File handler to decode */
    long                size,           /* Size within to decode */
    int                 is_indef,       /* Flag indicating if encoding is indefinite (FALSE/TRUE) */
    int                 is_root,        /* Flag indicating if it's the root of the encoding */
    int                 recno,          /* Root Record number */
    int                 file_type,      /* Flag indicating if the file is TAP */
    int                 depth           /* Depth in which we are (to print) */
)
{
    asn1item            a_item;
    int                 is_root_loc = is_root, recno_loc = recno;
    long long           sum_up = 0;
    long                loc_pos = pos, i = 0;

    memset(&a_item, 0x00, sizeof(a_item));

    /* 1. Process all size received from our parent */

    while (size >0 || is_indef)
    {
        /* 1.1. TAG:   decode */

        if (decode_tag(file, &a_item) == -1)
        {
            fprintf(stderr, "Error decoding tag at position: %ld\n", pos);
            return -1;
        }

        //size -= a_item.tag_l;


        /* 1.2. SIZE:  decode */

        if (decode_size(file, &a_item) == -1)
        {
            fprintf(stderr, "Error decoding size at position: %ld\n", pos);
            return -1;
        }

        //size -= a_item.size_l;


        /* 1.3. Did we find 2 null bytes or trash byte? */

        if ( a_item.tag == 0 && a_item.size == 0 && is_indef)
        {

            /* 1.3.1. End of indefinite length found */

            {
                /* Display */
                printout(depth, loc_pos, recno, "%sTag: 000 \"00\"h Size: 0 \"00\"h {\"\" \"\"h}\n",
                    use_tagnames
                        ? "EoE => "
                        : ""
                );
            }

            break;
        }
        else if ( a_item.tag_x[0] == 0x00 && a_item.size != 0 && ! is_indef)
        {

            /* 1.3.2. Trash byte: Rewind one byte in order to try to recover and keep decoding */

            loc_pos++;
            pos = loc_pos;
            size--;
            if (fseek(file, pos, SEEK_SET) != 0) // rewind 1 byte
            {
                fprintf(stderr, "Error moving 1 byte back in file: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            continue;
        }



        /* 1.4. VALUE: Primitive or Constructed */

        {

            /* 1.4.2. Proceed according to Primitive and Constructed Tag */

            if (a_item.pc == 0)
            {
                /* 1.4.2.1. Primitive */

                {
                    /* 1.4.2.1.1 Display */

                    printout(depth, loc_pos, recno, "%s%sTag: %03d \"%s\"h Size: %d \"%s\"h {", 
                        use_tagnames
                            ? tagname[a_item.tag][0] == '\0'
                                ? "Unknow Tag"
                                : tagname[a_item.tag]
                            : "", 
                        use_tagnames
                            ? " => "
                            : "",
                        a_item.tag, a_item.tag_h, a_item.size, a_item.size_h);
                }

                /* 1.4.2.1.2 Alloc and read element */

                if (!buffin_str_len)
                {
                    if ( ( buffin_str = (uchar *)malloc((size_t)a_item.size + 1 * sizeof(uchar)) ) == NULL )
                    {
                        fprintf(stderr, "Couldn't allocate memory. Size too long at pos: %ld\n", pos);
                        return -1;
                    }
                    buffin_str_len = a_item.size;
                }
                else
                {
                    if (a_item.size > buffin_str_len )
                    {
                        if ( ( buffin_str_tmp = (uchar *)realloc(buffin_str, (size_t)a_item.size + 1 * sizeof(uchar)) ) == NULL )
                        {
                            fprintf(stderr, "Couldn't allocate memory. Size too long at pos: %ld\n", pos);
                            return -1;
                        }
                        buffin_str = buffin_str_tmp;
                        buffin_str_len = a_item.size;
                    }
                }



                (void)fread(buffin_str, sizeof(uchar), (size_t)a_item.size, file);
                if(feof(file) != 0)
                {
                    fprintf(stderr, "Found end of file too soon at position: %ld\n", pos);
                    if (buffin_str) free(buffin_str);
                    return -1;
                }


                {
                    /* 1.4.2.1.3 Display */

                    if ((size_t)a_item.size <= sizeof(sum_up))
                    {
                        sum_up = 0;
                        for(i = 0; i < a_item.size ; i++)
                        {
                            sum_up <<= 8;
                            sum_up += (long)buffin_str[i];
                        }

                        printf("%lld ", sum_up);
                    }

                    if(is_printable(buffin_str, a_item.size))
                    {
                        printf("\"");
                        for(i = 0; i < a_item.size; i++)
                            printf("%c", buffin_str[i]);
                        printf("\"");
                    }
                    else
                    {
                        printf("\"\"");
                    }

                    printf(" \"");
                    for (i = 0; i < a_item.size; i++)
                        printf("%02x", (unsigned int)buffin_str[i]);

                    printf("\"h}\n");

                }

                pos += a_item.size;

            }
            else
            {
                /* 1.4.2.2. Constructed */

                {
                    /* 1.4.2.2.1 Display */

                    printout(depth, loc_pos, recno, "%s%sTag: %03d \"%s\"h Size: %d \"%s\"h\n", 
                            use_tagnames
                                ? tagname[a_item.tag][0] == '\0'
                                    ? "Unknow Tag"
                                    : tagname[a_item.tag]
                                : "", 
                            use_tagnames
                                ? " => "
                                : "",
                            a_item.tag, a_item.tag_h, a_item.size, a_item.size_h);

                    printout(depth, pos, recno, "{\n");

                }

                /* 1.4.2.2.2 Decode the constructed element */

                //if (!(!a_item.size && !a_item.size_x[0]) )
                if (a_item.size_x[0]) // If size == 0x80 then it's an indifinite constructed, if 0x00 might be an empty constructed.
                {


                    if (file_type == FT_UNK) {is_root_loc = FALSE; recno_loc = recno;}
                    if (file_type == FT_TAP) {is_root_loc = (a_item.tag == 3 ? TRUE : FALSE); recno_loc = (a_item.tag == 3 ? 1 : recno);}
                    if (file_type == FT_NOT) {is_root_loc = FALSE; recno_loc = recno;}
                    if (file_type == FT_NRT) {is_root_loc = (a_item.tag == 2 ? TRUE : FALSE); recno_loc = (a_item.tag == 2 ? 1 : recno);}
                    if (file_type == FT_RAP) {is_root_loc = (a_item.tag == 536 ? TRUE : FALSE); recno_loc = (a_item.tag == 536 ? 1 : recno);}
                    if (file_type == FT_ACK) {is_root_loc = FALSE; recno_loc = recno;}

                    if ( (decode_asn(
                                    file,                               /* file */
                                    a_item.size,                        /* size */
                                    (a_item.size == 0 ? TRUE : FALSE),  /* is_indef */
                                    is_root_loc,                        /* is_root */
                                    recno_loc,                          /* recno */
                                    file_type,                          /* file_type */
                                    depth +1)                           /* depth */
                         ) == -1 )
                    {
                        return -1;
                    }
                }

                {
                    /* 1.4.2.2.2 Display */

                    printout(depth, pos, recno, "}\n");

                }

            }

            size -= pos - loc_pos ; //a_item.size;

        }

        loc_pos = pos;

        if (is_root)
        {
            recno++;
        }

    }

    return 0;
}


/****************************************************************************
|* 
|* Function: decode_tag
|* 
|* Description; 
|* 
|*     decodes tag into a asn1item
|* 
|* Return:
|*      0: Successful
|*     -1: Error decoding
|* 
|* 
|* Author: Javier Gutierrez (JG)
|* 
|* Modifications:
|* 20050715    JG    Initial version
|* 
****************************************************************************/
static int decode_tag(
    FILE*       file,       /* File handler */
    asn1item*   a_item      /* pointer asn1item where to store the information */
)
{
    uchar       buffin;
    int         i;


    a_item->tag = 0;

    /* 1. Read from file */

    buffin = (uchar)fgetc(file);
    if(feof(file) != 0)
    {
        fprintf(stderr, "Found end of file too soon at position: %ld\n", pos);
        return -1;
    }
    pos++;


    /* 2. Store class, primitive/constructed info and hexa tag */

    a_item->class = (unsigned) buffin>>6;
    a_item->pc = (unsigned) (buffin>>5)&0x1;
    a_item->tag_x[0] = buffin;
    a_item->tag_l = 1;


    /* 3. Work according to number of tag octets */

    if ( ( buffin & 0x1F ) == 0x1F )
    {
        /* 3.1 Tag has more than one octect */

        for(i = 1; i<=4; i++) 
        {
            buffin = (uchar)fgetc(file);
            if(feof(file) != 0)
            {
                fprintf(stderr, "Found end of file too soon at position: %ld\n", pos);
                return -1;
            }
            pos++;

            a_item->tag <<= 7;
            a_item->tag += (int)(buffin&0x7F);
            a_item->tag_x[i] = buffin;
            a_item->tag_l += 1;

            if ( (buffin>>7) == 0 ) 
                break;

        }

        if ( i>3 )
        {
            fprintf(stderr, "Found tag bigger than 4 bytes at position: %ld\n", pos);
            return -1;
        }

    }
    else
    {
        /* 3.2 Tag has just one octect */

        a_item->tag = (int)buffin&0x1F;
    }

    bcd_2_hexa(a_item->tag_h, a_item->tag_x, a_item->tag_l);

    return 0;

}


/****************************************************************************
|* 
|* Function: decode_size
|* 
|* Description; 
|* 
|*     decodes size into a asn1item
|* 
|* Return:
|*      0: Successful
|*     -1: Error decoding
|* 
|* 
|* Author: Javier Gutierrez (JG)
|* 
|* Modifications:
|* 20050715    JG    Initial version
|* 
****************************************************************************/
static int decode_size(
    FILE*       file,         /* File handler */
    asn1item*   a_item        /* pointer asn1item where to store the information */
)
{
    uchar       buffin;
    int         i;

    
    a_item->size = 0;


    /* 1. Read from file */

    buffin = (uchar)fgetc(file);
    if(feof(file) != 0)
    {
        if (a_item->tag_x[0] == 0x00) /* To avoid giving error on trash bytes */
        {
            a_item->size = 1;
            return 0;
        }
        fprintf(stderr, "Found end of file too soon at position: %ld\n", pos);
        return -1;
    }
    pos++;


    /* 2. Storing size_x */

    a_item->size_x[0] = buffin;
    a_item->size_l = 1;


    /* 3. Work according the number of octets */

    if (buffin>>7)
    {
        /* 3.1. Size with more than one octet */

        for(i = 1; (i <= (int)(a_item->size_x[0] & 0x7F)) && (i <= 4); i++)
        {
            buffin = (uchar)fgetc(file);
            if(feof(file) != 0)
            {
                fprintf(stderr, "Found end of file too soon at position: %ld\n", pos);
                return -1;
            }
            pos++;

            a_item->size <<= 8;
            a_item->size += (int)buffin;
            a_item->size_x[i] = buffin;
            a_item->size_l += 1;
        }

        if ( i>7 )
        {
            fprintf(stderr, "Found size bigger than 8 bytes at position: %ld\n", pos);
            return -1;
        }


    }
    else
    {
        /* 3.2. Size with just one octet */

        a_item->size = (int)(buffin);
    }

    bcd_2_hexa(a_item->size_h, a_item->size_x, a_item->size_l);

    return 0;

}


/****************************************************************************
|* 
|* Function: bcd_2_hexa
|* 
|* Description; 
|* 
|*     Converts a bcd chain into an hexadecimal string
|* 
|* Return:
|*      void
|* 
|* 
|* Author: Javier Gutierrez (JG)
|* 
|* Modifications:
|* 20050719    JG    Initial version
|* 
****************************************************************************/
static void bcd_2_hexa(
    char*           str2,   /* String to store the converted value */
    const uchar*    str1,   /* String to convert */ 
    const int       len     /* Because the string can contain \0 we cannot use strlen() */
)
{
    int     i = 0;
    
    for (i = 0; i < len; i++)
        sprintf(&str2[i*2], "%02x", (unsigned) str1[i]);

    str2[i*2] = '\0';

}


/****************************************************************************
|* 
|* Function: isprintable
|* 
|* Description; 
|* 
|*     Check if a complete string is printable
|* 
|* Return:
|*      0: No printable
|*      1: Printable
|* 
|* Author: Javier Gutierrez (JG)
|* 
|* Modifications:
|* 20050719    JG    Initial version
|* 
****************************************************************************/
static int is_printable(
    uchar *str, 
    long len
)
{
    long i = 0, eol = 0;

    for (i = 0; i < len; i++)
    {
        /* Note: sometimes they send an End of Line (0A) in the RAP comments */
        if (!isprint(str[i]) && str[i] != 0x0a) 
            return 0;
        if (str[i] == 0x0a)
            eol++;
    }

    /* Note: we only accept End of Lines if the message is long enough. Let's say: 7 */
    if (eol > 0 && len < 7)
        return 0;

    return 1;
}

/****************************************************************************
|* 
|* Function: get_file_type
|* 
|* Description; 
|* 
|*     Show usage
|* 
|* Return:
|*      int
|* 
|* Author: Javier Gutierrez (JG)
|* 
|* Modifications:
|* 20120306    JG    Initial version
|* 
****************************************************************************/
static int get_file_type(FILE* file, int *file_type, gsmainfo_t *gsmainfo)
{
    uchar       buffin_str[200]; /* First bytes of the file */
    int         buffin_len = 200, i = 0;

    memset(buffin_str, 0x00, sizeof(buffin_str));

    if (file == NULL || file_type == NULL || gsmainfo == NULL)
    {
        fprintf(stderr, "Passed NULL Arguments");
        return -1;
    }

    if (fread(buffin_str, sizeof(uchar), (size_t)buffin_len, file) == 0)
    {
        fprintf(stderr, "Error reading file at position: %d\n", 1);
        return -1;
    }

    /* 
     * We try to recognize the type of the file with this algorithm:
     * 
     * Regular expression   File type   Description
     * ^61.+5f814405        FT_TAP      TAP file
     * ^62                  FT_NOT      Notification file
     * ^7f8416              FT_RAP      RAP file
     * ^7f8417              FT_ACK      Acknowledge file
     * ^61.+5f2901          FT_NRT      NRTRDE file
     * Otherwise            FT_UNK      Any other ASN.1 file
     *
     */
    switch(buffin_str[0])
    {
        case 0x61: /* TAP or NRT */
            for (i = 1; i < 150; i++)
            {
                if (buffin_str[i] == 0x5f &&
                        buffin_str[i + 1] == 0x81 &&
                        buffin_str[i + 2] == 0x49 &&
                        buffin_str[i + 3] == 0x01)
                {
                    gsmainfo->ver = (int)buffin_str[i + 4];
                }

                if (buffin_str[i] == 0x5f &&
                        buffin_str[i + 1] == 0x81 &&
                        buffin_str[i + 2] == 0x3d &&
                        buffin_str[i + 3] == 0x01)
                {
                    gsmainfo->rel = (int)buffin_str[i + 4];
                    *file_type =  FT_TAP;
                    break;
                }

            }
            for (i = 1; i < 28; i++)
            {
                if (buffin_str[i] == 0x5f &&
                        buffin_str[i + 1] == 0x29 &&
                        buffin_str[i + 2] == 0x01)
                {
                    gsmainfo->ver = (int)buffin_str[i + 3];
                }

                if (buffin_str[i] == 0x5f &&
                        buffin_str[i + 1] == 0x25 &&
                        buffin_str[i + 2] == 0x01)
                {
                    gsmainfo->rel = (int)buffin_str[i + 3];
                    *file_type =  FT_NRT;
                    break;
                }
            }
            break;
        case 0x62:
            for (i = 1; i < 150; i++)
            {
                if (buffin_str[i] == 0x5f &&
                        buffin_str[i + 1] == 0x81 &&
                        buffin_str[i + 2] == 0x49 &&
                        buffin_str[i + 3] == 0x01)
                {
                    gsmainfo->ver = (int)buffin_str[i + 4];
                }

                if (buffin_str[i] == 0x5f &&
                        buffin_str[i + 1] == 0x81 &&
                        buffin_str[i + 2] == 0x3d &&
                        buffin_str[i + 3] == 0x01)
                {
                    gsmainfo->rel = (int)buffin_str[i + 4];
                    *file_type =  FT_NOT;
                    break;
                }

            }
            break;
        case 0x7f: /* RAP or ACK */
            if (buffin_str[1] == 0x84 && buffin_str[2] == 0x16)
            {
                for (i = 1; i < 150; i++)
                {
                    if (buffin_str[i] == 0x5f &&
                            buffin_str[i + 1] == 0x81 &&
                            buffin_str[i + 2] == 0x49 &&
                            buffin_str[i + 3] == 0x01)
                    {
                        gsmainfo->ver = (int)buffin_str[i + 4];
                    }

                    if (buffin_str[i] == 0x5f &&
                            buffin_str[i + 1] == 0x81 &&
                            buffin_str[i + 2] == 0x3d &&
                            buffin_str[i + 3] == 0x01)
                    {
                        gsmainfo->rel = (int)buffin_str[i + 4];
                    }

                    if (buffin_str[i] == 0x5f &&
                            buffin_str[i + 1] == 0x84 &&
                            buffin_str[i + 2] == 0x20 &&
                            buffin_str[i + 3] == 0x01)
                    {
                        gsmainfo->rap_ver = (int)buffin_str[i + 4];
                    }

                    if (buffin_str[i] == 0x5f &&
                            buffin_str[i + 1] == 0x84 &&
                            buffin_str[i + 2] == 0x1f &&
                            buffin_str[i + 3] == 0x01)
                    {
                        gsmainfo->rap_rel = (int)buffin_str[i + 4];
                        *file_type =  FT_RAP;
                        break;
                    }

                }
            }
            if (buffin_str[1] == 0x84 && buffin_str[2] == 0x17)
            {
                gsmainfo->rap_ver = 1; /* Ack files have no version */
                gsmainfo->rap_rel = 5;
                *file_type =  FT_ACK;
            }
            break;
        default:
            *file_type = FT_UNK;
    }

    return 0;
}

/****************************************************************************
|* 
|* Function: help
|* 
|* Description; 
|* 
|*     Show usage
|* 
|* Return:
|*      int
|* 
|* Author: Javier Gutierrez (JG)
|* 
|* Modifications:
|* 20050730    JG    Initial version
|* 
****************************************************************************/
static void help(char *program_name)
{
    fprintf(stderr, "Copyright (c) 2005-2018 Javier Gutierrez. (https://github.com/tap3edit/readasn)\n");
    fprintf(stderr, "Usage: %s [-n] filename\n", program_name);
    fprintf(stderr, "  -n : Do not print default GSMA tagnames (TAP, RAP, NRT)\n");
    exit (EXIT_FAILURE);
}
