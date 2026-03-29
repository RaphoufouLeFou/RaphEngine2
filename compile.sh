cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
mv build/editor/RaphEditor editor/
