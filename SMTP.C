//---------------------------------------------------------------------------
//
//  Copyright (C) 2007        Folkert J. Wijnstra
//  Copyright (C) 2007 - 2015 Wilfred van Velzen
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


#include <dir.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <winsock.h>

#include "fmail.h"

#include "areainfo.h"
#include "log.h"
#include "smtp.h"
#include "utils.h"


// ALGEMENE VARIABELEN ========================================================


static	SOCKET	ws;
static  char    *sPartBoundary = "~---~fmAIL_bOUNDARY~---~";
static  int     connOpen;



// BUFFERFUNCTIES =============================================================


#define         BUFFERSIZE 8192

static  char    sbuffer[BUFFERSIZE + 1];
static  int     sbufcount, sbuferror;


static void initsbuf(void)
{
   sbufcount = sbuferror = 0;
}


static int flushsbuf(void)
{
   if ( !sbufcount )
      return 0;
   if ( send(ws, sbuffer, sbufcount, 0) == SOCKET_ERROR )
   {  logEntry("Error sending data", LOG_ALWAYS, 0);
      sbufcount = 0;
      return 1;
   }

   sbufcount = 0;
   return sbuferror;
}


static void addsbuf(char *data)
{
   int len;

   if ( (len = strlen(data)) + sbufcount > BUFFERSIZE )
      sbuferror |= flushsbuf();
   strcpy(sbuffer + sbufcount, data);
   sbufcount += len;
}



// RESPONECONTROLE ============================================================


#define GENERIC_SUCCESS  0
#define CONNECT_SUCCESS  1
#define DATA_SUCCESS     2
#define QUIT_SUCCESS     3
// Include any others here
#define LAST_RESPONSE    4
// Do not add entries past this one

// Note: the order of the entries is important.
//       They must be synchronized with eResponse entries.
static int response_code[4] =
{
  250,              // GENERIC_SUCCESS
  220,              // CONNECT_SUCCESS
  354,              // DATA_SUCCESS
  221               // QUIT_SUCCESS
};

static char *response_text[4] =
{
        "SMTP server error",                     // GENERIC_SUCCESS
        "SMTP server not available",             // CONNECT_SUCCESS
        "SMTP server not ready for data",        // DATA_SUCCESS
        "SMTP server didn't terminate session"   // QUIT_SUCCESS
};

#define         RESPONSE_BUFFER_SIZE    1024

static  char    response_buf[RESPONSE_BUFFER_SIZE];


static BOOL getresponse(UINT response_expected)
{
   char        buf[4];
   int         response;
   tempStrType tempStr;

   if ( recv(ws, response_buf, RESPONSE_BUFFER_SIZE, 0) == SOCKET_ERROR )
   {
      logEntry("Socket receive error", LOG_ALWAYS, 0);
      return FALSE;
   }
   strncpy(buf, response_buf, 3);
   buf[3] = 0;
   sscanf(buf, "%d", &response);
   if ( response != response_code[response_expected] )
   {
      sprintf(tempStr, "Expected %d, received %d: %s",
                       response_code[response_expected], response,
                       response_text[response_expected]);
      logEntry(tempStr, LOG_ALWAYS, 0);
      return FALSE;
   }
   return TRUE;
}



// MIME FUNCTIES ==============================================================


// The 7-bit alphabet used to encode binary information
char sBase64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int  nMask[] = { 0, 1, 3, 7, 15, 31, 63, 127, 255 };

#define         BYTES_TO_READ          54
static  char    szInput[BYTES_TO_READ];
static	int	nInputSize, nBitsRemaining;
static	int	lBitStorage;


int read_bits(int nNumBits, int *pBitsRead, int *lp)
{
   long lScratch;

   while( (nBitsRemaining < nNumBits) && (*lp < nInputSize) )
   {  int c = szInput[ (*lp)++ ];
      lBitStorage <<= 8;
      lBitStorage |= (c & 0xff);
      nBitsRemaining += 8;
   }
   if( nBitsRemaining < nNumBits )
   {  lScratch = lBitStorage << (nNumBits - nBitsRemaining);
      *pBitsRead = nBitsRemaining;
      nBitsRemaining = 0;
   }
   else
   {  lScratch = lBitStorage >> (nBitsRemaining - nNumBits);
      *pBitsRead = nNumBits;
      nBitsRemaining -= nNumBits;
   }
   return (int)lScratch & nMask[nNumBits];
}


