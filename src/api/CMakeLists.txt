set(Target_Name "mongo-service-api")
file(GLOB_RECURSE mongo-service_SRCS *.cpp *.h)
add_library(${Target_Name} STATIC ${mongo-service_SRCS})
target_link_libraries(${Target_Name}
  PRIVATE mongo::mongocxx_static
)

if (UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif (UNIX)

set_target_properties(${Target_Name} PROPERTIES OUTPUT_NAME mongoserviceapi)
install(TARGETS ${Target_Name} DESTINATION lib)
install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/api
  DESTINATION include/mongo-service
  FILES_MATCHING PATTERN "*.h*")