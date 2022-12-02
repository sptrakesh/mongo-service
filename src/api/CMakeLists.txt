file(GLOB_RECURSE mongo-service_SRCS *.cpp *.h)
add_library(mongo-service-api STATIC ${mongo-service_SRCS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
set_target_properties(mongo-service-api PROPERTIES OUTPUT_NAME mongoserviceapi)
install(TARGETS mongo-service-api DESTINATION lib)
install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/api
  DESTINATION include/mongo-service
  FILES_MATCHING PATTERN "*.h*")