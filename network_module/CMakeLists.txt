cmake_minimum_required(VERSION 3.16)
project(trueshot_network LANGUAGES CXX)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(unofficial-enet CONFIG REQUIRED)

# ---------------------------------------------------------------- library
# Only the support code (no main()) goes into the static library.
add_library(trueshot_network STATIC
    src/ENetWrapper.cpp
    src/NetCommon.cpp
)

target_include_directories(trueshot_network
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(trueshot_network
    PUBLIC unofficial::enet::enet
)

# ---------------------------------------------------------------- executables
# Server.cpp / Client.cpp each define a main(), but only when the matching
# TRUESHOT_SERVER / TRUESHOT_CLIENT macro is defined. Wire that in here so
# each executable has exactly one entry point.
add_executable(trueshot_server src/Server.cpp)
target_compile_definitions(trueshot_server PRIVATE TRUESHOT_SERVER)
target_link_libraries     (trueshot_server PRIVATE trueshot_network)

add_executable(trueshot_client src/Client.cpp)
target_compile_definitions(trueshot_client PRIVATE TRUESHOT_CLIENT)
target_link_libraries     (trueshot_client PRIVATE trueshot_network)
