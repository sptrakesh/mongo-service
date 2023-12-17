set(Target_Name "mongo-service-common")
file(GLOB_RECURSE mongo-service_SRCS *.cpp *.h)
add_library(${Target_Name} STATIC ${mongo-service_SRCS})
target_link_libraries(${Target_Name}
    PRIVATE mongo::mongocxx_static
)
set_target_properties(${Target_Name} PROPERTIES OUTPUT_NAME mongoservicecommon)
install(TARGETS ${Target_Name} DESTINATION lib)
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
