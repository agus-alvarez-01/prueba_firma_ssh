# Verify cmake version
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# Create project
project(SO1-TPs-2025
    VERSION 1.0.0
    DESCRIPTION "SO1-TPs-2025 project!"
    LANGUAGES C
)

# Enable testing
include(CTest)

# Flags for compiling
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_FLAGS_DEBUG "-g3 -Wall -pedantic -Werror -Wextra -Wconversion")

# Include headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Include subdirectories, add_subdirectory interpreta el CMakeLists.txt que esta en la ruta que le paso
add_subdirectory(prometheus-client-c/prom) # tiene que ir hasta donde estra el archivo CMakeLists.txt
add_subdirectory(prometheus-client-c/promhttp)

# add_executable --> .exe/binary
# add_library --> .o/.so/-sll STATIC SHARED
add_executable(${PROJECT_NAME}
	src/prom_metrics.c
	src/get_metrics.c
	src/main.c
)

# Incluyo los headers de prometheus-client-c
target_include_directories(${PROJECT_NAME} PRIVATE prometheus-client-c/prom/include)
target_include_directories(${PROJECT_NAME} PRIVATE prometheus-client-c/promhttp/include)

# Linkeo las librerias de prom y promhttp
target_link_libraries(${PROJECT_NAME} PRIVATE prom)
target_link_libraries(${PROJECT_NAME} PRIVATE promhttp)

# Find dependences
find_package(cJSON REQUIRED)
find_package(unity REQUIRED)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE cjson::cjson)
target_link_libraries(${PROJECT_NAME} PRIVATE unity::unity)

# Tests
# Carpeta de salida para los tests
set(TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Testing)

# Test get_metrics
add_executable(test_get_metrics
    tests/test_get_metrics.c
    src/get_metrics.c
)

set_target_properties(test_get_metrics PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
)

target_include_directories(test_get_metrics PRIVATE include)
target_link_libraries(test_get_metrics PRIVATE unity::unity)

add_test(
    NAME test_get_metrics
    COMMAND ${TEST_OUTPUT_DIR}/test_get_metrics
)

# Test prom_metrics
add_executable(test_prom_metrics
    tests/test_prom_metrics.c
    src/prom_metrics.c
)

set_target_properties(test_prom_metrics PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
)

target_include_directories(test_prom_metrics PRIVATE include)
target_link_libraries(test_prom_metrics PRIVATE unity::unity prom promhttp)

add_test(
    NAME test_prom_metrics
    COMMAND ${TEST_OUTPUT_DIR}/test_prom_metrics
)
