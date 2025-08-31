# Verify cmake version
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# Create project
project(prueba_firma_ssh
    VERSION 1.0.0
    DESCRIPTION "A nice shell project!"
    LANGUAGES C
)

# Enable testing
include(CTest)

# Flags for compiling
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_FLAGS_DEBUG "-g3 -Wall -pedantic -Werror -Wextra -Wconversion")

# Include headers
include_directories(include)

# Find dependences
# cJSON sí se puede buscar con find_package
find_package(cJSON REQUIRED)

# Unity no tiene package config => usar FetchContent
include(FetchContent)
FetchContent_Declare(
  unity
  GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
  GIT_TAG v2.5.2   
)
FetchContent_MakeAvailable(unity)

# add_executable --> .exe/binary
# add_library --> .o/.so/-sll STATIC SHARED
add_executable(${PROJECT_NAME} src/hola.c)

# Link cJSON
target_link_libraries(${PROJECT_NAME} PRIVATE cjson::cjson)

# Incluir Unity 
target_include_directories(${PROJECT_NAME} PRIVATE ${unity_SOURCE_DIR}/src)
