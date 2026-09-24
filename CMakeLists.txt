project(shell)
cmake_minimum_required(VERSION 3.0)

OPTION(BUILD_TESTS "Build the tests" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

IF(NOT SRC_LIST)
    set(SRC_LIST shell.cpp)
ENDIF()
add_library (${PROJECT_NAME}lib ${SRC_LIST})

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}lib)

// Source - https://stackoverflow.com/a/48867234
// Posted by LMX, modified by community. See post 'Timeline' for change history
// Retrieved 2026-09-24, License - CC BY-SA 3.0

include(ExternalProject)
find_package(Git REQUIRED)

# Build googletest
ExternalProject_Add(
    googletest
    PREFIX "vendor/gtm"
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    GIT_TAG release-1.8.0
    TIMEOUT 10
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)

# Build gtest
ExternalProject_Add(
    gtest_src
    PREFIX "vendor/gtm"
    SOURCE_DIR "vendor/gtm/src/googletest/googletest"
    INSTALL_DIR "vendor/gtm/gtest"
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/vendor/gtm/gtest
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    DOWNLOAD_COMMAND ""
    UPDATE_COMMAND ""
)

# Prepare gtest
ExternalProject_Get_Property(gtest_src install_dir)
set(GTEST_INCLUDE_DIR ${install_dir}/include)
set(GTEST_LIBRARY_PATH ${install_dir}/lib/libgtest.a)
file(MAKE_DIRECTORY ${GTEST_INCLUDE_DIR})
add_library(gtest STATIC IMPORTED)
set_property(TARGET gtest PROPERTY IMPORTED_LOCATION  ${GTEST_LIBRARY_PATH})
set_property(TARGET gtest APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIR})

add_dependencies(gtest_src googletest)
add_dependencies(gtest gtest_src)



if (BUILD_TESTS)
  add_subdirectory(ext/gtest)
  include_directories(${GTEST_INCLUDE_DIRS})
  set (test test.cpp)
  file(GLOB_RECURSE UNITTESTS *.test.cpp)
  add_executable (${PROJECT_NAME}test ${test} ${UNITTESTS})
  target_link_libraries(${PROJECT_NAME}test ${PROJECT_NAME}lib)
  add_dependencies(${PROJECT_NAME}test ${PROJECT_NAME} googletest)
  target_link_libraries(${PROJECT_NAME}test ${GTEST_LIBS_DIR}/libgtest.a ${GTEST_LIBS_DIR}/libgtest_main.a)

  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
      find_package(Threads)
      target_link_libraries(${PROJECT_NAME}test ${CMAKE_THREAD_LIBS_INIT})
  endif()
endif()

