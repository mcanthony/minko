@echo off

pushd ..\..\..
tool\win\bin\premake5.exe --no-test --no-framework --no-plugin vs2013
popd
pause
