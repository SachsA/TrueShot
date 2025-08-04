cmake_minimum_required(VERSION 3.16)
project(TrueShot)

set(CMAKE_CXX_STANDARD 17)

# Find essential packages
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Find include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Executable
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/player_controller.cpp
    src/fps_camera.cpp
    src/shader.cpp
    src/weapon_system.cpp
    src/audio_system.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    glfw 
    OpenGL::GL 
    glad::glad
    glm::glm
)

# Import shaders
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)
