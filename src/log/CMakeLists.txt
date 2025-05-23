set(Target_Name "nanolog")

file(GLOB_RECURSE log_SRCS *.cpp *.h)
add_library(${Target_Name} STATIC ${log_SRCS})
add_library(nanolog::nanolog ALIAS ${Target_Name})
target_compile_definitions(${Target_Name} PRIVATE _FORTIFY_SOURCE=2)
target_compile_features(${Target_Name} INTERFACE cxx_std_23)

if (USE_MONGOCXX_CONFIG)
  find_package(bsoncxx QUIET)
  if (bsoncxx_FOUND)
    message(STATUS "Adding BSON support to Nanolog")
    add_compile_definitions(WITH_BSON_SUPPORT)
    target_link_libraries(${Target_Name} PRIVATE mongo::bsoncxx_static)
    target_link_libraries(${Target_Name} INTERFACE mongo::bsoncxx_static)
  endif (bsoncxx_FOUND)
else ()
  message(STATUS "Adding BSON support to Nanolog")
  add_compile_definitions(WITH_BSON_SUPPORT)
  target_include_directories(${Target_Name} PRIVATE /opt/local/include/bsoncxx/v_noabi)
  target_include_directories(${Target_Name} INTERFACE /opt/local/include/bsoncxx/v_noabi)
  target_link_libraries(${Target_Name} PRIVATE bson2 bsoncxx-static)
  target_link_libraries(${Target_Name} INTERFACE bson2 bsoncxx-static)
endif (USE_MONGOCXX_CONFIG)

if (UNIX AND NOT APPLE)
  target_link_libraries(${Target_Name} PRIVATE stdc++exp)
  target_link_libraries(${Target_Name} INTERFACE stdc++exp)
endif (UNIX AND NOT APPLE)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Debug build, not enabling IPO/LTO for NanoLog")
else()
  include(CheckIPOSupported)
  check_ipo_supported(RESULT result OUTPUT output LANGUAGES CXX)
  if (result)
    message(STATUS "IPO is supported: ${output} for NanoLog")
    set_property(TARGET ${Target_Name} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(STATUS "IPO is not supported: ${output} for NanoLog")
  endif(result)
endif (CMAKE_BUILD_TYPE MATCHES Debug)

target_include_directories(${Target_Name}
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS ${Target_Name} EXPORT NanoLogTargets DESTINATION lib)
install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/log
  DESTINATION include
  FILES_MATCHING PATTERN "*.h*")
install(EXPORT NanoLogTargets
    FILE NanoLogTargets.cmake
    NAMESPACE nanolog::
    DESTINATION lib/cmake/NanoLog
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "NanoLogConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion)

install(FILES "NanoLogConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/NanoLogConfigVersion.cmake" DESTINATION lib/cmake/NanoLog)
