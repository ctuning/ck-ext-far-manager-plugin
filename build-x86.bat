echo "*** Setting up compiler environment ..."

rem call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86
call env-mvs2013-12-x86.bat

echo "*** Entering src directory ..."

cd src

echo "*** Cleaning files ..."

del *.obj
del fgg.dll
del fgg.lib
del fgg.exp

echo "*** Compiling plugin ..."

cl /c *.cpp
link @..\build-link-flags.inc

echo "*** Copying files to plugin directory ..."
move fgg.dll ..\fggmisc-x86
cp ..\*.txt ..\fggmisc-x86
cp ..\*.md ..\fggmisc-x86
