//----------------------------------------------------------------------------
//
// Copyright (C) 2011  Wilfred van Velzen
//
// This file is part of FMail.
//
// FMail is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// FMail is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//----------------------------------------------------------------------------

#include "Version.h"

#include "pp_date.h"

#include <stdio.h>
#ifdef __WIN32__
#include <windows.h>
#endif

#ifdef __OS2__
	#define PTYPE "/OS2"
	#define P386
#elif defined __WIN32__ && !defined __DPMI32__
	#define PTYPE "-W32"
   #define P386
#elif defined __FMAILX__
	#ifdef __DPMI32__
		#define PTYPE "X32"
		#define P386
	#elif defined __DPMI16__
		#define PTYPE "X"
		#define P386
	#else
		#define PTYPE "W"
		#define P386
	#endif
#elif defined P386
	#define PTYPE "/386"
#else
	#define PTYPE ""
#endif

#ifdef GOLDBASE
#define MBTYPE "G"
#define MBNAME "QuickBBS Gold"
#else
#define MBTYPE
#define MBNAME "Hudson"
#endif

#ifdef BETA
  #define BLSTR "-Beta"
  #define BSSTR "-B"
#else
  #define BLSTR ""
  #define BSSTR ""
#endif  

#if   defined(FSETUP)
  #define TOOL "FSetup"
#elif defined(FTOOLS)
  #define TOOL "FTools"
#else // FMAIL
  #define TOOL "FMail"
#endif  

#define VERSION_STR  TOOL PTYPE MBTYPE"-"VERNUM BLSTR
#define TEARLINE_STR "--- "TOOL PTYPE"-"VERNUM BSSTR
#define TID_STR      TOOL PTYPE"-"VERNUM BSSTR

