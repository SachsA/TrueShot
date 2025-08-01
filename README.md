# 🎯 TrueShot

**Tactical 5v5 FPS built in C++**

---

## 📜 License

This project is **not licensed for public use.**

> **All rights reserved.**\
> You may **not** use, copy, modify, merge, publish, distribute, sublicense, or sell any part of this project without explicit permission from the author.

---

## 🛠️ Development Setup

**TrueShot can be dev on both Windows and Mac.**

Follow these steps to get the project up and running on Windows/Mac.

### 1. Install Visual Studio (ONLY WINDOWS USERS)

🔗 [Download Visual Studio](https://visualstudio.microsoft.com/)

- During installation, make sure to **check** the workload:
  > ✅ **Desktop development with C++**
- After installation, **restart your PC**.

---

### 2. Install GCC / MinGW (ONLY WINDOWS USERS)

🔗 [Follow this guide for MinGW setup](https://code.visualstudio.com/docs/cpp/config-mingw)

- Be sure to complete **all steps**, including setting up the environment variables.
- **Restart all terminals**, Visual Studio, and any open IDEs after setup.

---

### 3. Install CMake

- Windows:

🔗 [Download CMake](https://cmake.org/download/)

- Choose: **Windows x64 Installer**
- **Important**: During installation, check the option:
  > ✅ **Add CMake to system PATH**

- Mac:

```bash
brew install cmake
```

**Do not forget to add cmake to your PATH.**

---

### 4. Install vcpkg

- Windows:

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

- Mac:

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
```

**Add vcpkg to your system PATH:**

- Windows:

1. Open **"Edit environment variables for your account"**
2. Select the `PATH` variable and click **Edit**
3. Add the path to your `vcpkg` folder, e.g.:
   ```
   C:\Users\alexa\vcpkg
   ```
4. Click **OK** to confirm changes
5. **Restart all terminals**, Visual Studio, and other tools

- Mac:

```bash
nano ~/.zshrc
export PATH="/Users/asachs/vcpkg:$PATH"
source ~/.zshrc
```

---

## 📦 Clone and Install Dependencies

```bash
git clone git@github.com:SachsA/TrueShot.git
cd TrueShot
vcpkg install glfw3 glm glad[gl-api-33]
```

---

## ⚙️ Build and Run TrueShot

### Automatically

- Windows:

```bash
.\RunTrueShot.bat
```

- Mac:

```bash
./RunTrueShot.sh
```

---

### Manually

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Debug
```

> 📝 Example:
>
> ```
> -DCMAKE_TOOLCHAIN_FILE=C:/Users/alexa/vcpkg/scripts/buildsystems/vcpkg.cmake
> ```

- Windows:

After building, run the generated `.exe` inside the `build/Debug` folder:

```bash
cd Debug
TrueShot.exe
```

- Mac:

```bash
./TrueShot
```

---

## 🌐 Socials

Stay connected and follow the game's development!

- 🦆 **X / Twitter**\
  [https://x.com/TrueShotGame](https://x.com/TrueShotGame)

- 📺 **YouTube**\
  [https://www.youtube.com/channel/UC0cwNEc0hI77cCWwX7EaNTg](https://www.youtube.com/channel/UC0cwNEc0hI77cCWwX7EaNTg)

- 🎮 **Twitch**\
  [https://www.twitch.tv/trueshotgame](https://www.twitch.tv/trueshotgame)

---
