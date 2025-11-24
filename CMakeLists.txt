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
# ejecutable principal, shell script
add_executable(${PROJECT_NAME}
    src/flags_bash.c
	src/main.c
    src/shell_core.c
)
# ejecuatble para monitoring
add_executable(monitoring
    src/prom_metrics.c
	src/get_metrics.c
    src/monitoring.c
)

# Incluyo los headers de prometheus-client-c
target_include_directories(${PROJECT_NAME} PRIVATE prometheus-client-c/prom/include)
target_include_directories(${PROJECT_NAME} PRIVATE prometheus-client-c/promhttp/include)

# Linkeo las librerias de prom y promhttp
target_link_libraries(monitoring PRIVATE prom)
target_link_libraries(monitoring PRIVATE promhttp)

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

# # Test main / shell functions
# add_executable(test_main_funcs
#     tests/test_main_funcs.c
#     src/shell_core.c
# )

# set_target_properties(test_main_funcs PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
# )

# target_include_directories(test_main_funcs PRIVATE include)
# target_link_libraries(test_main_funcs PRIVATE unity::unity pthread)

# add_test(
#     NAME test_main_funcs
#     COMMAND ${TEST_OUTPUT_DIR}/test_main_funcs
# )

set_target_properties(test_prom_metrics PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
)

target_include_directories(test_prom_metrics PRIVATE include)
target_link_libraries(test_prom_metrics PRIVATE unity::unity prom promhttp)

add_test(
    NAME test_prom_metrics
    COMMAND ${TEST_OUTPUT_DIR}/test_prom_metrics
)

# # Test flags_bash (simple checks)
# add_executable(test_flags_bash_simple
#     tests/test_flags_bash_simple.c
# )

# set_target_properties(test_flags_bash_simple PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
# )

# target_include_directories(test_flags_bash_simple PRIVATE include)
# target_link_libraries(test_flags_bash_simple PRIVATE unity::unity)

# add_test(
#     NAME test_flags_bash_simple
#     COMMAND ${TEST_OUTPUT_DIR}/test_flags_bash_simple
# )

# # Test create_directory
# add_executable(test_create_directory
#     tests/test_create_directory.c
# )

# set_target_properties(test_create_directory PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
# )

# target_include_directories(test_create_directory PRIVATE include)
# target_link_libraries(test_create_directory PRIVATE unity::unity)

# add_test(
#     NAME test_create_directory
#     COMMAND ${TEST_OUTPUT_DIR}/test_create_directory
# )
# Flags para coverage, solo para los ejecutables
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -fprofile-arcs -ftest-coverage)
    target_link_options(${PROJECT_NAME} PRIVATE -fprofile-arcs -ftest-coverage)

    target_compile_options(test_get_metrics PRIVATE -fprofile-arcs -ftest-coverage)
    target_link_options(test_get_metrics PRIVATE -fprofile-arcs -ftest-coverage)

    target_compile_options(test_prom_metrics PRIVATE -fprofile-arcs -ftest-coverage)
    target_link_options(test_prom_metrics PRIVATE -fprofile-arcs -ftest-coverage)
endif()

# --- Limpieza automática de coverage ---
# para que no haya problemas con "different checksum"

add_custom_target(prebuild_clean_coverage
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
    COMMENT "Limpiando .gcda viejos antes de compilar"
)

add_dependencies(${PROJECT_NAME} prebuild_clean_coverage)
add_dependencies(test_get_metrics prebuild_clean_coverage)
add_dependencies(test_prom_metrics prebuild_clean_coverage)
