#include "qm100.h"
#include <errno.h>
#include <error.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
/*---------------------------------------------------------------------*
 *                                                                     *
 * setPathName - local routine to construct name of the                *
 *               user's gphoto directory.                              *
 *                                                                     *
 *---------------------------------------------------------------------*/
static void setPathName(char *fname)
{
   char *sp;
   int   flen;
   sp = getenv("HOME");
   if (!sp)
      sp = ".";
   strcpy(fname, sp);
   flen = strlen(fname);
   while (fname[flen-1] == '/')
      {
      fname[flen-1] = '\0';
      flen--;
      }
   if (!strstr(fname, "/.gphoto"))
      strcat(fname, "/.gphoto");
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * setFileName - local routine to construct the name of the user's     *
 *               konica configuration file.                            *
 *                                                                     *
 *---------------------------------------------------------------------*/
static void setFileName(char *fname)
{
   setPathName(fname);
   strcat(fname, "/konicarc");
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * setDefaults - populate the configData structure with                *
 *               default values.                                       *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_setDefaults(QM100_CONFIGDATA *cp)
{
   setPathName(cp->path);
   strcpy(cp->device,     DEFAULT_PORT);
   strcpy(cp->speed,      DEFAULT_SPEED);
   strcpy(cp->pacing,     DEFAULT_PACING);
   strcpy(cp->tracefile,  DEFAULT_TRACE);
   strcpy(cp->tracebytes, DEFAULT_TRACEB);
   strcpy(cp->quality,    DEFAULT_QUALITY);
   strcpy(cp->focus,      DEFAULT_FOCUS);
   strcpy(cp->flash,      DEFAULT_FLASH);
   strcpy(cp->autooff,    DEFAULT_AUTOOFF);
   strcpy(cp->timer,      DEFAULT_TIMER);
   strcpy(cp->redeye,     DEFAULT_REDEYE);
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * readConfigData - read saved settings from the configuration file.   *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_readConfigData(QM100_CONFIGDATA *cp)
{
   FILE *fd;
   char  fname[128];
   qm100_setDefaults(cp);
   setFileName(fname);
   fd = fopen(fname, "r");
   if (fd)
      {
      char buf[256];
      char *sp, *vp;
      while ((sp = fgets(buf, sizeof(buf)-1, fd)) != NULL)
         {
         if (*sp == '#' || *sp == '*')
            continue;
         sp = strtok(buf, " \t\r\n");
         if (!sp)
            continue;    /* skip blank lines */
         vp = strtok(NULL, " \t\r\n");
         if (!vp)
            {
            printf("No value for %s - ignored\n", sp);
            continue;
            }
         if (strcasecmp(sp, "Speed") == 0)
            strcpy(cp->speed, vp);
         else if (strcasecmp(sp, "Pacing") == 0)
            strcpy(cp->pacing, vp);
         else if (strcasecmp(sp, "Camera") == 0)
            strcpy(cp->device, vp);
         else if (strcasecmp(sp, "Trace") == 0)
            strcpy(cp->tracefile, vp);
         else if (strcasecmp(sp, "Trace_Bytes") == 0)
            strcpy(cp->tracebytes, vp);
         else if (strcasecmp(sp, "Quality") == 0)
            strcpy(cp->quality, vp);
         else if (strcasecmp(sp, "Focus") == 0)
            strcpy(cp->focus, vp);
         else if (strcasecmp(sp, "Flash") == 0)
            strcpy(cp->flash, vp);
         else if (strcasecmp(sp, "AutoOff") == 0)
            strcpy(cp->autooff, vp);
         else if (strcasecmp(sp, "Timer") == 0)
            strcpy(cp->timer, vp);
         else if (strcasecmp(sp, "RedEye") == 0)
            strcpy(cp->redeye, vp);
         else
            printf("Unknown keyword %s in %s - ignored\n",
                   sp, fname);
         }
      fclose(fd);
      }
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * saveConfigData - write  curent settings to the configuration file,  *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_saveConfigData(QM100_CONFIGDATA *cp)
{
   FILE *fd;
   char  fname[128];
   setFileName(fname);
   fd = fopen(fname, "w");
   if (!fd)
      {
      char cmd[140];
      sprintf(cmd, "mkdir %s",  cp->path);
      system(cmd);
      fd = fopen(fname, "w");
      }
   if (fd)
      {
      struct tm *tp;
      time_t    mytime;
      mytime = time(NULL);
      tp = localtime(&mytime);
      fprintf(fd, "#  konicarc - saved on %4.4d/%2.2d/%2.2d at %2.2d:%2.2d\n",
              tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday,
              tp->tm_hour, tp->tm_min);
      fprintf(fd, "%-12.12s %s\n", "Camera", cp->device);
      fprintf(fd, "%-12.12s %s\n", "Speed", cp->speed);
      fprintf(fd, "%-12.12s %s\n", "Pacing", cp->pacing);
      fprintf(fd, "%-12.12s %s\n", "Quality",cp->quality);
      fprintf(fd, "%-12.12s %s\n", "Focus", cp->focus);
      fprintf(fd, "%-12.12s %s\n", "Flash", cp->flash);
      fprintf(fd, "%-12.12s %s\n", "AutoOff", cp->autooff);
      fprintf(fd, "%-12.12s %s\n", "Timer", cp->timer);
      fprintf(fd, "%-12.12s %s\n", "RedEye", cp->redeye);
      fprintf(fd, "%-12.12s %s\n", "Trace", cp->tracefile);
      fprintf(fd, "%-12.12s %s\n", "Trace_Bytes", cp->tracebytes);
      }
   else
      printf("Unable to open/create %s - configuration not saved\n",
             fname);
}
