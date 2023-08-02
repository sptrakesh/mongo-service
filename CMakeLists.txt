cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
cmake_policy(SET CMP0069 NEW)
project(mongo-service)

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

# Enable C++20 features
set(CMAKE_CXX_STANDARD 20)
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
    set(OPENSSL_ROOT_DIR "${BREW_OPENSSL_PREFIX}/")
    set(OPENSSL_INCLUDE_DIR "${BREW_OPENSSL_PREFIX}/include")
    set(OPENSSL_LIBRARIES "${BREW_OPENSSL_PREFIX}/lib")
    set(OPENSSL_CRYPTO_LIBRARY "${BREW_OPENSSL_PREFIX}/lib/libcrypto.dylib")
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

  find_package(OpenSSL REQUIRED)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-warning-option")
endif()
find_package(libmongocxx-static REQUIRED)
find_package(fmt REQUIRED)
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
  ${LIBMONGOCXX_STATIC_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIR}
)

if (APPLE)
  set(CMAKE_MACOSX_RPATH 1)
  include_directories(
    /usr/local/fmt/include
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

