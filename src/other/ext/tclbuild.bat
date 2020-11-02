set VSINSTALLDIR="C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise"
set DevEnvDir="C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE"
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
nmake -f makefile.vc install INSTALLDIR=%1 SUFX=
