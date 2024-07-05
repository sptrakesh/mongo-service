set(Target_Name "ilp")
file(GLOB_RECURSE ilp_SRCS *.cpp *.h)
add_library(${Target_Name} STATIC ${ilp_SRCS})
add_library(mongo-service::ilp ALIAS ${Target_Name})
install(TARGETS ilp DESTINATION lib)
target_compile_features(${Target_Name} INTERFACE cxx_std_23)
target_include_directories(${Target_Name}
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  ${Boost_INCLUDE_DIR}
)
target_link_libraries(${Target_Name} INTERFACE nanolog Boost::boost)

install(TARGETS ${Target_Name} EXPORT MongoServiceIlpTargets DESTINATION lib)
install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/ilp
  DESTINATION include
  FILES_MATCHING PATTERN "*.h*")
install(EXPORT MongoServiceIlpTargets
    FILE MongoServiceIlpTargets.cmake
    NAMESPACE mongo-service::
    DESTINATION lib/cmake/MongoService
)
