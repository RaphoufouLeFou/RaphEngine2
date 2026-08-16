python3 shaderUtils.py
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=RelWithDebInfo 
sudo cmake --build build --target install -j${nproc}
sudo ldconfig
