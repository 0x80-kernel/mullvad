cmake_minimum_required(VERSION 3.10)
project(YourProject)

find_package(cpr REQUIRED)
find_package(fmt REQUIRED)
find_package(nlomann_json REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(fmt REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE cpr::cpr)
