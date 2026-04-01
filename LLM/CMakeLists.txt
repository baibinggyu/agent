add_library(llm SHARED 
    "${CMAKE_CURRENT_SOURCE_DIR}/llm.cpp"
)
target_include_directories(llm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

set_target_properties(llm PROPERTIES
    VERSION 1.0
    SOVERSION 1
)
add_library(baiyu::llm ALIAS llm)
