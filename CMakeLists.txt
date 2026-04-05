
cmake_minimum_required(VERSION 4.3.1)
project(bai_yu_agent VERSION 1.0 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/json")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/boost")
if(LINUX)
    find_package(OpenSSL REQUIRED)
elseif(WIN32)
    message(STATUS "!!!暂未处理windows平台下的OpenSSL,做跨平台的时候注意!!!")
endif()
if(LINUX)
    message("Cur system is LINUX")
elseif(WIN32)
    message("Cur system is Windows")
else()
    message("-------Cur system isn't supported!-------")
endif()
# here library options!
set(isOpenAgent ON)
if(isOpenAgent)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/LLM")
endif()
add_executable(${PROJECT_NAME} main.cpp)
if(LINUX)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL)
endif() 
if(isOpenAgent)
    target_link_libraries(${PROJECT_NAME} PRIVATE baiyu::llm ) 
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/LLM)
endif()
install(
    EXPORT baiyuTargets
    FILE baiyuTargets.cmake 
    NAMESPACE baiyu::
    DESTINATION lib/cmake/baiyu
)