cmake_minimum_required(VERSION 3.16)
project(trueshot_netcode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(unofficial-enet CONFIG REQUIRED)

# glm is used by net_sim.h (the shared simulation) and lag_compensation.cpp
# (rewind geometry). Both ship as part of this library, so we declare the
# dependency here rather than relying on the main TrueShot target's link
# line — otherwise `trueshot_server` alone wouldn't find glm.
find_package(glm CONFIG REQUIRED)

# ---------------------------------------------------------------- library
# The shared protocol + the authoritative Server, built as a static library
# so that BOTH the standalone `trueshot_server` binary and the game client
# (listen-server mode, Phase 2) link the exact same simulation.
#
# `server.cpp` lives here and ONLY here; `main_server.cpp` provides the
# executable's entry point separately. That way server.cpp is never
# compiled twice and we never risk an ODR violation. See docs/adr/0003.
add_library(trueshot_netcode STATIC
    src/net_common.cpp
    src/server.cpp
    src/lag_compensation.cpp
)

target_include_directories(trueshot_netcode
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(trueshot_netcode
    PUBLIC unofficial::enet::enet
           glm::glm
)

# ENet on Windows depends on the Winsock + Windows Multimedia APIs
# (WSAStartup / socket / htons + timeGetTime), but the vcpkg config
# doesn't propagate those transitive system libs. Link them ourselves.
if(WIN32)
    target_link_libraries(trueshot_netcode PUBLIC ws2_32 winmm)

    # ENet's enetwin32.h pulls <windows.h>, which leaks the legacy
    # min/max macros. They collide with std::min/std::max and produce
    # cryptic C2589/C2059 errors at every `std::clamp` or `std::max`
    # call site. NOMINMAX disables them for any TU that pulls the
    # netcode headers (and so transitively reaches windows.h).
    target_compile_definitions(trueshot_netcode PUBLIC NOMINMAX)
endif()

# ------------------------------------------------------------- executable
# Standalone dedicated server: a thin main() over the library.
add_executable(trueshot_server src/main_server.cpp)
target_link_libraries(trueshot_server PRIVATE trueshot_netcode)
