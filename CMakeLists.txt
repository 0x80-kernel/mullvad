cmake_minimum_required(VERSION 3.10)
project(MullvadChecker VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/mullvad.cpp
    src/utils.cpp
    src/ui.cpp
)

set(HEADERS
    include/curl_handle_manager.hpp
    include/curl_header_list.hpp
    include/mullvad.hpp
    include/thread_safe_queue.hpp
    include/utils.hpp
    include/ui.hpp
)

add_executable(mullvad_checker ${SOURCES} ${HEADERS})

find_package(CURL REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(${CURL_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(mullvad_checker PRIVATE
    ${CURL_LIBRARIES}
    fmt::fmt
    nlohmann_json::nlohmann_json
    curl-impersonate-chrome
    pthread  
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mullvad_checker PRIVATE -Wall -Wextra)
elseif(MSVC)
    target_compile_options(mullvad_checker PRIVATE /W4)
endif()

install(TARGETS mullvad_checker
    RUNTIME DESTINATION bin
)
