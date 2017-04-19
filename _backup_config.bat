@echo off
set BCKDST=cfg_back
mkdir %BCKDST%
copy /b fmail.ar    %BCKDST%
copy /b fmail.ard   %BCKDST%
copy /b fmail.bde   %BCKDST%
copy /b fmail.cfg   %BCKDST%
copy /b fmail.nod   %BCKDST%
copy /b fmail.pck   %BCKDST%
copy /b fmail32.dup %BCKDST%
