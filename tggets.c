/* file tggets.c - testing ggets() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ggets.h"

int main(int argc, char **argv)
{
   FILE *infile;
   char *line;
   int   cnt;

   if (argc == 2)
      if ((infile = fopen(argv[1], "r"))) {
         cnt = 0;
         while (0 == fggets(&line, infile)) {
            fprintf(stderr, "%4d %4d\n", ++cnt, (int)strlen(line));
            (void)puts(line);
            free(line);
         }
         return 0;
      }
   (void)puts("Usage: tggets filetodisplay");
   return EXIT_FAILURE;
} /* main */
/* END file tggets.c */
