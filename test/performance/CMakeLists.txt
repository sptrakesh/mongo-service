FILE(GLOB_RECURSE performance_SRCS *.cpp *.hpp *.h)
set(performance_SRCS ${performance_SRCS})

if (APPLE)
  execute_process(
    COMMAND brew --prefix hayai
    RESULT_VARIABLE BREW_HAYAI
    OUTPUT_VARIABLE BREW_HAYAI_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (BREW_HAYAI EQUAL 0 AND EXISTS "${BREW_HAYAI_PREFIX}")
    message(STATUS "Found hayai keg installed by Homebrew at ${BREW_HAYAI_PREFIX}")
    set(HAYAI_ROOT_DIR "${BREW_HAYAI_PREFIX}/")
    set(HAYAI_INCLUDES "${BREW_HAYAI_PREFIX}/include")
    set(HAYAI_LIBRARIES "${BREW_HAYAI_PREFIX}/lib")
    set(HAYAI_LIBRARY "${BREW_HAYAI_PREFIX}/lib/libhayai_main.a")
    include_directories(${HAYAI_INCLUDES})
  endif()
endif()
find_package(hayai REQUIRED)

add_executable(performance ${performance_SRCS})

target_link_libraries(performance
  PRIVATE ${HAYAI_LIBRARY} Boost::boost mongo::mongocxx_static
)

add_test(NAME performance COMMAND performance)
install(TARGETS performance DESTINATION bin)
