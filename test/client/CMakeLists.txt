set(CMAKE_CXX_FLAGS "-DBOOST_ASIO_ENABLE_HANDLER_TRACKING")
FILE(GLOB_RECURSE client_SRCS *.cpp *.hpp *.h)
set(client_SRCS ${client_SRCS})
add_executable(client ${client_SRCS})

target_compile_definitions(client
  PRIVATE ${LIBMONGOCXX_STATIC_DEFINITIONS}
)

target_link_libraries(client
  PRIVATE nanolog mongo-service-common mongo-service-api ${LIBMONGOCXX_STATIC_LIBRARIES} Boost::boost ${OPENSSL_LIBRARIES} z
)

add_test(NAME client COMMAND client)
install(TARGETS client DESTINATION bin)
