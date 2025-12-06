cmake_minimum_required(VERSION 3.27)
project(snake-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(BOOST_ASIO_NO_DEPRECATED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-deprecated-declarations")

# Выбираем, какие модули snake-shared включать
set(VSHARED_ENABLE_UTILS ON  CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_LOGGING ON  CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_WEBSOCKET     ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_SERVICE_LOADER     ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_COROUTINE     ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_MYSQL     ON CACHE BOOL "" FORCE)
set(VSHARED_ENABLE_CRYPT     ON CACHE BOOL "" FORCE)


include_directories(
        src
        /usr/local/include
)

add_subdirectory(snake-shared)

file(GLOB_RECURSE SNAKE_SERVER_SOURCES
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
)

add_executable(snake-server
        ${SNAKE_SERVER_SOURCES}
)

# ОДНА строка – и все включённые модули автоматически прилетают
target_link_libraries(snake-server
        PRIVATE
        snake-shared::all
)

target_compile_features(snake-server PUBLIC cxx_std_23)