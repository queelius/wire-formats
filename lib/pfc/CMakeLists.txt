cmake_minimum_required(VERSION 3.15)
project(pfc VERSION 1.0.0 LANGUAGES CXX)

# Main interface library
add_library(pfc INTERFACE)

target_include_directories(pfc INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(pfc INTERFACE cxx_std_20)


include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS pfc EXPORT pfcTargets)
install(EXPORT pfcTargets
    FILE pfcConfig.cmake
    NAMESPACE pfc::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pfc
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/pfcConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/pfcConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/pfc"
)

# Find packages
find_package(Threads REQUIRED)

# --- Examples ---
add_executable(tutorial examples/tutorial.cpp)
target_link_libraries(tutorial PRIVATE pfc)
target_compile_options(tutorial PRIVATE -Wall -Wextra -O2)

# Advanced demo with parallel execution support
add_executable(advanced_demo examples/advanced_demo.cpp)
target_link_libraries(advanced_demo PRIVATE pfc Threads::Threads)
target_compile_options(advanced_demo PRIVATE -Wall -Wextra -O2)

# New codecs demo
add_executable(new_codecs_demo examples/new_codecs_demo.cpp)
target_link_libraries(new_codecs_demo PRIVATE pfc)
target_compile_options(new_codecs_demo PRIVATE -Wall -Wextra -O2)

# Misc examples (optional)
option(BUILD_MISC_EXAMPLES "Build miscellaneous examples" OFF)
if(BUILD_MISC_EXAMPLES)
    add_executable(simple_test examples/misc/simple_test.cpp)
    target_link_libraries(simple_test PRIVATE pfc)

    add_executable(examine_bytes examples/misc/examine_bytes.cpp)
    target_link_libraries(examine_bytes PRIVATE pfc)
endif()
# Link TBB if available for parallel execution
find_package(TBB QUIET)
if(TBB_FOUND)
    target_link_libraries(advanced_demo PRIVATE TBB::tbb)
    target_compile_definitions(advanced_demo PRIVATE HAS_TBB)
endif()

# --- Testing ---
enable_testing()

add_executable(pfc_tests tests/test_unified.cpp tests/catch_amalgamated.cpp)
target_link_libraries(pfc_tests PRIVATE pfc)
target_compile_options(pfc_tests PRIVATE -Wall -Wextra -O2)

# Advanced tests
add_executable(test_advanced tests/test_advanced.cpp tests/catch_amalgamated.cpp)
target_link_libraries(test_advanced PRIVATE pfc Threads::Threads)
target_compile_options(test_advanced PRIVATE -Wall -Wextra -O2)
if(TBB_FOUND)
    target_link_libraries(test_advanced PRIVATE TBB::tbb)
endif()

# Production features tests
add_executable(test_production tests/test_production.cpp tests/catch_amalgamated.cpp)
target_link_libraries(test_production PRIVATE pfc)
target_compile_options(test_production PRIVATE -Wall -Wextra -O2)

# Stream I/O and arithmetic coding tests
add_executable(test_stream_arithmetic tests/test_stream_arithmetic.cpp tests/catch_amalgamated.cpp)
target_link_libraries(test_stream_arithmetic PRIVATE pfc)
target_compile_options(test_stream_arithmetic PRIVATE -Wall -Wextra -O2)

# CRC checksum tests
add_executable(test_crc tests/test_crc.cpp tests/catch_amalgamated.cpp)
target_link_libraries(test_crc PRIVATE pfc)
target_compile_options(test_crc PRIVATE -Wall -Wextra -O2)

# New codecs tests (VByte, ExpGolomb, EliasOmega)
add_executable(test_new_codecs tests/test_new_codecs.cpp tests/catch_amalgamated.cpp)
target_link_libraries(test_new_codecs PRIVATE pfc)
target_compile_options(test_new_codecs PRIVATE -Wall -Wextra -O2)

# Succinct data structures tests
add_executable(test_succinct tests/test_succinct.cpp tests/catch_amalgamated.cpp)
target_link_libraries(test_succinct PRIVATE pfc)
target_compile_options(test_succinct PRIVATE -Wall -Wextra -O2)

include(CTest)
add_test(NAME pfc_tests COMMAND pfc_tests)
add_test(NAME test_advanced COMMAND test_advanced)
add_test(NAME test_production COMMAND test_production)
add_test(NAME test_stream_arithmetic COMMAND test_stream_arithmetic)
add_test(NAME test_crc COMMAND test_crc)
add_test(NAME test_new_codecs COMMAND test_new_codecs)
add_test(NAME test_succinct COMMAND test_succinct)
