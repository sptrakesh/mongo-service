FILE(GLOB_RECURSE mongo-service_SRCS *.cpp *.cc *.hpp *.h)
set(mongo-service_SRCS ${mongo-service_SRCS})
add_executable(mongo-service ${mongo-service_SRCS} )

target_compile_definitions(mongo-service
  PRIVATE ${LIBMONGOCXX_STATIC_DEFINITIONS}
)

if (APPLE)
  target_link_libraries(mongo-service
    PRIVATE nanolog mongo-service-common ilp ${LIBMONGOCXX_STATIC_LIBRARIES} Boost::boost Boost::json fmt::fmt ${OPENSSL_LIBRARIES} ${JEMALLOC_LIBRARY} z
  )
else()
  target_link_libraries(mongo-service
    PRIVATE nanolog mongo-service-common ilp ${LIBMONGOCXX_STATIC_LIBRARIES} Boost::boost Boost::json fmt::fmt ${OPENSSL_LIBRARIES} jemalloc z
  )
endif()

install(TARGETS mongo-service DESTINATION bin)
