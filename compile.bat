python3 shaderUtils.py
cmake -B build -G "Visual Studio 17 2022" -DCMAKE_INSTALL_PREFIX="C:/RaphEngine2Install"
cmake --build build --config Release --target RaphEngine2
cmake --install build --config Release