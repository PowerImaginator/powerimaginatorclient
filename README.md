# PowerImaginator Client

PowerImaginator creates 3D environments by stitching together content from image generation and depth estimation models. This repository contains the desktop app, which provides a UI to control generation and implements the core 3D stitching logic. You will also need to run the [PowerImaginator server](https://github.com/PowerImaginator/powerimaginatorserver), which is written in Python and exposes the image generation and depth estimation models via HTTP endpoints.

## Quick Start

For a quick demo/tutorial video and instructions on setting up PowerImaginator, please visit [powerimaginator.com](https://www.powerimaginator.com).

## Development

If you want to modify and experiment with the PowerImaginator source code (we hope so!), then this section is for you.

### Install dependencies

#### Windows

On Windows devices, please install [CMake](https://cmake.org/), [Visual Studio](https://visualstudio.microsoft.com/) with C++ support activated, and [vcpkg](https://vcpkg.io/) (C++ and vcpkg features can be selected during the Visual Studio installation process). 

### macOS

On macOS devices, please install [Xcode](https://developer.apple.com/xcode/) and [Homebrew](https://brew.sh/). Then use Homebrew to install the following packages: `brew install cmake ninja glfw vtk openssl`

### Building

We recommend using [VS Code](https://code.visualstudio.com/) with the [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack), or another compatible editor (e.g. Cursor with Anysphere C/C++). This repo already has a `.vscode` folder, and a debug configuration should appear if you go to the "Run and Debug" tab in your editor.

If you would like to build manually from the command line, instructions are provided below (you may also want to refer to the [.github/workflows](.github/workflows/) directory):

#### Windows

Note: You may need to use "Developer Command Prompt for VS 2022" (or equivalent for your Visual Studio installation) to run these commands.

```batch
git clone --recursive https://github.com/PowerImaginator/powerimaginatorclient.git
cd powerimaginatorclient
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
cmake --build . --config Debug
```

#### macOS

```bash
git clone --recursive https://github.com/PowerImaginator/powerimaginatorclient.git
cd powerimaginatorclient
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug
```

If you run into any issues, feel free to join our [Discord server](https://discord.gg/Vk3Wh5ZQYD) and we'll do our best to help you. Enjoy using PowerImaginator, and please submit a PR if you make any improvements!

## License

Apache License 2.0
