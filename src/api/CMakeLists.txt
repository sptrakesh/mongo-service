set(Target_Name "api")
file(GLOB_RECURSE mongo-service_SRCS *.cpp *.h)
add_library(${Target_Name} STATIC ${mongo-service_SRCS})
add_library(mongo-service::api ALIAS ${Target_Name})

if (USE_MONGOCXX_CONFIG)
  target_link_libraries(${Target_Name} PRIVATE mongo::mongocxx_static)
  target_link_libraries(${Target_Name} INTERFACE nanolog common ilp Boost::boost Boost::json mongo::bsoncxx_static)
else ()
  target_link_libraries(${Target_Name} PRIVATE bson2 bsoncxx-static)
  target_link_libraries(${Target_Name} INTERFACE nanolog common ilp Boost::boost Boost::json bson2 bsoncxx-static)
  target_include_directories(${Target_Name} PRIVATE /opt/local/include/bsoncxx/v_noabi)
  target_include_directories(${Target_Name} INTERFACE /opt/local/include/bsoncxx/v_noabi)
endif (USE_MONGOCXX_CONFIG)
target_compile_definitions(${Target_Name} PRIVATE _FORTIFY_SOURCE=2)
target_compile_features(${Target_Name} INTERFACE cxx_std_23)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Debug build, not enabling IPO/LTO for MongoServiceApi")
else()
  include(CheckIPOSupported)
  check_ipo_supported(RESULT result OUTPUT output LANGUAGES CXX)
  if (result)
    message(STATUS "IPO is supported: ${output} for api")
    set_property(TARGET ${Target_Name} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=auto")
  else()
    message(STATUS "IPO is not supported: ${output} for api")
  endif(result)
endif (CMAKE_BUILD_TYPE MATCHES Debug)

target_include_directories(${Target_Name}
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

if (UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif (UNIX)

set_target_properties(${Target_Name} PROPERTIES OUTPUT_NAME mongoserviceapi)

install(TARGETS ${Target_Name} EXPORT MongoServiceApiTargets DESTINATION lib)
install(TARGETS ${Target_Name} DESTINATION lib)
install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/api
  DESTINATION include/mongo-service
  FILES_MATCHING PATTERN "*.h*")
install(EXPORT MongoServiceApiTargets
    FILE MongoServiceApiTargets.cmake
    NAMESPACE mongo-service::
    DESTINATION lib/cmake/MongoService
)
