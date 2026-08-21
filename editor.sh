cmake -B build
sudo cmake --build build -j${nproc}
sudo mv ./build/editor/RaphEditor ./
./RaphEditor
