set(CMAKE_CXX_FLAGS "-DBOOST_ASIO_ENABLE_HANDLER_TRACKING")
FILE(GLOB_RECURSE client_SRCS *.cpp *.hpp *.h)
set(client_SRCS ${client_SRCS})
add_executable(client ${client_SRCS})

target_link_libraries(client PRIVATE api ${OPENSSL_LIBRARIES} ZLIB::ZLIB)
add_test(NAME client COMMAND client)
install(TARGETS client DESTINATION bin)
