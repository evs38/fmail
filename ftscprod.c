//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2017 Wilfred van Velzen
//
//
//  This file is part of FMail.
//
//  FMail is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  FMail is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//---------------------------------------------------------------------------

#include <errno.h>
#include <fcntl.h>    // open(); O_flags
#include <stdio.h>    // sprintf()
#include <stdlib.h>   // malloc
#include <string.h>   // strerror()
#include <unistd.h>

#include "fmail.h"

#include "ftscprod.h"
#include "log.h"
#include "os.h"
#include "os_string.h"
#include "utils.h"

#define dTBLSIZE  600
//---------------------------------------------------------------------------
typedef struct
{
  u16   prodcode;
  char *prodname;
} ftscps;

static ftscps *fileTbl = NULL
            , *tbl     = NULL;
static int     tSize   = 0;

const static char *dMEM_ERR_STR = "Not enough memory available in GetFtscProdStr";
//---------------------------------------------------------------------------
// Updated upto ftscprod.017
//---------------------------------------------------------------------------
static ftscps ftscprod[] =
{
  { 0x0000, "Fido" }
, { 0x0001, "Rover" }
, { 0x0002, "SEAdog" }
, { 0x0003, "WinDog" }
, { 0x0004, "Slick-150" }
, { 0x0005, "Opus" }
, { 0x0006, "Dutchie" }
, { 0x0007, "WPL Library" }
, { 0x0008, "Tabby" }
, { 0x0009, "SWMail" }
, { 0x000A, "Wolf-68k" }
, { 0x000B, "QMM" }
, { 0x000C, "FrontDoor" }
, { 0x000D, "GOmail" }
, { 0x000E, "FFGate" }
, { 0x000F, "FileMgr" }
, { 0x0010, "FIDZERCP" }
, { 0x0011, "MailMan" }
, { 0x0012, "OOPS" }
, { 0x0013, "GS-Point" }
, { 0x0014, "BGMail" }
, { 0x0015, "ComMotion/2" }
, { 0x0016, "OurBBS Fidomailer" }
, { 0x0017, "FidoPcb" }
, { 0x0018, "WimpLink" }
, { 0x0019, "BinkScan" }
, { 0x001A, "D'Bridge" }
, { 0x001B, "BinkleyTerm" }
, { 0x001C, "Yankee" }
, { 0x001D, "uuGate" }
, { 0x001E, "Daisy" }
, { 0x001F, "Polar Bear" }
, { 0x0020, "The-Box" }
, { 0x0021, "STARgate/2" }
, { 0x0022, "TMail" }
, { 0x0023, "TCOMMail" }
, { 0x0024, "Bananna" }
, { 0x0025, "RBBSMail" }
, { 0x0026, "Apple-Netmail" }
, { 0x0027, "Chameleon" }
, { 0x0028, "Majik Board" }
, { 0x0029, "QM" }
, { 0x002A, "Point And Click" }
, { 0x002B, "Aurora Three Bundler" }
, { 0x002C, "FourDog" }
, { 0x002D, "MSG-PACK" }
, { 0x002E, "AMAX" }
, { 0x002F, "Domain Communication System" }
, { 0x0030, "LesRobot" }
, { 0x0031, "Rose" }
, { 0x0032, "Paragon" }
, { 0x0033, "BinkleyTerm/oMMM/ST" }
, { 0x0034, "StarNet" }
, { 0x0035, "ZzyZx" }
, { 0x0036, "QEcho" }
, { 0x0037, "BOOM" }
, { 0x0038, "PBBS" }
, { 0x0039, "TrapDoor" }
, { 0x003A, "Welmat" }
, { 0x003B, "NetGate" }
, { 0x003C, "Odie" }
, { 0x003D, "Quick Gimme" }
, { 0x003E, "dbLink" }
, { 0x003F, "TosScan" }
, { 0x0040, "Beagle" }
, { 0x0041, "Igor" }
, { 0x0042, "TIMS" }
, { 0x0043, "Phoenix" }
, { 0x0044, "FrontDoor APX" }
, { 0x0045, "XRS" }
, { 0x0046, "Juliet Mail System" }
, { 0x0047, "Jabberwocky" }
, { 0x0048, "XST" }
, { 0x0049, "MailStorm" }
, { 0x004A, "BIX-Mail" }
, { 0x004B, "IMAIL" }
, { 0x004C, "FTNGate" }
, { 0x004D, "RealMail" }
, { 0x004E, "Lora-CBIS" }
, { 0x004F, "TDCS" }
, { 0x0050, "InterMail" }
, { 0x0051, "RFD" }
, { 0x0052, "Yuppie!" }
, { 0x0053, "EMMA" }
, { 0x0054, "QBoxMail" }
, { 0x0055, "Number 4" }
, { 0x0056, "Number 5" }
, { 0x0057, "GSBBS" }
, { 0x0058, "Merlin" }
, { 0x0059, "TPCS" }
, { 0x005A, "Raid" }
, { 0x005B, "Outpost" }
, { 0x005C, "Nizze" }
, { 0x005D, "Armadillo" }
, { 0x005E, "rfmail" }
, { 0x005F, "Msgtoss" }
, { 0x0060, "InfoTex" }
, { 0x0061, "GEcho" }
, { 0x0062, "CDEhost" }
, { 0x0063, "Pktize" }
, { 0x0064, "PC-RAIN" }
, { 0x0065, "Truffle" }
, { 0x0066, "Foozle" }
, { 0x0067, "White Pointer" }
, { 0x0068, "GateWorks" }
, { 0x0069, "Portal of Power" }
, { 0x006A, "MacWoof" }
, { 0x006B, "Mosaic" }
, { 0x006C, "TPBEcho" }
, { 0x006D, "HandyMail" }
, { 0x006E, "EchoSmith" }
, { 0x006F, "FileHost" }
, { 0x0070, "SFTS" }
, { 0x0071, "Benjamin" }
, { 0x0072, "RiBBS" }
, { 0x0073, "MP" }
, { 0x0074, "Ping" }
, { 0x0075, "Door2Europe" }
, { 0x0076, "SWIFT" }
, { 0x0077, "WMAIL" }
, { 0x0078, "RATS" }
, { 0x0079, "Harry the Dirty Dog" }
, { 0x007A, "Maximus-CBCS" }
, { 0x007B, "SwifEcho" }
, { 0x007C, "GCChost" }
, { 0x007D, "RPX-Mail" }
, { 0x007E, "Tosser" }
, { 0x007F, "TCL" }
, { 0x0080, "MsgTrack" }
, { 0x0081, "FMail - Hey that's me! ;)" }
, { 0x0082, "Scantoss" }
, { 0x0083, "Point Manager" }
, { 0x0084, "IMBINK" }
, { 0x0085, "Simplex" }
, { 0x0086, "UMTP" }
, { 0x0087, "Indaba" }
, { 0x0088, "Echomail Engine" }
, { 0x0089, "DragonMail" }
, { 0x008A, "Prox" }
, { 0x008B, "Tick" }
, { 0x008C, "RA-Echo" }
, { 0x008D, "TrapToss" }
, { 0x008E, "Babel" }
, { 0x008F, "UMS" }
, { 0x0090, "RWMail" }
, { 0x0091, "WildMail" }
, { 0x0092, "AlMAIL" }
, { 0x0093, "XCS" }
, { 0x0094, "Fone-Link" }
, { 0x0095, "Dogfight" }
, { 0x0096, "Ascan" }
, { 0x0097, "FastMail" }
, { 0x0098, "DoorMan" }
, { 0x0099, "PhaedoZap" }
, { 0x009A, "SCREAM" }
, { 0x009B, "MoonMail" }
, { 0x009C, "Backdoor" }
, { 0x009D, "MailLink" }
, { 0x009E, "Mail Manager" }
, { 0x009F, "Black Star" }
, { 0x00A0, "Bermuda" }
, { 0x00A1, "PT" }
, { 0x00A2, "UltiMail" }
, { 0x00A3, "GMD" }
, { 0x00A4, "FreeMail" }
, { 0x00A5, "Meliora" }
, { 0x00A6, "Foodo" }
, { 0x00A7, "MSBBS" }
, { 0x00A8, "Boston BBS" }
, { 0x00A9, "XenoMail" }
, { 0x00AA, "XenoLink" }
, { 0x00AB, "ObjectMatrix" }
, { 0x00AC, "Milquetoast" }
, { 0x00AD, "PipBase" }
, { 0x00AE, "EzyMail" }
, { 0x00AF, "FastEcho" }
, { 0x00B0, "IOS" }
, { 0x00B1, "Communique" }
, { 0x00B2, "PointMail" }
, { 0x00B3, "Harvey's Robot" }
, { 0x00B4, "2daPoint" }
, { 0x00B5, "CommLink" }
, { 0x00B6, "fronttoss" }
, { 0x00B7, "SysopPoint" }
, { 0x00B8, "PTMAIL" }
, { 0x00B9, "MHS" }
, { 0x00BA, "DLGMail" }
, { 0x00BB, "GatePrep" }
, { 0x00BC, "Spoint" }
, { 0x00BD, "TurboMail" }
, { 0x00BE, "FXMAIL" }
, { 0x00BF, "NextBBS" }
, { 0x00C0, "EchoToss" }
, { 0x00C1, "SilverBox" }
, { 0x00C2, "MBMail" }
, { 0x00C3, "SkyFreq" }
, { 0x00C4, "ProMailer" }
, { 0x00C5, "Mega Mail" }
, { 0x00C6, "YaBom" }
, { 0x00C7, "TachEcho" }
, { 0x00C8, "XAP" }
, { 0x00C9, "EZMAIL" }
, { 0x00CA, "Arc-Binkley" }
, { 0x00CB, "Roser" }
, { 0x00CC, "UU2" }
, { 0x00CD, "NMS" }
, { 0x00CE, "BBCSCAN" }
, { 0x00CF, "XBBS" }
, { 0x00D0, "LoTek Vzrul" }
, { 0x00D1, "Private Point Project" }
, { 0x00D2, "NoSnail" }
, { 0x00D3, "SmlNet" }
, { 0x00D4, "STIR" }
, { 0x00D5, "RiscBBS" }
, { 0x00D6, "Hercules" }
, { 0x00D7, "AMPRGATE" }
, { 0x00D8, "BinkEMSI" }
, { 0x00D9, "EditMsg" }
, { 0x00DA, "Roof" }
, { 0x00DB, "QwkPkt" }
, { 0x00DC, "MARISCAN" }
, { 0x00DD, "NewsFlash" }
, { 0x00DE, "Paradise" }
, { 0x00DF, "DogMatic-ACB" }
, { 0x00E0, "T-Mail" }
, { 0x00E1, "JetMail" }
, { 0x00E2, "MainDoor" }
, { 0x00E3, "StarGate" }
, { 0x00E4, "BMB" }
, { 0x00E5, "BNP" }
, { 0x00E6, "MailMaster" }
, { 0x00E7, "Mail Manager +Plus+" }
, { 0x00E8, "BloufGate" }
, { 0x00E9, "CrossPoint" }
, { 0x00EA, "DeltaEcho" }
, { 0x00EB, "ALLFIX" }
, { 0x00EC, "NetWay" }
, { 0x00ED, "MARSmail" }
, { 0x00EE, "ITRACK" }
, { 0x00EF, "GateUtil" }
, { 0x00F0, "Bert" }
, { 0x00F1, "Techno" }
, { 0x00F2, "AutoMail" }
, { 0x00F3, "April" }
, { 0x00F4, "Amanda" }
, { 0x00F5, "NmFwd" }
, { 0x00F6, "FileScan" }
, { 0x00F7, "FredMail" }
, { 0x00F8, "TP Kom" }
, { 0x00F9, "FidoZerb" }
, { 0x00FA, "!!MessageBase" }
, { 0x00FB, "EMFido" }
, { 0x00FC, "GS-Toss" }
, { 0x00FD, "QWKDoor" }
, { 0x00FE, "No product id allocated" }
, { 0x00FF, "16-bit product id" }
, { 0x0100, "Reserved" }
, { 0x0101, "The Brake!" }
, { 0x0102, "Zeus BBS" }
, { 0x0103, "XenoPhobe-Mailer" }
, { 0x0104, "None" }
, { 0x0105, "Terminate" }
, { 0x0106, "TeleMail" }
, { 0x0107, "CMBBS" }
, { 0x0108, "Shuttle" }
, { 0x0109, "Quater" }
, { 0x010A, "Windo" }
, { 0x010B, "Xenia" }
, { 0x010C, "GMS" }
, { 0x010D, "HNET" }
, { 0x010E, "Shotgun Professional" }
, { 0x010F, "SLIPgate" }
, { 0x0110, "BBBS" }
, { 0x0111, "NewsGate" }
, { 0x01FF, "BBBS" }
, { 0x02FF, "NewsGate" }
, { 0x03FF, "Ravel" }
, { 0x04FF, "Beemail" }
, { 0x05FF, "QuickToss" }
, { 0x06FF, "SpaceMail" }
, { 0x07FF, "Argus" }
, { 0x08FF, "Hurricane" }
, { 0x09FF, "Hub Mailer" }
, { 0x0AFF, "FDInt" }
, { 0x0BFF, "GPMail" }
, { 0x0CFF, "FTrack" }
, { 0x0DFF, "Nice Tosser" }
, { 0x0EFF, "LuckyGate" }
, { 0x0FFF, "McMail" }
, { 0x10FF, "HPT" }
, { 0x11FF, "MBSEBBS" }
, { 0x12FF, "SBBSecho" }
, { 0x13FF, "binkd" }
, { 0x14FF, "Mail-ennium/32" }
, { 0x15FF, "Radius" }
, { 0x16FF, "RNtrack" }
, { 0x17FF, "Msg2Pkt" }
, { 0x18FF, "Marena" }
, { 0x19FF, "jNode" }
, { 0x1AFF, "mfreq" }
, { 0x1BFF, "AfterShock" }
, { 0x1CFF, "FTN::Packet" }
, { 0x1DFF, "WWIV_BBS" }
};
//---------------------------------------------------------------------------
void GetTable(void)
{
  if (*config.ftscProdFile)
  {
    tempStrType fname;
    char *fn;
    int f;

    if (NULL == strpbrk(config.ftscProdFile, ":\\/"))  // configuration filename doesn't contain path chars
    {
      // Look for ftscprod file in default configPath
      strcpy(stpcpy(fname, configPath), config.ftscProdFile);
      fn = fname;
    }
    else
      fn = config.ftscProdFile;

    if ((f = open(fn, O_RDONLY | O_BINARY)) >= 0)
    {
      char *buf;
      off_t fl = fileLength(f);

#ifdef _DEBUG0
      logEntry("DEBUG GetTable opened", LOG_DEBUG, 0);
#endif // _DEBUG

      if ((buf = malloc(fl + 1)) == NULL)
        logEntry(dMEM_ERR_STR, LOG_ALWAYS, 2);

      if (read(f, buf, fl) == fl)
      {
        char *pbuf = buf;
        unsigned long pc;
        int p;

#ifdef _DEBUG0
        logEntry("DEBUG GetTable read", LOG_DEBUG, 0);
#endif // _DEBUG

        if ((fileTbl = calloc(dTBLSIZE, sizeof(ftscps))) == NULL)
          logEntry(dMEM_ERR_STR, LOG_ALWAYS, 2);

        buf[fl] = 0;  // Make sure str(c)spn functions don't overshoot buffer end.
        for (tSize = 0; tSize < dTBLSIZE && pbuf < buf + fl;)
        {
          // Get product code
          pc = strtoul(pbuf, &pbuf, 16);
          if (pc <= UINT16_MAX && *pbuf == ',')
          {
            fileTbl[tSize].prodcode = pc;
            // Get product name
            p = strcspn(++pbuf, ",\r\n");
            if (p > 0)
            {
              if ((fileTbl[tSize].prodname = malloc(p + 1)) == NULL)
                logEntry(dMEM_ERR_STR, LOG_ALWAYS, 2);

              memcpy(fileTbl[tSize].prodname, pbuf, p);
              fileTbl[tSize].prodname[p] = 0;
              pbuf += p;
#ifdef _DEBUG0
              logEntryf(LOG_DEBUG, 0, "DEBUG ftscprod %3d %04X,%s", tSize, fileTbl[tSize].prodcode, fileTbl[tSize].prodname);
#endif // _DEBUG
              tSize++;
            }
          }
#ifdef _DEBUG0
          else
            logEntryf(LOG_DEBUG, 0, "DEBUG GetTable %lu %u", pc, (unsigned int)*pbuf);
#endif // _DEBUG
          pbuf += strcspn(pbuf, "\r\n");  // Skip unused extra text in current line
          pbuf += strspn (pbuf, "\r\n");  // Skip line ending chars
          while (*pbuf == 0 && pbuf < buf + fl)
            ++pbuf;
        }
        tbl = fileTbl;
        logEntryf(LOG_ALWAYS, 0, "Using ftscprod file: %s", fn);
      }
      else
        logEntryf(LOG_ALWAYS, 0, "Problem reading ftscprod file: %s [%s]", config.ftscProdFile, strError(errno));

      close(f);
      free(buf);
    }
    else
      logEntryf(LOG_ALWAYS, 0, "Problem opening ftscprod file: %s [%s]", config.ftscProdFile, strError(errno));
  }
  if (NULL == tbl)
  {
#ifdef _DEBUG
    logEntry("DEBUG Using internal ftsc product table", LOG_DEBUG, 0);
#endif
    tbl = ftscprod;
    tSize = sizeof(ftscprod) / sizeof(ftscps);
  }
#ifdef _DEBUG0
  logEntryf(LOG_DEBUG, 0, "DEBUG ftscprod table size: %d", tSize);
#endif
}
//---------------------------------------------------------------------------
const char *GetFtscProdStr(u16 pcode)
{
  const char *helpPtr = NULL;
  int L
    , R
    , m
    , r;

  if (NULL == tbl)
    GetTable();

  // binary search for the product code in the array
  L = 0;
  R = tSize - 1;
  for (;;)
  {
    if (L > R)
      break;   // Not found

    m = (L + R) / 2;
    if ((r = pcode - tbl[m].prodcode) == 0)
    {
      // Found
      helpPtr = tbl[m].prodname;
      break;
    }
    if (r > 0)
      L = m + 1;
    else
      R = m - 1;
  }

  if (helpPtr == NULL || *helpPtr == 0)
  {
    static char prodCodeStr[20] = "Program id ";

    sprintf(prodCodeStr + 11, "%04hX", pcode);
    helpPtr = prodCodeStr;
  }

  return helpPtr;
}
//---------------------------------------------------------------------------
void freeFtscTable(void)
{
  if (fileTbl != NULL)
  {
    int i;

    for (i = 0; i < tSize; i++)
      free(fileTbl[i].prodname);

    free(fileTbl);
    tbl = fileTbl = NULL;
  }
}
//---------------------------------------------------------------------------