//----------------------------------------------------------------------------
const char versionStr[] = VERSION_STR;
const char *VersionStr(void)
{
  return versionStr;
}
//----------------------------------------------------------------------------
char tearStr[82] = { 0 };
const char *TearlineStr(void)
{
#ifdef BETA
  if (*tearStr == 0)
    sprintf(tearStr, TEARLINE_STR"%04d%02d%02d\r", YEAR, MONTH + 1, DAY);
#else
  if (*tearStr == 0)
    sprintf(tearStr, TEARLINE_STR"\r");
#endif
  return tearStr;
}
//----------------------------------------------------------------------------
#ifdef BETA
char tidStr[82] = { 0 };
#else
const char tidStr[] = TID_STR;
#endif
const char *TIDStr(void)
{
#ifdef BETA
  if (*tidStr == 0)
    sprintf(tidStr, TID_STR"%04d%02d%02d", YEAR, MONTH + 1, DAY);
#endif
  return tidStr;
}
//----------------------------------------------------------------------------
#ifdef __WIN32__
char *VersionString(void)
{
  static char resultStr[MAX_PATH];
  char        fullPath [MAX_PATH];
  DWORD       dummy;
  DWORD       verInfoSize;

  GetModuleFileName(NULL, fullPath, sizeof(fullPath));
  verInfoSize = GetFileVersionInfoSize(fullPath, &dummy);

  if (verInfoSize == 0)
    strcpy(resultStr, VERNUM);
  else
  {
    unsigned int verValueSize;
    VS_FIXEDFILEINFO *verValue;
    DWORD dwFileVersionMS
        , dwFileVersionLS;
    byte *verInfo = malloc(verInfoSize);
    GetFileVersionInfo(fullPath, 0, verInfoSize, verInfo);
    VerQueryValue(verInfo, "\\", (void**)&verValue, &verValueSize);

    dwFileVersionMS = verValue->dwFileVersionMS;
    dwFileVersionLS = verValue->dwFileVersionLS;

    sprintf(resultStr, "%d.%d.%d.%d", HIWORD(dwFileVersionMS), LOWORD(dwFileVersionMS), HIWORD(dwFileVersionLS), LOWORD(dwFileVersionLS));

    free(verInfo);
  }

  return resultStr;
}
#if 0
char *GetInfo(char *InfoItem)
{
  static char szResult  [MAX_PATH];
  char        szFullPath[MAX_PATH];
  char        szGetName [MAX_PATH];
  LPSTR       lpVersion;        // String pointer to Item text
  DWORD       dwVerInfoSize;    // Size of version information block
  DWORD       dwVerHnd = 0;     // An 'ignored' parameter, always '0'
  UINT        uVersionLen;
  BOOL        bRetCode;

  szResult[0] = 0;
  GetModuleFileName(NULL, szFullPath, sizeof(szFullPath));
  dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd);
  if (dwVerInfoSize)
  {
    LPSTR  lpstrVffInfo;
    HANDLE hMem;

    hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
    lpstrVffInfo = (LPSTR)GlobalLock(hMem);
    GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpstrVffInfo);

    // Get a codepage from base_file_info_sctructure
    lstrcpy(szGetName, "\\VarFileInfo\\Translation");

    uVersionLen = 0;
    lpVersion   = NULL;
    bRetCode    = VerQueryValue((LPVOID)lpstrVffInfo, (LPSTR)szGetName, (void **)&lpVersion, (UINT *)&uVersionLen);
    if (bRetCode && uVersionLen && lpVersion)
      sprintf(szResult, "%04x%04x", (WORD)(*((DWORD *)lpVersion)), (WORD)(*((DWORD *)lpVersion)>>16));
    else
    {
      // 041904b0 is a very common one, because it means:
      //   US English/Russia, Windows MultiLingual characterset
      // Or to pull it all apart:
      // 04------        = SUBLANG_ENGLISH_USA
      // --09----        = LANG_ENGLISH
      // --19----        = LANG_RUSSIA
      // ----04b0 = 1200 = Codepage for Windows:Multilingual
      lstrcpy(szResult, "041904b0");
    }

    // Add a codepage to base_file_info_sctructure
    sprintf(szGetName, "\\StringFileInfo\\%s\\", szResult);
    // Get a specific item
    lstrcat(szGetName, InfoItem);

    uVersionLen = 0;
    lpVersion   = NULL;
    bRetCode    = VerQueryValue((LPVOID)lpstrVffInfo, (LPSTR)szGetName, (void **)&lpVersion, (UINT *)&uVersionLen);
    if (bRetCode && uVersionLen && lpVersion)
      lstrcpy(szResult, lpVersion);
    else
      lstrcpy(szResult, "");
  }
  return szResult;
}
//---------------------------------------------------------------------------
void GetFileVersionOfApplication()
{
   //give your application full path
   LPTSTR lpszFilePath = "D:\\SAN\\MyTest.exe";

   DWORD dwDummy;
   DWORD dwFVISize = GetFileVersionInfoSize( lpszFilePath , &dwDummy );

   LPBYTE lpVersionInfo = new BYTE[dwFVISize];

   GetFileVersionInfo( lpszFilePath , 0 , dwFVISize , lpVersionInfo );

   UINT uLen;
   VS_FIXEDFILEINFO *lpFfi;

   VerQueryValue( lpVersionInfo , _T("\\") , (LPVOID *)&lpFfi , &uLen );

   DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
   DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;

   delete [] lpVersionInfo;

   printf( "Higher: %x\n" , dwFileVersionMS );

   printf( "Lower: %x\n" , dwFileVersionLS );

   DWORD dwLeftMost     = HIWORD(dwFileVersionMS);
   DWORD dwSecondLeft   = LOWORD(dwFileVersionMS);
   DWORD dwSecondRight  = HIWORD(dwFileVersionLS);
   DWORD dwRightMost    = LOWORD(dwFileVersionLS);

   CString str;

   str.Format("Version: %d.%d.%d.%d\n" , dwLeftMost, dwSecondLeft,
      dwSecondRight, dwRightMost);

   AfxMessageBox(str);
}
#endif
#endif
//----------------------------------------------------------------------------
