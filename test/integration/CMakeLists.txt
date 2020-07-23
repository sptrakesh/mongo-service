FILE(GLOB_RECURSE integration_SRCS *.cpp *.hpp *.h)
set(integration_SRCS ${integration_SRCS})
add_executable(integration ${integration_SRCS})

target_compile_definitions(integration
  PRIVATE ${LIBMONGOCXX_STATIC_DEFINITIONS}
)

target_link_libraries(integration
  PRIVATE mongo-service-lib ${LIBMONGOCXX_STATIC_LIBRARIES} Boost::boost ${OPENSSL_LIBRARIES} z
)

add_test(NAME integration COMMAND integration)
install(TARGETS integration DESTINATION bin)
