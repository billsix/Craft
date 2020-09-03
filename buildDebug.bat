mkdir debugBuild
mkdir debugBuildInstall
cd debugBuild
REM create the visual studio solution and project files
cmake -DCMAKE_INSTALL_PREFIX=..\debugBuildInstall ..\
cmake --build . --target ALL_BUILD --config Debug
cmake --build . --target INSTALL 
cd ..
