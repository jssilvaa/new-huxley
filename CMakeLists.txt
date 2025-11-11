cmake_minimum_required(VERSION 3.10)
project(HuxleyMessagingHub)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

add_executable(huxley ${SOURCES})
target_link_libraries(huxley PRIVATE SQLite::SQLite3 Threads::Threads)
