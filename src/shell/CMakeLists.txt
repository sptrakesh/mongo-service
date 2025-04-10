file(GLOB_RECURSE mongo-service_SRCS *.cpp *.h)
add_executable(mongo-service-shell ${mongo-service_SRCS} )

if (APPLE)
  target_link_libraries(mongo-service-shell PRIVATE api Boost::boost readline)
elseif(WIN32)
  target_link_libraries(mongo-service-shell PRIVATE api Boost::boost readline)
else()
  target_link_libraries(mongo-service-shell PRIVATE api Boost::boost readline pthread)
endif()

set_target_properties(mongo-service-shell PROPERTIES OUTPUT_NAME mongoservicesh)
install(TARGETS mongo-service-shell DESTINATION bin)
