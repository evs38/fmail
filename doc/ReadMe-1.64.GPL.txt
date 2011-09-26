2011-09-11

FToolsW32 is now also compiled with C++ Builder 6.
Watch the name change from: FToolsW3.exe to: FToolsW32.exe. So either adapt your batch files or rename it back!
You should be carefull when you use this version of FTools, make sure you make backups of your message base files before you use it.
I didn't test every function of FTools, just the ones that I use on my system:

 MAINT /D /N /O /P
 MSGM -net -sent -rcvd /N
 Notify /A
 Post ... <to JAM area>

 So be extra carefull when you use other functions of FTools!
 
2011-09-03

This release of FMailW32 1.64.GPL should be regarded as a Beta!

There are 2 important changes from the previous release:

1) FMailW32.exe is now build with Borland C++ Builder 6, which is a bit 
more modern then Borland C++ 4.52 (1995 vs 2002). The main advantage for 
development is that BCB6 is able to debug Win32 programs, which BC45 was 
not. 

2) FMailW32 creates files and directory names in lowercase as much as 
possible, so it should function a bit better where files are on a 
filesystem that is case sensitive. Like my setup where fmail runs on
virtual Windows, and binkd runs on the linux side of the machine.

For the other tools in the FMail package you should still use those from 
the previous release, because they are not converted yet!

Please let me know your experience with this release, in the fidonet FMAIL_HELP echomail area. 


Regards, Wilfred van Velzen (2:280/464)
