cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
)

find_package(PkgConfig)
pkg_check_modules(MIRAL miral REQUIRED)
find_package(GTest REQUIRED)
add_executable(miracle-wm-tests window_tree_test.cpp)

target_include_directories(miracle-wm-tests PUBLIC SYSTEM ${GTEST_INCLUDE_DIRS} ${MIRAL_INCLUDE_DIRS})
target_link_libraries(miracle-wm-tests GTest::gtest_main ${GTEST_LIBRARIES} ${MIRAL_LDFLAGS} pthread)
gtest_discover_tests(miracle-wm-tests)