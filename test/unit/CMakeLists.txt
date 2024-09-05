Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.7.0
)

FetchContent_MakeAvailable(Catch2)

add_compile_definitions(WITH_BSON_SUPPORT)
file(GLOB_RECURSE test_SRCS *.cpp *.h)
add_executable(unitTest ${test_SRCS} )

target_link_libraries(unitTest PRIVATE common ilp Catch2::Catch2 fmt::fmt)
