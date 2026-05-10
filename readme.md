![Screenshot](assets/screenshot.png)

Visualization of satellites using SGP4 propagation, OpenGL and C++.

To build the project:
```bash
# Install tools to speed up compilation
sudo apt install mold ccache libssl-dev

# Development build setup
cmake -S . -B build -G Ninja -D CMAKE_BUILD_TYPE=Debug -D CMAKE_C_COMPILER_LAUNCHER=ccache -D CMAKE_LINKER_TYPE=MOLD

# Release build setup
cmake -S . -B build -G Ninja -D CMAKE_BUILD_TYPE=Release
```
