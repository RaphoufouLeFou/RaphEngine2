python3 shaderUtils.py
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
sudo cmake --build build --target install -j${nproc}
sudo ldconfig
