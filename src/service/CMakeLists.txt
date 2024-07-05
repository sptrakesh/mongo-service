set(Target_Name "mongo-service")
FILE(GLOB_RECURSE mongo-service_SRCS *.cpp *.cc *.hpp *.h)
set(mongo-service_SRCS ${mongo-service_SRCS})
add_executable(${Target_Name} ${mongo-service_SRCS} )

target_compile_definitions(${Target_Name} PRIVATE _FORTIFY_SOURCE=2)

if (UNIX)
  if (APPLE)
    target_link_libraries(${Target_Name}
      PRIVATE common ilp mongo::mongocxx_static fmt::fmt ${OPENSSL_LIBRARIES} ${JEMALLOC_LIBRARY} ${ZSTD_LIBRARY} ZLIB::ZLIB
    )
  else()
    target_link_libraries(${Target_Name}
      PRIVATE common ilp mongo::mongocxx_static fmt::fmt ${OPENSSL_LIBRARIES} jemalloc ZLIB::ZLIB
    )
  endif(APPLE)
else()
  target_link_libraries(${Target_Name}
    PRIVATE common ilp mongo::mongocxx_static fmt::fmt ${OPENSSL_LIBRARIES} ZLIB::ZLIB
  )
endif(UNIX)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Debug build, not enabling IPO/LTO for service")
else()
  include(CheckIPOSupported)
  check_ipo_supported(RESULT result OUTPUT output LANGUAGES CXX)
  if (result)
    message(STATUS "IPO is supported: ${output} for service")
    set_property(TARGET ${Target_Name} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=auto")
  else()
    message(STATUS "IPO is not supported: ${output} for service")
  endif(result)
endif (CMAKE_BUILD_TYPE MATCHES Debug)

install(TARGETS ${Target_Name} DESTINATION bin)
