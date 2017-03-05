//---------------------------------------------------------------------------
//
//  Copyright (C) 2017 Wilfred van Velzen
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __WIN32__
# include "io.h"
# include "winsock2.h"      // WSAGetLastError, WSAStartUp
# define snprintf _snprintf
#endif

#ifdef __linux__
# include <netdb.h>         // gethostbyname
# include <netinet/in.h>    // htons
# include <sys/socket.h>
# include <unistd.h>        // close()
#endif

#include "fmail.h"

#include "stpcpy.h"
#include "utils.h"
#include "version.h"
#include "window.h"

extern const char *mailerName[6];

//---------------------------------------------------------------------------
const char *sendMsg(const char *msg)
{
  static char buf[0x2000];
  int sock
    , i;
  struct hostent *host;
  struct sockaddr_in saddr_in;
  char h1[] = { 65, 48,  5, 103, 28, 84, 87, 22, 20, 41,   9, 20, 16, 106, 52, 0 }   // obfuscate hostname in source
     , h2[] = { 37, 61, 92,   2, 80, 20, 18, 24, 98, 67, 113, 90, 30,   4, 56, 0 };
#ifdef __WIN32__
  WSADATA wsaData;
  *buf = 0;
  if (WSAStartup(0x0202, &wsaData) != 0)
    return strcpy(buf, " * Internet not available * ");
#else
  *buf = 0;
#endif
  // Decode
  for (i = 0; i < sizeof(h1); i++)
    h1[i] += h2[i];

  if (  (sock = socket(AF_INET, SOCK_STREAM, 0)) == -1
     || (host = gethostbyname(h1)) == NULL
     )
    strcpy(buf, " * Service not available * ");
  else
  {
    saddr_in.sin_family      = AF_INET;
    saddr_in.sin_port        = htons(63335);
    saddr_in.sin_addr.s_addr = 0;

    memcpy((char*)&(saddr_in.sin_addr), host->h_addr, host->h_length);

    if (connect(sock, (struct sockaddr*)&saddr_in, sizeof(saddr_in)) == 0)
    {
      char *bp;

      bp  = buf;
      bp  = stpcpy (bp, "HELO FMAILHI\n");
      bp += sprintf(bp, "VERS: %s\n", VersionStr());
      bp += sprintf(bp, "TIME: %s\n", isoFmtTime(time(NULL)));
      bp += sprintf(bp, "SYSO: %s\n", config.sysopName);
      bp += sprintf(bp, "MAIL: %s\n", mailerName[config.mailer]);
      for (i = 0; i < MAX_AKAS; i++)
        if (config.akaList[i].nodeNum.zone != 0)
          bp += sprintf(bp, "ADDR: %s\n", nodeStr(&config.akaList[i].nodeNum));

      bp += sprintf(bp, "MESG: %s\n", msg);

      send(sock, buf, bp - buf, 0);
#ifdef __WIN32__
      shutdown(sock, SD_SEND);
#endif // __WIN32__
#ifdef __linux__
      shutdown(sock, SHUT_WR);
#endif // __linux__
      i = recv(sock, buf, sizeof(buf) - 1, 0);
      buf[max(i, 0)] = 0;
    }
    else
      strcpy(buf, " * Connect to server failed * ");
  }

#ifdef __WIN32__
  shutdown(sock, SD_BOTH);
  closesocket(sock);
  WSACleanup();
#endif // __WIN32__
#ifdef __linux__
  shutdown(sock, SHUT_RDWR);
  close(sock);
#endif // __linux__
  return buf;
}
//---------------------------------------------------------------------------
s16 sayHi(void)
{
  char *hiStr = getStr(" Message to author ", "Hi!");

  if (*hiStr)
  {
    const char *r;
    printStringFill("Sending message...", ' ', 79, 0, 24, RED, BLACK | COLOR_BLINK, MONO_NORM_BLINK);

    r = sendMsg(hiStr);

    displayMessage(*r ? r : " Message send ");
  }

  return 0;
}
//---------------------------------------------------------------------------
