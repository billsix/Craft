mkdir releaseBuild
mkdir releaseBuildInstall
cd releaseBuild
REM create the visual studio solution and project files
cmake -DCMAKE_INSTALL_PREFIX=..\releaseBuildInstall -DCMAKE_BUILD_TYPE=Release ..\
cmake --build . --config Release
cmake --build . --config Release --target INSTALL 
cd ..
