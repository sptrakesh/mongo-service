Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.7.1
)

FetchContent_MakeAvailable(Catch2)

FILE(GLOB_RECURSE integration_SRCS *.cpp *.hpp *.h)
set(integration_SRCS ${integration_SRCS})
if (APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif (APPLE)
add_executable(integration ${integration_SRCS})

find_package(cpr REQUIRED)
target_compile_definitions(integration PRIVATE ${LIBMONGOCXX_STATIC_DEFINITIONS})

target_link_libraries(integration
  PRIVATE api ilp Catch2::Catch2 cpr::cpr ${OPENSSL_LIBRARIES} ZLIB::ZLIB)

add_test(NAME integration COMMAND integration)
install(TARGETS integration DESTINATION bin)
