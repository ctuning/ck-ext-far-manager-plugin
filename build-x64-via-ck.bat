echo "*** Setting up compiler environment ..."

rem call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_amd64
rem call env-mvs2013-12-x64.bat

call ck set env tags=compiler,lang-c bat_file=tmp-ck-env.bat --target_os=windows-64 --bat_new --print && call tmp-ck-env.bat && del /Q tmp-ck-env.bat
if %errorlevel% neq 0 exit /b %errorlevel%

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
move fgg.dll ..\fggmisc-x64
cp ..\*.txt ..\fggmisc-x64
cp ..\*.md ..\fggmisc-x64
