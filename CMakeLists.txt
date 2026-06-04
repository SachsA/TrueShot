cmake_minimum_required(VERSION 3.16)
project(TrueShot
    VERSION   0.2.0
    LANGUAGES CXX
    DESCRIPTION "Tactical 5v5 FPS in C++"
)

# ---------------------------------------------------------------- options
option(TRUESHOT_BUILD_NETWORK     "Build the network module (server/client)" ON)
option(TRUESHOT_WARNINGS_AS_ERRORS "Treat compiler warnings as errors"        OFF)

# ---------------------------------------------------------------- defaults
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# Default to Release when the user hasn't picked a build type (single-config gens).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

# Output directories — one place to find every binary.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ---------------------------------------------------------------- deps
find_package(glfw3            CONFIG REQUIRED)
find_package(OpenGL                  REQUIRED)
find_package(glad             CONFIG REQUIRED)
find_package(glm              CONFIG REQUIRED)
find_package(unofficial-enet  CONFIG REQUIRED)
find_package(imgui            CONFIG REQUIRED)

# ---------------------------------------------------------------- target
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/application.cpp
    src/renderer.cpp
    src/game_world.cpp
    src/target.cpp
    src/player_controller.cpp
    src/fps_camera.cpp
    src/shader.cpp
    src/weapon_system.cpp
    src/audio_system.cpp
    src/hud.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
    glad::glad
    glm::glm
    unofficial::enet::enet
    imgui::imgui
)

# ENet on Windows pulls in Winsock + winmm transitively. The vcpkg
# config doesn't propagate these, so we add them explicitly.
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 winmm)
endif()

# ---------------------------------------------------------------- warnings
# Goal: same effective strictness on every compiler.
#  * /W4 on MSVC is roughly equivalent to -Wall -Wextra on GCC/Clang.
#  * We silence "unused parameter" everywhere — overloaded interface
#    methods naturally have unused params and they don't indicate a bug.
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /permissive-
        /wd4100   # C4100 unreferenced formal parameter — matches GCC -Wno-unused-parameter
        /wd4458   # C4458 declaration hides class member — too noisy on getters/setters
        /wd4267   # C4267 size_t -> int narrowing — GCC ignores it under -Wno-conversion
        /wd4244   # C4244 same family (float -> int loss) — pervasive in audio/anim code
    )
    if(TRUESHOT_WARNINGS_AS_ERRORS)
        target_compile_options(${PROJECT_NAME} PRIVATE /WX)
    endif()
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter)
    if(TRUESHOT_WARNINGS_AS_ERRORS)
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    endif()
endif()

# ---------------------------------------------------------------- assets
# Copy shaders next to the produced binary so the game can find them at runtime.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMENT "Copying shaders to runtime directory"
    VERBATIM
)

# ---------------------------------------------------------------- network
if(TRUESHOT_BUILD_NETWORK)
    add_subdirectory(network_module)
endif()

# ---------------------------------------------------------------- summary
message(STATUS "")
message(STATUS "TrueShot ${PROJECT_VERSION} configured:")
message(STATUS "  Build type     : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler       : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Network module : ${TRUESHOT_BUILD_NETWORK}")
message(STATUS "  Werror         : ${TRUESHOT_WARNINGS_AS_ERRORS}")
message(STATUS "")
