set(Target_Name "mongo-service")
FILE(GLOB_RECURSE mongo-service_SRCS *.cpp *.cc *.hpp *.h)
set(mongo-service_SRCS ${mongo-service_SRCS})
add_executable(${Target_Name} ${mongo-service_SRCS} )

target_compile_definitions(${Target_Name}
  PRIVATE _FORTIFY_SOURCE=2
)


if (UNIX)
  if (APPLE)
    target_link_libraries(${Target_Name}
      PRIVATE nanolog mongo-service-common ilp mongo::mongocxx_static Boost::boost Boost::json fmt::fmt ${OPENSSL_LIBRARIES} ${JEMALLOC_LIBRARY} ${ZSTD_LIBRARY} ZLIB::ZLIB
    )
  else()
    target_link_libraries(${Target_Name}
      PRIVATE nanolog mongo-service-common ilp mongo::mongocxx_static Boost::boost Boost::json fmt::fmt ${OPENSSL_LIBRARIES} jemalloc ZLIB::ZLIB
    )
  endif(APPLE)
else()
  target_link_libraries(${Target_Name}
    PRIVATE nanolog mongo-service-common ilp mongo::mongocxx_static Boost::boost Boost::json fmt::fmt ${OPENSSL_LIBRARIES} ZLIB::ZLIB
  )
endif(UNIX)

install(TARGETS ${Target_Name} DESTINATION bin)
