FILE(GLOB_RECURSE performance_SRCS *.cpp *.hpp *.h)
set(performance_SRCS ${performance_SRCS})
find_package(hayai REQUIRED)

add_executable(performance ${performance_SRCS})

target_compile_definitions(integration
  PRIVATE ${LIBMONGOCXX_STATIC_DEFINITIONS}
)

target_link_libraries(performance
  PRIVATE hayai_main Boost::boost ${LIBMONGOCXX_STATIC_LIBRARIES}
)

add_test(NAME performance COMMAND performance)
install(TARGETS performance DESTINATION bin)
