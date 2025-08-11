cmake_minimum_required(VERSION 3.15)
project(trueshot_network)

# ENet via vcpkg
find_package(unofficial-enet CONFIG REQUIRED)

file(GLOB NETWORK_HEADERS include/Network/*.h)
file(GLOB NETWORK_SOURCES src/*.cpp)

add_library(trueshot_network ${NETWORK_HEADERS} ${NETWORK_SOURCES})
target_include_directories(trueshot_network PUBLIC include)
target_link_libraries(trueshot_network PUBLIC unofficial::enet::enet)

# Sample server executable
add_executable(trueshot_server src/Server.cpp src/ENetWrapper.cpp src/NetCommon.cpp)
target_include_directories(trueshot_server PRIVATE include)
target_link_libraries(trueshot_server PRIVATE trueshot_network)

# Sample client executable
add_executable(trueshot_client src/Client.cpp src/ENetWrapper.cpp src/NetCommon.cpp)
target_include_directories(trueshot_client PRIVATE include)
target_link_libraries(trueshot_client PRIVATE trueshot_network)
