mkdir build
mkdir buildInstall
cd build
REM create the visual studio solution and project files
cmake -DCMAKE_INSTALL_PREFIX=..\buildInstall -DCMAKE_BUILD_TYPE=Debug ..\
cmake --build . --target INSTALL