static void encode(char *szEncoding, int nSize)
{
   char	*helpPtr;
   int nNumBits = 6;
   int nDigit;
   int lp = 0;
   int xu;
   tempStrType sbuf;

   *sbuf = 0;
   helpPtr = sbuf;
   xu = 0;
   if ( szEncoding == NULL )
      return;
   memcpy(szInput, szEncoding, nInputSize = nSize);
   szInput[nInputSize] = 0;

   nBitsRemaining = 0;
   nDigit = read_bits( nNumBits, &nNumBits, &lp);
   while( nNumBits > 0 )
   {  *helpPtr++ = sBase64Alphabet[nDigit];
      ++xu;
      nDigit = read_bits(nNumBits, &nNumBits, &lp);
   }
   // Pad with '=' as per RFC 1521
   while( (xu % 4) != 0 )
   {
      *helpPtr++ = '=';
      ++xu;
   }
   *helpPtr = 0;
   addsbuf(sbuf);
}


static void appendMIMEfile(char *fname)
{
   int	handle;
   int  nBytesRead;
   char szFName[32], szExt[32];
   char szBuffer[BYTES_TO_READ + 1];
   tempStrType sbuf;

   if ( !fname || !*fname )
      return;
   if ( (handle = open(fname, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) == -1 )
      return;
   fnsplit(fname, NULL, NULL, szFName, szExt);
   sprintf(sbuf, "--%s\r\n", sPartBoundary);
   addsbuf(sbuf);
   sprintf(sbuf, "Content-Type: application/octet-stream; name=%s%s\r\n", szFName, szExt);
   addsbuf(sbuf);
   sprintf(sbuf, "Content-Transfer-Encoding: base64\r\n");
   addsbuf(sbuf);
   sprintf(sbuf, "Content-Disposition: attachment; filename=%s%s\r\n\r\n", szFName, szExt );
   addsbuf(sbuf);

   do
   {  nBytesRead = read(handle, szBuffer, BYTES_TO_READ);
      szBuffer[nBytesRead] = 0;     // Terminate the string
      encode(szBuffer, nBytesRead);
      addsbuf("\r\n");
   } while( nBytesRead == BYTES_TO_READ );
   addsbuf("\r\n");

   close(handle);
}


// TEKST FILE FUNCTIE =========================================================

static void appendtext(char *txt)
{
   int nlcount, txtlen, startpos, spacepos, xu;
   tempStrType sbuf;

   if ( !txt || !*txt )
      return;

   sprintf(sbuf, "--%s\r\n", sPartBoundary);
   addsbuf(sbuf);
   sprintf(sbuf, "Content-Type: text/plain\r\n");
   addsbuf(sbuf);
   sprintf(sbuf, "Content-Transfer-Encoding: 7Bit\r\n\r\n");
   addsbuf(sbuf);

   txtlen = strlen(txt);
   xu = nlcount = spacepos = startpos = 0;
   while( xu < txtlen )
   {  if ( txt[xu] == ' ' )
	 spacepos = xu;
      if ( txt[xu] == '\n' )
      {  *(txt + xu) = 0;
	 addsbuf(txt + startpos);
	 if ( startpos + 1 == xu && *(txt + startpos) == '.' )
	    addsbuf(" ");
         addsbuf("\r\n");
         *(txt + xu) = '\n';
         startpos = xu + 1;
	 nlcount = spacepos = 0;
      }
      if ( nlcount > 64 && spacepos )
      {  *(txt + spacepos) = 0;
         addsbuf(txt + startpos);
	 *(txt + spacepos) = ' ';
         addsbuf("\r\n");
	 startpos = spacepos + 1;
	 nlcount =  spacepos = 0;
      }
      else
	 nlcount++;
      xu++;
   }
   addsbuf(txt + startpos);
   addsbuf("\r\n");
}



// MESSAGE FUNCTIES ===========================================================

static  WSADATA wsdata;

static  char    *weekday[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static  char    *month[12]  = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static int openConnection(u8 *SMTPServerName)
{
   tempStrType  hostnamebuf, buf;
   SOCKADDR_IN  sockaddrL, sockaddrR;
   HOSTENT     	*hostent;
   SERVENT     	*servent;

   if ( WSAStartup(2, &wsdata) )
   {  logEntry("WinSock startup error", LOG_ALWAYS, 0);
      return 1;
   }
   if ( (ws = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
   {  logEntry("Socket initialization error", LOG_ALWAYS, 0);
      return 1;
   }
   memset(&sockaddrL, 0, sizeof(sockaddrL));
   sockaddrL.sin_family = AF_INET;
   sockaddrL.sin_port = 0;
   sockaddrL.sin_addr.s_addr = INADDR_ANY;
   if ( bind(ws, (SOCKADDR *)&sockaddrL, sizeof(sockaddrL)) )
   {  logEntry("Socket bind error", LOG_ALWAYS, 0);
      goto einde;
   }
   if ( (hostent = gethostbyname(SMTPServerName)) == NULL )
   {  sprintf(buf, "Hostname not found: %s", SMTPServerName);
      logEntry(buf, LOG_ALWAYS, 0);
      goto einde;
   }
   if ( (servent = getservbyname("smtp", "tcp")) == NULL )
   {  logEntry("Service SMTP(TCP) not found", LOG_ALWAYS, 0);
      goto einde;
   }

   sprintf(buf, "Connecting to %s (%u.%u.%u.%u) on port %u (SMTP)",
		 SMTPServerName,
		 (int)((struct in_addr*)(hostent->h_addr_list[0]))->S_un.S_un_b.s_b1,
		 (int)((struct in_addr*)(hostent->h_addr_list[0]))->S_un.S_un_b.s_b2,
		 (int)((struct in_addr*)(hostent->h_addr_list[0]))->S_un.S_un_b.s_b3,
		 (int)((struct in_addr*)(hostent->h_addr_list[0]))->S_un.S_un_b.s_b4,
		 (int)(((servent->s_port & 0xFF) << 8) + ((servent->s_port & 0xFF00) >> 8)));
   logEntry(buf, LOG_ALWAYS, 0);

   memset(&sockaddrR, 0, sizeof(sockaddrR));
   sockaddrR.sin_family = AF_INET;
   sockaddrR.sin_port = servent->s_port;
   memcpy((char FAR *)&(sockaddrR.sin_addr), hostent->h_addr_list[0], hostent->h_length);
   if ( connect(ws, (SOCKADDR *)&sockaddrR, sizeof(sockaddrR)) )
   {  logEntry("Can't connect to host", LOG_ALWAYS, 0);
      goto einde;
   }
   if( !getresponse(CONNECT_SUCCESS) )
   {  logEntry("Can't connect to host", LOG_ALWAYS, 0);
      goto einde;
   }
   if ( gethostname(hostnamebuf, sizeof(tempStrType)) )
   {  logEntry("Can't get local host name", LOG_ALWAYS, 0);
einde:
      closesocket(ws);
      WSACleanup();
      return 1;
   }
   sprintf(buf, "HELO %s\r\n", hostnamebuf);
   if ( send(ws, buf, strlen(buf), 0) == SOCKET_ERROR )
   {   logEntry("Send HELO error", LOG_ALWAYS, 0);
       goto einde;
   }
   if ( !getresponse(GENERIC_SUCCESS) )
   {  logEntry("No HELO success reply", LOG_ALWAYS, 0);
      goto einde;
   }
   return 0;
}


int sendMessage(u8 *SMTPServerName, u8 *mailfrom, u8 *mailto, internalMsgType *message, u8 *attach)
{
   u8           *fileName;
   tempStrType  sbuf;
   time_t	timer;
   struct tm    *tms;
   int		error;

   if ( !connOpen )
      if ( !(connOpen = !openConnection(SMTPServerName)) )
      {  newLine();
         return 1;
      }
   if ( message->destNode.zone )
      sprintf(sbuf, "Sending mail for node %s to %s", nodeStr(&message->destNode), mailto);
   else
      sprintf(sbuf, "Sending mail to %s", mailto);
   logEntry(sbuf, LOG_ALWAYS, 0);
   error = 0;
   sprintf(sbuf, "MAIL FROM: <%s>\r\n", mailfrom);
   if ( send(ws, sbuf, strlen(sbuf), 0) == SOCKET_ERROR )
   {   logEntry("Send MAIL FROM: error", LOG_ALWAYS, 0);
       ++error;
       goto einde1;
   }
   if ( !getresponse(GENERIC_SUCCESS) )
   {  logEntry("No MAIL FROM: success reply", LOG_ALWAYS, 0);
      ++error;
      goto einde1;
   }
   sprintf(sbuf, "RCPT TO: <%s>\r\n", mailto);
   if ( send(ws, sbuf, strlen(sbuf), 0) == SOCKET_ERROR )
   {   logEntry("Send RCPT TO: error", LOG_ALWAYS, 0);
       ++error;
       goto einde1;
   }
   if ( !getresponse(GENERIC_SUCCESS) )
   {  logEntry("No RCPT TO: success reply", LOG_ALWAYS, 0);
      ++error;
      goto einde1;
   }
   if ( send(ws, "DATA\r\n", 6, 0) == SOCKET_ERROR )
   {   logEntry("Send DATA error", LOG_ALWAYS, 0);
      ++error;
       goto einde1;
   }
   if ( !getresponse(DATA_SUCCESS) )
   {  logEntry("No DATA success reply", LOG_ALWAYS, 0);
      ++error;
      goto einde1;
   }

   initsbuf();
   sprintf(sbuf, "X-Mailer: %s\r\n", smtpID);
   addsbuf(sbuf);
   time(&timer);
   tms = gmtime(&timer);
   sprintf(sbuf, "Date: %s, %02u %s %u %02u:%02u:%02u GMT\r\n",
		 weekday[tms->tm_wday], tms->tm_mday,
                 month[tms->tm_mon], tms->tm_year+1900,
		 tms->tm_hour, tms->tm_min, tms->tm_sec);
   addsbuf(sbuf);
   if ( *message->fromUserName )
      sprintf(sbuf, "From: \"%s\" <%s>\r\n", message->fromUserName, mailfrom);
   else
      sprintf(sbuf, "From: %s\r\n", mailfrom);
   addsbuf(sbuf);
   if ( *message->toUserName )
      sprintf(sbuf, "To: \"%s\" <%s>\r\n", message->toUserName, mailto);
   else
      sprintf(sbuf, "To: %s\r\n", mailto);
   addsbuf(sbuf);

   sprintf(sbuf, "Subject: %s\r\n", message->subject);
   addsbuf(sbuf);

   sprintf(sbuf, "MIME-Version: 1.0\r\n");
   addsbuf(sbuf);
   sprintf(sbuf, "Content-Type: multipart/mixed; boundary=%s\r\n\r\n", sPartBoundary);
   addsbuf(sbuf);

   strcpy(sbuf, "This is a multi-part message in MIME format.\r\n\r\n");
   addsbuf(sbuf);

   appendtext(message->text);
   if ( firstFile(attach, &fileName) )
   {  do
      {  appendMIMEfile(fileName);
      }  while ( nextFile(&fileName) );
   }

   // Add message termination
   sprintf(sbuf, "--%s--\r\n\r\n.\r\n", sPartBoundary);
   addsbuf(sbuf);
   flushsbuf();

   if ( !getresponse(GENERIC_SUCCESS) )
   {  logEntry("No EODATA success reply", LOG_ALWAYS, 0);
      ++error;
      goto einde1;
   }
einde1:
   return error;
}


int closeConnection(void)
{
   int error;

   if ( !connOpen )
      return 1;

   error = 0;
   if ( send(ws, "QUIT\r\n", 6, 0) == SOCKET_ERROR )
   {  logEntry("Send QUIT error", LOG_ALWAYS, 0);
      ++error;
      goto einde1;
   }
   if ( !getresponse(QUIT_SUCCESS) )
   {  logEntry("No QUIT success reply", LOG_ALWAYS, 0);
      ++error;
      goto einde1;
   }
einde1:
   connOpen = 0;
   shutdown(ws, 2);
   closesocket(ws);
   logEntry("SMTP connection closed", LOG_ALWAYS, 0);
   newLine();
   WSACleanup();
   return error;
}

