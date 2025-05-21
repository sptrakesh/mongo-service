set(Target_Name "common")
file(GLOB_RECURSE mongo-service_SRCS *.cpp *.h)
add_library(${Target_Name} STATIC ${mongo-service_SRCS})
add_library(mongo-service::common ALIAS ${Target_Name})

if (USE_MONGOCXX_CONFIG)
  target_link_libraries(${Target_Name} PRIVATE mongo::bsoncxx_static)
  target_link_libraries(${Target_Name} INTERFACE nanolog Boost::boost Boost::json mongo::bsoncxx_static)
else ()
  target_link_libraries(${Target_Name} PRIVATE bson2 bsoncxx-static)
  target_link_libraries(${Target_Name} INTERFACE nanolog Boost::boost Boost::json bson2 bsoncxx-static)
  target_include_directories(${Target_Name} PRIVATE /opt/local/include/bsoncxx/v_noabi)
  target_include_directories(${Target_Name} INTERFACE /opt/local/include/bsoncxx/v_noabi)
endif (USE_MONGOCXX_CONFIG)
target_compile_definitions(${Target_Name} PRIVATE _FORTIFY_SOURCE=2)
target_compile_features(${Target_Name} INTERFACE cxx_std_23)
set_target_properties(${Target_Name} PROPERTIES OUTPUT_NAME mongoservicecommon)
target_include_directories(${Target_Name}
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS ${Target_Name} EXPORT MongoServiceCommonTargets DESTINATION lib)
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
  FILES_MATCHING PATTERN "clara.hpp")
install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/common/simdjson
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp")
install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/common/visit_struct
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp")
install (
    DIRECTORY ${CMAKE_SOURCE_DIR}/src/common/magic_enum
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp")
install(EXPORT MongoServiceCommonTargets
    FILE MongoServiceCommonTargets.cmake
    NAMESPACE mongo-service::
    DESTINATION lib/cmake/MongoService
)
