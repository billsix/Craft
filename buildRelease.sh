mkdir build
mkdir buildInstall
cd build
cmake -DCMAKE_INSTALL_PREFIX=../buildInstall ../
cmake --build  . --target all --config Release
cmake --build  . --target install --config Release
cd ../buildInstall
./bin/craft
