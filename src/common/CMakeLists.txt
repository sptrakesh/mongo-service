file(GLOB_RECURSE mongo-service_SRCS *.cpp *.h)
add_library(mongo-service-common STATIC ${mongo-service_SRCS})
set_target_properties(mongo-service-common PROPERTIES OUTPUT_NAME mongoservicecommon)
install(TARGETS mongo-service-common DESTINATION lib)
install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/common
  DESTINATION include/mongo-service
  FILES_MATCHING PATTERN "*.h*")
install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/common/util
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp")
install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/common/util
  DESTINATION include
  FILES_MATCHING PATTERN "clara.h")
install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/common/simdjson
  DESTINATION include
  FILES_MATCHING PATTERN "*.h")
install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/common/visit_struct
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp")
