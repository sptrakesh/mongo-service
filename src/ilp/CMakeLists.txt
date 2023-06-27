file(GLOB_RECURSE ilp_SRCS *.cpp *.h)
add_library(ilp STATIC ${ilp_SRCS})
install(TARGETS ilp DESTINATION lib)

install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/ilp
  DESTINATION include
  FILES_MATCHING PATTERN "*.h*")
