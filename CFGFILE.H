/*
   CFGFILE.H

   Config file interface header file for FMail 1.59/beta
   Copyright (C) 2001 Folkert J. Wijnstra. All rights reserved.

   All information in this document is subject to change at any time
   without prior notice!
*/


#define CFG_GENERAL   0
#define CFG_NODES     1
#define CFG_ECHOAREAS 2
#define CFG_AREADEF   3
#define MAX_CFG_FILES 4

s16 openConfig (u16 fileType, headerType **header, void **buf);
s16 getRec (u16 fileType, s16 index);
s16 putRec (u16 fileType, s16 index);
s16 insRec (u16 fileType, s16 index);
s16 delRec (u16 fileType, s16 index);
s16 chgNumRec   (u16 fileType, s16 number);
s16 closeConfig (u16 fileType);
