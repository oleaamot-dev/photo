#ifndef RCUTIL_H
#define RCUTIL_H
typedef struct
{
   char device[64];
   char path[128];
   char speed[16];
   char pacing[16];
   char quality[16];
   char focus[16];
   char flash[16];
   char autooff[16];
   char timer[16];
   char redeye[16];
   char tracefile[128];
   char tracebytes[16];
}  QM100_CONFIGDATA;
void qm100_setDefaults(QM100_CONFIGDATA *);
void qm100_readConfigData(QM100_CONFIGDATA *);
void qm100_saveConfigData(QM100_CONFIGDATA *);
#endif
