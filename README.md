### License

This project is not licensed for public use.

All rights reserved. You may not use, copy, modify, merge, publish, distribute, sublicense, or sell any part of this project without explicit permission from the author.

# TrueShot

Tactical FPS 5V5 in C++.

# Development

## Programs to install

### Downalod Visual Studio
https://visualstudio.microsoft.com/

During installation, make sur to tick the case: "Desktop development with C++"

Restart your PC.

### Install gcc/mingw
https://code.visualstudio.com/docs/cpp/config-mingw

Make sur to do all the steps, including envrionement steps.
Restart all your terminals, visual studio and others.

### Install cmake
https://cmake.org/download/

Choose "Windows x64 Installer"
IMPORTANT, make sure to tick: "Add CMake to system PATH"

### Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

On Windows, search "Edit environment variables for your account"
On PATH variable, click edit
Add C:\Users\alexa\vcpkg
Click OK
Click OK again.
Restart all your terminals, visual studio and others.

## Clone and dependencies

git clone git@github.com:SachsA/TrueShot.git

vcpkg install glfw3 glm glad[gl-api-33]

## Build and execute

mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .

For example mine is: -DCMAKE_TOOLCHAIN_FILE=C:/Users/alexa/vcpkg/scripts/buildsystems/vcpkg.cmake

Then, execute your .exe generated in build folder.

# Socials

### X / Twitter

https://x.com/TrueShotGame

### Youtube

https://www.youtube.com/channel/UC0cwNEc0hI77cCWwX7EaNTg

### Twitch

https://www.twitch.tv/trueshotgame
