# RaphEngine2
A home-made game engine in c++

## How to build

To build the shared library, go to the root folder, and execute
```shell
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
sudo cmake --build build --target install
sudo ldconfig
```

To build AND export the shared library, go to the root folder, and execute
```shell
cmake -B build
cmake --build build --target RaphEngine2
```
And then the .so file will be in `./build/RaphEngine2.so`

To build the editor, go to the root folder, and execute
```shell
cmake -B build
cmake --build build
mv ./build/editor/RaphEditor ./
```

You can start the editor with `./RaphEditor`
