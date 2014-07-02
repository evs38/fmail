2014-07-01

Release of FMailW32 1.68.10.91 (GPL)


2014-03-21

Release of FMailW32 1.67 (GPL)


2013-10-15

Release of FMailW32 1.66 (GPL)


2011-09-11

FToolsW32 is now also compiled with C++ Builder 6. Watch the name change from:
FToolsW3.exe to: FToolsW32.exe. So either adapt your batch files or rename it
back! You should be carefull when you use this version of FTools, make sure you
make backups of your message base files before you use it. I didn't test every
function of FTools, just the ones that I use on my system:

 MAINT /D /N /O /P
 MSGM -net -sent -rcvd /N
 Notify /A
 Post ... <to JAM area>

 So be extra carefull when you use other functions of FTools!


2011-09-03

This release of FMailW32 1.64.GPL should be regarded as a Beta!

There are 2 important changes from the previous release:

1) FMailW32.exe is now build with Borland C++ Builder 6, which is a bit more
modern then Borland C++ 4.52 (1995 vs 2002). The main advantage for development
is that BCB6 is able to debug Win32 programs, which BC45 was not.

2) FMailW32 creates files and directory names in lowercase as much as possible,
so it should function a bit better where files are on a filesystem that is case
sensitive. Like my setup where fmail runs on virtual Windows, and binkd runs on
the linux side of the machine.

For the other tools in the FMail package you should still use those from the
previous release, because they are not converted yet!

Please let me know your experience with this release, in the fidonet FMAIL_HELP
echomail area.


Regards, Wilfred van Velzen (2:280/464)


2008-02-12

This is the opensource release of FMail 1.60.GPL. You will find a copy of the
GPL license as the file COPYING in this release.

FMail was compiled using the Borland C++ 4.52 compiler. In order to compile the
DPMI versions, you need the Borland Power Pack.

If you install Borland C++ in C:\Program fies\BC45\ and extract this archive in
D:\FMail, you should be able to open the IDE file and compile FMail.

If you have questions, please post them on the Sourceforge project space:
http://sourceforge.net/projects/fmail/

Or try your luck by contacting Folkert at the e-mail address listed on
http://fmail.nl.eu.org

Folkert J. Wijnstra  &  Wilfred van Velzen

