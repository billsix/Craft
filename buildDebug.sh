mkdir build
mkdir buildInstall
cd build
#-DENABLE_ONLY_RENDER_ONE_CHUNK=YES
cmake -DCMAKE_INSTALL_PREFIX=../buildInstall -DCMAKE_BUILD_TYPE=Debug ../
cmake --build  . --target all
cmake --build  . --target install
cd ../buildInstall
./bin/craft
