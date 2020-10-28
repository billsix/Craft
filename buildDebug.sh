mkdir build
mkdir buildInstall
cd build
#-DENABLE_ONLY_RENDER_ONE_CHUNK=YES

# If you disable threads, the framerate skyrockets on old hardware.
#  On my 2011 mac mini, core 2 duo, with threads, I get 30fps, but
#  the world is created dynamically and smoothly as the player
#  walks around.
#  With threads disabled, I get 60fps, at the experse of the world
#  being built too slowly, and the user can end up falling through
#  empty spaces, getting stuck.
# -DENABLE_NO_THREADS=YES

# use vulkan by doing the following
# -DENABLE_VULKAN_RENDERER=YES -DENABLE_OPENGL_CORE_PROFILE_RENDERER=NO
cmake -DCMAKE_INSTALL_PREFIX=../buildInstall -DENABLE_VULKAN_RENDERER=NO -DENABLE_OPENGL_CORE_PROFILE_RENDERER=YES -DCMAKE_BUILD_TYPE=Debug ../
cmake --build  . --target all
cmake --build  . --target install
cd ../buildInstall
./bin/craft
