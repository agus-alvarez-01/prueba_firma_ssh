# Verify cmake version
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# Create project
project {
	"ShellProject"
	VERSION 1.0.0
	DESCTIPTION "A nice shell project!"
	LANGUEAGES C
}

# Enable testing
include(CTest)

# Flags for compiling
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_FLAGG_DEBUG "-g3 -Wall -pedantic -Werror - Wextra -Wconversion")

# Include headers
include_directories(include)

# Find dependences
find_package(cJASON REQUIRED)
find_package(unity REQUIRED)

# add_executable --> .exe/binary
# add_library --> .o/.so/-sll STATIC SHARED
add_executable(${PROJECT_NAME} src/main.c)

target_link_library(${PROJECT_NAME} PRIVATE cjson::cjson)
