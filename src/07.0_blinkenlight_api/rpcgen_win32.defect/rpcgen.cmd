
rem generate client and server stubs for blinkenlight api

rem program names
set IFNAME=blinkenlight_api

set RPCGEN=..\..\3rdparty\oncrpc_win32\win32\bin\rpcgen.exe
rem -N = new style.
rem set RPCGENFLAGS=-N
set RPCGENFLAGS=


del /q %IFNAME%_xdr.c
del /q %IFNAME%.h
del /q %IFNAME%.clnt.c
del /q %IFNAME%.svc.c

rem rpcgen wants to execute teh C-Compilter "cl.exe"
call "c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat"

@echo + generating interface header and stub sources
copy ..\%IFNAME%.x .
rem xdr file
%RPCGEN% %RPCGENFLAGS% -c -o %IFNAME%_xdr.c %IFNAME%.x

rem header file
%RPCGEN% %RPCGENFLAGS% -h -o %IFNAME%.h %IFNAME%.x

rem client stub
%RPCGEN% %RPCGENFLAGS% -l -o %IFNAME%_clnt.c %IFNAME%.x

rem server stub without main()
%RPCGEN% %RPCGENFLAGS% -m -o %IFNAME%_svc.c %IFNAME%.x



