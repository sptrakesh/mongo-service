FILE(GLOB_RECURSE mongo-service_SRCS *.cpp *.cc *.hpp *.h)
set(mongo-service_SRCS ${mongo-service_SRCS})
add_executable(mongo-service ${mongo-service_SRCS} )

if (BUILD_TESTING)
  add_library(mongo-service-lib STATIC ${mongo-service_SRCS})
endif()

target_compile_definitions(mongo-service
  PRIVATE ${LIBMONGOCXX_STATIC_DEFINITIONS}
)

target_link_libraries(mongo-service
  PRIVATE ${LIBMONGOCXX_STATIC_LIBRARIES} Boost::boost ${OPENSSL_LIBRARIES} z
)

install(TARGETS mongo-service DESTINATION bin)
