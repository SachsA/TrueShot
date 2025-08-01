cmake_minimum_required(VERSION 3.16)
project(TrueShot)

set(CMAKE_CXX_STANDARD 17)

# Find packages
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Find include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Executable
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/shader.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} 
    glfw 
    OpenGL::GL 
    glad::glad
    glm::glm
)
