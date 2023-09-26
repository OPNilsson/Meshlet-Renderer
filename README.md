# Jinsoku

## Installation on Windows
Currently tested on Windows 10, x64

Install Vulkan SDK from https://vulkan.lunarg.com/

Install vcpkg as described in https://vcpkg.io/en/getting-started.html

Install CMake from https://cmake.org/download/

Add vcpkg as a System Variable in System Properties->Advanced->Environment Variables->System variables
* Variable Name: VCPKG and Variable Value: the path to the vcpkg base directory

Add VCPKG_ROOT  as a user variable for CMake in System Properties->Advanced->Environment Variables->System variables
* Variable Name: VCPKG_ROOT  and Variable Value: the path to the vcpkg base directory

Then run the dependencies.bat file from the root directory:
```sh
.\dependencies.bat
```

Finally create a folder named build in the root directory and run the following command:

```sh
cmake -S ./src/ -B ./build/
```

If successful you can click on the solution in the build folder to run your Visual Studio project.
