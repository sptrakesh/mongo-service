cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

if (POLICY CMP0069)
  cmake_policy(SET CMP0069 NEW)
endif(POLICY CMP0069)
if (POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif(POLICY CMP0079)
if (POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

file(STRINGS "docker/env.sh" ver REGEX "VERSION=([0-9]*)")
string(REGEX REPLACE "VERSION='" "" project_version ${ver})
string(REGEX REPLACE "'" "" project_version ${project_version})
message("Project version: ${project_version}")

project(mongo-service LANGUAGES CXX VERSION ${project_version})

if (WIN32)
  if (POLICY CMP0111)
    cmake_policy(SET CMP0111 NEW)
  endif (POLICY CMP0111)
  if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW) 
  endif()
  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "/EHsc /await:strict /std:c++20 /utf-8")
    add_compile_definitions(_WIN32_WINNT=0x0A000007)
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif(WIN32)

# Enable C++23 features
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Find includes in the build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

if (UNIX AND NOT APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")

  if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64" )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=mold")
    message(STATUS "Enabling mold linker")
  elseif (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64" )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=mold")
    message(STATUS "Enabling mold linker")
  endif (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64" )
endif (UNIX AND NOT APPLE)

if (UNIX)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME ON)
  set(Boost_USE_MULTITHREADED ON)
  set(OPENSSL_USE_STATIC_LIBS TRUE)
endif (UNIX)

find_package(Boost REQUIRED COMPONENTS json)

if (APPLE)
  execute_process(
    COMMAND brew --prefix OpenSSL
    RESULT_VARIABLE BREW_OPENSSL
    OUTPUT_VARIABLE BREW_OPENSSL_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (BREW_OPENSSL EQUAL 0 AND EXISTS "${BREW_OPENSSL_PREFIX}")
    message(STATUS "Found OpenSSL keg installed by Homebrew at ${BREW_OPENSSL_PREFIX}")
    list(APPEND CMAKE_PREFIX_PATH "${BREW_OPENSSL_PREFIX}/")
  endif()

  execute_process(
      COMMAND brew --prefix zstd
      RESULT_VARIABLE BREW_ZSTD
      OUTPUT_VARIABLE BREW_ZSTD_PREFIX
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (BREW_ZSTD EQUAL 0 AND EXISTS "${BREW_ZSTD_PREFIX}")
    message(STATUS "Found zstd keg installed by Homebrew at ${BREW_ZSTD_PREFIX}")
    list(APPEND CMAKE_PREFIX_PATH "${BREW_ZSTD_PREFIX}/")
  endif()

  execute_process(
    COMMAND brew --prefix jemalloc
    RESULT_VARIABLE BREW_JEMALLOC
    OUTPUT_VARIABLE BREW_JEMALLOC_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (BREW_JEMALLOC EQUAL 0 AND EXISTS "${BREW_JEMALLOC_PREFIX}")
    message(STATUS "Found OpenSSL keg installed by Homebrew at ${BREW_JEMALLOC_PREFIX}")
    set(JEMALLOC_ROOT_DIR "${BREW_JEMALLOC_PREFIX}/")
    set(JEMALLOC_LIBRARIES "${BREW_JEMALLOC_PREFIX}/lib")
    set(JEMALLOC_LIBRARY "${BREW_JEMALLOC_PREFIX}/lib/libjemalloc.dylib")
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-warning-option")
endif()
find_package(fmt REQUIRED)
find_package(mongocxx REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(range-v3 REQUIRED)
find_package(Threads)
find_package(ZLIB REQUIRED)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
endif()

if (UNIX AND NOT APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
endif()

include_directories(
  ${Boost_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
)

if (APPLE)
  set(CMAKE_MACOSX_RPATH 1)
  include_directories(
    /usr/local/boost/include
    /usr/local/fmt/include
    /usr/local/range-v3/include
  )
else()
  include_directories(
    /opt/local/include
  )
  link_directories(
    /opt/local/lib
  )
endif()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX /opt/spt)
endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

add_subdirectory(src/ilp)
add_subdirectory(src/log)
add_subdirectory(src/common)
add_subdirectory(src/service)
add_subdirectory(src/api)
add_subdirectory(src/genoid)

if (BUILD_TESTING)
  if (UNIX)
    add_subdirectory(test/unit)
    add_subdirectory(test/integration)
    add_subdirectory(test/performance)
    add_subdirectory(test/client)
  else()
    add_subdirectory(test/unit)
    add_subdirectory(test/integration)
    add_subdirectory(test/client)
  endif(UNIX)
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "MongoServiceConfigVersion.cmake"
  VERSION ${mongoservice_VERSION}
  COMPATIBILITY AnyNewerVersion)

install(FILES "MongoServiceConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/MongoServiceConfigVersion.cmake" DESTINATION lib/cmake/MongoService)
