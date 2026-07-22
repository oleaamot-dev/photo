/*---------------------------------------------------------------------*
 *                                                                     *
 * dump.c - formatted storage dump to file                             *
 *                                                                     *
 *---------------------------------------------------------------------*/
#include <stdio.h>
#include <ctype.h>
#include "dump.h"
/*---------------------------------------------------------------------*
 *                                                                     *
 * hex - return ascii char which represent                             *
 *       a hex digit                                                   *
 *                                                                     *
 *---------------------------------------------------------------------*/
static unsigned char hex(int digit)
{
   if (digit <= 9)
      return (digit + '0');
   else
      return (digit - 10 + 'A');
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * dump - format storage to file in dump format.                       *
 *                                                                     *
 *---------------------------------------------------------------------*/
void dump(FILE *fd, const char *title,
          const void *address, unsigned int len)
{
   unsigned char   low,
      hi;
   char            text[24],
      *linad,
      *tp;
   int             linelim,
      wordlim;
   int             offset=0;
   char           *area = (char *) address;
   linelim = 0;
   wordlim = 0;
   tp = text;
   fprintf(fd, "%s\n", title);
   linad = area;
   fprintf(fd, "   +%4.4x ", offset);
   sprintf(text, "%-20.20s", "");
   while (len--)
   {
      low = *area++;
      if (++linelim > 16)
      {
         linad += 16;
         wordlim = 0;
         linelim = 1;
         *(text + 16) = '\0';
         offset += 16;
         fprintf(fd, "  * %s *\n   +%4.4x ",text, offset);
         sprintf(text, "%-20.20s", "");
         tp = text;
      }
      *tp++ = isalnum (low) ? low : '.';
      if (++wordlim > 4)
      {
         wordlim = 1;
         fprintf(fd, "%c", ' ');
      }
      hi = low / 16;
      low = low - hi * 16;
      fprintf(fd, "%c", hex(hi));
      fprintf(fd, "%c", hex(low));
   }
   while (++linelim <= 16)
      {
      if (++wordlim > 4)
         {
         wordlim = 1;
         fprintf(fd, "%c", ' ');
         }
      fprintf(fd, "%c", ' ');
      fprintf(fd, "%c", ' ');
      }
   *(text + linelim) = '\0';
   fprintf(fd, "  * %s *\n", text);
   fflush(fd);
}
