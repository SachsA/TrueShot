cmake_minimum_required(VERSION 3.16)
project(trueshot_network LANGUAGES CXX)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(unofficial-enet CONFIG REQUIRED)

# glm is used by NetSim.h (shared sim) and LagCompensation.cpp (rewind
# geometry). Both ship as part of the network library so we declare the
# dep here rather than relying on the main TrueShot target's link line.
find_package(glm CONFIG REQUIRED)

# ---------------------------------------------------------------- library
# Support code + the production Server class go into the static library.
# Server.cpp's main() is guarded by `#ifdef TRUESHOT_SERVER`, so when
# we don't define that macro for the library target, only the class
# definition is compiled. The standalone `trueshot_server` executable
# compiles Server.cpp a second time with the macro on to get its main.
add_library(trueshot_network STATIC
    src/ENetWrapper.cpp
    src/NetCommon.cpp
    src/Server.cpp
    src/LagCompensation.cpp
)

target_include_directories(trueshot_network
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(trueshot_network
    PUBLIC unofficial::enet::enet
           glm::glm
)

# ENet on Windows depends on the Winsock + Windows Multimedia APIs
# (WSAStartup / socket / htons + timeGetTime), but the vcpkg config
# doesn't propagate those transitive system libs. Link them ourselves.
if(WIN32)
    target_link_libraries(trueshot_network PUBLIC ws2_32 winmm)

    # ENet's enetwin32.h pulls <windows.h>, which leaks the legacy
    # min/max macros. They collide with std::min/std::max and produce
    # cryptic C2589/C2059 errors at every `std::clamp` or `std::max`
    # call site. NOMINMAX disables them globally for any TU that pulls
    # the network headers (transitively reaches windows.h).
    target_compile_definitions(trueshot_network PUBLIC NOMINMAX)
endif()

# ---------------------------------------------------------------- executables
# Standalone server: just a main() linking against the library which
# contains the actual Server class. Avoids ODR violations from
# compiling Server.cpp twice.
add_executable(trueshot_server src/main_server.cpp)
target_link_libraries(trueshot_server PRIVATE trueshot_network)

# Client.cpp is still a self-contained prototype with its main()
# guarded by TRUESHOT_CLIENT. Phase 1.4 has moved the real client code
# into NetworkClient (game-side); this binary stays as a CI smoke test
# until we delete it in a later phase.
add_executable(trueshot_client src/Client.cpp)
target_compile_definitions(trueshot_client PRIVATE TRUESHOT_CLIENT)
target_link_libraries(trueshot_client PRIVATE trueshot_network)
