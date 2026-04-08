add_library(llm SHARED 
    "${CMAKE_CURRENT_SOURCE_DIR}/llm.cpp"
)
target_include_directories(llm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/json/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/spdlog>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(llm PRIVATE ${CMAKE_SOURCE_DIR}/json/lib/libjsoncpp.a)
set_target_properties(llm PROPERTIES
    VERSION 1.0
    SOVERSION 1
)
add_library(baiyu::llm ALIAS llm)
install(TARGETS llm 
    EXPORT baiyuTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/llm.h" 
    DESTINATION include
)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/boost"
    DESTINATION include
)
